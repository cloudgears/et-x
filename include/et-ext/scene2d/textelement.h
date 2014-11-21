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
		class TextElement : public Element2d
		{
		public:
			ET_DECLARE_POINTER(TextElement)
			
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
			
			void loadProperties(const Dictionary&) override;
			
		protected:
			void processMessage(const Message&) override;
			virtual void invalidateText() { }
			
		private:
			Font::Pointer _font;;
			float _fontSize = 12.0f;
			float _fontSmoothing = 1.0f;
		};
	}
}
