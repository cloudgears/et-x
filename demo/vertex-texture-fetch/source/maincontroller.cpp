#include <et/models/objloader.h>
#include <et/primitives/primitives.h>
#include "maincontroller.h"

using namespace et;
using namespace demo;

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.multisamplingQuality = MultisamplingQuality_Best;
	
	p.supportedInterfaceOrientations =
		InterfaceOrientation_Any & (~InterfaceOrientation_PortraitUpsideDown);

#if (ET_PLATFORM_MAC || ET_PLATFORM_WIN)
	p.contextBaseSize = vec2i(800, 600);
	p.contextSize = p.contextBaseSize;
#endif
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	_sample.prepare(rc);

	_gestures.pressed.connect([this](vec2, PointerType)
	{
		_sample.stopCamera();
	});
	
	_gestures.pointerPressed.connect([this](et::PointerInputInfo i)
	{
		_pointerCaptured = _gui->pointerPressed(i);
	});
	
	_gestures.pointerMoved.connect([this](et::PointerInputInfo i)
	{
		_gui->pointerMoved(i);
	});
	
	_gestures.pointerReleased.connect([this](et::PointerInputInfo i)
	{
		_gui->pointerReleased(i);
		_pointerCaptured = false;
	});
	
	_gestures.pointerCancelled.connect([this](et::PointerInputInfo i)
	{
		_gui->pointerCancelled(i);
		_pointerCaptured = false;
	});
	
	ET_CONNECT_EVENT(input().charactersEntered, MainController::onCharactersEntered)
	ET_CONNECT_EVENT(_gestures.drag, MainController::onDrag)
	ET_CONNECT_EVENT(_gestures.zoom, MainController::onZoom)
	ET_CONNECT_EVENT(_gestures.scroll, MainController::onScroll)
	
	rc->renderState().setClearColor(vec4(0.07f, 0.085f, 0.1f, 1.0f));
	
	_mainLayout = MainLayout::Pointer::create();
	_mainLayout->timeChanged.connect([this](float t)
	{
		vec3 n = fromSpherical(-DEG_15 + (PI + DEG_30) * t, QUARTER_PI);
		_sample.setLightPosition(n);
	});
	
	_gui = s2d::Scene::Pointer::create(rc);
	_gui->pushLayout(_mainLayout);
	
	_atmosphere = Atmosphere::Pointer::create(rc);
	
	auto envInternal = GL_RGB;
	auto envFormat = GL_RGB;
	auto envType = GL_UNSIGNED_BYTE;
	
	_environmentBuffer = rc->framebufferFactory().createCubemapFramebuffer(512, "main-env",
		envInternal, envFormat, envType, 0, 0, 0);
	
	_environmentBlurredBuffer[0] = rc->framebufferFactory().createCubemapFramebuffer(256, "ds-1-env",
		envInternal, envFormat, envType, 0, 0, 0);
	
	_environmentBlurredBuffer[1] = rc->framebufferFactory().createCubemapFramebuffer(128, "ds-2-env",
		envInternal, envFormat, envType, 0, 0, 0);
	
	_environmentBlurredBuffer[2] = rc->framebufferFactory().createCubemapFramebuffer(64, "ds-3-env",
		envInternal, envFormat, envType, 0, 0, 0);
	
	ObjectsCache localCache;
	_environmentBlurProgram = rc->programFactory().loadProgram("data/shaders/cubemapblur.program", localCache);
	_environmentBlurProgram->setUniform("environmentMap", 0);
	_environmentBlurProgram->setUniform("noiseMap", 1);
	
	_noiseTexture = rc->textureFactory().genNoiseTexture(vec2i(512), true, "noise-map");
	_noiseTexture->setWrap(rc, TextureWrap_Repeat, TextureWrap_Repeat);
	
	//
	OBJLoader loader(rc, "data/models/bunny.obj");
	auto container = loader.load(localCache, OBJLoader::Option_SupportMeshes);
	s3d::SupportMesh::Pointer mesh = container->childrenOfType(s3d::ElementType_SupportMesh).front();

	if (mesh.valid())
	{
		const auto& tris = mesh->triangles();
		
		VertexDeclaration decl(true, Usage_Position, Type_Vec3);
		decl.push_back(Usage_Normal, Type_Vec3);
		
		VertexArray::Pointer vdata = VertexArray::Pointer::create(decl, 3 * tris.size());
		
		size_t i = 0;
		auto verts = vdata->chunk(Usage_Position).accessData<vec3>(0);
		for (const auto& t : tris)
		{
			verts[i++] = t.v1();
			verts[i++] = t.v2();
			verts[i++] = t.v3();
		}
		IndexArray::Pointer idata = IndexArray::Pointer::create(IndexArrayFormat_32bit, 0, PrimitiveType_Triangles);
		vdata = primitives::buildLinearIndexArray(vdata, idata);
		primitives::calculateNormals(vdata, idata, 0, idata->primitivesCount());
		
		_sample.setModelToDraw(rc->vertexBufferFactory().createVertexArrayObject("model", vdata, BufferDrawType_Static,
			idata, BufferDrawType_Static));
	}
	// */
	
	
	/*/
	{
		VertexDeclaration decl(true, Usage_Position, Type_Vec3);
		decl.push_back(Usage_Normal, Type_Vec3);
		
		vec2i sphereDensity(50);
		float radius = 0.075f;
		
		VertexArray::Pointer vdata = VertexArray::Pointer::create(decl, 0);
		IndexArray::Pointer idata = IndexArray::Pointer::create(IndexArrayFormat_32bit,
			primitives::indexCountForRegularMesh(sphereDensity, PrimitiveType_TriangleStrips), PrimitiveType_TriangleStrips);
		
		primitives::createSphere(vdata, radius, sphereDensity, vec3(0.0f, radius, 0.0f));
		primitives::buildTriangleStripIndexes(idata, sphereDensity, 0, 0);
		
		_sample.setModelToDraw(rc->vertexBufferFactory().createVertexArrayObject("model", vdata,
			BufferDrawType_Static, idata, BufferDrawType_Static));
	}
	// */
}

void MainController::applicationWillTerminate()
{
	
}

void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
}

void MainController::applicationWillActivate()
{

}

void MainController::applicationWillDeactivate()
{
	
}

void MainController::blurCubemap(RenderContext* rc)
{
	static const vec3 lookDirections[] =
	{
		vec3(+1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(0.0f, +1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, 0.0f, +1.0f),
		vec3(0.0f, 0.0f, -1.0f),
	};
	
	static const vec3 upVectors[] =
	{
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, 0.0f, +1.0f),
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
	};
	
	Camera adjustedCam;
	adjustedCam.perspectiveProjection(HALF_PI, 1.0f, 1.0f, 100.0f);

	auto& rs = rc->renderState();
	
	rs.bindTexture(0, _environmentBuffer->renderTarget());
	rs.bindTexture(1, _noiseTexture);
	
	rs.bindProgram(_environmentBlurProgram);
	for (size_t b = 0; b < 3; ++b)
	{
		rs.bindFramebuffer(_environmentBlurredBuffer[b]);
		for (size_t i = 0; i < 6; ++i)
		{
			adjustedCam.lookAt(vec3(0.0f), lookDirections[i], upVectors[i]);
			_environmentBlurredBuffer[b]->setCurrentCubemapFace(i);
			_environmentBlurProgram->setUniform("mModelViewProjectionInverse", adjustedCam.inverseModelViewProjectionMatrix());
			rc->renderer()->fullscreenPass();
		}
		rs.bindTexture(0, _environmentBlurredBuffer[b]->renderTarget());
	}
}

void MainController::render(et::RenderContext* rc)
{
	_atmosphere->setLightDirection(_sample.lightPosition());
	_atmosphere->generateCubemap(_environmentBuffer);
	
	blurCubemap(rc);
	
	rc->renderState().bindDefaultFramebuffer();
	rc->renderState().setDepthMask(true);
	rc->renderState().setColorMask(ColorMask_RGBA);
	rc->renderer()->clear();
	
	_atmosphere->renderAtmosphereWithGeometry(_sample.camera(), true, false);
	
	_sample.render(rc, _environmentBlurredBuffer[2]->renderTarget(),
		_environmentBuffer->renderTarget(), rc->renderState().defaultFramebuffer());
	
	_gui->render(rc);
}

void MainController::idle(float)
{
	
}

void MainController::onDrag(vec2 p, PointerType t)
{
	if (_pointerCaptured) return;

#if (!ET_PLATFORM_IOS)
	if (t == PointerType_General)
		_sample.panCamera(5.0f * p);
	else
#endif
		_sample.dragCamera(p);
}

void MainController::onScroll(vec2 p, PointerOrigin o)
{
	if (_pointerCaptured) return;
	
	if (o == PointerOrigin_Mouse)
		_sample.zoom(1.0f + 0.25f * p.y);
	else
		_sample.panCamera(300.0f * vec2(p.x, -p.y));
}

void MainController::onZoom(float v)
{
	if (!_pointerCaptured)
		_sample.zoom(v);
}

void MainController::onCharactersEntered(std::string chars)
{
	int upCase = ::toupper(static_cast<int>(chars.front() & 0xffffffff));

	if (upCase == 'O')
		_sample.toggleObserving();

	if (upCase == 'W')
		_sample.toggleWireframe();
}

ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "Test"); }

IApplicationDelegate* Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }
