//
//  MainLayout.h
//  demo
//
//  Created by Sergey Reznik on 28/11/2014.
//  Copyright (c) 2014 Sergey Reznik. All rights reserved.
//

#pragma once

#include <et-ext/scene2d/scene.h>

namespace demo
{
	class MainLayout : public et::s2d::Layout
	{
	public:
		ET_DECLARE_POINTER(MainLayout)
		
		ET_DECLARE_EVENT1(timeChanged, float)
		
	public:
		MainLayout();
	};
		
}
