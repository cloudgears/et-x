/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/renderingelement.h>

using namespace et;
using namespace et::s2d;

#define ET_RENDER_CHUNK_USE_MAP_BUFFER	1

/*
 * Render chunk
 */
RenderChunk::RenderChunk(uint32_t aFirst, uint32_t aCount, const recti& aClip, const Texture::Pointer& aTexture,
	const SceneProgram& aProgram, Element2d* aObject, PrimitiveType pt) : first(aFirst), count(aCount), clip(aClip),
	texture(aTexture), program(aProgram), object(aObject), primitiveType(pt) { }

/*
 * Rendering element
 */
RenderingElement::RenderingElement(RenderContext* rc, uint32_t capacity) :
	renderState(rc->renderState())
{
	auto indexArray = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, capacity, PrimitiveType::Triangles);
	indexArray->linearize(capacity);
	
	VertexDeclaration decl(true, VertexAttributeUsage::TexCoord0, DataType::Vec4);
	decl.push_back(VertexAttributeUsage::Color, DataType::Vec4);
	decl.push_back(VertexAttributeUsage::Position, DataType::Vec3);
	dataSize = decl.dataSize() * capacity;
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER == 0)
	vertexData = sharedBlockAllocator().alloc(dataSize);
#endif
	
	IndexBuffer::Pointer sharedIndexBuffer;
	VertexArray::Pointer sharedVertexArray = VertexArray::Pointer::create(decl, capacity);
	
	auto nameId = intToStr(reinterpret_cast<size_t>(this)) + "-vao-1";
	vertices[0] = rc->vertexBufferFactory().createVertexArrayObject(nameId,
		sharedVertexArray, BufferDrawType::Stream, indexArray, BufferDrawType::Static);
	sharedIndexBuffer = vertices[0]->indexBuffer();
	
	for (size_t i = 1; i < VertexBuffersCount; ++i)
	{
		nameId = intToStr(reinterpret_cast<size_t>(this)) + "-vao-" + intToStr(i + 1);
		vertices[i] = rc->vertexBufferFactory().createVertexArrayObject(nameId);
		auto vb = rc->vertexBufferFactory().createVertexBuffer(nameId + "-vb",
			sharedVertexArray, BufferDrawType::Stream);
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
	renderState.bindVertexArray(vertices[currentBufferIndex]);
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
	renderState.bindVertexArray(vao);
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	vao->vertexBuffer()->unmap();
#else
	vao->vertexBuffer()->setData(vertexData, sizeof(SceneVertex) * allocatedVertices);
#endif
}

const VertexArrayObject& RenderingElement::vertexArrayObject()
{
	const auto& vao = vertices[currentBufferIndex];
	renderState.bindVertexArray(vao);
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	ET_ASSERT(!vao->vertexBuffer()->mapped());
#endif
	
	return vao;
}
