//
//  MainLayout.cpp
//  demo
//
//  Created by Sergey Reznik on 28/11/2014.
//  Copyright (c) 2014 Sergey Reznik. All rights reserved.
//

#include "MainLayout.h"

using namespace et;
using namespace demo;

MainLayout::MainLayout() {
  s2d::Slider::Pointer timeSlider = s2d::Slider::Pointer::create(this);
  timeSlider->setValue(0.5f);
  timeSlider->setAutolayoutRelativeToParent(vec2(0.0f), vec2(1.0f, 0.05f), vec2(0.0f));
  timeSlider->valueChanged.connect([this](float t) { timeChanged.invoke(t); });
}
