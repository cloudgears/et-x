/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/button.h>

using namespace et;
using namespace et::s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(Button)

Button::Button(const std::string& title, Font::Pointer font, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _title(title), _font(font),
	_textSize(font->measureStringSize(title)), _textColor(vec3(0.0f), 1.0f), _pressedColor(0.5f, 0.5f, 0.5f, 1.0f),
	_textPressedColor(vec3(0.0f), 1.0f), _type(Button::Type_PushButton), _state(State_Default),
	_imageLayout(ImageLayout_Left), _contentMode(ContentMode_Fit), _horizontalAlignment(Alignment_Center),
	_verticalAlignment(Alignment_Center), _pressed(false), _hovered(false), _selected(false)
{
	setSize(sizeForText(title));
}

void Button::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	if (!contentValid() || !transformValid())
		buildVertices(rc, r);

	if (_bgVertices.lastElementIndex() > 0)
		r.addVertices(_bgVertices, _background[_state].texture, r.defaultProgram(), this);

	if (_textVertices.lastElementIndex() > 0)
		r.addVertices(_textVertices, _font->texture(), r.defaultProgram(), this);

	if (_imageVertices.lastElementIndex() > 0)
		r.addVertices(_imageVertices, _image[_state].texture, r.defaultProgram(), this);
}

void Button::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	
	vec2 frameSize = size() + _contentOffset;
	vec2 imageSize = absv(_image[_state].descriptor.size);
	
	size_t sizeMode = _contentMode & 0x0000ffff;
	if (sizeMode == ContentMode_Fit)
	{
		float imageAspect = imageSize.aspect();
		if (imageSize.x > frameSize.x)
		{
			imageSize.x = frameSize.x;
			imageSize.y = frameSize.x / imageAspect;
		}
		if (imageSize.y > frameSize.y)
		{
			imageSize.x = frameSize.y * imageAspect;
			imageSize.y = frameSize.y;
		}
	}
	else if (sizeMode == ContentMode_ScaleMaxToMin)
	{
		float maxImageDim = etMax(imageSize.x, imageSize.y);
		float minFrameDim = etMin(frameSize.x, frameSize.y);
		imageSize *= minFrameDim / maxImageDim;
	}
	else
	{
		assert("Uknown content mode" && 0);
	}
	
	float contentGap = (imageSize.x > 0.0f) && (_textSize.x > 0.0f) ? 5.0f : 0.0f;
	vec2 contentSize = imageSize + _textSize + vec2(contentGap);

	vec2 imageOrigin;
	vec2 textOrigin;
	
	if (_imageLayout == ImageLayout_Right)
	{
		textOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
			(frameSize - vec2(contentSize.x, _textSize.y));
		
		imageOrigin.x = textOrigin.x + contentGap + _textSize.x;
		imageOrigin.y = alignmentFactor(_verticalAlignment) * (frameSize.y - imageSize.y);
	}
	else if (_imageLayout == ImageLayout_Top)
	{
		imageOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
		(frameSize - vec2(imageSize.x, contentSize.y));
		textOrigin.x = alignmentFactor(_horizontalAlignment) * (frameSize.x - _textSize.x);
		textOrigin.y = imageOrigin.y + contentGap + imageSize.y;
	}
	else if (_imageLayout == ImageLayout_Bottom)
	{
		textOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
			(frameSize - vec2(_textSize.x, contentSize.y));
		imageOrigin.x = alignmentFactor(_horizontalAlignment) * (frameSize.x - imageSize.x);
		imageOrigin.y = textOrigin.y + contentGap + _textSize.y;
	}
	else
	{
		imageOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
			(frameSize - vec2(contentSize.x, imageSize.y));
		
		textOrigin.x = imageOrigin.x + contentGap + imageSize.x;
		textOrigin.y = alignmentFactor(_verticalAlignment) * (frameSize.y - _textSize.y);
	}
	
	vec4 alphaScale = vec4(1.0f, 1.0f, 1.0f, alpha());
	
	_bgVertices.setOffset(0);
	_textVertices.setOffset(0);
	_imageVertices.setOffset(0);
	
	if (_backgroundColor.w > 0.0f)
		buildColorVertices(_bgVertices, rect(vec2(0.0f), size()), _backgroundColor * alphaScale, transform);

	if (_background[_state].texture.valid())
	{
		vec4 backgroundScale = ((_state == State_Pressed) && _adjustPressedBackground) ?
			_pressedColor * alphaScale : alphaScale;
		
		buildImageVertices(_bgVertices, _background[_state].texture, _background[_state].descriptor,
			rect(vec2(0.0f), size()), backgroundScale * color(), transform);
	}

	if (_title.size() > 0)
	{
		vec4 aColor = _state == State_Pressed ? _textPressedColor : _textColor;
		if (aColor.w > 0.0f)
		{
			buildStringVertices(_textVertices, _font->buildString(_title), Alignment_Near,
				Alignment_Near, textOrigin, aColor * alphaScale, transform);
		}
	}

	if (_image[_state].texture.valid())
	{
		vec4 aColor = ((_state == State_Pressed) ? pressedColor() : color()) * alphaScale;
		if (aColor.w > 0.0f)
		{
			buildImageVertices(_imageVertices, _image[_state].texture, _image[_state].descriptor,
				rect(imageOrigin, imageSize), aColor, transform);
		}
	}
}

void Button::setBackgroundForState(const Texture& tex, const ImageDescriptor& desc, State s)
{
	setBackgroundForState(Image(tex, desc), s);
}

void Button::setBackgroundForState(const Image& img, State s)
{
	_background[s] = img;
	invalidateContent();
}

bool Button::pointerPressed(const PointerInputInfo& p)
{
	if (p.type != PointerType_General) return false;

	_pressed = true;
	setCurrentState(_selected ? State_SelectedPressed : State_Pressed);
	
	pressed.invoke(this);
	return true;
}

bool Button::pointerReleased(const PointerInputInfo& p)
{
	if ((p.type != PointerType_General) || !_pressed) return false;
	
	_pressed = false;

	if (containLocalPoint(p.pos))
	{
		performClick();
		setCurrentState(adjustState(_selected ? State_SelectedHovered : State_Hovered));
		releasedInside.invoke(this);
	}
	else
	{
		setCurrentState(adjustState(_selected ? State_Selected : State_Default));
		releasedOutside.invoke(this);
	}

	return true;
}

bool Button::pointerCancelled(const PointerInputInfo& p)
{
	if ((p.type != PointerType_General) || !_pressed) return false;
	
	_pressed = false;
	State newState = _selected ? State_Selected : State_Default;
	
	setCurrentState(newState);
	cancelled.invoke(this);
	return true;
}

void Button::pointerEntered(const PointerInputInfo&)
{
	if (_selected)
		setCurrentState(adjustState(_pressed ? State_SelectedPressed : State_SelectedHovered));
	else
		setCurrentState(adjustState(_pressed ? State_Pressed : State_Hovered));
}

void Button::pointerLeaved(const PointerInputInfo&)
{
	if (_selected)
		setCurrentState(_pressed ? State_SelectedPressed : State_Selected);
	else
		setCurrentState(_pressed ? State_Pressed : State_Default);
}

void Button::setCurrentState(State s)
{
	if (_state == s) return;

	_state = s;
	invalidateContent();
}

bool Button::capturePointer() const
{
	return true;
}

void Button::performClick()
{
	if (_type == Type_CheckButton)
		setSelected(!_selected);
	
	clicked.invoke(this);
}

const vec2& Button::textSize()
{
	return _textSize; 
}

void Button::setTitle(const std::string& t)
{
	if (_title == t) return;

	_title = t;
	_textSize = _font->measureStringSize(_title);
	invalidateContent();
}

void Button::setTextColor(const vec4& color)
{
	_textColor = color;
	invalidateContent();
}

const vec4& Button::textColor() const
{
	return _textColor;
}

void Button::setTextPressedColor(const vec4& color)
{
	_textPressedColor = color;
	invalidateContent();
}

const vec4& Button::textPressedColor() const
{
	return _textPressedColor;
}

void Button::adjustSize(float duration)
{
	adjustSizeForText(_title, duration);
}

void Button::adjustSizeForText(const std::string& text, float duration)
{
	setSize(sizeForText(text), duration);
}

vec2 Button::sizeForText(const std::string& text)
{
	vec2 textSize = _font.valid() ? _font->measureStringSize("AA" + text + "AA") : vec2(0.0f);
	
	for (size_t i = 0; i < State_max; ++i)
		textSize = maxv(textSize, _background[i].descriptor.size);

	return vec2(floorf(textSize.x), floorf(1.25f * textSize.y));
}

void Button::setImage(const Image& img)
{
	for (size_t i = 0; i < State_max; ++i)
		_image[i] = img;
	
	invalidateContent();
}

void Button::setImageForState(const Image& img, State s)
{
	_image[s] = img;
	invalidateContent();
}

void Button::setSelected(bool s)
{
	bool wasSelected = _selected;
	_selected = s && (_type == Type_CheckButton);

	if (wasSelected != _selected)
	{
		if (elementIsSelected(_state))
			setCurrentState(static_cast<State>(_state - 3 * static_cast<int>(!_selected)));
		else
			setCurrentState(static_cast<State>(_state + 3 * static_cast<int>(_selected)));
	}
}

void Button::setType(Button::Type t)
{
	_type = t;
	setSelected(false);
}

void Button::setContentOffset(const vec2& o)
{
	_contentOffset = o;
	invalidateContent();
}

void Button::setImageLayout(ImageLayout l)
{
	_imageLayout = l;
	invalidateContent();
}

void Button::setBackgroundColor(const vec4& color)
{
	_backgroundColor = color;
	invalidateContent();
}

const vec4& Button::backgroundColor() const
{
	return _backgroundColor;
}

void Button::setContentMode(ContentMode m)
{
	_contentMode = m;
	invalidateContent();
}

vec2 Button::contentSize()
{
	return sizeForText(_title);
}

void Button::adjustsPressedBackground(bool b)
{
	_adjustPressedBackground = b;
	invalidateContent();
}

void Button::setHorizontalAlignment(Alignment a)
{
	_horizontalAlignment = a;
	invalidateContent();
}

void Button::setVerticalAlignment(Alignment a)
{
	_verticalAlignment = a;
	invalidateContent();
}

void Button::setPressedColor(const vec4& color)
{
	_pressedColor = color;
	invalidateContent();
}

const vec4& Button::pressedColor() const
{
	return _pressedColor;
}
