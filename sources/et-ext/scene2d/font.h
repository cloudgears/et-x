/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/charactergenerator.h>
#include <et/rendering/interface/texture.h>

namespace et {
namespace s2d {
class Font : public Object {
 public:
  ET_DECLARE_POINTER(Font);

 public:
  Font(const CharacterGenerator::Pointer& generator);

  CharacterGenerator::Pointer& generator() {
    return _generator;
  }

  const CharacterGenerator::Pointer& generator() const {
    return _generator;
  }

  bool loadFromDictionary(RenderInterface::Pointer&, const Dictionary&, ObjectsCache&, const std::string&);
  bool loadFromFile(RenderInterface::Pointer&, const std::string&, ObjectsCache&);

  void saveToFile(RenderInterface::Pointer&, const std::string&);

  CharDescriptorList buildString(const std::string&, float, float = 1.0f);
  CharDescriptorList buildString(const std::wstring&, float, float = 1.0f);

  vec2 measureStringSize(const std::string&, float, float = 1.0f);
  vec2 measureStringSize(const std::wstring&, float, float = 1.0f);

  vec2 measureStringSize(const CharDescriptorList&);

 private:
  CharacterGenerator::Pointer _generator;
};
}  // namespace s2d
}  // namespace et
