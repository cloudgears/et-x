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
		class SceneRenderer;
		
		class TextElement : public Element2d
		{
		public:
			ET_DECLARE_POINTER(TextElement)
			
			enum TextStyle
			{
				TextStyle_SignedDistanceField,
				TextStyle_SignedDistanceFieldShadow,
				TextStyle_SignedDistanceFieldBevel,
				TextStyle_Plain,
				TextStyle_max
			};
			
		public:
			TextElement(Element2d*, const Font::Pointer&, float, const std::string& = emptyString);
			
			Font::Pointer& font()
				{ return _font; }
			
			const Font::Pointer& font() const
				{ return _font; }
			
			float fontSize() const
				{ return _fontSize; }

			float fontSmoothing() const
				{ return _fontSmoothing; }
			
			void setFont(const Font::Pointer&);
			void setFontSize(float);
			void setFontSmoothing(float);
			void setTextStyle(TextStyle);
			
			void loadProperties(const Dictionary&) override;
			
			SceneProgram& textProgram(SceneRenderer&);
			
			void setShadowOffset(const vec2&);
			
		protected:
			void processMessage(const Message&) override;
			void setProgramParameters(et::Program::Pointer&) override;
			
			virtual void invalidateText() { }
			
			void initTextProgram(SceneRenderer&);
			
		private:
			Font::Pointer _font;
			SceneProgram _textProgram;
			ProgramUniform _shadowUniform;
			vec2 _shadowOffset;
			float _fontSize = 12.0f;
			float _fontSmoothing = 1.0f;
			TextStyle _textStyle = TextStyle_SignedDistanceField;
		};
	}
}
