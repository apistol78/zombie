/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/DialogEditorPage.h"

#include "Actions/AdvanceQuestAction.h"
#include "Actions/EventAction.h"
#include "Actions/GiveItemAction.h"
#include "Actions/SetFlagAction.h"
#include "Actions/TakeItemAction.h"
#include "Conditions/AllCondition.h"
#include "Conditions/AnyCondition.h"
#include "Conditions/HasItemCondition.h"
#include "Core/Containers/SmallMap.h"
#include "Core/Containers/SmallSet.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/String.h"
#include "Core/Serialization/DeepClone.h"
#include "Database/Database.h"
#include "Database/Instance.h"
#include "DialogData.h"
#include "DialogNode.h"
#include "DialogResponse.h"
#include "Editor/DialogAsset.h"
#include "Editor/DialogClipboardData.h"
#include "Editor/DialogPipeline.h"
#include "Editor/IDocument.h"
#include "Editor/IEditor.h"
#include "Editor/IEditorPageSite.h"
#include "Editor/PropertiesView.h"
#include "I18N/Text.h"
#include "ItemData.h"
#include "QuestData.h"
#include "Ui/Application.h"
#include "Ui/Button.h"
#include "Ui/ChatView.h"
#include "Ui/CheckBox.h"
#include "Ui/Clipboard.h"
#include "Ui/Container.h"
#include "Ui/Events/MouseDoubleClickEvent.h"
#include "Ui/Graph/AlignNodesLayoutOperation.h"
#include "Ui/Graph/DefaultNodeShape.h"
#include "Ui/Graph/Edge.h"
#include "Ui/Graph/EdgeConnectEvent.h"
#include "Ui/Graph/EdgeDisconnectEvent.h"
#include "Ui/Graph/EvenSpaceLayoutOperation.h"
#include "Ui/Graph/GraphControl.h"
#include "Ui/Graph/Node.h"
#include "Ui/Graph/NodeMovedEvent.h"
#include "Ui/Graph/Pin.h"
#include "Ui/ListBox/ListBox.h"
#include "Ui/Menu.h"
#include "Ui/MenuItem.h"
#include "Ui/Splitter.h"
#include "Ui/Static.h"
#include "Ui/StyleBitmap.h"
#include "Ui/TableLayout.h"
#include "Ui/ToolBar/ToolBar.h"
#include "Ui/ToolBar/ToolBarButton.h"
#include "Ui/ToolBar/ToolBarButtonClickEvent.h"
#include "Ui/ToolBar/ToolBarSeparator.h"

#include <algorithm>
#include <deque>
#include <limits>

namespace zombie
{
	namespace
	{

const int32_t c_columnWidth = 360;
const int32_t c_lineHeight = 70;
const int32_t c_pinHeight = 22;
const int32_t c_maxExcerpt = 40;
const int32_t c_pasteOffset = 40;

std::wstring excerpt(const std::wstring& text, int32_t length)
{
	if ((int32_t)text.length() <= length)
		return text;
	return text.substr(0, length - 3) + L"...";
}

std::wstring lineTitle(const DialogNodeAsset* nodeAsset)
{
	return !nodeAsset->name.empty() ? nodeAsset->name : std::wstring(i18n::Text(L"STORY_DIALOG_UNNAMED"));
}

std::wstring responseLabel(int32_t index, const std::wstring& text, bool condition, bool once, bool actions)
{
	std::wstring label = toString(index + 1) + L". " + excerpt(text, c_maxExcerpt);
	if (condition)
		label += L" [?]";
	if (once)
		label += L" [1x]";
	if (actions)
		label += L" [!]";
	return label;
}

std::wstring startLabel(int32_t index, const DialogStartAsset* start)
{
	return toString(index + 1) + (start->condition ? L". [?]" : L".");
}

/*! Name of the instance with guid id; the guid if there is none. */
std::wstring instanceName(db::Database* database, const Guid& id)
{
	Ref< db::Instance > instance = (database && id.isNotNull()) ? database->getInstance(id) : nullptr;
	return instance ? instance->getName() : id.format();
}

/*! What an action does, for the test transcript; keys and steps by the names their instances give them. */
std::wstring describe(const IAction* action, db::Database* database)
{
	std::wstring name = type_name(action);
	const size_t dot = name.find_last_of(L'.');
	if (dot != name.npos)
		name = name.substr(dot + 1);
	if (auto eventAction = dynamic_type_cast< const EventAction* >(action))
		name += L" \"" + instanceName(database, eventAction->getEvent()) + L"\"";
	else if (auto setFlagAction = dynamic_type_cast< const SetFlagAction* >(action))
		name += L" \"" + instanceName(database, setFlagAction->getFlag()) + L"\" = " + (setFlagAction->getValue() ? L"true" : L"false");
	else if (auto advanceQuestAction = dynamic_type_cast< const AdvanceQuestAction* >(action))
	{
		name += L" \"" + instanceName(database, advanceQuestAction->getQuest()) + L"\"";
		Ref< QuestData > questData = database ? database->getObjectReadOnly< QuestData >(advanceQuestAction->getQuest()) : nullptr;
		const int32_t step = questData ? questData->findStep(advanceQuestAction->getStep()) : -1;
		if (step >= 0)
			name += L", step " + toString(step + 1) + (!questData->getSteps()[step]->getName().empty() ? L" \"" + questData->getSteps()[step]->getName() + L"\"" : L"");
	}
	else if (auto giveItemAction = dynamic_type_cast< const GiveItemAction* >(action))
		name += L" x" + toString(giveItemAction->getCount());
	else if (auto takeItemAction = dynamic_type_cast< const TakeItemAction* >(action))
		name += L" x" + toString(takeItemAction->getCount());
	return name;
}

/*! Items which a condition checks, nested ones included. */
void collectItems(const ICondition* condition, AlignedVector< Guid >& outItems)
{
	if (auto hasItemCondition = dynamic_type_cast< const HasItemCondition* >(condition))
	{
		const Guid& item = hasItemCondition->getItem();
		if (item.isNotNull() && std::find(outItems.begin(), outItems.end(), item) == outItems.end())
			outItems.push_back(item);
	}
	else if (auto allCondition = dynamic_type_cast< const AllCondition* >(condition))
	{
		for (auto child : allCondition->getConditions())
			collectItems(child, outItems);
	}
	else if (auto anyCondition = dynamic_type_cast< const AnyCondition* >(condition))
	{
		for (auto child : anyCondition->getConditions())
			collectItems(child, outItems);
	}
}

int32_t indexOf(const RefArray< ui::Pin >& pins, const ui::Pin* pin)
{
	for (int32_t i = 0; i < (int32_t)pins.size(); ++i)
	{
		if (pins[i] == pin)
			return i;
	}
	return -1;
}

bool isZero(const Vector2& v)
{
	return v.x == 0.0f && v.y == 0.0f;
}

ui::UnitPoint toUnit(const Vector2& position)
{
	return ui::UnitPoint(ui::Unit((int32_t)position.x), ui::Unit((int32_t)position.y));
}

Vector2 fromUnit(const ui::UnitPoint& position)
{
	return Vector2((float)position.x.get(), (float)position.y.get());
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.DialogEditorPage", DialogEditorPage, editor::IEditorPage)

DialogEditorPage::DialogEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document)
:	m_editor(editor)
,	m_site(site)
,	m_document(document)
{
}

bool DialogEditorPage::create(ui::Container* parent)
{
	m_dialog = m_document->getObject< DialogAsset >(0);
	if (!m_dialog)
		return false;

	Ref< ui::Container > container = new ui::Container();
	container->create(parent, ui::WsNone, new ui::TableLayout(L"100%", L"*,100%", 0_ut, 0_ut));

	m_toolBar = new ui::ToolBar();
	m_toolBar->create(container);
	for (int32_t i = 0; i < 6; ++i)
		m_toolBar->addImage(new ui::StyleBitmap(L"Animation.Alignment", i));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ADD_LINE"), ui::Command(L"Dialog.Editor.CreateLine")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ADD_RESPONSE"), ui::Command(L"Dialog.Editor.AddResponse")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ADD_START"), ui::Command(L"Dialog.Editor.AddStart")));
	m_toolBar->addItem(new ui::ToolBarSeparator());
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ALIGN_LEFT"), 0, ui::Command(L"Dialog.Editor.AlignLeft")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ALIGN_RIGHT"), 1, ui::Command(L"Dialog.Editor.AlignRight")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ALIGN_TOP"), 2, ui::Command(L"Dialog.Editor.AlignTop")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ALIGN_BOTTOM"), 3, ui::Command(L"Dialog.Editor.AlignBottom")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_EVEN_VERTICALLY"), 4, ui::Command(L"Dialog.Editor.EvenSpaceVertically")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_EVEN_HORIZONTALLY"), 5, ui::Command(L"Dialog.Editor.EvenSpaceHorizontally")));
	m_toolBar->addItem(new ui::ToolBarSeparator());
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_DIALOG_ARRANGE"), ui::Command(L"Dialog.Editor.Arrange")));
	m_toolBar->addEventHandler< ui::ToolBarButtonClickEvent >(this, &DialogEditorPage::eventToolBarClick);

	m_graph = new ui::GraphControl();
	m_graph->create(container, ui::GraphControl::WsEdgeSelectable | ui::WsDoubleBuffer | ui::WsAccelerated);
	m_graph->setText(L"DIALOG");
	m_graph->addEventHandler< ui::MouseButtonDownEvent >(this, &DialogEditorPage::eventButtonDown);
	m_graph->addEventHandler< ui::SelectionChangeEvent >(this, &DialogEditorPage::eventSelect);
	m_graph->addEventHandler< ui::NodeMovedEvent >(this, &DialogEditorPage::eventNodeMoved);
	m_graph->addEventHandler< ui::EdgeConnectEvent >(this, &DialogEditorPage::eventEdgeConnect);
	m_graph->addEventHandler< ui::EdgeDisconnectEvent >(this, &DialogEditorPage::eventEdgeDisconnect);

	m_menuPopup = new ui::Menu();
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Dialog.Editor.CreateLine"), i18n::Text(L"STORY_DIALOG_ADD_LINE")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Dialog.Editor.AddResponse"), i18n::Text(L"STORY_DIALOG_ADD_RESPONSE")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Dialog.Editor.AddStart"), i18n::Text(L"STORY_DIALOG_ADD_START")));
	m_menuPopup->add(new ui::MenuItem(L"-"));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Cut"), i18n::Text(L"STORY_DIALOG_CUT")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Copy"), i18n::Text(L"STORY_DIALOG_COPY")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Paste"), i18n::Text(L"STORY_DIALOG_PASTE")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Delete"), i18n::Text(L"STORY_DIALOG_DELETE")));
	m_menuPopup->add(new ui::MenuItem(L"-"));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Dialog.Editor.Arrange"), i18n::Text(L"STORY_DIALOG_ARRANGE")));

	m_propertiesView = m_site->createPropertiesView(parent, {});
	m_propertiesView->addEventHandler< ui::ContentChangeEvent >(this, &DialogEditorPage::eventPropertiesChanged);
	m_site->createAdditionalPanel(m_propertiesView, 400_ut, false);

	// The dialog panel: the items held and the responses open now to the left; to the right,
	// the conversation so far.
	m_containerTest = new ui::Container();
	m_containerTest->create(parent, ui::WsNone, new ui::TableLayout(L"100%", L"100%", 0_ut, 0_ut));
	m_containerTest->setText(i18n::Text(L"STORY_DIALOG_TEST"));

	Ref< ui::Splitter > splitter = new ui::Splitter();
	splitter->create(m_containerTest, true, 360_ut);

	Ref< ui::Container > containerControls = new ui::Container();
	containerControls->create(splitter, ui::WsNone, new ui::TableLayout(L"100%", L"*,*,100%", 4_ut, 4_ut));

	Ref< ui::Button > buttonRestart = new ui::Button();
	buttonRestart->create(containerControls, i18n::Text(L"STORY_DIALOG_TEST_RESTART"));
	buttonRestart->addEventHandler< ui::ButtonClickEvent >(this, &DialogEditorPage::eventTestRestart);

	m_testItems = new ui::Container();
	m_testItems->create(containerControls, ui::WsNone, new ui::TableLayout(L"100%", L"*", 0_ut, 2_ut));

	// A response is chosen by pressing it, single or double click alike.
	m_testResponses = new ui::ListBox();
	m_testResponses->create(containerControls, ui::ListBox::WsSingle);
	m_testResponses->addEventHandler< ui::MouseButtonDownEvent >(this, &DialogEditorPage::eventTestResponse);
	m_testResponses->addEventHandler< ui::MouseDoubleClickEvent >(this, &DialogEditorPage::eventTestResponseDoubleClick);

	m_testChat = new ui::ChatView();
	m_testChat->create(splitter);

	m_site->createAdditionalPanel(m_containerTest, 300_ut, true);

	// A dialog never laid out, such as one converted from elsewhere, is arranged first.
	bool laidOut = !isZero(m_dialog->startPosition);
	for (auto node : m_dialog->nodes)
		laidOut |= !isZero(node->position);
	if (!laidOut)
		arrange();

	rebuildGraph(nullptr);

	parent->update();
	m_graph->center();

	select(m_dialog);
	testRestart();
	return true;
}

void DialogEditorPage::destroy()
{
	if (m_propertiesView)
		m_site->destroyAdditionalPanel(m_propertiesView);
	if (m_containerTest)
		m_site->destroyAdditionalPanel(m_containerTest);

	safeDestroy(m_propertiesView);
	safeDestroy(m_containerTest);
	safeDestroy(m_graph);

	m_site = nullptr;
}

bool DialogEditorPage::dropInstance(db::Instance* instance, const ui::Point& position)
{
	return false;
}

bool DialogEditorPage::handleCommand(const ui::Command& command)
{
	if (m_propertiesView->handleCommand(command))
		return true;

	if (command == L"Editor.SelectAll")
		m_graph->selectAllNodes();
	else if (command == L"Editor.Unselect")
		m_graph->deselectAllNodes();
	else if (command == L"Editor.Delete")
		deleteSelection();
	else if (command == L"Editor.Cut" || command == L"Editor.Copy")
		copySelection(command == L"Editor.Cut");
	else if (command == L"Editor.Paste")
		paste();
	else if (command == L"Editor.Undo" || command == L"Editor.Redo")
	{
		if (!(command == L"Editor.Undo" ? m_document->undo() : m_document->redo()))
			return false;

		m_dialog = m_document->getObject< DialogAsset >(0);
		rebuildGraph(nullptr);
		select(m_dialog);
	}
	else if (command == L"Dialog.Editor.CreateLine")
	{
		const ui::Size size = m_graph->getInnerRect().getSize();
		createNode(ui::Point(size.cx / 2, size.cy / 2) - m_graph->getOffset());
	}
	else if (command == L"Dialog.Editor.AddResponse")
	{
		const RefArray< ui::Node > nodes = m_graph->getSelectedNodes();
		DialogNodeAsset* nodeAsset = (nodes.size() == 1) ? nodes.front()->getData< DialogNodeAsset >(L"NODE") : nullptr;
		if (nodeAsset)
			addResponse(nodeAsset);
		else
			addStart();
	}
	else if (command == L"Dialog.Editor.AddStart")
		addStart();
	else if (command == L"Dialog.Editor.Arrange")
	{
		m_document->push();
		arrange();
		rebuildGraph(nullptr);
	}
	else if (command == L"Dialog.Editor.AlignLeft" || command == L"Dialog.Editor.AlignRight" || command == L"Dialog.Editor.AlignTop" || command == L"Dialog.Editor.AlignBottom")
	{
		ui::AlignNodesLayoutOperation::Alignment alignment = ui::AlignNodesLayoutOperation::AnLeft;
		if (command == L"Dialog.Editor.AlignRight")
			alignment = ui::AlignNodesLayoutOperation::AnRight;
		else if (command == L"Dialog.Editor.AlignTop")
			alignment = ui::AlignNodesLayoutOperation::AnTop;
		else if (command == L"Dialog.Editor.AlignBottom")
			alignment = ui::AlignNodesLayoutOperation::AnBottom;

		m_document->push();
		const ui::AlignNodesLayoutOperation op(alignment);
		m_graph->apply(&op);
		syncPositions();
	}
	else if (command == L"Dialog.Editor.EvenSpaceVertically" || command == L"Dialog.Editor.EvenSpaceHorizontally")
	{
		m_document->push();
		const ui::EvenSpaceLayoutOperation op(command == L"Dialog.Editor.EvenSpaceVertically" ? ui::EvenSpaceLayoutOperation::EsVertically : ui::EvenSpaceLayoutOperation::EsHorizontally);
		m_graph->apply(&op);
		syncPositions();
	}
	else
		return false;

	m_graph->update();
	return true;
}

void DialogEditorPage::handleDatabaseEvent(db::Database* database, const Guid& eventId)
{
}

void DialogEditorPage::rebuildGraph(const ISerializable* select)
{
	m_graph->removeAllEdges();
	m_graph->removeAllNodes();

	// The start, with a pin for every entry.
	ui::Node* startNode = m_graph->createNode(
		i18n::Text(L"STORY_DIALOG_START"),
		m_dialog->name,
		toUnit(m_dialog->startPosition),
		new ui::DefaultNodeShape(ui::DefaultNodeShape::StDefault)
	);
	startNode->setData(L"DIALOG", m_dialog);
	for (int32_t i = 0; i < (int32_t)m_dialog->start.size(); ++i)
		startNode->createOutputPin(startLabel(i, m_dialog->start[i]), Guid());
	startNode->setSelected(select == m_dialog);

	// A node for every line, with a pin for every response.
	SmallMap< Guid, ui::Node* > nodes;
	for (auto nodeAsset : m_dialog->nodes)
	{
		ui::Node* node = m_graph->createNode(
			lineTitle(nodeAsset),
			excerpt(nodeAsset->text, c_maxExcerpt),
			toUnit(nodeAsset->position),
			new ui::DefaultNodeShape(nodeAsset->responses.empty() ? ui::DefaultNodeShape::StExternal : ui::DefaultNodeShape::StDefault)
		);
		node->setData(L"NODE", nodeAsset);
		node->createInputPin(L"Enter", Guid(), false, false);
		for (int32_t i = 0; i < (int32_t)nodeAsset->responses.size(); ++i)
		{
			const DialogResponseAsset* response = nodeAsset->responses[i];
			node->createOutputPin(responseLabel(i, response->text, response->condition != nullptr, response->once, !response->actions.empty()), Guid());
		}
		node->setSelected(select == nodeAsset);
		nodes.insert(nodeAsset->id, node);
	}

	auto connect = [&](ui::Pin* from, const Guid& to, const wchar_t* key, Object* data) {
		const auto it = nodes.find(to);
		if (it == nodes.end())
			return;
		Ref< ui::Edge > edge = new ui::Edge(from, it->second->findInputPin(L"Enter"));
		edge->setData(key, data);
		m_graph->addEdge(edge);
	};

	for (int32_t i = 0; i < (int32_t)m_dialog->start.size(); ++i)
		connect(startNode->getOutputPin(i), m_dialog->start[i]->node, L"START", m_dialog->start[i]);

	for (auto nodeAsset : m_dialog->nodes)
	{
		ui::Node* node = nodes[nodeAsset->id];
		for (int32_t i = 0; i < (int32_t)nodeAsset->responses.size(); ++i)
		{
			DialogResponseAsset* response = nodeAsset->responses[i];
			if (response->target.isNotNull())
				connect(node->getOutputPin(i), response->target, L"RESPONSE", response);
		}
	}

	m_graph->update();

	// Conditions may have been edited; the dialog panel lists the items they now check.
	testUpdateItems();
}

void DialogEditorPage::syncPositions()
{
	for (auto node : m_graph->getNodes())
	{
		if (auto nodeAsset = node->getData< DialogNodeAsset >(L"NODE"))
			nodeAsset->position = fromUnit(node->getPosition());
		else if (node->getData< DialogAsset >(L"DIALOG"))
			m_dialog->startPosition = fromUnit(node->getPosition());
	}
}

void DialogEditorPage::arrange()
{
	// Columns by how many responses away from the start a line is; lines nothing leads to last.
	SmallMap< Guid, int32_t > depth;
	std::deque< Guid > queue;

	for (auto start : m_dialog->start)
	{
		if (m_dialog->findNode(start->node) && depth.insert(start->node, 0))
			queue.push_back(start->node);
	}
	if (queue.empty() && !m_dialog->nodes.empty())
	{
		depth.insert(m_dialog->nodes.front()->id, 0);
		queue.push_back(m_dialog->nodes.front()->id);
	}

	int32_t maxDepth = 0;
	while (!queue.empty())
	{
		const Guid id = queue.front();
		queue.pop_front();

		const DialogNodeAsset* nodeAsset = m_dialog->findNode(id);
		if (!nodeAsset)
			continue;

		const int32_t d = depth[id];
		for (auto response : nodeAsset->responses)
		{
			if (m_dialog->findNode(response->target) && depth.find(response->target) == depth.end())
			{
				depth.insert(response->target, d + 1);
				maxDepth = std::max(maxDepth, d + 1);
				queue.push_back(response->target);
			}
		}
	}

	AlignedVector< int32_t > columnY;
	columnY.resize(maxDepth + 2, 0);
	for (auto nodeAsset : m_dialog->nodes)
	{
		const auto it = depth.find(nodeAsset->id);
		const int32_t column = (it != depth.end()) ? it->second : maxDepth + 1;
		nodeAsset->position = Vector2((float)((column + 1) * c_columnWidth), (float)columnY[column]);
		columnY[column] += c_lineHeight + (int32_t)nodeAsset->responses.size() * c_pinHeight;
	}

	m_dialog->startPosition = Vector2(0.0f, 0.0f);
}

void DialogEditorPage::createNode(const ui::Point& at)
{
	m_document->push();

	std::wstring name;
	for (int32_t i = 1; name.empty() || m_dialog->findNodeByName(name) != nullptr; ++i)
		name = L"line" + toString(i);

	Ref< DialogNodeAsset > nodeAsset = new DialogNodeAsset();
	nodeAsset->id = Guid::create();
	nodeAsset->name = name;
	nodeAsset->position = Vector2((float)at.x, (float)at.y);
	m_dialog->nodes.push_back(nodeAsset);

	// The first line is where a conversation starts.
	if (m_dialog->start.empty())
	{
		Ref< DialogStartAsset > start = new DialogStartAsset();
		start->node = nodeAsset->id;
		m_dialog->start.push_back(start);
	}

	rebuildGraph(nodeAsset);
	select(nodeAsset);
}

void DialogEditorPage::addResponse(DialogNodeAsset* node)
{
	m_document->push();

	Ref< DialogResponseAsset > response = new DialogResponseAsset();
	response->text = L"...";
	node->responses.push_back(response);

	rebuildGraph(node);
	select(node);
}

void DialogEditorPage::addStart()
{
	m_document->push();

	Ref< DialogStartAsset > start = new DialogStartAsset();
	if (!m_dialog->nodes.empty())
		start->node = m_dialog->nodes.front()->id;
	m_dialog->start.push_back(start);

	rebuildGraph(m_dialog);
	select(m_dialog);
}

void DialogEditorPage::copySelection(bool cut)
{
	Ref< DialogClipboardData > data = new DialogClipboardData();
	for (auto node : m_graph->getSelectedNodes())
	{
		if (auto nodeAsset = node->getData< DialogNodeAsset >(L"NODE"))
			data->nodes.push_back(DeepClone(nodeAsset).create< DialogNodeAsset >());
	}
	if (data->nodes.empty())
		return;

	ui::Application::getInstance()->getClipboard()->setObject(data);

	if (cut)
		deleteSelection();
}

void DialogEditorPage::paste()
{
	Ref< DialogClipboardData > data = dynamic_type_cast< DialogClipboardData* >(ui::Application::getInstance()->getClipboard()->getObject());
	if (!data || data->nodes.empty())
		return;

	m_document->push();

	// Pasted lines are new lines: new guids, links among them kept, unique names.
	SmallMap< Guid, Guid > remap;
	for (auto nodeAsset : data->nodes)
		remap.insert(nodeAsset->id, Guid::create());

	Vector2 topLeft(std::numeric_limits< float >::max(), std::numeric_limits< float >::max());
	for (auto nodeAsset : data->nodes)
		topLeft = Vector2(std::min(topLeft.x, nodeAsset->position.x), std::min(topLeft.y, nodeAsset->position.y));

	const ui::Size size = m_graph->getInnerRect().getSize();
	const ui::Point center = ui::Point(size.cx / 2, size.cy / 2) - m_graph->getOffset();
	const Vector2 at((float)(center.x + c_pasteOffset), (float)(center.y + c_pasteOffset));

	SmallSet< const DialogNodeAsset* > pasted;
	for (auto nodeAsset : data->nodes)
	{
		nodeAsset->id = remap[nodeAsset->id];
		if (!nodeAsset->name.empty())
		{
			const std::wstring base = nodeAsset->name;
			for (int32_t i = 2; m_dialog->findNodeByName(nodeAsset->name) != nullptr; ++i)
				nodeAsset->name = base + L"_" + toString(i);
		}
		for (auto response : nodeAsset->responses)
		{
			response->id = Guid::create();
			const auto it = remap.find(response->target);
			if (it != remap.end())
				response->target = it->second;
			else if (!m_dialog->findNode(response->target))
				response->target = Guid();
		}
		nodeAsset->position = at + (nodeAsset->position - topLeft);
		m_dialog->nodes.push_back(nodeAsset);
		pasted.insert(nodeAsset);
	}

	rebuildGraph(nullptr);
	for (auto node : m_graph->getNodes())
		node->setSelected(pasted.find(node->getData< DialogNodeAsset >(L"NODE")) != pasted.end());
	select(m_dialog);

	// The clipboard keeps its own copy, so the same lines can be pasted again.
	ui::Application::getInstance()->getClipboard()->setObject(DeepClone(data).create());
}

void DialogEditorPage::deleteSelection()
{
	const RefArray< ui::Node > nodes = m_graph->getSelectedNodes();
	const RefArray< ui::Edge > edges = m_graph->getSelectedEdges();
	if (nodes.empty() && edges.empty())
		return;

	m_document->push();

	// Lines deleted; what led to them leads nowhere now.
	SmallSet< Guid > deleted;
	for (auto node : nodes)
	{
		DialogNodeAsset* nodeAsset = node->getData< DialogNodeAsset >(L"NODE");
		if (nodeAsset)
		{
			deleted.insert(nodeAsset->id);
			m_dialog->nodes.remove(nodeAsset);
		}
	}

	for (auto edge : edges)
	{
		if (auto response = edge->getData< DialogResponseAsset >(L"RESPONSE"))
			response->target = Guid();
		else if (auto start = edge->getData< DialogStartAsset >(L"START"))
			start->node = Guid();
	}

	for (auto nodeAsset : m_dialog->nodes)
	{
		for (auto response : nodeAsset->responses)
		{
			if (deleted.find(response->target) != deleted.end())
				response->target = Guid();
		}
	}
	for (auto start : m_dialog->start)
	{
		if (deleted.find(start->node) != deleted.end())
			start->node = Guid();
	}

	rebuildGraph(nullptr);
	select(m_dialog);
}

void DialogEditorPage::select(ISerializable* object)
{
	m_propertiesView->setPropertyObject(object);
}

void DialogEditorPage::testUpdateItems()
{
	AlignedVector< Guid > items;
	for (auto startAsset : m_dialog->start)
		collectItems(startAsset->condition, items);
	for (auto node : m_dialog->nodes)
	{
		for (auto response : node->responses)
			collectItems(response->condition, items);
	}

	// Rebuilt only as the items change, never while a checkbox is being clicked.
	if (!m_testItemWidgets.empty() && items.size() == m_testItemIds.size())
	{
		bool same = true;
		for (const auto& item : items)
			same &= std::find(m_testItemIds.begin(), m_testItemIds.end(), item) != m_testItemIds.end();
		if (same)
			return;
	}

	// Items listed by name, as they are named to the player.
	Ref< db::Database > database = m_editor->getSourceDatabase();
	AlignedVector< std::pair< std::wstring, Guid > > named;
	for (const auto& item : items)
	{
		std::wstring name;
		if (Ref< ItemData > itemData = database->getObjectReadOnly< ItemData >(item))
			name = itemData->getName();
		if (name.empty())
		{
			Ref< db::Instance > instance = database->getInstance(item);
			name = instance ? instance->getName() : item.format();
		}
		named.push_back({ name, item });
	}
	std::stable_sort(named.begin(), named.end(), [](const auto& lh, const auto& rh) { return lh.first < rh.first; });

	for (size_t i = 0; i < m_testItemWidgets.size(); ++i)
		m_testItemWidgets[i]->destroy();
	m_testItemWidgets.clear();
	m_testItemIds.clear();

	for (const auto& it : named)
	{
		Ref< ui::CheckBox > checkBox = new ui::CheckBox();
		checkBox->create(m_testItems, it.first, m_testHeld.find(it.second) != m_testHeld.end());
		checkBox->addEventHandler< ui::ButtonClickEvent >(this, &DialogEditorPage::eventTestItem);
		m_testItemWidgets.push_back(checkBox);
		m_testItemIds.push_back(it.second);
	}

	if (m_testItemIds.empty())
	{
		Ref< ui::Static > note = new ui::Static();
		note->create(m_testItems, i18n::Text(L"STORY_DIALOG_TEST_NO_ITEMS"));
		m_testItemWidgets.push_back(note);
	}

	m_containerTest->update();
}

bool DialogEditorPage::testEvaluate(const ICondition* condition) const
{
	if (!condition)
		return true;

	if (auto hasItemCondition = dynamic_type_cast< const HasItemCondition* >(condition))
	{
		const bool held = m_testHeld.find(hasItemCondition->getItem()) != m_testHeld.end();
		return held != hasItemCondition->getNegate();
	}
	else if (auto allCondition = dynamic_type_cast< const AllCondition* >(condition))
	{
		for (auto child : allCondition->getConditions())
		{
			if (!testEvaluate(child))
				return false;
		}
		return true;
	}
	else if (auto anyCondition = dynamic_type_cast< const AnyCondition* >(condition))
	{
		for (auto child : anyCondition->getConditions())
		{
			if (child != nullptr && testEvaluate(child))
				return true;
		}
		return false;
	}

	// Quests, flags and scripts are not known here; taken to hold.
	return true;
}

bool DialogEditorPage::testExecute(const IAction* action, std::wstring& outNotice)
{
	outNotice += (!outNotice.empty() ? L"\n! " : L"! ") + describe(action, m_editor->getSourceDatabase());

	// Items given and taken are checked and unchecked, so later lines see them.
	bool changed = false;
	if (auto giveItemAction = dynamic_type_cast< const GiveItemAction* >(action))
		changed = m_testHeld.insert(giveItemAction->getItem());
	else if (auto takeItemAction = dynamic_type_cast< const TakeItemAction* >(action))
		changed = m_testHeld.erase(takeItemAction->getItem());

	if (changed)
	{
		for (size_t i = 0; i < m_testItemIds.size(); ++i)
		{
			if (auto checkBox = dynamic_type_cast< ui::CheckBox* >(m_testItemWidgets[i]))
				checkBox->setChecked(m_testHeld.find(m_testItemIds[i]) != m_testHeld.end());
		}
	}

	auto eventAction = dynamic_type_cast< const EventAction* >(action);
	return eventAction != nullptr && eventAction->getEndConversation();
}

void DialogEditorPage::testRestart()
{
	m_testChat->removeAllMessages();
	m_testNode = -1;
	m_testSaid.clear();
	testUpdateItems();

	m_testDialog = DialogPipeline::compile(m_dialog, L"test");
	if (!m_testDialog)
	{
		testAppend(i18n::Text(L"STORY_DIALOG_TEST_CANNOT_COMPILE"));
		testEnter(-1);
		return;
	}

	testAppend(i18n::Text(L"STORY_DIALOG_TEST_NO_CONDITIONS"));

	// The first start whose condition holds, as when the actor is talked to.
	for (const auto& start : m_testDialog->getStart())
	{
		if (testEvaluate(start.condition))
		{
			testEnter(start.node);
			return;
		}
	}

	testAppend(i18n::Text(L"STORY_DIALOG_TEST_NO_START"));
	testEnter(-1);
}

void DialogEditorPage::testEnter(int32_t node)
{
	m_testNode = (m_testDialog && node >= 0 && node < (int32_t)m_testDialog->getNodes().size()) ? node : -1;

	bool ended = false;
	if (m_testNode >= 0)
	{
		const DialogNode* dialogNode = m_testDialog->getNodes()[m_testNode];
		const std::wstring speaker = !m_testDialog->getName().empty() ? m_testDialog->getName() : std::wstring(L"ACTOR");
		const std::wstring line = !dialogNode->getName().empty() ? dialogNode->getName() : std::wstring(i18n::Text(L"STORY_DIALOG_UNNAMED"));
		m_testChat->addMessage(ui::ChatView::Speaker::Assistant, speaker + L" (" + line + L")\n" + dialogNode->getText());

		std::wstring notice;
		for (auto action : dialogNode->getOnEnter())
			ended |= testExecute(action, notice);
		if (!notice.empty())
			testAppend(notice);
	}

	if (ended)
		m_testNode = -1;

	testShowResponses();

	if (ended || (m_testNode >= 0 && m_testShown.empty()))
		testAppend(i18n::Text(L"STORY_DIALOG_TEST_ENDED"));
}

void DialogEditorPage::testShowResponses()
{
	m_testResponses->removeAll();
	m_testShown.clear();

	if (m_testDialog && m_testNode >= 0)
	{
		const auto& responses = m_testDialog->getNodes()[m_testNode]->getResponses();
		for (int32_t i = 0; i < (int32_t)responses.size(); ++i)
		{
			const DialogResponse* response = responses[i];
			if (response->isOnce() && m_testSaid.find({ m_testNode, i }) != m_testSaid.end())
				continue;
			if (!testEvaluate(response->getCondition()))
				continue;

			m_testResponses->add(responseLabel((int32_t)m_testShown.size(), response->getText(), response->getCondition() != nullptr, response->isOnce(), !response->getActions().empty()));
			m_testShown.push_back(i);
		}
	}

	m_testResponses->update();
}

void DialogEditorPage::testAppend(const std::wstring& text)
{
	m_testChat->addMessage(ui::ChatView::Speaker::Notice, text);
}

bool DialogEditorPage::testChoose(const ui::Point& position)
{
	int32_t row = -1;
	for (int32_t i = 0; i < m_testResponses->count(); ++i)
	{
		if (m_testResponses->getItemRect(i).offset(m_testResponses->getScrollOffset()).inside(position))
			row = i;
	}
	if (row < 0 || row >= (int32_t)m_testShown.size() || !m_testDialog || m_testNode < 0)
		return false;

	const int32_t index = m_testShown[row];
	const DialogResponse* response = m_testDialog->getNodes()[m_testNode]->getResponses()[index];
	m_testChat->addMessage(ui::ChatView::Speaker::User, response->getText());

	if (response->isOnce())
		m_testSaid.insert({ m_testNode, index });

	bool ended = false;
	std::wstring notice;
	for (auto action : response->getActions())
		ended |= testExecute(action, notice);
	if (!notice.empty())
		testAppend(notice);

	if (ended || response->getTarget() < 0)
	{
		testAppend(i18n::Text(L"STORY_DIALOG_TEST_ENDED"));
		testEnter(-1);
	}
	else
		testEnter(response->getTarget());

	return true;
}

void DialogEditorPage::eventToolBarClick(ui::ToolBarButtonClickEvent* event)
{
	handleCommand(event->getCommand());
}

void DialogEditorPage::eventButtonDown(ui::MouseButtonDownEvent* event)
{
	if (event->getButton() != ui::MbtRight)
		return;

	const ui::MenuItem* selected = m_menuPopup->showModal(m_graph, event->getPosition());
	if (!selected)
		return;

	if (selected->getCommand() == L"Dialog.Editor.CreateLine")
		createNode(event->getPosition() - m_graph->getOffset());
	else
		handleCommand(selected->getCommand());
}

void DialogEditorPage::eventSelect(ui::SelectionChangeEvent* event)
{
	const RefArray< ui::Node > nodes = m_graph->getSelectedNodes();
	const RefArray< ui::Edge > edges = m_graph->getSelectedEdges();

	if (nodes.size() == 1)
	{
		if (auto nodeAsset = nodes[0]->getData< DialogNodeAsset >(L"NODE"))
			select(nodeAsset);
		else
			select(m_dialog);
	}
	else if (edges.size() == 1)
	{
		if (auto response = edges[0]->getData< DialogResponseAsset >(L"RESPONSE"))
			select(response);
		else if (auto start = edges[0]->getData< DialogStartAsset >(L"START"))
			select(start);
		else
			select(m_dialog);
	}
	else
		select(m_dialog);
}

void DialogEditorPage::eventNodeMoved(ui::NodeMovedEvent* event)
{
	ui::Node* node = event->getNode();
	if (auto nodeAsset = node->getData< DialogNodeAsset >(L"NODE"))
		nodeAsset->position = fromUnit(node->getPosition());
	else if (node->getData< DialogAsset >(L"DIALOG"))
		m_dialog->startPosition = fromUnit(node->getPosition());
}

void DialogEditorPage::eventEdgeConnect(ui::EdgeConnectEvent* event)
{
	ui::Edge* edge = event->getEdge();
	ui::Pin* fromPin = edge->getSourcePin();
	ui::Pin* toPin = edge->getDestinationPin();

	DialogNodeAsset* target = toPin->getNode()->getData< DialogNodeAsset >(L"NODE");
	if (!target)
		return;

	ui::Node* fromNode = fromPin->getNode();
	const int32_t index = indexOf(fromNode->getOutputPins(), fromPin);
	if (index < 0)
		return;

	m_document->push();

	if (auto nodeAsset = fromNode->getData< DialogNodeAsset >(L"NODE"))
	{
		DialogResponseAsset* response = nodeAsset->responses[index];
		response->target = target->id;
		edge->setData(L"RESPONSE", response);
	}
	else if (fromNode->getData< DialogAsset >(L"DIALOG"))
	{
		DialogStartAsset* start = m_dialog->start[index];
		start->node = target->id;
		edge->setData(L"START", start);
	}
	else
		return;

	// A response leads to one line only; the edge it had is replaced.
	for (auto other : m_graph->getConnectedEdges(fromPin))
	{
		if (other->getSourcePin() == fromPin)
			m_graph->removeEdge(other);
	}

	m_graph->addEdge(edge);
	m_graph->update();
}

void DialogEditorPage::eventEdgeDisconnect(ui::EdgeDisconnectEvent* event)
{
	ui::Edge* edge = event->getEdge();

	m_document->push();

	if (auto response = edge->getData< DialogResponseAsset >(L"RESPONSE"))
		response->target = Guid();
	else if (auto start = edge->getData< DialogStartAsset >(L"START"))
		start->node = Guid();

	m_graph->update();
}

void DialogEditorPage::eventPropertiesChanged(ui::ContentChangeEvent* event)
{
	// Names, texts, responses and start entries may have changed; titles and pins follow.
	const RefArray< ui::Node > nodes = m_graph->getSelectedNodes();
	const ISerializable* selected = nullptr;
	if (nodes.size() == 1)
	{
		if (auto nodeAsset = nodes[0]->getData< DialogNodeAsset >(L"NODE"))
			selected = nodeAsset;
		else
			selected = m_dialog;
	}
	rebuildGraph(selected);
}

void DialogEditorPage::eventTestRestart(ui::ButtonClickEvent* event)
{
	testRestart();
}

void DialogEditorPage::eventTestItem(ui::ButtonClickEvent* event)
{
	for (size_t i = 0; i < m_testItemIds.size(); ++i)
	{
		if (m_testItemWidgets[i] != event->getSender())
			continue;

		if (static_cast< const ui::CheckBox* >(m_testItemWidgets[i].ptr())->isChecked())
			m_testHeld.insert(m_testItemIds[i]);
		else
			m_testHeld.erase(m_testItemIds[i]);
	}

	// The current line's responses are offered anew with the items now held.
	testShowResponses();
}

void DialogEditorPage::eventTestResponse(ui::MouseButtonDownEvent* event)
{
	if (event->getButton() == ui::MbtLeft && testChoose(event->getPosition()))
		event->consume();
}

void DialogEditorPage::eventTestResponseDoubleClick(ui::MouseDoubleClickEvent* event)
{
	if (event->getButton() == ui::MbtLeft && testChoose(event->getPosition()))
		event->consume();
}

}
