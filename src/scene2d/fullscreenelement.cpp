/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/fullscreenelement.h>

using namespace et;
using namespace et::s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(FullscreenElement)

FullscreenElement::FullscreenElement(Element* parent, const std::string& name) :
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS)
{
	setFlag(Flag_TransparentForPointer);
}

void FullscreenElement::layout(const vec2& sz)
{
	setFrame(vec2(0.0f), sz);
	autoLayout(sz);
	layoutChildren();
}
