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

Scene::Scene(RenderContext* rc) : _rc(rc), _renderer(rc),
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

void Scene::keyPressed(size_t key)
{
	if (_keyboardFocusedLayout.valid() && _keyboardFocusedElement.valid())
		_keyboardFocusedElement->processMessage(Message(Message::Type_TextFieldControl, key));
}

void Scene::charactersEntered(std::string p)
{
	if (_keyboardFocusedLayout.valid() && _keyboardFocusedElement.valid())
		_keyboardFocusedElement->processMessage(Message(Message::Type_TextInput, p));
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
	_renderer.setRendernigElement(_renderingElementBackground);
	
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
	_background.setPosition(0.5f * size);
	_background.setSize(size);
	_backgroundValid = false;

	for (auto& i : _layouts)
		i->layout->autoLayout(size, 0.0f);
}

void Scene::render(RenderContext* rc)
{
	_renderer.beginRender(rc);

	if (_background.texture().valid())
	{
		_renderer.setAdditionalOffsetAndAlpha(vec3(0.0f, 0.0f, 1.0f));
		buildBackgroundVertices(rc);
		_renderer.render(rc);
	}

	for (auto& obj : _layouts)
	{
		_renderer.setAdditionalOffsetAndAlpha(obj->offsetAlpha);
		buildLayoutVertices(rc, obj->layout->renderingElement(), obj->layout);
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
	float fromLeft = (flags & AnimationFlag_FromLeft) ? 1.0f : 0.0f;
	float fromRight = (flags & AnimationFlag_FromRight) ? 1.0f : 0.0f;
	float fromTop = (flags & AnimationFlag_FromTop) ? 1.0f : 0.0f;
	float fromBottom = (flags & AnimationFlag_FromBottom) ? 1.0f : 0.0f;
	float fade = (flags & AnimationFlag_Fade) ? 1.0f : 0.0f;

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
		LayoutEntry::Pointer newEntry(new LayoutEntry(this, _rc, l.newLayout));
		_layouts.insert(i, newEntry);

		animateLayoutAppearing(l.newLayout, newEntry.ptr(), desc.flags, desc.duration);
	}
}

void Scene::internal_removeLayout(Layout::Pointer oldLayout, AnimationDescriptor desc)
{
	LayoutEntry* entry = entryForLayout(oldLayout);
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
		entry->animateTo(destOffsetAlpha, std::abs(desc.duration), Scene::LayoutEntry::State_Disappear);
	}
}

void Scene::internal_pushLayout(Layout::Pointer newLayout, AnimationDescriptor desc)
{
	if (newLayout.invalid()) return;

	if (hasLayout(newLayout))
		internal_removeLayout(newLayout, AnimationDescriptor());

	_layouts.push_back(LayoutEntry::Pointer::create(this, _rc, newLayout));
	animateLayoutAppearing(newLayout, _layouts.back().ptr(), desc.flags, desc.duration);
}

void Scene::animateLayoutAppearing(Layout::Pointer newLayout, LayoutEntry* newEntry,
	size_t animationFlags, float duration)
{
	newLayout->autoLayout(_screenSize, 0.0f);

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
		newEntry->animateTo(destOffsetAlpha, std::abs(duration), Scene::LayoutEntry::State_Appear);
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

void Scene::removeLayoutEntryFromList(LayoutEntry* ptr)
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

void Scene::layoutEntryTransitionFinished(LayoutEntry* l)
{
	if (l->state == LayoutEntry::State_Disappear)
	{
		layoutDidDisappear.invoke(l->layout);
		l->layout->didDisappear();

		removeLayoutEntryFromList(l);
	}
	else 
	{
		layoutDidAppear.invoke(l->layout);
		l->layout->didAppear();

		l->state = Scene::LayoutEntry::State_Still;
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

Scene::LayoutEntry* Scene::entryForLayout(Layout::Pointer ptr)
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
		if (i.ptr()->state != Scene::LayoutEntry::State_Still)
			return true;
	}
	
	return false;
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

void Scene::reloadObject(LoadableObject::Pointer obj, ObjectsCache&)
{
	Element::Pointer l = obj;
	l->autolayoutFromFile(obj->origin());
	l->autoLayout(_rc->size(), 0.0f);
}

/*
 * Layout Entry
 */

Scene::LayoutEntry::LayoutEntry(Scene* own, RenderContext* rc, Layout::Pointer l) :  owner(own),
	layout(l), animator(own->timerPoolForLayout(l)), offsetAlpha(0.0f, 0.0f, 1.0f),
	state(Scene::LayoutEntry::State_Still)
{
	animator.setDelegate(this);
	l->initRenderingElement(rc);
}

void Scene::LayoutEntry::animateTo(const vec3& oa, float duration, State s)
{
	state = s;
	animator.animate(&offsetAlpha, offsetAlpha, oa, duration);
}

void Scene::LayoutEntry::animatorFinished(BaseAnimator*)
{
	owner->layoutEntryTransitionFinished(this);
}
