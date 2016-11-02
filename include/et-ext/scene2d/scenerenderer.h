/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <stack>
#include <et/core/objectscache.h>
#include <et/core/containers.h>
#include <et/camera/camera.h>
#include <et-ext/scene2d/renderingelement.h>
#include <et-ext/scene2d/vertexbuilder.h>

namespace et
{
namespace s2d
{
class SceneRenderer
{
public:
	SceneRenderer(RenderContext* rc);
	~SceneRenderer();

	void beginRender(RenderContext* rc);
	void render(RenderContext* rc);
	void endRender(RenderContext* rc);

	void resetClipRect();
	void pushClipRect(const recti&);
	void popClipRect();

	void setProjectionMatrices(const vec2& contextSize);
	void setRenderingElement(const RenderingElement::Pointer& r);

	void addVertices(const SceneVertexList&, const Texture::Pointer, const MaterialInstance::Pointer&,
		Element2d*, PrimitiveType = PrimitiveType::Triangles);
	
	void setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha);

	Texture::Pointer transparentTexture();
	Material::Pointer defaultMaterial();

private:
	ET_DENY_COPY(SceneRenderer);

	void init(RenderContext* rc);
	SceneVertex* allocateVertices(uint32_t, const Texture::Pointer, const MaterialInstance::Pointer&,
		Element2d*, PrimitiveType);

private:
	RenderContext* _rc = nullptr;
	RenderingElement::Pointer _renderingElement;
	RenderPass::Pointer _renderPass;
	Camera::Pointer _sceneCamera = Camera::Pointer::create();
	Material::Pointer _defaultMaterial;
	Texture::Pointer _transparentTexture;
	std::stack<recti> _clip;

	// mat4 _defaultTransform;
	vec3 _additionalOffsetAndAlpha;
	recti _additionalWindowOffset;
};

inline Texture::Pointer SceneRenderer::transparentTexture() { return _transparentTexture; }
inline Material::Pointer SceneRenderer::defaultMaterial() { return _defaultMaterial; }

}
}
