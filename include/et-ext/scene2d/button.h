/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>
#include <et-ext/scene2d/font.h>

namespace et
{
	namespace s2d
	{
		class Button : public Element2d
		{
		public:
			ET_DECLARE_POINTER(Button)
			typedef std::list<Pointer> List;

			enum Type
			{
				Type_PushButton,
				Type_CheckButton
			};

			enum ImageLayout
			{
				ImageLayout_Left,
				ImageLayout_Right,
				ImageLayout_Top,
				ImageLayout_Bottom
			};
			
			enum ContentMode
			{
				ContentMode_Fit,
				ContentMode_ScaleMaxToMin,
			};

		public:
			Button(const std::string& title, const Font::Pointer& font, Element2d* parent,
				const std::string& name = std::string());
			
			void setImage(const Image& img);
			void setImageForState(const Image& img, State s);
			
			void setCommonBackground(const Image& img);
			
			void setBackground(const Image& img);
			void setBackgroundForState(const Image& img, State s);

			void adjustSize(float duration = 0.0f, bool vertical = true, bool horizontal = true);
			void adjustSizeForText(const std::string&, float duration = 0.0f, bool vertical = true, bool horizontal = true);
			vec2 sizeForText(const std::string&);
			
			void setContentMode(ContentMode);
			
			const Font::Pointer& font() const
				{ return _font; }

			Font::Pointer& font()
				{ return _font; }
			
			ET_DECLARE_EVENT1(clicked, Button*)
			ET_DECLARE_EVENT1(pressed, Button*)
			ET_DECLARE_EVENT1(released, Button*)
			ET_DECLARE_EVENT1(releasedInside, Button*)
			ET_DECLARE_EVENT1(releasedOutside, Button*)
			ET_DECLARE_EVENT1(cancelled, Button*)
			
			const Image& backgroundForState(State state) const
				{ return _background[state]; }

			bool capturePointer() const;

			const std::string& title() const 
				{ return _nextTitle.cachedText; }

			void setTitle(const std::string&, float duration = 0.0f);

			const Image& imageForState(State s) const
				{ return _image[s]; }
			
			void setImageLayout(ImageLayout l);

			const vec2& textSize() const
				{ return _maxTextSize; }

			void setTextColor(const vec4& color);
			const vec4& textColor() const;

			void setPressedColor(const vec4& color);
			const vec4& pressedColor() const;
			
			void setTextPressedColor(const vec4& color);
			const vec4& textPressedColor() const;
			
			void setBackgroundColor(const vec4&);
			const vec4& backgroundColor() const;
			
			void setBackgroundTintColor(const vec4&, float);
			const vec4& backgroundTintColor() const;

			void setCommonBackgroundTintColor(const vec4&, float);
			const vec4& commonBackgroundTintColor() const;
			
			bool pointerPressed(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			bool pointerCancelled(const PointerInputInfo&);
			void pointerEntered(const PointerInputInfo&);
			void pointerLeaved(const PointerInputInfo&);

			Button::Type type() const
				{ return _type; }

			void setType(Button::Type t);
			
			bool selected() const
				{ return _selected; }
			
			void setSelected(bool s);

			void setContentOffset(const vec2& o);
			
			vec2 contentSize();
			
			void setShouldAdjustPressedBackground(bool);
			
			void setHorizontalAlignment(Alignment);
			void setVerticalAlignment(Alignment);
			
			void processMessage(const Message&) override;
			
			void setClickTreshold(float);
			void setShouldInvokeClickInRunLoop(bool);

		protected:
			void performClick();

		private:
			void buildVertices(RenderContext* rc, SceneRenderer& gr);
			void addToRenderQueue(RenderContext* rc, SceneRenderer& renderer);
			
			void setCurrentState(State s);

		private:
			Font::Pointer _font;
			
			LocalizedText _currentTitle;
			LocalizedText _nextTitle;
			
			SceneVertexList _bgVertices;
			SceneVertexList _textVertices;
			SceneVertexList _imageVertices;
			
			CharDescriptorList _currentTitleCharacters;
			CharDescriptorList _nextTitleCharacters;
			
			Image _commonBackground;
			StaticDataStorage<Image, State_max> _background;
			StaticDataStorage<Image, State_max> _image;
			
			vec4 _textColor;
			vec4 _textPressedColor;
			vec4 _pressedColor;
			vec4 _backgroundColor;
			vec4 _backgroundTintColor;
			vec4 _commonBackgroundTintColor;
			vec2 _currentTextSize;
			vec2 _nextTextSize;
			vec2 _maxTextSize;
			vec2 _contentOffset;
			
			FloatAnimator _titleAnimator;
			Vector4Animator _backgroundTintAnimator;
			Vector4Animator _commonBackgroundTintAnimator;

			Type _type;
			State _state;
			ImageLayout _imageLayout;
			ContentMode _contentMode;
			Alignment _horizontalAlignment;
			Alignment _verticalAlignment;
			
			float _lastClickTime = 0.0f;
			float _clickTreshold = 0.0f;
			float _titleTransition = 0.0f;
			
			bool _pressed = false;
			bool _hovered = false;
			bool _selected = false;
			bool _adjustPressedBackground = false;
			bool _shouldInvokeClickInRunLoop = false;
		};
	}
}
