//
//  MainUI.h
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include "ResourceManager.h"

namespace emb
{
	class MainUI : public et::s2d::Layout
	{
	public:
		ET_DECLARE_POINTER(MainUI)
		
	public:
		MainUI(ResourceManager&);
		
		void setImage(const et::s2d::Image&);
		
	public:
		ET_DECLARE_EVENT1(fileSelected, std::string)
		
	private:
		et::s2d::ImageView::Pointer _imageView;
	};
}