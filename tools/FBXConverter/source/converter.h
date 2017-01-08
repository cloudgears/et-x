#include <et/app/application.h>
#include <et/scene3d/scene3d.h>
#include <et/scene3d/scene3drenderer.h>
#include <et/core/interpolationvalue.h>
#include <et/input/gestures.h>
#include <et/camera/cameramovingcontroller.h>
#include "ui/mainlayout.h"

namespace et
{
class Converter : public et::IApplicationDelegate, public et::EventReceiver
{
private:
	et::ApplicationIdentifier applicationIdentifier() const;

	void setApplicationParameters(et::ApplicationParameters&);
	void setRenderContextParameters(et::RenderContextParameters&);
	void applicationDidLoad(et::RenderContext*);
	void render(et::RenderContext*);

private:
	void onBtnOpenClick(et::s2d::Button*);
	void onBtnSaveClick(et::s2d::Button*);
	void performLoading(std::string);
	void performBinarySaving(std::string);
	void performBinaryWithReadableMaterialsSaving(std::string);

	void renderMeshList(et::RenderContext* rc, const et::s3d::BaseElement::List& meshes);
	void renderSkeleton(et::RenderContext* rc, const et::s3d::BaseElement::List& bones);
	void performSceneRendering();

	void printStructureRecursive(et::s3d::BaseElement::Pointer, const std::string&);

private:
	et::RenderContext* _rc = nullptr;
	et::ObjectsCache _texCache;
	et::GesturesRecognizer _gestures;
	et::s3d::Scene::Pointer _scene;
	et::s3d::Renderer::Pointer _sceneRenderer;
	et::CameraMovingController::Pointer _cameraController;

	et::s2d::Scene::Pointer _gui;
	MainLayout::Pointer _mainLayout;
};
}