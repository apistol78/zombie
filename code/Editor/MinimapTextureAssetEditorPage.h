/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <atomic>
#include "Core/Ref.h"
#include "Editor/IEditorPage.h"
#include "Editor/MinimapRasterizer.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class Thread;

}

namespace traktor::drawing
{

class Image;

}

namespace traktor::editor
{

class IDocument;
class IEditor;
class IEditorPageSite;
class PropertiesView;

}

namespace traktor::render
{

class TextureControl;

}

namespace traktor::ui
{

class ContentChangeEvent;
class StatusBar;
class TimerEvent;

}

namespace zombie
{

using namespace traktor;

class MinimapTextureAsset;

/*! Minimap texture asset editor page; the asset's properties beside a preview of the minimap.
 * \ingroup Zombie
 *
 * The preview is rasterized in a thread of its own, from the asset as edited, and again
 * whenever a property or the scene changes; a change while it runs cancels it and starts over.
 */
class T_DLLCLASS MinimapTextureAssetEditorPage : public editor::IEditorPage
{
	T_RTTI_CLASS;

public:
	explicit MinimapTextureAssetEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document);

	virtual bool create(ui::Container* parent) override final;

	virtual void destroy() override final;

	virtual bool dropInstance(db::Instance* instance, const ui::Point& position) override final;

	virtual bool handleCommand(const ui::Command& command) override final;

	virtual void handleDatabaseEvent(db::Database* database, const Guid& eventId) override final;

private:
	editor::IEditor* m_editor;
	editor::IEditorPageSite* m_site;
	editor::IDocument* m_document;
	Ref< MinimapTextureAsset > m_asset;
	Ref< MinimapRasterizer > m_rasterizer;
	Ref< render::TextureControl > m_textureControl;
	Ref< ui::StatusBar > m_statusBar;
	Ref< editor::PropertiesView > m_propertiesView;

	// Rasterization in progress; the results are only read once the thread has finished.
	Thread* m_thread = nullptr;
	std::atomic< bool > m_cancelled = false;
	bool m_restart = false;
	Ref< drawing::Image > m_image;
	MinimapRasterizer::Statistics m_statistics;
	double m_duration = 0.0;

	void updatePreview();

	void eventPropertiesChanged(ui::ContentChangeEvent* event);

	void eventTimer(ui::TimerEvent* event);
};

}
