#include <et/app/application.h>
#include <et/scene3d/scene3d.h>
#include <et/timers/interpolationvalue.h>
#include <et/input/gestures.h>
#include <et/camera/cameramovingcontroller.h>
#include <et-ext/scene2d/scene.h>

namespace fbxc
{
	class MainLayout : public et::s2d::Layout
	{
	public:
		ET_DECLARE_POINTER(MainLayout)
	};

	class Converter : public et::IApplicationDelegate, public et::EventReceiver
	{
	public:
		Converter();

	private:
		et::ApplicationIdentifier applicationIdentifier() const;

		void setRenderContextParameters(et::RenderContextParameters&);
		void applicationDidLoad(et::RenderContext*);
		void render(et::RenderContext*);

	private:
		void buildSupportMeshes(et::RenderContext*);

		void onPointerPressed(et::PointerInputInfo);
		void onPointerMoved(et::PointerInputInfo);
		void onPointerReleased(et::PointerInputInfo);
		void onZoom(float);
		void onDrag(const et::GesturesRecognizer::DragGesture& gest);
		void onScroll(et::vec2, et::PointerOrigin);
		void onCameraUpdated();

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
		et::RenderContext* _rc;
		et::ObjectsCache _texCache;
		et::GesturesRecognizer _gestures;
		et::s2d::Scene::Pointer _gui;
		et::s3d::Scene::Pointer _scene;
		et::Program::Pointer _skinnedProgram;
		et::Program::Pointer _transformedProgram;
		
		et::Camera _camera;
		et::CameraMovingController _cameraController;

		MainLayout::Pointer _mainLayout;

		et::s2d::Font::Pointer _mainFont;
		et::s2d::Label::Pointer _labStatus;
		et::s2d::Button::Pointer _btnDrawNormalMeshes;
		et::s2d::Button::Pointer _btnDrawSupportMeshes;
		et::s2d::Button::Pointer _btnWireframe;

		et::InterpolationValue<float> _vDistance;
		et::InterpolationValue<et::vec2> _vAngle;
		
		et::VertexArrayObject _sphereVao;
		et::VertexArrayObject _lineVao;
		et::VertexArrayObject _testVao;
	};
}