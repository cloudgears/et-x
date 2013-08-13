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

			size_t addVertices(const GuiVertexList& vertices, const Texture& texture);
			size_t addVertices(const GuiVertexList& vertices, const Texture& texture,
				ElementRepresentation cls);
			
			void setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha);
			
			const Camera& camera3d() const 
				{ return _cameraFor3dElements; }
			
			SceneProgram defaultProgram() const
				{ return _defaultProgram; }
			
			SceneProgram createProgramWithFragmentshader(const std::string& name, const std::string& fs);

		private:
			void init(RenderContext* rc);
			void alloc(size_t count);
			
			GuiVertexPointer allocateVertices(size_t, const Texture&, const SceneProgram&, ElementRepresentation);

			ET_DENY_COPY(SceneRenderer)
			
		private:
			RenderContext* _rc;
			RenderingElement::Pointer _renderingElement;
			
			Texture _lastTexture;
			Texture _defaultTexture;
			
			SceneProgram _defaultProgram;
			SceneProgram _lastProgram;
			
			ObjectsCache _sharedCache;
			
			Camera _cameraFor3dElements;
			mat4 _defaultTransform;
			
			std::stack<recti> _clip;
			recti _clipRect;
			
			vec3 _additionalOffsetAndAlpha;
			recti _additionalWindowOffset;
			
			size_t _blendState;
			bool _depthTestEnabled;
			bool _depthMask;
			bool _blendEnabled;
			bool _clipEnabled;
		};
	}
}
