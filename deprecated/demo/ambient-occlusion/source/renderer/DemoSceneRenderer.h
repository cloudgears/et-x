//
//  DemoCameraController.cpp
//  Ambient Occlusion Demo
//
//  Created by Sergey Reznik on 15/12/2014.
//  Copyright (c) 2015 Cheetek. All rights reserved.
//

#pragma once

#include <et/scene3d/scene3d.h>

namespace et {
namespace demo {
struct AOParameters {
  int32_t numSamples = 32;
  float aoPower = 6.0f;
  float depthDifference = 5.0f;
  vec2 sampleScale = vec2(0.001f, 1.0f);

  bool showDiffuse = true;
  bool performBlur = false;
};

class SceneRenderer {
 public:
  void init(RenderContext*);
  void render(const Camera&, const AOParameters&, bool);

  void setScene(s3d::Scene::Pointer);

  void handlePressedKey(size_t);

 private:
  void renderToInterleavedBuffer(const Camera&);
  void renderToGeometryBuffer(const Camera&, const AOParameters&);
  void computeAmbientOcclusion(const Camera&, const AOParameters&);
  void interleaveAmbientOcclusion();
  void blurAmbientOcclusion(const Camera&, const AOParameters&);

 private:
  struct {
    Program::Pointer prepass;
    Program::Pointer ambientOcclusion;
    Program::Pointer ambientOcclusionFixed;
    Program::Pointer ambientOcclusionBlur;
    Program::Pointer final;
    Program::Pointer interleave;
    Program::Pointer deinterleave;
  } programs;

 private:
  RenderContext* _rc = nullptr;
  /*/
  Framebuffer::Pointer _geometryBuffer;
  Framebuffer::Pointer _aoFramebuffer;
  Framebuffer::Pointer _interleavedFramebuffer;
  Framebuffer::Pointer _finalFramebuffer;
  // */

  s3d::Scene::Pointer _scene;
  std::vector<s3d::Mesh::Pointer> _allObjects;

  Texture::Pointer _noiseTexture;
  Texture::Pointer _defaultTexture;
  Texture::Pointer _defaultNormalTexture;

  std::vector<std::vector<vec3>> _jitterSamples;
};
}  // namespace demo
}  // namespace et
