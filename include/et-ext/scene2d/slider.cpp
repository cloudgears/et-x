/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Additional thanks to Kirill Polezhaiev
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/slider.h>

namespace et
{
namespace s2d
{

const float colorPlaceholdersSize = 0.5f;

Slider::Slider(Element2d* parent) :
	Element2d(parent), _value(timerPool())
{
	for (size_t i = 0; i < State_max; ++i)
		_handleScale[i] = 1.0f;

	_value.updated.connect<Slider>(this, &Slider::invalidateContent);
}

void Slider::setRange(float aMin, float aMax, float duration)
{
	float oldValue = value();

	_min = aMin;
	_max = aMax;

	if (oldValue < _min)
		setValue(_min, duration);

	if (oldValue > _max)
		setValue(_max, duration);

	invalidateContent();
}

void Slider::setValue(float v, float d)
{
	_value.animate(clamp(v, _min, _max), d);
	invalidateContent();
}

float Slider::value() const
{
	return _value.value();
}

float Slider::normalizedValue() const
{
	return (_value.value() - _min) / (_max - _min);
}

void Slider::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	if (_backgroundVertices.lastElementIndex() > 0)
	{
		materialInstance()->setTexture(MaterialTexture::BaseColor, _background.texture);
		r.addVertices(_backgroundVertices, materialInstance(), this);
	}

	if (_sliderLeftVertices.lastElementIndex() > 0)
	{
		materialInstance()->setTexture(MaterialTexture::BaseColor, _sliderLeft.texture);
		r.addVertices(_sliderLeftVertices, materialInstance(), this);
	}

	if (_sliderRightVertices.lastElementIndex() > 0)
	{
		materialInstance()->setTexture(MaterialTexture::BaseColor, _sliderRight.texture);
		r.addVertices(_sliderRightVertices, materialInstance(), this);
	}

	if (_handleVertices.lastElementIndex() > 0)
	{
		materialInstance()->setTexture(MaterialTexture::BaseColor, _handle[_state].texture);
		r.addVertices(_handleVertices, materialInstance(), this);
	}
}

void Slider::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	rectf mainRect(vec2(0.0f), size());

	_backgroundVertices.setOffset(0);
	_sliderLeftVertices.setOffset(0);
	_sliderRightVertices.setOffset(0);
	_handleVertices.setOffset(0);

	const auto& handleImage = _handle[_state];

	float hw = handleWidth();
	float halfHw = 0.5f * hw;
	float valuePoint = halfHw + normalizedValue() * (mainRect.width - hw);

	vec4 finalColorValue = finalColor();
	vec4 alphaScale = vec4(1.0f, finalColorValue.w);

	if (_backgroundColor.w > 0.0f)
		buildColorVertices(_backgroundVertices, mainRect, _backgroundColor * alphaScale, transform);

	if (_background.texture.valid())
	{
		if (_backgroundImageMode == BackgroundImageMode_Stretch)
		{
			buildImageVertices(_backgroundVertices, _background.texture,
				_background.descriptor, mainRect, finalColorValue, transform);
		}
		else if (_backgroundImageMode == BackgroundImageMode_Center)
		{
			buildImageVertices(_backgroundVertices, _background.texture, _background.descriptor,
				rectf(0.5f * (mainRect.size() - _background.descriptor.size), _background.descriptor.size),
				finalColorValue, transform);
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

		rectf r(0.0f, 0.5f * (mainRect.height - desc.size.y), valuePoint, desc.size.y);
		buildImageVertices(_sliderLeftVertices, _sliderLeft.texture, desc, r, finalColorValue, transform);
	}
	else if (_sliderLeftColor.w > 0.0f)
	{
		rectf r(0.0f, 0.5f * mainRect.height * (1.0f - colorPlaceholdersSize), valuePoint, mainRect.height * colorPlaceholdersSize);
		buildColorVertices(_sliderLeftVertices, r, _sliderLeftColor * alphaScale, transform);
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

		rectf r(valuePoint, 0.5f * (mainRect.height - desc.size.y), mainRect.width - valuePoint, desc.size.y);
		buildImageVertices(_sliderRightVertices, _sliderRight.texture, desc, r, finalColorValue, transform);
	}
	else if (_sliderRightColor.w > 0.0f)
	{
		rectf r(valuePoint, 0.5f * mainRect.height * (1.0f - colorPlaceholdersSize),
			mainRect.width - valuePoint, mainRect.height * colorPlaceholdersSize);
		buildColorVertices(_sliderLeftVertices, r, _sliderRightColor * alphaScale, transform);
	}

	if (handleImage.texture.valid())
	{
		rectf r(vec2(0.0f), _handleScale[_state] * handleImage.descriptor.size);
		r.top = 0.5f * (mainRect.height - r.height);
		r.left = clamp(valuePoint - halfHw, 0.0f, mainRect.width - hw);
		buildImageVertices(_handleVertices, handleImage.texture, handleImage.descriptor, r, finalColorValue, transform);
	}
	else if (_handleFillColor.w > 0.0f)
	{
		rectf r(valuePoint - halfHw, 0.5f * (mainRect.height - hw), hw, hw);
		buildColorVertices(_sliderLeftVertices, r, _handleFillColor * alphaScale, transform);
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
	_value.animate(mix(_min, _max, v), 0.0f);

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
	if (_handle[_state].texture.valid())
		return _handle[_state].descriptor.size.x;

	if (_handleFillColor.w > 0.0f)
		return 1.5f * colorPlaceholdersSize * size().y;

	return 0.0f;
}

void Slider::setHandleFillColor(const vec4& clr)
{
	_handleFillColor = clr;
	invalidateContent();
}

}
}
