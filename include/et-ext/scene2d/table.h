/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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
			ET_DECLARE_POINTER(Table);
			
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
			
			const std::vector<Section*>& sections() const
				{ return _sections; }
			
			void clean();

			void layoutChildren();

		protected:
			void validateSections();
			void layoutChildren(const vec2&);
			void didAutoLayout(float);
			void setOffsetDirectly(const vec2&);
			
		private:
			std::vector<Section*> _sections;
		};
	}
}
