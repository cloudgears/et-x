/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/vertexarrayobject.h>
#include <et-ext/scene2d/element2d.h>

namespace et
{
	namespace s2d
	{
		struct RenderChunk
		{
			size_t first;
			size_t count;
			recti clip;
			
			Texture texture;
			SceneProgram program;
			Element2d* object;
						
			RenderChunk(size_t aFirst, size_t aCount, const recti& aClip, const Texture& aTexture,
				const SceneProgram& aProgram, Element2d* aObject);
		};
		
		class RenderingElement : public Shared
		{
		public:
			ET_DECLARE_POINTER(RenderingElement)
			
		public:
			RenderingElement(RenderContext* rc);
			void clear();

			const VertexArrayObject& vertexArrayObject();

		private:
			friend class SceneRenderer;
			
			RenderState& renderState;
			std::vector<RenderChunk> chunks;
			
			IndexArray::Pointer indexArray;
			SceneVertexList vertexList;
			
			VertexArrayObject vao;
			bool changed = false;
		};
	}
}
