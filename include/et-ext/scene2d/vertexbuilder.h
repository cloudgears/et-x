

/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/fontbase.h>
#include <et-ext/scene2d/baseclasses.h>

namespace et
{
namespace s2d
{
uint32_t measuseVertexCountForImageDescriptor(const ImageDescriptor& desc);

void buildQuad(SceneVertexList& vertices, const SceneVertex& topLeft, const SceneVertex& topRight,
	const SceneVertex& bottomLeft, const SceneVertex& bottomRight);

void buildQuad(SceneVertexList& vertices, SceneVertex topLeft, SceneVertex topRight,
	SceneVertex bottomLeft, SceneVertex bottomRight, const mat4& t);

void buildStringVertices(SceneVertexList& vertices, const CharDescriptorList& chars, Alignment hAlign,
	Alignment vAlign, const vec2& pos, const vec4& color, const mat4& transform, float lineInterval = 1.0f);

void buildImageVertices(SceneVertexList& vertices, const Texture::Pointer& tex, const ImageDescriptor& desc,
	const rectf& p, const vec4& color, const mat4& transform);

void buildColorVertices(SceneVertexList& vertices, const rectf& p, const vec4& color,
	const mat4& transform);
}
}
