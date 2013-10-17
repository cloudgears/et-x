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

const short caretChar = '|';
const short securedChar = '*';

ET_DECLARE_SCENE_ELEMENT_CLASS(TextField)

TextField::TextField(Font::Pointer font, Element* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _font(font), _alignmentH(Alignment_Near),
	_alignmentV(Alignment_Center), _secured(false), _caretVisible(false)
{
	setEditingFlags(EditingFlag_ResignFocusOnReturn);
	setFlag(Flag_RequiresKeyboard);
	ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired)
}

TextField::TextField(const std::string& text, Font::Pointer font, Element* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _font(font), _alignmentH(Alignment_Near),
	_alignmentV(Alignment_Center), _secured(false), _caretVisible(false)
{
	setText(text);
	setFlag(Flag_RequiresKeyboard);
	ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired)
}

TextField::TextField(const Image& background, const std::string& text, Font::Pointer font,
	Element* parent, const std::string& name) : Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS),
	_font(font), _background(background), _alignmentH(Alignment_Near), _alignmentV(Alignment_Center),
	_secured(false), _caretVisible(false)
{
	setText(text);
	
	setFlag(Flag_RequiresKeyboard);
	setSize(font->measureStringSize(text));
	
	ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired)
}

void TextField::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	if (!contentValid() || !transformValid())
		buildVertices(rc, r);
	
	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, _background.texture, r.defaultProgram(), this);

	if (_imageVertices.lastElementIndex() > 0)
		r.addVertices(_imageVertices, _background.texture, r.defaultProgram(), this);
	
	if (_textVertices.lastElementIndex() > 0)
		r.addVertices(_textVertices, _font->texture(), r.defaultProgram(), this);
}

void TextField::buildVertices(RenderContext*, SceneRenderer&)
{
	vec4 alphaVector = vec4(1.0f, 1.0f, 1.0f, alpha());
	mat4 transform = finalTransform();
	rect wholeRect(vec2(0.0), size());

	_backgroundVertices.setOffset(0);
	_imageVertices.setOffset(0);
	_textVertices.setOffset(0);
	
	if (_backgroundColor.w > 0.0f)
		buildColorVertices(_backgroundVertices, wholeRect, _backgroundColor, transform);
	
	if (_background.texture.valid())
	{
		buildImageVertices(_imageVertices, _background.texture, _background.descriptor,
			wholeRect, alphaVector, transform);
	}

	_charList = _secured ?
		CharDescriptorList(_actualText.length(), _font->charDescription(securedChar)) :
		_font->buildString(_actualText);
	
	if (_charList.size())
	{
		buildStringVertices(_textVertices, _charList, _alignmentH, _alignmentV,
			size() * vec2(alignmentFactor(_alignmentH), alignmentFactor(_alignmentV)),
			color() * alphaVector, transform);
	}

	if (_caretVisible)
	{
		vec2 textSize = _font->measureStringSize(_charList);
		vec2 textOrigin = vec2(alignmentFactor(_alignmentH), alignmentFactor(_alignmentV)) * (size() - textSize);
		buildStringVertices(_textVertices, { _font->charDescription(caretChar) }, Alignment_Near,
			Alignment_Near, textOrigin + vec2(textSize.x, 0.0f), color() * alphaVector, transform);
	}
	
	setContentValid();
}

void TextField::setText(const std::string& s)
{
	_text = s;
	_actualText = _prefix + _text;
	invalidateContent();
}

void TextField::processMessage(const Message& msg)
{
	if (msg.type == Message::Type_TextFieldControl)
	{
		switch (msg.param)
		{
			case ET_KEY_RETURN:
			{
				returnReceived.invoke(this);
				if (_editingFlags.hasFlag(EditingFlag_ResignFocusOnReturn))
					owner()->setActiveElement(nullptr);
				break;
			}
				
			case ET_KEY_BACKSPACE:
			{
				if (_text.length() > 0)
				{
					size_t charToDelete = 1;
					while ((_text.size() > charToDelete) && ((_text[_text.length() - charToDelete - 1] & 0x80) != 0))
						   ++charToDelete;
						   
					setText(_text.substr(0, _text.length() - charToDelete));
				}
				
				textChanged.invoke(this);
				break;
			}
				
			case ET_KEY_ESCAPE:
			{
				if (_editingFlags.hasFlag(EditingFlag_ClearOnEscape))
				{
					setText(std::string());
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
}

void TextField::setSecured(bool s)
{
	_secured = s;
	invalidateContent();
}

void TextField::setFocus()
{
	_caretBlinkTimer.start(timerPool(), 0.5f, NotifyTimer::RepeatForever);
	_caretVisible = true;
	invalidateContent();
	
	editingStarted.invoke(this);
}

void TextField::resignFocus(Element*)
{
	_caretBlinkTimer.cancelUpdates();
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
