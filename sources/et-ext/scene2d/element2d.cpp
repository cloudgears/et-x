/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/scene2d/element2d.hpp>
#include <et-ext/scene2d/scenerenderer.hpp>
#include <et/core/filesystem.hpp>
#include <et/core/json.hpp>
#include <et/geometry/conversion.hpp>

namespace et {
namespace s2d {

Element2d::Element2d(Element2d* parent, const std::string& name)
  : ElementHierarchy(parent)
  , _positionAnimator(timerPool())
  , _sizeAnimator(timerPool())
  , _colorAnimator(timerPool())
  , _scaleAnimator(timerPool())
  , _angleAnimator(timerPool()) {
  setName(name);
  initAnimators();
}

Element2d::Element2d(const rect& frame, Element2d* parent, const std::string& name)
  : ElementHierarchy(parent)
  , _positionAnimator(timerPool())
  , _sizeAnimator(timerPool())
  , _colorAnimator(timerPool())
  , _scaleAnimator(timerPool())
  , _angleAnimator(timerPool())
  , _layout(frame)
  , _desiredLayout(frame) {
  setName(name);
  initAnimators();
}

void Element2d::initAnimators() {
  _sizeAnimator.setTag(AnimatedProperty_Size);
  _sizeAnimator.updated.connect(this, &Element2d::invalidateTransform);
  _sizeAnimator.updated.connect(this, &Element2d::invalidateContent);
  _sizeAnimator.finished.connect([this]() { elementAnimationFinished.invoke(this, AnimatedProperty_Size); });

  _colorAnimator.animate(vec4(1.0f), 0.0f);
  _colorAnimator.setTag(AnimatedProperty_Color);
  _colorAnimator.updated.connect([this]() { invalidateContent(); });
  _colorAnimator.finished.connect([this]() { elementAnimationFinished.invoke(this, AnimatedProperty_Color); });

  _scaleAnimator.setTag(AnimatedProperty_Scale);
  _scaleAnimator.updated.connect(this, &Element2d::invalidateTransform);
  _scaleAnimator.updated.connect(this, &Element2d::invalidateContent);
  _scaleAnimator.finished.connect([this]() { elementAnimationFinished.invoke(this, AnimatedProperty_Scale); });

  _angleAnimator.setTag(AnimatedProperty_Angle);
  _angleAnimator.updated.connect(this, &Element2d::invalidateTransform);
  _angleAnimator.updated.connect(this, &Element2d::invalidateContent);
  _angleAnimator.finished.connect([this]() { elementAnimationFinished.invoke(this, AnimatedProperty_Angle); });

  _positionAnimator.setTag(AnimatedProperty_Position);
  _positionAnimator.updated.connect(this, &Element2d::invalidateTransform);
  _positionAnimator.updated.connect(this, &Element2d::invalidateContent);
  _positionAnimator.finished.connect([this]() { elementAnimationFinished.invoke(this, AnimatedProperty_Position); });
}

void Element2d::setAngle(float anAngle, float duration) {
  _angleAnimator.cancelUpdates();
  _desiredLayout.angle = anAngle;

  if (duration <= std::numeric_limits<float>::epsilon()) {
    _layout.angle = _desiredLayout.angle;
    invalidateTransform();
  } else {
    _angleAnimator.animate(&_layout.angle, _layout.angle, _desiredLayout.angle, duration);
  }
}

void Element2d::setPivotPoint(const vec2& p, bool preservePosition) {
  _layout.pivotPoint = p;
  _desiredLayout.pivotPoint = p;

  if (preservePosition) setPosition(_layout.position - offset());
}

void Element2d::setScale(const vec2& aScale, float duration) {
  _scaleAnimator.cancelUpdates();
  _desiredLayout.scale = aScale;

  if (duration <= std::numeric_limits<float>::epsilon()) {
    _layout.scale = _desiredLayout.scale;
    invalidateTransform();
  } else {
    _scaleAnimator.animate(&_layout.scale, _layout.scale, _desiredLayout.scale, duration);
  }
}

void Element2d::setColor(const vec4& aColor, float duration) {
  _colorAnimator.animate(aColor, duration);
}

void Element2d::setAlpha(float alpha, float duration) {
  setColor(vec4(_colorAnimator.value().xyz(), alpha), duration);
}

void Element2d::setPosition(const vec2& p, float duration) {
  willChangeFrame();

  _positionAnimator.cancelUpdates();
  _desiredLayout.position = p;

  if (duration <= std::numeric_limits<float>::epsilon()) {
    _layout.position = _desiredLayout.position;
    invalidateTransform();
    invalidateContent();
  } else {
    _positionAnimator.animate(&_layout.position, _layout.position, _desiredLayout.position, duration);
  }

  didChangeFrame();
}

void Element2d::setSize(const vec2& s, float duration) {
  willChangeFrame();

  _sizeAnimator.cancelUpdates();
  _desiredLayout.size = s;

  if (duration <= std::numeric_limits<float>::epsilon()) {
    _layout.size = _desiredLayout.size;
    invalidateTransform();
    invalidateContent();
  } else {
    _sizeAnimator.animate(&_layout.size, _layout.size, _desiredLayout.size, duration);
  }

  didChangeFrame();
}

void Element2d::setVisible(bool vis, float duration) {
  setAlpha(vis ? 1.0f : 0.0f, duration);
}

const mat4& Element2d::transform() {
  if (!transformValid()) buildFinalTransform();

  return _transform;
}

const mat4& Element2d::finalTransform() {
  if (!transformValid()) buildFinalTransform();

  return _finalTransform;
}

mat4 Element2d::buildFinalTransform(const vec2& aOffset, float aAngle, const vec2& aScale, const vec2& aPosition) {
  return translationMatrix(vec3(aOffset, 0.0f)) * transform2DMatrix(aAngle, aScale, aPosition);
}

void Element2d::buildFinalTransform() {
  _transform = buildFinalTransform(offset(), _layout.angle, _layout.scale, _layout.position);
  _finalTransform = _transform * parentFinalTransform();
  setTransformValid(true);
}

const mat4& Element2d::finalInverseTransform() {
  if (!inverseTransformValid()) {
    _finalInverseTransform = finalTransform().inverted();
    setInverseTransformValid(true);
  }

  return _finalInverseTransform;
}

const vec4& Element2d::ownColor() const {
  return _colorAnimator.value();
}

vec4 Element2d::finalColor() {
  return vec4(_colorAnimator.value().xyz(), finalAlpha());
}

const vec2& Element2d::size() const {
  return _layout.size;
}

const vec2& Element2d::position() const {
  return _layout.position;
}

const vec2& Element2d::desiredSize() const {
  return _desiredLayout.size;
}

const vec2& Element2d::desiredPosition() const {
  return _desiredLayout.position;
}

const vec2& Element2d::desiredScale() const {
  return _desiredLayout.scale;
}

const vec2& Element2d::pivotPoint() const {
  return _layout.pivotPoint;
}

const vec2& Element2d::scale() const {
  return _layout.scale;
}

float Element2d::angle() const {
  return _layout.angle;
}

rect Element2d::frame() const {
  return rect(origin(), size());
}

void Element2d::setPosition(float x, float y, float duration) {
  return setPosition(vec2(x, y), duration);
}

void Element2d::setSize(float w, float h, float duration) {
  return setSize(vec2(w, h), duration);
}

bool Element2d::visible() {
  return (_colorAnimator.value().w > 0.0f) && (finalAlpha() > 0.0f);
}

void Element2d::rotate(float anAngle, float duration) {
  return setAngle(_layout.angle + anAngle, duration);
}

bool Element2d::containsPoint(const vec2& p, const vec2&) {
  return containLocalPoint(finalInverseTransform() * p);
}

bool Element2d::containLocalPoint(const vec2& p) {
  return (p.x >= 0.0f) && (p.y >= 0.0f) && (p.x < _layout.size.x) && (p.y < _layout.size.y);
}

vec2 Element2d::offset() const {
  return -_layout.size * _layout.pivotPoint;
}

vec2 Element2d::origin() const {
  return _layout.position + offset();
}

vec2 Element2d::positionInElement(const vec2& p) {
  return finalInverseTransform() * p;
}

vec2 Element2d::contentSize() {
  return _layout.size;
}

/*
 * From Element
 */
void Element2d::setParent(Element2d* element) {
  ElementHierarchy::setParent(element);
  invalidateContent();
  invalidateTransform();
}

void Element2d::removeAllChildren() {
  removeChildren();
}

void Element2d::childRemoved(Element2d*) {
  invalidateContent();
  invalidateTransform();
}

void Element2d::invalidateContent() {
  _contentValid = false;
  _finalAlphaValid = false;

  for (auto& c : children()) c->invalidateContent();

  setInvalid();
}

void Element2d::invalidateTransform() {
  setTransformValid(false);
  setInverseTransformValid(false);

  for (auto& c : children()) c->invalidateTransform();

  setInvalid();
}

void Element2d::bringToFront(Element2d* c) {
  ElementHierarchy::bringToFront(c);
  invalidateContent();
}

void Element2d::sendToBack(Element2d* c) {
  ElementHierarchy::sendToBack(c);
  invalidateContent();
}

void Element2d::broadcastMessage(const Message& msg) {
  for (auto& c : children()) {
    c->processMessage(msg);
    c->broadcastMessage(msg);
  }
}

Element2d* Element2d::childWithNameCallback(const std::string& name, Element2d* root, bool recursive) {
  if (root->name() == name) return root;

  if (recursive) {
    for (auto& c : root->children()) {
      Element2d* aElement = childWithNameCallback(name, c.pointer(), recursive);
      if (aElement != nullptr) return aElement;
    }
  } else {
    for (auto& c : root->children()) {
      if (c->name() == name) return c.pointer();
    }
  }

  return nullptr;
}

void Element2d::setAutolayout(const vec2& pos, LayoutMode pMode, const vec2& sz, LayoutMode sMode, const vec2& pivot) {
  _autoLayout.position = pos;
  _autoLayout.size = sz;
  _autoLayout.pivotPoint = pivot;
  _autoLayout.layoutPositionMode = pMode;
  _autoLayout.layoutSizeMode = sMode;
}

void Element2d::setAutolayoutPosition(const vec2& pos) {
  _autoLayout.position = pos;
}

void Element2d::setAutolayoutSize(const vec2& sz) {
  _autoLayout.size = sz;
}

void Element2d::autoLayout(const vec2& contextSize, float duration) {
  willAutoLayout(duration);

  if (_autoLayout.layoutMask & LayoutMask_Size) {
    vec2 aSize = _autoLayout.size;

    switch (_autoLayout.layoutSizeMode) {
      case LayoutMode_RelativeToContext:
        aSize = contextSize * _autoLayout.size;
        break;

      case LayoutMode_RelativeToParent:
        aSize = _autoLayout.size * ((parent() == nullptr) ? contextSize : parent()->desiredSize());
        break;

      case LayoutMode_WrapContent:
        aSize = contentSize();
        break;

      default:
        break;
    }

    setSize(aSize, duration);
  }

  if (_autoLayout.layoutMask & LayoutMask_Position) {
    vec2 aPos = _autoLayout.position;

    switch (_autoLayout.layoutPositionMode) {
      case LayoutMode_RelativeToContext:
        aPos = contextSize * _autoLayout.position;
        break;

      case LayoutMode_RelativeToParent:
        aPos = _autoLayout.position * ((parent() == nullptr) ? contextSize : parent()->desiredSize());
        break;

      case LayoutMode_WrapContent:
        abort();
        break;

      default:
        break;
    }

    setPosition(aPos, duration);
  }

  if (_autoLayout.layoutMask & LayoutMask_Pivot) setPivotPoint(_autoLayout.pivotPoint, false);

  if (_autoLayout.layoutMask & LayoutMask_Angle) setAngle(_autoLayout.angle, duration);

  if (_autoLayout.layoutMask & LayoutMask_Scale) setScale(_autoLayout.scale, duration);

  if (!hasFlag(Flag_HandlesChildLayout)) {
    for (auto aChild : children()) aChild->autoLayout(contextSize, duration);
  }

  didAutoLayout(duration);
}

void Element2d::setLocationInParent(Location l, const vec2& offset) {
  vec2 actualOffset(0.5f * static_cast<float>(l % 3), 0.5f * static_cast<float>(l / 3));
  setAutolayoutRelativeToParent(actualOffset + offset, vec2(0.0f), actualOffset);
  setAutolayoutMask(LayoutMask_NoSize);
}

void Element2d::setAutolayoutRelativeToParent(const vec2& pos, const vec2& sz, const vec2& pivot) {
  setAutolayout(pos, LayoutMode_RelativeToParent, sz, LayoutMode_RelativeToParent, pivot);
}

void Element2d::setAutolayoutSizeMode(LayoutMode mode) {
  _autoLayout.layoutSizeMode = mode;
}

void Element2d::setAutolayoutPositionMode(LayoutMode mode) {
  _autoLayout.layoutPositionMode = mode;
}

Dictionary Element2d::autoLayoutDictionary() const {
  Dictionary result;

  result.setStringForKey("name", name());

  result.setArrayForKey("position", vec2ToArray(_autoLayout.position));
  result.setArrayForKey("size", vec2ToArray(_autoLayout.size));
  result.setArrayForKey("scale", vec2ToArray(_autoLayout.scale));
  result.setArrayForKey("pivotPoint", vec2ToArray(_autoLayout.pivotPoint));
  result.setFloatForKey("angle", _autoLayout.angle);

  result.setIntegerForKey("autolayout_position", (_autoLayout.layoutMask & LayoutMask_Position) ? 1 : 0);
  result.setIntegerForKey("autolayout_size", (_autoLayout.layoutMask & LayoutMask_Size) ? 1 : 0);
  result.setIntegerForKey("autolayout_pivot", (_autoLayout.layoutMask & LayoutMask_Pivot) ? 1 : 0);
  result.setIntegerForKey("autolayout_angle", (_autoLayout.layoutMask & LayoutMask_Angle) ? 1 : 0);
  result.setIntegerForKey("autolayout_scale", (_autoLayout.layoutMask & LayoutMask_Scale) ? 1 : 0);
  result.setIntegerForKey("autolayout_children", (_autoLayout.layoutMask & LayoutMask_Children) ? 1 : 0);

  result.setStringForKey("positionMode", layoutModeToString(_autoLayout.layoutPositionMode));
  result.setStringForKey("sizeMode", layoutModeToString(_autoLayout.layoutSizeMode));

  storeProperties(result);

  if ((_autoLayout.layoutMask & LayoutMask_Children) && (children().size() > 0)) {
    Dictionary childrenValues;

    for (auto& c : children()) childrenValues.setDictionaryForKey(c->name(), c->autoLayoutDictionary());

    result.setDictionaryForKey("children", childrenValues);
  }

  return result;
}

void Element2d::setAutolayout(const Dictionary& d) {
  ElementLayout l = _autoLayout;

  if (d.hasKey("position")) l.position = arrayToVec2(d.arrayForKey("position"));

  if (d.hasKey("size")) l.size = arrayToVec2(d.arrayForKey("size"));

  if (d.hasKey("scale")) l.scale = arrayToVec2(d.arrayForKey("scale"));

  if (d.hasKey("pivotPoint")) l.pivotPoint = arrayToVec2(d.arrayForKey("pivotPoint"));

  if (d.hasKey("angle")) l.angle = d.floatForKey("angle", 0.0f)->content;

  if (d.hasKey("positionMode")) l.layoutPositionMode = layoutModeFromString(d.stringForKey("positionMode", "parent_relative")->content);

  if (d.hasKey("sizeMode")) l.layoutSizeMode = layoutModeFromString(d.stringForKey("sizeMode", "parent_relative")->content);

  l.layoutMask = (d.integerForKey("autolayout_position", l.layoutMask & LayoutMask_Position)->content ? LayoutMask_Position : 0) | (d.integerForKey("autolayout_size", l.layoutMask & LayoutMask_Size)->content ? LayoutMask_Size : 0) |
                 (d.integerForKey("autolayout_pivot", l.layoutMask & LayoutMask_Pivot)->content ? LayoutMask_Pivot : 0) | (d.integerForKey("autolayout_angle", l.layoutMask & LayoutMask_Angle)->content ? LayoutMask_Angle : 0) |
                 (d.integerForKey("autolayout_scale", l.layoutMask & LayoutMask_Scale)->content ? LayoutMask_Scale : 0) | (d.integerForKey("autolayout_children", l.layoutMask & LayoutMask_Children)->content ? LayoutMask_Children : 0);

  loadProperties(d);

  if (l.layoutMask & LayoutMask_Children) {
    Dictionary dChildren = d.dictionaryForKey("children");
    for (auto p : dChildren->content) {
      Element2d* el = baseChildWithName(p.first, false);
      if (el != nullptr) el->setAutolayout(p.second);
    }
  }

  setAutolayout(l);
}

void Element2d::autoLayoutFromFile(const std::string& fileName) {
  setOrigin(fileName);

  VariantClass c = VariantClass::Invalid;
  VariantBase::Pointer base = json::deserialize(loadTextFile(fileName), c);

  if (base.valid() && (c == VariantClass::Dictionary)) {
    setAutolayout(base);
  } else {
    log::error("Unable to load layout from file %s", fileName.c_str());
  }
}

float Element2d::finalAlpha() {
  if (!_finalAlphaValid) {
    _finalAlpha = ownColor().w * (parent() ? parent()->finalAlpha() : 1.0f);
    _finalAlphaValid = true;
  }

  return _finalAlpha;
}

bool Element2d::enabled() const {
  return _enabled;
}

void Element2d::setEnabled(bool enabled) {
  _enabled = enabled;
}

Element2d* Element2d::baseChildWithName(const std::string& name, bool recursive) {
  return childWithNameCallback(name, this, recursive);
}

void Element2d::setAutolayout(const ElementLayout& al) {
  _autoLayout = al;
}

void Element2d::setAutolayoutMask(size_t m) {
  _autoLayout.layoutMask = m;
}

void Element2d::fillParent() {
  setAutolayoutRelativeToParent(vec2(0.0f), vec2(1.0f), vec2(0.0f));
}

MaterialInstance::Pointer& Element2d::materialInstance() {
  return _material;
}

const MaterialInstance::Pointer& Element2d::materialInstance() const {
  return _material;
}

void Element2d::validateMaterialInstance(SceneRenderer& renderer) {
  if (_material.invalid()) {
    _material = allocateMaterial(renderer);
  }
}

MaterialInstance::Pointer Element2d::allocateMaterial(SceneRenderer& renderer) {
  return renderer.defaultMaterial()->instance();
}

/*
 * Service functions
 */
float alignmentFactor(Alignment a) {
  static const float alignmentValues[static_cast<uint32_t>(Alignment::max)] = {0.0f, 0.5f, 1.0f};
  return alignmentValues[static_cast<uint32_t>(a)];
}

State adjustState(State s) {
  bool isHovered = (s == State_Hovered) || (s == State_SelectedHovered);
  return (!Input::canGetCurrentPointerInfo() && isHovered) ? static_cast<State>(s - 1) : s;
}

static const std::string kLayoutMode_Absolute = "absolute";
static const std::string kLayoutMode_RelativeToContext = "context_relative";
static const std::string kLayoutMode_RelativeToParent = "parent_relative";
static const std::string kLayoutMode_WrapContent = "wrap_content";

#define RETURN_STR_IF(V) \
  case V:                \
    return k##V;
#define RETURN_VAL_IF(V) \
  if (s == k##V) return V;

std::string layoutModeToString(LayoutMode m) {
  switch (m) {
    RETURN_STR_IF(LayoutMode_Absolute)
    RETURN_STR_IF(LayoutMode_RelativeToContext)
    RETURN_STR_IF(LayoutMode_RelativeToParent)
    RETURN_STR_IF(LayoutMode_WrapContent)

    default:
      log::error("Invalid layout mode");
  }

  return kLayoutMode_Absolute;
}

LayoutMode layoutModeFromString(const std::string& s) {
  RETURN_VAL_IF(LayoutMode_Absolute);
  RETURN_VAL_IF(LayoutMode_RelativeToContext);
  RETURN_VAL_IF(LayoutMode_RelativeToParent);
  RETURN_VAL_IF(LayoutMode_WrapContent);

  log::error("Invalid layout mode string %s", s.c_str());
  return LayoutMode_Absolute;
}

}  // namespace s2d
}  // namespace et
