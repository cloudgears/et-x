/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#pragma once

#include <et/input/input.h>
#include <et/timers/animator.h>
#include <et/apiobjects/program.h>
#include <et-ext/scene2d/baseclasses.h>

namespace et
{
	class RenderContext;
	
	namespace s2d
	{
		class Element;
		class Layout;
		
		typedef Hierarchy<Element, LoadableObject> ElementHierarchy;
		class Element : public ElementHierarchy, public FlagsHolder, public EventReceiver,
			public TimedObject
		{
		public:
			ET_DECLARE_POINTER(Element)

			union 
			{
				size_t tag;
				int tag_i;
			};

		public:
			Element(Element* parent, const std::string& name);
			virtual ~Element() { }

			void setParent(Element* element);

			void invalidateTransform();
			void invalidateContent();
			
			virtual bool enabled() const;
			virtual void setEnabled(bool enabled);

			virtual void broardcastMessage(const Message&);
			virtual void processMessage(const Message&) { }

			virtual void addToRenderQueue(RenderContext*, SceneRenderer&) { }
			virtual void addToOverlayRenderQueue(RenderContext*, SceneRenderer&) { }
			
			virtual SceneProgram initProgram(SceneRenderer&);
			virtual void setProgramParameters(et::Program::Pointer&) { }

			virtual bool pointerPressed(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerMoved(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerReleased(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerScrolled(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual bool pointerCancelled(const PointerInputInfo&)
				{ return !hasFlag(Flag_TransparentForPointer); }

			virtual void pointerEntered(const PointerInputInfo&) { }
			
			virtual void pointerLeaved(const PointerInputInfo&) { }

			virtual void keyPressed(size_t) { }

			virtual bool capturesPointer() const
				{ return false; }
			
			virtual bool containLocalPoint(const vec2&)
				{ return false; }
			
			virtual vec2 positionInElement(const vec2& p)
				{ return p; }

			virtual bool visible() const
				{ return true; }

			virtual float finalAlpha() const 
				{ return 1.0f; }

			virtual const mat4& finalTransform()
				{ return identityMatrix; }

			virtual const mat4& finalInverseTransform() 
				{ return identityMatrix; }
			
			const ElementLayout& autoLayout() const
				{ return _autoLayout; }
			
			Dictionary autoLayoutDictionary() const;

			void setAutolayout(Dictionary);
			void setAutolayout(const ElementLayout&);
			void autolayoutFromFile(const std::string&);
			
			void setAutolayout(const vec2& pos, LayoutMode pMode, const vec2& sz,
				LayoutMode sMode, const vec2& pivot);

			void setAutolayoutPosition(const vec2& pos);
			void setAutolayoutSize(const vec2& pos);

			void setAutolayoutRelativeToParent(const vec2& pos, const vec2& sz, const vec2& pivot);
			
			void setAutolayoutSizeMode(LayoutMode);
			
			void setAutolayoutMask(size_t);
			
			void fillParent();
			void setLocationInParent(Location, const vec2& offset = vec2(0.0f));

			void autoLayout(const vec2& contextSize, float duration = 0.0f);
			
			virtual void storeProperties(Dictionary) const { }
			virtual void loadProperties(Dictionary) { }
			
			virtual bool focused() const
				{ return false; }
			
			virtual void setFocus() { };
			virtual void resignFocus(Element*) { };
			virtual void didAppear() { }
			virtual void willAppear() { }
			virtual void didDisappear() { }
			virtual void willDisappear() { }
			virtual void willAutoLayout(float) { }
			virtual void didAutoLayout(float) { }
			virtual void willChangeFrame() { }
			virtual void didChangeFrame() { }
			
			void bringToFront(Element* c);
			void sendToBack(Element* c);

			Element* baseChildWithName(const std::string&, bool recursive);

			template <typename T>
			T* childWithName(const std::string& name, bool recursive)
				{ return static_cast<T*>(baseChildWithName(name, recursive)); }
			
			void removeAllChildren();

			/*
			 * Required Methods
			 */
			virtual SceneProgram program() = 0;
			
			virtual vec2 origin() const = 0;
			
			virtual const vec2& position() const = 0;
			virtual const vec2& desiredPosition() const = 0;
			virtual const vec2& desiredScale() const = 0;
			
			virtual const vec2& size() const = 0;
			virtual const vec2& desiredSize() const = 0;
			
			virtual const vec2& pivotPoint() const = 0;

			virtual void setPosition(const vec2&, float duration) = 0;
			virtual void setSize(const vec2&, float duration) = 0;
			virtual void setScale(const vec2&, float duration) = 0;
			virtual void setPivotPoint(const vec2&, bool preservePosition) = 0;
			virtual void setAngle(float, float duration) = 0;

			virtual bool containsPoint(const vec2&, const vec2&) = 0;
			
			virtual vec2 contentSize() = 0;

			/*
			 * Events
			 */
			ET_DECLARE_EVENT2(dragStarted, Element*, const ElementDragInfo&)
			ET_DECLARE_EVENT2(dragged, Element*, const ElementDragInfo&)
			ET_DECLARE_EVENT2(dragFinished, Element*, const ElementDragInfo&)
			
			ET_DECLARE_EVENT1(hoverStarted, Element*)
			ET_DECLARE_EVENT1(hoverEnded, Element*)

		protected:
			void setContentValid()
				{ _contentValid = true; }

			bool contentValid() const
				{ return _contentValid; }

			bool transformValid() const
				{ return _transformValid; }

			bool inverseTransformValid()
				{ return _inverseTransformValid; }

			virtual void setInvalid()
				{ if (parent()) parent()->setInvalid(); }

			void setTransformValid(bool v) 
				{ _transformValid = v; }

			void setIverseTransformValid(bool v)
				{ _inverseTransformValid = v; }

			virtual mat4 parentFinalTransform()
				{ return parent() ? parent()->finalTransform() : identityMatrix; }

			virtual Layout* owner()
				{ return parent() ? parent()->owner() : 0; }

			void startUpdates();
			TimerPool::Pointer timerPool();

			Element* childWithNameCallback(const std::string&, Element*, bool recursive);

			ET_DECLARE_PROPERTY_GET_REF_SET_REF(std::string, name, setName)

		protected:
			void startUpdates(TimerPool* timerPool);
			
		private:
			friend class Hierarchy<Element, LoadableObject>;

			Element(const Element&) : 
				ElementHierarchy(0) { }

			Element& operator = (const Element&)
				{ return *this; }

		private:
			ElementLayout _autoLayout;

			bool _enabled;
			bool _transformValid;
			bool _inverseTransformValid;
			bool _contentValid;
		};

		inline bool elementIsSelected(State s)
			{ return (s >= State_Selected) && (s < State_max); }

		State adjustState(State s);
		float alignmentFactor(Alignment a);
		std::string layoutModeToString(LayoutMode);
		LayoutMode layoutModeFromString(const std::string&);
		
		inline float linearInerpolation(float t)
			{ return t; }
		
		inline float sinBounceInterpolation(float t)
			{ float v = t + 0.4f * std::sin(t * PI); return v * v; }
		
		inline float polynomialBounceInterpolation(float t)
			{ float ts = t * t; return t + ts * (1.0f - ts); }
	}
	
	typedef Animator<vec2> Vector2Animator;
	typedef Animator<vec3> Vector3Animator;
	typedef Animator<vec4> Vector4Animator;
	typedef Animator<mat4> MatrixAnimator;
	typedef Animator<rect> RectAnimator;
}
	
