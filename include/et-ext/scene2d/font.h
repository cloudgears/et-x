/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/rendering/texture.h>
#include <et-ext/scene2d/charactergenerator.h>

namespace et
{
	namespace s2d
	{
		class Font : public Object
		{
		public:
			ET_DECLARE_POINTER(Font)
			
		public:
			Font(const CharacterGenerator::Pointer& generator);
			
			CharacterGenerator::Pointer& generator()
				{ return _generator; }

			const CharacterGenerator::Pointer& generator() const
				{ return _generator; }

			bool loadFromDictionary(RenderContext*, const Dictionary&, ObjectsCache&, const std::string&);
			bool loadFromFile(RenderContext*, const std::string&, ObjectsCache&);
			
			void saveToFile(RenderContext*, const std::string&);
			
			CharDescriptorList buildString(const std::string&, float, float = 1.0f);
			CharDescriptorList buildString(const std::wstring&, float, float = 1.0f);

			vec2 measureStringSize(const std::string&, float, float = 1.0f);
			vec2 measureStringSize(const std::wstring&, float, float = 1.0f);
			
			vec2 measureStringSize(const CharDescriptorList&);
			
		private:
			CharacterGenerator::Pointer _generator;
		};
	}
}
