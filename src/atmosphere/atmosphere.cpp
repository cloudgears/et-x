#include <et/camera/camera.h>
#include <et/geometry/geometry.h>
#include <et/primitives/primitives.h>
#include <et-ext/atmosphere/atmosphere.h>

using namespace et;

extern const std::string atmospherePerVertexVS;
extern const std::string atmospherePerVertexFS;

extern const std::string atmospherePerPixelVS;
extern const std::string atmospherePerPixelFS;

extern const std::string groundVS;
extern const std::string groundFS;

extern const std::string environmentVS;
extern const std::string environmentFS;

float innerRadius = 100.0f;
float outerRadius = 1.05f * innerRadius;

vec3 positionOnSphere = vec3(0.0f, innerRadius, 0.0f);

int numSamples = 10;

Atmosphere::Atmosphere(RenderContext* rc, size_t textureSize) :
	_rc(rc)
{
	ObjectsCache localCache;
	
	_cubemapCamera.perspectiveProjection(HALF_PI, 1.0f, 0.25f * (outerRadius - innerRadius), 2.0f * outerRadius);
	_cubemapMatrices = cubemapMatrixProjectionArray(_cubemapCamera.modelViewProjectionMatrix(), positionOnSphere);
	
	_atmospherePerVertexProgram = rc->programFactory().genProgram("et~atmosphere~per-vertex~program",
		atmospherePerVertexVS, atmospherePerVertexFS);
	setProgramParameters(_atmospherePerVertexProgram);

	_atmospherePerPixelProgram = rc->programFactory().genProgram("et~atmosphere~per-pixel~program",
		atmospherePerPixelVS, atmospherePerPixelFS);
	setProgramParameters(_atmospherePerPixelProgram);
		
	_framebuffer = rc->framebufferFactory().createCubemapFramebuffer(textureSize, "et~atmosphere~cubemap",
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 0);
	
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
	auto va = VertexArray::Pointer::create(VertexDeclaration(true, Usage_Position, Type_Vec3), 0);
	primitives::createIcosahedron(va, 1.0f, true, true, false);
	primitives::tesselateTriangles(va);
	primitives::tesselateTriangles(va);
	auto ia = IndexArray::Pointer::create(IndexArrayFormat_16bit, va->size(), PrimitiveType_Triangles);
	ia->linearize(va->size());
	va = primitives::buildIndexArray(va, ia);
	
	RawDataAcessor<vec3> pos = va->chunk(Usage_Position).accessData<vec3>(0);
	for (size_t i = 0, e = va->size(); i < e; ++i)
		pos[i] = outerRadius * normalize(pos[i]);
	
	_atmosphereVAO = rc->vertexBufferFactory().createVertexArrayObject("et~sky-sphere", va,
		BufferDrawType_Static, ia, BufferDrawType_Static);
}

void Atmosphere::performRendering()
{
	auto& rs = _rc->renderState();

	rs.setBlend(false);
	rs.setCulling(false);
	rs.setDepthTest(false);
	rs.setDepthMask(false);
	
	rs.bindVertexArray(_atmosphereVAO);
	rs.bindProgram(_atmospherePerPixelProgram);
	_atmospherePerPixelProgram->setPrimaryLightPosition(_lightDirection);
	_atmospherePerPixelProgram->setCameraPosition(positionOnSphere);
	
	rs.bindFramebuffer(_framebuffer);
	for (uint32_t i = 0; i < 6; ++i)
	{
		_framebuffer->setCurrentCubemapFace(i);
		_rc->renderer()->clear(true, false);
		
		_atmospherePerPixelProgram->setMVPMatrix(_cubemapMatrices[i]);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	}
	
	rs.setDepthTest(true);
	rs.setDepthMask(true);
	rs.bindDefaultFramebuffer();
	_textureValid = true;
}

void Atmosphere::updateTexture()
{
	if (!_textureValid)
		performRendering();
}

et::Texture Atmosphere::environmentTexture()
{
	assert(_textureValid);
	return _framebuffer->renderTarget();
}

void Atmosphere::setLightDirection(const vec3& l)
{
	_lightDirection = l;
	_textureValid = false;
}

////////////////////////////////////////////////////////////////
//
// Atmosphere per vertex
//
////////////////////////////////////////////////////////////////
const std::string atmospherePerVertexVS = ET_TO_CONST_CHAR
   (
	uniform mat4 mModelViewProjection;
	uniform vec3 vCamera;
	uniform vec3 vPrimaryLight;
	uniform vec3 v3InvWavelength;
	uniform float fOuterRadius;
	uniform float fOuterRadius2;
	uniform float fInnerRadius;
	uniform float fInnerRadius2;
	uniform float fKrESun;
	uniform float fKmESun;
	uniform float fKr4PI;
	uniform float fKm4PI;
	uniform float fScale;
	uniform float fScaleDepth;
	uniform float fScaleOverScaleDepth;
	uniform float fSamples;
	uniform int nSamples;
	
	etVertexIn vec3 Vertex;
	etVertexOut vec3 v3Direction;
	etVertexOut vec3 aFrontColor;
	etVertexOut vec3 aSecondaryColor;

	float scale(float fCos)
	{
		float x = 1.0 - fCos;
		return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287);
	}
	
	void main(void)
	{
		float fCameraHeight = length(vCamera);
		vec3 v3Ray = Vertex - vCamera;
		float fFar = length(v3Ray);
		v3Ray /= fFar;
	
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
			float C = fCameraHeight * fCameraHeight - fOuterRadius2;
			float fDet = max(0.0, B * B - 4.0 * C);
			float fNear = -0.5 * (B + sqrt(fDet));
			fFar -= fNear;
			
			v3Start += v3Ray * fNear;
			fStartAngle = dot(v3Ray, v3Start) / fOuterRadius;
			fStartOffset = exp(-1.0 / fScaleDepth) * scale(fStartAngle);
		}
		
		float fSampleLength = fFar / fSamples;
		float fScaledLength = fSampleLength * fScale;
		vec3 v3SampleRay = v3Ray * fSampleLength;
		vec3 v3SamplePoint = v3Start;
		
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
		
		v3Direction = vCamera - Vertex;
		aFrontColor = fKrESun * (v3FrontColor * v3InvWavelength);
		aSecondaryColor = fKmESun * v3FrontColor;
		
		gl_Position = mModelViewProjection * vec4(Vertex, 1.0);
	}
);

const std::string atmospherePerVertexFS = ET_TO_CONST_CHAR
   (
	precision highp float;
	uniform vec3 vPrimaryLight;
	uniform vec3 vCamera;
	
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
//
// Atmosphere per pixel
//
////////////////////////////////////////////////////////////////
const std::string atmospherePerPixelVS = ET_TO_CONST_CHAR
   (
	uniform mat4 mModelViewProjection;
	etVertexIn vec3 Vertex;
	etVertexOut vec3 vVertex;
	void main(void)
	{
		vVertex = Vertex;
		gl_Position = mModelViewProjection * vec4(vVertex, 1.0);
	}
);

const std::string atmospherePerPixelFS = ET_TO_CONST_CHAR
   (
	precision highp float;
	uniform vec3 vCamera;
	uniform vec3 vPrimaryLight;
	uniform vec3 v3InvWavelength;
	uniform float fOuterRadius;
	uniform float fOuterRadius2;
	uniform float fInnerRadius;
	uniform float fInnerRadius2;
	uniform float fKrESun;
	uniform float fKmESun;
	uniform float fKr4PI;
	uniform float fKm4PI;
	uniform float fScale;
	uniform float fScaleDepth;
	uniform float fScaleOverScaleDepth;
	uniform float fSamples;
	uniform int nSamples;
	uniform float g;
	uniform float g2;
	
	etFragmentIn vec3 vVertex;
	
	float scale(float fCos)
	{
		float x = 1.0 - fCos;
		return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287);
	}
	
	void main(void)
	{
		float fCameraHeight = length(vCamera);
		vec3 v3Ray = vVertex - vCamera;
		float fFar = length(v3Ray);
		v3Ray /= fFar;
		
/*
 *		uncomment to enable atmosphere from space
 *
 *
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
			float C = fCameraHeight * fCameraHeight - fOuterRadius2;
			float fDet = max(0.0, B * B - 4.0 * C);
			float fNear = -0.5 * (B + sqrt(fDet));
			fFar -= fNear;
			
			v3Start += v3Ray * fNear;
			fStartAngle = dot(v3Ray, v3Start) / fOuterRadius;
			fStartOffset = exp(-1.0 / fScaleDepth) * scale(fStartAngle);
		}
 *
 */
		vec3 v3Start = vCamera;
		float fStartAngle = dot(v3Ray, v3Start) / length(v3Start);
		float fStartOffset = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight)) * scale(fStartAngle);
		
		float fSampleLength = fFar / fSamples;
		float fScaledLength = fSampleLength * fScale;
		vec3 v3SampleRay = v3Ray * fSampleLength;
		vec3 v3SamplePoint = v3Start;
		
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
		
		vec3 aFrontColor = fKrESun * (v3FrontColor * v3InvWavelength);
		vec3 aSecondaryColor = fKmESun * v3FrontColor;
		
		float fCos = -dot(vPrimaryLight, v3Ray);
		float fRayleighPhase = 0.75 * (1.0 + fCos*fCos);
		float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);
		vec4 aColor = vec4(fRayleighPhase * aFrontColor + fMiePhase * aSecondaryColor, 1.0);
		etFragmentOut = 1.0 - exp(-aColor);
	}
);
