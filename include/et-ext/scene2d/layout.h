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

			void setActiveElement(Element* e);
			
			ET_DECLARE_EVENT2(layoutRequiresKeyboard, Layout*, Element*)
			ET_DECLARE_EVENT1(layoutDoesntNeedKeyboard, Layout*)
			
			vec2 contentSize();
			
			template <typename F>
			void setPositionInterpolationFunction(F func)
				{ _positionInterpolationFunction = func; }
			
			const std::function<float(float)>& positionInterpolationFunction() const
				{ return _positionInterpolationFunction; }
			
		public:
			/*
			 * Virtual functions to subclass
			 */
			virtual void adjustVerticalOffset(float dy) { }
			
			virtual void resetVerticallastElementIndex() { }
			
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
			void addToRenderQueue(RenderContext* rc, SceneRenderer& gr);

		private:
			RenderingElement::Pointer _renderingElement;
			
			std::function<float(float)> _positionInterpolationFunction;
			
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
			ModalLayout(const std::string& name = std::string());
						
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
