/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/gui/table.h>

using namespace et;
using namespace et::gui;

ET_DECLARE_GUI_ELEMENT_CLASS(Scroll)

Table::Table(et::gui::Element2d* parent, const std::string& name) :
	Scroll(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS)
{
	setBackgroundColor(vec4(1.0f));
}