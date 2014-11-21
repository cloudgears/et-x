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
	invalidateText();
}

void TextElement::setFontSize(float fsz)
{
	_fontSize = fsz;
	invalidateText();
}

void TextElement::setFontSmoothing(float fsm)
{
	_fontSmoothing = fsm;
	invalidateText();
}

void TextElement::processMessage(const Message& msg)
{
	if (msg.type == Message::Type_SetFontSmoothing)
		setFontSmoothing(msg.paramf);
}

void TextElement::loadProperties(const Dictionary& d)
{
	if (d.hasKey("font_size"))
		setFontSize(d.floatForKey("font_size")->content);
	
	if (d.hasKey("font_smoothing"))
		setFontSmoothing(d.floatForKey("font_smoothing")->content);
}
