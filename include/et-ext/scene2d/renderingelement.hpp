/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/base/vertexstream.hpp>
#include <et/scene2d/element2d.hpp>

namespace et {
namespace s2d {
struct RenderChunk {
  uint32_t first = 0;
  uint32_t count = 0;
  PrimitiveType primitiveType = PrimitiveType::Triangles;
  recti clip;
  MaterialInstance::Pointer material;
  Element2d* object = nullptr;

  RenderChunk(uint32_t aFirst, uint32_t aCount, const recti& aClip, const MaterialInstance::Pointer& aMaterial, Element2d* aObject, PrimitiveType pt);
};

class RenderingElement : public Object {
 public:
  ET_DECLARE_POINTER(RenderingElement);

  enum : uint32_t {
    MaxCapacity = 65536,
  };

 public:
  RenderingElement(RenderInterface::Pointer& rc, uint32_t capacity);
  ~RenderingElement();

  void startAllocatingVertices();
  SceneVertex* allocateVertices(uint32_t);
  void commitAllocatedVertices();

  void clear();

  const VertexStream::Pointer& vertexStream();

 private:
  friend class SceneRenderer;
  enum : uint32_t { VertexBuffersCount = 3 };

  union {
    void* vertexData = nullptr;
    SceneVertex* mappedVertices;
  };

  std::vector<RenderChunk, SharedBlockAllocatorSTDProxy<RenderChunk>> chunks;
  VertexStream::Pointer vertices[VertexBuffersCount];
  uint32_t allocatedVertices = 0;
  uint32_t currentBufferIndex = 0;
  uint32_t dataSize = 0;
};
}  // namespace s2d
}  // namespace et
