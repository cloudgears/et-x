/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/texture.h>
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
			Font(const CharacterGenerator::Pointer& generator, size_t);
			
			CharacterGenerator::Pointer& generator()
				{ return _generator; }

			const CharacterGenerator::Pointer& generator() const
				{ return _generator; }

			void loadFromFile(RenderContext*, const std::string&, ObjectsCache&);
			void saveToFile(RenderContext*, const std::string&);
			
			CharDescriptorList buildString(const std::string&);
			CharDescriptorList buildString(const std::wstring&);

			vec2 measureStringSize(const std::string&);
			vec2 measureStringSize(const std::wstring&);
			vec2 measureStringSize(const CharDescriptorList&);
			
		private:
			CharacterGenerator::Pointer _generator;
			size_t _size = 0;
			float _scale = 1.0f;
		};
	}
}
