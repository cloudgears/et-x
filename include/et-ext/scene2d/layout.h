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
			Layout(const std::string& name = std::string());
			
			bool valid() const
				{ return _valid; }
			
			void update(float);
			
			void cancelDragging(float returnDuration = 0.0f);
			void cancelInteractions();

			void setActiveElement(Element::Pointer e);
			
			ET_DECLARE_EVENT2(layoutRequiresKeyboard, Layout*, Element*)
			ET_DECLARE_EVENT1(layoutDoesntNeedKeyboard, Layout*)
			
			vec2 contentSize();
			
			template <typename F>
			void setPositionInterpolationFunction(F func)
				{ _positionInterpolationFunction = func; }
			
			const std::function<float(float)>& positionInterpolationFunction() const
				{ return _positionInterpolationFunction; }
			
						
		public:
			virtual void activeElementChanged(Element*) { }
			
		protected:
			friend class Scene;
			
			RenderingElement::Pointer renderingElement()
				{ return _renderingElement; }
			
			void initRenderingElement(RenderContext* rc);
			
			bool pointerPressed(const PointerInputInfo&) override;
			bool pointerMoved(const PointerInputInfo&) override;
			bool pointerReleased(const PointerInputInfo&) override;
			bool pointerCancelled(const PointerInputInfo&) override;
			bool pointerScrolled(const PointerInputInfo&) override;
			
			bool elementIsBeingDragged(Element::Pointer);
			
			void cancelInteractionsInElement(Element::Pointer, const PointerInputInfo&);
			
		private:
			Layout* owner()
				{ return this; }
			
			void setInvalid();
			void collectTopmostElements(Element::Pointer);
			
			void collectPreRenderingObjects(Element::Pointer, Element::List&);
			
			Element::Pointer activeElement(const PointerInputInfo& p);
			Element::Pointer getActiveElement(const PointerInputInfo& p, Element::Pointer e);
			
			void setCurrentElement(const PointerInputInfo& p, Element::Pointer e);
			void addElementToRenderQueue(Element::Pointer element, RenderContext* rc, SceneRenderer& gr);
			
			void performDragging(const PointerInputInfo&);
			void addToRenderQueue(RenderContext* rc, SceneRenderer& gr);

		private:
			RenderingElement::Pointer _renderingElement;
			
			std::function<float(float)> _positionInterpolationFunction;
			
			Element::Pointer _currentElement;
			Element::Pointer _focusedElement;
			Element::Pointer _capturedElement;
			Element::List _topmostElements;
			
			vec2 _dragInitialPosition;
			vec2 _dragInitialOffset;
			
			bool _valid;
			bool _dragging;
		};

		class ModalLayout : public Layout
		{
		public:
			ModalLayout(const std::string& name = std::string());
						
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
