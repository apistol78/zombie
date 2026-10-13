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
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "Editor/IEditorPage.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::editor
{

class IDocument;
class IEditor;
class IEditorPageSite;
class PropertiesView;

}

namespace traktor::ui
{

class Container;
class ContentChangeEvent;
class GraphControl;
class ListBox;
class Menu;
class MouseButtonDownEvent;
class MouseDoubleClickEvent;
class NodeActivateEvent;
class NodeMovedEvent;
class SelectionChangeEvent;
class ToolBar;
class ToolBarButtonClickEvent;

}

namespace zombie
{

using namespace traktor;

class QuestData;
class QuestStep;

/*! Quest editor page; the steps as a vertical flow, and a panel of problems and references.
 * \ingroup Zombie
 */
class T_DLLCLASS QuestEditorPage : public editor::IEditorPage
{
	T_RTTI_CLASS;

public:
	explicit QuestEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document);

	virtual bool create(ui::Container* parent) override final;

	virtual void destroy() override final;

	virtual bool dropInstance(db::Instance* instance, const ui::Point& position) override final;

	virtual bool handleCommand(const ui::Command& command) override final;

	virtual void handleDatabaseEvent(db::Database* database, const Guid& eventId) override final;

private:
	editor::IEditor* m_editor;
	editor::IEditorPageSite* m_site;
	editor::IDocument* m_document;
	Ref< QuestData > m_quest;
	Guid m_questId;
	Ref< ui::ToolBar > m_toolBar;
	Ref< ui::GraphControl > m_graph;
	Ref< ui::Menu > m_menuPopup;
	Ref< editor::PropertiesView > m_propertiesView;
	Ref< ui::Container > m_containerQuest;
	Ref< ui::ListBox > m_listQuest;

	/*! An object elsewhere which starts, advances, finishes or checks the quest. */
	struct Reference
	{
		Ref< db::Instance > instance;
		std::wstring where;		//!< Where in the instance, such as a dialog's line; may be empty.
		Ref< const ISerializable > object;
	};

	AlignedVector< Reference > m_references;

	void rebuildGraph(const RefArray< QuestStep >& select, bool selectQuest);

	void findReferences();

	void updateQuestPanel();

	std::wstring checkStep(int32_t index) const;

	std::wstring describeTarget(const QuestStep* step) const;

	std::wstring nameOf(const Guid& id) const;

	RefArray< QuestStep > getSelectedSteps() const;

	void addStep(const TypeInfo& stepType);

	void moveSelection(int32_t direction);

	void copySelection(bool cut);

	void paste();

	void deleteSelection();

	void select(ISerializable* object);

	void eventToolBarClick(ui::ToolBarButtonClickEvent* event);

	void eventButtonDown(ui::MouseButtonDownEvent* event);

	void eventSelect(ui::SelectionChangeEvent* event);

	void eventNodeMoved(ui::NodeMovedEvent* event);

	void eventNodeActivate(ui::NodeActivateEvent* event);

	void eventPropertiesChanged(ui::ContentChangeEvent* event);

	void eventQuestPanelDoubleClick(ui::MouseDoubleClickEvent* event);
};

}
