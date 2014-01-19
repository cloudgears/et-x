/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/element2d.h>

using namespace et;
using namespace et::s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(Element2d)

Element2d::Element2d(Element* parent, const std::string& name) :
	Element(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _positionAnimator(timerPool()),
	_sizeAnimator(timerPool()), _colorAnimator(timerPool()), _scaleAnimator(timerPool()),
	_angleAnimator(timerPool()), _color(1.0f)
{
	_sizeAnimator.setTag(AnimatedProperty_Size);
	_colorAnimator.setTag(AnimatedProperty_Color);
	_scaleAnimator.setTag(AnimatedProperty_Scale);
	_angleAnimator.setTag(AnimatedProperty_Angle);
	_positionAnimator.setTag(AnimatedProperty_Position);
	
	_sizeAnimator.setDelegate(this);
	_colorAnimator.setDelegate(this);
	_scaleAnimator.setDelegate(this);
	_angleAnimator.setDelegate(this);
	_positionAnimator.setDelegate(this);
}

Element2d::Element2d(const rect& frame, Element* parent, const std::string& name) :
	Element(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _positionAnimator(timerPool()),
	_sizeAnimator(timerPool()), _colorAnimator(timerPool()), _scaleAnimator(timerPool()),
	_angleAnimator(timerPool()), _layout(frame), _desiredLayout(frame), _color(1.0f)
{
	_sizeAnimator.setTag(AnimatedProperty_Size);
	_colorAnimator.setTag(AnimatedProperty_Color);
	_scaleAnimator.setTag(AnimatedProperty_Scale);
	_angleAnimator.setTag(AnimatedProperty_Angle);
	_positionAnimator.setTag(AnimatedProperty_Position);
	
	_sizeAnimator.setDelegate(this);
	_colorAnimator.setDelegate(this);
	_scaleAnimator.setDelegate(this);
	_angleAnimator.setDelegate(this);
	_positionAnimator.setDelegate(this);
}

void Element2d::setAngle(float anAngle, float duration)
{
	_angleAnimator.cancelUpdates();
	_desiredLayout.angle = anAngle;
	
	if (duration <= std::numeric_limits<float>::epsilon())
	{
		_layout.angle = _desiredLayout.angle;
		invalidateTransform();
	}
	else 
	{
		_angleAnimator.animate(&_layout.angle, _layout.angle, _desiredLayout.angle, duration);
	}
}

void Element2d::setPivotPoint(const vec2& p, bool preservePosition)
{
	_layout.pivotPoint = p;
	_desiredLayout.pivotPoint = p;
	
	if (preservePosition)
		setPosition(_layout.position - offset());
}

void Element2d::setScale(const vec2& aScale, float duration)
{
	_scaleAnimator.cancelUpdates();
	_desiredLayout.scale = aScale;
	
	if (duration <= std::numeric_limits<float>::epsilon())
	{
		_layout.scale = _desiredLayout.scale;
		invalidateTransform();
	}
	else 
	{
		_scaleAnimator.animate( &_layout.scale, _layout.scale, _desiredLayout.scale, duration);
	}
}

void Element2d::setColor(const vec4& aColor, float duration)
{ 
	_colorAnimator.cancelUpdates();
	
	if (duration <= std::numeric_limits<float>::epsilon())
	{
		_color = aColor;
		invalidateContent(); 
	}
	else 
	{
		_colorAnimator.animate(&_color, _color, aColor, duration);
	}
}

void Element2d::setAlpha(float alpha, float duration) 
{ 
	_colorAnimator.cancelUpdates();
	
	if (duration <= std::numeric_limits<float>::epsilon())
	{
		_color.w = alpha;
		invalidateContent();
	}
	else 
	{
		_colorAnimator.animate(&_color, _color, vec4(_color.xyz(), alpha), duration);
	}
}

void Element2d::setPosition(const vec2& p, float duration) 
{
	willChangeFrame();
	
	_positionAnimator.cancelUpdates();
	_desiredLayout.position = p;
	
	if (duration <= std::numeric_limits<float>::epsilon())
	{
		_layout.position = _desiredLayout.position;
		invalidateTransform();
		invalidateContent();
	}
	else
	{
		_positionAnimator.animate(&_layout.position, _layout.position, _desiredLayout.position, duration);
	}
	
	didChangeFrame();
}

void Element2d::setSize(const vec2& s, float duration) 
{ 
	willChangeFrame();
	
	_sizeAnimator.cancelUpdates();
	_desiredLayout.size = s;
	
	if (duration <= std::numeric_limits<float>::epsilon())
	{
		_layout.size = _desiredLayout.size;
		invalidateTransform();
		invalidateContent();
	}
	else
	{
		_sizeAnimator.animate(&_layout.size, _layout.size, _desiredLayout.size, duration);
	}
	
	didChangeFrame();
}

void Element2d::setVisible(bool vis, float duration)
{
	setAlpha(vis ? 1.0f : 0.0f, duration);
}

const mat4& Element2d::transform()
{
	if (!transformValid())
		buildFinalTransform();
	
	return _transform;
}

const mat4& Element2d::finalTransform()
{
	if (!transformValid())
		buildFinalTransform();

	return _finalTransform;
}

void Element2d::buildFinalTransform()
{
	_transform = translationMatrix(vec3(offset(), 0.0f)) *
		transform2DMatrix(_layout.angle, _layout.scale, _layout.position);
	
	_finalTransform = _transform * parentFinalTransform();

	setTransformValid(true);
}

void Element2d::animatorUpdated(BaseAnimator* a)
{
	AnimatedPropery prop = static_cast<AnimatedPropery>(a->tag());
	
	bool isAngle = (prop & AnimatedProperty_Angle) == AnimatedProperty_Angle;
	bool isScale = (prop & AnimatedProperty_Scale) == AnimatedProperty_Scale;
	bool isColor = (prop & AnimatedProperty_Color) == AnimatedProperty_Color;
	
	bool isFrame = ((prop & AnimatedProperty_Position) == AnimatedProperty_Position) ||
		((prop & AnimatedProperty_Size) == AnimatedProperty_Size);

	if (isFrame || isAngle || isScale)
		invalidateTransform();

	if (isFrame || isColor)
		invalidateContent();
}

const mat4& Element2d::finalInverseTransform()
{
	if (!inverseTransformValid())
	{
		_finalInverseTransform = finalTransform().inverse();
		setIverseTransformValid(true);
	}

	return _finalInverseTransform;
}

SceneProgram Element2d::initProgram(SceneRenderer& r)
{
	if (_defaultProgram.invalid())
		_defaultProgram = r.defaultProgram();
	
	return _defaultProgram;
}

const vec4 Element2d::color() const
	{ return vec4(_color.xyz(), finalAlpha()); }

const vec2& Element2d::size() const
	{ return _layout.size; }

const vec2& Element2d::position() const
	{ return _layout.position; }

const vec2& Element2d::desiredSize() const
	{ return _desiredLayout.size; }

const vec2& Element2d::desiredPosition() const
	{ return _desiredLayout.position; }

const vec2& Element2d::desiredScale() const
	{ return _desiredLayout.scale; }

const vec2& Element2d::pivotPoint() const
	{ return _layout.pivotPoint; }

const vec2& Element2d::scale() const
	{ return _layout.scale; }

float Element2d::angle() const
	{ return _layout.angle; }

rect Element2d::frame() const
	{ return rect(origin(), size()); }

float Element2d::alpha() const
	{ return finalAlpha(); }

void Element2d::setPosition(float x, float y, float duration)
	{ return setPosition(vec2(x, y), duration); }

void Element2d::setSize(float w, float h, float duration)
	{ return setSize(vec2(w, h), duration); }

float Element2d::finalAlpha() const
	{ return parent() ? parent()->finalAlpha() * _color.w : _color.w; }

bool Element2d::visible() const
	{ return (finalAlpha() > 0.0f); }

void Element2d::rotate(float anAngle, float duration)
	{ return setAngle(_layout.angle + anAngle, duration); }

void Element2d::animatorFinished(BaseAnimator* a)
	{ elementAnimationFinished.invoke(this, static_cast<AnimatedPropery>(a->tag())); }

bool Element2d::containsPoint(const vec2& p, const vec2&)
	{ return containLocalPoint(finalInverseTransform() * p); }

bool Element2d::containLocalPoint(const vec2& p)
	{ return (p.x >= 0.0f) && (p.y >= 0.0f) && (p.x < _layout.size.x) && (p.y < _layout.size.y); }

vec2 Element2d::offset() const
	{ return -_layout.size * _layout.pivotPoint; }

vec2 Element2d::origin() const
	{ return _layout.position + offset(); }

vec2 Element2d::positionInElement(const vec2& p)
	{ return finalInverseTransform() * p; }

vec2 Element2d::contentSize()
	{ return _layout.size; }
