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
		
		void testRender(RenderContext*, const Camera&, const vec3& lightPosition);
		
	private:
		void generateGeometry(RenderContext*);
		void setProgramParameters(Program::Pointer p);
		
	private:
		Program::Pointer _atmosphereProgram;
		Program::Pointer _groundProgram;
		
		Program::Pointer _testProgram;
		
		VertexArrayObject _atmosphereVAO;
		VertexArrayObject _groundVAO;
	};
}
