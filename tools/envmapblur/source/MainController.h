#pragma once

#include <et/app/application.h>
#include <et/input/gestures.h>
#include "ui/MainUI.h"

namespace emb
{
	class MainController : public et::IApplicationDelegate, public et::InputHandler
	{
	private:
		et::ApplicationIdentifier applicationIdentifier() const;

		void setRenderContextParameters(et::RenderContextParameters&);
		void applicationDidLoad(et::RenderContext*);
		void applicationWillResizeContext(const et::vec2i&);
		void render(et::RenderContext*);

	private:
		et::RenderContext* _rc;
		et::GesturesRecognizer _gestures;
		et::s2d::Scene::Pointer _ui;
		et::Texture _loadedTexture;
		
		ResourceManager _rm;
		MainUI::Pointer _mainUi;
		
	
	};
}
