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
	const SceneProgram& aProgram, Element* aObject) : first(aFirst), count(aCount), clip(aClip),
	texture(aTexture), program(aProgram), object(aObject) { }

/*
 * Rendering element
 */
RenderingElement::RenderingElement(RenderContext* rc) :
	renderState(rc->renderState()), changed(false)
{
	indexArray = IndexArray::Pointer::create(IndexArrayFormat_16bit, 0, PrimitiveType_Triangles);
	VertexDeclaration decl(true, Usage_Position, Type_Vec3);
	decl.push_back(Usage_TexCoord0, Type_Vec4);
	decl.push_back(Usage_Color, Type_Vec4);
	
	std::string nameId = intToStr(reinterpret_cast<size_t>(this)) + "-vao";

	vao = rc->vertexBufferFactory().createVertexArrayObject(nameId, VertexArray::Pointer::create(decl, true),
		BufferDrawType_Stream, indexArray, BufferDrawType_Static);
}

void RenderingElement::clear()
{
	vertexList.setOffset(0);
	indexArray->setActualSize(0);
	chunks.clear();
	changed = true;
}

const VertexArrayObject& RenderingElement::vertexArrayObject()
{
	renderState.bindVertexArray(vao);

	if (changed)
	{
		size_t count = vertexList.lastElementIndex();
		indexArray->setActualSize(count);
		vao->vertexBuffer()->setData(vertexList.data(), count * vertexList.typeSize());
		vao->indexBuffer()->setData(indexArray);
		changed = false;
	}

	return vao;
}
