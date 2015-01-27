#pragma once

#include <et/app/application.h>
#include <et/input/gestures.h>
#include <et/timers/interpolationvalue.h>
#include "ui/MainUI.h"

namespace emb
{
	class MainController : public et::IApplicationDelegate, public et::InputHandler
	{
	private:
		et::ApplicationIdentifier applicationIdentifier() const;

		void setApplicationParameters(et::ApplicationParameters&);
		void setRenderContextParameters(et::RenderContextParameters&);
		void applicationDidLoad(et::RenderContext*);
		void applicationWillResizeContext(const et::vec2i&);
		
		void render(et::RenderContext*);
		void renderPreview();
		
		void processImage_Pass1();
		void processImage_Pass2();
		
		void loadPrograms();
		void loadGeometry();
		
		void onDrag(et::vec2, et::PointerType);
		
		void updateCamera();
		
	private:
		struct
		{
			et::Program::Pointer gaussianBlur;
			et::Program::Pointer preview;
			et::Program::Pointer cubemap;
		} programs;

	private:
		et::RenderContext* _rc = nullptr;
		et::GesturesRecognizer _gestures;
		et::s2d::Scene::Pointer _ui;
		et::Texture::Pointer _loadedTexture;
		et::Framebuffer::Pointer _framebuffer;
		et::Framebuffer::Pointer _cubemapFramebuffer;
		et::VertexArrayObject _vaoSphere;
		et::InterpolationValue<et::vec2> _cameraAngles;
		et::Camera _camera;
		
		std::vector<std::pair<et::vec2, et::vec2>> _offsetAndScales;
		
		ResourceManager _rm;
		MainUI::Pointer _mainUi;
		
		std::string _fileToSave;
		size_t _processingSample = 0;
		
		bool _shouldProcessPass1 = false;
		bool _shouldProcessPass2 = false;
		bool _shouldSaveToFile = false;
	};
}
