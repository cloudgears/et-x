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
	Element2d(parent), _handleScale(1.0f), _min(0.0f), _max(1.0f), _value(0.5f), _drag(false),
	_sliderImagesMode(SliderImagesMode_Crop)
{
	
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
	_value = (v - _min) / (_max - _min);
	invalidateContent();
}

float Slider::value() const
{
	return mix(_min, _max, _value);
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
		r.addVertices(_handleVertices, _handle.texture, program(), this);
}

void Slider::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	rect mainRect(vec2(0.0f), size());
	
	_backgroundVertices.setOffset(0);
	_sliderLeftVertices.setOffset(0);
	_sliderRightVertices.setOffset(0);
	_handleVertices.setOffset(0);
	
	float handleWidth = _handle.descriptor.size.x;
	float halfHandleWidth = 0.5f * handleWidth;
	float valuePoint = _value * mainRect.width;
	
	if (_backgroundColor.w > 0.0f)
		buildColorVertices(_backgroundVertices, mainRect, _backgroundColor, transform);

	if (_background.texture.valid())
	{
		buildImageVertices(_backgroundVertices, _background.texture,
			_background.descriptor, mainRect, vec4(1.0f), transform);
	}
	
	if (_value > 0.0f)
	{
		if (_sliderLeft.texture.valid())
		{
			auto desc = _sliderLeft.descriptor;
			rect r(vec2(0.0f, 0.0f), desc.size);
			r.top = 0.5f * (mainRect.height - r.height);
			r.width = clamp(valuePoint, 0.0f, mainRect.width);
			
			if (_sliderImagesMode == SliderImagesMode_Crop)
				desc.size.x *= _value;
			
			buildImageVertices(_sliderLeftVertices, _sliderLeft.texture, desc, r, vec4(1.0f), transform);
		}
		else
		{
			rect r(halfHandleWidth, 0.0f, 0.0f, colorPlaceholdersSize * mainRect.height);
			r.top = 0.5f * (mainRect.height - r.height);
			r.width = clamp(valuePoint - halfHandleWidth, 0.0f, mainRect.width - handleWidth);
			buildColorVertices(_sliderLeftVertices, r, vec4(0.25f, 0.5f, 1.0f, 1.0f), transform);
		}
	}
	
	if (_value < 1.0f)
	{
		if (_sliderRight.texture.valid())
		{
			auto desc = _sliderRight.descriptor;
			
			rect r(vec2(0.0f), desc.size);
			r.top = 0.5f * (mainRect.height - r.height);
			r.left = clamp(valuePoint, 0.0f, mainRect.width);
			r.width = etMax(0.0f, mainRect.width - r.left);
			
			if (_sliderImagesMode == SliderImagesMode_Crop)
			{
				desc.origin.x += _value * desc.size.x;
				desc.size.x *= (1.0f - _value);
			}
			
			buildImageVertices(_sliderRightVertices, _sliderRight.texture, desc, r, vec4(1.0f), transform);
		}
		else
		{
			rect r(0.0f, 0.0f, 0.0f, colorPlaceholdersSize * mainRect.height);
			r.top = 0.5f * (mainRect.height - r.height);
			r.left = clamp(valuePoint, halfHandleWidth, mainRect.width - halfHandleWidth);
			r.width = etMax(0.0f, mainRect.width - halfHandleWidth - r.left);
			buildColorVertices(_sliderLeftVertices, r, vec4(0.5f, 0.5f, 0.5f, 1.0f), transform);
		}
	}

	if (_handle.texture.valid())
	{
		rect r(vec2(0.0f), _handleScale * _handle.descriptor.size);
		r.top = 0.5f * (mainRect.height - r.height);
		r.left = clamp(valuePoint - halfHandleWidth, 0.0f, mainRect.width - handleWidth);
		
		buildImageVertices(_handleVertices, _handle.texture, _handle.descriptor, r, vec4(1.0f), transform);
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

void Slider::setHandleImage(const Image& i, float scale)
{
	_handle = i;
	_handleScale = scale;
	invalidateContent();
}

void Slider::setSliderImages(const Image& left, const Image& right)
{
	_sliderLeft = left;
	_sliderRight = right;
	invalidateContent();
}

bool Slider::pointerPressed(const PointerInputInfo& p)
{
	_drag = true;
	updateValue(clamp(p.pos.x / size().x, 0.0f, 1.0f));
	return true;
}

bool Slider::pointerMoved(const PointerInputInfo& p)
{
	if (_drag)
	{
		updateValue(clamp(p.pos.x / size().x, 0.0f, 1.0f));
	}
	
	return true;
}

bool Slider::pointerReleased(const PointerInputInfo&)
{
	_drag = false;
	return true;
}

bool Slider::pointerCancelled(const PointerInputInfo&)
{
	_drag = false;
	return true;
}

void Slider::updateValue(float v)
{
	_value = v;
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
