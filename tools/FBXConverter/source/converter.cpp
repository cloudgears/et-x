#include <et/platform/platformtools.h>
#include <et/models/fbxloader.h>
#include "converter.h"

using namespace fbxc;
using namespace et;

const float invocationDelayTime = 1.0f / 3.0f;

Converter::Converter() :
	_rc(nullptr)
{
}

void Converter::setRenderContextParameters(RenderContextParameters& p)
{
	p.multisamplingQuality = MultisamplingQuality_Best;
	p.contextSize = vec2i(1280, 720);
}

void Converter::applicationDidLoad(RenderContext* rc)
{
	_rc = rc;
	_rc->renderState().setDepthTest(true);

	_gestures.pointerPressed.connect(this, &Converter::onPointerPressed);
	_gestures.pointerMoved.connect(this, &Converter::onPointerMoved);
	_gestures.pointerReleased.connect(this, &Converter::onPointerReleased);
	_gestures.scroll.connect(this, &Converter::onScroll);
	_gestures.zoom.connect(this, &Converter::onZoom);
	_gestures.drag.connect(this, &Converter::onDrag);

	_gui = s2d::Scene::Pointer::create(rc);
	_mainLayout = MainLayout::Pointer::create();
	_gui->pushLayout(_mainLayout);
	
	s2d::CharacterGenerator::Pointer defaultCharacters = s2d::CharacterGenerator::Pointer::create(rc, "Helvetica", "Helvetica");
	_mainFont = s2d::Font::Pointer::create(defaultCharacters);
	
	vec4 defaultBackgroundColor = vec4(1.0f, 0.5f, 0.25f, 1.0f);
	float defaultFontSize = 16.0f;
	float defaultButtonSize = 0.15f;
	float defaultButtonOffset = 0.16f;
	float defaultButtonGap = 0.005f;
	
	s2d::Button::Pointer btnOpen = s2d::Button::Pointer::create("Open", _mainFont, defaultFontSize, _mainLayout.ptr());
	btnOpen->setAutolayoutRelativeToParent(vec2(defaultButtonGap), vec2(defaultButtonSize, 0.05f), vec2(0.0f));
	btnOpen->setBackgroundColor(defaultBackgroundColor);
	btnOpen->clicked.connect(this, &Converter::onBtnOpenClick);

	s2d::Button::Pointer btnSaveBin = s2d::Button::Pointer::create("Save (etm)", _mainFont, defaultFontSize, _mainLayout.ptr());
	btnSaveBin->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonGap, defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(1.0f, 0.0f));
	btnSaveBin->tag = 1;
	btnSaveBin->setBackgroundColor(defaultBackgroundColor);
	btnSaveBin->clicked.connect(this, &Converter::onBtnSaveClick);

	s2d::Button::Pointer btnSaveHRM = s2d::Button::Pointer::create("Save (etm+xml)", _mainFont, defaultFontSize, _mainLayout.ptr());
	btnSaveHRM->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonOffset - defaultButtonGap, defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(1.0f, 0.0f));
	btnSaveHRM->tag = 2;
	btnSaveHRM->setBackgroundColor(defaultBackgroundColor);
	btnSaveHRM->clicked.connect(this, &Converter::onBtnSaveClick);

	_btnDrawNormalMeshes = s2d::Button::Pointer::create("Normal", _mainFont, defaultFontSize, _mainLayout.ptr());
	_btnDrawNormalMeshes->setAutolayoutRelativeToParent(vec2(defaultButtonGap, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnDrawNormalMeshes->setBackgroundColor(defaultBackgroundColor);
	_btnDrawNormalMeshes->setType(s2d::Button::Type_CheckButton);
	_btnDrawNormalMeshes->setSelected(true);

	_btnDrawSupportMeshes = s2d::Button::Pointer::create("Support", _mainFont, defaultFontSize, _mainLayout.ptr());
	_btnDrawSupportMeshes->setAutolayoutRelativeToParent(vec2(defaultButtonOffset, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnDrawSupportMeshes->setBackgroundColor(defaultBackgroundColor);
	_btnDrawSupportMeshes->setType(s2d::Button::Type_CheckButton);
	_btnDrawSupportMeshes->setSelected(true);

	_btnWireframe = s2d::Button::Pointer::create("Wireframe", _mainFont, defaultFontSize, _mainLayout.ptr());
	_btnWireframe->setAutolayoutRelativeToParent(vec2(2.0f * defaultButtonOffset, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnWireframe->setBackgroundColor(defaultBackgroundColor);
	_btnWireframe->setType(s2d::Button::Type_CheckButton);
	_btnWireframe->setSelected(false);
	
	_vDistance.setTargetValue(300.0f);
	_vDistance.updated.connect(this, &Converter::onCameraUpdated);
	_vDistance.finishInterpolation();
	_vDistance.run();

	_vAngle.setTargetValue(vec2(QUARTER_PI));
	_vAngle.updated.connect(this, &Converter::onCameraUpdated);
	_vAngle.finishInterpolation();
	_vAngle.run();

	_labStatus = s2d::Label::Pointer::create("Ready", _mainFont, defaultFontSize, _mainLayout.ptr());
	_labStatus->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonGap), vec2(0.0f), vec2(1.0f));
	_labStatus->setHorizontalAlignment(s2d::Alignment_Far);
	_labStatus->setVerticalAlignment(s2d::Alignment_Far);

	_camera.perspectiveProjection(QUARTER_PI, rc->size().aspect(), 1.0f, 2048.0f);
	_camera.lookAt(fromSpherical(_vAngle.value().x, _vAngle.value().y) * _vDistance.value());

	ObjectsCache localCache;
	
	_defaultProgram = rc->programFactory().loadProgram("data/shaders/default.program", localCache);
	_defaultProgram->setPrimaryLightPosition(500.0f * vec3(0.0f, 1.0f, 0.0f));
	_defaultProgram->setUniform("diffuseMap", 0);
	_defaultProgram->setUniform("specularMap", 1);
	_defaultProgram->setUniform("normalMap", 2);

	rc->renderState().setClearColor(vec4(0.25f));
	rc->renderState().setDepthTest(true);
	rc->renderState().setDepthMask(true);
	rc->renderState().setBlend(true, BlendState::Default);
	
	const std::string& lp = application().launchParameter(1);
	if (fileExists(lp))
	{
		Invocation1 i;
		i.setTarget(this, &Converter::performLoading, lp);
		i.invokeInMainRunLoop(invocationDelayTime);
	}
}

void Converter::renderMeshList(RenderContext* rc, const s3d::Element::List& meshes)
{
	for (auto i = meshes.begin(), e = meshes.end(); i != e; ++i)
	{
		s3d::Mesh::Pointer mesh = *i;
		if (mesh->active())
		{
			const s3d::Material::Pointer& m = mesh->material();
			
			_defaultProgram->setUniform("ambientColor", m->getVector(MaterialParameter_AmbientColor));
			_defaultProgram->setUniform("diffuseColor", m->getVector(MaterialParameter_DiffuseColor));
			_defaultProgram->setUniform("specularColor", m->getVector(MaterialParameter_SpecularColor));
			_defaultProgram->setUniform("roughness", m->getFloat(MaterialParameter_Roughness));
			_defaultProgram->setUniform("mTransform", mesh->finalTransform());
			
			rc->renderState().bindTexture(0, mesh->material()->getTexture(MaterialParameter_DiffuseMap));
			rc->renderState().bindTexture(1, mesh->material()->getTexture(MaterialParameter_SpecularMap));
			rc->renderState().bindTexture(2, mesh->material()->getTexture(MaterialParameter_NormalMap));
			
			rc->renderState().bindVertexArray(mesh->vertexArrayObject());
			rc->renderer()->drawElements(mesh->indexBuffer(), mesh->startIndex(), mesh->numIndexes());
		}
	}
}

void Converter::performSceneRendering()
{
	_rc->renderState().setSampleAlphaToCoverage(true);
	_rc->renderState().bindProgram(_defaultProgram);
	_defaultProgram->setCameraProperties(_camera);

	if (_btnDrawNormalMeshes->selected())
		renderMeshList(_rc, _scene.childrenOfType(s3d::ElementType_Mesh));

	if (_btnDrawSupportMeshes->selected())
		renderMeshList(_rc, _scene.childrenOfType(s3d::ElementType_SupportMesh));
	
	_rc->renderState().setSampleAlphaToCoverage(false);
}

void Converter::render(RenderContext* rc)
{
	rc->renderer()->clear();

	rc->renderState().setWireframeRendering(_btnWireframe->selected());
	performSceneRendering();
	rc->renderState().setWireframeRendering(false);
	
	_gui->render(rc);
}

void Converter::onPointerPressed(et::PointerInputInfo p)
{
	_vDistance.cancelInterpolation();
	_vAngle.cancelInterpolation();
	
	_gui->pointerPressed(p);
}

void Converter::onPointerMoved(et::PointerInputInfo p)
{
	_gui->pointerMoved(p);
}

void Converter::onPointerReleased(et::PointerInputInfo p)
{
	_gui->pointerReleased(p);
}

void Converter::onZoom(float z)
{
	_vDistance.addTargetValue(z);
}

void Converter::onDrag(et::vec2 v, et::PointerType)
{
	_vAngle.addTargetValue(0.25f * vec2(-v.y, v.x));
}

void Converter::onScroll(et::vec2 s, et::PointerOrigin o)
{
	if (o == PointerOrigin_Mouse)
		onZoom(s.y);
	else if (o == PointerOrigin_Trackpad)
		onZoom(100.0f * s.y);
}

void Converter::onBtnOpenClick(et::s2d::Button*)
{
	std::string fileName = selectFile({"fbx", "etm"}, SelectFileMode::Open, std::string());
	
	if (fileExists(fileName))
	{
		_scene.clear();
		
		Invocation1 i;
		i.setTarget(this, &Converter::performLoading, fileName);
		i.invokeInMainRunLoop(invocationDelayTime);
		
		_labStatus->setText("Loading...");
	}
	else
	{
		_labStatus->setText("No file selected or unable to locate selected file");
	}
}

void Converter::onBtnSaveClick(et::s2d::Button* b)
{
	std::string fileName = selectFile(StringList(), SelectFileMode::Save, _scene.name());
	
	Invocation1 i;

	i.setTarget(this, (b->tag == 1 ? &Converter::performBinarySaving :
		&Converter::performBinaryWithReadableMaterialsSaving), fileName);
	
	i.invokeInMainRunLoop(invocationDelayTime);
	
	_labStatus->setText("Saving...");
}

void Converter::performLoading(std::string path)
{
	lowercase(path);
	size_t value = path.find_last_of(".etm");
	size_t len = path.length();

	if (value == len - 1)
	{
		_scene.deserialize(path, _rc, _texCache);
	}
	else
	{
		FBXLoader loader(path);
		s3d::ElementContainer::Pointer loadedScene = loader.load(_rc, _texCache);
		if (loadedScene.valid())
			loadedScene->setParent(&_scene);
	}
	
	auto nodesWithAnimation = _scene.childrenHavingFlag(s3d::Flag_HasAnimations);
	
	for (auto& i : nodesWithAnimation)
		i->animate();
	
	_labStatus->setText("Completed.");
}

void Converter::performBinarySaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	_scene.serialize(path);
	_labStatus->setText("Completed.");
}

void Converter::performBinaryWithReadableMaterialsSaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	_scene.serialize(path);
	_labStatus->setText("Completed.");
}

void Converter::onCameraUpdated()
{
	_camera.lookAt(fromSpherical(_vAngle.value().x, _vAngle.value().y) * _vDistance.value());
}

et::ApplicationIdentifier Converter::applicationIdentifier() const
	{ return ApplicationIdentifier("com.cheetek.fbxconverter", "Cheetek", "FBXConverter"); }

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<Converter>(); }
