#include "maincontroller.h"

using namespace et;
using namespace emb;

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.supportedInterfaceOrientations = InterfaceOrientation_AnyLandscape;

#if (ET_PLATFORM_MAC || ET_PLATFORM_WIN)
	p.contextBaseSize = vec2i(960, 640);
	p.contextSize = p.contextBaseSize;
#endif
	
	_gestures.pointerPressed.connect([this](PointerInputInfo p)
	{
		_ui->pointerPressed(p);
	});
	_gestures.pointerReleased.connect([this](PointerInputInfo p)
	{
		_ui->pointerReleased(p);
	});
	_gestures.pointerMoved.connect([this](PointerInputInfo p)
	{
		_ui->pointerMoved(p);
	});
	_gestures.pointerCancelled.connect([this](PointerInputInfo p)
	{
		_ui->pointerCancelled(p);
	});
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	_rm.load(rc);
	_mainUi = MainUI::Pointer::create(_rm);
	_mainUi->fileSelected.connect([this, rc](std::string name)
	{
		ObjectsCache localCache;
		_loadedTexture = rc->textureFactory().loadTexture(name, localCache);
		_mainUi->setImage(_loadedTexture);
	});
	
	_ui = s2d::Scene::Pointer::create(rc);
	_ui->pushLayout(_mainUi);
}

void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	_ui->layout(vector2ToFloat(sz));
}

void MainController::render(et::RenderContext* rc)
{
	rc->renderer()->clear(true, false);
	_ui->render(rc);
}

ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "EnvMapBlur"); }

IApplicationDelegate* Application::initApplicationDelegate()
	{ return new MainController; }
