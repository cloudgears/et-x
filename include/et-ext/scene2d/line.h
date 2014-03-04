/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>

namespace et
{
	namespace s2d
	{
		class Line : public Element2d
		{
		public:
			ET_DECLARE_POINTER(Line)
			
			enum Type
			{
				Type_Linear,
				Type_QuadraticBezier
			};
			
		public:
			Line(const vec2&, const vec2&, Element2d*);
			
			void setType(Type);
			void setControlPoint(size_t index, const vec2&);
			void setWidth(float);
			
			void setShadowColor(const vec4&);
			void setShadowOffset(const vec2&);
			
		private:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			void buildVertices(SceneRenderer&);
			
			void buildLine(const vec2&, const vec2&, const vec4&, const vec4&, const mat4&);
			
		private:
			std::vector<vec2> _controlPoints;
			SceneVertexList _vertices;
			vec4 _shadowColor;
			vec2 _shadowOffset;
			float _width;
			Type _type;
		};
	}
}