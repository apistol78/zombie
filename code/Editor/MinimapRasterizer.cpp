/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapRasterizer.h"

#include "Core/Io/FileSystem.h"
#include "Core/Io/IStream.h"
#include "Core/Log/Log.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Serialization/DeepClone.h"
#include "Core/Settings/PropertyString.h"
#include "Core/Thread/Acquire.h"
#include "Core/Thread/JobManager.h"
#include "Core/Thread/Semaphore.h"
#include "Database/Database.h"
#include "Database/Instance.h"
#include "Drawing/Filters/ScaleFilter.h"
#include "Drawing/Image.h"
#include "Drawing/PixelFormat.h"
#include "Editor/IPipelineCommon.h"
#include "Editor/IPipelineSettings.h"
#include "Editor/MinimapAttributesEntityComponentData.h"
#include "Editor/MinimapTextureAsset.h"
#include "Heightfield/Editor/HeightfieldAsset.h"
#include "Heightfield/Heightfield.h"
#include "Heightfield/HeightfieldFormat.h"
#include "Model/Model.h"
#include "Model/Operations/Triangulate.h"
#include "Render/Editor/Texture/TextureAsset.h"
#include "Scene/Editor/SceneAsset.h"
#include "Scene/Editor/SceneOperatorChain.h"
#include "Terrain/Editor/TerrainAsset.h"
#include "Terrain/OceanComponentData.h"
#include "Terrain/TerrainComponentData.h"
#include "World/Editor/IEntityReplicator.h"
#include "World/Editor/ResolveExternal.h"
#include "World/Editor/Traverser.h"
#include "World/EntityData.h"
#include "World/IEntityComponentData.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace zombie
{
namespace
{

class PipelineCommonTransformContext : public scene::ISceneOperator::TransformContext
{
public:
	explicit PipelineCommonTransformContext(editor::IPipelineCommon* pipelineCommon)
	:	m_pipelineCommon(pipelineCommon)
	{
	}

	virtual Ref< const ISerializable > getObjectReadOnly(const Guid& instanceGuid) const override final
	{
		return m_pipelineCommon->getObjectReadOnly(instanceGuid);
	}

	virtual db::Database* getSourceDatabase() const override final
	{
		return m_pipelineCommon->getSourceDatabase();
	}

	virtual world::World* getWorld() const override final
	{
		return nullptr;
	}

private:
	editor::IPipelineCommon* m_pipelineCommon;
};

const Vector4 c_sunDirection = Vector4(1.0f, -2.0f, -1.0f).normalized();
const float c_noHeight = -std::numeric_limits< float >::max();

struct MinimapTexture : public Object
{
	RefArray< const drawing::Image > mips;
	bool linear = false;
	bool alphaTest = false;
};

int32_t wrap(int32_t v, int32_t l)
{
	const int32_t c = v % l;
	return (c < 0) ? c + l : c;
}

Ref< MinimapTexture > createTexture(const drawing::Image* image, bool linear, bool alphaTest)
{
	Ref< MinimapTexture > texture = new MinimapTexture();
	texture->linear = linear;
	texture->alphaTest = alphaTest;
	texture->mips.push_back(image);

	for (int32_t width = image->getWidth(), height = image->getHeight(); width > 1 || height > 1;)
	{
		width = std::max(width / 2, 1);
		height = std::max(height / 2, 1);

		const drawing::ScaleFilter scaleFilter(width, height, drawing::ScaleFilter::MnAverage, drawing::ScaleFilter::MgLinear);
		Ref< drawing::Image > mip = texture->mips.back()->clone();
		mip->apply(&scaleFilter);
		texture->mips.push_back(mip);
	}

	return texture;
}

Ref< MinimapTexture > loadTexture(editor::IPipelineCommon* pipelineCommon, const std::wstring& assetPath, const Guid& textureId)
{
	Ref< const render::TextureAsset > textureAsset = pipelineCommon->getObjectReadOnly< render::TextureAsset >(textureId);
	if (!textureAsset)
		return nullptr;

	const Path filePath = FileSystem::getInstance().getAbsolutePath(Path(assetPath) + textureAsset->getFileName());
	Ref< IStream > file = FileSystem::getInstance().open(filePath, File::FmRead);
	if (!file)
		return nullptr;

	Ref< drawing::Image > image = drawing::Image::load(file, textureAsset->getFileName().getExtension());
	file->close();
	if (!image)
		return nullptr;

	return createTexture(
		image,
		textureAsset->m_output.m_assumeLinearGamma,
		image->getPixelFormat().getAlphaBits() > 0 && !textureAsset->m_output.m_ignoreAlpha
	);
}

void rasterizeModel(
	const model::Model* model,
	const RefArray< MinimapTexture >& materialTextures,
	const Transform& transform,
	const Aabb2& bounds,
	const Color4f& modelColor,
	float modelColorBlend,
	drawing::Image* image,
	AlignedVector< float >& heights)
{
	const int32_t width = image->getWidth();
	const int32_t height = image->getHeight();
	const float sx = width / bounds.getSize().x;
	const float sy = height / bounds.getSize().y;
	const auto& materials = model->getMaterials();

	for (const auto& polygon : model->getPolygons())
	{
		if (polygon.getVertexCount() != 3)
			continue;

		Vector4 p[3];
		Vector4 n[3];
		Vector2 sp[3];
		for (int32_t i = 0; i < 3; ++i)
		{
			const model::Vertex& vertex = model->getVertex(polygon.getVertex(i));
			p[i] = transform * model->getPosition(vertex.getPosition()).xyz1();
			n[i] = (vertex.getNormal() != model::c_InvalidIndex) ? transform * model->getNormal(vertex.getNormal()).xyz0() : Vector4::zero();
			sp[i] = Vector2((p[i].x() - bounds.mn.x) * sx, (bounds.mx.y - p[i].z()) * sy);
		}

		// Twice the signed area in pixels; faces seen edge-on cover nothing.
		const float area = (sp[1].x - sp[0].x) * (sp[2].y - sp[0].y) - (sp[2].x - sp[0].x) * (sp[1].y - sp[0].y);
		if (std::abs(area) <= std::numeric_limits< float >::epsilon())
			continue;

		// Culled as the renderer does, as seen from above; double sided materials are flattened into both windings when built.
		const Vector4 faceNormal = cross(p[2] - p[0], p[1] - p[0]);
		const bool doubleSided = polygon.getMaterial() < materials.size() && materials[polygon.getMaterial()].isDoubleSided();
		if (!doubleSided && faceNormal.y() <= 0.0f)
			continue;

		const int32_t x0 = std::max((int32_t)std::floor(std::min({ sp[0].x, sp[1].x, sp[2].x })), 0);
		const int32_t x1 = std::min((int32_t)std::ceil(std::max({ sp[0].x, sp[1].x, sp[2].x })), width - 1);
		const int32_t y0 = std::max((int32_t)std::floor(std::min({ sp[0].y, sp[1].y, sp[2].y })), 0);
		const int32_t y1 = std::min((int32_t)std::ceil(std::max({ sp[0].y, sp[1].y, sp[2].y })), height - 1);
		if (x0 > x1 || y0 > y1)
			continue;

		// Vertices without normals are shaded flat.
		for (int32_t i = 0; i < 3; ++i)
		{
			if (n[i].length2() <= 0.0f)
				n[i] = faceNormal.normalized();
		}

		Color4f color(1.0f, 1.0f, 1.0f, 1.0f);
		const MinimapTexture* texture = nullptr;
		Vector2 uv[3];
		int32_t lod = 0;

		if (polygon.getMaterial() < materials.size())
		{
			const model::Material& material = materials[polygon.getMaterial()];
			color = material.getColor().linear();

			if ((texture = materialTextures[polygon.getMaterial()]) != nullptr)
			{
				for (int32_t i = 0; i < 3; ++i)
				{
					const uint32_t texCoord = model->getVertex(polygon.getVertex(i)).getTexCoord(material.getDiffuseMap().channel);
					uv[i] = (texCoord != model::c_InvalidIndex) ? model->getTexCoord(texCoord) : Vector2::zero();
				}

				// Mip where a texel covers about a pixel.
				const drawing::Image* top = texture->mips.front();
				const float texelArea = std::abs((uv[1].x - uv[0].x) * (uv[2].y - uv[0].y) - (uv[2].x - uv[0].x) * (uv[1].y - uv[0].y)) * top->getWidth() * top->getHeight();
				lod = std::min((int32_t)(0.5f * std::log2(std::max(texelArea / std::abs(area), 1.0f))), (int32_t)texture->mips.size() - 1);
			}
		}

		const drawing::Image* mip = texture ? texture->mips[lod] : nullptr;
		const float invArea = 1.0f / area;

		for (int32_t y = y0; y <= y1; ++y)
		{
			for (int32_t x = x0; x <= x1; ++x)
			{
				const float px = x + 0.5f;
				const float py = y + 0.5f;

				const float w0 = ((sp[1].x - px) * (sp[2].y - py) - (sp[2].x - px) * (sp[1].y - py)) * invArea;
				const float w1 = ((sp[2].x - px) * (sp[0].y - py) - (sp[0].x - px) * (sp[2].y - py)) * invArea;
				const float w2 = 1.0f - w0 - w1;
				if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
					continue;

				const float h = p[0].y() * w0 + p[1].y() * w1 + p[2].y() * w2;
				float& topHeight = heights[x + y * width];
				if (h <= topHeight)
					continue;

				Color4f c = color;
				if (mip)
				{
					const Vector2 tc = uv[0] * w0 + uv[1] * w1 + uv[2] * w2;
					mip->getPixelUnsafe(
						wrap((int32_t)std::floor(tc.x * mip->getWidth()), mip->getWidth()),
						wrap((int32_t)std::floor(tc.y * mip->getHeight()), mip->getHeight()),
						c);
					if (texture->alphaTest && c.getAlpha() < 0.5f)
						continue;
					if (!texture->linear)
						c = c.linear();
				}
				c = lerp(c, modelColor, Scalar(modelColorBlend));

				// The back of a double sided face is lit as its flattened copy.
				Vector4 normal = (n[0] * Scalar(w0) + n[1] * Scalar(w1) + n[2] * Scalar(w2)).normalized();
				if (normal.y() < 0.0f)
					normal = -normal;
				const Scalar diffuse = clamp(dot3(normal, -c_sunDirection), 0.0_simd, 1.0_simd) * 0.7_simd + 0.3_simd;

				image->setPixelUnsafe(x, y, (c * diffuse).sRGB().rgb1());
				topHeight = h;
			}
		}
	}
}

Ref< hf::Heightfield > readHeightfield(editor::IPipelineCommon* pipelineCommon, const Guid& terrainId)
{
	Ref< const terrain::TerrainAsset > terrainAsset = pipelineCommon->getObjectReadOnly< terrain::TerrainAsset >(terrainId);
	if (!terrainAsset)
		return nullptr;

	Ref< db::Instance > heightfieldInstance = pipelineCommon->getSourceDatabase()->getInstance(terrainAsset->getHeightfield());
	if (!heightfieldInstance)
		return nullptr;

	Ref< const hf::HeightfieldAsset > heightfieldAsset = heightfieldInstance->getObject< const hf::HeightfieldAsset >();
	if (!heightfieldAsset)
		return nullptr;

	Ref< IStream > stream = heightfieldInstance->readData(L"Data");
	if (!stream)
		return nullptr;

	Ref< hf::Heightfield > heightfield = hf::HeightfieldFormat().read(stream, heightfieldAsset->getWorldExtent());
	stream->close();
	return heightfield;
}

float terrainHeight(const AlignedVector< float >& terrainHeights, int32_t size, int32_t x, int32_t y, float fallback)
{
	if (x < 0 || y < 0 || x >= size || y >= size)
		return fallback;

	const float h = terrainHeights[x + y * size];
	return (h != c_noHeight) ? h : fallback;
}

/*! Coverage of an anti-aliased line, width in pixels, where height crosses level; gradient in metres per pixel. */
float lineCoverage(float h, float level, float gradient, float width)
{
	if (gradient <= std::numeric_limits< float >::epsilon())
		return 0.0f;

	const float d = std::abs(h - level) / gradient;
	return std::clamp(0.5f * (width + 1.0f) - d, 0.0f, 1.0f);
}

}

T_IMPLEMENT_RTTI_CLASS(L"zombie.MinimapRasterizer", MinimapRasterizer, Object)

bool MinimapRasterizer::create(const editor::IPipelineSettings* settings)
{
	m_assetPath = settings->getPropertyExcludeHash< std::wstring >(L"Pipeline.AssetPath", L"");

	for (const auto& entityReplicatorType : type_of< world::IEntityReplicator >().findAllOf(false))
	{
		Ref< world::IEntityReplicator > entityReplicator = mandatory_non_null_type_cast< world::IEntityReplicator* >(entityReplicatorType->createInstance());
		if (!entityReplicator->create(settings))
			return false;

		for (auto supportedType : entityReplicator->getSupportedTypes())
			m_entityReplicators[supportedType] = entityReplicator;
	}

	m_sceneOperatorChain = new scene::SceneOperatorChain();
	return m_sceneOperatorChain->create(settings);
}

void MinimapRasterizer::destroy()
{
	safeDestroy(m_sceneOperatorChain);
	m_entityReplicators.clear();
}

Ref< drawing::Image > MinimapRasterizer::rasterize(editor::IPipelineCommon* pipelineCommon, const MinimapTextureAsset* asset, const std::atomic< bool >* cancelled, Statistics* outStatistics) const
{
	const auto isCancelled = [=]() { return cancelled != nullptr && cancelled->load(); };

	if (asset->m_bounds.mx.x <= asset->m_bounds.mn.x || asset->m_bounds.mx.y <= asset->m_bounds.mn.y)
	{
		log::error << L"Minimap rasterizer failed; bounds are empty." << Endl;
		return nullptr;
	}

	// The scene as built, after its geometric operators.
	Ref< scene::SceneAsset > sceneAsset = DeepClone(pipelineCommon->getObjectReadOnly(asset->m_source)).create< scene::SceneAsset >();
	if (!sceneAsset)
	{
		log::error << L"Minimap rasterizer failed; unable to read scene." << Endl;
		return nullptr;
	}

	if (!m_sceneOperatorChain->apply(sceneAsset, PipelineCommonTransformContext(pipelineCommon)))
	{
		log::error << L"Minimap rasterizer failed; unable to apply scene operators." << Endl;
		return nullptr;
	}

	Ref< const ISerializable > sourceData = world::resolveExternal(
		[&](const Guid& objectId) -> Ref< const ISerializable > {
			return pipelineCommon->getObjectReadOnly(objectId);
		},
		sceneAsset,
		Guid::null,
		nullptr
	);

	Ref< drawing::Image > image = new drawing::Image(drawing::PixelFormat::getR8G8B8A8(), asset->m_size, asset->m_size);
	image->clear(Color4f(0.0f, 0.0f, 0.0f, 0.0f));

	AlignedVector< float > heights((size_t)asset->m_size * asset->m_size, c_noHeight);
	SmallMap< Guid, Ref< MinimapTexture > > textures;
	RefArray< const world::EntityData > terrainEntities;
	float oceanHeight = c_noHeight;
	int32_t modelCount = 0;
	Semaphore lock;
	RefArray< Job > jobs;

	world::Traverser::visit(sourceData, [&](const world::EntityData* entityData) -> world::Traverser::Result {
		// Dynamic layers do not get included in minimap.
		if (!entityData->getState().visible || entityData->getState().dynamic)
			return world::Traverser::Result::Skip;

		// Entities without attributes are included.
		if (auto minimapAttributes = entityData->getComponent< MinimapAttributesEntityComponentData >())
		{
			if (!minimapAttributes->getIncludeInMinimap())
				return world::Traverser::Result::Skip;
		}

		if (auto oceanComponentData = entityData->getComponent< terrain::OceanComponentData >())
			oceanHeight = std::max(oceanHeight, oceanComponentData->getElevation());

		// Terrain is drawn from its heightfield once all models are rasterized.
		if (entityData->getComponent< terrain::TerrainComponentData >())
		{
			terrainEntities.push_back(entityData);
			return world::Traverser::Result::Continue;
		}

		Ref< Job > job = JobManager::getInstance().add([=, this, &image, &heights, &textures, &modelCount, &lock]() {
			if (isCancelled())
				return;

			Ref< model::Model > model;
			for (auto componentData : entityData->getComponents())
			{
				const world::IEntityReplicator* entityReplicator = m_entityReplicators[&type_of(componentData)];
				if (entityReplicator)
				{
					if ((model = entityReplicator->createModel(pipelineCommon, entityData, componentData, world::IEntityReplicator::Usage::Visual, world::IEntityReplicator::Flags::Default)) != nullptr)
						break;
				}
			}
			if (!model)
				return;

			model->apply(model::Triangulate());

			T_ANONYMOUS_VAR(Acquire< Semaphore >)(lock);

			// Shader previews are bound as images, textures by reference.
			RefArray< MinimapTexture > materialTextures;
			for (const auto& material : model->getMaterials())
			{
				const model::Material::Map& diffuseMap = material.getDiffuseMap();
				if (diffuseMap.image)
					materialTextures.push_back(createTexture(diffuseMap.image, false, false));
				else if (diffuseMap.texture.isNotNull())
				{
					if (textures.find(diffuseMap.texture) == textures.end())
						textures[diffuseMap.texture] = loadTexture(pipelineCommon, m_assetPath, diffuseMap.texture);
					materialTextures.push_back(textures[diffuseMap.texture]);
				}
				else
					materialTextures.push_back(nullptr);
			}

			rasterizeModel(model, materialTextures, entityData->getTransform(), asset->m_bounds, asset->m_modelColor.linear(), asset->m_modelColorBlend, image, heights);
			++modelCount;
		});
		if (!job)
			return world::Traverser::Result::Failed;

		jobs.push_back(job);
		return world::Traverser::Result::Continue;
	});

	while (!jobs.empty())
	{
		jobs.back()->wait();
		jobs.pop_back();
	}

	if (isCancelled())
		return nullptr;

	if (modelCount == 0 && terrainEntities.empty())
	{
		log::error << L"Minimap rasterizer failed; no models or terrain found in scene." << Endl;
		return nullptr;
	}

	// Terrain height of every pixel, also beneath models, since elevation lines depend on neighbours.
	const int32_t size = (int32_t)asset->m_size;
	const float sx = asset->m_bounds.getSize().x / size;
	const float sz = asset->m_bounds.getSize().y / size;
	AlignedVector< float > terrainHeights((size_t)size * size, c_noHeight);

	for (auto terrainEntity : terrainEntities)
	{
		Ref< const hf::Heightfield > heightfield = readHeightfield(pipelineCommon, terrainEntity->getComponent< terrain::TerrainComponentData >()->getTerrain());
		if (!heightfield)
		{
			log::error << L"Minimap rasterizer failed; unable to read heightfield of terrain \"" << terrainEntity->getName() << L"\"." << Endl;
			return nullptr;
		}

		const Vector4 origin = terrainEntity->getTransform().translation();
		const Vector4 halfExtent = heightfield->getWorldExtent() * 0.5_simd;

		for (int32_t y = 0; y < size; ++y)
		{
			if (isCancelled())
				return nullptr;

			const float tz = asset->m_bounds.mx.y - (y + 0.5f) * sz - origin.z();
			for (int32_t x = 0; x < size; ++x)
			{
				const float tx = asset->m_bounds.mn.x + (x + 0.5f) * sx - origin.x();
				if (std::abs(tx) > halfExtent.x() || std::abs(tz) > halfExtent.z() || !heightfield->getWorldCut(tx, tz))
					continue;

				float& h = terrainHeights[x + y * size];
				h = std::max< float >(h, heightfield->getWorldHeight(tx, tz) + origin.y());
			}
		}
	}

	// Terrain is tinted over the land's range of heights, which starts at the ocean.
	const bool ocean = (oceanHeight != c_noHeight);
	float lowHeight = std::numeric_limits< float >::max();
	float highHeight = c_noHeight;
	for (const float th : terrainHeights)
	{
		if (th != c_noHeight)
		{
			lowHeight = std::min(lowHeight, th);
			highHeight = std::max(highHeight, th);
		}
	}
	if (ocean)
		lowHeight = std::max(lowHeight, oceanHeight);
	const float heightRange = std::max(highHeight - lowHeight, std::numeric_limits< float >::epsilon());

	// Terrain covers models beneath it and the ocean covers everything beneath its elevation.
	for (int32_t y = 0; y < size; ++y)
	{
		if (isCancelled())
			return nullptr;

		for (int32_t x = 0; x < size; ++x)
		{
			const float th = terrainHeights[x + y * size];
			const float mh = heights[x + y * size];

			float g = 0.0f;
			if (th != c_noHeight)
			{
				const float gx = (terrainHeight(terrainHeights, size, x + 1, y, th) - terrainHeight(terrainHeights, size, x - 1, y, th)) * 0.5f;
				const float gy = (terrainHeight(terrainHeights, size, x, y + 1, th) - terrainHeight(terrainHeights, size, x, y - 1, th)) * 0.5f;
				g = std::sqrt(gx * gx + gy * gy);
			}

			Color4f color;
			if (ocean && std::max(th, mh) < oceanHeight)
				color = asset->m_oceanColor;
			else if (th != c_noHeight && th >= mh)
			{
				const Color4f terrainColor = lerp(asset->m_terrainLowColor, asset->m_terrainHighColor, Scalar(std::clamp((th - lowHeight) / heightRange, 0.0f, 1.0f)));
				color = lerp(terrainColor, asset->m_contourColor, Scalar(lineCoverage(th, std::round(th / asset->m_contourInterval) * asset->m_contourInterval, g, 1.0f)));
			}
			else
				continue;

			if (ocean && th != c_noHeight)
				color = lerp(color, asset->m_coastColor, Scalar(lineCoverage(th, oceanHeight, g, 2.0f)));

			image->setPixelUnsafe(x, y, color.rgb1());
		}
	}

	if (outStatistics)
	{
		outStatistics->models = modelCount;
		outStatistics->terrains = (int32_t)terrainEntities.size();
	}

	return image;
}

}
