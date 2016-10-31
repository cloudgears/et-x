/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>
#include <et-ext/scene2d/font.h>

namespace et
{
namespace s2d
{
class SceneRenderer;

class TextElement : public Element2d
{
public:
	ET_DECLARE_POINTER(TextElement);
	
	enum class TextStyle : uint32_t
	{
		SignedDistanceField,
		SignedDistanceFieldShadow,
		SignedDistanceFieldBevel,
		SignedDistanceFieldInverseBevel,
		Plain,
		max
	};

	const float DefaultFontSmoothing = 0.5f;
	
public:
	TextElement(Element2d*, const Font::Pointer&, float, const std::string& = emptyString);
	
	Font::Pointer& font()
		{ return _font; }
	
	const Font::Pointer& font() const
		{ return _font; }
	
	float fontSize() const
		{ return _fontSize.value(); }

	float fontSmoothing() const
		{ return _fontSmoothing.value(); }
	
	void setFont(const Font::Pointer&);
	void setFontSize(float, float duration = 0.0f);
	void setFontSmoothing(float, float duration = 0.0f);
	void setTextStyle(TextStyle);
	
	void loadProperties(const Dictionary&) override;
	
	SceneProgram& textProgram(SceneRenderer&);
	
	void setShadowOffset(const vec2&);
	
	Alignment textHorizontalAlignment() const
		{ return _horizontalAlignment; }
	
	Alignment textVerticalAlignment() const
		{ return _verticalAlignment; }
	
	void setTextHorizontalAlignment(Alignment);
	void setTextVerticalAlignment(Alignment);
	void setTextAlignment(Alignment horzontal, Alignment vertical);
	
protected:
	bool processMessage(const Message&) override;
	void setProgramParameters(et::RenderContext*, et::Program::Pointer&) override;
	
	virtual void invalidateText() { }
	
	void initTextProgram(SceneRenderer&);
	
private:
	Font::Pointer _font;
	FloatAnimator _fontSize;
	FloatAnimator _fontSmoothing;
	SceneProgram _textProgram;
	// TODO : FIX
	// Program::Uniform _shadowUniform;
	vec2 _shadowOffset;
	TextStyle _textStyle = TextStyle::SignedDistanceField;
	Alignment _horizontalAlignment = Alignment_Near;
	Alignment _verticalAlignment = Alignment_Near;
};
}
}
