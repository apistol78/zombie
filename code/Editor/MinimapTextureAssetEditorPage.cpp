/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapTextureAssetEditorPage.h"

#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/String.h"
#include "Core/Serialization/DeepClone.h"
#include "Core/Settings/PropertyGroup.h"
#include "Core/Thread/Thread.h"
#include "Core/Thread/ThreadManager.h"
#include "Core/Timer/Timer.h"
#include "Database/Database.h"
#include "Database/Instance.h"
#include "Drawing/Image.h"
#include "Editor/IDocument.h"
#include "Editor/IEditor.h"
#include "Editor/IEditorPageSite.h"
#include "Editor/IPipelineCommon.h"
#include "Editor/MinimapTextureAsset.h"
#include "Editor/Pipeline/PipelineSettings.h"
#include "Editor/PropertiesView.h"
#include "Render/Editor/Texture/TextureControl.h"
#include "Render/Editor/Texture/TextureOutput.h"
#include "Ui/Container.h"
#include "Ui/Events/ContentChangeEvent.h"
#include "Ui/Events/TimerEvent.h"
#include "Ui/StatusBar/StatusBar.h"
#include "Ui/TableLayout.h"

namespace zombie
{
	namespace
	{

class PreviewPipelineCommon : public editor::IPipelineCommon
{
public:
	explicit PreviewPipelineCommon(db::Database* database)
	:	m_database(database)
	{
	}

	virtual db::Database* getSourceDatabase() const override final
	{
		return m_database;
	}

	virtual Ref< const ISerializable > getObjectReadOnly(const Guid& instanceGuid) override final
	{
		return m_database->getObjectReadOnly(instanceGuid);
	}

private:
	db::Database* m_database;
};

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.MinimapTextureAssetEditorPage", MinimapTextureAssetEditorPage, editor::IEditorPage)

MinimapTextureAssetEditorPage::MinimapTextureAssetEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document)
:	m_editor(editor)
,	m_site(site)
,	m_document(document)
{
}

bool MinimapTextureAssetEditorPage::create(ui::Container* parent)
{
	m_asset = m_document->getObject< MinimapTextureAsset >(0);
	if (!m_asset)
		return false;

	const editor::PipelineSettings pipelineSettings(m_editor->getSettings());
	m_rasterizer = new MinimapRasterizer();
	if (!m_rasterizer->create(&pipelineSettings))
		return false;

	Ref< ui::Container > container = new ui::Container();
	container->create(parent, ui::WsNone, new ui::TableLayout(L"100%", L"100%,*", 0_ut, 0_ut));

	m_textureControl = new render::TextureControl();
	m_textureControl->create(container);
	m_textureControl->addEventHandler< ui::TimerEvent >(this, &MinimapTextureAssetEditorPage::eventTimer);
	m_textureControl->startTimer(100);

	m_statusBar = new ui::StatusBar();
	m_statusBar->create(container);
	m_statusBar->addColumn(600);

	m_propertiesView = m_site->createPropertiesView(parent, {});
	m_propertiesView->setPropertyObject(m_asset);
	m_propertiesView->addEventHandler< ui::ContentChangeEvent >(this, &MinimapTextureAssetEditorPage::eventPropertiesChanged);
	m_site->createAdditionalPanel(m_propertiesView, 400_ut, false);

	updatePreview();
	return true;
}

void MinimapTextureAssetEditorPage::destroy()
{
	if (m_thread)
	{
		m_cancelled = true;
		m_thread->wait();
		ThreadManager::getInstance().destroy(m_thread);
		m_thread = nullptr;
	}

	if (m_textureControl)
		m_textureControl->stopTimer();

	if (m_propertiesView)
		m_site->destroyAdditionalPanel(m_propertiesView);

	safeDestroy(m_propertiesView);
	safeDestroy(m_rasterizer);

	m_asset = nullptr;
	m_site = nullptr;
}

bool MinimapTextureAssetEditorPage::dropInstance(db::Instance* instance, const ui::Point& position)
{
	return false;
}

bool MinimapTextureAssetEditorPage::handleCommand(const ui::Command& command)
{
	return m_propertiesView->handleCommand(command);
}

void MinimapTextureAssetEditorPage::handleDatabaseEvent(db::Database* database, const Guid& eventId)
{
	// Only the scene itself is watched, not everything placed in it.
	if (eventId == m_asset->m_source)
		updatePreview();
}

void MinimapTextureAssetEditorPage::updatePreview()
{
	if (m_thread)
	{
		m_cancelled = true;
		m_restart = true;
		return;
	}

	// Rasterized from a copy, so the properties can keep changing meanwhile.
	Ref< const MinimapTextureAsset > asset = DeepClone(m_asset).create< MinimapTextureAsset >();
	m_cancelled = false;
	m_restart = false;
	m_image = nullptr;

	m_thread = ThreadManager::getInstance().create([=, this]() {
		PreviewPipelineCommon pipelineCommon(m_editor->getSourceDatabase());
		Timer timer;
		m_image = m_rasterizer->rasterize(&pipelineCommon, asset, &m_cancelled, &m_statistics);
		m_duration = timer.getElapsedTime();
	}, L"Minimap preview");
	if (!m_thread)
		return;

	m_thread->start();
	m_statusBar->setText(0, L"Rasterizing...");
}

void MinimapTextureAssetEditorPage::eventPropertiesChanged(ui::ContentChangeEvent* event)
{
	updatePreview();
}

void MinimapTextureAssetEditorPage::eventTimer(ui::TimerEvent* event)
{
	if (!m_thread || !m_thread->finished())
		return;

	m_thread->wait();
	ThreadManager::getInstance().destroy(m_thread);
	m_thread = nullptr;

	if (m_restart)
	{
		updatePreview();
		return;
	}

	if (m_image)
	{
		m_textureControl->setImage(m_image, render::TextureOutput(), ~0U);
		m_statusBar->setText(0, str(L"%d x %d, %d model(s), %d terrain(s), %.1f s", m_image->getWidth(), m_image->getHeight(), m_statistics.models, m_statistics.terrains, m_duration));
	}
	else
		m_statusBar->setText(0, L"Failed; see log.");
}

}
