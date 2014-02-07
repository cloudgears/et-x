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
	_imageViewOriginal = s2d::ImageView::Pointer::create(this);
	_imageViewOriginal->setAutolayoutRelativeToParent(vec2(0.0f), vec2(0.5f, 0.5f), vec2(0.0f));
	_imageViewOriginal->setContentMode(s2d::ImageView::ContentMode_Fit);
	_imageViewOriginal->setBackgroundColor(vec4(0.0f, 0.0f, 1.0f, 0.25f));

	_imageViewProcessed = s2d::ImageView::Pointer::create(this);
	_imageViewProcessed->setAutolayoutRelativeToParent(vec2(0.0f, 0.5f), vec2(0.5f, 0.5f), vec2(0.0f));
	_imageViewProcessed->setContentMode(s2d::ImageView::ContentMode_Fit);
	_imageViewProcessed->setBackgroundColor(vec4(0.0f, 1.0f, 0.0f, 0.25f));
	
	auto b = rm.button("Load", this);
	b->setLocationInParent(s2d::Location_TopLeft);
	b->clicked.connect([this](s2d::Button*)
	{
		auto fileName = selectFile(StringList(), SelectFileMode_Open, std::string());
		if (!fileName.empty() && fileExists(fileName))
			fileSelected.invokeInMainRunLoop(fileName);
	});

	b = rm.button("Process", this);
	b->setAutolayoutRelativeToParent(vec2(0.5f, 0.0f), vec2(0.0f), vec2(1.0f, 0.0f));
	b->clicked.connect([this](s2d::Button*)
	{
		processSelected.invokeInMainRunLoop();
	});
	
	_sliderAngle = s2d::Slider::Pointer::create(this);
	_sliderAngle->setAutolayoutRelativeToParent(vec2(0.75f, 0.0f), vec2(0.5f, 0.05f), vec2(0.5f, 0.0));
	_sliderAngle->setSliderFillColors(vec4(0.25f, 1.0f, 0.5f, 1.0f), vec4(2.0f / 3.0f));
	_sliderAngle->setRange(0.0f, HALF_PI);
	_sliderAngle->setValue(5.0f * DEG_1);
	_sliderAngle->changed.connect([this](s2d::Slider* slider)
	{
		s2d::Label::Pointer lab = slider->children().front();
		lab->setText(floatToStr(slider->value() * TO_DEGREES, 2) + "º");
	});
	
	auto lab = rm.label("5.00º", _sliderAngle.ptr());
	lab->setAutolayoutRelativeToParent(vec2(0.5f, 1.0f), vec2(0.0f), vec2(0.5f, 0.0f));
	
	_sliderExposure = s2d::Slider::Pointer::create(this);
	_sliderExposure->setAutolayoutRelativeToParent(vec2(0.75f, 1.0f), vec2(0.5f, 0.05f), vec2(0.5f, 1.0f));
	_sliderExposure->setSliderFillColors(vec4(0.25f, 1.0f, 0.5f, 1.0f), vec4(2.0f / 3.0f));
	_sliderExposure->setRange(0.01f, 4.0f);
	_sliderExposure->setValue(1.0f);
	_sliderExposure->changed.connect([this](s2d::Slider* slider)
	{
		s2d::Label::Pointer lab = slider->children().front();
		lab->setText(floatToStr(slider->value(), 2));
	});
	
	lab = rm.label("1.00", _sliderExposure.ptr());
	lab->setAutolayoutRelativeToParent(vec2(0.5f, 0.0f), vec2(0.0f), vec2(0.5f, 1.0f));
}

void MainUI::setImages(const et::s2d::Image& original, const et::s2d::Image& processed)
{
	_imageViewOriginal->setImage(original);
	_imageViewProcessed->setImage(processed);
}
