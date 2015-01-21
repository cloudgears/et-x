//
//  MainController.h
//  PBR
//
//  Created by Sergey Reznik on 21/1/2015.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et/app/application.h>

namespace demo
{
	class MainController : public et::IApplicationDelegate
	{
		et::ApplicationIdentifier applicationIdentifier() const;
		void setApplicationParameters(et::ApplicationParameters&);
		void setRenderContextParameters(et::RenderContextParameters&);
		void applicationDidLoad(et::RenderContext*);
		void applicationWillTerminate();
		void applicationWillResizeContext(const et::vec2i&);
		void render(et::RenderContext*);
	};
}