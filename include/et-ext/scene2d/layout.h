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
			Layout(const std::string& name = emptyString);
			
			bool valid() const
				{ return _valid; }
			
			void update(float);
			
			void cancelDragging(float returnDuration = 0.0f);
			void cancelInteractions();

			void setActiveElement(Element2d::Pointer e);
			
			ET_DECLARE_EVENT2(layoutRequiresKeyboard, Layout*, Element2d*)
			ET_DECLARE_EVENT1(layoutDoesntNeedKeyboard, Layout*)
			
			vec2 contentSize();
			
			template <typename F>
			void setPositionInterpolationFunction(F func)
				{ _positionInterpolationFunction = func; }
			
			const std::function<float(float)>& positionInterpolationFunction() const
				{ return _positionInterpolationFunction; }
			
			Element2d::Pointer findFirstResponder(const Message&);
						
		public:
			virtual void activeElementChanged(Element2d*) { }
			
		protected:
			friend class Scene;
			
			RenderingElement::Pointer renderingElement()
				{ return _renderingElement; }
			
			void initRenderingElement(RenderContext* rc);
			
			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerCancelled(const PointerInputInfo&);
			bool pointerScrolled(const PointerInputInfo&);
			
			bool elementIsBeingDragged(Element2d::Pointer);
			
			void cancelInteractionsInElement(Element2d::Pointer, const PointerInputInfo&);
			
			Element2d::Pointer findFirstResponderStartingFrom(Element2d::Pointer, const Message&);
			
		private:
			Layout* owner()
				{ return this; }
			
			void setInvalid();
			void collectTopmostElements(Element2d::Pointer);
			
			void collectPreRenderingObjects(Element2d::Pointer, Element2d::List&);
			
			Element2d::Pointer activeElement(const PointerInputInfo& p);
			Element2d::Pointer getActiveElement(const PointerInputInfo& p, Element2d::Pointer e);
			
			void setCurrentElement(const PointerInputInfo& p, Element2d::Pointer e);
			void performDragging(const PointerInputInfo&);

			void addToRenderQueue(RenderContext* rc, SceneRenderer& gr);
			void addElementToRenderQueue(Element2d::Pointer& element, RenderContext* rc, SceneRenderer& gr);
			
		private:
			RenderingElement::Pointer _renderingElement;
			
			std::function<float(float)> _positionInterpolationFunction;
			
			Element2d::Pointer _currentElement;
			Element2d::Pointer _focusedElement;
			Element2d::Pointer _capturedElement;
			Element2d::List _topmostElements;
			
			vec2 _dragInitialPosition;
			vec2 _dragInitialOffset;
			
			bool _valid;
			bool _dragging;
		};

		class ModalLayout : public Layout
		{
		public:
			ModalLayout(const std::string& name = emptyString);
						
		protected:
			ImageView::Pointer backgroundFade()
				{ return _backgroundFade; }

			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerScrolled(const PointerInputInfo&);
			
		private:
			ImageView::Pointer _backgroundFade;
		};
	}
}
