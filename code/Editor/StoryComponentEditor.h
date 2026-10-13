/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Guid.h"
#include "Core/Math/Color4ub.h"
#include "Core/Math/Vector4.h"
#include "Scene/Editor/IComponentEditor.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::scene
{

class EntityAdapter;
class SceneEditorContext;

}

namespace traktor::world
{

class IEntityComponentData;

}

namespace zombie
{

using namespace traktor;

/*! Label over an actor, a place, a pickup or a container in the scene editor's viewport.
 * \ingroup Zombie
 */
class T_DLLCLASS StoryComponentEditor : public scene::IComponentEditor
{
	T_RTTI_CLASS;

public:
	constexpr static const wchar_t* GuideId = L"Story.Labels";

	explicit StoryComponentEditor(scene::SceneEditorContext* context, scene::EntityAdapter* entityAdapter, world::IEntityComponentData* componentData);

	virtual void drawGuide(render::PrimitiveRenderer* primitiveRenderer) const override final;

private:
	scene::SceneEditorContext* m_context;
	scene::EntityAdapter* m_entityAdapter;
	AlignedVector< Guid > m_ids;
	std::wstring m_text;
	Color4ub m_color;
	float m_height = 1.0f;

	static void drawLabel(render::PrimitiveRenderer* primitiveRenderer, const Vector4& position, const std::wstring& text, const Color4ub& color);
};

}
