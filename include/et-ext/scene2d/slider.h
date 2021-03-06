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
        class Slider : public Element2d
        {
        public:
			ET_DECLARE_POINTER(Slider)
			
			enum SliderImagesMode
			{
				SliderImagesMode_Stretch,
				SliderImagesMode_Crop
			};
			
			enum BackgroundImageMode
			{
				BackgroundImageMode_Center,
				BackgroundImageMode_Stretch,
			};
			
		public:
			Slider(Element2d* parent);
			
			void setBackgroundImage(const Image&);
			void setBackgroundColor(const vec4&);
			
			void setHandleImage(const Image&, float scale);
			void setHandleImageForState(const Image&, float scale, State);
			void setHandleFillColor(const vec4&);
			
			void setSliderImages(const Image& left, const Image& right);
			void setSliderFillColors(const vec4& l, const vec4& r);
			
			void setSliderImageMode(SliderImagesMode);
			
			const s2d::Image& sliderLeftImage() const
				{ return _sliderLeft; }
			
			const s2d::Image& sliderRightImage() const
				{ return _sliderRight; }
			
			float minValue() const
				{ return _min; }
			
			float maxValue() const
				{ return _max; }

			void setRange(float aMin, float aMax, float duration = 0.0f);
			
			void setValue(float v, float duration = 0.0f);
			float value() const;
			float normalizedValue() const;
			
			ET_DECLARE_EVENT1(changed, Slider*)
			ET_DECLARE_EVENT1(valueChanged, float)
			ET_DECLARE_EVENT0(draggingFinished)
			
        private:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			void buildVertices(RenderContext*, SceneRenderer& renderer);
			
			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerCancelled(const PointerInputInfo&);
			
			void updateValue(float);
			
			float handleWidth() const;
			
        private:
			Image _background;
			Image _sliderLeft;
			Image _sliderRight;
			Image _handle[State_max];

			vec4 _sliderLeftColor = vec4(0.0f);
			vec4 _sliderRightColor = vec4(0.0f);
			vec4 _handleFillColor = vec4(0.0f);
			
			SceneVertexList _backgroundVertices;
			SceneVertexList _sliderLeftVertices;
			SceneVertexList _sliderRightVertices;
			SceneVertexList _handleVertices;
			
			vec4 _backgroundColor = vec4(0.0f);
			
			float _handleScale[State_max];
			
			float _min = 0.0f;
			float _max = 1.0f;
			FloatAnimator _value;
			
			State _state = State_Default;
			SliderImagesMode _sliderImagesMode;
			BackgroundImageMode _backgroundImageMode;
        };
    }
}
