//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include "MainController.h"

#include <et/core/conversion.h>
#include <et/core/et.h>
#include <et/core/hardware.h>
#include <et/core/json.h>
#include <et/input/input.h>
#include <et/rendering/rendercontext.h>

namespace et {
namespace demo {

void MainController::setApplicationParameters(ApplicationParameters& params) {
  vec2i contextSize = 4 * currentScreen().availableFrame.size() / 5;

  auto optionsFileName = "data/options.json";
  if (!fileExists(optionsFileName)) optionsFileName = "../data/options.json";
  if (!fileExists(optionsFileName)) optionsFileName = "../../data/options.json";

  if (fileExists(optionsFileName)) {
    VariantClass vc = VariantClass::Invalid;
    auto obj = json::deserialize(loadTextFile(optionsFileName), vc);
    if (vc == VariantClass::Dictionary) {
      Dictionary options = obj;
      if (options.hasKey("context-size")) {
        contextSize = arrayToVec2i(options.arrayForKey("context-size"));
      }
    }
  }

  params.context.size = contextSize;
}

void MainController::setRenderContextParameters(RenderContextParameters& params) {
  params.multisamplingQuality = MultisamplingQuality::None;
  params.swapInterval = 0;
}

void MainController::applicationDidLoad(RenderContext* rc) {
#if (ET_PLATFORM_WIN)
  application().pushRelativeSearchPath("..");
  application().pushRelativeSearchPath("..\\..");
  application().pushRelativeSearchPath("..\\..\\..");
  application().pushRelativeSearchPath("..\\data");
  application().pushRelativeSearchPath("..\\..\\data");
  application().pushRelativeSearchPath("..\\..\\..\\data");
  application().pushSearchPath("Q:\\SDK\\Models\\");
  application().pushSearchPath("Q:\\SDK\\");
  application().setFrameRateLimit(0);
#elif (ET_PLATFORM_MAC)
  application().pushSearchPath("/Volumes/Development/SDK/");
  application().pushSearchPath("/Volumes/Development/SDK/Models/");
#endif

  /*
  rc->renderState().setClearColor(vec4(0.25f, 0.0f));
  rc->renderingInfoUpdated.connect([this](const RenderingInfo& info) {
    log::info("Rendering stats: %lu fps, %lu polys, %lu draw calls", info.averageFramePerSecond,
      info.averagePolygonsPerSecond, info.averageDIPPerSecond);
  });
  */

  _loader.init(rc);
  _renderer.init(rc);
  _cameraController.init(rc);

  auto loadedScene = _loader.loadFromFile(application().resolveFileName("models/sibenik/sibenik-updated.obj"));
  _renderer.setScene(loadedScene);

  _mainUI = MainUI::Pointer::create(rc);

  _ui = s2d::Scene::Pointer::create(rc);
  _ui->pushLayout(_mainUI);

  connectInputEvents();
}

void MainController::connectInputEvents() {
  input().keyPressed.connect([this](size_t key) {
    _cameraController.handlePressedKey(key);
    _renderer.handlePressedKey(key);
  });

  _gestures.drag.connect([this](const GesturesRecognizer::DragGesture& gesture) {
    if (!_uiCaptured) _cameraController.handlePointerDrag(gesture.delta);
  });

  input().keyReleased.connect([this](size_t key) { _cameraController.handleReleasedKey(key); });

  input().pointerPressed.connect([this](PointerInputInfo p) { _uiCaptured = _ui->pointerPressed(p); });

  input().pointerMoved.connect([this](PointerInputInfo p) { _ui->pointerMoved(p); });

  input().pointerReleased.connect([this](PointerInputInfo p) {
    _ui->pointerReleased(p);
    _uiCaptured = false;
  });

  input().pointerCancelled.connect([this](PointerInputInfo p) {
    _ui->pointerCancelled(p);
    _uiCaptured = false;
  });
}

void MainController::applicationWillResizeContext(const vec2i& sz) {
  _cameraController.adjustCameraToNextContextSize(vector2ToFloat(sz));
  _ui->layout(vector2ToFloat(sz));
}

void MainController::render(RenderContext* rc) {
  _renderer.render(_cameraController.camera(), _mainUI->aoParameters(), false);
  _ui->render(rc);
}

ApplicationIdentifier MainController::applicationIdentifier() const {
  return ApplicationIdentifier("com.cheetek.aodemo", "Cheetek", "Ambient Occlusion Demo");
}

}  // namespace demo

IApplicationDelegate* Application::initApplicationDelegate() {
  return sharedObjectFactory().createObject<demo::MainController>();
}

}  // namespace et
