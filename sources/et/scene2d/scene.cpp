/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/app/application.hpp>
#include <et/scene2d/scene.hpp>

namespace et {
namespace s2d {

Scene::Scene(RenderInterface::Pointer& rc, const RenderPass::ConstructionInfo& passInfo)
  : _rc(rc)
  , _renderer(rc, passInfo)
  , _renderingElementBackground(sharedObjectFactory().createObject<RenderingElement>(rc, 256))
  , _renderingElementOverlay(sharedObjectFactory().createObject<RenderingElement>(rc, 256))
  , _background(Image(), nullptr)
  , _overlay(Image(), nullptr) {
  _background.setPivotPoint(vec2(0.5f));
  _background.setContentMode(ImageView::ContentMode_Fill);

  _overlay.setPivotPoint(vec2(0.5f));
  _overlay.setContentMode(ImageView::ContentMode_Fill);

  layout(vector2ToFloat(rc->contextSize()));
}

bool Scene::pointerPressed(PointerInputInfo p) {
  if (animatingTransition()) return true;

  for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i) {
    if ((*i)->layout->pointerPressed(p)) return true;
  }

  return false;
}

bool Scene::pointerMoved(PointerInputInfo p) {
  if (animatingTransition()) return true;

  for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i) {
    if ((*i)->layout->pointerMoved(p)) return true;
  }

  return false;
}

bool Scene::pointerReleased(PointerInputInfo p) {
  if (animatingTransition()) return true;

  for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i) {
    if ((*i)->layout->pointerReleased(p)) return true;
  }

  return false;
}

bool Scene::pointerCancelled(PointerInputInfo p) {
  for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i) (*i)->layout->pointerCancelled(p);

  return false;
}

bool Scene::pointerScrolled(PointerInputInfo p) {
  if (animatingTransition()) return true;

  for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i) {
    if ((*i)->layout->pointerScrolled(p)) return true;
  }

  return false;
}

bool Scene::keyPressed(size_t key) {
  if (_keyboardFocusedElement.valid()) {
    return _keyboardFocusedElement->processMessage(Message(Message::Type_TextFieldControl, key));
  }

  if ((key == ET_KEY_RETURN) || (key == ET_KEY_ESCAPE)) {
    Message msg(Message::Type_PerformAction);
    msg.param = (key == ET_KEY_RETURN) ? Action_Confirm : Action_Cancel;
    for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i) {
      auto responder = (*i)->layout->findFirstResponder(msg);
      if (responder.valid()) {
        return responder->processMessage(msg);
      }
    }
  } else {
    for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i) {
      if ((*i)->layout->keyPressed(key)) return true;
    }
  }

  return false;
}

bool Scene::charactersEntered(std::string p) {
  if (_keyboardFocusedElement.valid()) {
    return _keyboardFocusedElement->processMessage(Message(Message::Type_TextInput, p));
  }

  return false;
}

void Scene::buildLayoutVertices(RenderInterface::Pointer& rc, RenderingElement::Pointer element, Layout::Pointer layout) {
  ET_ASSERT(element.valid());

  _renderer.setRenderingElement(element);

  if (!layout->valid()) {
    element->startAllocatingVertices();
    layout->addToRenderQueue(rc, _renderer);
    element->commitAllocatedVertices();
  }
}

void Scene::buildBackgroundVertices(RenderInterface::Pointer& rc) {
  _renderer.setRenderingElement(_renderingElementBackground);

  if (!_background.contentValid()) {
    _renderingElementBackground->startAllocatingVertices();
    _background.addToRenderQueue(rc, _renderer);
    _renderingElementBackground->commitAllocatedVertices();
  }
}

void Scene::buildOverlayVertices(RenderInterface::Pointer& rc) {
  _renderer.setRenderingElement(_renderingElementOverlay);

  if (!_overlay.contentValid()) {
    _renderingElementOverlay->startAllocatingVertices();
    _overlay.addToRenderQueue(rc, _renderer);
    _renderingElementOverlay->commitAllocatedVertices();
  }
}

void Scene::layout(const vec2& size, float duration) {
  _screenSize = size;
  _renderer.setProjectionMatrices(size);

  _background.setPosition(0.5f * size);
  _background.setSize(size);

  _overlay.setPosition(0.5f * size);
  _overlay.setSize(size);

  for (auto& i : _layouts) i->layout->autoLayout(size, duration);
}

void Scene::render(RenderInterface::Pointer& rc) {
  _prerenderElements.clear();

  for (auto& obj : _layouts) obj->layout->collectPreRenderingObjects(obj->layout, _prerenderElements);

  if (!_prerenderElements.empty()) {
    /*
     * TODO : render
     */
    // auto currentBuffer = rc.renderState().boundFramebuffer();
    // auto viewportSize = rc.renderState().viewportSize();

    for (auto e : _prerenderElements) e->preRender(rc);

    // rc.renderState().bindFramebuffer(currentBuffer);
    // rc.renderState().setViewportSize(viewportSize);
  }

  _renderer.beginRender(rc);

  if (_background.texture().valid()) {
    _renderer.setAdditionalOffsetAndAlpha(vec3(0.0f, 0.0f, 1.0f));
    buildBackgroundVertices(rc);
    _renderer.render(rc);
  }

  for (auto& obj : _layouts) {
    if (obj->layout->visible()) {
      _renderer.setAdditionalOffsetAndAlpha(obj->offsetAlpha);
      buildLayoutVertices(rc, obj->layout->renderingElement(), obj->layout);
      _renderer.render(rc);
    }
  }

  if (_overlay.texture().valid()) {
    _renderer.setAdditionalOffsetAndAlpha(vec3(0.0f, 0.0f, 1.0f));
    buildOverlayVertices(rc);
    _renderer.render(rc);
  }

  _renderer.endRender(rc);
}

ImageView& Scene::backgroundImageView() {
  return _background;
}

const ImageView& Scene::backgroundImageView() const {
  return _background;
}

void Scene::setBackgroundImage(const Image& img) {
  _background.setImage(img);
}

ImageView& Scene::overlayImageView() {
  return _overlay;
}

const ImageView& Scene::overlayImageView() const {
  return _overlay;
}

void Scene::setOverlayImage(const Image& img) {
  _overlay.setImage(img);
}

void Scene::onKeyboardNeeded(Layout*, Element2d* element) {
  _keyboardFocusedElement = Element2d::Pointer(element);
  input().activateSoftwareKeyboard();
}

void Scene::onKeyboardResigned(Layout*) {
  input().deactivateSoftwareKeyboard();
  _keyboardFocusedElement.reset(nullptr);
}

void Scene::getAnimationParams(size_t flags, vec3* nextSrc, vec3* nextDst, vec3* currDst) {
  float fromLeft = (flags & AnimationFlag_FromLeft) ? 1.0f : 0.0f;
  float fromRight = (flags & AnimationFlag_FromRight) ? 1.0f : 0.0f;
  float fromTop = (flags & AnimationFlag_FromTop) ? 1.0f : 0.0f;
  float fromBottom = (flags & AnimationFlag_FromBottom) ? 1.0f : 0.0f;
  float fade = (flags & AnimationFlag_Fade) ? 1.0f : 0.0f;

  if (nextSrc) *nextSrc = vec3(-1.0f * fromLeft + 1.0f * fromRight, 1.0f * fromTop - 1.0f * fromBottom, 1.0f - fade);

  if (nextDst) *nextDst = vec3(0.0, 0.0, 1.0f);

  if (currDst) *currDst = vec3(1.0f * fromLeft - 1.0f * fromRight, -1.0f * fromTop + 1.0f * fromBottom, 1.0f - fade);
}

void Scene::internal_replaceTopmostLayout(Layout::Pointer newLayout, AnimationDescriptor desc) {
  internal_removeLayout(topmostLayout(), desc);
  internal_pushLayout(newLayout, desc);
}

void Scene::internal_replaceLayout(LayoutPair l, AnimationDescriptor desc) {
  auto i = std::find_if(_layouts.begin(), _layouts.end(), [l](const LayoutEntry::Pointer& entry) { return entry->layout == l.oldLayout; });

  if (i == _layouts.end()) {
    internal_pushLayout(l.newLayout, desc);
  } else {
    LayoutEntry::Pointer layoutToShow(entryForLayout(l.newLayout));

    if (layoutToShow.invalid()) {
      layoutToShow = LayoutEntry::Pointer::create(this, _rc, l.newLayout);
    } else {
      _layouts.erase(std::find_if(_layouts.begin(), _layouts.end(), [layoutToShow](const LayoutEntry::Pointer& lp) { return (lp.pointer() == layoutToShow.pointer()); }));
    }

    _layouts.insert(i, layoutToShow);
    validateTopLevelLayout();

    animateLayoutAppearing(l.newLayout, layoutToShow.pointer(), desc.flags, desc.duration);
  }

  internal_removeLayout(l.oldLayout, desc);
}

void Scene::internal_removeLayout(Layout::Pointer oldLayout, AnimationDescriptor desc) {
  LayoutEntry* entry = entryForLayout(oldLayout);
  if (entry == nullptr) return;

  layoutWillDisappear.invoke(oldLayout);

  if (oldLayout->hasFlag(Flag_RequiresKeyboard)) onKeyboardResigned(oldLayout.pointer());

  oldLayout->cancelInteractions();
  oldLayout->willDisappear();

  oldLayout->layoutDoesntNeedKeyboard.disconnect(this);
  oldLayout->layoutRequiresKeyboard.disconnect(this);

  if ((desc.flags == AnimationFlag_None) || (std::abs(desc.duration) < std::numeric_limits<float>::epsilon())) {
    oldLayout->didDisappear();
    removeLayoutEntryFromList(entry);
  } else {
    vec3 destOffsetAlpha;
    getAnimationParams(desc.flags, 0, 0, &destOffsetAlpha);
    entry->animateTo(destOffsetAlpha, std::abs(desc.duration), Scene::LayoutEntry::State_Disappear);
  }
}

void Scene::internal_pushLayout(Layout::Pointer newLayout, AnimationDescriptor desc) {
  if (newLayout.invalid()) return;

  if (hasLayout(newLayout)) internal_removeLayout(newLayout, AnimationDescriptor());

  _layouts.push_back(LayoutEntry::Pointer::create(this, _rc, newLayout));
  validateTopLevelLayout();

  animateLayoutAppearing(newLayout, _layouts.back().pointer(), desc.flags, desc.duration);
}

void Scene::animateLayoutAppearing(Layout::Pointer newLayout, LayoutEntry* newEntry, size_t animationFlags, float duration) {
  newLayout->autoLayout(_screenSize, 0.0f);

  layoutWillAppear.invoke(newLayout);
  newLayout->willAppear();

  ET_CONNECT_EVENT(newLayout->layoutRequiresKeyboard, Scene::onKeyboardNeeded);
  ET_CONNECT_EVENT(newLayout->layoutDoesntNeedKeyboard, Scene::onKeyboardResigned);

  bool smallDuration = std::abs(duration) < std::numeric_limits<float>::epsilon();
  if ((animationFlags == AnimationFlag_None) || smallDuration) {
    newLayout->didAppear();

    if (newLayout->hasFlag(Flag_RequiresKeyboard)) onKeyboardNeeded(newLayout.pointer(), nullptr);

    layoutDidAppear.invoke(newLayout);
  } else {
    vec3 destOffsetAlpha;
    getAnimationParams(animationFlags, &newEntry->offsetAlpha, &destOffsetAlpha, 0);
    newEntry->animateTo(destOffsetAlpha, std::abs(duration), Scene::LayoutEntry::State_Appear);
  }
}

void Scene::removeLayoutEntryFromList(LayoutEntry* ptr) {
  for (auto i = _layouts.begin(), e = _layouts.end(); i != e; ++i) {
    if (i->pointer() == ptr) {
      _layouts.erase(i);
      validateTopLevelLayout();
      return;
    }
  }
}

void Scene::layoutEntryTransitionFinished(LayoutEntry* l) {
  if (l->state == LayoutEntry::State_Disappear) {
    if (l->layout.valid()) {
      layoutDidDisappear.invoke(l->layout);
      l->layout->didDisappear();
    }
    removeLayoutEntryFromList(l);
  } else {
    layoutDidAppear.invoke(l->layout);
    l->layout->didAppear();

    if (l->layout->hasFlag(Flag_RequiresKeyboard)) onKeyboardNeeded(l->layout.pointer(), nullptr);

    l->state = Scene::LayoutEntry::State_Still;
  }
}

bool Scene::hasLayout(Layout::Pointer aLayout) {
  if (aLayout.invalid()) return false;

  for (auto& i : _layouts) {
    if (i->layout == aLayout) return true;
  }

  return false;
}

Scene::LayoutEntry* Scene::entryForLayout(Layout::Pointer ptr) {
  if (ptr.invalid()) return nullptr;

  for (auto& i : _layouts) {
    if (i->layout == ptr) return i.pointer();
  }

  return nullptr;
}

bool Scene::animatingTransition() {
  for (auto& i : _layouts) {
    if (i.pointer()->state != Scene::LayoutEntry::State_Still) return true;
  }

  return false;
}

void Scene::replaceTopmostLayout(Layout::Pointer newLayout, size_t animationFlags, float duration) {
  Invocation2 inv;
  inv.setTarget(this, &Scene::internal_replaceTopmostLayout, newLayout, AnimationDescriptor(animationFlags, duration));
  inv.invokeInCurrentRunLoop();
}

void Scene::popTopmostLayout(size_t animationFlags, float duration) {
  Invocation2 inv;
  inv.setTarget(this, &Scene::internal_removeLayout, topmostLayout(), AnimationDescriptor(animationFlags, duration));
  inv.invokeInCurrentRunLoop();
}

void Scene::replaceLayout(Layout::Pointer oldLayout, Layout::Pointer newLayout, size_t animationFlags, float duration) {
  Invocation2 inv;
  inv.setTarget(this, &Scene::internal_replaceLayout, LayoutPair(oldLayout, newLayout), AnimationDescriptor(animationFlags, duration));
  inv.invokeInCurrentRunLoop();
}

void Scene::removeLayout(Layout::Pointer oldLayout, size_t animationFlags, float duration) {
  Invocation2 inv;
  inv.setTarget(this, &Scene::internal_removeLayout, oldLayout, AnimationDescriptor(animationFlags, duration));
  inv.invokeInCurrentRunLoop();
}

void Scene::pushLayout(Layout::Pointer newLayout, size_t animationFlags, float duration) {
  Invocation2 inv;
  inv.setTarget(this, &Scene::internal_pushLayout, newLayout, AnimationDescriptor(animationFlags, duration));
  inv.invokeInCurrentRunLoop();
}

void Scene::reloadObject(LoadableObject::Pointer obj, ObjectsCache&) {
  Element2d::Pointer l = obj;
  l->reloadFromFile(obj->origin());
  l->autoLayoutFromFile(obj->origin());
  l->autoLayout(vector2ToFloat(_rc->contextSize()), 0.0f);
}

void Scene::broadcastMessage(const Message& msg) {
  for (auto l : _layouts) l->layout->broadcastMessage(msg);
}

void Scene::removeAllLayouts() {
  for (auto l : _layouts) l->layout.reset(nullptr);

  _layouts.clear();
}

void Scene::setTopLevelLayout(Layout::Pointer l) {
  _topLayout = l;
  validateTopLevelLayout();
}

void Scene::validateTopLevelLayout() {
  if (_layouts.size() > 1) {
    auto topLevel = std::find_if(_layouts.begin(), _layouts.end(), [this](const LayoutEntry::Pointer& le) { return (le->layout == _topLayout); });

    if (topLevel != _layouts.end()) _layouts.splice(_layouts.end(), _layouts, topLevel);
  }
}

/*
 * Layout Entry
 */

Scene::LayoutEntry::LayoutEntry(Scene* own, RenderInterface::Pointer& rc, Layout::Pointer l)
  : layout(l)
  , animator(l->timerPool())
  , offsetAlpha(0.0f, 0.0f, 1.0f)
  , state(Scene::LayoutEntry::State_Still) {
  animator.finished.connect([this, own]() {
    Invocation1 i;
    i.setTarget(own, &Scene::layoutEntryTransitionFinished, this);
    i.invokeInMainRunLoop();
  });
  l->initRenderingElement(rc);
}

Scene::LayoutEntry::~LayoutEntry() {
  layout->renderingElement()->clear();
}

void Scene::LayoutEntry::animateTo(const vec3& oa, float duration, State s) {
  state = s;
  animator.setTimeInterpolationFunction(layout->positionInterpolationFunction());
  animator.animate(&offsetAlpha, offsetAlpha, oa, duration);
}

}  // namespace s2d
}  // namespace et
