//
//  MainUI.cpp
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/platform/platformtools.h>
#include "MainUI.h"

using namespace et;
using namespace emb;

MainUI::MainUI(ResourceManager& rm)
{
	_imageView = s2d::ImageView::Pointer::create(this);
	_imageView->fillParent();
	_imageView->setContentMode(s2d::ImageView::ContentMode_Fit);
	
	auto b = rm.button("Load", this);
	b->setLocationInParent(s2d::Location_TopLeft);
	b->clicked.connect([this](s2d::Button*)
	{
		auto fileName = selectFile(StringList(), SelectFileMode_Open, std::string());
		if (!fileName.empty() && fileExists(fileName))
			fileSelected.invokeInMainRunLoop(fileName);
	});
	
}

void MainUI::setImage(const et::s2d::Image& img)
{
	_imageView->setImage(img);
}