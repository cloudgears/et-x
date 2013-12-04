/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element.h>

namespace et
{
	namespace s2d
	{
		class Element2d : public Element
		{
		public:
			ET_DECLARE_POINTER(Element2d)
			typedef std::list<Pointer> List;

		public:
			Element2d(Element* parent, const std::string& name = std::string());
			Element2d(const rect& frame, Element* parent, const std::string& name = std::string());

			virtual ElementRepresentation representation() const
				{ return ElementRepresentation_2d; };

			const vec2& size() const;
			const vec2& position() const;
			
			const vec2& desiredSize() const;
			const vec2& desiredPosition() const;
			const vec2& desiredScale() const;
			
			const vec2& scale() const;
			const vec2& pivotPoint() const;

			const vec4 color() const;

			rect frame() const;
			
			vec2 origin() const;
			vec2 offset() const;
			vec2 contentSize();

			float angle() const;
			float alpha() const;
			bool visible() const;

			/*
			 * Setters
			 */
			virtual void setAngle(float angle, float duration = 0.0f);
			virtual void setScale(const vec2& scale, float duration = 0.0f);
			virtual void setColor(const vec4& color, float duration = 0.0f);
			virtual void setPivotPoint(const vec2& p, bool preservePosition = true);

			/*
			 * Convenience
			 */
			void setPosition(const vec2& p, float duration = 0.0f);
			void setAlpha(float alpha, float duration = 0.0f);
			void setPosition(float x, float y, float duration = 0.0f);
			void setSize(const vec2& s, float duration = 0.0f);
			void setSize(float w, float h, float duration = 0.0f);
			void rotate(float angle, float duration = 0.0f);
			void setVisible(bool visible, float duration = 0.0f);

			virtual bool containsPoint(const vec2& p, const vec2&);
			virtual bool containLocalPoint(const vec2& p);

			const mat4& finalTransform();
			const mat4& finalInverseTransform();
			vec2 positionInElement(const vec2& p);

			ET_DECLARE_EVENT2(elementAnimationFinished, Element2d*, AnimatedPropery)

		protected:
			void buildFinalTransform();

			mat4 transform();
			float finalAlpha() const;

			void animatorUpdated(BaseAnimator*);
			void animatorFinished(BaseAnimator*);
			
			SceneProgram program()
				{ return _defaultProgram; }
			
			SceneProgram initProgram(SceneRenderer&);

		private:
			Vector2Animator _positionAnimator;
			Vector2Animator _sizeAnimator;
			Vector4Animator _colorAnimator;
			Vector2Animator _scaleAnimator;
			FloatAnimator _angleAnimator;
			
			SceneProgram _defaultProgram;
			
			mat4 _finalTransform;
			mat4 _finalInverseTransform;
			
			ElementLayout _layout;
			ElementLayout _desiredLayout;
			
			vec4 _color;
		};
	}
}
