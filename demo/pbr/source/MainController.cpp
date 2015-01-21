//
//  MainController.cpp
//  PBR
//
//  Created by Sergey Reznik on 21/1/2015.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include "MainController.h"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters&)
{
	
}

void MainController::setRenderContextParameters(et::RenderContextParameters&)
{
	
}

void MainController::applicationDidLoad(et::RenderContext*)
{
	
}

void MainController::applicationWillTerminate()
{
	
}

void MainController::applicationWillResizeContext(const et::vec2i&)
{
	
}

void MainController::render(et::RenderContext*)
{
	
}

IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "PBR"); }
