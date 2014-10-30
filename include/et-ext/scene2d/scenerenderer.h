/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <stack>
#include <et/core/objectscache.h>
#include <et/core/containers.h>
#include <et/camera/camera.h>
#include <et-ext/scene2d/renderingelement.h>
#include <et-ext/scene2d/vertexbuilder.h>

namespace et
{
	namespace s2d
	{
		class SceneRenderer
		{
		public:
			static const std::string defaultProgramName;
			static const std::string defaultTextProgramName;
			
		public:
			SceneRenderer(RenderContext* rc);

			void beginRender(RenderContext* rc);
			void render(RenderContext* rc);
			void endRender(RenderContext* rc);

			void resetClipRect();
			void pushClipRect(const recti&);
			void popClipRect();

			void setProjectionMatrices(const vec2& contextSize);
			void setRendernigElement(const RenderingElement::Pointer& r);

			void addVertices(const SceneVertexList&, const Texture&, const SceneProgram&, Element2d*);
			
			void setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha);
						
			const SceneProgram& defaultProgram() const
				{ return _defaultProgram; }

			const SceneProgram& defaultTextProgram() const
				{ return _defaultTextProgram; }
			
			const Texture& lastUsedTexture() const
				{ return _lastTexture; }

			const Texture& defaultTexture() const
				{ return _defaultTexture; }
			
			SceneProgram createProgramWithFragmentshader(const std::string& name, const std::string& fs);
			SceneProgram createProgramWithShaders(const std::string& name, const std::string& vs, const std::string& fs);

		private:
			void init(RenderContext* rc);
			void alloc(size_t count);
			
			SceneVertex* allocateVertices(size_t, const Texture&, const SceneProgram&, Element2d*);

			ET_DENY_COPY(SceneRenderer)
			
		private:
			RenderContext* _rc;
			RenderingElement::Pointer _renderingElement;
			
			Texture _lastTexture;
			Texture _defaultTexture;
			
			ObjectsCache _programsCache;
			
			SceneProgram _defaultProgram;
			SceneProgram _defaultTextProgram;
			SceneProgram _lastProgram;
					
			mat4 _defaultTransform;
			
			std::stack<recti> _clip;
			
			vec3 _additionalOffsetAndAlpha;
			recti _additionalWindowOffset;
		};
	}
}
