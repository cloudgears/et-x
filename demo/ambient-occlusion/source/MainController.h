//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et/app/application.h>
#include <et/input/gestures.h>

#include "renderer/DemoSceneRenderer.h"
#include "renderer/DemoSceneLoader.h"
#include "renderer/DemoCameraController.h"

#include "ui/MainUI.h"

namespace demo
{
	class MainController : public et::IApplicationDelegate
	{
	private:
		et::ApplicationIdentifier applicationIdentifier() const;
		
		void setApplicationParameters(et::ApplicationParameters&);
		void setRenderContextParameters(et::RenderContextParameters&);
		
		void applicationDidLoad(et::RenderContext*);
		void applicationWillResizeContext(const et::vec2i&);
		
		void render(et::RenderContext*);
		void connectInputEvents();
		
	private:
		demo::SceneRenderer _renderer;
		demo::SceneLoader _loader;
		demo::CameraController _cameraController;
		demo::MainUI::Pointer _mainUI;

		et::GesturesRecognizer _gestures;
		et::s2d::Scene::Pointer _ui;

		bool _uiCaptured = false;
	};
}
