/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/fontbase.h>
#include <et-ext/scene2d/guibaseclasses.h>

namespace et
{
	namespace s2d
	{
		size_t measuseVertexCountForImageDescriptor(const ImageDescriptor& desc);
		
		void buildQuad(SceneVertexList& vertices, const SceneVertex& topLeft, const SceneVertex& topRight,
			const SceneVertex& bottomLeft, const SceneVertex& bottomRight);
		
		void buildStringVertices(SceneVertexList& vertices, const CharDescriptorList& chars,  Alignment hAlign,
			Alignment vAlign, const vec2& pos, const vec4& color, const mat4& transform);

		void buildImageVertices(SceneVertexList& vertices, const Texture& tex, const ImageDescriptor& desc,
			const rect& p, const vec4& color, const mat4& transform);

		void buildColorVertices(SceneVertexList& vertices, const rect& p, const vec4& color,
			const mat4& transform);
	}
}
