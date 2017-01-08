#include <et/core/json.h>
#include <et/core/hardware.h>
#include <et/rendering/base/primitives.h>
#include <et/scene3d/objloader.h>
#include <et-ext/formats/fbxloader.h>
#include "converter.h"

namespace et
{
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

	/*
	 * Create 3D scene and renderer
	 */
	_scene = s3d::Scene::Pointer::create();
	_scene->setMainCamera(Camera::Pointer::create());
	_scene->mainCamera()->perspectiveProjection(DEG_60, vector2ToFloat(rc->size()).aspect(), 1.0f, 2048.0f);
	_scene->mainCamera()->lookAt(50.0f * fromSpherical(QUARTER_PI, QUARTER_PI));

	_cameraController = CameraMovingController::Pointer::create(_scene->mainCamera(), true);
	_cameraController->setIntepolationRate(10.0f);
	_cameraController->setMovementSpeed(vec3(25.0f));
	_cameraController->startUpdates();

	_sceneRenderer = s3d::Renderer::Pointer::create();

	/*
	 * Create 2D renderer
	 */
	RenderPass::ConstructionInfo passInfo;
	passInfo.color[0].loadOperation = FramebufferOperation::Load;
	passInfo.color[0].enabled = true;
	passInfo.depth.enabled = false;

	const StringList& lp = application().launchParameters();
	if ((lp.size() > 1) && fileExists(lp.at(1)))
	{
		Invocation1 i;
		i.setTarget(this, &Converter::performLoading, lp.at(1));
		i.invokeInMainRunLoop();
	}

	_gui = s2d::Scene::Pointer::create(rc, passInfo);
	_mainLayout = MainLayout::Pointer::create(_rc);

	_mainLayout->openSelected.connect([this](std::string name)
	{
		_scene->clear();
		performLoading(name);
	});
	
	_mainLayout->saveSelected.connect([](std::string name)
	{
		Dictionary serialized;// = _scene->serialize(fileName);		
		std::string serializedString = json::serialize(serialized);

		StringDataStorage data(static_cast<uint32_t>(serializedString.size() + 1, 0));
		memcpy(data.binary(), serializedString.data(), serializedString.length());
		data.writeToFile(replaceFileExt(name, ".json"));
	});

	_gui->pushLayout(_mainLayout);
	_gestures.pointerPressed.connect(_gui.pointer(), &s2d::Scene::pointerPressed);
	_gestures.pointerMoved.connect(_gui.pointer(), &s2d::Scene::pointerMoved);
	_gestures.pointerReleased.connect(_gui.pointer(), &s2d::Scene::pointerReleased);
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
			_mainLayout->setStatus("Failed to parse file " + getFileName(path));
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
	_mainLayout->setStatus("Loading complete");
	
	_cameraController->synchronize(_scene->mainCamera());
}

void Converter::performBinarySaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	// _scene->serialize(path)
	_mainLayout->setStatus("Saving complete");
}

void Converter::performBinaryWithReadableMaterialsSaving(std::string path)
{
	if (path.find_last_of(".etm") != path.length() - 1)
		path += ".etm";

	// _scene->serialize(path);
	_mainLayout->setStatus("Saving complete");
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
