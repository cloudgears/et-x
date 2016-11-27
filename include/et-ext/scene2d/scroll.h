/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>

namespace et
{
namespace s2d
{
class Scroll : public Element2d
{
public:
	ET_DECLARE_POINTER(Scroll);

	enum Bounce
	{
		Bounce_Horizontal = 0x01,
		Bounce_Vertical = 0x02,
	};

public:
	Scroll(Element2d* parent, const std::string& name = emptyString);

	void setBounce(size_t);
	void setBounceExtent(const vec2&);

	void setContentSize(const vec2& cs);

	void setContentOffset(const vec2& aOffset, float duration = 0.0f);
	void applyContentOffset(const vec2& dOffset, float duration = 0.0f);

	vec2 contentSize();
	void adjustContentSize(bool includeHiddenItems = false);

	void setBackgroundColor(const vec4& color);
	void setOverlayColor(const vec4& color);
	void setScrollbarsColor(const vec4&);

	void setMovementTreshold(float);

	void setPointerScrollDuration(float);

	const vec2& contentOffset() const
	{
		return _contentOffset;
	}

	void scrollToBottom(float duration = 0.0f);

	float pointerScrollDuration() const
	{
		return _pointerScrollDuration;
	}

	void setShouldUseIntegerValuesForScroll(bool);

	ET_DECLARE_EVENT1(contentOffsetPercentageUpdated, vec2);
	ET_DECLARE_EVENT0(manualScrollStarted);
	ET_DECLARE_EVENT2(manualScroll, vec2, vec2);
	ET_DECLARE_EVENT0(manualScrollEnded);
	ET_DECLARE_EVENT0(scrollEnded);

protected:
	virtual void setOffsetDirectly(const vec2& o);

protected:
	void update(float t);

	bool pointerPressed(const PointerInputInfo&);
	bool pointerMoved(const PointerInputInfo&);
	bool pointerReleased(const PointerInputInfo&);
	bool pointerCancelled(const PointerInputInfo&);
	bool pointerScrolled(const PointerInputInfo&);

private:
	void buildVertices(RenderContext* rc, SceneRenderer& r);

	void addToRenderQueue(RenderContext*, SceneRenderer&);
	void addToOverlayRenderQueue(RenderContext*, SceneRenderer&);

	const mat4& finalTransform();
	const mat4& finalInverseTransform();

	bool containsPoint(const vec2& p, const vec2& np);

	void invalidateChildren();
	void broadcastPressed(const PointerInputInfo&);
	void broadcastMoved(const PointerInputInfo&);
	void broadcastReleased(const PointerInputInfo&);
	void broadcastCancelled(const PointerInputInfo&);

	float scrollOutOfContentXSize() const;
	float scrollOutOfContentYSize() const;

	void getLimits(vec4&);
	void getDefaultValues(vec4&);

	void updateBouncing(float deltaTime);

	bool horizontalBounce() const
	{
		return (_bounce & Bounce_Horizontal) != 0;
	}

	bool verticalBounce() const
	{
		return (_bounce & Bounce_Vertical) != 0;
	}

	Element2d* getActiveElement(const PointerInputInfo&, Element2d* root);
	void setActiveElement(const PointerInputInfo& p, Element2d*, bool setFocus);

private:
	enum BounceDirection
	{
		BounceDirection_None,
		BounceDirection_ToNear,
		BounceDirection_ToFar
	};

private:
	Element2d::Pointer _activeElement;
	Element2d::Pointer _capturedElement;

	SceneVertexList _backgroundVertices;
	SceneVertexList _overlayVertices;

	Vector2Animator _contentOffsetAnimator;

	PointerInputInfo _currentPointer;
	PointerInputInfo _previousPointer;

	mat4 _localFinalTransform = mat4(1.0f);
	mat4 _localInverseTransform = mat4(1.0f);
	vec4 _backgroundColor = vec4(0.0f);
	vec4 _scrollbarsColor = vec4(0.0f);
	vec4 _overlayColor = vec4(0.0f);
	vec2 _contentSize = vec2(0.0f);
	vec2 _contentOffset = vec2(0.0f);
	vec2 _velocity = vec2(0.0f);
	vec2 _bounceExtent = vec2(0.5f);
	vector2<BounceDirection> _bouncing = vector2<BounceDirection>(BounceDirection_None);
	size_t _bounce = 0;
	float _updateTime = 0.0f;
	float _scrollbarsAlpha = 0.0f;
	float _scrollbarsAlphaTarget = 0.0f;
	float _pointerScrollDuration = 0.0f;
	float _movementTreshold = 9.0f;

	bool _pointerCaptured = false;
	bool _manualScrolling = false;
	bool _floorOffset = false;
};

}
}
