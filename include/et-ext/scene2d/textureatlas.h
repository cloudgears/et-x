/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/guibase.h>

namespace et
{
	namespace s2d
	{
		class TextureAtlas
		{
		public:
			TextureAtlas();
			TextureAtlas(RenderContext* rc, const std::string& filename, ObjectsCache& cache);
			
			bool loaded() const
				{ return _loaded; }
			
			void loadFromFile(RenderContext* rc, const std::string& filename, ObjectsCache& cache);
			void unload();
			
			const s2d::Image& image(const std::string& key) const;
			s2d::ImageList imagesForTexture(Texture t) const;
			
			Texture firstTexture() const;
			
		private:
			typedef std::map<std::string, Texture> TextureMap;

			TextureMap _textures;
			s2d::ImageMap _images;
			bool _loaded;
		};
	}
}