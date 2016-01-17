/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/geometry/geometry.h>
#include <et-ext/scene2d/element2d.h>
#include <et-ext/scene2d/vertexbuilder.h>

using namespace et;
using namespace et::s2d;

void et::s2d::buildQuad(SceneVertexList& vertices, const SceneVertex& topLeft, const SceneVertex& topRight,
	const SceneVertex& bottomLeft, const SceneVertex& bottomRight)
{
	vertices.fitToSize(6);
	vertices.push_back(bottomLeft);
	vertices.push_back(bottomRight);
	vertices.push_back(topRight);
	vertices.push_back(bottomLeft);
	vertices.push_back(topRight);
	vertices.push_back(topLeft);
}

void et::s2d::buildQuad(SceneVertexList& vertices, SceneVertex topLeft, SceneVertex topRight,
	SceneVertex bottomLeft, SceneVertex bottomRight, const mat4& m)
{
	topLeft.position = m * topLeft.position;
	topRight.position = m * topRight.position;
	bottomLeft.position = m * bottomLeft.position;
	bottomRight.position = m * bottomRight.position;
	buildQuad(vertices, topLeft, topRight, bottomLeft, bottomRight);
}

void et::s2d::buildStringVertices(SceneVertexList& vertices, const CharDescriptorList& chars,
	Alignment hAlign, Alignment vAlign, const vec2& pos, const vec4& color, const mat4& transform,
	float lineInterval)
{
	if (chars.empty()) return;
	
	vec4 line(0.0f);
	std::vector<vec4> lines;
	
	for (const CharDescriptor& desc : chars)
	{
		line.w = std::max(line.w, lineInterval * desc.originalSize.y);
		
		if ((desc.value == ET_NEWLINE) || (desc.value == ET_RETURN))
		{
			lines.push_back(line);
			line = vec4(0.0f, line.y + line.w, 0.0f, 0.0f);
		}
		else 
		{
			line.z += desc.originalSize.x;
		}
	}
	lines.push_back(line);
	
	if (lines.empty()) return;
	
	float textHeight = lines.back().y + lines.back().w;
	
	float hAlignFactor = alignmentFactor(hAlign);
	float vAlignFactor = alignmentFactor(vAlign);
	for (vec4& i : lines)
	{
		i.x -= hAlignFactor * i.z;
		i.y -= vAlignFactor * textHeight;
	}
	
	size_t lineIndex = 0;
	line = lines.front();
	
	vertices.fitToSize(6 * chars.size());
	for (const CharDescriptor& desc : chars)
	{
		vec2 sdfParameters = desc.parameters.xy();
		
		if ((desc.value == ET_NEWLINE) || (desc.value == ET_RETURN))
		{
			line = lines[++lineIndex];
		}
		else 
		{
			vec2 topLeft = pos + line.xy() + desc.contentRect.origin();
			vec2 bottomLeft = topLeft + vec2(0.0f, desc.contentRect.height);
			vec2 topRight = topLeft + vec2(desc.contentRect.width, 0.0f);
			vec2 bottomRight = bottomLeft + vec2(desc.contentRect.width, 0.0f);
			
			vec2 topLeftUV = desc.uvRect.origin();
			vec2 topRightUV = topLeftUV + vec2(desc.uvRect.width, 0.0f);
			vec2 bottomLeftUV = topLeftUV - vec2(0.0f, desc.uvRect.height);
			vec2 bottomRightUV = bottomLeftUV + vec2(desc.uvRect.width, 0.0f);
			vec4 charColor = desc.color * color;
			
			buildQuad(vertices,
				SceneVertex(floorv(transform * topLeft), vec4(topLeftUV, sdfParameters), charColor),
				SceneVertex(floorv(transform * topRight), vec4(topRightUV, sdfParameters), charColor),
				SceneVertex(floorv(transform * bottomLeft), vec4(bottomLeftUV, sdfParameters), charColor),
				SceneVertex(floorv(transform * bottomRight), vec4(bottomRightUV, sdfParameters), charColor));
			
			line.x += desc.originalSize.x;
		}
	}
}

size_t et::s2d::measuseVertexCountForImageDescriptor(const ImageDescriptor& desc)
{
	bool hasLeftSafe = desc.contentOffset.left > 0;
	bool hasTopSafe = desc.contentOffset.top > 0;
	bool hasRightSafe = desc.contentOffset.right > 0;
	bool hasBottomSafe = desc.contentOffset.bottom > 0;
	
	bool hasLeftTopCorner = hasLeftSafe && hasTopSafe;
	bool hasRightTopCorner = hasRightSafe && hasTopSafe;
	bool hasLeftBottomCorner = hasLeftSafe && hasBottomSafe;
	bool hasRightBottomCorner = hasRightSafe && hasBottomSafe;

	int numBorders = hasLeftSafe + hasTopSafe + hasRightSafe + hasBottomSafe;
	int numCorners = hasLeftTopCorner + hasRightTopCorner + hasLeftBottomCorner + hasRightBottomCorner;

	return 6 * (1 + numCorners + numBorders);
}

void et::s2d::buildImageVertices(SceneVertexList& vertices, const Texture::Pointer& tex, const ImageDescriptor& desc,
	const rect& p, const vec4& color, const mat4& transform)
{
	if (!tex.valid()) return;

	bool hasLeftSafe = desc.contentOffset.left > 0;
	bool hasTopSafe = desc.contentOffset.top > 0;
	bool hasRightSafe = desc.contentOffset.right > 0;
	bool hasBottomSafe = desc.contentOffset.bottom > 0;
	bool hasLeftTopCorner = hasLeftSafe && hasTopSafe;
	bool hasRightTopCorner = hasRightSafe && hasTopSafe;
	bool hasLeftBottomCorner = hasLeftSafe && hasBottomSafe;
	bool hasRightBottomCorner = hasRightSafe && hasBottomSafe;

	int numBorders = hasLeftSafe + hasTopSafe + hasRightSafe + hasBottomSafe;
	int numCorners = hasLeftTopCorner + hasRightTopCorner + hasLeftBottomCorner + hasRightBottomCorner;

	vertices.fitToSize(6 * (1 + numCorners + numBorders));

	vec2 mask(0.0f, 0.0f);

	float width = std::abs(p.width);
	float height = std::abs(p.height);
	
	vec2 topLeft = (p.origin());
	vec2 topRight = (topLeft + vec2(width, 0.0f));
	vec2 bottomLeft = (topLeft + vec2(0.0f, height));
	vec2 bottomRight = (bottomLeft + vec2(width, 0.0f));
	vec2 centerTopLeft = (p.origin() + desc.contentOffset.origin());
	vec2 centerTopRight = (p.origin() + vec2(width - desc.contentOffset.right, desc.contentOffset.top));
	vec2 centerBottomLeft = (p.origin() + vec2(desc.contentOffset.left, height - desc.contentOffset.bottom));
	vec2 centerBottomRight = (p.origin() + vec2(width - desc.contentOffset.right, height - desc.contentOffset.bottom));
	vec2 topCenterTopLeft = (topLeft + vec2(desc.contentOffset.left, 0.0f));
	vec2 topCenterTopRight = (topLeft + vec2(width - desc.contentOffset.right, 0));
	vec2 leftCenterTopLeft = (topLeft + vec2(0, desc.contentOffset.top));
	vec2 rightCenterTopRight = (topLeft + vec2(width, desc.contentOffset.top));
	vec2 leftCenterBottomLeft = (topLeft + vec2(0, height - desc.contentOffset.bottom));
	vec2 bottomCenterBottomLeft = (topLeft + vec2(desc.contentOffset.left, height));
	vec2 bottomCenterBottomRigth = (topLeft + vec2(width - desc.contentOffset.right, height));
	vec2 rightCenterBottomRigth = (topLeft + vec2(width, height - desc.contentOffset.bottom));

	vec2 topLeftUV = tex->getTexCoord( desc.origin );
	vec2 topRightUV = tex->getTexCoord( desc.origin + vec2(desc.size.x, 0.0f) );
	vec2 bottomLeftUV = tex->getTexCoord( desc.origin + vec2(0.0f, desc.size.y) );
	vec2 bottomRightUV = tex->getTexCoord( desc.origin + desc.size );
	vec2 centerTopLeftUV = tex->getTexCoord( desc.centerPartTopLeft() );
	vec2 centerBottomLeftUV = tex->getTexCoord( desc.centerPartBottomLeft() );
	vec2 centerTopRightUV = tex->getTexCoord( desc.centerPartTopRight() );
	vec2 centerBottomRightUV = tex->getTexCoord( desc.centerPartBottomRight() );
	vec2 topCenterTopLeftUV = tex->getTexCoord( desc.origin + vec2(desc.contentOffset.left, 0) );
	vec2 topCenterTopRightUV = tex->getTexCoord( desc.origin + vec2(desc.size.x - desc.contentOffset.right, 0) );
	vec2 leftCenterTopLeftUV = tex->getTexCoord( desc.origin + vec2(0, desc.contentOffset.top) );
	vec2 rightCenterTopRightUV = tex->getTexCoord( desc.origin + vec2(desc.size.x, desc.contentOffset.top) );
	vec2 leftCenterBottomLeftUV = tex->getTexCoord( desc.origin + vec2(0, desc.size.y - desc.contentOffset.bottom) );
	vec2 bottomCenterBottomLeftUV = tex->getTexCoord( desc.origin + vec2(desc.contentOffset.left, desc.size.y) );
	vec2 bottomCenterBottomRigthUV = tex->getTexCoord( desc.origin + vec2(desc.size.x - desc.contentOffset.right, desc.size.y) );
	vec2 rightCenterBottomRigthUV = tex->getTexCoord( desc.origin + vec2( desc.size.x, desc.size.y - desc.contentOffset.bottom));

	buildQuad(vertices, 
		SceneVertex(transform * centerTopLeft, vec4(centerTopLeftUV, mask), color ), 
		SceneVertex(transform * centerTopRight, vec4(centerTopRightUV, mask), color ),
		SceneVertex(transform * centerBottomLeft, vec4(centerBottomLeftUV, mask), color ),
		SceneVertex(transform * centerBottomRight, vec4(centerBottomRightUV, mask), color ) );

	if (hasLeftTopCorner)
	{
		buildQuad(vertices, 
			SceneVertex(transform * topLeft, vec4(topLeftUV, mask), color), 
			SceneVertex(transform * topCenterTopLeft, vec4(topCenterTopLeftUV, mask), color), 
			SceneVertex(transform * leftCenterTopLeft, vec4(leftCenterTopLeftUV, mask), color), 
			SceneVertex(transform * centerTopLeft, vec4(centerTopLeftUV, mask), color) );
	}

	if (hasRightTopCorner)
	{
		buildQuad(vertices,
			SceneVertex(transform * topCenterTopRight, vec4(topCenterTopRightUV, mask), color),
			SceneVertex(transform * topRight, vec4(topRightUV, mask), color), 
			SceneVertex(transform * centerTopRight, vec4(centerTopRightUV, mask), color), 
			SceneVertex(transform * rightCenterTopRight, vec4(rightCenterTopRightUV, mask), color) );
	}

	if (hasLeftBottomCorner)
	{
		buildQuad(vertices, 
			SceneVertex(transform * leftCenterBottomLeft, vec4(leftCenterBottomLeftUV, mask), color), 
			SceneVertex(transform * centerBottomLeft, vec4(centerBottomLeftUV, mask), color), 
			SceneVertex(transform * bottomLeft, vec4(bottomLeftUV, mask), color), 
			SceneVertex(transform * bottomCenterBottomLeft, vec4(bottomCenterBottomLeftUV, mask), color) );
	}

	if (hasRightBottomCorner)
	{
		buildQuad(vertices, 
			SceneVertex(transform * centerBottomRight, vec4(centerBottomRightUV, mask), color), 
			SceneVertex(transform * rightCenterBottomRigth, vec4(rightCenterBottomRigthUV, mask), color), 
			SceneVertex(transform * bottomCenterBottomRigth, vec4(bottomCenterBottomRigthUV, mask), color), 
			SceneVertex(transform * bottomRight, vec4(bottomRightUV, mask), color) );
	}

	if (hasTopSafe)
	{
		vec2 tl = hasLeftTopCorner ? topCenterTopLeft : topLeft;
		vec2 tr = hasRightTopCorner ? topCenterTopRight : topRight;
		vec2 bl = hasLeftTopCorner ? centerTopLeft : leftCenterTopLeft;
		vec2 br = hasRightTopCorner ? centerTopRight : rightCenterTopRight;
		vec2 tlUV = hasLeftTopCorner ? topCenterTopLeftUV : topLeftUV;
		vec2 trUV = hasRightTopCorner ? topCenterTopRightUV : topRightUV;
		vec2 blUV = hasLeftTopCorner ? centerTopLeftUV : leftCenterTopLeftUV;
		vec2 brUV = hasRightTopCorner ? centerTopRightUV : rightCenterTopRightUV;

		buildQuad(vertices, 
			SceneVertex(transform * tl, vec4(tlUV, mask), color),
			SceneVertex(transform * tr, vec4(trUV, mask), color),
			SceneVertex(transform * bl, vec4(blUV, mask), color), 
			SceneVertex(transform * br, vec4(brUV, mask), color) );
	}

	if (hasLeftSafe)
	{
		vec2 tl = hasLeftTopCorner ? leftCenterTopLeft : topLeft;
		vec2 tr = hasLeftTopCorner ? centerTopLeft : topCenterTopLeft;
		vec2 bl = hasLeftBottomCorner ? leftCenterBottomLeft : bottomLeft;
		vec2 br = hasLeftBottomCorner ? centerBottomLeft : bottomCenterBottomLeft;
		vec2 tlUV = hasLeftTopCorner ? leftCenterTopLeftUV : topLeftUV;
		vec2 trUV = hasLeftTopCorner ? centerTopLeftUV : topCenterTopLeftUV;
		vec2 blUV = hasLeftBottomCorner ? leftCenterBottomLeftUV : bottomLeftUV;
		vec2 brUV = hasLeftBottomCorner ? centerBottomLeftUV : bottomCenterBottomLeftUV;

		buildQuad(vertices,
			SceneVertex(transform * tl, vec4(tlUV, mask), color), 
			SceneVertex(transform * tr, vec4(trUV, mask), color),
			SceneVertex(transform * bl, vec4(blUV, mask), color),
			SceneVertex(transform * br, vec4(brUV, mask), color) );
	}

	if (hasBottomSafe)
	{
		vec2 tl = hasLeftBottomCorner ? centerBottomLeft : leftCenterBottomLeft;
		vec2 tr = hasRightBottomCorner ? centerBottomRight : rightCenterBottomRigth;
		vec2 bl = hasLeftBottomCorner ? bottomCenterBottomLeft : bottomLeft;
		vec2 br = hasRightBottomCorner ? bottomCenterBottomRigth : bottomRight;
		vec2 tlUV = hasLeftBottomCorner ? centerBottomLeftUV : leftCenterBottomLeftUV;
		vec2 trUV = hasRightBottomCorner ? centerBottomRightUV : rightCenterBottomRigthUV;
		vec2 blUV = hasLeftBottomCorner ? bottomCenterBottomLeftUV : bottomLeftUV;
		vec2 brUV = hasRightBottomCorner ? bottomCenterBottomRigthUV : bottomRightUV;

		buildQuad(vertices,
			SceneVertex(transform * tl, vec4(tlUV, mask), color), 
			SceneVertex(transform * tr, vec4(trUV, mask), color), 
			SceneVertex(transform * bl, vec4(blUV, mask), color), 
			SceneVertex(transform * br, vec4(brUV, mask), color) );
	}

	if (hasRightSafe)
	{
		vec2 tl = hasRightTopCorner ? centerTopRight : topCenterTopRight;
		vec2 tr = hasRightTopCorner ? rightCenterTopRight : topRight;
		vec2 bl = hasRightBottomCorner ? centerBottomRight : bottomCenterBottomRigth;
		vec2 br = hasRightBottomCorner ? rightCenterBottomRigth : bottomRight;
		vec2 tlUV = hasRightTopCorner ? centerTopRightUV : topCenterTopRightUV;
		vec2 trUV = hasRightTopCorner ? rightCenterTopRightUV : topRightUV;
		vec2 blUV = hasRightBottomCorner ? centerBottomRightUV : bottomCenterBottomRigthUV;
		vec2 brUV = hasRightBottomCorner ? rightCenterBottomRigthUV : bottomRightUV;

		buildQuad(vertices, 
			SceneVertex(transform * tl, vec4(tlUV, mask), color),
			SceneVertex(transform * tr, vec4(trUV, mask), color),
			SceneVertex(transform * bl, vec4(blUV, mask), color), 
			SceneVertex(transform * br, vec4(brUV, mask), color) );
	}
}

void et::s2d::buildColorVertices(SceneVertexList& vertices, const rect& p, const vec4& color,
	const mat4& transform)
{
	static const vec4 texCoord[] =
	{
		vec4(0.0f, 1.0f, 0.0f, 1.0f),
		vec4(1.0f, 1.0f, 0.0f, 1.0f),
		vec4(0.0f, 0.0f, 0.0f, 1.0f),
		vec4(1.0f, 0.0f, 0.0f, 1.0f),
	};
	
	vec2 topLeft = p.origin();
	vec2 topRight = topLeft + vec2(p.width, 0.0f);
	vec2 bottomLeft = topLeft + vec2(0.0f, p.height);
	vec2 bottomRight = bottomLeft + vec2(p.width, 0.0f);
	
	buildQuad(vertices,
		SceneVertex(transform * topLeft, texCoord[0], color),
		SceneVertex(transform * topRight, texCoord[1], color),
		SceneVertex(transform * bottomLeft, texCoord[2], color),
		SceneVertex(transform * bottomRight, texCoord[3], color));
}
