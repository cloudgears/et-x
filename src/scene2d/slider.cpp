/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Additional thanks to Kirill Polezhaiev
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/slider.h>

using namespace et;
using namespace et::s2d;

const float colorPlaceholdersSize = 0.5f;

Slider::Slider(Element2d* parent) :
	Element2d(parent), _min(0.0f), _max(1.0f), _value(0.5f), _state(State_Default),
	_sliderImagesMode(SliderImagesMode_Crop), _backgroundImageMode(BackgroundImageMode_Center)
{
	for (size_t i = 0; i < State_max; ++i)
		_handleScale[i] = 1.0f;
}

void Slider::setRange(float aMin, float aMax)
{
	float oldValue = value();
	
	_min = aMin;
	_max = aMax;
	
	if (oldValue < _min)
		setValue(_min);
	
	if (oldValue > _max)
		setValue(_max);
	
	invalidateContent();
}

void Slider::setValue(float v)
{
	_value = clamp(v, _min, _max);
	log::info("Set value: %f (%f -> %f)", _value, _min, _max);
	invalidateContent();
}

float Slider::value() const
{
	return _value;
}

float Slider::normalizedValue() const
{
	return (_value - _min) / (_max - _min);
}

void Slider::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	initProgram(r);
	
	if (!contentValid() || !transformValid())
		buildVertices(rc, r);

	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, _background.texture, program(), this);

	if (_sliderLeftVertices.lastElementIndex() > 0)
		r.addVertices(_sliderLeftVertices, _sliderLeft.texture, program(), this);

	if (_sliderRightVertices.lastElementIndex() > 0)
		r.addVertices(_sliderRightVertices, _sliderRight.texture, program(), this);
	
	if (_handleVertices.lastElementIndex() > 0)
		r.addVertices(_handleVertices, _handle[_state].texture, program(), this);
}

void Slider::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	rect mainRect(vec2(0.0f), size());
	
	_backgroundVertices.setOffset(0);
	_sliderLeftVertices.setOffset(0);
	_sliderRightVertices.setOffset(0);
	_handleVertices.setOffset(0);
	
	const auto& handleImage = _handle[_state];
	
	float hw = handleWidth();
	float halfHw = 0.5f * hw;
	float valuePoint = halfHw + normalizedValue() * (mainRect.width - hw);
	
	log::info("Value (SL): %f", normalizedValue());
		
	if (_backgroundColor.w > 0.0f)
		buildColorVertices(_backgroundVertices, mainRect, _backgroundColor, transform);

	if (_background.texture.valid())
	{
		if (_backgroundImageMode == BackgroundImageMode_Stretch)
		{
			buildImageVertices(_backgroundVertices, _background.texture,
				_background.descriptor, mainRect, color(), transform);
		}
		else if (_backgroundImageMode == BackgroundImageMode_Center)
		{
			buildImageVertices(_backgroundVertices, _background.texture, _background.descriptor,
				rect(0.5f * (mainRect.size() - _background.descriptor.size), _background.descriptor.size), color(), transform);
		}
		else
		{
			ET_FAIL("Invalid background image mode.");
		}
	}
	
	if (_sliderLeft.texture.valid())
	{
		auto desc = _sliderLeft.descriptor;
		if (_sliderImagesMode == SliderImagesMode_Crop)
			desc.size.x *= valuePoint / mainRect.width;
		
		rect r(0.0f, 0.5f * (mainRect.height - desc.size.y), valuePoint, desc.size.y);
		buildImageVertices(_sliderLeftVertices, _sliderLeft.texture, desc, r, color(), transform);
	}
	else
	{
		rect r(halfHw, 0.0f, 0.0f, colorPlaceholdersSize * mainRect.height);
		r.top = 0.5f * (mainRect.height - r.height);
		r.width = clamp(valuePoint - halfHw, 0.0f, mainRect.width - hw);
		buildColorVertices(_sliderLeftVertices, r, _sliderLeftColor, transform);
	}
	
	if (_sliderRight.texture.valid())
	{
		auto desc = _sliderRight.descriptor;
		if (_sliderImagesMode == SliderImagesMode_Crop)
		{
			float aScale = valuePoint / mainRect.width;
			desc.origin.x += aScale * desc.size.x;
			desc.size.x *= 1.0f - aScale;
		}
		
		rect r(valuePoint, 0.5f * (mainRect.height - desc.size.y), mainRect.width - valuePoint, desc.size.y);
		buildImageVertices(_sliderRightVertices, _sliderRight.texture, desc, r, color(), transform);
	}
	else
	{
		rect r(0.0f, 0.0f, 0.0f, colorPlaceholdersSize * mainRect.height);
		r.top = 0.5f * (mainRect.height - r.height);
		r.left = clamp(valuePoint, halfHw, mainRect.width - halfHw);
		r.width = etMax(0.0f, mainRect.width - halfHw - r.left);
		buildColorVertices(_sliderLeftVertices, r, _sliderRightColor, transform);
	}

	if (handleImage.texture.valid())
	{
		rect r(vec2(0.0f), _handleScale[_state] * handleImage.descriptor.size);
		r.top = 0.5f * (mainRect.height - r.height);
		r.left = clamp(valuePoint - halfHw, 0.0f, mainRect.width - hw);
		buildImageVertices(_handleVertices, handleImage.texture, handleImage.descriptor, r, color(), transform);
	}
	else
	{
		rect r(vec2(0.0f), vec2(1.5f * colorPlaceholdersSize * mainRect.height));
		r.top = 0.5f * (mainRect.height - r.height);
		r.left = clamp(valuePoint - 0.5f * r.width, 0.0f, mainRect.width - r.width);
		buildColorVertices(_sliderLeftVertices, r, vec4(1.0f, 0.5f, 0.25f, 1.0f), transform);
	}

	setContentValid();
}

void Slider::setBackgroundImage(const Image& i)
{
	_background = i;
	invalidateContent();
}

void Slider::setHandleImage(const Image& img, float scale)
{
	for (size_t i = 0; i < State_max; ++i)
		setHandleImageForState(img, scale, static_cast<State>(i));
}

void Slider::setHandleImageForState(const Image& img, float scale, State s)
{
	_handle[s] = img;
	_handleScale[s] = scale;
	invalidateContent();
}

void Slider::setSliderImages(const Image& left, const Image& right)
{
	_sliderLeft = left;
	_sliderRight = right;
	invalidateContent();
}

void Slider::setSliderFillColors(const vec4& l, const vec4& r)
{
	_sliderLeftColor = l;
	_sliderRightColor = r;
	invalidateContent();
}

bool Slider::pointerPressed(const PointerInputInfo& p)
{
	_state = State_Pressed;
	
	float width = size().x;
	float a = p.pos.x + (p.pos.x / width - 0.5f) * handleWidth();
	updateValue(clamp(a / size().x, 0.0f, 1.0f));
	
	return true;
}

bool Slider::pointerMoved(const PointerInputInfo& p)
{
	if (_state == State_Pressed)
	{
		float width = size().x;
		float a = p.pos.x + (p.pos.x / width - 0.5f) * handleWidth();
		updateValue(clamp(a / size().x, 0.0f, 1.0f));
	}
	
	return true;
}

bool Slider::pointerReleased(const PointerInputInfo&)
{
	_state = State_Default;
	draggingFinished.invokeInMainRunLoop();
	return true;
}

bool Slider::pointerCancelled(const PointerInputInfo&)
{
	_state = State_Default;
	return true;
}

void Slider::updateValue(float v)
{
	_value = mix(_min, _max, v);
	log::info("Update value: %f", _value);
	
	invalidateContent();
	
	changed.invoke(this);
	valueChanged.invoke(value());
}

void Slider::setBackgroundColor(const vec4& c)
{
	_backgroundColor = c;
	invalidateContent();
}

void Slider::setSliderImageMode(SliderImagesMode mode)
{
	_sliderImagesMode = mode;
	invalidateContent();
}

float Slider::handleWidth() const
{
	return _handle[_state].descriptor.size.x;
}
