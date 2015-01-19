//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include "DemoSceneRenderer.h"

using namespace et;
using namespace demo;

#define ENABLE_DEBUG_RENDERING	0

extern const std::string basicVertexShader;
extern const std::string basicFragmentShader;

enum
{
	diffuseTextureUnit = 0,
	normalTextureUnit = 1,
	transparencyTextureUnit = 2,
	depthTextureUnit = 3,
	noiseTextureUnit = 4,
	occlusionTextureUnit = 5,
};

void SceneRenderer::init(et::RenderContext* rc)
{
	_rc = rc;
	
	_geometryBuffer = rc->framebufferFactory().createFramebuffer(rc->sizei(), "geometry-buffer");
	_geometryBuffer->addSameRendertarget();
	
	_aoBuffer = rc->framebufferFactory().createFramebuffer(rc->sizei(), "ao-buffer",
		TextureFormat::RGBA, TextureFormat::RGBA, DataType::UnsignedChar, TextureFormat::Invalid);
	_aoBuffer->addSameRendertarget();
	
	_defaultTexture = _rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA,
		vec2i(1), TextureFormat::RGBA, DataType::UnsignedChar, BinaryDataStorage(4, 255), "white-texture");
	
	_noiseTexture = _rc->textureFactory().genNoiseTexture(vec2i(64), true, "noise-texture");
	
	BinaryDataStorage normalData(4, 128);
	normalData[2] = 255;
	
	_defaultNormalTexture = _rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA,
		vec2i(1), TextureFormat::RGBA, DataType::UnsignedChar, normalData, "normal-texture");
	
	ObjectsCache localCache;
	programs.prepass = _rc->programFactory().loadProgram("data/shaders/prepass.program", localCache);
	programs.prepass->setUniform("texture_mask", transparencyTextureUnit);
	programs.prepass->setUniform("texture_diffuse", diffuseTextureUnit);

	programs.ambientOcclusion = _rc->programFactory().loadProgram("data/shaders/ao.program", localCache);
	programs.ambientOcclusion->setUniform("texture_depth", depthTextureUnit);
	programs.ambientOcclusion->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.ambientOcclusion->setUniform("texture_normal", normalTextureUnit);
	programs.ambientOcclusion->setUniform("texture_noise", noiseTextureUnit);
	programs.ambientOcclusion->setUniform("noiseTextureScale", vector2ToFloat(_aoBuffer->size()) / _noiseTexture->sizeFloat());
	programs.ambientOcclusion->setUniform("texel", vec2(1.0f) / vector2ToFloat(_geometryBuffer->size()));
	
	programs.ambientOcclusionBlur = _rc->programFactory().loadProgram("data/shaders/blur.program", localCache);
	programs.ambientOcclusionBlur->setUniform("texture_depth", depthTextureUnit);
	programs.ambientOcclusionBlur->setUniform("texture_color", diffuseTextureUnit);
	programs.ambientOcclusionBlur->setUniform("texture_normal", normalTextureUnit);
	
	programs.final = _rc->programFactory().loadProgram("data/shaders/final.program", localCache);
	programs.final->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.final->setUniform("texture_normal", normalTextureUnit);
	programs.final->setUniform("texture_depth", depthTextureUnit);
	programs.final->setUniform("texture_occlusion", occlusionTextureUnit);
	programs.final->setUniform("texture_noise", noiseTextureUnit);
	programs.final->setUniform("noiseTextureScale", vector2ToFloat(_aoBuffer->size()) / _noiseTexture->sizeFloat());
}

void SceneRenderer::setScene(et::s3d::Scene::Pointer aScene)
{
	_allObjects.clear();
	
	_scene = aScene;
	
	auto meshes = _scene->childrenOfType(s3d::ElementType_SupportMesh);
	for (auto e : meshes)
		_allObjects.push_back(e);
	
	for (s3d::SupportMesh::Pointer& e : _allObjects)
	{
		auto mat = e->material();
		for (size_t i = MaterialParameter_AmbientMap; i < MaterialParameter_AmbientFactor; ++i)
		{
			Texture::Pointer tex = mat->getTexture(i);
			if (tex.valid())
				tex->setAnisotropyLevel(_rc, RenderingCapabilities::instance().maxAnisotropyLevel());
		}
	}
	
	log::info("Scene set. %llu objects to render", static_cast<uint64_t>(_allObjects.size()));
}

void SceneRenderer::renderToGeometryBuffer(const et::Camera& cam, const AOParameters& params)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setDepthMask(true);
	rs.setDepthTest(true);
	
	rs.setSampleAlphaToCoverage(true);
		
	rs.bindFramebuffer(_geometryBuffer);
	rn->clear(true, true);

	_geometryBuffer->setDrawBuffersCount(2);

	rs.bindProgram(programs.prepass);
	programs.prepass->setCameraProperties(cam);

	for (s3d::SupportMesh::Pointer& e : _allObjects)
	{
		if (cam.frustum().containsAABB(e->aabb()))
		{
			const auto& mat = e->material();
			
			programs.prepass->setTransformMatrix(e->finalTransform());
			programs.prepass->setUniform("diffuseColor", mat->getVector(MaterialParameter_DiffuseColor));
			
			rs.bindVertexArray(e->vertexArrayObject());
			
			if (params.showDiffuse && mat->hasTexture(MaterialParameter_DiffuseMap))
				rs.bindTexture(diffuseTextureUnit, mat->getTexture(MaterialParameter_DiffuseMap));
			else
				rs.bindTexture(diffuseTextureUnit, _defaultTexture);
			
			if (mat->hasTexture(MaterialParameter_NormalMap))
				rs.bindTexture(normalTextureUnit, mat->getTexture(MaterialParameter_NormalMap));
			else
				rs.bindTexture(normalTextureUnit, _defaultNormalTexture);
			
			if (mat->hasTexture(MaterialParameter_TransparencyMap))
				rs.bindTexture(transparencyTextureUnit, mat->getTexture(MaterialParameter_TransparencyMap));
			else
				rs.bindTexture(transparencyTextureUnit, _defaultTexture);
			
			rn->drawElements(e->indexBuffer(), e->startIndex(), e->numIndexes());
		}
	}

	rs.setSampleAlphaToCoverage(false);
}

void SceneRenderer::computeAmbientOcclusion(const et::Camera& cam, const AOParameters& params)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setDepthMask(false);
	rs.setDepthTest(false);
	rs.bindFramebuffer(_aoBuffer);
	
	_aoBuffer->setCurrentRenderTarget(0);
	
	rs.bindTexture(noiseTextureUnit, _noiseTexture);
	rs.bindTexture(diffuseTextureUnit, _geometryBuffer->renderTarget(0));
	rs.bindTexture(normalTextureUnit, _geometryBuffer->renderTarget(1));
	rs.bindTexture(depthTextureUnit, _geometryBuffer->depthBuffer());
	
	rs.bindProgram(programs.ambientOcclusion);
	programs.ambientOcclusion->setUniform("numSamples", params.numSamples);
	programs.ambientOcclusion->setUniform("depthDifferenceScale", params.depthDifference);
	programs.ambientOcclusion->setUniform("sampleScale", params.sampleScale);
	programs.ambientOcclusion->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.ambientOcclusion->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	rn->fullscreenPass();

	if (params.performBlur)
	{
		rs.bindProgram(programs.ambientOcclusionBlur);
		programs.ambientOcclusionBlur->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
		programs.ambientOcclusionBlur->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
		programs.ambientOcclusionBlur->setUniform("depthDifferenceScale", params.depthDifference);

		_aoBuffer->setCurrentRenderTarget(1);
		rs.bindTexture(diffuseTextureUnit, _aoBuffer->renderTarget(0));
		programs.ambientOcclusionBlur->setUniform("direction", _aoBuffer->renderTarget(0)->texel() * vec2(1.0, 0.0));
		rn->fullscreenPass();

		_aoBuffer->setCurrentRenderTarget(0);
		rs.bindTexture(diffuseTextureUnit, _aoBuffer->renderTarget(1));
		programs.ambientOcclusionBlur->setUniform("direction", _aoBuffer->renderTarget(1)->texel() * vec2(0.0, 1.0));
		rn->fullscreenPass();
	}
}

void SceneRenderer::handlePressedKey(size_t key)
{
}

void SceneRenderer::render(const et::Camera& cam, const AOParameters& params, bool obs)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setBlend(false);

	renderToGeometryBuffer(cam, params);

	computeAmbientOcclusion(cam, params);
	
	rs.bindDefaultFramebuffer();

	rs.bindProgram(programs.final);
	
	programs.final->setCameraProperties(cam);
	programs.final->setUniform("aoPower", params.aoPower);
	programs.final->setUniform("texel", _geometryBuffer->renderTarget(0)->texel());
	programs.final->setUniform("mProjection", cam.projectionMatrix());
	programs.final->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.final->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	
	rs.bindTexture(diffuseTextureUnit, _geometryBuffer->renderTarget(0));
	rs.bindTexture(normalTextureUnit, _geometryBuffer->renderTarget(1));
	rs.bindTexture(depthTextureUnit, _geometryBuffer->depthBuffer());
	rs.bindTexture(occlusionTextureUnit, _aoBuffer->renderTarget(0));
	rs.bindTexture(noiseTextureUnit, _noiseTexture);
	rn->fullscreenPass();
}
