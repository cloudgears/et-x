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

Button::Button(const std::string& title, const Font::Pointer& font, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _font(font), _textColor(vec3(0.0f), 1.0f),
	_pressedColor(0.5f, 0.5f, 0.5f, 1.0f), _backgroundTintColor(1.0f), _commonBackgroundTintColor(1.0f),
	_textPressedColor(vec3(0.0f), 1.0f), _backgroundTintAnimator(timerPool()),
	_commonBackgroundTintAnimator(timerPool()), _titleAnimator(timerPool()), _type(Button::Type_PushButton),
	_state(State_Default), _imageLayout(ImageLayout_Left), _contentMode(ContentMode_Fit),
	_horizontalAlignment(Alignment_Center), _verticalAlignment(Alignment_Center)
{
	_currentTitle.setKey(title);
	_currentTextSize = _font->measureStringSize(_currentTitle.cachedText, true);
	
	_nextTitle = _currentTitle;
	_maxTextSize = _currentTextSize;
	_currentTitleCharacters = _font->buildString(_currentTitle.cachedText, true);
	
	setSize(sizeForText(title));
	
	_backgroundTintAnimator.updated.connect<s2d::Button>(this, &Button::invalidateContent);
	_commonBackgroundTintAnimator.updated.connect<s2d::Button>(this, &Button::invalidateContent);
	
	_titleAnimator.updated.connect<Button>(this, &Button::invalidateContent);
	
	_titleAnimator.finished.connect([this]()
	{
		_currentTitle = _nextTitle;
		_currentTextSize = _nextTextSize;
		_currentTitleCharacters = _nextTitleCharacters;
		
		_maxTextSize = _currentTextSize;
		_titleTransition = 0.0f;
		
		_nextTextSize = vec2(0.0f);
		_nextTitleCharacters.clear();
	});
}

void Button::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	initProgram(r);
	
	if (!contentValid() || !transformValid())
		buildVertices(rc, r);

	if (_bgVertices.lastElementIndex() > 0)
		r.addVertices(_bgVertices, _background[_state].texture, program(), this);

	if (_textVertices.lastElementIndex() > 0)
		r.addVertices(_textVertices, _font->texture(), r.defaultTextProgram(), this);

	if (_imageVertices.lastElementIndex() > 0)
		r.addVertices(_imageVertices, _image[_state].texture, program(), this);
}

void Button::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	
	vec2 frameSize = size() + maxv(_contentOffset, vec2(0.0f));
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
		ET_FAIL("Uknown content mode");
	}
	
	float contentGap = (imageSize.x > 0.0f) && ((_currentTextSize.x > 0.0f) || (_nextTextSize.x > 0.0f)) ? 5.0f : 0.0f;
	vec2 contentSize = imageSize + _maxTextSize + vec2(contentGap);

	vec2 imageOrigin;
	vec2 textOrigin;
	
	if (_imageLayout == ImageLayout_Right)
	{
		textOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
			(frameSize - vec2(contentSize.x, _maxTextSize.y));
				
		imageOrigin.x = textOrigin.x + contentGap + _maxTextSize.x;
		imageOrigin.y = alignmentFactor(_verticalAlignment) * (frameSize.y - imageSize.y);
	}
	else if (_imageLayout == ImageLayout_Top)
	{
		imageOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
			(frameSize - vec2(imageSize.x, contentSize.y));
		textOrigin.x = alignmentFactor(_horizontalAlignment) * (frameSize.x - _maxTextSize.x);
		textOrigin.y = imageOrigin.y + contentGap + imageSize.y;
	}
	else if (_imageLayout == ImageLayout_Bottom)
	{
		textOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
			(frameSize - vec2(_maxTextSize.x, contentSize.y));
		imageOrigin.x = alignmentFactor(_horizontalAlignment) * (frameSize.x - imageSize.x);
		imageOrigin.y = textOrigin.y + contentGap + _maxTextSize.y;
	}
	else
	{
		imageOrigin = vec2(alignmentFactor(_horizontalAlignment), alignmentFactor(_verticalAlignment)) *
			(frameSize - vec2(contentSize.x, imageSize.y));
		
		textOrigin.x = imageOrigin.x + contentGap + imageSize.x;
		textOrigin.y = alignmentFactor(_verticalAlignment) * (frameSize.y - _maxTextSize.y);
	}
	
	vec4 alphaScale = vec4(1.0f, finalAlpha());
	
	imageOrigin += minv(vec2(0.0f), _contentOffset);
	textOrigin += minv(vec2(0.0f), _contentOffset);
	
	_bgVertices.setOffset(0);
	_textVertices.setOffset(0);
	_imageVertices.setOffset(0);
	
	vec4 finalColorValue = finalColor();
	
	if (_backgroundColor.w > 0.0f)
		buildColorVertices(_bgVertices, rect(vec2(0.0f), size()), _backgroundColor * alphaScale, transform);
	
	if (_commonBackground.texture.valid())
	{
		buildImageVertices(_bgVertices, _commonBackground.texture, _commonBackground.descriptor,
			rect(vec2(0.0f), size()), _commonBackgroundTintColor * finalColorValue, transform);
	}

	if (_background[_state].texture.valid())
	{
		vec4 backgroundScale = (_adjustPressedBackground && ((_state == State_Pressed) || (_state == State_SelectedPressed))) ?
			_pressedColor * alphaScale : alphaScale;
		
		buildImageVertices(_bgVertices, _background[_state].texture, _background[_state].descriptor,
			rect(vec2(0.0f), size()), _backgroundTintColor * backgroundScale * finalColorValue, transform);
	}

	vec4 currentTextColor = (_state == State_Pressed) ? _textPressedColor : _textColor;
	if (currentTextColor.w > 0.0f)
	{
		vec4 currentAlphaScale = alphaScale;
		
		currentAlphaScale.w = alphaScale.w * (1.0f - _titleTransition);
		if (!_currentTitleCharacters.empty())
		{
			buildStringVertices(_textVertices, _currentTitleCharacters, Alignment_Center,
				Alignment_Center, textOrigin + 0.5f * _maxTextSize, currentTextColor * currentAlphaScale, transform);
		}
		
		currentAlphaScale.w = alphaScale.w * _titleTransition;
		if (!_nextTitleCharacters.empty())
		{
			buildStringVertices(_textVertices, _nextTitleCharacters, Alignment_Center,
				Alignment_Center, textOrigin + 0.5f * _maxTextSize, currentTextColor * currentAlphaScale, transform);
		}
	}

	if (_image[_state].texture.valid())
	{
		vec4 aColor = (_state == State_Pressed) ? pressedColor() * alphaScale : finalColorValue;
		if (aColor.w > 0.0f)
		{
			buildImageVertices(_imageVertices, _image[_state].texture, _image[_state].descriptor,
				rect(imageOrigin, imageSize), aColor, transform);
		}
	}
}

void Button::setBackground(const Image& img)
{
	for (size_t i = 0; i < State_max; ++i)
		_background[i] = img;
	
	invalidateContent();
}

void Button::setBackgroundForState(const Image& img, State s)
{
	_background[s] = img;
	invalidateContent();
}

void Button::setCommonBackground(const Image& img)
{
	_commonBackground = img;
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
	released.invoke(this);

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

bool Button::pointerCancelled(const PointerInputInfo&)
{
	_pressed = false;
	
	setCurrentState(_selected ? State_Selected : State_Default);
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
	float currentTime = timerPool()->actualTime();
	if (currentTime > _lastClickTime + _clickTreshold)
	{
		_lastClickTime = currentTime;
		
		if (_type == Type_CheckButton)
			setSelected(!_selected);
	
		if (_shouldInvokeClickInRunLoop)
			clicked.invokeInMainRunLoop(this);
		else
			clicked.invoke(this);
	}
}

void Button::setTitle(const std::string& t, float duration)
{
	_nextTitle.setKey(t);
	_nextTitleCharacters = _font->buildString(_nextTitle.cachedText, true);
	_nextTextSize = _font->measureStringSize(_nextTitle.cachedText, true);
	
	_maxTextSize = maxv(_currentTextSize, _nextTextSize);
	
	_titleAnimator.cancelUpdates();
	
	if (duration == 0.0f)
		_titleAnimator.finished.invoke();
	else
		_titleAnimator.animate(&_titleTransition, 0.0f, 1.0f, duration);
		
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

void Button::adjustSize(float duration, bool vertical, bool horizontal)
{
	adjustSizeForText(_currentTitle.cachedText, duration, vertical, horizontal);
}

void Button::adjustSizeForText(const std::string& text, float duration, bool vertical, bool horizontal)
{
	vec2 currentSize = size();
	vec2 newSize = sizeForText(text);
	
	newSize.x += _image[0].descriptor.size.x;
	newSize.y = etMax(newSize.y, _image[0].descriptor.size.y);
	
	if (horizontal)
		currentSize.x = newSize.x;
	
	if (vertical)
		currentSize.y = newSize.y;
	
	setSize(currentSize, duration);
}

vec2 Button::sizeForText(const std::string& text)
{
	vec2 textSize = _font.valid() ? _font->measureStringSize("AA" + text + "AA", true) : vec2(0.0f);
	
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

void Button::setBackgroundColor(const vec4& clr)
{
	_backgroundColor = clr;
	invalidateContent();
}

const vec4& Button::backgroundColor() const
{
	return _backgroundColor;
}

void Button::setCommonBackgroundTintColor(const vec4& clr, float duration)
{
	_commonBackgroundTintAnimator.cancelUpdates();
	
	if (duration == 0.0f)
	{
		_commonBackgroundTintColor = clr;
		invalidateContent();
	}
	else
	{
		_commonBackgroundTintAnimator.animate(&_commonBackgroundTintColor, _commonBackgroundTintColor, clr, duration);
	}
}

const vec4& Button::commonBackgroundTintColor() const
{
	return _commonBackgroundTintColor;
}

void Button::setBackgroundTintColor(const vec4& clr, float duration)
{
	_backgroundTintAnimator.cancelUpdates();
	
	if (duration == 0.0f)
	{
		_backgroundTintColor = clr;
		invalidateContent();
	}
	else
	{
		_backgroundTintAnimator.animate(&_backgroundTintColor, _backgroundTintColor, clr, duration);
	}
}

const vec4& Button::backgroundTintColor() const
{
	return _backgroundTintColor;
}

void Button::setContentMode(ContentMode m)
{
	_contentMode = m;
	invalidateContent();
}

vec2 Button::contentSize()
{
	return maxv(sizeForText(_currentTitle.cachedText), sizeForText(_nextTitle.cachedText));
}

void Button::setShouldAdjustPressedBackground(bool b)
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

void Button::processMessage(const Message& msg)
{
	if (msg.type == Message::Type_SetText)
		setTitle(msg.text, msg.duration);
	else if (msg.type == Message::Type_UpdateText)
		setTitle(_nextTitle.key);
}

void Button::setClickTreshold(float value)
{
	_clickTreshold = value;
}

void Button::setShouldInvokeClickInRunLoop(bool b)
{
	_shouldInvokeClickInRunLoop = b;
}
