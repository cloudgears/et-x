/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#pragma once

#include <et/apiobjects/texture.h>
#include <et/app/events.h>
#include <et-ext/scene2d/fontbase.h>

namespace et
{
	class RenderContext;
	
	namespace s2d
	{
		class CharacterGeneratorPrivate;
		class CharacterGenerator : public Object
		{
		public:
			ET_DECLARE_POINTER(CharacterGenerator)

		public:
			CharacterGenerator(RenderContext* _rc, const std::string& face,
				const std::string& boldFace, size_t size, size_t textureSize = 1024);
			
			~CharacterGenerator();
			
			const Texture& texture() const 
				{ return _texture; }

			size_t size() const
				{ return _size; }

			const std::string& face() const
				{ return _face; }
			
			size_t charactersCount() const
				{ return _chars.size() + _boldChars.size(); }

			CharDescriptor charDescription(int c)
			{
				auto i = _chars.find(c);
				return (i != _chars.end()) ? i->second : generateCharacter(c, true);
			}

			CharDescriptor boldCharDescription(int c)
			{
				auto i = _boldChars.find(c);
				return (i != _boldChars.end()) ? i->second : generateBoldCharacter(c, true);
			}
			
			const CharDescriptorMap& characters() const
				{ return _chars; }

			const CharDescriptorMap& boldCharacters() const
				{ return _boldChars; }
			
			float lineHeight() const
				{ return _chars.size() ? _chars.begin()->second.size.y : static_cast<float>(_size); }
			
			void setTexture(Texture);
			void pushCharacter(const CharDescriptor&);
			
			ET_DECLARE_EVENT1(characterGenerated, int)

		private:
			CharDescriptor generateCharacter(int value, bool updateTexture);
			CharDescriptor generateBoldCharacter(int value, bool updateTexture);

		private:
			RenderContext* _rc;
			CharacterGeneratorPrivate* _private;

			Texture _texture;
			CharDescriptorMap _chars;
			CharDescriptorMap _boldChars;

			std::string _face;
			std::string _boldFace;

			size_t _size;
		};

	}

}
