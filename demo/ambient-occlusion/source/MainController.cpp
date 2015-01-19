//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include <et/input/input.h>
#include <et/rendering/rendercontext.h>
#include "MainController.h"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters& params)
{
//	params.windowSize = WindowSize::Fullscreen;
}

void MainController::setRenderContextParameters(et::RenderContextParameters& params)
{
	params.contextSize = vec2i(1024, 768);
	params.contextBaseSize = params.contextSize;
	params.multisamplingQuality = MultisamplingQuality_Best;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
#if (ET_PLATFORM_WIN)
	application().pushRelativeSearchPath("..");
	application().pushRelativeSearchPath("..\\..");
	application().pushRelativeSearchPath("..\\..\\..");
	application().pushSearchPath("Q:\\SDK\\Models\\");
	application().pushSearchPath("Q:\\SDK\\");
#elif (ET_PLATFORM_MAC)
	application().pushSearchPath("/Volumes/Development/SDK/");
	application().pushSearchPath("/Volumes/Development/SDK/Models/");
#endif

	rc->renderState().setClearColor(vec4(0.25f, 0.0f));
	
	rc->renderingInfoUpdated.connect([this](const et::RenderingInfo& info)
	{
		log::info("Rendering stats: %lu fps, %lu polys, %lu draw calls", info.averageFramePerSecond,
			info.averagePolygonsPerSecond, info.averageDIPPerSecond);
	});
	
	_loader.init(rc);
	_renderer.init(rc);
	_cameraController.init(rc);
	
	auto loadedScene = _loader.loadFromFile(application().resolveFileName("models/sibenik/sibenik-updated.etm"));
	_renderer.setScene(loadedScene);

	_mainUI = MainUI::Pointer::create(rc);

	_ui = s2d::Scene::Pointer::create(rc);
	_ui->pushLayout(_mainUI);

	connectInputEvents();
}

void MainController::connectInputEvents()
{
	input().keyPressed.connect([this](size_t key)
	{
		_cameraController.handlePressedKey(key);
		_renderer.handlePressedKey(key);
	});
	
	_gestures.drag.connect([this](et::vec2 d, size_t)
	{
		if (!_uiCaptured)
			_cameraController.handlePointerDrag(d);
	});

	input().keyReleased.connect([this](size_t key)
	{
		_cameraController.handleReleasedKey(key);
	});

	input().pointerPressed.connect([this](PointerInputInfo p)
	{
		_uiCaptured = _ui->pointerPressed(p);
	});

	input().pointerMoved.connect([this](PointerInputInfo p)
	{
		_ui->pointerMoved(p);
	});

	input().pointerReleased.connect([this](PointerInputInfo p)
	{
		_ui->pointerReleased(p);
		_uiCaptured = false;
	});

	input().pointerCancelled.connect([this](PointerInputInfo p)
	{
		_ui->pointerCancelled(p);
		_uiCaptured = false;
	});

}

 void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	_cameraController.adjustCameraToNextContextSize(vector2ToFloat(sz));
}

void MainController::render(et::RenderContext* rc)
{
	_renderer.render(_cameraController.camera(), _mainUI->aoParameters(), false);
	_ui->render(rc);
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier("com.cheetek.aodemo", "Cheetek", "Ambient Occlusion Demo"); }
