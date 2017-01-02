#include <et/core/json.h>
#include <et/core/hardware.h>
#include <et/platform/platformtools.h>
#include <et/rendering/base/primitives.h>
#include <et/scene3d/objloader.h>
#include <et-ext/formats/fbxloader.h>
#include "converter.h"

namespace et
{

#if (ET_DEBUG)
const float invocationDelayTime = 0.5f;
#else
const float invocationDelayTime = 0.0f;
#endif

void Converter::setApplicationParameters(ApplicationParameters& p)
{
	auto screens = availableScreens();
	vec2i minScreenSize = screens.front().frame.size();
	for (const auto& s : screens)
		minScreenSize = minv(minScreenSize, s.frame.size());

	p.context.size = 4 * minScreenSize / 5;
}

void Converter::setRenderContextParameters(RenderContextParameters& p)
{
}

void Converter::applicationDidLoad(RenderContext* rc)
{
	ObjectsCache localCache;

	_rc = rc;

	_gestures.pointerPressed.connect(this, &Converter::onPointerPressed);
	_gestures.pointerMoved.connect(this, &Converter::onPointerMoved);
	_gestures.pointerReleased.connect(this, &Converter::onPointerReleased);
	_gestures.scroll.connect(this, &Converter::onScroll);
	_gestures.zoom.connect(this, &Converter::onZoom);
	_gestures.drag.connect(this, &Converter::onDrag);

	/*
	 * Create 3D scene and renderer
	 */
	_scene = s3d::Scene::Pointer::create();
	_scene->setMainCamera(Camera::Pointer::create());
	_scene->mainCamera()->perspectiveProjection(QUARTER_PI, vector2ToFloat(rc->size()).aspect(), 1.0f, 2048.0f);
	_scene->mainCamera()->lookAt(fromSpherical(_vAngle.value().x, _vAngle.value().y) * _vDistance.value());
	_cameraController = CameraMovingController::Pointer::create(_scene->mainCamera(), true);
	_cameraController->setIntepolationRate(10.0f);

	_sceneRenderer = s3d::Renderer::Pointer::create();

	/*
	 * Create 2D renderer
	 */
	RenderPass::ConstructionInfo passInfo;
	passInfo.color[0].loadOperation = FramebufferOperation::Load;
	passInfo.color[0].enabled = true;
	passInfo.depth.enabled = false;

	_gui = s2d::Scene::Pointer::create(rc, passInfo);
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
	_mainFont->loadFromFile(rc, application().resolveFileName("engine_ext_data/fonts/tahoma.font"), localCache);

	vec4 defaultBackgroundColor = vec4(1.0f, 0.5f, 0.25f, 1.0f);
	float defaultFontSize = 16.0f;
	float defaultButtonSize = 0.15f;
	float defaultButtonOffset = 0.16f;
	float defaultButtonGap = 0.005f;

	s2d::Button::Pointer btnOpen = s2d::Button::Pointer::create("Open", _mainFont, defaultFontSize, _mainLayout.pointer());
	btnOpen->setFontSmoothing(2.25f);
	btnOpen->setAutolayoutRelativeToParent(vec2(defaultButtonGap), vec2(defaultButtonSize, 0.05f), vec2(0.0f));
	btnOpen->setBackgroundColor(defaultBackgroundColor);
	btnOpen->setTextPressedColor(vec4(1.0f));
	btnOpen->clicked.connect(this, &Converter::onBtnOpenClick);

	s2d::Button::Pointer btnExport = s2d::Button::Pointer::create("Export", _mainFont, defaultFontSize, _mainLayout.pointer());
	btnExport->setFontSmoothing(2.25f);
	btnExport->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonGap, defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(1.0f, 0.0f));
	btnExport->setBackgroundColor(defaultBackgroundColor);
	btnExport->setTextPressedColor(vec4(1.0f));
	btnExport->clicked.connect(this, &Converter::onBtnSaveClick);

	_btnDrawNormalMeshes = s2d::Button::Pointer::create("Normal", _mainFont, defaultFontSize, _mainLayout.pointer());
	_btnDrawNormalMeshes->setAutolayoutRelativeToParent(vec2(defaultButtonGap, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnDrawNormalMeshes->setBackgroundColor(defaultBackgroundColor);
	_btnDrawNormalMeshes->setTextPressedColor(vec4(1.0f));
	_btnDrawNormalMeshes->setType(s2d::Button::Type_CheckButton);
	_btnDrawNormalMeshes->setSelected(true);
	_btnDrawNormalMeshes->setFontSmoothing(2.25f);

	_btnDrawSupportMeshes = s2d::Button::Pointer::create("Support", _mainFont, defaultFontSize, _mainLayout.pointer());
	_btnDrawSupportMeshes->setAutolayoutRelativeToParent(vec2(defaultButtonOffset, 1.0f - defaultButtonGap),
		vec2(defaultButtonSize, 0.05f), vec2(0.0f, 1.0f));
	_btnDrawSupportMeshes->setBackgroundColor(defaultBackgroundColor);
	_btnDrawSupportMeshes->setTextPressedColor(vec4(1.0f));
	_btnDrawSupportMeshes->setType(s2d::Button::Type_CheckButton);
	_btnDrawSupportMeshes->setSelected(true);
	_btnDrawSupportMeshes->setFontSmoothing(2.25f);

	_btnWireframe = s2d::Button::Pointer::create("Wireframe", _mainFont, defaultFontSize, _mainLayout.pointer());
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

	_labStatus = s2d::Label::Pointer::create("Ready", _mainFont, defaultFontSize, _mainLayout.pointer());
	_labStatus->setAutolayoutRelativeToParent(vec2(1.0f - defaultButtonGap), vec2(0.0f), vec2(1.0f));
	_labStatus->setTextAlignment(s2d::Alignment::Far, s2d::Alignment::Far);
	_labStatus->setFontSmoothing(2.25f);

	/*/ TODO : load materials
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
	// */

	buildSupportMeshes(rc);

	/*/ TODO : move to render passes
	rc->renderState().setClearColor(vec4(0.25f));
	rc->renderState().setDepthTest(true);
	rc->renderState().setDepthMask(true);
	rc->renderState().setBlend(false, BlendState::Default);
	// */

	const StringList& lp = application().launchParameters();
	if ((lp.size() > 1) && fileExists(lp.at(1)))
	{
		Invocation1 i;
		i.setTarget(this, &Converter::performLoading, lp.at(1));
		i.invokeInMainRunLoop(invocationDelayTime);
	}
}

void Converter::renderMeshList(RenderContext* rc, const s3d::BaseElement::List& meshes)
{
	/*/ TODO : render
	for (auto i = meshes.begin(), e = meshes.end(); i != e; ++i)
	{
		s3d::Mesh::Pointer mesh = *i;
		if (mesh->vertexBuffer().invalid()) continue;

		Program::Pointer programToUse = _transformedProgram;
		if (mesh->vertexBuffer()->declaration().has(VertexAttributeUsage::BlendIndices))
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
	// */
}

void Converter::renderSkeleton(RenderContext* rc, const s3d::BaseElement::List& bones)
{
	/*/ TODO : render using debug renderer
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
	// */
}

void Converter::performSceneRendering()
{
	/*/ TODO : render using scene renderer
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
	// */
}

void Converter::render(RenderContext* rc)
{
	_sceneRenderer->render(_rc->renderer(), _scene);
	_gui->render(rc);
}

void Converter::onPointerPressed(PointerInputInfo p)
{
	_vDistance.cancelInterpolation();
	_vAngle.cancelInterpolation();

	_gui->pointerPressed(p);
}

void Converter::onPointerMoved(PointerInputInfo p)
{
	_gui->pointerMoved(p);
}

void Converter::onPointerReleased(PointerInputInfo p)
{
	_gui->pointerReleased(p);
}

void Converter::onZoom(float z)
{
	_vDistance.addTargetValue(z);
}

void Converter::onDrag(const GesturesRecognizer::DragGesture& gest)
{
	_vAngle.addTargetValue(0.25f * vec2(-gest.delta.y, gest.delta.x));
}

void Converter::onScroll(vec2 s, PointerOrigin o)
{
	onZoom(s.y * ((o == PointerOrigin::Mouse) ? 1.0f : 100.0f));
}

void Converter::onBtnOpenClick(s2d::Button*)
{
	std::string fileName = selectFile({ "fbx", "json", "etmx" }, SelectFileMode::Open, std::string());

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

void Converter::onBtnSaveClick(s2d::Button* b)
{
	_labStatus->setText("Saving...");

	Invocation([this]()
	{
		std::string fileName = selectFile(StringList(), SelectFileMode::Save, _scene->name());

		fileName = replaceFileExt(fileName, ".json");
		{
			Dictionary serialized;// = _scene->serialize(fileName);
			auto serializedString = json::serialize(serialized, json::SerializationFlag_ReadableFormat);
			StringDataStorage data(static_cast<uint32_t>(serializedString.size() + 1, 0));
			memcpy(data.binary(), serializedString.data(), serializedString.length());
			data.writeToFile(fileName);
		}

		fileName = replaceFileExt(fileName, ".etmx");
		{
			Dictionary serialized;// = _scene->serialize(fileName);
			auto serializedString = json::serialize(serialized);
			StringDataStorage data(static_cast<uint32_t>(serializedString.size() + 1, 0));
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
		VariantClass vc = VariantClass::Invalid;
		Dictionary info = json::deserialize(loadTextFile(path), vc);
		if (vc == VariantClass::Dictionary)
		{
			// _scene->deserializeWithOptions(_rc, info, getFilePath(path), _texCache, s3d::DeserializeOption_KeepAndCreateEverything);
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
		s3d::ElementContainer::Pointer loadedScene = loader.load(_rc->renderer(), _scene->storage(), _texCache);
		if (loadedScene.valid())
		{
			auto loadedObjects = loadedScene->children();
			for (auto obj : loadedObjects)
				obj->setParent(_scene.pointer());
		}
	}
	else if (extension == "obj")
	{
		OBJLoader loader(path, OBJLoader::Option_JustLoad);
		s3d::ElementContainer::Pointer loadedScene = loader.load(_rc->renderer(), _scene->storage(), _texCache);
		if (loadedScene.valid())
		{
			auto loadedObjects = loadedScene->children();
			for (auto obj : loadedObjects)
				obj->setParent(_scene.pointer());
		}
	}

	vec3 sceneMaxVector(1.0f);
	vec3 sceneMinVector(-1.0f);
	s3d::BaseElement::Collection meshes = _scene->childrenOfType(s3d::ElementType::Mesh);
	for (s3d::Mesh::Pointer mesh : meshes)
	{
		const BoundingBox& bbox = mesh->tranformedBoundingBox();
		sceneMaxVector = maxv(sceneMaxVector, bbox.maxVertex());
		sceneMinVector = minv(sceneMinVector, bbox.minVertex());
	}
	vec3 sceneCenter = 0.5f * (sceneMinVector + sceneMaxVector);
	float sceneRadius = (sceneMaxVector - sceneMinVector).length();

	// printStructureRecursive(_scene, "|");

	_scene->mainCamera()->lookAt(fromSpherical(QUARTER_PI, QUARTER_PI) * sceneRadius * SQRT_2, sceneCenter);
	_scene->storage().flush();
	_scene->animateRecursive();
	_labStatus->setText("Completed.");
	
	_cameraController->synchronize(_scene->mainCamera());
}

void Converter::performBinarySaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	// _scene->serialize(path)
	_labStatus->setText("Completed.");
}

void Converter::performBinaryWithReadableMaterialsSaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	// _scene->serialize(path);
	_labStatus->setText("Completed.");
}

void Converter::onCameraUpdated()
{
	_scene->mainCamera()->lookAt(fromSpherical(_vAngle.value().x, _vAngle.value().y) * _vDistance.value());
}

void Converter::buildSupportMeshes(RenderContext* rc)
{
	VertexDeclaration decl(true, VertexAttributeUsage::Position, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::Normal, DataType::Vec3);
	decl.push_back(VertexAttributeUsage::TexCoord0, DataType::Vec2);

	{
		vec2i sphereDensity(4);
		VertexStorage::Pointer va = VertexStorage::Pointer::create(decl, 0);
		primitives::createSphere(va, 0.05f, sphereDensity);
		IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit,
			primitives::indexCountForRegularMesh(sphereDensity, PrimitiveType::TriangleStrips),
			PrimitiveType::TriangleStrips);
		primitives::buildTriangleStripIndexes(ia, sphereDensity, 0, 0);
		_sphereVao = VertexStream::Pointer::create();
		_sphereVao->setIndexBuffer(rc->renderer()->createIndexBuffer("sphere-ib", ia, Buffer::Location::Device), ia->format(), ia->primitiveType());
		_sphereVao->setVertexBuffer(rc->renderer()->createVertexBuffer("sphere-vb", va, Buffer::Location::Device), va->declaration());
	}
	{
		IndexArray::Pointer ia = IndexArray::Pointer::create(IndexArrayFormat::Format_8bit, 2, PrimitiveType::Lines);
		ia->setIndex(0, 0);
		ia->setIndex(1, 1);
		VertexStorage::Pointer va = VertexStorage::Pointer::create(decl, 2);

		_lineVao = VertexStream::Pointer::create();
		_lineVao->setIndexBuffer(rc->renderer()->createIndexBuffer("line-ib", ia, Buffer::Location::Device), ia->format(), ia->primitiveType());
		_lineVao->setVertexBuffer(rc->renderer()->createVertexBuffer("line-vb", va, Buffer::Location::Device), va->declaration());
	}
}

void Converter::printStructureRecursive(s3d::BaseElement::Pointer e, const std::string& tag)
{
	log::info("%s%s", tag.c_str(), e->name().c_str());
	for (auto c : e->children())
		printStructureRecursive(c, tag + "--|");
}


ApplicationIdentifier Converter::applicationIdentifier() const
{
	return ApplicationIdentifier("com.cheetek.fbxconverter", "Cheetek", "FBXConverter");
}

IApplicationDelegate* Application::initApplicationDelegate()
{
	return sharedObjectFactory().createObject<Converter>();
}

}
