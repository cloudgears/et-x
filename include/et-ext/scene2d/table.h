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

				Section();
				
			private:
				friend class Table;
				
			private:
				float headerOffset = 0.0f;
				float footerOffset = 0.0f;
				
				float headerSize = 0.0f;
				float itemsSize = 0.0f;
				float footerSize = 0.0f;
				
				float sectionSize = 0.0f;
			};
			
		public:
			Table(et::s2d::Element2d*, const std::string& name = emptyString);
			~Table();
			
			Section* addSection(Element2d::Pointer header, const Element2d::List& items,
				Element2d::Pointer footer);
			
			void clean();

			void layoutChildren();

		protected:
			void layoutChildren(const vec2&);
			
		private:
			void didAutoLayout(float);
			void setOffsetDirectly(const vec2&);
			
		private:
			std::vector<Section*> _sections;
		};
	}
}
