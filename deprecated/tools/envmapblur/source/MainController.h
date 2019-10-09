#pragma once

#include <et/app/application.h>
#include <et/core/interpolationvalue.h>
#include <et/input/gestures.h>

#include "ui/MainUI.h"

namespace et {
class MainController : public IApplicationDelegate, public InputHandler {
 private:
  ApplicationIdentifier applicationIdentifier() const;

  void setApplicationParameters(ApplicationParameters&);
  void setRenderContextParameters(RenderContextParameters&);
  void applicationDidLoad(RenderContext*);
  void applicationWillResizeContext(const vec2i&);

  void render(RenderContext*);
  void renderPreview();

  void processImage_Pass1();
  void processImage_Pass2();

  void loadPrograms();
  void loadGeometry();

  void onDrag(const GesturesRecognizer::DragGesture&);

  void updateCamera();

 private:
  struct {
    Program::Pointer gaussianBlur;
    Program::Pointer preview;
    Program::Pointer cubemap;
  } programs;

 private:
  RenderContext* _rc = nullptr;
  RenderPass::Pointer _mainPass;
  Camera::Pointer _camera;

  GesturesRecognizer _gestures;
  s2d::Scene::Pointer _ui;
  Texture::Pointer _loadedTexture;
  VertexStream _vaoSphere;
  InterpolationValue<vec2> _cameraAngles;

  std::vector<std::pair<vec2, vec2>> _offsetAndScales;

  ResourceManager _rm;
  MainUI::Pointer _mainUi;

  std::string _fileToSave;
  size_t _processingSample = 0;

  bool _shouldProcessPass1 = false;
  bool _shouldProcessPass2 = false;
  bool _shouldSaveToFile = false;
};
}  // namespace et
