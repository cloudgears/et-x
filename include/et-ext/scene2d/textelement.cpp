/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/textelement.h>

namespace et {
namespace s2d {

const float maxShadowDistance = 1.0f;

TextElement::TextElement(Element2d* parent, const Font::Pointer& f, float fsz, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _font(f), _fontSize(timerPool()), _fontSmoothing(timerPool())
{
	_fontSize.setValue(fsz);
	_fontSize.updated.connect(this, &TextElement::invalidateText);

	_fontSmoothing.setValue(DefaultFontSmoothing);
	_fontSmoothing.updated.connect(this, &TextElement::invalidateText);
	
	setFlag(s2d::Flag_DynamicRendering);
}

void TextElement::setFont(const Font::Pointer& f)
{
	_font = f;
	if (_font.valid() && _textMaterial.valid())
	{
		_textMaterial->setTexture(MaterialTexture::Albedo, _font->generator()->texture());
	}
	invalidateText();
}

void TextElement::setFontSize(float fsz, float duration)
{
	_fontSize.animate(fsz, duration);
}

void TextElement::setFontSmoothing(float fsm, float duration)
{
	_fontSmoothing.animate(fsm, duration);
}

bool TextElement::processMessage(const Message& msg)
{
	if (msg.type == Message::Type_SetFontSmoothing)
	{
		setFontSmoothing(msg.paramf);
		return true;
	}
	
	return false;
}

void TextElement::loadProperties(const Dictionary& d)
{
	if (d.hasKey("font_size"))
	{
		auto obj = d.objectForKey("font_size");
		
		if (obj->variantClass() == VariantClass::Float)
			setFontSize(FloatValue(obj)->content);
		else if (obj->variantClass() == VariantClass::Integer)
			setFontSize(static_cast<float>(IntegerValue(obj)->content));
	}
	
	if (d.hasKey("font_smoothing"))
	{
		auto obj = d.objectForKey("font_smoothing");
		
		if (obj->variantClass() == VariantClass::Float)
			setFontSmoothing(FloatValue(obj)->content);
		else if (obj->variantClass() == VariantClass::Integer)
			setFontSmoothing(static_cast<float>(IntegerValue(obj)->content));
	}
	
	if (d.hasKey("font_style"))
	{
		auto obj = d.objectForKey("font_style");
		if (obj->variantClass() == VariantClass::String)
		{
			setTextStyle(TextStyle::SignedDistanceField);
		}
	}
}

void TextElement::setTextStyle(TextStyle style)
{
	if (style != _textStyle)
	{
		_textStyle = style;
		_textMaterial.reset(nullptr);
	}
}

void TextElement::setShadowOffset(const vec2& o)
{
	_shadowOffset.x = +(std::abs(o.x) > maxShadowDistance ? maxShadowDistance : o.x);
	_shadowOffset.y = -(std::abs(o.y) > maxShadowDistance ? maxShadowDistance : o.y);
}

/*
 * TODO : text parameters here
 *
void TextElement::setProgramParameters(et::RenderContext*, et::Program::Pointer& p)
{
	if ((_textStyle == TextStyle::SignedDistanceFieldShadow) && (p == _textProgram.program))
		p->setUniform(_shadowUniform, _shadowOffset * _font->generator()->texture()->texel());
}
// */

void TextElement::setTextHorizontalAlignment(Alignment a)
{
	_horizontalAlignment = a;
	invalidateContent();
}

void TextElement::setTextVerticalAlignment(Alignment a)
{
	_verticalAlignment = a;
	invalidateContent();
}

void TextElement::setTextAlignment(Alignment h, Alignment v)
{
	_horizontalAlignment = h;
	_verticalAlignment = v;
	invalidateContent();
}

MaterialInstance::Pointer TextElement::textMaterial(SceneRenderer& r)
{
	if (_textMaterial.invalid())
	{
		_textMaterial = r.fontMaterial()->instance();
		if (_font.valid())
		{
			_textMaterial->setTexture(MaterialTexture::Albedo, _font->generator()->texture());
		}
	}
	return _textMaterial;
}

}
}
