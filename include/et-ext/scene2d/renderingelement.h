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
			size_t first = 0;
			size_t count = 0;
			uint32_t primitiveType = 0;
			
			recti clip;
			
			Texture texture;
			SceneProgram program;
			Element2d* object = nullptr;
			
			RenderChunk(size_t aFirst, size_t aCount, const recti& aClip, const Texture& aTexture,
				const SceneProgram& aProgram, Element2d* aObject, PrimitiveType pt);
		};
		
		class RenderingElement : public Shared
		{
		public:
			ET_DECLARE_POINTER(RenderingElement)
			
			enum
			{
				MaxCapacity = 65536
			};
			
		public:
			RenderingElement(RenderContext* rc, size_t capacity);
			
			void startAllocatingVertices();
			SceneVertex* allocateVertices(size_t);
			void commitAllocatedVertices();

			const VertexArrayObject& vertexArrayObject();

		private:
			friend class SceneRenderer;
			
			RenderState& renderState;
			std::vector<RenderChunk, SharedBlockAllocatorSTDProxy<RenderChunk>> chunks;
			
			union
			{
				SceneVertex* mappedVertices;
				void* mappedData = nullptr;
			};
			
//			SceneVertexList vertexList;
			size_t allocatedVertices = 0;
			size_t dataSize = 0;
			VertexArrayObject vao;
		};
	}
}
