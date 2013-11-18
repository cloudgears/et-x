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
		void setParameters(Dictionary);
		
		void renderAtmosphereWithGeometry(const Camera&);
		
		void performRendering(bool shouldClear, bool renderPlanet);
		
		Texture environmentTexture();
		
		Dictionary parameters()
			{ return _parameters; }
		
		void setPlanetFragmentShader(const std::string&);
		
	public:
		static Dictionary defaultParameters();
		static const std::string& defaultPlanetFragmentShader();
		
		static const std::string kAmbientColor;
		
		static const std::string kWaveLength;
		static const std::string kKr;
		static const std::string kKm;
		static const std::string kG;
		static const std::string kSunExponent;
		static const std::string kRayleighScaleDepth;
		
		static const std::string kPlanetRadius;
		static const std::string kAtmosphereHeight;
		
		static const std::string kIterationCount;
		
		static const std::string kLatitude;
		static const std::string kLongitude;
		static const std::string kHeightAboveSurface;
		
	private:
		void generateGeometry(RenderContext*);
		void setProgramParameters(Program::Pointer p);
		
	private:
		RenderContext* _rc;
		
		VertexArrayObject _atmosphereVAO;
		
		Program::Pointer _atmospherePerVertexProgram;
		Program::Pointer _atmospherePerPixelProgram;
		
		Program::Pointer _planetPerVertexProgram;
		Program::Pointer _planetPerPixelProgram;
		
		Framebuffer::Pointer _framebuffer;

		Camera _cubemapCamera;
		CubemapProjectionMatrixArray _cubemapMatrices;
		
		Dictionary _parameters;
		vec3 _lightDirection = unitY;
		vec3 _cameraPosition;
		vec4 _ambientColor;
		bool _parametersValid = false;
	};
}
