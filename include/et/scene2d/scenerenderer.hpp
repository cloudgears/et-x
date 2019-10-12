/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene2d/renderingelement.hpp>
#include <et/scene2d/vertexbuilder.hpp>
#include <et/app/objectscache.hpp>
#include <et/camera/camera.hpp>
#include <et/core/containers.hpp>
#include <stack>

namespace et {
namespace s2d {
class SceneRenderer {
 public:
  SceneRenderer(RenderInterface::Pointer& rc, const RenderPass::ConstructionInfo&);
  ~SceneRenderer();

  void beginRender(RenderInterface::Pointer& rc);
  void render(RenderInterface::Pointer& rc);
  void endRender(RenderInterface::Pointer& rc);

  void resetClipRect();
  void pushClipRect(const recti&);
  void popClipRect();

  void setProjectionMatrices(const vec2& contextSize);
  void setRenderingElement(const RenderingElement::Pointer& r);

  void addVertices(const SceneVertexList&, const MaterialInstance::Pointer&, Element2d*, PrimitiveType = PrimitiveType::Triangles);

  void setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha);

  Texture::Pointer whiteTexture();
  Texture::Pointer transparentTexture();
  Material::Pointer defaultMaterial();
  Material::Pointer fontMaterial();

 private:
  ET_DENY_COPY(SceneRenderer);

  SceneVertex* allocateVertices(uint32_t, const MaterialInstance::Pointer&, Element2d*, PrimitiveType);

 private:
  RenderInterface::Pointer _rc;
  RenderingElement::Pointer _renderingElement;
  RenderPass::Pointer _renderPass;
  Material::Pointer _defaultMaterial;
  Material::Pointer _fontMaterial;
  Texture::Pointer _whiteTexture;
  Texture::Pointer _transparentTexture;
  std::stack<recti> _clip;

  // mat4 _defaultTransform;
  vec3 _additionalOffsetAndAlpha;
  recti _additionalWindowOffset;
};

inline Texture::Pointer SceneRenderer::whiteTexture() {
  return _whiteTexture;
}
inline Texture::Pointer SceneRenderer::transparentTexture() {
  return _transparentTexture;
}
inline Material::Pointer SceneRenderer::defaultMaterial() {
  return _defaultMaterial;
}
inline Material::Pointer SceneRenderer::fontMaterial() {
  return _fontMaterial;
}

}  // namespace s2d
}  // namespace et
