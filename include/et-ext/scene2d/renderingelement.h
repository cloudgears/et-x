/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/apiobjects/vertexarrayobject.h>
#include <et-ext/scene2d/guibase.h>

namespace et
{
	namespace s2d
	{
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

		private:
			RenderState& _rs;
			RenderChunkList _chunks;
			IndexArray::Pointer _indexArray;
			GuiVertexList _vertexList;

			VertexArrayObject _vao;

			bool _changed;
		};
	}
}
