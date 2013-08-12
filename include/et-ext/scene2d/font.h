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
		class Font : public Shared
		{
		public:
			ET_DECLARE_POINTER(Font)
			
		public:
			Font();
			Font(RenderContext* rc, const std::string& fileName, ObjectsCache& cache);
			Font(const CharacterGenerator::Pointer& generator);
			~Font();

			void loadFromFile(RenderContext* rc, const std::string& fileName, ObjectsCache& cache);

			const Texture& texture() const 
				{ return _generator.valid() ? _generator->texture() : _texture; }
			const std::string& face() const
				{ return _generator.valid() ? _generator->face() : _face; }
			size_t size() const
				{ return _generator.valid() ? _generator->size() : _size; }

			CharDescriptor charDescription(int c);
			CharDescriptor boldCharDescription(int c);

			float lineHeight() const;

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

			Texture _texture;
			CharDescriptorMap _chars;
			CharDescriptorMap _boldChars;
			std::string _face;
			et::vec2 _biggestChar;
			et::vec2 _biggestBoldChar;
			size_t _size;
		};
	}
}