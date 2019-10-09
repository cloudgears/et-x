//
//  ResourceManager.cpp
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include "ResourceManager.h"

#include <et/app/application.h>

namespace et {

void ResourceManager::load(RenderContext* rc) {
  _rc = rc;
  s2d::CharacterGenerator::Pointer commonFont = s2d::CharacterGenerator::Pointer::create(rc, "Tahoma", "Tahoma");
  fonts.main = s2d::Font::Pointer::create(commonFont);
}

s2d::Label::Pointer ResourceManager::label(const std::string& text, s2d::Element2d* parent) {
  auto result = s2d::Label::Pointer::create(text, fonts.main, 24.0f, parent);
  return result;
}

s2d::Button::Pointer ResourceManager::button(const std::string& text, s2d::Element2d* parent) {
  auto result = s2d::Button::Pointer::create(text, fonts.main, 18.0f, parent);
  result->setTextColor(vec4(1.0f));
  result->setTextPressedColor(vec4(0.5f, 0.5f, 0.5f, 1.0f));
  result->setBackgroundColor(vec4(0.1f, 0.2f, 0.3f, 1.0f));
  result->setAutolayoutMask(s2d::LayoutMask_NoSize);
  result->adjustSize();
  return result;
}

}  // namespace et
