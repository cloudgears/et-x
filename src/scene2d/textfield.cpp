/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/layout.h>
#include <et-ext/scene2d/textfield.h>

using namespace et;
using namespace et::s2d;

const int caretChar = 0x007C;
const int securedChar = 0x2022;

ET_DECLARE_SCENE_ELEMENT_CLASS(TextField)

TextField::TextField(const Font::Pointer& f, float fsz, Element2d* parent, const std::string& name) :
	TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS), _alignmentH(Alignment_Near),
	_alignmentV(Alignment_Center), _secured(false), _caretVisible(false)
{
	_caretChar.push_back(font()->generator()->charDescription(caretChar));
	
	setEditingFlags(EditingFlag_ResignFocusOnReturn);
	setFlag(Flag_RequiresKeyboard | Flag_ClipToBounds);
	ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired)
}

TextField::TextField(const std::string& text, const Font::Pointer& f, float fsz, Element2d* parent, const std::string& name) :
	TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS), _alignmentH(Alignment_Near),
	_alignmentV(Alignment_Center), _secured(false), _caretVisible(false)
{
	_caretChar.push_back(font()->generator()->charDescription(caretChar));
	
	setText(text);
	setEditingFlags(EditingFlag_ResignFocusOnReturn);
	setFlag(Flag_RequiresKeyboard | Flag_ClipToBounds);
	ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired)
}

TextField::TextField(const Image& background, const std::string& text, const Font::Pointer& f, float fsz,
	Element2d* parent, const std::string& name) : TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS),
	_background(background), _alignmentH(Alignment_Near), _alignmentV(Alignment_Center),
	_secured(false), _caretVisible(false)
{
	_caretChar.push_back(font()->generator()->charDescription(caretChar));
	
	setSize(font()->measureStringSize(text, fontSize(), fontSmoothing()));
	
	setText(text);
	setEditingFlags(EditingFlag_ResignFocusOnReturn);
	setFlag(Flag_RequiresKeyboard | Flag_ClipToBounds);
	ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired)
}

void TextField::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	initProgram(r);
	
	if (!contentValid() || !transformValid())
		buildVertices(rc, r);
	
	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, _background.texture, program(), this);

	if (_imageVertices.lastElementIndex() > 0)
		r.addVertices(_imageVertices, _background.texture, program(), this);
	
	if (_textVertices.lastElementIndex() > 0)
		r.addVertices(_textVertices, font()->generator()->texture(), textProgram(r), this);
}

void TextField::buildVertices(RenderContext*, SceneRenderer&)
{
	vec4 alphaScale = vec4(1.0f, finalAlpha());
	mat4 transform = finalTransform();
	rect wholeRect(vec2(0.0f), size());

	_backgroundVertices.setOffset(0);
	_imageVertices.setOffset(0);
	_textVertices.setOffset(0);
	
	if (_backgroundColor.w > 0.0f)
		buildColorVertices(_backgroundVertices, wholeRect, _backgroundColor * alphaScale, transform);
	
	if (_background.texture.valid())
	{
		buildImageVertices(_imageVertices, _background.texture, _background.descriptor,
			wholeRect, alphaScale, transform);
	}

	vec2 textSize = font()->measureStringSize(_textCharacters);
	vec2 caretSize = font()->measureStringSize(_caretChar);
	
	auto actualAlignment = _alignmentH;
	
	if (!_focused && !_placeholderCharacters.empty() && _textCharacters.empty())
	{
		vec2 placeholderSize = font()->measureStringSize(_placeholderCharacters);
		
		vec2 placeholderOrigin = _contentOffset + vec2(alignmentFactor(actualAlignment), alignmentFactor(_alignmentV)) *
			((wholeRect.size() - 2.0f * _contentOffset) - placeholderSize);
		
		buildStringVertices(_textVertices, _placeholderCharacters, Alignment_Near, Alignment_Near,
			placeholderOrigin, finalColor() * vec4(1.0f, 0.5f), transform, 1.0f);
	}
	
	float widthAdjustment = 0.0f;
	actualAlignment = _alignmentH;
	if (_focused && (textSize.x + caretSize.x >= wholeRect.width - _contentOffset.x))
	{
		actualAlignment = Alignment_Far;
		textSize.x += caretSize.x;
		textSize.y = etMax(caretSize.y, textSize.y);
		widthAdjustment = caretSize.x;
	}
	
	vec2 textOrigin = _contentOffset + vec2(alignmentFactor(actualAlignment), alignmentFactor(_alignmentV)) *
		((wholeRect.size() - 2.0f * _contentOffset) - textSize);
	
	if (!_textCharacters.empty())
	{
		buildStringVertices(_textVertices, _textCharacters, Alignment_Near, Alignment_Near, textOrigin,
			finalColor(), transform, 1.0f);
	}

	if (_caretVisible)
	{
		buildColorVertices(_imageVertices, rect(textOrigin.x + textSize.x - widthAdjustment,
			0.5f * (wholeRect.height - caretSize.y), caretSize.x, caretSize.y), finalColor(), transform);
	}
	
	setContentValid();
}

void TextField::setText(const std::string& s)
{
	_text = s;
	_actualText = _prefix + _text;
	
	_textCharacters = _secured ? CharDescriptorList(_actualText.length(),
		font()->generator()->charDescription(securedChar)) : font()->buildString(_actualText, fontSize(), fontSmoothing());
	
	invalidateContent();
}

void TextField::processMessage(const Message& msg)
{
	TextElement::processMessage(msg);
	
	if (msg.type == Message::Type_TextFieldControl)
	{
		switch (msg.param)
		{
			case ET_KEY_RETURN:
			{
				returnReceived.invoke(this);
				if (_editingFlags.hasFlag(EditingFlag_ResignFocusOnReturn))
					owner()->setActiveElement(Element2d::Pointer());
				break;
			}
				
			case ET_KEY_BACKSPACE:
			{
				if (_text.length() > 0)
				{
					size_t charToErase = 1;
					while (_text.size() > charToErase)
					{
						auto lastChar = _text[_text.length() - charToErase];
						if (lastChar & 0x80)
						{
							do
							{
								charToErase++;
								lastChar = _text[_text.length() - charToErase];
							}
							while ((_text.size() > charToErase) && (lastChar & 0x40) == 0);
							break;
						}
						else
						{
							break;
						}
					}
						   
					setText(_text.substr(0, _text.length() - charToErase));
				}
				
				textChanged.invoke(this);
				break;
			}
				
			case ET_KEY_ESCAPE:
			{
				if (_editingFlags.hasFlag(EditingFlag_ClearOnEscape))
				{
					setText(emptyString);
					textChanged.invoke(this);
				}
				break;
			}
				
			default:
				break;
		}
	}
	else if (msg.type == Message::Type_TextInput)
	{
		setText(_text + msg.text);
		textChanged.invoke(this);
		invalidateContent();
	}
	else if (msg.type == Message::Type_UpdateText)
	{
		setPlaceholder(_placeholder.key);
	}
}

void TextField::setSecured(bool s)
{
	_secured = s;
	invalidateContent();
}

void TextField::setFocus()
{
	_caretBlinkTimer.start(timerPool(), 0.5f, NotifyTimer::RepeatForever);
	_focused = true;
	_caretVisible = true;
	invalidateContent();
	
	editingStarted.invoke(this);
}

void TextField::resignFocus(Element2d*)
{
	_caretBlinkTimer.cancelUpdates();
	_focused = false;
	_caretVisible = false;
	invalidateContent();
	
	editingFinished.invoke(this);
}

void TextField::onCreateBlinkTimerExpired(NotifyTimer*)
{
	_caretVisible = !_caretVisible;
	invalidateContent();
}

const std::string& TextField::text() const
{
	return _text;
}

void TextField::setBackgroundColor(const vec4& color)
{
	_backgroundColor = color;
	invalidateContent();
}

void TextField::setVerticalAlignment(s2d::Alignment a)
{
	_alignmentV = a;
	invalidateContent();
}

void TextField::setHorizontalAlignment(s2d::Alignment a)
{
	_alignmentH = a;
	invalidateContent();
}

void TextField::setPrefix(const std::string& s)
{
	_prefix = s;
	setText(_text);
	invalidateContent();
}

void TextField::setEditingFlags(size_t f)
{
	_editingFlags.setFlags(f);
}

void TextField::setContentOffset(const vec2& o)
{
	_contentOffset = o;
	invalidateContent();
}

void TextField::setBackgroundImage(const Image& img)
{
	_background = img;
	invalidateContent();
}

void TextField::setPlaceholder(const std::string& s)
{
	_placeholder.setKey(s);
	_placeholderCharacters = font()->buildString(_placeholder.cachedText, fontSize(), fontSmoothing());
	
	invalidateContent();
}
