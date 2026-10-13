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
#include "Core/Ref.h"
#include "Ui/ConfigDialog.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::ui
{

class ListBox;
class MouseDoubleClickEvent;

}

namespace zombie
{

using namespace traktor;

/*! Pick one of a list of names, such as quest steps or dialog lines.
 * \ingroup Zombie
 */
class T_DLLCLASS BrowsePartDialog : public ui::ConfigDialog
{
	T_RTTI_CLASS;

public:
	bool create(ui::Widget* parent, const std::wstring& title, const AlignedVector< std::wstring >& names, int32_t selected);

	/*! Index of the name picked, -1 if none. */
	int32_t getSelected() const;

private:
	Ref< ui::ListBox > m_list;

	void eventDoubleClick(ui::MouseDoubleClickEvent* event);
};

}
