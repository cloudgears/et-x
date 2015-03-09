//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include <et/app/application.h>
#include "MainUI.h"

using namespace et;
using namespace demo;

s2d::Label::Pointer label(const std::string& title, s2d::Font::Pointer font, s2d::Slider* parent, bool isValueLabel)
{
	s2d::Label::Pointer result = s2d::Label::Pointer::create(title, font, 16.0f, parent);

	if (isValueLabel)
	{
		result->setAutolayoutRelativeToParent(vec2(1.0f, 0.0f), vec2(1.0f), vec2(1.0f, 1.0f));
		result->setHorizontalAlignment(s2d::Alignment_Far);
	}
	else
	{
		result->setHorizontalAlignment(s2d::Alignment_Near);
		result->setAutolayoutRelativeToParent(vec2(0.0f), vec2(1.0f), vec2(0.0f, 1.0f));
	}

	result->setTextStyle(s2d::TextElement::TextStyle_SignedDistanceFieldShadow);
	result->setVerticalAlignment(s2d::Alignment_Center);

	return result;
}

MainUI::MainUI(et::RenderContext* rc)
{
	ObjectsCache localCache;
	auto generator = s2d::CharacterGenerator::Pointer::create(rc, "Tahoma", "Tahoma");
	auto mainFont = s2d::Font::Pointer::create(generator);
	mainFont->loadFromFile(rc, application().resolveFileName("data/caches/tahoma.font"), localCache);

	auto slidePower = s2d::Slider::Pointer::create(this);
	slidePower->setRange(0.01f, 12.0f);
	slidePower->setValue(_aoParameters.aoPower);
	slidePower->setAutolayoutRelativeToParent(vec2(0.025f, 0.1f), vec2(0.175f, 0.033333f), vec2(0.0f));
	label("AO power", mainFont, slidePower.ptr(), false);
	label(floatToStr(_aoParameters.aoPower, 4), mainFont, slidePower.ptr(), true);
	slidePower->changed.connect([this](s2d::Slider* slider)
	{
		s2d::Label::Pointer(slider->children().back())->setText(floatToStr(slider->value(), 4));
		_aoParameters.aoPower = slider->value();
	});

	auto slideMinScale = s2d::Slider::Pointer::create(this);
	slideMinScale->setRange(0.0f, 5.0f);
	slideMinScale->setValue(_aoParameters.sampleScale.x);
	slideMinScale->setAutolayoutRelativeToParent(vec2(0.025f, 0.2f), vec2(0.175f, 0.033333f), vec2(0.0f));
	label("MIN sample size", mainFont, slideMinScale.ptr(), false);
	label(floatToStr(_aoParameters.sampleScale.x, 4), mainFont, slideMinScale.ptr(), true);
	slideMinScale->changed.connect([this](s2d::Slider* slider)
	{
		s2d::Label::Pointer(slider->children().back())->setText(floatToStr(slider->value(), 4));
		_aoParameters.sampleScale.x = slider->value();
	});

	auto slideMaxScale = s2d::Slider::Pointer::create(this);
	slideMaxScale->setRange(0.0f, 5.0f);
	slideMaxScale->setValue(_aoParameters.sampleScale.y);
	slideMaxScale->setAutolayoutRelativeToParent(vec2(0.025f, 0.3f), vec2(0.175f, 0.033333f), vec2(0.0f));
	label("MAX sample size", mainFont, slideMaxScale.ptr(), false);
	label(floatToStr(_aoParameters.sampleScale.y, 4), mainFont, slideMaxScale.ptr(), true);
	slideMaxScale->changed.connect([this](s2d::Slider* slider)
	{
		s2d::Label::Pointer(slider->children().back())->setText(floatToStr(slider->value(), 4));
		_aoParameters.sampleScale.y = slider->value();
	});

	auto slideDepthDifference = s2d::Slider::Pointer::create(this);
	slideDepthDifference->setRange(0.0f, 10.0f);
	slideDepthDifference->setValue(_aoParameters.depthDifference);
	slideDepthDifference->setAutolayoutRelativeToParent(vec2(0.025f, 0.4f), vec2(0.175f, 0.033333f), vec2(0.0f));
	label("Depth scale", mainFont, slideDepthDifference.ptr(), false);
	label(floatToStr(_aoParameters.depthDifference, 4), mainFont, slideDepthDifference.ptr(), true);
	slideDepthDifference->changed.connect([this](s2d::Slider* slider)
	{
		s2d::Label::Pointer(slider->children().back())->setText(floatToStr(slider->value(), 4));
		_aoParameters.depthDifference = slider->value();
	});

	auto sliderNumSamples = s2d::Slider::Pointer::create(this);
	sliderNumSamples->setRange(1.0f, 128.0f);
	sliderNumSamples->setValue(static_cast<float>(_aoParameters.numSamples));
	sliderNumSamples->setAutolayoutRelativeToParent(vec2(0.025f, 0.5f), vec2(0.175f, 0.033333f), vec2(0.0f));
	label("Num samples", mainFont, sliderNumSamples.ptr(), false);
	label(intToStr(_aoParameters.numSamples), mainFont, sliderNumSamples.ptr(), true);
	sliderNumSamples->changed.connect([this](s2d::Slider* slider)
	{
		int intValue = static_cast<int>(slider->value() + 0.5f);
		slider->setValue(static_cast<float>(intValue));

		s2d::Label::Pointer(slider->children().back())->setText(intToStr(intValue));
		_aoParameters.numSamples = intValue;
	});

	auto labFPS = s2d::Label::Pointer::create("FPS", mainFont, 32.0f, this);
	labFPS->setLocationInParent(s2d::Location_TopRight);
	labFPS->setTextStyle(s2d::TextElement::TextStyle_SignedDistanceFieldShadow);
	auto labFPSPointer = labFPS.ptr();
	rc->renderingInfoUpdated.connect([this, labFPSPointer](const RenderingInfo& info)
	{
		labFPSPointer->setText(intToStr(info.averageFramePerSecond) + " ");
	});

	auto buttonDiffuse = s2d::Button::Pointer::create("Toggle diffuse maps", mainFont, 16.0f, this);
	buttonDiffuse->setBackgroundColor(vec4(1.0f, 0.5f, 0.25f, 0.85f));
	buttonDiffuse->setAutolayoutRelativeToParent(vec2(0.025f, 0.6f), vec2(0.175f, 0.05f), vec2(0.0f));
	buttonDiffuse->clicked.connect([this](s2d::Button*)
	{
		_aoParameters.showDiffuse = !_aoParameters.showDiffuse;
	});

	auto buttonBlur = s2d::Button::Pointer::create("Toggle blur", mainFont, 16.0f, this);
	buttonBlur->setBackgroundColor(vec4(1.0f, 0.5f, 0.25f, 0.85f));
	buttonBlur->setAutolayoutRelativeToParent(vec2(0.025f, 0.7f), vec2(0.175f, 0.05f), vec2(0.0f));
	buttonBlur->clicked.connect([this](s2d::Button*)
	{
		_aoParameters.performBlur = !_aoParameters.performBlur;
	});

}
