/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#pragma once

#include <et/apiobjects/texture.h>
#include <et/app/events.h>
#include <et/geometry/rectplacer.h>
#include <et-ext/scene2d/fontbase.h>

namespace et
{
	class RenderContext;
	
	namespace s2d
	{
		namespace sdf
		{
			struct Point
			{
				int dx = 0;
				int dy = 0;
				int f = 0;
				
				Point(int x, int y, int af) :
					dx(x), dy(y), f(af) { }
			};

			struct Grid
			{
				int w = 0;
				int h = 0;
				DataStorage<Point> grid;
			};
		}
		
		class CharacterGeneratorImplementationPrivate;
		class CharacterGeneratorImplementation
		{
		public:
			CharacterGeneratorImplementation(const std::string&, const std::string&);
			~CharacterGeneratorImplementation();
			
		public:
			bool processCharacter(const CharDescriptor&, vec2i&, vec2i&, BinaryDataStorage&);
			
		private:
			ET_DECLARE_PIMPL(CharacterGeneratorImplementation, 192)
		};
		
		class CharacterGenerator : public Object
		{
		public:
			ET_DECLARE_POINTER(CharacterGenerator)

			static const float baseFontSize;
			static const vec2i charactersRenderingExtent;
			
		public:
			CharacterGenerator(RenderContext* _rc, const std::string& face, const std::string& boldFace);
			
			const Texture& texture() const
				{ return _texture; }

			const std::string& face() const
				{ return _fontFace; }
			
			const std::string& boldFace() const
				{ return _fontBoldFace; }

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
			CharDescriptor generateCharacter(int, CharacterFlags);
			
			void generateSignedDistanceField(BinaryDataStorage&, int, int);
			void generateSignedDistanceFieldOnGrid(sdf::Grid&);
			
			bool performCropping(const BinaryDataStorage&, const vec2i&, BinaryDataStorage&, vec2i&, vec2i&);
			
			void updateTexture(const vec2i&, const vec2i&, BinaryDataStorage&);
			
			BinaryDataStorage downsample(BinaryDataStorage&, const vec2i&);

		private:
			CharacterGeneratorImplementation _impl;
			
			RenderContext* _rc = nullptr;
			
			et::Texture _texture;
			std::string _fontFace;
			std::string _fontBoldFace;
			RectPlacer _placer;
			
			sdf::Grid _grid0;
			sdf::Grid _grid1;
			
			CharDescriptorMap _chars;
			CharDescriptorMap _boldChars;
		};
	}
}
