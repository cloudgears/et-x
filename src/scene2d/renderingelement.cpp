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

/*
 * Render chunk
 */
RenderChunk::RenderChunk(size_t aFirst, size_t aCount, const recti& aClip, const Texture& aTexture,
	const SceneProgram& aProgram, Element2d* aObject, PrimitiveType pt) : first(aFirst), count(aCount), clip(aClip),
	texture(aTexture), program(aProgram), object(aObject), primitiveType(primitiveTypeValue(pt)) { }

/*
 * Rendering element
 */
RenderingElement::RenderingElement(RenderContext* rc, size_t capacity) :
	renderState(rc->renderState())
{
	auto indexArray = IndexArray::Pointer::create(IndexArrayFormat_16bit, capacity, PrimitiveType_Triangles);
	indexArray->linearize(capacity);
	
	VertexDeclaration decl(true, Usage_Position, Type_Vec3);
	decl.push_back(Usage_TexCoord0, Type_Vec4);
	decl.push_back(Usage_Color, Type_Vec4);
	dataSize = decl.dataSize() * capacity;
	
	std::string nameId = intToStr(reinterpret_cast<size_t>(this)) + "-vao";

	vao = rc->vertexBufferFactory().createVertexArrayObject(nameId, VertexArray::Pointer::create(decl, capacity),
		BufferDrawType_Stream, indexArray, BufferDrawType_Static);
}

void RenderingElement::clear()
{
	chunks.clear();
	allocatedVertices = 0;
}

void RenderingElement::startAllocatingVertices()
{
	clear();
	
	renderState.bindVertexArray(vao);
	mappedData = vao->vertexBuffer()->map(0, dataSize, VertexBufferData::MapBufferMode_WriteOnly);
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
	vao->vertexBuffer()->unmap();
}

const VertexArrayObject& RenderingElement::vertexArrayObject()
{
	renderState.bindVertexArray(vao);
	return vao;
}
