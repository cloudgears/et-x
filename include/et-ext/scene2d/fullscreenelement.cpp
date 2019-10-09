/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene2d/fullscreenelement.hpp>

namespace et {
namespace s2d {

FullscreenElement::FullscreenElement(Element2d* parent, const std::string& name)
  : Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS) {
  setAutolayout(vec2(0.0f), LayoutMode_Absolute, vec2(1.0f), LayoutMode_RelativeToContext, vec2(0.0f));
}

}  // namespace s2d
}  // namespace et
