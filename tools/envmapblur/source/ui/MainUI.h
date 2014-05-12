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
		
		void setImages(const et::s2d::Image& original, const et::s2d::Image& processed);
		
		float angleValue() const
			{ return _sliderAngle->value(); }

		float exposureValue() const
			{ return _sliderExposure->value(); }
		
	public:
		ET_DECLARE_EVENT1(fileSelected, std::string)
		ET_DECLARE_EVENT0(processSelected)
		ET_DECLARE_EVENT1(saveSelected, std::string)
		
	private:
		et::s2d::ImageView::Pointer _imageViewOriginal;
		et::s2d::ImageView::Pointer _imageViewProcessed;
		
		et::s2d::Slider::Pointer _sliderAngle;
		et::s2d::Slider::Pointer _sliderExposure;
	};
}