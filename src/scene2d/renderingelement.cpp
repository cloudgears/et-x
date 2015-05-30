/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
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
RenderChunk::RenderChunk(size_t aFirst, size_t aCount, const recti& aClip, const Texture::Pointer& aTexture,
	const SceneProgram& aProgram, Element2d* aObject, PrimitiveType pt) : first(aFirst), count(aCount), clip(aClip),
	texture(aTexture), program(aProgram), object(aObject), primitiveType(pt) { }

/*
 * Rendering element
 */
RenderingElement::RenderingElement(RenderContext* rc, size_t capacity) :
	renderState(rc->renderState())
{
	auto indexArray = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, capacity, PrimitiveType::Triangles);
	indexArray->linearize(capacity);
	
	VertexDeclaration decl(true, VertexAttributeUsage::TexCoord0, VertexAttributeType::Vec4);
	decl.push_back(VertexAttributeUsage::Color, VertexAttributeType::Vec4);
	decl.push_back(VertexAttributeUsage::Position, VertexAttributeType::Vec3);
	dataSize = decl.dataSize() * capacity;
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER == 0)
	mappedData = sharedBlockAllocator().alloc(dataSize);
#endif
	
	std::string nameId = intToStr(reinterpret_cast<size_t>(this)) + "-vao";
	vao = rc->vertexBufferFactory().createVertexArrayObject(nameId, VertexArray::Pointer::create(decl, capacity),
		BufferDrawType::Stream, indexArray, BufferDrawType::Static);
}

RenderingElement::~RenderingElement()
{
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER == 0)
	sharedBlockAllocator().free(mappedData);
#endif
}

void RenderingElement::clear()
{
	chunks.clear();
	allocatedVertices = 0;
}

void RenderingElement::startAllocatingVertices()
{
	clear();
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	renderState.bindVertexArray(vao);
	mappedData = vao->vertexBuffer()->map(0, dataSize, MapBufferMode::WriteOnly);
#endif
}

SceneVertex* RenderingElement::allocateVertices(size_t n)
{
	auto result = mappedVertices + allocatedVertices;
	allocatedVertices += n;
	return result;
}

void RenderingElement::commitAllocatedVertices()
{
	renderState.bindVertexArray(vao);
	
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	vao->vertexBuffer()->unmap();
#else
	vao->vertexBuffer()->setData(mappedData, sizeof(SceneVertex) * allocatedVertices);
#endif
}

const VertexArrayObject& RenderingElement::vertexArrayObject()
{
#if (ET_RENDER_CHUNK_USE_MAP_BUFFER)
	ET_ASSERT(!vao->vertexBuffer()->mapped());
#endif
	
	renderState.bindVertexArray(vao);
	return vao;
}
