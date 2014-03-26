/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/scroll.h>

using namespace et;
using namespace et::s2d;

float deccelerationRate = 10.0f;
float accelerationRate = 0.5f;
float scrollbarSize = 5.0f;
float maxScrollbarsVisibilityVelocity = 50.0f;
float minAlpha = 1.0f / 255.0f;
float alphaAnimationScale = 5.0f;
float bounceStopTreshold = 0.5f;

ET_DECLARE_SCENE_ELEMENT_CLASS(Scroll)

Scroll::Scroll(Element2d* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _offsetAnimator(timerPool()),
	_updateTime(0.0f), _scrollbarsAlpha(0.0f), _scrollbarsAlphaTarget(0.0f), _bounceExtent(0.5f),
	_pointerScrollMultiplier(1.0f), _pointerScrollDuration(0.0f), _movementTreshold(9.0f), _bounce(0),
	_pointerCaptured(false), _manualScrolling(false)
{
	_offsetAnimator.updated.connect([this](){ setOffsetDirectly(_offset); });

	setFlag(Flag_HandlesChildEvents);
	setFlag(Flag_ClipToBounds);
	startUpdates();
}

void Scroll::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	if (!contentValid())
		buildVertices(rc, r);
	
	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, r.lastUsedTexture(), r.defaultProgram(), this);
}

void Scroll::addToOverlayRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	if (!contentValid())
		buildVertices(rc, r);

	if (_overlayVertices.lastElementIndex() > 0)
		r.addVertices(_overlayVertices, r.lastUsedTexture(), r.defaultProgram(), this);
}

void Scroll::buildVertices(RenderContext* rc, SceneRenderer&)
{
	_backgroundVertices.setOffset(0);
	_overlayVertices.setOffset(0);

	if (_backgroundColor.w > 0.0f)
	{
		buildColorVertices(_backgroundVertices, rect(vec2(0.0f), size()), _backgroundColor,
			Element2d::finalTransform());
	}
	
	if (_scrollbarsColor.w > 0.0f)
	{
		float scaledScollbarSize = scrollbarSize * static_cast<float>(rc->screenScaleFactor());
		float barHeight = size().y * (size().y / _contentSize.y);
		float barOffset = size().y * (_offset.y / _contentSize.y);
		vec2 origin(size().x - 2.0f * scaledScollbarSize, -barOffset);
		
		vec4 adjutsedColor = _scrollbarsColor;
		adjutsedColor.w *= _scrollbarsAlpha;
		
		buildColorVertices(_overlayVertices, rect(origin, vec2(scaledScollbarSize, barHeight)), adjutsedColor,
		  Element2d::finalTransform());
	}
	
	if (_overlayColor.w > 0.0f)
		buildColorVertices(_overlayVertices, rect(vec2(0.0f), size()), _overlayColor, Element2d::finalTransform());
	
	setContentValid();
}

const mat4& Scroll::finalTransform()
{
	_localFinalTransform = Element2d::finalTransform();
	_localFinalTransform[3] += vec4(_offset, 0.0f, 0.0f);
	return _localFinalTransform;
}

const mat4& Scroll::finalInverseTransform()
{
	_localInverseTransform = Element2d::finalTransform().inverse();
	return _localInverseTransform;
}

bool Scroll::pointerPressed(const PointerInputInfo& p)
{
	if ((_currentPointer.id == 0) && (p.type == PointerType_General))
	{
		_previousPointer = p;
		_currentPointer = p;
		_manualScrolling = false;
		_pointerCaptured = false;
		_velocity = vec2(0.0f);
	}

	broadcastPressed(p);
	return true;
}

bool Scroll::pointerMoved(const PointerInputInfo& p)
{
	if (p.id != _currentPointer.id)
	{
		broadcastMoved(p);
		return true;
	}
	
	vec2 offset = p.pos - _currentPointer.pos;
	if (!_manualScrolling && (offset.dotSelf() < sqr(_movementTreshold)))
	{
		if (_capturedElement.valid() && _capturedElement->capturesPointer())
			broadcastMoved(p);
		
		return true;
	}

	if (_manualScrolling)
	{
		vec2 offsetScale(1.0f);

		if (-_offset.x < scrollLeftDefaultValue())
		{
			float diff = std::abs(-_offset.x - scrollLeftDefaultValue());
			offsetScale.x *= etMax(0.0f, 1.0f - diff / scrollOutOfContentXSize());
		}
		else if (-_offset.x > scrollRightDefaultValue())
		{
			float diff = std::abs(-_offset.x - scrollRightDefaultValue());
			offsetScale.x *= etMax(0.0f, 1.0f - diff / scrollOutOfContentXSize());
		}
		if (-_offset.y < scrollUpperDefaultValue())
		{
			float diff = std::abs(-_offset.y - scrollUpperDefaultValue());
			offsetScale.y *= etMax(0.0f, 1.0f - diff / scrollOutOfContentYSize());
		}
		else if (-_offset.y > scrollLowerDefaultValue())
		{
			float diff = std::abs(-_offset.y - scrollLowerDefaultValue());
			offsetScale.y *= etMax(0.0f, 1.0f - diff / scrollOutOfContentYSize());
		}
		
		applyOffset(sqr(offsetScale) * offset);
	}
	else if (_capturedElement.valid() && _capturedElement->capturesPointer())
	{
		broadcastMoved(p);
	}
	else if (!_pointerCaptured && (p.type == PointerType_General))
	{
		_manualScrolling = true;
		_pointerCaptured = true;
		_scrollbarsAlphaTarget = 1.0f;
		_bouncing = vector2<BounceDirection>(BounceDirection_None);
		broadcastCancelled(p);
	}
	
	_previousPointer = _currentPointer;
	_currentPointer = p;

	return true;
}

bool Scroll::pointerReleased(const PointerInputInfo& p)
{
	if (p.id == _currentPointer.id)
	{
		if (!_pointerCaptured)
			broadcastReleased(p);

		_pointerCaptured = false;
		_manualScrolling = false;
		_currentPointer = PointerInputInfo();
		_previousPointer = _currentPointer;
	}
	else 
	{
		broadcastReleased(p);
	}

	return true;
}

bool Scroll::pointerCancelled(const PointerInputInfo& p)
{
	_pointerCaptured = false;
	broadcastCancelled(p);
	return true;
}

bool Scroll::pointerScrolled(const PointerInputInfo& p)
{
	applyOffset(p.scroll * _pointerScrollMultiplier, _pointerScrollDuration);
	return true;
}

void Scroll::invalidateChildren()
{
	for (auto& c : children())
	{
		c->invalidateTransform();
		c->invalidateContent();
	}
}

Element* Scroll::getActiveElement(const PointerInputInfo& p, Element* root)
{
	if (root == this)
	{
		for (auto cI = root->children().rbegin(), cE = root->children().rend(); cI != cE; ++cI)
		{
			auto el = getActiveElement(p, cI->ptr());
			if (el != nullptr)
				return el;
		}
	}
	else if (root->enabled() && root->visible() && root->containsPoint(p.pos, p.normalizedPos) && !root->hasFlag(Flag_TransparentForPointer))
	{
		for (auto cI = root->children().rbegin(), cE = root->children().rend(); cI != cE; ++cI)
		{
			auto el = getActiveElement(p, cI->ptr());
			if (el != nullptr)
				return el;
		}
		return root;
	}
	
	return nullptr;
}

void Scroll::broadcastPressed(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos,
		p.scroll, p.id, p.timestamp, p.origin);
	
	auto active = getActiveElement(globalPos, this);
	if (active)
	{
		_capturedElement.reset(active);
		
		PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos),
			globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
		
		setActiveElement(localPos, active);
		active->pointerPressed(localPos);
	}
	else
	{
		setActiveElement(globalPos, nullptr);
	}
}

void Scroll::broadcastMoved(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll,
		p.id, p.timestamp, p.origin);
	
	Element* active = getActiveElement(globalPos, this);
	
	if (_capturedElement.valid() && (_capturedElement.ptr() != active))
	{
		PointerInputInfo localPos(p.type, _capturedElement->positionInElement(globalPos.pos),
			globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
		_capturedElement->pointerMoved(localPos);
	}
	
	if (active != nullptr)
	{
		PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos),
			globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
		setActiveElement(localPos, active);
		active->pointerMoved(localPos);
	}
	else
	{
		setActiveElement(globalPos, nullptr);
	}
}

void Scroll::broadcastReleased(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll,
		p.id, p.timestamp, p.origin);

	Element* active = getActiveElement(globalPos, this);
	
	if (_capturedElement.valid() && (_capturedElement.ptr() != active))
	{
		PointerInputInfo localPos(p.type, _capturedElement->positionInElement(globalPos.pos),
			globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
		_capturedElement->pointerReleased(localPos);
		_capturedElement.reset(nullptr);
	}
	
	if (active != nullptr)
	{
		PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos),
			globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
		setActiveElement(localPos, active);
		active->pointerReleased(localPos);
	}
	else
	{
		setActiveElement(globalPos, nullptr);
	}
}

void Scroll::broadcastCancelled(const PointerInputInfo& p)
{
	PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll,
		p.id, p.timestamp, p.origin);
	
	Element* active = getActiveElement(globalPos, this);
	
	if (_capturedElement.valid() && (_capturedElement.ptr() != active))
	{
		PointerInputInfo localPos(p.type, _capturedElement->positionInElement(globalPos.pos),
			globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
		_capturedElement->pointerCancelled(localPos);
		_capturedElement.reset(nullptr);
	}
	
	if (active != nullptr)
	{
		PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos),
			globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
		active->pointerCancelled(localPos);
	}
}

bool Scroll::containsPoint(const vec2& p, const vec2& np)
{
	return Element2d::containsPoint(p, np);
}

void Scroll::setActiveElement(const PointerInputInfo& p, Element* e)
{
	if (e == _activeElement.ptr()) return;
	
	if (_activeElement.valid())
	{
		_activeElement->pointerLeaved(p);
		_activeElement->hoverEnded.invoke(_activeElement.ptr());
	}
	
	_activeElement.reset(e);
	
	if (_activeElement.valid())
	{
		_activeElement->pointerEntered(p);
		_activeElement->hoverStarted.invoke(_activeElement.ptr());
	}
}

void Scroll::updateBouncing(float deltaTime)
{
	if (-_offset.x < scrollLeftDefaultValue())
		_bouncing.x = BounceDirection_ToNear;
	
	if (-_offset.x > scrollRightDefaultValue())
		_bouncing.x = BounceDirection_ToFar;
	
	if (-_offset.y < scrollUpperDefaultValue())
		_bouncing.y = BounceDirection_ToNear;
	
	if (-_offset.y > scrollLowerDefaultValue())
		_bouncing.y = BounceDirection_ToFar;

	if (_bouncing.x == BounceDirection_ToNear)
	{
		float diff = -_offset.x - scrollLeftDefaultValue();
		_velocity.x += 0.25f * size().x * diff * deltaTime;
		if ((_velocity.x <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.x = 0.0f;
			_offset.x = -scrollLeftDefaultValue();
			_bouncing.x = BounceDirection_None;
		}
	}
	else if (_bouncing.x == BounceDirection_ToFar)
	{
		float diff = -_offset.x - scrollRightDefaultValue();
		_velocity.x += 0.25f * size().x * diff * deltaTime;
		if ((_velocity.x <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.x = 0.0f;
			_offset.x = -scrollRightDefaultValue();
			_bouncing.x = BounceDirection_None;
		}
	}

	if (_bouncing.y == BounceDirection_ToNear)
	{
		float diff = -_offset.y - scrollUpperDefaultValue();
		_velocity.y += 0.25f * size().y * diff * deltaTime;
		if ((_velocity.y <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.y = 0.0f;
			_offset.y = -scrollUpperDefaultValue();
			_bouncing.y = BounceDirection_None;
		}
	}
	else if (_bouncing.y == BounceDirection_ToFar)
	{
		float diff = -_offset.y - scrollLowerDefaultValue();
		_velocity.y += 0.25f * size().y * diff * deltaTime;
		if ((_velocity.y <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold))
		{
			_velocity.y = 0.0f;
			_offset.y = -scrollLowerDefaultValue();
			_bouncing.y = BounceDirection_None;
		}
	}
}

void Scroll::update(float t)
{
	if (_updateTime == 0.0f)
		_updateTime = t;
	
	float deltaTime = t - _updateTime;
	
	_updateTime = t;
	
	_scrollbarsAlpha =
		mix(_scrollbarsAlpha, _scrollbarsAlphaTarget, etMin(1.0f, alphaAnimationScale * deltaTime));
	
	if (_scrollbarsAlpha < minAlpha)
		_scrollbarsAlpha = 0.0f;

	if (_manualScrolling || _offsetAnimator.running())
	{
		_scrollbarsAlphaTarget = 1.0f;
		float dt = _currentPointer.timestamp - _previousPointer.timestamp;
		if (dt > 1.0e-2)
		{
			vec2 dp = _currentPointer.pos - _previousPointer.pos;
			_velocity = mix(_velocity, dp * (accelerationRate / dt), 0.5f);
		}
		invalidateContent();
		return;
	}

	updateBouncing(deltaTime);
	
	float dt = etMin(1.0f, deltaTime * deccelerationRate);
	_velocity *= 1.0f - dt;
	
	_scrollbarsAlphaTarget = etMin(1.0f, _velocity.dotSelf() / maxScrollbarsVisibilityVelocity);

	if (_velocity.dotSelf() < 1.0f)
		_velocity = vec2(0.0f);

	vec2 dp = _velocity * deltaTime;
	
	if (dp.dotSelf() > 1.0e-6)
	{
		applyOffset(dp);
	}
	else if (std::abs(_scrollbarsAlpha - _scrollbarsAlphaTarget) > minAlpha)
	{
		invalidateContent();
	}
}

void Scroll::setContentSize(const vec2& cs)
{
	_contentSize = cs;
	invalidateContent();
}

void Scroll::adjustContentSize()
{
	vec2 size;
	
	for (const auto& ptr : children())
	{
		if (ptr->visible())
			size = maxv(size, ptr->origin() + ptr->size());
	}

	setContentSize(size);
}

void Scroll::applyOffset(const vec2& dOffset, float duration)
{
	setOffset(_offset + dOffset, duration);
}

void Scroll::setOffset(const vec2& aOffset, float duration)
{
	_offsetAnimator.cancelUpdates();
	
	if (duration == 0.0f)
	{
		_offsetAnimator.cancelUpdates();
		setOffsetDirectly(aOffset);
	}
	else
	{
		_offsetAnimator.animate(&_offset, _offset, aOffset, duration);
	}
}

float Scroll::scrollOutOfContentXSize() const
	{ return horizontalBounce() ? _bounceExtent.x * size().x : 0.001f; }

float Scroll::scrollOutOfContentYSize() const
	{ return verticalBounce() ? _bounceExtent.y * size().y : 0.001f; }

float Scroll::scrollUpperDefaultValue() const
	{ return 0.0f; }

float Scroll::scrollLowerDefaultValue() const
	{ return etMax(0.0f, _contentSize.y - size().y); }

float Scroll::scrollLeftDefaultValue() const
	{ return 0.0f; }

float Scroll::scrollRightDefaultValue() const
	{ return etMax(0.0f, _contentSize.x - size().x); }

float Scroll::scrollUpperLimit() const
	{ return scrollUpperDefaultValue() - scrollOutOfContentYSize(); }

float Scroll::scrollLowerLimit() const
	{ return scrollLowerDefaultValue() + scrollOutOfContentYSize(); }

float Scroll::scrollLeftLimit() const
	{ return scrollLeftDefaultValue() - scrollOutOfContentXSize(); }

float Scroll::scrollRightLimit() const
	{ return scrollRightDefaultValue() + scrollOutOfContentXSize(); }

void Scroll::setOffsetDirectly(const vec2& o)
{
	_offset = o;
	vec2 actualOffset = -_offset;
	
	float leftLimit = scrollLeftLimit();
	float rightLimit = scrollRightLimit();
	float upperLimit = scrollUpperLimit();
	float lowerLimit = scrollLowerLimit();

	if (actualOffset.x < leftLimit)
	{
		_offset.x = -leftLimit;
		_velocity.x = 0.0f;
	}
	
	if (actualOffset.x > rightLimit)
	{
		_offset.x = -rightLimit;
		_velocity.x = 0.0f;
	}

	if (actualOffset.y < upperLimit)
	{
		_offset.y = -upperLimit;
		_velocity.y = 0.0f;
	}
	
	if (actualOffset.y > lowerLimit)
	{
		_offset.y = -lowerLimit;
		_velocity.y = 0.0f;
	}
	
	invalidateContent();
	invalidateChildren();
}

void Scroll::setBackgroundColor(const vec4& color)
{
	_backgroundColor = color;
	invalidateContent();
}

void Scroll::setScrollbarsColor(const vec4& c)
{
	_scrollbarsColor = c;
	invalidateContent();
}

void Scroll::setPointerScrollMultiplier(const vec2& m)
{
	_pointerScrollMultiplier = m;
}

void Scroll::setPointerScrollDuration(float d)
{
	_pointerScrollDuration = d;
}

void Scroll::setBounce(size_t b)
{
	_bounce = b;
}

void Scroll::setBounceExtent(const vec2& e)
{
	_bounceExtent = e;
}

void Scroll::scrollToBottom(float delay)
{
	setOffset(vec2(_offset.x, etMin(0.0f, size().y - _contentSize.y)), delay);
}

vec2 Scroll::contentSize()
{
	return _contentSize;
}

void Scroll::setOverlayColor(const vec4& color)
{
	_overlayColor = color;
	invalidateContent();
}

void Scroll::setMovementTreshold(float t)
{
	_movementTreshold = t;
}
