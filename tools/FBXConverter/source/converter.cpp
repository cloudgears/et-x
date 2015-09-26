#include <et/json/json.h>
#include <et/platform/platformtools.h>
#include <et/primitives/primitives.h>
#include <et/models/fbxloader.h>
#include <et/models/objloader.h>
#include "converter.h"

using namespace fbxc;
using namespace et;

#if (ET_DEBUG)
	const float invocationDelayTime = 5.0f;
#else
	const float invocationDelayTime = 1.0f;
#endif

Converter::Converter() :
	_rc(nullptr), _cameraController(_camera, true)
{
	
}

void Converter::setRenderContextParameters(RenderContextParameters& p)
{
	auto screens = availableScreens();
	vec2i minScreenSize = screens.front().frame.size();
	for (const auto& s  : screens)
		minScreenSize = minv(minScreenSize, s.frame.size());
	
	p.multisamplingQuality = MultisamplingQuality_Best;
	p.contextSize = 4 * minScreenSize / 5;
	p.contextBaseSize = p.contextSize;
}

void Converter::applicationDidLoad(RenderContext* rc)
{
	ObjectsCache localCache;

	_rc = rc;
	_rc->renderState().setDepthTest(true);

	_gestures.pointerPressed.connect(this, &Converter::onPointerPressed);
	_gestures.pointerMoved.connect(this, &Converter::onPointerMoved);
	_gestures.pointerReleased.connect(this, &Converter::onPointerReleased);
	_gestures.scroll.connect(this, &Converter::onScroll);
	_gestures.zoom.connect(this, &Converter::onZoom);
	_gestures.drag.connect(this, &Converter::onDrag);
	
	// _cameraController.startUpdates();

	_gui = s2d::Scene::Pointer::create(rc);
	_mainLayout = MainLayout::Pointer::create();
	_gui->pushLayout(_mainLayout);
	
#if (ET_PLATFORM_WIN)
	s2d::CharacterGenerator::Pointer defaultCharacters =
		s2d::CharacterGenerator::Pointer::create(rc, "Tahoma", "Tahoma");
#else
	s2d::CharacterGenerator::Pointer defaultCharacters =
		s2d::CharacterGenerator::Pointer::create(rc, "Helvetica", "Helvetica");
#endif
	_mainFont = s2d::Font::Pointer::create(defaultCharacters);
	_mainFont->loadFromFile(rc, application().resolveFileName("data/tahoma.font"), localCache);
	
	vec4 defaultBackgroundColor = vec4(1.0f, 0.5f, 0.25f, 1.0f);
	float defaultFontSize = 16.0f;
	float defaultButtonSize = 0.15f;
	float defaultButtonOffset = 0.16f;
	float defaultButtonGap = 0.005f;
	
	s2d::Button::Pointer btnOpen = s2d::Button::Pointer::create("Open", _mainFont, defaultFontSize, _mainLayout.ptr());
	btnOpen->setFontSmoothing(2.25f);
	btnOpen->setAutolayoutRelativeToParent(vec2(defaultButtonGap), vec2(defaultButtonSize, 0.05f), vec2(0.0f));
	btnOpen->setBackgroundColor(defaultBackgroundColor);
	btnOpen->setTextPressedColor(vec4(1.0f));
	btnOpen->clicked.connect(this, &Converter::onBtnOpenClick);

	s2d::Button::Pointer btnExport = s2d::Button::Pointer::create("Export", _mainFont, defaultFontSize, _mainLayout.ptr());
	btnExport->setFontSmoothing(2.25f);
	btnExport->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonGap, defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(1.0f, 0.0f));
	btnExport->setBackgroundColor(defaultBackgroundColor);
	btnExport->setTextPressedColor(vec4(1.0f));
	btnExport->clicked.connect(this, &Converter::onBtnSaveClick);

	_btnDrawNormalMeshes = s2d::Button::Pointer::create("Normal", _mainFont, defaultFontSize, _mainLayout.ptr());
	_btnDrawNormalMeshes->setAutolayoutRelativeToParent(vec2(defaultButtonGap, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnDrawNormalMeshes->setBackgroundColor(defaultBackgroundColor);
	_btnDrawNormalMeshes->setTextPressedColor(vec4(1.0f));
	_btnDrawNormalMeshes->setType(s2d::Button::Type_CheckButton);
	_btnDrawNormalMeshes->setSelected(true);
	_btnDrawNormalMeshes->setFontSmoothing(2.25f);

	_btnDrawSupportMeshes = s2d::Button::Pointer::create("Support", _mainFont, defaultFontSize, _mainLayout.ptr());
	_btnDrawSupportMeshes->setAutolayoutRelativeToParent(vec2(defaultButtonOffset, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnDrawSupportMeshes->setBackgroundColor(defaultBackgroundColor);
	_btnDrawSupportMeshes->setTextPressedColor(vec4(1.0f));
	_btnDrawSupportMeshes->setType(s2d::Button::Type_CheckButton);
	_btnDrawSupportMeshes->setSelected(true);
	_btnDrawSupportMeshes->setFontSmoothing(2.25f);

	_btnWireframe = s2d::Button::Pointer::create("Wireframe", _mainFont, defaultFontSize, _mainLayout.ptr());
	_btnWireframe->setAutolayoutRelativeToParent(vec2(2.0f * defaultButtonOffset, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnWireframe->setBackgroundColor(defaultBackgroundColor);
	_btnWireframe->setTextPressedColor(vec4(1.0f));
	_btnWireframe->setType(s2d::Button::Type_CheckButton);
	_btnWireframe->setSelected(false);
	_btnWireframe->setFontSmoothing(2.25f);
	
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
	_labStatus->setFontSmoothing(2.25f);

	_camera.perspectiveProjection(QUARTER_PI, rc->size().aspect(), 1.0f, 2048.0f);
	_camera.lookAt(fromSpherical(_vAngle.value().x, _vAngle.value().y) * _vDistance.value());
	
	_skinnedProgram = rc->programFactory().loadProgram("data/shaders/skinned.program", localCache);
	_skinnedProgram->setPrimaryLightPosition(500.0f * vec3(0.0f, 1.0f, 0.0f));
	_skinnedProgram->setUniform("diffuseMap", 0);
	_skinnedProgram->setUniform("specularMap", 1);
	_skinnedProgram->setUniform("normalMap", 2);

	_transformedProgram = rc->programFactory().loadProgram("data/shaders/transformed.program", localCache);
	_transformedProgram->setPrimaryLightPosition(500.0f * vec3(0.0f, 1.0f, 0.0f));
	_transformedProgram->setUniform("diffuseMap", 0);
	_transformedProgram->setUniform("specularMap", 1);
	_transformedProgram->setUniform("normalMap", 2);

	_scene = s3d::Scene::Pointer::create();
	
	buildSupportMeshes(rc);

	rc->renderState().setClearColor(vec4(0.25f));
	rc->renderState().setDepthTest(true);
	rc->renderState().setDepthMask(true);
	rc->renderState().setBlend(false, BlendState::Default);
	
	const std::string& lp = application().launchParameter(1);
	if (fileExists(lp))
	{
		Invocation1 i;
		i.setTarget(this, &Converter::performLoading, lp);
		i.invokeInMainRunLoop(invocationDelayTime);
	}
}

void Converter::renderMeshList(RenderContext* rc, const s3d::BaseElement::List& meshes)
{
	for (auto i = meshes.begin(), e = meshes.end(); i != e; ++i)
	{
		s3d::Mesh::Pointer mesh = *i;
		if (mesh->vertexBuffer().invalid()) continue;
		
		Program::Pointer programToUse = _transformedProgram;
		if (mesh->vertexBuffer()->declaration().has(et::VertexAttributeUsage::BlendIndices))
			programToUse = _skinnedProgram;
		
		rc->renderState().bindProgram(programToUse);
		
		s3d::Material::Pointer m = mesh->material();
		const auto& bones = mesh->deformationMatrices();
		
		programToUse->setUniform<mat4>("bones[0]", bones.data(), bones.size());
		programToUse->setUniform("ambientColor", m->getVector(MaterialParameter_AmbientColor));
		programToUse->setUniform("diffuseColor", m->getVector(MaterialParameter_DiffuseColor));
		programToUse->setUniform("specularColor", m->getVector(MaterialParameter_SpecularColor));
		programToUse->setUniform("roughness", m->getFloat(MaterialParameter_Roughness));
		programToUse->setUniform("mTransform", mesh->finalTransform());
			
		rc->renderState().bindTexture(0, mesh->material()->getTexture(MaterialParameter_DiffuseMap));
		rc->renderState().bindTexture(1, mesh->material()->getTexture(MaterialParameter_SpecularMap));
		rc->renderState().bindTexture(2, mesh->material()->getTexture(MaterialParameter_NormalMap));
			
		rc->renderState().bindVertexArray(mesh->vertexArrayObject());
		rc->renderer()->drawElements(mesh->indexBuffer(), mesh->startIndex(), mesh->numIndexes());
	}
}

void Converter::renderSkeleton(et::RenderContext* rc, const et::s3d::BaseElement::List& bones)
{
	_rc->renderState().setWireframeRendering(true);
	_rc->renderState().setDepthTest(false);
	_rc->renderState().setBlend(false);
	_rc->renderState().bindProgram(_transformedProgram);

	if (_testVao.valid())
	{
		_rc->renderState().bindVertexArray(_testVao);
		_transformedProgram->setTransformMatrix(identityMatrix);
		_rc->renderer()->drawAllElements(_testVao->indexBuffer());
	}
	
	_rc->renderState().bindVertexArray(_sphereVao);
	for (auto c : bones)
	{
		_transformedProgram->setTransformMatrix(c->finalTransform());
		_rc->renderer()->drawAllElements(_sphereVao->indexBuffer());
	}

	_rc->renderState().bindVertexArray(_lineVao);
	_transformedProgram->setTransformMatrix(identityMatrix);
	for (auto c : bones)
	{
		if ((c->parent() != nullptr) && (c->parent()->type() == s3d::ElementType::Skeleton))
		{
			vec3* lineData = reinterpret_cast<vec3*>(
				_lineVao->vertexBuffer()->map(0, 2 * sizeof(vec3), MapBufferMode::WriteOnly));
			vec3 parentPos = c->parent()->finalTransform()[3].xyz();
			vec3 bonePos = c->finalTransform()[3].xyz();
			lineData[0] = parentPos;
			lineData[1] = parentPos.normalized();
			lineData[2] = bonePos;
			lineData[3] = bonePos.normalized();
			_lineVao->vertexBuffer()->unmap();
			_rc->renderer()->drawAllElements(_lineVao->indexBuffer());
		}
	}
	
	_rc->renderState().setBlend(false);
	_rc->renderState().setDepthTest(true);
	_rc->renderState().setWireframeRendering(false);
}

void Converter::performSceneRendering()
{
	_rc->renderState().setSampleAlphaToCoverage(true);
	
	_rc->renderState().bindProgram(_transformedProgram);
	_transformedProgram->setCameraProperties(_camera);
	_rc->renderState().bindProgram(_skinnedProgram);
	_skinnedProgram->setCameraProperties(_camera);
	
	_rc->renderState().setWireframeRendering(_btnWireframe->selected());
	
	if (_btnDrawNormalMeshes->selected())
		renderMeshList(_rc, _scene->childrenOfType(s3d::ElementType::Mesh));

	if (_btnDrawSupportMeshes->selected())
		renderMeshList(_rc, _scene->childrenOfType(s3d::ElementType::SupportMesh));
	
	renderSkeleton(_rc, _scene->childrenOfType(s3d::ElementType::Skeleton));
	
	_rc->renderState().setSampleAlphaToCoverage(false);
}

void Converter::render(RenderContext* rc)
{
	rc->renderer()->clear();
	performSceneRendering();
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

void Converter::onDrag(const GesturesRecognizer::DragGesture& gest)
{
	_vAngle.addTargetValue(0.25f * vec2(-gest.offset.y, gest.offset.x));
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
	std::string fileName = selectFile({"fbx", "json", "etmx"}, SelectFileMode::Open, std::string());
	
	if (fileExists(fileName))
	{
		_scene->clear();
		
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
	_labStatus->setText("Saving...");

	Invocation([this]()
	{
		std::string fileName = selectFile(StringList(), SelectFileMode::Save, _scene->name());

		fileName = replaceFileExt(fileName, ".json");
		{
			Dictionary serialized = _scene->serialize(fileName);
			auto serializedString = json::serialize(serialized, json::SerializationFlag_ReadableFormat);
			StringDataStorage data(serializedString.size() + 1, 0);
			memcpy(data.binary(), serializedString.data(), serializedString.length());
			data.writeToFile(fileName);
		}

		fileName = replaceFileExt(fileName, ".etmx");
		{
			Dictionary serialized = _scene->serialize(fileName);
			auto serializedString = json::serialize(serialized);
			StringDataStorage data(serializedString.size() + 1, 0);
			memcpy(data.binary(), serializedString.data(), serializedString.length());
			data.writeToFile(fileName);
		}

		_labStatus->setText("Saving completed.");

	}).invokeInMainRunLoop();
}

void Converter::performLoading(std::string path)
{
	lowercase(path);

	_scene->clearRecursively();

	auto extension = getFileExt(path);
	if ((extension == "etmx") || (extension == "json"))
	{
		ValueClass vc = ValueClass_Invalid;
		et::Dictionary info = json::deserialize(loadTextFile(path), vc);
		if (vc == ValueClass_Dictionary)
		{
			_scene->deserializeWithOptions(_rc, info, getFilePath(path), _texCache,
				s3d::DeserializeOption_KeepAndCreateEverything);
		}
		else 
		{
			_labStatus->setText("Failed to parse file " + getFileName(path));
			return;
		}
	}
	else if (extension == "fbx")
	{
		FBXLoader loader(path);
		s3d::ElementContainer::Pointer loadedScene = loader.load(_rc, _scene->storage(), _texCache);
		if (loadedScene.valid())
		{
			auto loadedObjects = loadedScene->children();
			for (auto obj : loadedObjects)
				obj->setParent(_scene.ptr());
		}
	}
	else if (extension == "obj")
	{
		OBJLoader loader(path, OBJLoader::Option_JustLoad);
		s3d::ElementContainer::Pointer loadedScene = loader.load(_rc, _scene->storage(), _texCache);
		if (loadedScene.valid())
		{
			auto loadedObjects = loadedScene->children();
			for (auto obj : loadedObjects)
				obj->setParent(_scene.ptr());
		}
	}
	
	// printStructureRecursive(_scene, "|");
	
	_scene->storage().flush();
	_scene->animateRecursive();
	_labStatus->setText("Completed.");
}

void Converter::performBinarySaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	_scene->serialize(path);
	_labStatus->setText("Completed.");
}

void Converter::performBinaryWithReadableMaterialsSaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	_scene->serialize(path);
	_labStatus->setText("Completed.");
}

void Converter::onCameraUpdated()
{
	_camera.lookAt(fromSpherical(_vAngle.value().x, _vAngle.value().y) * _vDistance.value());
}

void Converter::buildSupportMeshes(et::RenderContext* rc)
{
	VertexDeclaration decl(true, VertexAttributeUsage::Position, VertexAttributeType::Vec3);
	decl.push_back(VertexAttributeUsage::Normal, VertexAttributeType::Vec3);
	{
		vec2i sphereDensity(4);
		VertexArray::Pointer va = VertexArray::Pointer::create(decl, 0);
		primitives::createSphere(va, 0.05f, sphereDensity);
		IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit,
			primitives::indexCountForRegularMesh(sphereDensity, PrimitiveType::TriangleStrips),
			PrimitiveType::TriangleStrips);
		primitives::buildTriangleStripIndexes(ia, sphereDensity, 0, 0);
		_sphereVao = rc->vertexBufferFactory().createVertexArrayObject("sphere-vao", va, BufferDrawType::Static,
			ia, BufferDrawType::Static);
	}
	{
		IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_8bit, 2, PrimitiveType::Lines);
		ia->setIndex(0, 0);
		ia->setIndex(1, 1);
		VertexArray::Pointer va = VertexArray::Pointer::create(decl, 2);
		_lineVao = rc->vertexBufferFactory().createVertexArrayObject("line-vao", va, BufferDrawType::Dynamic,
			ia, BufferDrawType::Static);
	}
}

void Converter::printStructureRecursive(et::s3d::BaseElement::Pointer e, const std::string& tag)
{
	log::info("%s%s", tag.c_str(), e->name().c_str());
	for (auto c : e->children())
		printStructureRecursive(c, tag + "--|");
}

et::ApplicationIdentifier Converter::applicationIdentifier() const
	{ return ApplicationIdentifier("com.cheetek.fbxconverter", "Cheetek", "FBXConverter"); }

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<Converter>(); }
