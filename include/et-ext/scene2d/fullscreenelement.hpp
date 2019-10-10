/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.hpp>

namespace et {
namespace s2d {
class FullscreenElement : public Element2d {
 public:
  ET_DECLARE_POINTER(FullscreenElement);

 public:
  FullscreenElement(Element2d* parent, const std::string& name = emptyString);
};
}  // namespace s2d
}  // namespace et
