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

Atmosphere::Atmosphere(RenderContext* rc, size_t textureSize) :
	_rc(rc)
{
	ObjectsCache localCache;
	
	_atmospherePerVertexProgram = rc->programFactory().genProgram("et~atmosphere~per-vertex~program",
		atmospherePerVertexVS, atmospherePerVertexFS);
	setProgramParameters(_atmospherePerVertexProgram);

	_atmospherePerPixelProgram = rc->programFactory().genProgram("et~atmosphere~per-pixel~program",
		atmospherePerPixelVS, atmospherePerPixelFS);
	setProgramParameters(_atmospherePerPixelProgram);
		
	_framebuffer = rc->framebufferFactory().createCubemapFramebuffer(textureSize, "et~atmosphere~cubemap",
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 0);
	
	generateGeometry(rc);
	
	setParameters(defaultParameters());
}

Dictionary Atmosphere::defaultParameters()
{
	Dictionary result;
	
	ArrayValue waveLength;
	waveLength->content.push_back(FloatValue(0.720f));
	waveLength->content.push_back(FloatValue(0.550f));
	waveLength->content.push_back(FloatValue(0.480f));

	ArrayValue ambientColor;
	ambientColor->content.push_back(FloatValue(0.0f));
	ambientColor->content.push_back(FloatValue(0.0f));
	ambientColor->content.push_back(FloatValue(0.0f));
	
	result.setArrayForKey(kAmbientColor, ambientColor);
	result.setArrayForKey(kWaveLength, waveLength);
	
	result.setFloatForKey(kKr, 0.0025f);
	result.setFloatForKey(kKm, 0.0015f);
	result.setFloatForKey(kG, -0.999f);
	result.setFloatForKey(kSunExponent, 25.0f);
	result.setFloatForKey(kRayleighScaleDepth, 1.0f / 3.0f);
	result.setFloatForKey(kPlanetRadius, 100.0f);
	result.setFloatForKey(kAtmosphereHeight, 5.0f);
	result.setFloatForKey(kLatitude, HALF_PI);
	result.setFloatForKey(kLongitude, 0.0f);
	result.setFloatForKey(kHeightAboveSurface, 0.0f);
	result.setIntegerForKey(kIterationCount, 10);
	
	return result;
}

void Atmosphere::setProgramParameters(Program::Pointer prog)
{
	vec3 waveLength;
	waveLength.x = _parameters.floatForKeyPath({kWaveLength, "0"})->content;
	waveLength.y = _parameters.floatForKeyPath({kWaveLength, "1"})->content;
	waveLength.z = _parameters.floatForKeyPath({kWaveLength, "2"})->content;
	
	float fRayleighScaleDepth = _parameters.floatForKey(kRayleighScaleDepth)->content;
	float fESun = _parameters.floatForKey(kSunExponent)->content;
	float fKr = _parameters.floatForKey(kKr)->content;
	float fKm = _parameters.floatForKey(kKm)->content;
	float fG = _parameters.floatForKey(kG)->content;
	float innerRadius = _parameters.floatForKey(kPlanetRadius)->content;
	float atmosphereHeight = _parameters.floatForKey(kAtmosphereHeight)->content;
	float outerRadius = innerRadius + atmosphereHeight;
	float lat = _parameters.floatForKey(kLatitude)->content;
	float lon = _parameters.floatForKey(kLongitude)->content;
	float h = innerRadius + atmosphereHeight * _parameters.floatForKey(kHeightAboveSurface)->content;
	
	_ambientColor.x =  _parameters.floatForKeyPath({kAmbientColor, "0"})->content;
	_ambientColor.y =  _parameters.floatForKeyPath({kAmbientColor, "1"})->content;
	_ambientColor.z =  _parameters.floatForKeyPath({kAmbientColor, "2"})->content;
	
	int numSamples = _parameters.integerForKey(kIterationCount)->content;
	
	float fKr4PI = fKr * 4.0f * PI;
	float fKm4PI = fKm * 4.0f * PI;

	_cameraPosition = h * fromSpherical(lat, lon);
	
	vec3 invWaveLenth(1.0f / std::pow(waveLength.x, 4.0f), 1.0f / std::pow(waveLength.y, 4.0f),
		1.0f / std::pow(waveLength.z, 4.0f));
	
	_cubemapCamera.perspectiveProjection(HALF_PI, 1.0f, 0.25f * (outerRadius - innerRadius), 2.0f * outerRadius);
	_cubemapMatrices = cubemapMatrixProjectionArray(_cubemapCamera.modelViewProjectionMatrix(), _cameraPosition);
	
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
		pos[i] = normalize(pos[i]);
	
	_atmosphereVAO = rc->vertexBufferFactory().createVertexArrayObject("et~sky-sphere", va,
		BufferDrawType_Static, ia, BufferDrawType_Static);
}

void Atmosphere::performRendering(bool shouldClear)
{
	auto& rs = _rc->renderState();

	auto boundFramebuffer = rs.boundFramebuffer();
	CullState cullState = rs.cullState();
	BlendState blendState = rs.blendState();
	bool blendEnabled = rs.blendEnabled();
	bool cullEnabled = rs.cullEnabled();
	bool depthTest = rs.depthTestEnabled();
	bool depthMask = rs.depthMask();
	vec4 clearColor = rs.clearColor();
	
	rs.setBlend(true, BlendState_Additive);
	rs.setCulling(true, CullState_Front);
	rs.setDepthTest(false);
	rs.setDepthMask(false);
	
	rs.bindVertexArray(_atmosphereVAO);
	rs.bindProgram(_atmospherePerPixelProgram);
	
	if (!_parametersValid)
		setProgramParameters(_atmospherePerPixelProgram);
	
	_atmospherePerPixelProgram->setPrimaryLightPosition(_lightDirection);
	_atmospherePerPixelProgram->setCameraPosition(_cameraPosition);
	
	if (shouldClear)
		rs.setClearColor(_ambientColor);
	
	rs.bindFramebuffer(_framebuffer);
	for (uint32_t i = 0; i < 6; ++i)
	{
		_framebuffer->setCurrentCubemapFace(i);
		
		if (shouldClear)
			_rc->renderer()->clear(true, false);
		
		_atmospherePerPixelProgram->setMVPMatrix(_cubemapMatrices[i]);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	}
	
	rs.bindFramebuffer(boundFramebuffer);
	rs.setBlend(blendEnabled, blendState);
	rs.setCulling(cullEnabled, cullState);
	rs.setDepthMask(depthMask);
	rs.setDepthTest(depthTest);
	
	if (shouldClear)
		rs.setClearColor(clearColor);
	
	_parametersValid = true;
}

et::Texture Atmosphere::environmentTexture()
{
	return _framebuffer->renderTarget();
}

void Atmosphere::setLightDirection(const vec3& l)
{
	_lightDirection = l;
}

void Atmosphere::setParameters(Dictionary d)
{
	_parameters->content = d->content;
	_parametersValid = false;
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
		vec3 v3Ray = fOuterRadius * Vertex - vCamera;
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
		
		v3Direction = vCamera - fOuterRadius * Vertex;
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
	uniform float fOuterRadius;
	etVertexIn vec3 Vertex;
	etVertexOut vec3 vVertex;
	void main(void)
	{
		vVertex = fOuterRadius * Vertex;
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

/*
 * Constants
 */
const std::string Atmosphere::kAmbientColor = "ambient-color";

const std::string Atmosphere::kWaveLength = "wave-length";
const std::string Atmosphere::kKr = "kr";
const std::string Atmosphere::kKm = "km";
const std::string Atmosphere::kG = "g";
const std::string Atmosphere::kSunExponent = "sun-exponent";
const std::string Atmosphere::kRayleighScaleDepth = "rayleigh-scale-depth";

const std::string Atmosphere::kPlanetRadius = "planet-radius";
const std::string Atmosphere::kAtmosphereHeight = "atmosphere-height";

const std::string Atmosphere::kIterationCount = "iterations-count";

const std::string Atmosphere::kLatitude = "latitude";
const std::string Atmosphere::kLongitude = "longitude";
const std::string Atmosphere::kHeightAboveSurface = "height";
