#pragma once

#include <et/rendering/rendercontext.h>

namespace et
{
	class Atmosphere : public Shared
	{
	public:
		ET_DECLARE_POINTER(Atmosphere)
		
	public:
		Atmosphere(RenderContext*);
		
		void setLightDirection(const vec3&);
		void setParameters(const Dictionary&);
		
		void renderAtmosphereWithGeometry(const Camera&, bool drawSky, bool drawPlanet);
		
		Dictionary parameters()
			{ return _parameters; }
		
		void setPlanetFragmentShader(const std::string&);

		void setShouldComputeScatteringOnPlanet(bool);
		
		const vec3& cameraPosition() const
			{ return _cameraPosition; }
		
		void generateCubemap(et::Framebuffer::Pointer);
		
		Program::Pointer planetProgram();
		
	public:
		static Dictionary defaultParameters();
		static const std::string& defaultPlanetFragmentShader();
				
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
		VertexArrayObject _gridVAO;
		
		Program::Pointer _atmospherePerVertexProgram;
		Program::Pointer _atmosphereFastPerVertexProgram;
		Program::Pointer _planetPerVertexProgram;
		
		Dictionary _parameters;
		vec3 _lightDirection = unitY;
		vec3 _cameraPosition;
		
		bool _skyParametersValid = false;
		bool _groundParametersValid = false;
		bool _computeScatteringOnPlanet = true;
	};
}
