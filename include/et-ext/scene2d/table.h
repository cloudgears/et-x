/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/scroll.h>

namespace et
{
	namespace s2d
	{
		class Table : public et::s2d::Scroll
		{
		public:
			ET_DECLARE_POINTER(Table)
			
			class Section
			{
			public:
				Element2d::Pointer header;
				Element2d::List items;
				Element2d::Pointer footer;
				
			private:
				Section();
				friend class Table;
				
			private:
				float headerOffset;
				float footerOffset;
				
				float headerSize;
				float itemsSize;
				float footerSize;
				
				float sectionSize;
			};
			
		public:
			Table(et::s2d::Element2d*, const std::string& name = std::string());
			~Table();
			
			Section* addSection(Element2d::Pointer header, const Element2d::List& items,
				Element2d::Pointer footer);
			
			void clean();

		private:
			void layout(const vec2&);
			void autoLayout(const vec2& contextSize, float duration);
			
			void layoutChildren(const vec2&);
			
			void setOffsetDirectly(const vec2&);
			
		private:
			std::vector<Section*> _sections;
		};
	}
}