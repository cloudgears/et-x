//
//  ResourceManager.cpp
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/app/application.h>
#include "ResourceManager.h"

using namespace et;
using namespace emb;

void ResourceManager::load(et::RenderContext* rc)
{
	_rc = rc;
	
	auto commonFont = s2d::CharacterGenerator::Pointer::create(rc, "Helvetica", "Helvetica");
	auto coolFont = s2d::CharacterGenerator::Pointer::create(rc, "Helvetica", "Helvetica");
	
	fonts.buttonsFont = s2d::Font::Pointer::create(coolFont, 18);
	fonts.labelsFont = s2d::Font::Pointer::create(commonFont, 28);
}

et::s2d::Label::Pointer ResourceManager::label(const std::string& text, et::s2d::Element2d* parent)
{
	auto result = s2d::Label::Pointer::create(text, fonts.labelsFont, parent);
	return result;
}

et::s2d::Button::Pointer ResourceManager::button(const std::string& text, et::s2d::Element2d* parent)
{
	auto result = s2d::Button::Pointer::create(text, fonts.buttonsFont, parent);
	result->setTextColor(vec4(1.0f));
	result->setTextPressedColor(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	result->setBackgroundColor(vec4(0.1f, 0.2f, 0.3f, 1.0f));
	result->setAutolayoutMask(s2d::LayoutMask_NoSize);
	return result;
}

void ResourceManager::cacheFonts()
{

}
