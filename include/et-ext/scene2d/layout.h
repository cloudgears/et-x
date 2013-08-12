/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>
#include <et-ext/scene2d/imageview.h>

namespace et
{
	namespace s2d
	{
		class Layout : public Element2d
		{
		public:
			ET_DECLARE_POINTER(Layout)
			
		public:
			Layout();

			bool valid() const
				{ return _valid; }

			void layout(const vec2& sz);

			void addToRenderQueue(RenderContext* rc, SceneRenderer& gr);

			void update(float);
			void cancelDragging(float returnDuration = 0.0f);

			virtual void adjustVerticalOffset(float dy);
			virtual void resetVerticalOffset();
			
			void setActiveElement(Element* e);
			
			ET_DECLARE_EVENT2(layoutRequiresKeyboard, Layout*, Element*)
			ET_DECLARE_EVENT1(layoutDoesntNeedKeyboard, Layout*)
			
			vec2 contentSize();
			
		protected:
			friend class Scene;
			
			RenderingElement::Pointer renderingElement()
				{ return _renderingElement; }
			
			void initRenderingElement(et::RenderContext* rc);
			
			bool pointerPressed(const et::PointerInputInfo&);
			bool pointerMoved(const et::PointerInputInfo&);
			bool pointerReleased(const et::PointerInputInfo&);
			bool pointerScrolled(const et::PointerInputInfo&);
			
			bool elementIsBeingDragged(s2d::Element*);
			
		private:
			Layout* owner()
				{ return this; }
			
			void setInvalid();
			void collectTopmostElements(Element* element);
			
			Element* activeElement(const PointerInputInfo& p);
			Element* getActiveElement(const PointerInputInfo& p, Element* e);
			void setCurrentElement(const PointerInputInfo& p, Element* e);
			void addElementToRenderQueue(Element* element, RenderContext* rc, SceneRenderer& gr);
			
			void performDragging(const PointerInputInfo&);

		private:
			RenderingElement::Pointer _renderingElement;
			
			Element* _currentElement;
			Element* _focusedElement;
			Element* _capturedElement;
			Element::List _topmostElements;
			vec2 _dragInitialPosition;
			vec2 _dragInitialOffset;
			bool _valid;
			bool _dragging;
		};

		class ModalLayout : public Layout
		{
		public:
			ModalLayout();
						
		protected:
			ImageView::Pointer backgroundFade()
				{ return _backgroundFade; }

			bool pointerPressed(const et::PointerInputInfo&);
			bool pointerMoved(const et::PointerInputInfo&);
			bool pointerReleased(const et::PointerInputInfo&);
			bool pointerScrolled(const et::PointerInputInfo&);
			
		private:
			ImageView::Pointer _backgroundFade;
		};
	}
}
