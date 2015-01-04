#include <et/core/tools.h>
#include <et/camera/camera.h>
#include <et/geometry/geometry.h>
#include <et/primitives/primitives.h>
#include <et/imaging/imagewriter.h>
#include <et-ext/atmosphere/atmosphere.h>

using namespace et;

extern const std::string atmospherePerVertexVS;
extern const std::string atmospherePerVertexFS;

extern const std::string planetPerVertexVS;
extern const std::string planetPerVertexSimpleVS;
extern const std::string planetPerVertexFS;

#define ET_ATMOSPHERE_DRAW_WIREFRAME_OVERLAY	0

Atmosphere::Atmosphere(RenderContext* rc) :
	_rc(rc)
{
	setParameters(defaultParameters());
	
	ObjectsCache localCache;
	
	_atmospherePerVertexProgram = rc->programFactory().genProgram("et~atmosphere~per-vertex~program",
		atmospherePerVertexVS, atmospherePerVertexFS);
	setProgramParameters(_atmospherePerVertexProgram);

	setPlanetFragmentShader(defaultPlanetFragmentShader());
	
	generateGeometry(rc);
}

Dictionary Atmosphere::defaultParameters()
{
	Dictionary result;
	
	ArrayValue waveLength;
	waveLength->content.push_back(FloatValue(0.720f));
	waveLength->content.push_back(FloatValue(0.550f));
	waveLength->content.push_back(FloatValue(0.480f));
	
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
	if (_skyParametersValid) return;
	
	float fRayleighScaleDepth = _parameters.floatForKey(kRayleighScaleDepth)->content;
	float fESun = _parameters.floatForKey(kSunExponent)->content;
	float fKr = _parameters.floatForKey(kKr)->content;
	float fKm = _parameters.floatForKey(kKm)->content;
	float fG = _parameters.floatForKey(kG)->content;
	float innerRadius = _parameters.floatForKey(kPlanetRadius)->content;
	float atmosphereHeight = _parameters.floatForKey(kAtmosphereHeight)->content;
	float outerRadius = innerRadius + atmosphereHeight;
	
	vec3 waveLength(1.0f / std::pow(_parameters.floatForKeyPath({kWaveLength, "0"})->content, 4.0f),
		1.0f / std::pow(_parameters.floatForKeyPath({kWaveLength, "1"})->content, 4.0f),
		1.0f / std::pow(_parameters.floatForKeyPath({kWaveLength, "2"})->content, 4.0f));
	
	int numSamples = _parameters.integerForKey(kIterationCount)->content & 0xffffffff;
	
	float fKr4PI = fKr * 4.0f * PI;
	float fKm4PI = fKm * 4.0f * PI;
	
	prog->setUniform("nSamples", numSamples);
	prog->setUniform("fSamples", static_cast<float>(numSamples));
	prog->setUniform("vInvWavelength", waveLength);
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
	prog->setPrimaryLightPosition(_lightDirection);
	prog->setCameraPosition(_cameraPosition);
}

void Atmosphere::generateGeometry(RenderContext* rc)
{
	auto va = VertexArray::Pointer::create(VertexDeclaration(true, VertexAttributeUsage::Position, VertexAttributeType::Vec3), 0);
	auto ia = IndexArray::Pointer::create(IndexArrayFormat::Format_32bit, 0, PrimitiveType::Triangles);
	primitives::createIcosahedron(va, 1.0f, true, false, false);
	for (size_t i = 0; i < 7; ++i)
	{
		primitives::tesselateTriangles(va, ia);
		ia->resize(va->size());
		ia->linearize(va->size());
		va = primitives::buildLinearIndexArray(va, ia);
	}
	
	RawDataAcessor<vec3> pos = va->chunk(VertexAttributeUsage::Position).accessData<vec3>(0);
	for (size_t i = 0, e = va->size(); i < e; ++i)
		pos[i] = normalize(pos[i]);
	
	_atmosphereVAO = rc->vertexBufferFactory().createVertexArrayObject("et~sky-sphere", va,
		BufferDrawType::Static, ia, BufferDrawType::Static);
}

void Atmosphere::renderAtmosphereWithGeometry(const Camera& baseCamera, bool drawSky, bool drawPlanet)
{
	ET_ASSERT(drawSky || drawPlanet);
	
	auto& rs = _rc->renderState();
	
	Camera adjustedCamera(baseCamera);
	adjustedCamera.setPosition(_cameraPosition);

	rs.bindVertexArray(_atmosphereVAO);
	
	bool shouldDrawPlanet = drawPlanet &&
		adjustedCamera.frustum().containsSphere(Sphere(vec3(0.0f), _parameters.floatForKey(kPlanetRadius)->content));

	if (shouldDrawPlanet)
	{
		rs.setBlend(false);
		rs.setCulling(true, CullState::Back);
		rs.bindProgram(_planetPerVertexProgram);
		
		if (!_groundParametersValid)
		{
			setProgramParameters(_planetPerVertexProgram);
			_groundParametersValid = true;
		}
		
		_planetPerVertexProgram->setPrimaryLightPosition(_lightDirection);
		_planetPerVertexProgram->setCameraProperties(adjustedCamera);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
		
#	if (ET_ATMOSPHERE_DRAW_WIREFRAME_OVERLAY)
		rs.setDepthFunc(DepthFunc_LessOrEqual);
		rs.setBlend(true, BlendState_Additive);
		rs.setWireframeRendering(true);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
		rs.setWireframeRendering(false);
		rs.setDepthFunc(DepthFunc_Less);
#	endif
	}
	
	if (drawSky)
	{
		CullState lastCullState = rs.cullState();
		BlendState lastBlendState = rs.blendState();
		bool cullingEnabled = rs.cullEnabled();
		bool depthMaskEnabled = rs.depthMask();
		bool blendEnabled = rs.blendEnabled();
		
		rs.setDepthMask(false);
		rs.setBlend(true, BlendState::Additive);
		rs.setCulling(true, CullState::Front);
		rs.bindProgram(_atmospherePerVertexProgram);
		setProgramParameters(_atmospherePerVertexProgram);
		_atmospherePerVertexProgram->setCameraProperties(adjustedCamera);
		_atmospherePerVertexProgram->setPrimaryLightPosition(_lightDirection);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
		
#	if (ET_ATMOSPHERE_DRAW_WIREFRAME_OVERLAY)
		rs.setWireframeRendering(true);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
		rs.setWireframeRendering(false);
		rs.setDepthFunc(DepthFunc_Less);
#	endif
		
		rs.setBlend(blendEnabled, lastBlendState);
		rs.setCulling(cullingEnabled, lastCullState);
		rs.setDepthMask(depthMaskEnabled);
	}
}

void Atmosphere::generateCubemap(et::Framebuffer::Pointer framebuffer)
{
	ET_ASSERT(framebuffer->isCubemap());
	
	static const vec3 lookDirections[] =
	{
		vec3(+1.0f, 0.0f, 0.0f),
		vec3(-1.0f, 0.0f, 0.0f),
		vec3(0.0f, +1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, 0.0f, +1.0f),
		vec3(0.0f, 0.0f, -1.0f),
	};
	
	static const vec3 upVectors[] =
	{
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, 0.0f, +1.0f),
		vec3(0.0f, 0.0f, -1.0f),
		vec3(0.0f, -1.0f, 0.0f),
		vec3(0.0f, -1.0f, 0.0f),
	};
	
	Camera cubemapCamera;
	cubemapCamera.perspectiveProjection(HALF_PI, 1.0f, 1.0f, _cameraPosition.dotSelf());
	
	auto& rs = _rc->renderState();
	
	CullState lastCullState = rs.cullState();
	BlendState lastBlendState = rs.blendState();
	bool cullingEnabled = rs.cullEnabled();
	bool depthMaskEnabled = rs.depthMask();
	bool blendEnabled = rs.blendEnabled();
	bool depthTestEnabled = rs.depthTestEnabled();
	
	rs.setDepthMask(false);
	rs.setDepthTest(false);
	rs.setBlend(true, BlendState::Additive);
	rs.setCulling(true, CullState::Front);
	
	rs.bindProgram(_atmospherePerVertexProgram);
	setProgramParameters(_atmospherePerVertexProgram);
	_atmospherePerVertexProgram->setPrimaryLightPosition(_lightDirection);

	rs.bindVertexArray(_atmosphereVAO);
	rs.bindFramebuffer(framebuffer);
	for (uint32_t i = 0; i < 6; ++i)
	{
		framebuffer->setCurrentCubemapFace(i);
		cubemapCamera.lookAt(vec3(0.0f), lookDirections[i], upVectors[i]);
		cubemapCamera.setPosition(_cameraPosition);
		
		_rc->renderer()->clear(true, false);
		_atmospherePerVertexProgram->setCameraProperties(cubemapCamera);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	}
	
	rs.setDepthTest(depthTestEnabled);
	rs.setBlend(blendEnabled, lastBlendState);
	rs.setCulling(cullingEnabled, lastCullState);
	rs.setDepthMask(depthMaskEnabled);
}

void Atmosphere::setLightDirection(const vec3& l)
{
	_lightDirection = l.normalized();
} 

void Atmosphere::setParameters(const Dictionary& d)
{
	_parameters->content = d->content;
	
	float innerRadius = _parameters.floatForKey(kPlanetRadius)->content;
	float atmosphereHeight = _parameters.floatForKey(kAtmosphereHeight)->content;
	float lat = _parameters.floatForKey(kLatitude)->content;
	float lon = _parameters.floatForKey(kLongitude)->content;
	float h = innerRadius + atmosphereHeight * _parameters.floatForKey(kHeightAboveSurface)->content;
	
	_cameraPosition = h * fromSpherical(lat, lon);
	
	_skyParametersValid = false;
	_groundParametersValid = false;
}

void Atmosphere::setPlanetFragmentShader(const std::string& shader)
{
	_skyParametersValid = false;
	_groundParametersValid = false;
	
	_planetPerVertexProgram = _rc->programFactory().genProgram("et~planet~per-vertex~program",
		_computeScatteringOnPlanet ? planetPerVertexVS : planetPerVertexSimpleVS, shader);
	
	setProgramParameters(_planetPerVertexProgram);
}

const std::string& Atmosphere::defaultPlanetFragmentShader()
{
	return planetPerVertexFS;
}

Program::Pointer Atmosphere::planetProgram()
{
	return _planetPerVertexProgram;
}

void Atmosphere::setShouldComputeScatteringOnPlanet(bool c)
{
	_computeScatteringOnPlanet = c;
}

/*
 *
 * Shaders
 *
 */
#if (ET_PLATFORM_WIN)
#	define PRECISION_STRING
#else
#	define PRECISION_STRING precision highp float;
#endif

#define ATMOSPHERE_UNIFORMS \
	uniform etHighp mat4 mModelViewProjection; \
	uniform etHighp vec3 vCamera; \
	uniform etHighp vec3 vPrimaryLight; \
	uniform etHighp vec3 vInvWavelength; \
	uniform etHighp float fOuterRadius; \
	uniform etHighp float fOuterRadius2; \
	uniform etHighp float fInnerRadius; \
	uniform etHighp float fInnerRadius2; \
	uniform etHighp float fKrESun; \
	uniform etHighp float fKmESun; \
	uniform etHighp float fKr4PI; \
	uniform etHighp float fKm4PI; \
	uniform etHighp float g; \
	uniform etHighp float g2; \
	uniform etHighp float fScale; \
	uniform etHighp float fScaleDepth; \
	uniform etHighp float fScaleOverScaleDepth; \
	uniform etHighp float fSamples; \
	uniform etHighp int nSamples; \

#define SCALE_FUNCTION \
	float scale(float fCos) \
	{ \
		float x = 1.0 - fCos; \
		return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287); \
	}

#define SCATTERING_INTEGRAL_FUNCTION_INTERIOR \
{ \
	float fHeight = length(vSamplePoint); \
	float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight)); \
	float fLightAngle = dot(vPrimaryLight, vSamplePoint) / fHeight; \
	float fCameraAngle = dot(vRay, vSamplePoint) / fHeight; \
	float fScatter = fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle)); \
	color += exp(-fScatter * vScatterScale) * (fDepth * scaledLength); \
	vSamplePoint += vSampleRay; \
}

#define SCATTERING_INTEGRAL_FUNCTION  \
	vec3 solveScatteringIntegral(in vec3 vSamplePoint, in vec3 vRay, in float fFar, in float fStartOffset) \
	{ \
		float sampleLength = fFar / fSamples; \
		float scaledLength = fScale * sampleLength; \
		vec3 vSampleRay = sampleLength * vRay; \
		vec3 vScatterScale = vInvWavelength * fKr4PI + fKm4PI; \
		vec3 color = vec3(0.0); \
		for (int i = 0; i < nSamples; i++) \
			SCATTERING_INTEGRAL_FUNCTION_INTERIOR \
		return color; \
	} \
	vec3 solveFastScatteringIntegral(in vec3 vSamplePoint, in vec3 vRay, in float fFar, in float fStartOffset) \
	{ \
		float sampleLength = fFar / fSamples; \
		float scaledLength = fScale * sampleLength; \
		vec3 vSampleRay = sampleLength * vRay; \
		vec3 vScatterScale = vInvWavelength * fKr4PI + fKm4PI; \
		vec3 color = vec3(0.0); \
		SCATTERING_INTEGRAL_FUNCTION_INTERIOR \
		SCATTERING_INTEGRAL_FUNCTION_INTERIOR \
		SCATTERING_INTEGRAL_FUNCTION_INTERIOR \
		SCATTERING_INTEGRAL_FUNCTION_INTERIOR \
		SCATTERING_INTEGRAL_FUNCTION_INTERIOR \
		return color; \
	}
/*
 *
 * Atmosphere vertex shader
 *
 */
const std::string atmospherePerVertexVS = ET_TO_CONST_CHAR
(
 ATMOSPHERE_UNIFORMS
 SCALE_FUNCTION
 SCATTERING_INTEGRAL_FUNCTION
 
 etVertexIn vec3 Vertex;
 
 etVertexOut vec3 vVertex;
 etVertexOut vec3 vDirection;
 etVertexOut vec3 aFrontColor;
 etVertexOut vec3 aSecondaryColor;
 etVertexOut vec3 aSourceColor;
 
 void main(void)
 {
	 vVertex = normalize(Vertex) * fOuterRadius;
	 
	 float fCameraHeight = length(vCamera);
	 
	 vec3 vRay = vVertex - vCamera;
	 float fFar = length(vRay);
	 vRay /= fFar;
	 
	 float fStartAngle = dot(vRay, vCamera) / fCameraHeight;
	 float fStartOffset = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight)) * scale(fStartAngle);
	 
	 vec3 vFrontColor = solveFastScatteringIntegral(vCamera, vRay, fFar, fStartOffset);
	 
	 vDirection = vCamera - vVertex;
	 aFrontColor = fKrESun * (vFrontColor * vInvWavelength);
	 aSecondaryColor = fKmESun * vFrontColor;
	 
	 gl_Position = mModelViewProjection * vec4(vVertex, 1.0);
 }
 );

/*
 *
 * Atmosphere fragment shader
 *
 */

const std::string atmospherePerVertexFS = ET_TO_CONST_CHAR
(
 ATMOSPHERE_UNIFORMS
 
 etFragmentIn etHighp vec3 vVertex;
 etFragmentIn etHighp vec3 vDirection;
 etFragmentIn etHighp vec3 aFrontColor;
 etFragmentIn etHighp vec3 aSecondaryColor;
 
 void main (void)
 {
	 etHighp float fCos = dot(vPrimaryLight, vDirection / length(vDirection));
	 
	 etHighp float onePlusfCos2 = 1.0 + fCos * fCos;
	 etHighp float fRayleighPhase = 0.75 * onePlusfCos2;
	 
	 etHighp float fMieDenom = 1.0 + g2 - 2.0 * g * fCos;
	 etHighp float fMiePhase = max(0.0, 1.5 * ((1.0 - g2) / (2.0 + g2)) * onePlusfCos2 / fMieDenom / sqrt(fMieDenom));
	 etHighp vec3 aColor = fRayleighPhase * aFrontColor + fMiePhase * aSecondaryColor;
	 
	 etFragmentOut = vec4(1.0 - exp(-aColor), 1.0);
 }
 );

/*
 *
 * Planet vertex shader
 *
 */
const std::string planetPerVertexVS = ET_TO_CONST_CHAR
(
 ATMOSPHERE_UNIFORMS
 SCALE_FUNCTION
 SCATTERING_INTEGRAL_FUNCTION
 
 etVertexIn vec3 Vertex;
 etVertexOut vec3 vVertexWS;
 etVertexOut vec3 aColor;
 
 void main(void)
 {
	 vVertexWS = fInnerRadius * Vertex;
	 
	 float b = 2.0 * dot(vPrimaryLight, fOuterRadius * Vertex);
	 float d = sqrt(max(0.0, b * b - 4.0 * (dot(vVertexWS, vVertexWS) - fOuterRadius2)));
	 
	 vec3 skyToGoundColor = solveScatteringIntegral(vVertexWS, vPrimaryLight, 0.5 * (d - b),
		scale(dot(vPrimaryLight, Vertex)));
	 
	 vec3 vCameraWS = vCamera - vVertexWS;
	 vec3 vRay = normalize(vCameraWS);
	 float fCameraHeight = length(vCamera);
	 float fStartAngle = dot(vRay, vCamera) / fCameraHeight;
	 float fStartOffset = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight)) * scale(fStartAngle);
	 vec3 groundToCameraColor = solveScatteringIntegral(vVertexWS, vRay, length(vCameraWS), fStartOffset);
	 
	 aColor = (skyToGoundColor * groundToCameraColor) * (fKrESun * vInvWavelength + fKmESun);
	 
	 gl_Position = mModelViewProjection * vec4(vVertexWS, 1.0);
 }
 );

const std::string planetPerVertexSimpleVS = ET_TO_CONST_CHAR
(
 ATMOSPHERE_UNIFORMS
 
 etVertexIn vec3 Vertex;
 etVertexOut vec3 vVertexWS;
 etVertexOut vec3 aColor;
 
 void main(void)
 {
	 vVertexWS = fInnerRadius * Vertex;
	 aColor = vec3(1.0);
	 gl_Position = mModelViewProjection * vec4(vVertexWS, 1.0);
 }
 );

/*
 *
 * Planet fragment shader
 *
 */

const std::string planetPerVertexFS = ET_TO_CONST_CHAR
(
 etFragmentIn etHighp vec3 aColor;
 
 void main (void)
 {
	 etFragmentOut = vec4(1.0 - exp(-aColor), 1.0);
 }
 );

/*
 * Constants
 */
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
