/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/baseclasses.hpp>
#include <et/input/input.hpp>
#include <et/rendering/base/material.hpp>

namespace et {
namespace s2d {
class Layout;
class Element2d;
using ElementHierarchy = Hierarchy<Element2d, LoadableObject>;
class Element2d : public ElementHierarchy, public FlagsHolder, public EventReceiver, public TimedObject {
 public:
  ET_DECLARE_POINTER(Element2d);
  using Collection = Vector<Pointer>;

  int64_t tag = 0;

 public:
  Element2d(Element2d* parent, const std::string& name = emptyString);
  Element2d(const rect& frame, Element2d* parent, const std::string& name = emptyString);

  void setParent(Element2d* element);

  bool contentValid() const {
    return _contentValid;
  }

  void invalidateContent();

  const ElementLayout& autoLayout() const {
    return _autoLayout;
  }

  Dictionary autoLayoutDictionary() const;

  void setAutolayout(const Dictionary&);
  void setAutolayout(const ElementLayout&);
  void setAutolayout(const vec2&, LayoutMode, const vec2&, LayoutMode, const vec2&);
  void setAutolayoutPosition(const vec2&);
  void setAutolayoutPositionMode(LayoutMode);
  void setAutolayoutSize(const vec2&);
  void setAutolayoutSizeMode(LayoutMode);
  void setAutolayoutRelativeToParent(const vec2& pos, const vec2& sz, const vec2& pivot);
  void setLocationInParent(Location, const vec2& offset = vec2(0.0f));
  void setAutolayoutMask(size_t);
  void fillParent();

  void autoLayoutFromFile(const std::string&);
  void autoLayout(const vec2& contextSize, float duration = 0.0f);

  virtual bool focused() const {
    return _focused;
  }

  virtual void setFocus() {
    _focused = true;
  };

  virtual void resignFocus(Element2d*) {
    _focused = false;
  };

  virtual void didAppear() {}
  virtual void willAppear() {}

  virtual void didDisappear() {}
  virtual void willDisappear() {}

  virtual void willAutoLayout(float) {}
  virtual void didAutoLayout(float) {}

  virtual void willChangeFrame() {}
  virtual void didChangeFrame() {}

  virtual void storeProperties(const Dictionary&) const {}
  virtual void loadProperties(const Dictionary&) {}

  virtual bool enabled() const;
  virtual void setEnabled(bool enabled);

  virtual void broadcastMessage(const Message&);

  virtual bool processMessage(const Message&) {
    return false;
  }

  virtual bool respondsToMessage(const Message&) const {
    return false;
  }

  virtual void buildVertices(RenderInterface::Pointer&, SceneRenderer&) {}
  virtual void addToRenderQueue(RenderInterface::Pointer&, SceneRenderer&) {}
  virtual void addToOverlayRenderQueue(RenderInterface::Pointer&, SceneRenderer&) {}
  virtual void preRender(RenderInterface::Pointer&) {}

  virtual bool pointerPressed(const PointerInputInfo& info) {
    onPointerPressed.invoke(info);
    return !hasFlag(Flag_TransparentForPointer);
  }

  virtual bool pointerMoved(const PointerInputInfo& info) {
    onPointerMoved.invoke(info);
    return !hasFlag(Flag_TransparentForPointer);
  }

  virtual bool pointerReleased(const PointerInputInfo& info) {
    onPointerReleased.invoke(info);
    return !hasFlag(Flag_TransparentForPointer);
  }

  virtual bool pointerCancelled(const PointerInputInfo& info) {
    onPointerCancelled.invoke(info);
    return !hasFlag(Flag_TransparentForPointer);
  }

  virtual bool pointerScrolled(const PointerInputInfo&) {
    return !hasFlag(Flag_TransparentForPointer);
  }

  virtual void pointerEntered(const PointerInputInfo&) {}
  virtual void pointerLeaved(const PointerInputInfo&) {}

  virtual bool keyPressed(size_t) {
    return false;
  }

  virtual bool containsPoint(const vec2& p, const vec2&);
  virtual bool containLocalPoint(const vec2& p);

  vec2 positionInElement(const vec2& p);

  virtual Layout* owner() {
    Element2d* aParent = parent();
    if (aParent != nullptr) {
      return aParent->owner();
    }
    return nullptr;
  }

  void bringToFront(Element2d* c);
  void sendToBack(Element2d* c);
  void removeAllChildren();

  Element2d* baseChildWithName(const std::string&, bool recursive);

  template <typename T>
  T* childWithName(const std::string& name, bool recursive) {
    return static_cast<T*>(baseChildWithName(name, recursive));
  }

  /*
   * Animator accessors
   */
  Vector2Animator& positionAnimator() {
    return _positionAnimator;
  }

  Vector2Animator& sizeAnimator() {
    return _sizeAnimator;
  }

  Vector4Animator& colorAnimator() {
    return _colorAnimator;
  }

  Vector2Animator& scaleAnimator() {
    return _scaleAnimator;
  }

  FloatAnimator& angleAnimator() {
    return _angleAnimator;
  }

  rect frame() const;
  vec2 origin() const;
  vec2 offset() const;

  /*
   * Scale
   */
  const vec2& scale() const;
  const vec2& desiredScale() const;
  void setScale(const vec2& scale, float duration = 0.0f);

  /*
   * Pivot point
   */
  const vec2& pivotPoint() const;
  void setPivotPoint(const vec2& p, bool preservePosition = true);

  /*
   * Position
   */
  const vec2& position() const;
  const vec2& desiredPosition() const;
  void setPosition(const vec2& p, float duration = 0.0f);
  void setPosition(float x, float y, float duration = 0.0f);

  /*
   * Size
   */
  vec2 contentSize();
  const vec2& size() const;
  const vec2& desiredSize() const;
  void setSize(const vec2& s, float duration = 0.0f);
  void setSize(float w, float h, float duration = 0.0f);

  /*
   * Rotation
   */
  float angle() const;
  void setAngle(float angle, float duration = 0.0f);
  void rotate(float angle, float duration = 0.0f);

  /*
   * Visibility
   */
  bool visible();
  void setVisible(bool visible, float duration = 0.0f);

  /*
   * Color
   */
  vec4 finalColor();
  float finalAlpha();
  const vec4& ownColor() const;
  void setAlpha(float alpha, float duration = 0.0f);
  void setColor(const vec4& color, float duration = 0.0f);

  /*
   * Transformations
   */
  const mat4& transform();

  virtual const mat4& finalTransform();
  virtual const mat4& finalInverseTransform();

  bool transformValid() const {
    return _transformValid;
  }

  void invalidateTransform();
  virtual void validateMaterialInstance(SceneRenderer& renderer);

  virtual void reloadFromFile(const std::string&) {}

  /*
   * Events
   */
  ET_DECLARE_EVENT2(dragStarted, Element2d*, const ElementDragInfo&);
  ET_DECLARE_EVENT2(dragged, Element2d*, const ElementDragInfo&);
  ET_DECLARE_EVENT2(dragFinished, Element2d*, const ElementDragInfo&);

  ET_DECLARE_EVENT1(hoverStarted, Element2d*);
  ET_DECLARE_EVENT1(hoverEnded, Element2d*);

  ET_DECLARE_EVENT2(elementAnimationFinished, Element2d*, AnimatedPropery);

  ET_DECLARE_EVENT1(onPointerPressed, const PointerInputInfo&);
  ET_DECLARE_EVENT1(onPointerMoved, const PointerInputInfo&);
  ET_DECLARE_EVENT1(onPointerReleased, const PointerInputInfo&);
  ET_DECLARE_EVENT1(onPointerCancelled, const PointerInputInfo&);

 protected:
  void initAnimators();
  void buildFinalTransform();
  mat4 buildFinalTransform(const vec2& aOffset, float aAngle, const vec2& aScale, const vec2& aPosition);

  MaterialInstance::Pointer& materialInstance();
  const MaterialInstance::Pointer& materialInstance() const;
  virtual MaterialInstance::Pointer allocateMaterial(SceneRenderer&);

  /*
   * From Element
   */
  void childRemoved(Element2d*);

  void setContentValid() {
    _contentValid = true;
  }

  bool inverseTransformValid() {
    return _inverseTransformValid;
  }

  virtual void setInvalid() {
    if (parent()) parent()->setInvalid();
  }

  void setTransformValid(bool v) {
    _transformValid = v;
  }

  void setInverseTransformValid(bool v) {
    _inverseTransformValid = v;
  }

  virtual mat4 parentFinalTransform() {
    return parent() ? parent()->finalTransform() : mat4::identity();
  }

  Element2d* childWithNameCallback(const std::string&, Element2d*, bool recursive);

 private:
  friend class Hierarchy<Element2d, LoadableObject>;
  ET_DENY_COPY(Element2d);

  MaterialInstance::Pointer _material;
  Vector2Animator _positionAnimator;
  Vector2Animator _sizeAnimator;
  Vector4Animator _colorAnimator;
  Vector2Animator _scaleAnimator;
  FloatAnimator _angleAnimator;

  mat4 _transform = mat4::identity();
  mat4 _finalTransform = mat4::identity();
  mat4 _finalInverseTransform = mat4::identity();

  ElementLayout _layout;
  ElementLayout _autoLayout;
  ElementLayout _desiredLayout;

  float _finalAlpha = 1.0f;

  bool _enabled = true;
  bool _transformValid = false;
  bool _inverseTransformValid = false;
  bool _contentValid = false;
  bool _finalAlphaValid = false;
  bool _focused = false;
};

inline bool elementIsSelected(State s) {
  return (s >= State_Selected) && (s < State_max);
}

State adjustState(State s);
float alignmentFactor(Alignment a);
std::string layoutModeToString(LayoutMode);
LayoutMode layoutModeFromString(const std::string&);

inline float linearInerpolation(float t) {
  return t;
}

inline float easeOutInerpolation(float t) {
  return std::sqrt(t);
}

inline float easeInInerpolation(float t) {
  return t * t;
}

template <int bounce>
inline float sinBounceScaledInterpolation(float t) {
  float v = t + 0.1f * static_cast<float>(bounce) * std::sin(t * PI);
  return v * v;
}

template <int bounce>
inline float sinBounceScaledInterpolationWithBackMoving(float t) {
  return t - 0.1f * static_cast<float>(bounce) * std::sin(t * DOUBLE_PI);
}

inline float sinBounceInterpolation(float t) {
  return sinBounceScaledInterpolation<4>(t);
}

inline float polynomialBounceInterpolation(float t) {
  float ts = t * t;
  return t + ts * (1.0f - ts);
}
}  // namespace s2d
}  // namespace et
