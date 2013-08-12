/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et-ext/scene2d/scene.h>

using namespace et;
using namespace et::s2d;

#if (ET_PLATFORM_IOS || ET_PLATFORM_ANDROID)
static const bool shouldSaveFillRate = true;
#else
static const bool shouldSaveFillRate = false;
#endif

Scene::Scene(RenderContext* rc) : _rc(rc),  _renderer(rc, shouldSaveFillRate),
	_renderingElementBackground(new RenderingElement(rc)),
	_background(Texture(), 0), _backgroundValid(true)
{
	_background.setPivotPoint(vec2(0.5f));
	_background.setContentMode(ImageView::ContentMode_Fill);
	layout(rc->size());
}

bool Scene::pointerPressed(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerPressed(p))
			return true;
	}
	return false;
}

bool Scene::pointerMoved(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerMoved(p))
			return true;
	}

	return false;
}

bool Scene::pointerReleased(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerReleased(p))
			return true;
	}

	return false;
}

bool Scene::pointerCancelled(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;
	
	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerCancelled(p))
			return true;
	}
	
	return false;
}

bool Scene::pointerScrolled(const et::PointerInputInfo& p)
{
	if (animatingTransition()) return true;

	for (auto i = _layouts.rbegin(), e = _layouts.rend(); i != e; ++i)
	{
		if ((*i)->layout->pointerScrolled(p))
			return true;
	}

	return false;
}

bool Scene::characterEntered(size_t p)
{
	if (_keyboardFocusedLayout.invalid() && _keyboardFocusedElement.invalid()) return false;
	
	_keyboardFocusedElement->processMessage(GuiMessage(GuiMessage::Type_TextInput, p));
	return true;
}

void Scene::buildLayoutVertices(RenderContext* rc, RenderingElement::Pointer element, Layout::Pointer layout)
{
	_renderer.setRendernigElement(element);

	if (!layout->valid())
	{
		element->clear();
		layout->addToRenderQueue(rc, _renderer);
	}
}

void Scene::buildBackgroundVertices(RenderContext* rc)
{
	if (!_backgroundValid)
	{
		_renderingElementBackground->clear();
		_background.addToRenderQueue(rc, _renderer);
	}
	_backgroundValid = true;
}

void Scene::layout(const vec2& size)
{
	_screenSize = size;
	_renderer.setProjectionMatrices(size);
	_background.setFrame(0.5f * size, size);
	_backgroundValid = false;

	for (auto& i : _layouts)
		i->layout->layout(size);
}

void Scene::render(RenderContext* rc)
{
	_renderer.beginRender(rc);

	if (_background.texture().valid())
	{
		_renderer.setRendernigElement(_renderingElementBackground);
		buildBackgroundVertices(rc);
		_renderer.setCustomAlpha(1.0f);
		_renderer.setCustomOffset(vec2(0.0f));
		_renderer.render(rc);
	}

	for (auto& obj : _layouts)
	{
		buildLayoutVertices(rc, obj->layout->renderingElement(), obj->layout);
		_renderer.setCustomAlpha(obj->offsetAlpha.z);
		_renderer.setCustomOffset(obj->offsetAlpha.xy());
		_renderer.render(rc);
	}

	_renderer.endRender(rc);
}

void Scene::setBackgroundImage(const Image& img)
{
	_background.setImage(img);
	_backgroundValid = false;
}

void Scene::onKeyboardNeeded(Layout* l, Element* element)
{
	_keyboardFocusedElement = Element::Pointer(element);
	_keyboardFocusedLayout = Layout::Pointer(l);
	input().activateSoftwareKeyboard();
}

void Scene::onKeyboardResigned(Layout*)
{
	input().deactivateSoftwareKeyboard();
	_keyboardFocusedElement.reset(0);
	_keyboardFocusedLayout.reset(0);
}

void Scene::getAnimationParams(size_t flags, vec3* nextSrc, vec3* nextDst, vec3* currDst)
{
	float fromLeft = static_cast<float>((flags & AnimationFlag_FromLeft) == AnimationFlag_FromLeft);
	float fromRight = static_cast<float>((flags & AnimationFlag_FromRight) == AnimationFlag_FromRight);
	float fromTop = static_cast<float>((flags & AnimationFlag_FromTop) == AnimationFlag_FromTop);
	float fromBottom = static_cast<float>((flags & AnimationFlag_FromBottom) == AnimationFlag_FromBottom);
	float fade = static_cast<float>((flags & AnimationFlag_Fade) == AnimationFlag_Fade);

	if (nextSrc)
		*nextSrc = vec3(-1.0f * fromLeft + 1.0f * fromRight, 1.0f * fromTop - 1.0f * fromBottom, 1.0f - fade);

	if (nextDst)
		*nextDst = vec3(0.0, 0.0, 1.0f);

	if (currDst)
		*currDst = vec3(1.0f * fromLeft - 1.0f * fromRight, -1.0f * fromTop + 1.0f * fromBottom, 1.0f - fade);
}

void Scene::internal_replaceTopmostLayout(Layout::Pointer newLayout, AnimationDescriptor desc)
{
	removeLayout(topmostLayout(), desc.flags, desc.duration);
	pushLayout(newLayout, desc.flags, desc.duration);
}

void Scene::internal_replaceLayout(LayoutPair l, AnimationDescriptor desc)
{
	auto i = _layouts.begin();
	while (i != _layouts.end())
	{
		if ((*i)->layout == l.oldLayout)
			break;

		++i;
	}

	removeLayout(l.oldLayout, desc.flags, desc.duration);

	if (i == _layouts.end())
	{
		pushLayout(l.newLayout, desc.flags, desc.duration);
	}
	else 
	{
		LayoutEntry newEntry(new LayoutEntryObject(this, _rc, l.newLayout));
		_layouts.insert(i, newEntry);

		animateLayoutAppearing(l.newLayout, newEntry.ptr(), desc.flags, desc.duration);
	}
}

void Scene::internal_removeLayout(Layout::Pointer oldLayout, AnimationDescriptor desc)
{
	LayoutEntryObject* entry = entryForLayout(oldLayout);
	if (entry == 0) return;

	layoutWillDisappear.invoke(oldLayout);
	oldLayout->willDisappear();
	oldLayout->layoutDoesntNeedKeyboard.disconnect(this);
	oldLayout->layoutRequiresKeyboard.disconnect(this);

	if ((desc.flags == AnimationFlag_None) || (std::abs(desc.duration) < std::numeric_limits<float>::epsilon()))
	{
		oldLayout->didDisappear();
		removeLayoutFromList(oldLayout);
	}
	else 
	{
		vec3 destOffsetAlpha;
		getAnimationParams(desc.flags, 0, 0, &destOffsetAlpha);
		entry->animateTo(destOffsetAlpha, std::abs(desc.duration), Scene::LayoutEntryObject::State_Disappear);
	}
}

void Scene::internal_pushLayout(Layout::Pointer newLayout, AnimationDescriptor desc)
{
	if (newLayout.invalid()) return;

	if (hasLayout(newLayout))
		internal_removeLayout(newLayout, AnimationDescriptor());

	_layouts.push_back(LayoutEntry(new LayoutEntryObject(this, _rc, newLayout)));
	animateLayoutAppearing(newLayout, _layouts.back().ptr(), desc.flags, desc.duration);
}

void Scene::animateLayoutAppearing(Layout::Pointer newLayout, LayoutEntryObject* newEntry,
	size_t animationFlags, float duration)
{
	newLayout->layout(_screenSize);

	layoutWillAppear.invoke(newLayout);
	newLayout->willAppear();

	ET_CONNECT_EVENT(newLayout->layoutRequiresKeyboard, Scene::onKeyboardNeeded)
	ET_CONNECT_EVENT(newLayout->layoutDoesntNeedKeyboard, Scene::onKeyboardResigned)

	bool smallDuration = std::abs(duration) < std::numeric_limits<float>::epsilon();
	if ((animationFlags == AnimationFlag_None) || smallDuration)
	{
		newLayout->didAppear();
		layoutDidAppear.invoke(newLayout);
	}
	else 
	{
		vec3 destOffsetAlpha;
		getAnimationParams(animationFlags, &newEntry->offsetAlpha, &destOffsetAlpha, 0);
		newEntry->animateTo(destOffsetAlpha, std::abs(duration), Scene::LayoutEntryObject::State_Appear);
	}
}

void Scene::removeLayoutFromList(Layout::Pointer ptr)
{
	for (auto i = _layouts.begin(), e = _layouts.end(); i != e; ++i)
	{
		if ((*i)->layout == ptr)
		{
			_layouts.erase(i);
			return;
		}
	}
}

void Scene::removeLayoutEntryFromList(LayoutEntryObject* ptr)
{
	for (auto i = _layouts.begin(), e = _layouts.end(); i != e; ++i)
	{
		if (i->ptr() == ptr)
		{
			_layouts.erase(i);
			return;
		}
	}
}

void Scene::layoutEntryTransitionFinished(LayoutEntryObject* l)
{
	if (l->state == LayoutEntryObject::State_Disappear)
	{
		layoutDidDisappear.invoke(l->layout);
		l->layout->didDisappear();

		removeLayoutEntryFromList(l);
	}
	else 
	{
		layoutDidAppear.invoke(l->layout);
		l->layout->didAppear();

		l->state = Scene::LayoutEntryObject::State_Still;
	}
}

bool Scene::hasLayout(Layout::Pointer aLayout)
{
	if (aLayout.invalid()) return false;

	for (auto& i : _layouts)
	{
		if (i->layout == aLayout)
			return true;
	}

	return false;
}

Scene::LayoutEntryObject* Scene::entryForLayout(Layout::Pointer ptr)
{
	if (ptr.invalid()) return nullptr;

	for (auto& i : _layouts)
	{
		if (i->layout == ptr)
			return i.ptr();
	}

	return nullptr;
}

bool Scene::animatingTransition()
{
	for (auto& i : _layouts)
	{
		if (i.ptr()->state != Scene::LayoutEntryObject::State_Still)
			return true;
	}
	
	return false;
}

/*
 * Layout Entry
 */

Scene::LayoutEntryObject::LayoutEntryObject(Scene* own, RenderContext* rc, Layout::Pointer l) :  owner(own),
	layout(l), animator(0),  offsetAlpha(0.0f, 0.0f, 1.0f), state(Scene::LayoutEntryObject::State_Still)
{
	l->initRenderingElement(rc);
}

Scene::LayoutEntryObject::LayoutEntryObject(Scene::LayoutEntryObject&& l) : 
	owner(l.owner), layout(l.layout), animator(l.animator.extract()), 
	offsetAlpha(l.offsetAlpha), state(Scene::LayoutEntryObject::State_Still)
{
	moveDelegate();
}

Scene::LayoutEntryObject::LayoutEntryObject(Scene::LayoutEntryObject& l) : 
	owner(l.owner), layout(l.layout), animator(l.animator.extract()), 
	offsetAlpha(l.offsetAlpha), state(Scene::LayoutEntryObject::State_Still)
{
	moveDelegate();
}

Scene::LayoutEntryObject& Scene::LayoutEntryObject::operator = (Scene::LayoutEntryObject& l)
{
	owner = l.owner;
	layout = l.layout;
	animator = l.animator.extract();
	offsetAlpha = l.offsetAlpha;
	state = l.state;
	moveDelegate();
	return *this; 
}

void Scene::LayoutEntryObject::moveDelegate()
{
	if (animator.valid())
		animator->setDelegate(0);
}

void Scene::LayoutEntryObject::animateTo(const vec3& oa, float duration, State s)
{
	state = s;

	if (animator.valid())
		animator.extract()->destroy();

	animator = new Vector3Animator(this, &offsetAlpha, offsetAlpha, oa, duration, 0, mainTimerPool());
}

void Scene::LayoutEntryObject::animatorUpdated(BaseAnimator*)
{
}

void Scene::LayoutEntryObject::animatorFinished(BaseAnimator*)
{
	animator.extract()->destroy();
	owner->layoutEntryTransitionFinished(this);
}

void Scene::showMessageView(MessageView::Pointer mv, size_t animationFlags, float duration)
{
	ET_CONNECT_EVENT(mv->messageViewButtonSelected, Scene::onMessageViewButtonClicked)
	pushLayout(mv, animationFlags, duration);
}

void Scene::onMessageViewButtonClicked(MessageView* view, MessageViewButton)
{
	removeLayout(Layout::Pointer(view));
}

void Scene::replaceTopmostLayout(Layout::Pointer newLayout, size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Scene, internal_replaceTopmostLayout, newLayout,
		AnimationDescriptor(animationFlags, duration))
}

void Scene::popTopmostLayout(size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Scene, internal_removeLayout, topmostLayout(),
		AnimationDescriptor(animationFlags, duration))
}

void Scene::replaceLayout(Layout::Pointer oldLayout, Layout::Pointer newLayout,
	size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Scene, internal_replaceLayout, LayoutPair(oldLayout, newLayout),
		AnimationDescriptor(animationFlags, duration))
}

void Scene::removeLayout(Layout::Pointer oldLayout, size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Scene, internal_removeLayout, oldLayout,
		AnimationDescriptor(animationFlags, duration))
}

void Scene::pushLayout(Layout::Pointer newLayout, size_t animationFlags, float duration)
{
	ET_INVOKE_THIS_CLASS_METHOD2(Scene, internal_pushLayout, newLayout,
		AnimationDescriptor(animationFlags, duration))
}

