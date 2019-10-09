/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/containers.hpp>
#include <et/geometry/vector4.hpp>
#include <et/geometry/rect.hpp>

#define FONT_VERSION_1 0x0001
#define FONT_VERSION_2 0x0002
#define FONT_VERSION_3 0x0002
#define FONT_VERSION_4 0x0002
#define FONT_VERSION_CURRENT FONT_VERSION_4

namespace et {
namespace s2d {

enum CharacterFlags : uint32_t {
  CharacterFlag_Default = 0x0000,
  CharacterFlag_Bold = 0x0001,
};

struct CharDescriptor {
  uint32_t value = 0;
  uint32_t flags = CharacterFlag_Default;

  vec4 color = vec4(1.0f);
  vec2 originalSize = vec2(0.0f);
  rect contentRect;
  rect uvRect;
  vec4 parameters = vec4(0.0f);

  CharDescriptor() = default;

  CharDescriptor(int v)
    : value(v) {}
};

using CharDescriptorList = Vector<CharDescriptor>;
using CharDescriptorMap = Map<int, CharDescriptor>;

}  // namespace s2d
}  // namespace et
