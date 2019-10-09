//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et/scene3d/scene3d.h>

namespace et
{
namespace demo
{

class SceneLoader
{
public:
	void init(RenderContext*);

	s3d::Scene::Pointer loadFromFile(const std::string&);

private:
	void loadObjFile(const std::string&, s3d::Scene::Pointer);

private:
	RenderContext* _rc = nullptr;
};

}
}
