#pragma once

#include <et/rendering/rendercontext.h>

namespace et
{
	class Atmosphere : public Shared
	{
	public:
		ET_DECLARE_POINTER(Atmosphere)
		
	public:
		Atmosphere(RenderContext*, size_t textureSize);
		
		void setLightDirection(const vec3&);
		
		void updateTexture();
		et::Texture environmentTexture();
		
	private:
		void generateGeometry(RenderContext*);
		void setProgramParameters(Program::Pointer p);
		void performRendering();
		
	private:
		RenderContext* _rc;
		
		VertexArrayObject _atmosphereVAO;
		
		Program::Pointer _atmospherePerVertexProgram;
		Program::Pointer _atmospherePerPixelProgram;
		
		Framebuffer::Pointer _framebuffer;

		Camera _cubemapCamera;
		CubemapProjectionMatrixArray _cubemapMatrices;
		
		vec3 _lightDirection = unitY;
		bool _textureValid = false;
	};
}
