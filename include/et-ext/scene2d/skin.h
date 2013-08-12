/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

/*
#include <et-ext/scene2d/font.h>
#include <et-ext/scene2d/guibase.h>

namespace et
{
	namespace s2d
	{
		class Skin
		{
		public:
			Skin() { }
			Skin(Font::Pointer font, const Texture& texture) : _font(font), _texture(texture) { }

			inline Texture& texture()
				{ return _texture; }

			inline const Texture& texture() const 
				{ return _texture; }

			inline void setTexture(const Texture& texture)
				{ _texture = texture; }

			inline Font::Pointer font() const
				{ return _font; }

			inline void setFont(Font::Pointer font)
				{ _font = font; }

			inline const ImageDescriptor& viewImageDescriptor() const
				{ return _viewDescriptor; }

			inline void setViewImageDescriptor(const ImageDescriptor& d)
				{ _viewDescriptor = d; }

			inline const ImageDescriptor& buttonImageDescriptor(State s) const
				{ return _buttonDescriptors[s]; }

			inline void setButtonImageDescriptor(State s, const ImageDescriptor& d)
				{ _buttonDescriptors[s] = d; }

			inline const ImageDescriptor& checkboxImageDescriptor(State s) const
				{ return _checkboxDescriptors[s]; }

			void init();

			void serialize(const std::string& file);
			static Skin deserialize(const std::string& file);

		private:
			Font::Pointer _font;
			Texture _texture;
			StaticDataStorage<ImageDescriptor, State_max> _buttonDescriptors;
			StaticDataStorage<ImageDescriptor, State_max> _checkboxDescriptors;
			ImageDescriptor _viewDescriptor;
		};
	}
}

*/