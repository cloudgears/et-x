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
			Font(const CharacterGenerator::Pointer& generator);
			
			const Texture& texture() const
				{ ET_ASSERT(_generator.valid()); return _generator->texture(); }
			
			const std::string& face() const
				{ ET_ASSERT(_generator.valid()); return _generator->face(); }
			
			size_t size() const
				{ ET_ASSERT(_generator.valid()); return _generator->size(); }
			
			CharDescriptor charDescription(int c)
				{ ET_ASSERT(_generator.valid()); return _generator->charDescription(c); }
			
			CharDescriptor boldCharDescription(int c)
				{ ET_ASSERT(_generator.valid()); return _generator->boldCharDescription(c); }

			float lineHeight() const
				{ ET_ASSERT(_generator.valid()); return _generator->lineHeight(); }

			void loadFromFile(RenderContext* rc, const std::string& fileName, ObjectsCache& cache);
			void saveToFile(RenderContext* rc, const std::string& fileName);
			
			CharDescriptorList buildString(const std::string& s, bool formatted = false);
			CharDescriptorList buildString(const std::wstring& s, bool formatted = false);

			vec2 measureStringSize(const std::string& s, bool formatted = false);
			vec2 measureStringSize(const std::wstring& s, bool formatted = false);
			vec2 measureStringSize(const CharDescriptorList& s);
			
		private:
			bool isUtf8String(const std::string& s) const;

			CharDescriptorList parseString(const std::string& s);
			CharDescriptorList parseString(const std::wstring& s);

		private:
			CharacterGenerator::Pointer _generator;
		};
	}
}