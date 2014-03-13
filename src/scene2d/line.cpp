/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/line.h>

using namespace et;
using namespace et::s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(Line)

Line::Line(const vec2& from, const vec2& to, Element2d* parent) :
	Element2d(parent), _width(1.0f), _type(Type_Linear)
{
	setFlag(Flag_TransparentForPointer);
	
	_controlPoints.push_back(from);
	_controlPoints.push_back(0.5f * (from + to));
	_controlPoints.push_back(to);
	
	fillParent();
}

void Line::setType(Type t)
{
	_type = t;
	invalidateContent();
}

void Line::setControlPoint(size_t index, const vec2& p)
{
	ET_ASSERT(index < _controlPoints.size())
	_controlPoints[index] = p;
	invalidateContent();
}

void Line::setWidth(float w)
{
	_width = w;
	invalidateContent();
}

void Line::addToRenderQueue(RenderContext*, SceneRenderer& r)
{
	initProgram(r);
	
	if (!contentValid())
		buildVertices(r);
	
	r.addVertices(_vertices, Texture(), program(), this);
}

void Line::buildLine(const vec2& p1, const vec2& p2, const vec4& tc, const vec4& clr, const mat4& t)
{
	vec2 n = 0.5f * _width * normalize(vec2(p2.y - p1.y, p1.x - p2.x));
	SceneVertex tl(t * (p1 - n), tc, clr);
	SceneVertex tr(t * (p1 + n), tc, clr);
	SceneVertex bl(t * (p2 - n), tc, clr);
	SceneVertex br(t * (p2 + n), tc, clr);
	buildQuad(_vertices, tl, tr, bl, br);
}

void Line::buildVertices(SceneRenderer& r)
{
	_vertices.setOffset(0);
	
	vec4 clr = color();
	vec4 texCoord(0.0f, 0.0f, 0.0f, 1.0f);
	mat4 tr = finalTransform();
	
	if (_type == Type_Linear)
	{
		if (_shadowColor.w > 0.0f)
		{
			vec2 p1 = origin() + size() * _controlPoints.front() + _shadowOffset;
			vec2 p2 = origin() + size() * _controlPoints.back() + _shadowOffset;
			buildLine(p1, p2, texCoord, clr, tr);
		}
		
		vec2 p1 = origin() + size() * _controlPoints.front();
		vec2 p2 = origin() + size() * _controlPoints.back();
		buildLine(p1, p2, texCoord, clr, tr);
	}
	else if (_type == Type_QuadraticBezier)
	{
		vec2 cp1 = origin() + size() * _controlPoints[0];
		vec2 cp2 = origin() + size() * _controlPoints[1];
		vec2 cp3 = origin() + size() * _controlPoints[2];
		
		float curveLength = 0.0f;
		float t = 0.0f;
		float dt = 0.0001f;
		while (t <= 1.0f)
		{
			vec2 dCdT = 2.0f * (cp1 * (t - 1.0f) + cp2 * (1.0f - 2.0f * t) + cp3 * t);
			curveLength += dt * std::sqrt(dCdT.dotSelf());
			t += dt;
		}
		dt = 1.0f / std::sqrt(curveLength);
		
		if (_shadowColor.w > 0.0f)
		{
			t = 0.0f;
			while (t < 1.0f)
			{
				vec2 p1 = origin() + size() * bezierCurve(_controlPoints.data(), _controlPoints.size(), t) + _shadowOffset;
				vec2 p2 = origin() + size() * bezierCurve(_controlPoints.data(), _controlPoints.size(), etMin(1.0f, t + dt)) + _shadowOffset;
				buildLine(p1, p2, texCoord, _shadowColor, tr);
				t += dt;
			}
		}

		t = 0.0f;
		while (t < 1.0f)
		{
			vec2 p1 = origin() + size() * bezierCurve(_controlPoints.data(), _controlPoints.size(), t);
			vec2 p2 = origin() + size() * bezierCurve(_controlPoints.data(), _controlPoints.size(), etMin(1.0f, t + dt));
			buildLine(p1, p2, texCoord, clr, tr);
			t += dt;
		}
	}
	else
	{
		ET_FAIL("Invalid line type.")
	}
	
	setContentValid();
}

void Line::setShadowColor(const vec4& c)
{
	_shadowColor = c;
	invalidateContent();
}

void Line::setShadowOffset(const vec2& o)
{
	_shadowOffset = o;
	invalidateContent();
}
