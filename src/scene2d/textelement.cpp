/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/textelement.h>

using namespace et;
using namespace s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(TextElement)

TextElement::TextElement(Element2d* parent, const Font::Pointer& f, float fsz, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _font(f), _fontSize(fsz)
{
}

void TextElement::setFont(const Font::Pointer& f)
{
	_font = f;
	invalidateContent();
}

void TextElement::setFontSize(float fsz)
{
	_fontSize = fsz;
	invalidateContent();
}
