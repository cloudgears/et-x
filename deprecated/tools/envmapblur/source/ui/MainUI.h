//
//  MainUI.h
//  EnvMapBlur
//
//  Created by Sergey Reznik on 3/2/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include "ResourceManager.h"

namespace et
{
class MainUI : public s2d::Layout
{
public:
	ET_DECLARE_POINTER(MainUI);

public:
	MainUI(ResourceManager&);

	void setImages(const s2d::Image& original, const s2d::Image& processed);

	float angleValue() const
	{
		return _sliderAngle->value();
	}

	float exposureValue() const
	{
		return _sliderExposure->value();
	}

	float expoCorrection() const
	{
		return _sliderExposure->value() > 0.0f ? 1.0f : 0.0f;
	}

public:
	ET_DECLARE_EVENT1(fileSelected, std::string);
	ET_DECLARE_EVENT0(processSelected);
	ET_DECLARE_EVENT1(saveSelected, std::string);

private:
	s2d::ImageView::Pointer _imageViewOriginal;
	s2d::ImageView::Pointer _imageViewProcessed;

	s2d::Slider::Pointer _sliderAngle;
	s2d::Slider::Pointer _sliderExposure;

	NotifyTimer _testTimer;
};
}
