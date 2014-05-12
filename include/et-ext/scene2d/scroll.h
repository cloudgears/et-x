/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>

namespace et
{
	namespace s2d
	{
		class Scroll : public Element2d
		{
		public:
			ET_DECLARE_POINTER(Scroll)
			
			enum Bounce
			{
				Bounce_Horizontal = 0x01,
				Bounce_Vertical = 0x02,
			};
			
		public:
			Scroll(Element2d* parent, const std::string& name = std::string());
			
			void setBounce(size_t);
			void setBounceExtent(const vec2&);
			
			void setContentSize(const vec2& cs);
			
			void setOffset(const vec2& aOffset, float duration = 0.0f);
			void applyOffset(const vec2& dOffset, float duration = 0.0f);
			
			vec2 contentSize();
			void adjustContentSize();
			
			void setBackgroundColor(const vec4& color);
			void setOverlayColor(const vec4& color);
			void setScrollbarsColor(const vec4&);
			
			void setMovementTreshold(float);

			void setPointerScrollMultiplier(const vec2&);
			void setPointerScrollDuration(float);

			const vec2& lastElementIndex() const
				{ return _offset; }
			
			void scrollToBottom(float duration = 0.0f);
						
			const vec2& pointerScrollMultiplier() const
				{ return _pointerScrollMultiplier; }

			float pointerScrollDuration() const
				{ return _pointerScrollDuration; }

		protected:
			virtual void setOffsetDirectly(const vec2& o);
			
		protected:
			void update(float t);
			
		private:
			void buildVertices(RenderContext* rc, SceneRenderer& r);
			
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			void addToOverlayRenderQueue(RenderContext*, SceneRenderer&);
			
			const mat4& finalTransform();
			const mat4& finalInverseTransform();
			
			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerCancelled(const PointerInputInfo&);
			bool pointerScrolled(const PointerInputInfo&);
			bool containsPoint(const vec2& p, const vec2& np);
			
			void invalidateChildren();
			void broadcastPressed(const PointerInputInfo&);
			void broadcastMoved(const PointerInputInfo&);
			void broadcastReleased(const PointerInputInfo&);
			void broadcastCancelled(const PointerInputInfo&);

			float scrollOutOfContentXSize() const;
			float scrollOutOfContentYSize() const;

			float scrollUpperLimit() const;
			float scrollUpperDefaultValue() const;
			float scrollLowerLimit() const;
			float scrollLowerDefaultValue() const;

			float scrollLeftLimit() const;
			float scrollLeftDefaultValue() const;
			float scrollRightLimit() const;
			float scrollRightDefaultValue() const;

			void updateBouncing(float deltaTime);
			
			bool horizontalBounce() const
				{ return (_bounce & Bounce_Horizontal) != 0; }
			
			bool verticalBounce() const
				{ return (_bounce & Bounce_Vertical) != 0; }

			Element* getActiveElement(const PointerInputInfo&, Element* root);
			void setActiveElement(const PointerInputInfo& p, Element* e);
			
		private:
			enum BounceDirection
			{
				BounceDirection_None,
				BounceDirection_ToNear,
				BounceDirection_ToFar
			};
			
		private:
			Element::Pointer _activeElement;
			Element::Pointer _capturedElement;
			
			SceneVertexList _backgroundVertices;
			SceneVertexList _overlayVertices;

			Vector2Animator _offsetAnimator;
			
			PointerInputInfo _currentPointer;
			PointerInputInfo _previousPointer;
			
			mat4 _localFinalTransform;
			mat4 _localInverseTransform;
			vec4 _backgroundColor;
			vec4 _scrollbarsColor;
			vec4 _overlayColor;
			vec2 _contentSize;
			vec2 _offset;
			vec2 _velocity;
			vec2 _bounceExtent;
			vec2 _pointerScrollMultiplier;
			vector2<BounceDirection> _bouncing;
			size_t _bounce;
			float _updateTime;
			float _scrollbarsAlpha;
			float _scrollbarsAlphaTarget;
			float _pointerScrollDuration;
			float _movementTreshold;

			bool _pointerCaptured;
			bool _manualScrolling;
		};

	}
}
