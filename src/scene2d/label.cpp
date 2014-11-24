/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/tools.h>
#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/label.h>

using namespace et;
using namespace et::s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(Label)

Label::Label(const std::string& text, const Font::Pointer& f, float fsz, Element2d* parent, const std::string& name) :
	TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS), _vertices(0), _backgroundColor(0.0f),
	_shadowOffset(1.0f), _textFade(0.0f), _textFadeDuration(0.0f), _textFadeStartTime(0.0f), _lineInterval(1.0f),
	_horizontalAlignment(Alignment_Near), _verticalAlignment(Alignment_Near), _animatingText(false)
{
	setFlag(Flag_TransparentForPointer);
	
	_text.setKey(text);
	_nextText.setKey(text);
	
	invalidateText();
	
	adjustSize();
}

void Label::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	if (!contentValid() || !transformValid())
		buildVertices(rc, r);

	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, Texture(), r.defaultProgram(), this);
	
	if (_vertices.lastElementIndex() > 0)
		r.addVertices(_vertices, font()->generator()->texture(), textProgram(r), this);
}

void Label::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	vec2 alignment = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment));
	vec2 textOffset = size() * alignment;
	
	_vertices.setOffset(0);
	_backgroundVertices.setOffset(0);
	
	bool hasShadow = _shadowColor.w > std::numeric_limits<float>::epsilon();
	vec4 shadowColor = _shadowColor;
	
	vec4 finalColorValue = finalColor();
	vec4 alphaScale = vec4(1.0f, finalColorValue.w);

	if (_backgroundColor.w > std::numeric_limits<float>::epsilon())
		buildColorVertices(_backgroundVertices, rect(vec2(0.0f), size()), _backgroundColor * alphaScale, transform);

	if (_charListText.empty() && _charListNextText.empty())
	{
		setContentValid();
		return;
	}

	if (_animatingText)
	{
		float fadeIn = sqrtf(_textFade);
		float fadeOut = 1.0f - sqr(_textFade);

		if (hasShadow)
		{
			shadowColor.w = _shadowColor.w * alphaScale.w * fadeOut;
			vec2 shadowOffset = textOffset + _shadowOffset;
			buildStringVertices(_vertices, _charListText, _horizontalAlignment, _verticalAlignment,
				shadowOffset, shadowColor, transform, _lineInterval);
			
			shadowColor.w = _shadowColor.w * alphaScale.w * fadeIn;
			buildStringVertices(_vertices, _charListNextText, _horizontalAlignment,
				_verticalAlignment, shadowOffset, shadowColor, transform, _lineInterval);
		}

		buildStringVertices(_vertices, _charListText, _horizontalAlignment, _verticalAlignment,
			textOffset, finalColorValue * vec4(1.0, fadeOut), transform, _lineInterval);
		
		buildStringVertices(_vertices, _charListNextText, _horizontalAlignment, _verticalAlignment,
			textOffset, finalColorValue * vec4(1.0, fadeIn), transform, _lineInterval);
	}
	else 
	{
		if (hasShadow)
		{
			shadowColor.w = _shadowColor.w * alphaScale.w;
			buildStringVertices(_vertices, _charListText, _horizontalAlignment,
				_verticalAlignment, textOffset + _shadowOffset, shadowColor, transform, _lineInterval);
		}

		buildStringVertices(_vertices, _charListText, _horizontalAlignment, _verticalAlignment, 
			textOffset, finalColorValue, transform, _lineInterval);
	}

	setContentValid();
}

void Label::setText(const std::string& text, float duration)
{
	if (duration == 0.0f)
	{
		_animatingText = false;
		cancelUpdates();
		
		_text.setKey(text);
		_nextText.setKey(text);
	}
	else 
	{
		startUpdates();

		if (_animatingText)
			_text = _nextText;
		
		_nextText.setKey(text);
		
		_nextTextSize = font()->measureStringSize(_nextText.cachedText, fontSize(), fontSmoothing());
		
		_textFade = 0.0f;
		_animatingText = true;
		_textFadeStartTime = actualTime();
		_textFadeDuration = duration;
	}
	
	invalidateText();
	adjustSize();
}

vec2 Label::textSize()
{
	if (!contentValid())
	{
		_textSize = font()->measureStringSize(_text.cachedText, fontSize(), fontSmoothing());

		if (_animatingText)
			_nextTextSize = font()->measureStringSize(_nextText.cachedText, fontSize(), fontSmoothing());
	}

	return _animatingText ? _nextTextSize : _textSize;
}

void Label::update(float t)
{
	_textFade = (t - _textFadeStartTime) / _textFadeDuration;
	
	if (_textFade >= 1.0f)
	{
		_textFade = 0.0f;
		_animatingText = false;
		
		_text = _nextText;
		_textSize = font()->measureStringSize(_text.cachedText, fontSize(), fontSmoothing());
		
		_nextText.clear();
		_charListNextText.clear();
		_nextTextSize = vec2(0.0f);
		
		adjustSize();
		cancelUpdates();
		invalidateText();
	}

	invalidateContent();
}

void Label::adjustSize()
{
	_textSize = font()->measureStringSize(_text.cachedText, fontSize(), fontSmoothing());
	
	if (_animatingText)
		_textSize = maxv(_textSize, font()->measureStringSize(_nextText.cachedText, fontSize(), fontSmoothing()));
	
	if (_autoAdjustSize)
		setSize(_textSize);
}

void Label::setHorizontalAlignment(Alignment h)
{
	if (_horizontalAlignment == h) return;
	
	_horizontalAlignment = h;
	invalidateContent();
}

void Label::setVerticalAlignment(Alignment v)
{
	if (_verticalAlignment == v) return;
	
	_verticalAlignment = v;
	invalidateContent();
}

void Label::setBackgroundColor(const vec4& color)
{
	_backgroundColor = color;
	invalidateContent();
}

void Label::setShadowColor(const vec4& color)
{
	_shadowColor = color;
	invalidateContent();
}

void Label::setShadowOffset(const vec2& offset)
{
	_shadowOffset = offset;
	invalidateContent();
}

void Label::fitToWidth(float w)
{
	if (_text.cachedText.empty()) return;
	
	setText(fitStringToWidthWithFont(_text.cachedText, font(), fontSize(), w));
}

vec2 Label::contentSize()
{
	return maxv(_nextTextSize, _textSize);
}

void Label::setLineInterval(float i)
{
	_lineInterval = i;
	invalidateContent();
}

void Label::processMessage(const Message& msg)
{
	TextElement::processMessage(msg);
	
	if (msg.type == Message::Type_SetText)
		setText(msg.text, msg.duration);
	else if (msg.type == Message::Type_UpdateText)
		setText(_text.key, msg.duration);
}

void Label::setShouldAutoAdjustSize(bool v)
{
	_autoAdjustSize = v;
}

/*
 * Service functions
 */
std::string Label::fitStringToWidthWithFont(std::string oldText, Font::Pointer font, float fontSize, float width)
{
	float minimalWidthToFit = font->measureStringSize("W", fontSize).x;
	
	if (std::abs(width) < minimalWidthToFit)
		return oldText;
	
	std::string newText;
	std::string latestLine;
	
	const std::string dividers(" \n\t\r");
	
	while (oldText.size())
	{
		size_t wsPos = oldText.find_first_of(dividers);
		
		if (wsPos == std::string::npos)
		{
			std::string appended = latestLine + oldText;
			vec2 measuredSize = font->measureStringSize(appended, fontSize);
			if (measuredSize.x > width)
			{
				while (isWhitespaceChar(newText.back()))
					newText.erase(newText.end() - 1);
				newText.append("\n");
			}
			newText.append(oldText);
			break;
		}
		
		std::string word = oldText.substr(0, wsPos);
		
		char nextCharStr[] = { oldText[wsPos++], 0 };
		oldText.erase(0, wsPos);
		
		std::string appended = latestLine + word;
		if (!isNewLineChar(nextCharStr[0]))
			appended.append(nextCharStr);
		
		if (font->measureStringSize(appended, fontSize).x < width)
		{
			newText.append(word);
			latestLine.append(word);
			if (isNewLineChar(nextCharStr[0]))
			{
				latestLine = emptyString;
				newText.append("\n");
			}
			else
			{
				newText.append(nextCharStr);
				latestLine.append(nextCharStr);
			}
		}
		else if (font->measureStringSize(word, fontSize).x > width)
		{
			std::string wBegin = word.substr(0, word.size() - 1);
			appended = latestLine + wBegin;
			while (font->measureStringSize(appended, fontSize).x > width)
			{
				wBegin.erase(wBegin.size() - 1);
				appended = latestLine + wBegin;
			}
			newText.append(appended);
			newText.append("\n");
			newText.append(word.substr(wBegin.size()));
		}
		else
		{
			size_t lastCharPos = newText.size() - 1;
			char lastChar = (lastCharPos == std::string::npos) ? 0 : newText.at(lastCharPos);
			
			if (isWhitespaceChar(lastChar) && !isNewLineChar(lastChar))
			{
				newText[lastCharPos] = '\n';
				newText.append(word);
			}
			else
			{
				if (newText.empty())
					newText.append(word);
				else
					newText.append("\n" + word);
			}
			
			if (isNewLineChar(nextCharStr[0]))
			{
				latestLine = emptyString;
				newText.append("\n");
			}
			else
			{
				newText.append(nextCharStr);
				latestLine = word + nextCharStr;
			}
		}
	}
	
	return newText;
}

void Label::invalidateText()
{
	_charListText = font()->buildString(_text.cachedText, fontSize(), fontSmoothing());
	_charListNextText = font()->buildString(_nextText.cachedText, fontSize(), fontSmoothing());
	invalidateContent();
}
