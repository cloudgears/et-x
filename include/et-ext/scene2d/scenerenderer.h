/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
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

			void addVertices(const SceneVertexList&, const Texture::Pointer&, const SceneProgram&, Element2d*,
				PrimitiveType = PrimitiveType::Triangles);
			
			void setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha);
						
			const SceneProgram& defaultProgram() const
				{ return _defaultProgram; }
			
			const Texture::Pointer& lastUsedTexture() const
				{ return _lastTexture; }

			const Texture::Pointer& defaultTexture() const
				{ return _defaultTexture; }
			
			SceneProgram createProgramWithFragmentshader(const std::string& name, const std::string& fs,
				bool includeScreenSpacePosVarying);
			
			SceneProgram createProgramWithShaders(const std::string& name, const std::string& vs,
				const std::string& fs);

		private:
			void init(RenderContext* rc);
			
			SceneVertex* allocateVertices(uint32_t, const Texture::Pointer&, const SceneProgram&, Element2d*, PrimitiveType);

			ET_DENY_COPY(SceneRenderer);
			
		private:
			RenderContext* _rc;
			RenderingElement::Pointer _renderingElement;
			
			Texture::Pointer _lastTexture;
			Texture::Pointer _defaultTexture;
			
			ObjectsCache _programsCache;
			
			SceneProgram _defaultProgram;
			SceneProgram _defaultTextProgram;
			SceneProgram _lastProgram;
					
			mat4 _defaultTransform;
			
			std::stack<recti> _clip;

			BlendState _lastBlendState;
			DepthState _lastDepthState;
			RasterizerState _lastRasterizerState;
			
			vec3 _additionalOffsetAndAlpha;
			recti _additionalWindowOffset;
		};
	}
}
