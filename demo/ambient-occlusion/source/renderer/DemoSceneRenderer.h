//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et/scene3d/scene3d.h>

namespace demo
{
	struct AOParameters
	{
		int numSamples = 32;
		float aoPower = 6.0f;
		float depthDifference = 5.0f;
		et::vec2 sampleScale = et::vec2(0.001f, 1.0f);

		bool showDiffuse = false;
		bool performBlur = false;
	};

	class SceneRenderer
	{
	public:
		void init(et::RenderContext*);
		void render(const et::Camera&, const AOParameters&, bool);
		
		void setScene(et::s3d::Scene::Pointer);
		
		void handlePressedKey(size_t);
		
	private:
		void renderToGeometryBuffer(const et::Camera&, const AOParameters&);
		void computeAmbientOcclusion(const et::Camera&, const AOParameters&);
		
	private:
		struct
		{
			et::Program::Pointer prepass;
			et::Program::Pointer ambientOcclusion;
			et::Program::Pointer ambientOcclusionBlur;
			et::Program::Pointer final;
		} programs;
		
	private:
		et::RenderContext* _rc = nullptr;
		et::Framebuffer::Pointer _geometryBuffer;
		et::Framebuffer::Pointer _aoBuffer;
		
		et::s3d::Scene::Pointer _scene;
		std::vector<et::s3d::SupportMesh::Pointer> _allObjects;
		
		et::Texture::Pointer _noiseTexture;
		et::Texture::Pointer _defaultTexture;
		et::Texture::Pointer _defaultNormalTexture;
	};
}
