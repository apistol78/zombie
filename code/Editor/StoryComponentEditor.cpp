/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryComponentEditor.h"

#include "ActorComponentData.h"
#include "ActorData.h"
#include "Core/Misc/String.h"
#include "Database/Database.h"
#include "Database/Instance.h"
#include "Editor/StoryGuides.h"
#include "I18N/Text.h"
#include "ItemContainerComponentData.h"
#include "ItemData.h"
#include "ItemPickupComponentData.h"
#include "PlaceComponentData.h"
#include "PlaceData.h"
#include "Render/PrimitiveRenderer.h"
#include "Scene/Editor/EntityAdapter.h"
#include "Scene/Editor/SceneEditorContext.h"

#include <algorithm>

namespace zombie
{
	namespace
	{

const float c_glyphWidth = 0.14f;
const float c_glyphHeight = 0.24f;
const Color4ub c_colorActor(130, 200, 255, 255);
const Color4ub c_colorPlace(255, 205, 90, 255);
const Color4ub c_colorItem(150, 240, 150, 255);
const Color4ub c_colorTarget(255, 90, 70, 255);

/*! Name of what id refers to as the player knows it; the instance's name if it has none. */
std::wstring nameOf(db::Database* database, const Guid& id)
{
	if (id.isNull())
		return i18n::Text(L"STORY_QUEST_NONE");

	Ref< db::Instance > instance = database ? database->getInstance(id) : nullptr;
	if (!instance)
		return i18n::Text(L"STORY_QUEST_MISSING");

	std::wstring name;
	Ref< ISerializable > object = database->getObjectReadOnly(id);
	if (auto actorData = dynamic_type_cast< const ActorData* >(object))
		name = actorData->getName();
	else if (auto placeData = dynamic_type_cast< const PlaceData* >(object))
		name = placeData->getName();
	else if (auto itemData = dynamic_type_cast< const ItemData* >(object))
		name = itemData->getName();
	return !name.empty() ? name : instance->getName();
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.StoryComponentEditor", StoryComponentEditor, scene::IComponentEditor)

StoryComponentEditor::StoryComponentEditor(scene::SceneEditorContext* context, scene::EntityAdapter* entityAdapter, world::IEntityComponentData* componentData)
:	m_context(context)
,	m_entityAdapter(entityAdapter)
{
	db::Database* database = context->getSourceDatabase();

	if (auto actorComponentData = dynamic_type_cast< const ActorComponentData* >(componentData))
	{
		m_ids.push_back(actorComponentData->getActor());
		m_text = nameOf(database, actorComponentData->getActor());
		m_color = c_colorActor;
		m_height = 2.3f;
	}
	else if (auto placeComponentData = dynamic_type_cast< const PlaceComponentData* >(componentData))
	{
		m_ids.push_back(placeComponentData->getPlace());
		m_text = nameOf(database, placeComponentData->getPlace());
		m_color = c_colorPlace;
		m_height = 1.2f;
	}
	else if (auto pickupComponentData = dynamic_type_cast< const ItemPickupComponentData* >(componentData))
	{
		m_ids.push_back(pickupComponentData->getItem());
		m_text = (pickupComponentData->getCount() > 1 ? toString(pickupComponentData->getCount()) + L" " : L"") + nameOf(database, pickupComponentData->getItem());
		m_color = c_colorItem;
		m_height = 0.6f;
	}
	else if (auto containerComponentData = dynamic_type_cast< const ItemContainerComponentData* >(componentData))
	{
		for (const auto& entry : containerComponentData->getEntries())
		{
			m_ids.push_back(entry.item);
			m_text += (!m_text.empty() ? L", " : L"") + (entry.count > 1 ? toString(entry.count) + L" " : L"") + nameOf(database, entry.item);
		}
		if (m_text.empty())
			m_text = i18n::Text(L"STORY_SCENE_EMPTY");
		m_color = c_colorItem;
		m_height = 1.2f;
	}
}

void StoryComponentEditor::drawGuide(render::PrimitiveRenderer* primitiveRenderer) const
{
	if (m_text.empty() || !m_context->shouldDrawGuide(GuideId) || !m_entityAdapter->isVisible())
		return;

	// The steps of the picked quest about this entity, if any.
	const StoryGuides* guides = StoryGuides::get(m_context);
	std::wstring steps;
	float radius = 0.0f;
	for (const auto& id : m_ids)
	{
		if (const StoryGuides::Target* target = guides->find(id))
		{
			steps += (!steps.empty() ? L", " : L"") + target->label;
			radius = std::max(radius, target->radius);
		}
	}

	const Vector4 base = m_entityAdapter->getTransform().translation().xyz1();
	const Vector4 top = base + Vector4(0.0f, m_height, 0.0f, 0.0f);
	const Color4ub color = !steps.empty() ? c_colorTarget : m_color;

	primitiveRenderer->pushDepthState(steps.empty(), false, false);

	if (!steps.empty())
	{
		primitiveRenderer->drawLine(base, top, 2.0f, color);
		if (radius > 0.0f)
			primitiveRenderer->drawWireCircle(base, Vector4(0.0f, 1.0f, 0.0f, 0.0f), radius, 2.0f, color);
		drawLabel(primitiveRenderer, top + Vector4(0.0f, c_glyphHeight * 1.3f, 0.0f, 0.0f), steps, color);
	}
	drawLabel(primitiveRenderer, top, m_text, color);

	primitiveRenderer->popDepthState();
}

void StoryComponentEditor::drawLabel(render::PrimitiveRenderer* primitiveRenderer, const Vector4& position, const std::wstring& text, const Color4ub& color)
{
	const Matrix44 viewInverse = primitiveRenderer->getView().inverse();
	const Vector4 left = viewInverse.axisX() * Scalar(c_glyphWidth * text.length() * 0.5f);
	primitiveRenderer->drawText(position - left, c_glyphWidth, c_glyphHeight, text, color);
}

}
