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
#include "Core/Containers/SmallSet.h"
#include "Core/Guid.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "Editor/IEditorPage.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class ISerializable;

}

namespace traktor::editor
{

class IDocument;
class IEditor;
class IEditorPageSite;
class PropertiesView;

}

namespace traktor::ui
{

class ButtonClickEvent;
class ChatView;
class CheckBox;
class Container;
class ContentChangeEvent;
class EdgeConnectEvent;
class EdgeDisconnectEvent;
class GraphControl;
class ListBox;
class Menu;
class MouseButtonDownEvent;
class MouseDoubleClickEvent;
class Node;
class NodeMovedEvent;
class SelectionChangeEvent;
class ToolBar;
class ToolBarButtonClickEvent;
class Widget;

}

namespace zombie
{

using namespace traktor;

class DialogAsset;
class DialogData;
class DialogNodeAsset;
class IAction;
class ICondition;

/*! Dialog editor page; the dialog as a graph of lines and responses, and a panel to play it.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogEditorPage : public editor::IEditorPage
{
	T_RTTI_CLASS;

public:
	explicit DialogEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document);

	virtual bool create(ui::Container* parent) override final;

	virtual void destroy() override final;

	virtual bool dropInstance(db::Instance* instance, const ui::Point& position) override final;

	virtual bool handleCommand(const ui::Command& command) override final;

	virtual void handleDatabaseEvent(db::Database* database, const Guid& eventId) override final;

private:
	editor::IEditor* m_editor;
	editor::IEditorPageSite* m_site;
	editor::IDocument* m_document;
	Ref< DialogAsset > m_dialog;
	Ref< ui::ToolBar > m_toolBar;
	Ref< ui::GraphControl > m_graph;
	Ref< ui::Menu > m_menuPopup;
	Ref< editor::PropertiesView > m_propertiesView;

	// Dialog panel.
	Ref< ui::Container > m_containerTest;
	Ref< ui::Container > m_testItems;
	Ref< ui::ChatView > m_testChat;
	Ref< ui::ListBox > m_testResponses;
	RefArray< ui::Widget > m_testItemWidgets;	//!< Checkbox per item in m_testItemIds, or a note when there are none.
	AlignedVector< Guid > m_testItemIds;		//!< Items which conditions in the dialog check.
	SmallSet< Guid > m_testHeld;				//!< Items checked; kept by guid as the list is rebuilt.
	SmallSet< std::pair< int32_t, int32_t > > m_testSaid;	//!< Once-responses chosen, as (line, response).
	AlignedVector< int32_t > m_testShown;		//!< Response index per row in the response list.
	Ref< DialogData > m_testDialog;
	int32_t m_testNode = -1;

	void rebuildGraph(const ISerializable* select);

	void syncPositions();

	void arrange();

	void createNode(const ui::Point& at);

	void addResponse(DialogNodeAsset* node);

	void addStart();

	void copySelection(bool cut);

	void paste();

	void deleteSelection();

	void select(ISerializable* object);

	void testUpdateItems();

	bool testEvaluate(const ICondition* condition) const;

	bool testExecute(const IAction* action, std::wstring& outNotice);

	void testRestart();

	void testEnter(int32_t node);

	void testShowResponses();

	void testAppend(const std::wstring& text);

	bool testChoose(const ui::Point& position);

	void eventToolBarClick(ui::ToolBarButtonClickEvent* event);

	void eventButtonDown(ui::MouseButtonDownEvent* event);

	void eventSelect(ui::SelectionChangeEvent* event);

	void eventNodeMoved(ui::NodeMovedEvent* event);

	void eventEdgeConnect(ui::EdgeConnectEvent* event);

	void eventEdgeDisconnect(ui::EdgeDisconnectEvent* event);

	void eventPropertiesChanged(ui::ContentChangeEvent* event);

	void eventTestRestart(ui::ButtonClickEvent* event);

	void eventTestItem(ui::ButtonClickEvent* event);

	void eventTestResponse(ui::MouseButtonDownEvent* event);

	void eventTestResponseDoubleClick(ui::MouseDoubleClickEvent* event);
};

}
