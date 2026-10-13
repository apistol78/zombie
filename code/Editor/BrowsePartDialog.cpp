/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/BrowsePartDialog.h"

#include "Ui/Events/MouseDoubleClickEvent.h"
#include "Ui/FloodLayout.h"
#include "Ui/ListBox/ListBox.h"
#include "Ui/StyleBitmap.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.BrowsePartDialog", BrowsePartDialog, ui::ConfigDialog)

bool BrowsePartDialog::create(ui::Widget* parent, const std::wstring& title, const AlignedVector< std::wstring >& names, int32_t selected)
{
	if (!ui::ConfigDialog::create(parent, title, 500_ut, 500_ut, ui::ConfigDialog::WsCenterParent | ui::ConfigDialog::WsDefaultResizable, new ui::FloodLayout()))
		return false;

	setIcon(new ui::StyleBitmap(L"Editor.Icon"));

	m_list = new ui::ListBox();
	m_list->create(this, ui::ListBox::WsSingle);
	for (const auto& name : names)
		m_list->add(name);
	if (selected >= 0)
		m_list->select(selected);
	m_list->addEventHandler< ui::MouseDoubleClickEvent >(this, &BrowsePartDialog::eventDoubleClick);

	update();
	return true;
}

int32_t BrowsePartDialog::getSelected() const
{
	return m_list->getSelected();
}

void BrowsePartDialog::eventDoubleClick(ui::MouseDoubleClickEvent* event)
{
	if (m_list->getSelected() >= 0)
		endModal(ui::DialogResult::Ok);
}

}
