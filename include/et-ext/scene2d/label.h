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
		class Label : public Element2d
		{
		public:
			ET_DECLARE_POINTER(Label)

		public:
			Label(const std::string& text, Font::Pointer font, Element2d* parent,
				const std::string& name = std::string());

			vec2 textSize();
			
			Font::Pointer font()
				{ return _font; }
			
			const std::string& text() const
				{ return _text; }
			
			const vec4& backgroundColor() const
				{ return _backgroundColor; }
			
			void fitToWidth(float);
			void adjustSize();
			
			void setAllowFormatting(bool);
			void setBackgroundColor(const vec4&);
			
			void setHorizontalAlignment(Alignment);
			void setVerticalAlignment(Alignment);
			
			void setShadowColor(const vec4& color);
			void setShadowOffset(const vec2& offset);
			void setText(const std::string& text, float duration = 0.0f);
			
			vec2 contentSize();

		private:
			void addToRenderQueue(RenderContext* rc, SceneRenderer& renderer);
			void buildVertices(RenderContext* rc, SceneRenderer& renderer);
			void update(float t);
			
		private:
			std::string _text;
			std::string _nextText;
			
			CharDescriptorList _charListText;
			CharDescriptorList _charListNextText;
			
			Font::Pointer _font;
			SceneVertexList _vertices;
			vec4 _backgroundColor;
			vec4 _shadowColor;
			vec2 _textSize;
			vec2 _nextTextSize;
			vec2 _shadowOffset;
			float _textFade;
			float _textFadeDuration;
			float _textFadeStartTime;
			Alignment _horizontalAlignment;
			Alignment _verticalAlignment;
			bool _animatingText;
			bool _allowFormatting;
		};
	}
}