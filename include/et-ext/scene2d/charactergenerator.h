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
			CharacterGenerator(RenderContext* _rc, const std::string& face, const std::string& boldFace);
			~CharacterGenerator();
			
			const Texture& texture() const 
				{ return _texture; }

			size_t baseSize() const;
			
			const std::string& face() const;
			const std::string& boldFace() const;
			
			size_t charactersCount() const
				{ return _chars.size() + _boldChars.size(); }

			CharDescriptor charDescription(int c)
			{
				auto i = _chars.find(c);
				return (i != _chars.end()) ? i->second : generateCharacter(c, CharacterFlag_Default);
			}

			CharDescriptor boldCharDescription(int c)
			{
				auto i = _boldChars.find(c);
				return (i != _boldChars.end()) ? i->second : generateCharacter(c, CharacterFlag_Bold);
			}
			
			const CharDescriptorMap& characters() const
				{ return _chars; }

			const CharDescriptorMap& boldCharacters() const
				{ return _boldChars; }
						
			void setTexture(Texture);
			void pushCharacter(const CharDescriptor&);
			
			ET_DECLARE_EVENT1(characterGenerated, int)

		private:
			CharDescriptor generateCharacter(int value, CharacterFlags);

		private:
			ET_DECLARE_PIMPL(CharacterGenerator, 128)
			
			RenderContext* _rc = nullptr;
			
			Texture _texture;
			CharDescriptorMap _chars;
			CharDescriptorMap _boldChars;
		};

	}

}
