/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/vertexarrayobject.h>
#include <et-ext/scene2d/element2d.h>

namespace et
{
	namespace s2d
	{
		struct RenderChunk
		{
			uint32_t first = 0;
			uint32_t count = 0;
			PrimitiveType primitiveType = PrimitiveType::Triangles;
			
			recti clip;
			
			Texture::Pointer texture;
			SceneProgram program;
			Element2d* object = nullptr;
			
			RenderChunk(uint32_t aFirst, uint32_t aCount, const recti& aClip, const Texture::Pointer& aTexture,
				const SceneProgram& aProgram, Element2d* aObject, PrimitiveType pt);
		};
		
		class RenderingElement : public Shared
		{
		public:
			ET_DECLARE_POINTER(RenderingElement)
			
			enum : uint32_t
			{
				MaxCapacity = 65536,
			};
			
		public:
			RenderingElement(RenderContext* rc, uint32_t capacity);
			~RenderingElement();
			
			void startAllocatingVertices();
			SceneVertex* allocateVertices(size_t);
			void commitAllocatedVertices();
			
			void clear();

			const VertexArrayObject& vertexArrayObject();

		private:
			friend class SceneRenderer;
			enum : uint32_t { VertexBuffersCount = 3 };
			
			union
			{
				void* vertexData = nullptr;
				SceneVertex* mappedVertices;
			};
			
			RenderState& renderState;
			std::vector<RenderChunk, SharedBlockAllocatorSTDProxy<RenderChunk>> chunks;
			VertexArrayObject vertices[VertexBuffersCount];
			size_t allocatedVertices = 0;
			size_t dataSize = 0;
			size_t currentBufferIndex = 0;
		};
	}
}
