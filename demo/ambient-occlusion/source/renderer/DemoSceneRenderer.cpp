//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#include <et/opengl/opengl.h>
#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include "DemoSceneRenderer.h"

using namespace et;
using namespace demo;

#define ENABLE_DEBUG_RENDERING		0
#define NUM_SAMPLES					64
#define INTERLEAVE_X				4
#define INTERLEAVE_Y				4
#define TOTAL_INTERLEAVE_FRAMES		INTERLEAVE_X * INTERLEAVE_Y

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

	auto bufferSize = _rc->sizei();

	vec2i downsampledBufferSize
	(
		(bufferSize.x + INTERLEAVE_X - 1) / INTERLEAVE_X, 
		(bufferSize.y + INTERLEAVE_Y - 1) / INTERLEAVE_Y
	);
	
	log::info("Context size: %dx%d, downsampled size: %dx%d", bufferSize.x, bufferSize.y,
		downsampledBufferSize.x, downsampledBufferSize.y);
	
	_geometryBuffer = rc->framebufferFactory().createFramebuffer(bufferSize, TextureTarget::Texture_Rectangle, "geometry-buffer");
	_geometryBuffer->renderTarget(0)->setFiltration(rc, TextureFiltration::Nearest, TextureFiltration::Nearest);

	_geometryBuffer->addSameRendertarget();
	_geometryBuffer->renderTarget(1)->setFiltration(rc, TextureFiltration::Nearest, TextureFiltration::Nearest);

	_geometryBuffer->depthBuffer()->setFiltration(rc, TextureFiltration::Nearest, TextureFiltration::Nearest);

	_interleavedFramebuffer = rc->framebufferFactory().createFramebuffer(downsampledBufferSize, 
		TextureTarget::Texture_2D_Array, "interleaved-fbo", TextureFormat::RGBA32F, TextureFormat::RGBA, 
		DataType::Float, TextureFormat::Invalid, TextureFormat::Invalid, DataType::Char, TOTAL_INTERLEAVE_FRAMES);
	_interleavedFramebuffer->renderTarget()->setFiltration(rc, TextureFiltration::Nearest, TextureFiltration::Nearest);
	
	_aoFramebuffer = rc->framebufferFactory().createFramebuffer(downsampledBufferSize, TextureTarget::Texture_2D_Array, 
		"ao-buffer", TextureFormat::RGBA, TextureFormat::RGBA, DataType::UnsignedChar, 
		TextureFormat::Invalid, TextureFormat::Invalid, DataType::Char, TOTAL_INTERLEAVE_FRAMES);
	_aoFramebuffer->renderTarget(0)->setFiltration(rc, TextureFiltration::Nearest, TextureFiltration::Nearest);
	_aoFramebuffer->addSameRendertarget();
	_aoFramebuffer->renderTarget(1)->setFiltration(rc, TextureFiltration::Nearest, TextureFiltration::Nearest);

	_finalFramebuffer = rc->framebufferFactory().createFramebuffer(bufferSize, TextureTarget::Texture_Rectangle, 
		"final-buffer",  TextureFormat::RGBA, TextureFormat::RGBA, DataType::UnsignedChar, TextureFormat::Invalid);
	_finalFramebuffer->addSameRendertarget();

	_defaultTexture = _rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA,
		vec2i(1), TextureFormat::RGBA, DataType::UnsignedChar, BinaryDataStorage(4, 255), "white-texture");
	
	_noiseTexture = _rc->textureFactory().genNoiseTexture(vec2i(4), false, "noise-texture");
	
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
	programs.ambientOcclusion->setUniform("noiseTextureScale", vector2ToFloat(_aoFramebuffer->size()) / _noiseTexture->sizeFloat());
	programs.ambientOcclusion->setUniform("texel", vec2(1.0f) / vector2ToFloat(_geometryBuffer->size()));

	programs.ambientOcclusionFixed = _rc->programFactory().loadProgram("data/shaders/ao-fixed.program", localCache);
	programs.ambientOcclusionFixed->setUniform("texture_depth", depthTextureUnit);
	programs.ambientOcclusionFixed->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.ambientOcclusionFixed->setUniform("texture_normal", normalTextureUnit);
	programs.ambientOcclusionFixed->setUniform("texture_noise", noiseTextureUnit);
	programs.ambientOcclusionFixed->setUniform("noiseTextureScale", vector2ToFloat(_aoFramebuffer->size()) / _noiseTexture->sizeFloat());
	programs.ambientOcclusionFixed->setUniform("texel", vec2(1.0f) / vector2ToFloat(_geometryBuffer->size()));

	programs.ambientOcclusionBlur = _rc->programFactory().loadProgram("data/shaders/blur.program", localCache);
	
	programs.final = _rc->programFactory().loadProgram("data/shaders/final.program", localCache);
	programs.final->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.final->setUniform("texture_normal", normalTextureUnit);
	programs.final->setUniform("texture_depth", depthTextureUnit);
	programs.final->setUniform("texture_occlusion", occlusionTextureUnit);
	programs.final->setUniform("texture_noise", noiseTextureUnit);
	programs.final->setUniform("noiseTextureScale", vector2ToFloat(_aoFramebuffer->size()) / _noiseTexture->sizeFloat());

	programs.interleave = _rc->programFactory().loadProgram("data/shaders/interleave.program", localCache);
	programs.interleave->setUniform("interleave_x", INTERLEAVE_X);
	programs.interleave->setUniform("interleave_y", INTERLEAVE_Y);

	programs.deinterleave = _rc->programFactory().loadProgram("data/shaders/deinterleave.program", localCache);
	programs.deinterleave->setUniform("texture_depth", depthTextureUnit);
	programs.deinterleave->setUniform("texture_normal", normalTextureUnit);

	for (size_t i = 0; i < TOTAL_INTERLEAVE_FRAMES; ++i)
	{
		_jitterSamples.emplace_back();
		auto& lastSampples = _jitterSamples.back();

		lastSampples.reserve(NUM_SAMPLES);
		for (size_t s = 0; s < NUM_SAMPLES; ++s)
		{
			vec3 r = randomVectorOnHemisphere(-unitZ, HALF_PI);
			auto d = randomFloat(0.01f, 0.5f);

			lastSampples.push_back(d * r);
		}
	}
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
		if (cam.frustum().containsAABB(e->boundingBox()))
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
	rs.bindTexture(0, _interleavedFramebuffer->renderTarget(0));

	rs.bindProgram(programs.ambientOcclusionFixed);
	programs.ambientOcclusionFixed->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.ambientOcclusionFixed->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	programs.ambientOcclusionFixed->setUniform("depthDifferenceScale", params.depthDifference);
	programs.ambientOcclusionFixed->setUniform("aoPower", params.aoPower);

	rs.bindFramebuffer(_aoFramebuffer);
	for (uint32_t i = 0; i < TOTAL_INTERLEAVE_FRAMES; ++i)
	{
		_aoFramebuffer->setCurrentLayer(i);
		programs.ambientOcclusionFixed->setUniform("sampleLayer", static_cast<float>(i));
		programs.ambientOcclusionFixed->setUniform<vec3>("randomSamples[0]", _jitterSamples[i].data(), _jitterSamples[i].size());
		rn->fullscreenPass();
	}
}

void SceneRenderer::handlePressedKey(size_t key)
{
}

void SceneRenderer::renderToInterleavedBuffer(const et::Camera& cam)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();

	auto arrayTextureId = _interleavedFramebuffer->renderTarget(0)->apiHandle();
	rs.bindTexture(normalTextureUnit, _geometryBuffer->renderTarget(1));
	rs.bindTexture(depthTextureUnit, _geometryBuffer->depthBuffer());
	rs.bindFramebuffer(_interleavedFramebuffer);
	rs.setDrawBuffersCount(8);
	rs.setDepthMask(false);
	rs.setDepthTest(false);

	rs.bindProgram(programs.deinterleave);
	programs.deinterleave->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.deinterleave->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	programs.deinterleave->setUniform("total_interleave", vec2(INTERLEAVE_X, INTERLEAVE_Y));
	programs.deinterleave->setUniform("current_interleave", vec2i(INTERLEAVE_X, 2)); // hardcoded for now

	for (int i = 0; i < 8; ++i)
		glFramebufferTextureLayer(GL_FRAMEBUFFER, drawBufferTarget(i), arrayTextureId, 0, i);

	programs.deinterleave->setUniform("start_position", vec2i(0, 0));
	rn->fullscreenPass();

	for (int i = 0; i < 8; ++i)
		glFramebufferTextureLayer(GL_FRAMEBUFFER, drawBufferTarget(i), arrayTextureId, 0, 8 + i);

	programs.deinterleave->setUniform("start_position", vec2i(0, 2));
	rn->fullscreenPass();
}

void SceneRenderer::interleaveAmbientOcclusion()
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();

	rs.bindFramebuffer(_finalFramebuffer);
	_finalFramebuffer->setCurrentRenderTarget(0);
	rs.bindProgram(programs.interleave);
	rs.bindTexture(0, _aoFramebuffer->renderTarget(0));
	rn->fullscreenPass();
}

void SceneRenderer::blurAmbientOcclusion(const et::Camera& cam, const AOParameters& params)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();

	rs.bindFramebuffer(_finalFramebuffer);
	rs.bindProgram(programs.ambientOcclusionBlur);
	rs.bindTexture(1, _geometryBuffer->renderTarget(1));
	rs.bindTexture(2, _geometryBuffer->depthBuffer());

	programs.ambientOcclusionBlur->setUniform("clipPlanes", vec2(cam.zNear(), cam.zFar()));
	programs.ambientOcclusionBlur->setUniform("texCoordScales", vec2(-cam.inverseProjectionMatrix()[0][0], -cam.inverseProjectionMatrix()[1][1]));
	programs.ambientOcclusionBlur->setUniform("depthDifferenceScale", params.depthDifference);

	_finalFramebuffer->setCurrentRenderTarget(1);
	programs.ambientOcclusionBlur->setUniform("direction", vec2(1.0f, 0.0f));
	rs.bindTexture(0, _finalFramebuffer->renderTarget(0));
	rn->fullscreenPass();

	_finalFramebuffer->setCurrentRenderTarget(0);
	programs.ambientOcclusionBlur->setUniform("direction", vec2(0.0f, 1.0f));
	rs.bindTexture(0, _finalFramebuffer->renderTarget(1));
	rn->fullscreenPass();
}

void SceneRenderer::render(const et::Camera& cam, const AOParameters& params, bool obs)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setBlend(false);

	renderToGeometryBuffer(cam, params);

	renderToInterleavedBuffer(cam);

	computeAmbientOcclusion(cam, params);

	interleaveAmbientOcclusion();

	if (params.performBlur)
		blurAmbientOcclusion(cam, params);
	
	rs.bindDefaultFramebuffer();
	rn->renderFullscreenTexture(_finalFramebuffer->renderTarget());

	/*
	// */

	return;

	/*
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
	rs.bindTexture(occlusionTextureUnit, _aoFramebuffer->renderTarget(0));
	rs.bindTexture(noiseTextureUnit, _noiseTexture);
	rn->fullscreenPass();
	// */
}
