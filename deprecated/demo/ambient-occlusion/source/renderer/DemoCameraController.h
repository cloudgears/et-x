//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et/camera/camera.h>
#include <et/core/notifytimer.h>

namespace et {
namespace demo {

class CameraController {
 public:
  CameraController();

  void init(RenderContext*);

  const Camera& camera() const {
    return _mainCamera;
  }

  Camera& camera() {
    return _mainCamera;
  }

  void adjustCameraToNextContextSize(const vec2&);

  void handlePressedKey(size_t);
  void handleReleasedKey(size_t);

  void handlePointerDrag(const vec2&);

 private:
  Camera _mainCamera;
  NotifyTimer _updateTimer;
  vec2 _movements;
  bool _boostEnabled = false;
};
}  // namespace demo
}  // namespace et
