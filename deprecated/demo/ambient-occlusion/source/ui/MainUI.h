//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et-ext/scene2d/scene.h>

#include "../renderer/DemoSceneRenderer.h"

namespace et {
namespace demo {
class MainUI : public et::s2d::Layout {
 public:
  ET_DECLARE_POINTER(MainUI);

 public:
  MainUI(et::RenderContext*);

  const AOParameters& aoParameters() const {
    return _aoParameters;
  }

 private:
  AOParameters _aoParameters;
};
}  // namespace demo
}  // namespace et
