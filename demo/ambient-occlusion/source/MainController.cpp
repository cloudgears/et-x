//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include <et/input/input.h>
#include <et/core/conversion.h>
#include <et/rendering/rendercontext.h>
#include <et/json/json.h>
#include "MainController.h"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters& params)
{
//	params.windowSize = WindowSize::FillWorkarea;
}

void MainController::setRenderContextParameters(et::RenderContextParameters& params)
{
	vec2i contextSize(1024, 768);

	auto optionsFileName = "data/options.json";
	if (!fileExists(optionsFileName))
		optionsFileName = "../data/options.json";
	if (!fileExists(optionsFileName))
		optionsFileName = "../../data/options.json";

	if (fileExists(optionsFileName))
	{
		ValueClass vc = ValueClass_Invalid;
		auto obj = json::deserialize(loadTextFile(optionsFileName), vc);
		if (vc == ValueClass_Dictionary)
		{
			Dictionary options = obj;
			if (options.hasKey("context-size"))
			{
				contextSize = arrayToVec2i(options.arrayForKey("context-size"));
			}
		}
	}

	params.contextSize = contextSize;
	params.contextBaseSize = params.contextSize;
	params.multisamplingQuality = MultisamplingQuality::None;
	params.swapInterval = 0;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
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

	rc->renderState().setClearColor(vec4(0.25f, 0.0f));
	
	rc->renderingInfoUpdated.connect([this](const et::RenderingInfo& info)
	{
		log::info("Rendering stats: %lu fps, %lu polys, %lu draw calls", info.averageFramePerSecond,
			info.averagePolygonsPerSecond, info.averageDIPPerSecond);
	});
	
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

void MainController::connectInputEvents()
{
	input().keyPressed.connect([this](size_t key)
	{
		_cameraController.handlePressedKey(key);
		_renderer.handlePressedKey(key);
	});
	
	_gestures.drag.connect([this](const GesturesRecognizer::DragGesture& gesture)
	{
		if (!_uiCaptured)
			_cameraController.handlePointerDrag(gesture.delta);
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
	_ui->layout(vector2ToFloat(sz));
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
