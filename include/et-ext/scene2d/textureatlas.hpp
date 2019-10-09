/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene2d/element2d.hpp>

namespace et {
namespace s2d {
class TextureAtlas {
 public:
  TextureAtlas() = default;
  TextureAtlas(RenderInterface::Pointer& rc, const std::string& filename, ObjectsCache& cache);

  bool loaded() const {
    return _loaded;
  }

  void loadFromFile(RenderInterface::Pointer& rc, const std::string& filename, ObjectsCache& cache, bool async = false);
  void unload();

  bool hasImage(const std::string& key) const;

  const s2d::Image& image(const std::string& key) const;

  std::vector<Image> imagesForTexture(const Texture::Pointer& t) const;

  const Texture::Pointer& firstTexture() const;

 private:
  typedef std::map<std::string, Texture::Pointer> TextureMap;
  typedef std::map<std::string, Image> ImageMap;

  TextureMap _textures;
  ImageMap _images;

  bool _loaded = false;
};
}  // namespace s2d
}  // namespace et
