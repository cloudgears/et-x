#pragma once

#include <et/app/application.h>
#include <et/input/gestures.h>
#include <et-ext/atmosphere/atmosphere.h>
#include <et-ext/scene2d/scene.h>
#include "sample/sample.h"
#include "ui/MainLayout.h"

namespace demo
{
	class MainController : public et::IApplicationDelegate, public et::InputHandler
	{
	private:
		et::ApplicationIdentifier applicationIdentifier() const;

		void setRenderContextParameters(et::RenderContextParameters&);

		void applicationDidLoad(et::RenderContext*);
		void applicationWillTerminate();

		void applicationWillResizeContext(const et::vec2i&);
		
		void applicationWillActivate();
		void applicationWillDeactivate();

		void render(et::RenderContext*);
		void idle(float);

		void onDrag(et::vec2, et::PointerType);
		void onScroll(et::vec2, et::PointerOrigin);
		void onZoom(float);
		void onCharactersEntered(std::string chars);
		
	private:
		void blurCubemap(et::RenderContext*);
				
	private:
		et::s2d::Scene::Pointer _gui;
		et::ObjectsCache _mainTextureCache;
		et::GesturesRecognizer _gestures;
		et::VertexArrayObject _sphere;
		et::Atmosphere::Pointer _atmosphere;
		et::Framebuffer::Pointer _environmentBuffer;
		et::Framebuffer::Pointer _environmentBlurredBuffer[3];
		et::Program::Pointer _environmentBlurProgram;
		et::Texture _noiseTexture;
		
		Sample _sample;
		MainLayout::Pointer _mainLayout;
		bool _pointerCaptured = false;
	};
}
