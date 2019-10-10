/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.hpp>

namespace et {
namespace s2d {
class Line : public Element2d {
 public:
  ET_DECLARE_POINTER(Line);

  enum Type { Type_Linear, Type_QuadraticBezier };

 public:
  Line(const vec2&, const vec2&, Element2d*);

  void setType(Type);
  void setControlPoint(size_t index, const vec2&);
  void setWidth(float);

  void setGradientColors(const vec4&, const vec4&);

  void setShadowColor(const vec4&);
  void setShadowOffset(const vec2&);

 private:
  void addToRenderQueue(RenderInterface::Pointer&, SceneRenderer&) override;
  void buildVertices(RenderInterface::Pointer&, SceneRenderer&) override;

  void buildLine(const vec2&, const vec2&, const vec4&, const vec4&, const vec4&, const mat4&);

 private:
  std::vector<vec2> _controlPoints;
  SceneVertexList _vertices;
  vec4 _startColor = vec4(1.0f);
  vec4 _endColor = vec4(1.0f);
  vec4 _shadowColor = vec4(0.0f);
  vec2 _shadowOffset = vec2(0.0f);
  float _width = 1.0f;
  Type _type = Type_Linear;
};
}  // namespace s2d
}  // namespace et
