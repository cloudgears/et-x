/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.hpp>
#include <et/scene2d/renderingelement.hpp>

namespace et {
namespace s2d {

#define ET_RENDER_CHUNK_USE_MAP_BUFFER 1

/*
 * Render chunk
 */
RenderChunk::RenderChunk(uint32_t aFirst, uint32_t aCount, const recti& aClip, const MaterialInstance::Pointer& aMaterial, Element2d* aObject, PrimitiveType pt)
  : first(aFirst)
  , count(aCount)
  , primitiveType(pt)
  , clip(aClip)
  , material(aMaterial)
  , object(aObject) {}

/*
 * Rendering element
 */
RenderingElement::RenderingElement(RenderInterface::Pointer& rc, uint32_t capacity) {
  auto indexArray = IndexArray::make_pointer(IndexArrayFormat::Format_16bit, capacity, PrimitiveType::Triangles);
  indexArray->linearize(capacity);

  VertexDeclaration decl;
  decl.push_back(VertexAttributeUsage::Position, DataType::Vec4);
  decl.push_back(VertexAttributeUsage::Color, DataType::Vec4);
  decl.push_back(VertexAttributeUsage::TexCoord0, DataType::Vec4);
  dataSize = decl.sizeInBytes() * capacity;

#if (ET_RENDER_CHUNK_USE_MAP_BUFFER == 0)
  vertexData = sharedBlockAllocator().alloc(dataSize);
#endif

  Buffer::Pointer sharedIndexBuffer = rc->createIndexBuffer("dynamic-buffer-ib", indexArray, Buffer::Location::Device);
  VertexStorage::Pointer sharedVertexStorage = VertexStorage::make_pointer(decl, capacity);
  for (uint32_t i = 0; i < VertexBuffersCount; ++i) {
    char nameId[128] = {};
    sprintf(nameId, "dynamic-buffer-%u-ib", i);
    Buffer::Pointer vb = rc->createVertexBuffer(nameId, sharedVertexStorage, Buffer::Location::Host);

    vertices[i] = VertexStream::make_pointer();
    vertices[i]->setVertexBuffer(vb, sharedVertexStorage->declaration());
    vertices[i]->setIndexBuffer(sharedIndexBuffer, indexArray->format());
    vertices[i]->setPrimitiveType(indexArray->primitiveType());
  }
  currentBufferIndex = 0;
}

RenderingElement::~RenderingElement() {
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER == 0)
  sharedBlockAllocator().free(vertexData);
#endif
}

void RenderingElement::clear() {
  chunks.clear();
  allocatedVertices = 0;
}

void RenderingElement::startAllocatingVertices() {
  currentBufferIndex = (currentBufferIndex + 1) % VertexBuffersCount;
  clear();

#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
  vertexData = vertices[currentBufferIndex]->vertexBuffer()->map(0, dataSize);
#endif
}

SceneVertex* RenderingElement::allocateVertices(uint32_t n) {
  auto result = mappedVertices + allocatedVertices;
  allocatedVertices += n;
  return result;
}

void RenderingElement::commitAllocatedVertices() {
  VertexStream::Pointer vao = vertices[currentBufferIndex];
  uint32_t modifiedRange = sizeof(SceneVertex) * allocatedVertices;

#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
  vertices[currentBufferIndex]->vertexBuffer()->modifyRange(0, modifiedRange);
  vao->vertexBuffer()->unmap();
#else
  vao->vertexBuffer()->setData(vertexData, modifiedRange);
#endif
}

const VertexStream::Pointer& RenderingElement::vertexStream() {
  const auto& vao = vertices[currentBufferIndex];
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
  ET_ASSERT(!vao->vertexBuffer()->mapped());
#endif
  return vao;
}

}  // namespace s2d
}  // namespace et
