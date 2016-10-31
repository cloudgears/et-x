/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/renderingelement.h>

namespace et {
namespace s2d {

#define ET_RENDER_CHUNK_USE_MAP_BUFFER	1

/*
 * Render chunk
 */
RenderChunk::RenderChunk(uint32_t aFirst, uint32_t aCount, const recti& aClip, const Texture::Pointer& aTexture,
	const SceneProgram& aProgram, Element2d* aObject, PrimitiveType pt) : first(aFirst), count(aCount),
	primitiveType(pt), clip(aClip), texture(aTexture), program(aProgram), object(aObject) { }

/*
 * Rendering element
 */
RenderingElement::RenderingElement(RenderContext* rc, uint32_t capacity)
{
	auto indexArray = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, capacity, PrimitiveType::Triangles);
	indexArray->linearize(capacity);
	
	VertexDeclaration decl(true, VertexAttributeUsage::TexCoord0, DataType::Vec4);
	decl.push_back(VertexAttributeUsage::Color, DataType::Vec4);
	decl.push_back(VertexAttributeUsage::Position, DataType::Vec3);
	dataSize = decl.totalSize() * capacity;
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER == 0)
	vertexData = sharedBlockAllocator().alloc(dataSize);
#endif
	
	IndexBuffer::Pointer sharedIndexBuffer = rc->renderer()->createIndexBuffer("dynamic-buffer-ib", indexArray, BufferDrawType::Static);
	VertexStorage::Pointer sharedVertexStorage = VertexStorage::Pointer::create(decl, capacity);
	
	sharedIndexBuffer = vertices[0]->indexBuffer();
	
	for (uint32_t i = 0; i < VertexBuffersCount; ++i)
	{
		char nameId[128] = { };
		sprintf(nameId, "dynamic-buffer-%u-ib", i);
		vertices[i] = VertexStream::Pointer::create();
		auto vb = rc->renderer()->createVertexBuffer(nameId, sharedVertexStorage, BufferDrawType::Dynamic);
		vertices[i]->setBuffers(vb, sharedIndexBuffer);
	}
	currentBufferIndex = 0;
}

RenderingElement::~RenderingElement()
{
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER == 0)
	sharedBlockAllocator().free(vertexData);
#endif
}

void RenderingElement::clear()
{
	chunks.clear();
	allocatedVertices = 0;
}

void RenderingElement::startAllocatingVertices()
{
	currentBufferIndex = (currentBufferIndex + 1) % VertexBuffersCount;
	clear();
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	vertexData = vertices[currentBufferIndex]->vertexBuffer()->map(0, dataSize,
		MapBufferOptions::Write | MapBufferOptions::InvalidateRange);
#endif
}

SceneVertex* RenderingElement::allocateVertices(uint32_t n)
{
	auto result = mappedVertices + allocatedVertices;
	allocatedVertices += n;
	return result;
}

void RenderingElement::commitAllocatedVertices()
{
	auto vao = vertices[currentBufferIndex];
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	vao->vertexBuffer()->unmap();
#else
	vao->vertexBuffer()->setData(vertexData, sizeof(SceneVertex) * allocatedVertices);
#endif
}

const VertexStream::Pointer& RenderingElement::VertexStream()
{
	const auto& vao = vertices[currentBufferIndex];
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	ET_ASSERT(!vao->vertexBuffer()->mapped());
#endif
	return vao;
}

}
}
