//
//  ResourceManager.h
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et-ext/scene2d/scene.h>

namespace emb
{
	class ResourceManager
	{
	public:
		void load(et::RenderContext*);
		void cacheFonts();
		
		et::s2d::Label::Pointer label(const std::string&, et::s2d::Element2d*);
		et::s2d::Button::Pointer button(const std::string&, et::s2d::Element2d*);
		
	private:
		struct
		{
			et::s2d::Font::Pointer buttonsFont;
			et::s2d::Font::Pointer labelsFont;
		}fonts;
		
	private:
		et::RenderContext* _rc = nullptr;
	};
}