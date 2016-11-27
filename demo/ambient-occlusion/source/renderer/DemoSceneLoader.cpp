//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include "DemoSceneLoader.h"

#include <et/rendering/rendercontext.h>
#include <et/scene3d/objloader.h>
#include <et/app/application.h>

namespace et
{
namespace demo
{

void SceneLoader::init(RenderContext* rc)
{
	_rc = rc;
}

s3d::Scene::Pointer SceneLoader::loadFromFile(const std::string& fileName)
{
	ET_ASSERT(_rc);

	s3d::Scene::Pointer result = s3d::Scene::Pointer::create();

	auto ext = getFileExt(fileName);
	lowercase(ext);

	if (ext == "obj")
	{
		loadObjFile(fileName, result);
	}
	else if (ext == "etm")
	{
		ET_FAIL("Not implemented")
	}
	else
	{
		ET_FAIL("Not implemented")
	}

	return result;
}

void SceneLoader::loadObjFile(const std::string& fileName, s3d::Scene::Pointer scene)
{
	OBJLoader loader(fileName, OBJLoader::Option_CalculateTransforms);

	ObjectsCache localCache;
	auto container = loader.load(_rc->renderer(), scene->storage(), localCache);
	auto allObjects = container->children();

	for (auto c : allObjects)
		c->setParent(scene.pointer());
}

}
}
