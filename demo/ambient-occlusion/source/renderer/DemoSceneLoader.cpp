//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include <et/models/objloader.h>
#include <et/app/application.h>
#include "DemoSceneLoader.h"

using namespace et;
using namespace demo;

void SceneLoader::init(et::RenderContext* rc)
{
	_rc = rc;
}

et::s3d::Scene::Pointer SceneLoader::loadFromFile(const std::string& fileName)
{
	ET_ASSERT(_rc)
	
	et::s3d::Scene::Pointer result = et::s3d::Scene::Pointer::create();
	
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

void SceneLoader::loadObjFile(const std::string& fileName, et::s3d::Scene::Pointer scene)
{
	ObjectsCache localCache;
	OBJLoader loader(_rc, fileName);
	
	auto container = loader.load(localCache, OBJLoader::Option_SupportMeshes);
	auto allObjects = container->children();
	
	for (auto c : allObjects)
		c->setParent(scene.ptr());
}

