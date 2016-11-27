//
//  ResourceManager.h
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et-ext/scene2d/scene.h>

namespace et
{
class ResourceManager
{
public:
	void load(RenderContext*);

	s2d::Label::Pointer label(const std::string&, s2d::Element2d*);
	s2d::Button::Pointer button(const std::string&, s2d::Element2d*);

private:
	struct
	{
		s2d::Font::Pointer main;
	} fonts;

private:
	RenderContext* _rc = nullptr;
};
}