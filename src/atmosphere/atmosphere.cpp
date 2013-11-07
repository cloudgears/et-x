#include <et/camera/camera.h>
#include <et/geometry/geometry.h>
#include <et/primitives/primitives.h>
#include <et-ext/atmosphere/atmosphere.h>

using namespace et;

extern const std::string atmosphereVS;
extern const std::string atmosphereFS;

extern const std::string groundVS;
extern const std::string groundFS;

float innerRadius = 637.0f;
float outerRadius = 1.02f * innerRadius;

vec3 positionOnSphere = normalize(vec3(0.0f, 1.0f, 0.0f)) * mix(innerRadius, outerRadius, 0.2f);

int numSamples = 10;

Atmosphere::Atmosphere(RenderContext* rc)
{
	ObjectsCache localCache;
	_testProgram = rc->programFactory().loadProgram("data/shaders/default.program", localCache);
	
	_atmosphereProgram = rc->programFactory().genProgram("et~atmosphere~program", atmosphereVS, atmosphereFS);
	setProgramParameters(_atmosphereProgram);

	_groundProgram = rc->programFactory().genProgram("et~ground~program", groundVS, groundFS);
	setProgramParameters(_groundProgram);
	
	generateGeometry(rc);
}

void Atmosphere::setProgramParameters(Program::Pointer prog)
{
	vec3 waveLength(0.720f, 0.550f, 0.480f);
	
	vec3 invWaveLenth(1.0f / std::pow(waveLength.x, 4.0f),
		1.0f / std::pow(waveLength.y, 4.0f),
		1.0f / std::pow(waveLength.z, 4.0f));
	
	float fG = -0.999f;
	float fKr = 0.0025f;			// Rayleigh scattering constant
	float fKm = 0.0015f;			// Mie scattering constant
	
	float fESun = 25.0f;			// Sun brightness constant
	
	float fKr4PI = fKr * 4.0f * PI;
	float fKm4PI = fKm * 4.0f * PI;
	
	float fRayleighScaleDepth = 1.0f / 3.0f;
	
	prog->setUniform("nSamples", numSamples);
	prog->setUniform("fSamples", static_cast<float>(numSamples));
	prog->setUniform("v3InvWavelength", invWaveLenth);
	prog->setUniform("fOuterRadius", outerRadius);
	prog->setUniform("fOuterRadius2", sqr(outerRadius));
	prog->setUniform("fInnerRadius", innerRadius);
	prog->setUniform("fInnerRadius2", sqr(innerRadius));
	prog->setUniform("fScale", 1.0f / (outerRadius - innerRadius));
	prog->setUniform("fScaleDepth", fRayleighScaleDepth);
	prog->setUniform("fScaleOverScaleDepth", (1.0f / (outerRadius - innerRadius)) / fRayleighScaleDepth);
	prog->setUniform("g", fG);
	prog->setUniform("g2", sqr(fG));
	prog->setUniform("fKrESun", fKr * fESun);
	prog->setUniform("fKmESun", fKm * fESun);
	prog->setUniform("fKr4PI", fKr4PI);
	prog->setUniform("fKm4PI", fKm4PI);
}

void Atmosphere::generateGeometry(RenderContext* rc)
{
	size_t tesselationLimit = 65536 / 4;
	
	VertexDeclaration decl(true, Usage_Position, Type_Vec3);
	
	auto va = VertexArray::Pointer::create(decl, 0);
	auto ia = IndexArray::Pointer::create(IndexArrayFormat_32bit, va->size(), PrimitiveType_Triangles);
	primitives::createIcosahedron(va, 1.0f, true, false, false);
	do
	{
		bool tesselated = false;
		while (va->size() * 4 < tesselationLimit)
		{
			primitives::tesselateTriangles(va, ia);
			ia->resizeToFit(va->size());
			ia->linearize(va->size());
			tesselated = true;
		}
		if (!tesselated) break;
		va = primitives::buildIndexArray(va, ia);
	}
	while (va->size() < tesselationLimit);
	
	RawDataAcessor<vec3> pos = va->chunk(Usage_Position).accessData<vec3>(0);
	for (size_t i = 0, e = va->size(); i < e; ++i)
		pos[i] = outerRadius * normalize(pos[i]);
	
	_atmosphereVAO = rc->vertexBufferFactory().createVertexArrayObject("sky-sphere", va,
		BufferDrawType_Static, ia, BufferDrawType_Static);

	float deviation = 0.00125588697;
	for (size_t i = 0, e = va->size(); i < e; ++i)
		pos[i] = (outerRadius * randomFloat(1.0f - deviation, 1.0f + deviation)) * normalize(pos[i]);
	
	_groundVAO = rc->vertexBufferFactory().createVertexArrayObject("ground-sphere", va,
		BufferDrawType_Static, ia, BufferDrawType_Static);
}

void Atmosphere::testRender(RenderContext* rc, const Camera& cam, const vec3& lightPosition)
{
	vec3 lightDirection = normalize(lightPosition);
	
	Camera localCamera = cam;
	localCamera.lookAt(positionOnSphere + cam.direction(), positionOnSphere);
	
	/*
	 * render ground
	 */
	rc->renderState().setDepthMask(true);
	rc->renderState().setCulling(true, CullState_Back);
	rc->renderState().setBlend(false, BlendState_Additive);
	rc->renderState().bindVertexArray(_groundVAO);
	rc->renderState().bindProgram(_groundProgram);
	_groundProgram->setPrimaryLightPosition(lightDirection);
	_groundProgram->setCameraProperties(localCamera);
	_groundProgram->setTransformMatrix(scaleMatrix(vec3(innerRadius / outerRadius)));
	rc->renderer()->drawAllElements(_groundVAO->indexBuffer());

	/*
	 * render atmosphere
	 */
	rc->renderState().setDepthMask(false);
	rc->renderState().setCulling(true, CullState_Front);
	rc->renderState().setBlend(true, BlendState_Additive);
	rc->renderState().bindVertexArray(_atmosphereVAO);
	rc->renderState().bindProgram(_atmosphereProgram);
	_atmosphereProgram->setTransformMatrix(identityMatrix);
	_atmosphereProgram->setCameraProperties(localCamera);
	_atmosphereProgram->setPrimaryLightPosition(lightDirection);
	rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	
	rc->renderState().setBlend(false);
	rc->renderState().setDepthMask(true);
	rc->renderState().setCulling(true, CullState_Back);
	rc->renderState().setWireframeRendering(false);
	rc->renderer()->clear(false, true);
}

////////////////////////////////////////////////////////
//
// Data section
//
////////////////////////////////////////////////////////

#define ET_COMMON_UNIFORM_SET \
	uniform mat4 mModelViewProjection; \
	uniform mat4 mTransform; \
	uniform vec3 vCamera; \
	uniform vec3 vPrimaryLight; \
	uniform vec3 v3InvWavelength; \
	uniform float fOuterRadius; \
	uniform float fOuterRadius2; \
	uniform float fInnerRadius; \
	uniform float fInnerRadius2; \
	uniform float fKrESun; \
	uniform float fKmESun; \
	uniform float fKr4PI; \
	uniform float fKm4PI; \
	uniform float fScale; \
	uniform float fScaleDepth; \
	uniform float fScaleOverScaleDepth; \
	uniform float fSamples; \
	uniform int nSamples;

#define ET_COMMON_SHADER_VARIABLES \
	etVertexIn vec4 Vertex; \
	etVertexOut vec3 v3Direction; \
	etVertexOut vec3 aFrontColor; \
	etVertexOut vec3 aSecondaryColor; \

#define ET_COMMON_SCALE_FUNC \
	float scale(float fCos) { \
		float x = 1.0 - fCos; \
		return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287); }

#define ET_INIT_SCATTERING_LOOP_VARIABLES \
	float fSampleLength = fFar / fSamples; \
	float fScaledLength = fSampleLength * fScale; \
	vec3 v3SampleRay = v3Ray * fSampleLength; \
	vec3 v3SamplePoint = v3Start;

#define ET_GET_RAY \
	float fCameraHeight = length(vCamera); \
	float fCameraHeight2 = fCameraHeight * fCameraHeight; \
	vec4 vTransformedVertex = mTransform * Vertex; \
	vec3 v3Pos = vTransformedVertex.xyz; \
	vec3 v3Ray = v3Pos - vCamera; \
	float fFar = length(v3Ray); \
	v3Ray /= fFar;

const std::string atmosphereVS = ET_TO_CONST_CHAR
   (
	ET_COMMON_UNIFORM_SET
	ET_COMMON_SCALE_FUNC
	ET_COMMON_SHADER_VARIABLES
	
	void main(void)
	{
		ET_GET_RAY
	
		vec3 v3Start = vCamera;
		float fStartAngle = 0.0;
		float fStartOffset = 0.0;
		if (fCameraHeight < fOuterRadius)
		{
			fStartAngle = dot(v3Ray, v3Start) / length(v3Start);
			fStartOffset = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight)) * scale(fStartAngle);
		}
		else
		{
			float B = 2.0 * dot(vCamera, v3Ray);
			float C = fCameraHeight2 - fOuterRadius2;
			float fDet = max(0.0, B * B - 4.0 * C);
			float fNear = -0.5 * (B + sqrt(fDet));
			fFar -= fNear;
			
			v3Start += v3Ray * fNear;
			fStartAngle = dot(v3Ray, v3Start) / fOuterRadius;
			fStartOffset = exp(-1.0 / fScaleDepth) * scale(fStartAngle);
		}
		
		ET_INIT_SCATTERING_LOOP_VARIABLES
		
		vec3 v3FrontColor = vec3(0.0);
		for(int i = 0; i < nSamples; i++)
		{
			float fHeight = length(v3SamplePoint);
			float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
			float fLightAngle = dot(vPrimaryLight, v3SamplePoint) / fHeight;
			float fCameraAngle = dot(v3Ray, v3SamplePoint) / fHeight;
			float fScatter = fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle));
			vec3 v3Attenuate = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
			v3FrontColor += v3Attenuate * (fDepth * fScaledLength);
			v3SamplePoint += v3SampleRay;
		}
		
		v3Direction = vCamera - v3Pos;
		aFrontColor = fKrESun * (v3FrontColor * v3InvWavelength);
		aSecondaryColor = fKmESun * v3FrontColor;
		
		gl_Position = mModelViewProjection * vTransformedVertex;
	}
);

const std::string atmosphereFS = ET_TO_CONST_CHAR
   (
	precision highp float;
	uniform vec3 vPrimaryLight;
	
	uniform float g;
	uniform float g2;
	
	etFragmentIn vec3 v3Direction;
	etFragmentIn vec3 aFrontColor;
	etFragmentIn vec3 aSecondaryColor;
	
	void main (void)
	{
		float fCos = dot(vPrimaryLight, v3Direction) / length(v3Direction);
		float fRayleighPhase = 0.75 * (1.0 + fCos*fCos);
		float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);
		vec4 aColor = vec4(fRayleighPhase * aFrontColor + fMiePhase * aSecondaryColor, 1.0);
		etFragmentOut = 1.0 - exp(-aColor);
	}
);

////////////////////////////////////////////////////////////////
// Ground shader
////////////////////////////////////////////////////////////////

const std::string groundVS = ET_TO_CONST_CHAR
   (
	ET_COMMON_UNIFORM_SET
	ET_COMMON_SHADER_VARIABLES
	ET_COMMON_SCALE_FUNC
		
	void main(void)
	{
		ET_GET_RAY
/*
		vec3 v3Start = vCamera;
		
		float fTemp = 0.0;
		float fDepth = 0.0;
		float fCameraOffset = 0.0;
		float fCameraScale = 0.0;
		
		if (fCameraHeight < fOuterRadius)
		{
			fDepth = exp((fInnerRadius - fCameraHeight) / fScaleDepth);
			fCameraScale = scale(dot(-v3Ray, v3Pos) / length(v3Pos));
		}
		else
		{
			float B = 2.0 * dot(vCamera, v3Ray);
			float C = fCameraHeight2 - fOuterRadius2;
			float fDet = max(0.0, B*B - 4.0 * C);
			float fNear = 0.5 * (-B - sqrt(fDet));
			v3Start += v3Ray * fNear;
			fFar -= fNear;
			fDepth = exp((fInnerRadius - fOuterRadius) / fScaleDepth);
			fCameraScale = scale(dot(-v3Ray, v3Pos) / length(v3Pos));
		}
		
		float fLightScale = scale(dot(vPrimaryLight, v3Pos) / length(v3Pos));
		fCameraOffset = fDepth * fCameraScale;
		fTemp = fLightScale + fCameraScale;
		
		ET_INIT_SCATTERING_LOOP_VARIABLES
		
		vec3 v3FrontColor = vec3(0.0);
		for (int i = 0; i < nSamples; ++i)
		{
			float aHeight = length(v3SamplePoint);
			float aDepth = exp(fScaleOverScaleDepth * (fInnerRadius - aHeight));
			float fScatter = aDepth * fTemp - fCameraOffset;
			aSecondaryColor = exp(-fScatter * (v3InvWavelength * fKr4PI + fKm4PI));
			v3FrontColor += aSecondaryColor * vec3(aDepth * fScaledLength);
			v3SamplePoint += v3SampleRay;
		}
		aFrontColor = v3FrontColor * (v3InvWavelength * fKrESun + fKmESun);
*/
		aFrontColor = v3InvWavelength * fKrESun + fKmESun;
		aSecondaryColor = vec3(fKrESun / v3InvWavelength + fKmESun);
		
		gl_Position = mModelViewProjection * vTransformedVertex;
	}
);

const std::string groundFS = ET_TO_CONST_CHAR
   (
	precision highp float;
	etFragmentIn vec3 aFrontColor;
	etFragmentIn vec3 aSecondaryColor;
	void main (void)
	{
		vec4 aColor = vec4(aFrontColor + 0.25 * aSecondaryColor, 1.0);
		etFragmentOut = 1.0 - exp(-aColor);
	}
);