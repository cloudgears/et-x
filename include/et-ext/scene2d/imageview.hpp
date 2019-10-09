/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/scene2d/element2d.hpp>
#include <et/scene2d/scenerenderer.hpp>

namespace et {
namespace s2d {
class ImageView : public Element2d {
 public:
  ET_DECLARE_POINTER(ImageView);

 public:
  enum ContentMode {
    ContentMode_Stretch,
    ContentMode_Center,
    ContentMode_Fit,
    ContentMode_FitAnyway,
    ContentMode_Fill,
    ContentMode_Tile,
    ContentMode_Crop,
  };

 public:
  ImageView(Element2d* parent, const std::string& name = emptyString);

  ImageView(const Texture::Pointer& texture, Element2d* parent, const std::string& name = emptyString);

  ImageView(const Texture::Pointer& texture, const ImageDescriptor& i, Element2d* parent, const std::string& name = emptyString);

  ImageView(const Image&, Element2d* parent, const std::string& name = emptyString);

  const Texture::Pointer& texture() const {
    return _texture;
  }

  Texture::Pointer& texture() {
    return _texture;
  }

  const ImageDescriptor& imageDescriptor() const {
    return _descriptor.value();
  }

  const vec4& backgroundColor() const {
    return _backgroundColor;
  }

  void setImageDescriptor(const ImageDescriptor& d, float duration = 0.0f);
  void setContentMode(ImageView::ContentMode cm);
  void setTexture(const Texture::Pointer& t, bool updateDescriptor);
  void setImage(const Image& img);
  void setBackgroundColor(const vec4& color, float duration = 0.0f);

  vec2 contentSize();

  ImageDescriptor calculateImageFrame();

  const vec2& actualImageOrigin() const {
    return _actualImageOrigin;
  }

  const vec2& actualImageSize() const {
    return _actualImageSize;
  }

 public:
  void addToRenderQueue(RenderInterface::Pointer&, SceneRenderer&) override;

 private:
  void connectEvents();

  void buildVertices(RenderInterface::Pointer&, SceneRenderer&) override;

 private:
  Texture::Pointer _texture;
  SceneVertexList _vertices;
  Animator<ImageDescriptor> _descriptor;
  Vector4Animator _backgroundColorAnimator;
  vec4 _backgroundColor = vec4(0.0f);
  vec2 _actualImageOrigin = vec2(0.0f);
  vec2 _actualImageSize = vec2(0.0f);
  ;
  ContentMode _contentMode = ContentMode_Stretch;
};

typedef std::vector<ImageView::Pointer> ImageViewList;
}  // namespace s2d
}  // namespace et
