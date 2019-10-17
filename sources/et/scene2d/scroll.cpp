/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/hardware.hpp>
#include <et/core/tools.hpp>
#include <et/rendering/rendercontext.hpp>
#include <et/scene2d/layout.hpp>
#include <et/scene2d/scenerenderer.hpp>
#include <et/scene2d/scroll.hpp>

namespace et {
namespace s2d {

float deccelerationRate = 10.0f;
float accelerationRate = 0.5f;
float scrollbarSize = 5.0f;
float maxScrollbarsVisibilityVelocity = 50.0f;
float minAlpha = 1.0f / 255.0f;
float alphaAnimationScale = 5.0f;
float bounceStopTreshold = 0.5f;

Scroll::Scroll(Element2d* parent, const std::string& name)
  : Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS)
  , _contentOffsetAnimator(timerPool()) {
  _contentOffsetAnimator.updated.connect([this]() { setOffsetDirectly(_contentOffset); });
  _contentOffsetAnimator.finished.connect([this]() { scrollEnded.invoke(); });

  setFlag(Flag_HandlesChildEvents);
  setFlag(Flag_ClipToBounds);
  startUpdates();
}

void Scroll::addToRenderQueue(RenderInterface::Pointer& rc, SceneRenderer& r) {
  if (_backgroundVertices.lastElementIndex() > 0) {
    materialInstance()->setTexture(MaterialTexture::BaseColor, r.transparentTexture());
    r.addVertices(_backgroundVertices, materialInstance(), this);
  }
}

void Scroll::addToOverlayRenderQueue(RenderInterface::Pointer& rc, SceneRenderer& r) {
  if (_overlayVertices.lastElementIndex() > 0) {
    materialInstance()->setTexture(MaterialTexture::BaseColor, r.transparentTexture());
    r.addVertices(_overlayVertices, materialInstance(), this);
  }
}

void Scroll::buildVertices(RenderInterface::Pointer&, SceneRenderer&) {
  _backgroundVertices.setOffset(0);
  _overlayVertices.setOffset(0);

  vec4 alphaScale(1.0f, finalAlpha());

  if (_backgroundColor.w > 0.0f) {
    buildColorVertices(_backgroundVertices, rect(vec2(0.0f), size()), alphaScale * _backgroundColor, Element2d::finalTransform());
  }

  if (_scrollbarsColor.w > 0.0f) {
    float scaledScollbarSize = scrollbarSize * currentScreen().scaleFactor;
    float barHeight = size().y * (size().y / _contentSize.y);
    float barOffset = size().y * (_contentOffset.y / _contentSize.y);
    vec2 origin(size().x - 2.0f * scaledScollbarSize, -barOffset);

    vec4 adjutsedColor = alphaScale * _scrollbarsColor;
    adjutsedColor.w *= _scrollbarsAlpha;

    buildColorVertices(_overlayVertices, rect(origin, vec2(scaledScollbarSize, barHeight)), adjutsedColor, Element2d::finalTransform());
  }

  if (_overlayColor.w > 0.0f) buildColorVertices(_overlayVertices, rect(vec2(0.0f), size()), alphaScale * _overlayColor, Element2d::finalTransform());

  setContentValid();
}

const mat4& Scroll::finalTransform() {
  Element2d::finalTransform();

  _localFinalTransform = buildFinalTransform(offset(), angle(), scale(), position() + _contentOffset) * parentFinalTransform();

  if (_floorOffset) _localFinalTransform[3].xy() = floorv(_localFinalTransform[3].xy());

  return _localFinalTransform;
}

const mat4& Scroll::finalInverseTransform() {
  _localInverseTransform = Element2d::finalTransform().inverted();
  return _localInverseTransform;
}

bool Scroll::pointerPressed(const PointerInputInfo& p) {
  _contentOffsetAnimator.cancelUpdates();

  if ((_currentPointer.id == 0) && (p.type == PointerTypeMask::General)) {
    _previousPointer = p;
    _currentPointer = p;
    _manualScrolling = false;
    _pointerCaptured = false;
    _velocity = vec2(0.0f);
  }

  broadcastPressed(p);
  return true;
}

bool Scroll::pointerMoved(const PointerInputInfo& p) {
  if (p.id != _currentPointer.id) {
    broadcastMoved(p);
    return true;
  }

  vec2 aOffset = p.pos - _currentPointer.pos;
  if (!_manualScrolling && (aOffset.dotSelf() < sqr(_movementTreshold))) {
    if (is_valid(_capturedElement)) broadcastMoved(p);

    return true;
  }

  if (_manualScrolling) {
    vec4 defaultValues;
    getDefaultValues(defaultValues);

    vec2 offsetScale(1.0f);
    if (-_contentOffset.x < defaultValues.x) {
      float diff = std::abs(-_contentOffset.x - defaultValues.x);
      offsetScale.x *= std::max(0.0f, 1.0f - diff / scrollOutOfContentXSize());
    } else if (-_contentOffset.x > defaultValues.z) {
      float diff = std::abs(-_contentOffset.x - defaultValues.z);
      offsetScale.x *= std::max(0.0f, 1.0f - diff / scrollOutOfContentXSize());
    }
    if (-_contentOffset.y < defaultValues.y) {
      float diff = std::abs(-_contentOffset.y - defaultValues.y);
      offsetScale.y *= std::max(0.0f, 1.0f - diff / scrollOutOfContentYSize());
    } else if (-_contentOffset.y > defaultValues.w) {
      float diff = std::abs(-_contentOffset.y - defaultValues.w);
      offsetScale.y *= std::max(0.0f, 1.0f - diff / scrollOutOfContentYSize());
    }
    offsetScale *= offsetScale;

    applyContentOffset(offsetScale * aOffset);
    manualScroll.invoke(aOffset, offsetScale);
  } else if (!_pointerCaptured && (p.type == PointerTypeMask::General)) {
    _manualScrolling = true;
    _pointerCaptured = true;
    _scrollbarsAlphaTarget = 1.0f;
    _bouncing = vector2<BounceDirection>(BounceDirection_None);
    broadcastCancelled(p);

    manualScrollStarted.invoke();
  }

  _previousPointer = _currentPointer;
  _currentPointer = p;

  return true;
}

bool Scroll::pointerReleased(const PointerInputInfo& p) {
  if (p.id == _currentPointer.id) {
    if (!_pointerCaptured) broadcastReleased(p);

    _pointerCaptured = false;
    _manualScrolling = false;
    _currentPointer = PointerInputInfo();
    _previousPointer = _currentPointer;

    manualScrollEnded.invoke();
  } else {
    broadcastReleased(p);
  }

  return true;
}

bool Scroll::pointerCancelled(const PointerInputInfo& p) {
  _pointerCaptured = false;
  _currentPointer = PointerInputInfo();

  broadcastCancelled(p);

  manualScrollEnded.invoke();

  return true;
}

bool Scroll::pointerScrolled(const PointerInputInfo& p) {
  vec2 scrollValue;
  scrollValue.x = p.scroll.x * (std::abs(p.scroll.x) < 1.0f ? size().x : 0.05f * size().x);
  scrollValue.y = p.scroll.y * (std::abs(p.scroll.y) < 1.0f ? size().y : 0.05f * size().y);
  applyContentOffset(scrollValue, _pointerScrollDuration);
  return true;
}

void Scroll::invalidateChildren() {
  for (auto& c : children()) {
    c->invalidateTransform();
    c->invalidateContent();
  }
}

Element2d* Scroll::getActiveElement(const PointerInputInfo& p, Element2d* root) {
  if (root == this) {
    for (auto cI = root->children().rbegin(), cE = root->children().rend(); cI != cE; ++cI) {
      auto el = getActiveElement(p, *cI);
      if (el != nullptr) return el;
    }
  } else if (root->visible() && root->enabled() && root->containsPoint(p.pos, p.normalizedPos) && !root->hasFlag(Flag_TransparentForPointer)) {
    for (auto cI = root->children().rbegin(), cE = root->children().rend(); cI != cE; ++cI) {
      auto el = getActiveElement(p, *cI);
      if (el != nullptr) return el;
    }
    return root;
  }

  return nullptr;
}

void Scroll::broadcastPressed(const PointerInputInfo& p) {
  PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);

  auto active = getActiveElement(globalPos, this);
  if (active) {
    _capturedElement = active;

    PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos), globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);

    setActiveElement(localPos, active, true);
    active->pointerPressed(localPos);
  } else {
    setActiveElement(globalPos, nullptr, false);
  }
}

void Scroll::broadcastMoved(const PointerInputInfo& p) {
  PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);

  Element2d* active = getActiveElement(globalPos, this);

  if (is_valid(_capturedElement) && (_capturedElement != active)) {
    PointerInputInfo localPos(p.type, _capturedElement->positionInElement(globalPos.pos), globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
    _capturedElement->pointerMoved(localPos);
  }

  if (active != nullptr) {
    PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos), globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
    setActiveElement(localPos, active, false);
    active->pointerMoved(localPos);
  } else {
    setActiveElement(globalPos, nullptr, false);
  }
}

void Scroll::broadcastReleased(const PointerInputInfo& p) {
  PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);

  Element2d* active = getActiveElement(globalPos, this);

  if (is_valid(_capturedElement) && (_capturedElement != active)) {
    PointerInputInfo localPos(p.type, _capturedElement->positionInElement(globalPos.pos), globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
    _capturedElement->pointerReleased(localPos);
    _capturedElement = nullptr;
  }

  if (active != nullptr) {
    PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos), globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
    setActiveElement(localPos, active, false);
    active->pointerReleased(localPos);
  } else {
    setActiveElement(globalPos, nullptr, false);
  }
}

void Scroll::broadcastCancelled(const PointerInputInfo& p) {
  PointerInputInfo globalPos(p.type, Element2d::finalTransform() * p.pos, p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);

  Element2d* active = getActiveElement(globalPos, this);

  if (is_valid(_capturedElement) && (_capturedElement != active)) {
    PointerInputInfo localPos(p.type, _capturedElement->positionInElement(globalPos.pos), globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
    _capturedElement->pointerCancelled(localPos);
    _capturedElement = nullptr;
  }

  if (active != nullptr) {
    PointerInputInfo localPos(p.type, active->positionInElement(globalPos.pos), globalPos.normalizedPos, p.scroll, p.id, p.timestamp, p.origin);
    active->pointerCancelled(localPos);
  }
}

bool Scroll::containsPoint(const vec2& p, const vec2& np) {
  return Element2d::containsPoint(p, np);
}

void Scroll::setActiveElement(const PointerInputInfo& p, Element2d* e, bool setFocus) {
  if (e != _activeElement) {
    if (is_valid(_activeElement)) {
      _activeElement->pointerLeaved(p);
      _activeElement->hoverEnded.invoke(_activeElement);
    }

    _activeElement = e;

    if (is_valid(_activeElement)) {
      _activeElement->pointerEntered(p);
      _activeElement->hoverStarted.invoke(_activeElement);
    }
  }

  if (is_valid(_activeElement) && setFocus) {
    owner()->setFocusedElement(_activeElement);
  }
}

void Scroll::updateBouncing(float deltaTime) {
  vec4 defaultValues;
  getDefaultValues(defaultValues);

  float velocityBefore = _velocity.dotSelf();

  if (-_contentOffset.x < defaultValues.x) _bouncing.x = BounceDirection_ToNear;

  if (-_contentOffset.x > defaultValues.z) _bouncing.x = BounceDirection_ToFar;

  if (-_contentOffset.y < defaultValues.y) _bouncing.y = BounceDirection_ToNear;

  if (-_contentOffset.y > defaultValues.w) _bouncing.y = BounceDirection_ToFar;

  if (_bounce & Bounce_Horizontal) {
    if (_bouncing.x == BounceDirection_ToNear) {
      float diff = -_contentOffset.x - defaultValues.x;
      _velocity.x += 0.25f * size().x * diff * deltaTime;
      if ((_velocity.x <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold)) {
        _velocity.x = 0.0f;
        _contentOffset.x = -defaultValues.x;
        _bouncing.x = BounceDirection_None;
      }
    } else if (_bouncing.x == BounceDirection_ToFar) {
      float diff = -_contentOffset.x - defaultValues.z;
      _velocity.x += 0.25f * size().x * diff * deltaTime;
      if ((_velocity.x <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold)) {
        _velocity.x = 0.0f;
        _contentOffset.x = -defaultValues.z;
        _bouncing.x = BounceDirection_None;
      }
    }
  }

  if (_bounce & Bounce_Vertical) {
    if (_bouncing.y == BounceDirection_ToNear) {
      float diff = -_contentOffset.y - defaultValues.y;
      _velocity.y += 0.25f * size().y * diff * deltaTime;
      if ((_velocity.y <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold)) {
        _velocity.y = 0.0f;
        _contentOffset.y = -defaultValues.y;
        _bouncing.y = BounceDirection_None;
      }
    } else if (_bouncing.y == BounceDirection_ToFar) {
      float diff = -_contentOffset.y - defaultValues.w;
      _velocity.y += 0.25f * size().y * diff * deltaTime;
      if ((_velocity.y <= bounceStopTreshold) && (std::abs(diff) <= bounceStopTreshold)) {
        _velocity.y = 0.0f;
        _contentOffset.y = -defaultValues.w;
        _bouncing.y = BounceDirection_None;
      }
    }
  }

  if (velocityBefore > 0.0f) {
    if (_velocity.dotSelf() == 0.0f) scrollEnded.invoke();
  }
}

void Scroll::update(float t) {
  if (_updateTime == 0.0f) _updateTime = t;

  float deltaTime = t - _updateTime;

  _updateTime = t;

  _scrollbarsAlpha = mix(_scrollbarsAlpha, _scrollbarsAlphaTarget, std::min(1.0f, alphaAnimationScale * deltaTime));

  if (_scrollbarsAlpha < minAlpha) _scrollbarsAlpha = 0.0f;

  if (_manualScrolling || _contentOffsetAnimator.running()) {
    _scrollbarsAlphaTarget = 1.0f;
    float dt = _currentPointer.timestamp - _previousPointer.timestamp;
    if (dt > 1.0e-2) {
      vec2 dp = _currentPointer.pos - _previousPointer.pos;
      _velocity = mix(_velocity, dp * (accelerationRate / dt), 0.5f);
    }
    invalidateContent();
    return;
  }

  updateBouncing(deltaTime);

  float dt = std::min(1.0f, deltaTime * deccelerationRate);
  _velocity *= 1.0f - dt;

  _scrollbarsAlphaTarget = std::min(1.0f, _velocity.dotSelf() / maxScrollbarsVisibilityVelocity);

  if (_velocity.dotSelf() < 1.0f) _velocity = vec2(0.0f);

  vec2 dp = _velocity * deltaTime;

  if (dp.dotSelf() > std::numeric_limits<float>::epsilon()) {
    applyContentOffset(dp);
  } else if (std::abs(_scrollbarsAlpha - _scrollbarsAlphaTarget) > minAlpha) {
    invalidateContent();
  }
}

void Scroll::setContentSize(const vec2& cs) {
  _contentSize = cs;
  invalidateContent();
}

void Scroll::adjustContentSize(bool includeHiddenItems) {
  vec2 sizeToSet;

  for (auto ptr : children()) {
    if (includeHiddenItems || ptr->visible()) sizeToSet = maxv(sizeToSet, ptr->origin() + ptr->size());
  }

  setContentSize(sizeToSet);
}

void Scroll::applyContentOffset(const vec2& dOffset, float duration) {
  setContentOffset(_contentOffset + dOffset, duration);
}

void Scroll::setContentOffset(const vec2& aOffset, float duration) {
  _contentOffsetAnimator.cancelUpdates();

  if (duration == 0.0f) {
    setOffsetDirectly(aOffset);
    scrollEnded.invoke();
  } else {
    _contentOffsetAnimator.animate(&_contentOffset, _contentOffset, aOffset, duration);
  }
}

void Scroll::getLimits(vec4& values) {
  vec4 defaultValues;
  getDefaultValues(defaultValues);

  vec2 outSizes(scrollOutOfContentXSize(), scrollOutOfContentYSize());

  values.x = defaultValues.x - outSizes.x;
  values.z = defaultValues.z + outSizes.x;

  values.y = defaultValues.y - outSizes.y;
  values.w = defaultValues.w + outSizes.y;
}

void Scroll::getDefaultValues(vec4& values) {
  vec2 scaledContentSize = maxv(vec2(0.0f), _contentSize - size());

  values.x = 0.0f;
  values.y = 0.0f;
  values.z = scaledContentSize.x;
  values.w = scaledContentSize.y;
}

float Scroll::scrollOutOfContentXSize() const {
  return horizontalBounce() ? _bounceExtent.x * size().x : std::numeric_limits<float>::epsilon();
}

float Scroll::scrollOutOfContentYSize() const {
  return verticalBounce() ? _bounceExtent.y * size().y : std::numeric_limits<float>::epsilon();
}

void Scroll::setOffsetDirectly(const vec2& o) {
  _contentOffset = o;

  vec4 limits;
  getLimits(limits);

  vec2 actualOffset = -_contentOffset;

  if (actualOffset.x < limits.x) {
    _contentOffset.x = -limits.x;
    _velocity.x = 0.0f;
  }

  if (actualOffset.x > limits.z) {
    _contentOffset.x = -limits.z;
    _velocity.x = 0.0f;
  }

  if (actualOffset.y < limits.y) {
    _contentOffset.y = -limits.y;
    _velocity.y = 0.0f;
  }

  if (actualOffset.y > limits.w) {
    _contentOffset.y = -limits.w;
    _velocity.y = 0.0f;
  }

  vec2 dl(limits.x - limits.z, limits.y - limits.w);
  if (std::abs(dl.x) < std::numeric_limits<float>::epsilon()) dl.x = _contentSize.x;
  if (std::abs(dl.y) < std::numeric_limits<float>::epsilon()) dl.y = _contentSize.y;

  contentOffsetPercentageUpdated.invoke(_contentOffset / dl);

  invalidateContent();
  invalidateChildren();
}

void Scroll::setBackgroundColor(const vec4& color) {
  _backgroundColor = color;
  invalidateContent();
}

void Scroll::setScrollbarsColor(const vec4& c) {
  _scrollbarsColor = c;
  invalidateContent();
}

void Scroll::setPointerScrollDuration(float d) {
  _pointerScrollDuration = d;
}

void Scroll::setBounce(size_t b) {
  _bounce = b;
}

void Scroll::setBounceExtent(const vec2& e) {
  _bounceExtent = e;
}

void Scroll::scrollToBottom(float delay) {
  setContentOffset(vec2(_contentOffset.x, std::min(0.0f, size().y - _contentSize.y)), delay);
}

vec2 Scroll::contentSize() {
  return _contentSize;
}

void Scroll::setOverlayColor(const vec4& color) {
  _overlayColor = color;
  invalidateContent();
}

void Scroll::setMovementTreshold(float t) {
  _movementTreshold = t;
}

void Scroll::setShouldUseIntegerValuesForScroll(bool v) {
  _floorOffset = v;
  setOffsetDirectly(_contentOffset);
}

}  // namespace s2d
}  // namespace et
