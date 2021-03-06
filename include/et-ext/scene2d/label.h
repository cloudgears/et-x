/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/textelement.h>
#include <et-ext/scene2d/font.h>

namespace et
{
	namespace s2d
	{
		class Label : public TextElement
		{
		public:
			ET_DECLARE_POINTER(Label)
			
			static std::string fitStringToWidthWithFont(std::string inputString, Font::Pointer font,
				float fontSize, float width);

		public:
			Label(const std::string&, const Font::Pointer&, float, Element2d*, const std::string& = emptyString);

			vec2 textSize();
						
			const std::string& text() const
				{ return _text.cachedText; }
			
			const vec4& backgroundColor() const
				{ return _backgroundColor; }
			
			const Alignment horizontalAlignment() const
				{ return _horizontalAlignment; }

			const Alignment verticalAlignment() const
				{ return _verticalAlignment; }
			
			void fitToWidth(float);
			void adjustSize();
			
			void setShouldAutoAdjustSize(bool);
			
			void setBackgroundColor(const vec4&);
			
			void setHorizontalAlignment(Alignment);
			void setVerticalAlignment(Alignment);
			
			void setShadowColor(const vec4& color);
			void setShadowOffset(const vec2& offset);
			void setText(const std::string& text, float duration = 0.0f);
			
			vec2 contentSize();
			
			void setLineInterval(float);
			
			void processMessage(const Message&);

		private:
			void addToRenderQueue(RenderContext* rc, SceneRenderer& renderer);
			void buildVertices(RenderContext* rc, SceneRenderer& renderer);
			void update(float t);
			void invalidateText();
			
		private:
			LocalizedText _text;
			LocalizedText _nextText;
			
			CharDescriptorList _charListText;
			CharDescriptorList _charListNextText;
			
			SceneVertexList _backgroundVertices;
			SceneVertexList _vertices;
			vec4 _backgroundColor = vec4(0.0f);
			vec4 _shadowColor = vec4(0.0f);
			vec2 _textSize = vec2(0.0f);
			vec2 _nextTextSize = vec2(0.0f);
			vec2 _shadowOffset = vec2(1.0f);
			float _lineInterval = 1.0f;
			float _textFade = 0.0f;
			float _textFadeDuration = 0.0f;
			float _textFadeStartTime = 0.0f;
			Alignment _horizontalAlignment = Alignment_Near;
			Alignment _verticalAlignment = Alignment_Near;
			bool _animatingText = false;
			bool _autoAdjustSize = true;
		};
	}
}
