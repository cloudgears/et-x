//
//  ResourceManager.cpp
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include "ResourceManager.h"

using namespace et;
using namespace emb;

void ResourceManager::load(et::RenderContext* rc)
{
	fonts.mainFont = s2d::Font::Pointer::create(s2d::CharacterGenerator::Pointer::create(rc,
		"Helvetica", "Helvetica", 16));
}

et::s2d::Label::Pointer ResourceManager::label(const std::string& text, et::s2d::Element2d* parent)
{
	auto result = s2d::Label::Pointer::create(text, fonts.mainFont, parent);
	result->setColor(vec4(1.0f));
	return result;
}

et::s2d::Button::Pointer ResourceManager::button(const std::string& text, et::s2d::Element2d* parent)
{
	auto result = s2d::Button::Pointer::create(text, fonts.mainFont, parent);
	result->setTextColor(vec4(1.0f));
	result->setTextPressedColor(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	result->setBackgroundColor(vec4(0.1f, 0.2f, 0.3f, 1.0f));
	result->setAutolayoutMask(s2d::LayoutMask_NoSize);
	return result;
}