/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <deque>
#include <et/core/objectscache.h>
#include <et/rendering/rendercontext.h>
#include <et/input/input.h>
#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/layout.h>
#include <et-ext/scene2d/fullscreenelement.h>
#include <et-ext/scene2d/imageview.h>
#include <et-ext/scene2d/label.h>
#include <et-ext/scene2d/button.h>
#include <et-ext/scene2d/listbox.h>
#include <et-ext/scene2d/textfield.h>
#include <et-ext/scene2d/scroll.h>
#include <et-ext/scene2d/slider.h>
#include <et-ext/scene2d/table.h>
#include <et-ext/scene2d/line.h>
#include <et-ext/scene2d/particleselement.h>

namespace et
{
namespace s2d
{
class Scene : public ObjectLoader, public EventReceiver
{
public:
	ET_DECLARE_POINTER(Scene);

public:
	Scene(RenderContext*, const RenderPass::ConstructionInfo&);

	void layout(const vec2& size, float duration = 0.0);
	void render(RenderContext* rc);

	SceneRenderer& renderer()
	{
		return _renderer;
	}

	ObjectsCache& sharedCache()
	{
		return _sharedCache;
	}

	Layout::Pointer topmostLayout() const
	{
		return _layouts.size() ? _layouts.back()->layout : Layout::Pointer();
	}

	bool hasLayout(Layout::Pointer aLayout);

	void replaceTopmostLayout(Layout::Pointer newLayout,
		size_t animationFlags = AnimationFlag_None, float duration = 0.3f);

	void popTopmostLayout(size_t animationFlags = AnimationFlag_None,
		float duration = 0.3f);

	void replaceLayout(Layout::Pointer oldLayout, Layout::Pointer newLayout,
		size_t animationFlags = AnimationFlag_None, float duration = 0.3f);

	void removeLayout(Layout::Pointer oldLayout, size_t animationFlags = AnimationFlag_None,
		float duration = 0.3f);

	void pushLayout(Layout::Pointer newLayout, size_t animationFlags = AnimationFlag_None,
		float duration = 0.3f);

	void setTopLevelLayout(Layout::Pointer);

	ImageView& backgroundImageView();
	const ImageView& backgroundImageView() const;
	void setBackgroundImage(const Image& img);

	ImageView& overlayImageView();
	const ImageView& overlayImageView() const;
	void setOverlayImage(const Image& img);

	bool pointerPressed(et::PointerInputInfo);
	bool pointerMoved(et::PointerInputInfo);
	bool pointerReleased(et::PointerInputInfo);
	bool pointerCancelled(et::PointerInputInfo);
	bool pointerScrolled(et::PointerInputInfo);

	bool keyPressed(size_t);
	bool charactersEntered(std::string);

	void broadcastMessage(const Message&);

	void removeAllLayouts();

	ET_DECLARE_EVENT1(layoutDidAppear, Layout::Pointer);
	ET_DECLARE_EVENT1(layoutDidDisappear, Layout::Pointer);
	ET_DECLARE_EVENT1(layoutWillAppear, Layout::Pointer);
	ET_DECLARE_EVENT1(layoutWillDisappear, Layout::Pointer);

private:
	class LayoutEntry;

	struct LayoutPair
	{
		Layout::Pointer oldLayout;
		Layout::Pointer newLayout;

		LayoutPair(Layout::Pointer o, Layout::Pointer n) :
			oldLayout(o), newLayout(n)
		{
		}
	};

	void buildLayoutVertices(RenderContext* rc, RenderingElement::Pointer element,
		Layout::Pointer layout);

	void buildBackgroundVertices(RenderContext* rc);
	void buildOverlayVertices(RenderContext* rc);

	void onKeyboardNeeded(Layout* l, Element2d* e);
	void onKeyboardResigned(Layout* l);

	void getAnimationParams(size_t flags, vec3* nextSrc, vec3* nextDst, vec3* currDst);

	void removeLayoutEntryFromList(LayoutEntry*);
	void layoutEntryTransitionFinished(LayoutEntry*);

	LayoutEntry* entryForLayout(Layout::Pointer);
	bool animatingTransition();

	void animateLayoutAppearing(Layout::Pointer, LayoutEntry* newEntry,
		size_t animationFlags, float duration);

	void internal_replaceTopmostLayout(Layout::Pointer, AnimationDescriptor);
	void internal_popTopmostLayout(AnimationDescriptor desc);
	void internal_replaceLayout(LayoutPair, AnimationDescriptor);
	void internal_removeLayout(Layout::Pointer, AnimationDescriptor);
	void internal_pushLayout(Layout::Pointer, AnimationDescriptor);

	void validateTopLevelLayout();

	void reloadObject(LoadableObject::Pointer, ObjectsCache&);

private:
	class LayoutEntry : public Shared
	{
	public:
		ET_DECLARE_POINTER(LayoutEntry);

	public:
		enum State
		{
			State_Still,
			State_Appear,
			State_Disappear,
		};

	public:
		LayoutEntry(Scene* own, RenderContext* rc, Layout::Pointer l);
		~LayoutEntry();

		void animateTo(const vec3& oa, float duration, State s);

	private:
		ET_DENY_COPY(LayoutEntry);

	public:
		Layout::Pointer layout;
		Vector3Animator animator;
		vec3 offsetAlpha;
		State state;
	};

	typedef std::list<LayoutEntry::Pointer> LayoutEntryList;

private:
	RenderContext* _rc = nullptr;
	SceneRenderer _renderer;
	ObjectsCache _sharedCache;

	RenderingElement::Pointer _renderingElementBackground;
	RenderingElement::Pointer _renderingElementOverlay;
	ImageView _background;
	ImageView _overlay;

	Element2d::Pointer _keyboardFocusedElement;
	Element2d::Collection _prerenderElements;

	LayoutEntryList _layouts;
	Layout::Pointer _topLayout;
	vec2 _screenSize;
};

}
}
