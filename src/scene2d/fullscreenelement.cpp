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
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS)
{
	setAutolayout(vec2(0.0f), LayoutMode_Absolute, vec2(1.0f), LayoutMode_RelativeToContext, vec2(0.0f));
}
