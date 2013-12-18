#include <et/camera/camera.h>
#include <et/geometry/geometry.h>
#include <et/primitives/primitives.h>
#include <et-ext/atmosphere/atmosphere.h>

using namespace et;

/*
extern const std::string atmospherePerPixelVS;
extern const std::string atmospherePerPixelFS;
extern const std::string planetPerPixelVS;
extern const std::string planetPerPixelFS;
*/
extern const std::string atmospherePerVertexVS;
extern const std::string atmospherePerVertexFS;
extern const std::string planetPerVertexVS;
extern const std::string planetPerVertexFS;

#define DRAW_WIREFRAME	0

Atmosphere::Atmosphere(RenderContext* rc, size_t textureSize) :
	_rc(rc)
{
	setParameters(defaultParameters());
	
	ObjectsCache localCache;
	
	_atmospherePerVertexProgram = rc->programFactory().genProgram("et~atmosphere~per-vertex~program",
		atmospherePerVertexVS, atmospherePerVertexFS);
	setProgramParameters(_atmospherePerVertexProgram);

	setPlanetFragmentShader(defaultPlanetFragmentShader());

/*
	_atmospherePerPixelProgram = rc->programFactory().genProgram("et~atmosphere~per-pixel~program",
		atmospherePerPixelVS, atmospherePerPixelFS);
	setProgramParameters(_atmospherePerPixelProgram);

	_planetPerPixelProgram = rc->programFactory().genProgram("et~planet~per-pixel~program",
		planetPerPixelVS, planetPerPixelFS);
	setProgramParameters(_planetPerPixelProgram);
		
	_framebuffer = rc->framebufferFactory().createCubemapFramebuffer(textureSize, "et~atmosphere~cubemap",
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
*/
	
	generateGeometry(rc);
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
	if (_parametersValid) return;
	
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
	
	int numSamples = _parameters.integerForKey(kIterationCount)->content;
	
	float fKr4PI = fKr * 4.0f * PI;
	float fKm4PI = fKm * 4.0f * PI;
	
	prog->setUniform("ambientColor", _ambientColor);
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
	auto va = VertexArray::Pointer::create(VertexDeclaration(true, Usage_Position, Type_Vec3), 0);
	auto ia = IndexArray::Pointer::create(IndexArrayFormat_32bit, 0, PrimitiveType_Triangles);
	primitives::createIcosahedron(va, 1.0f, true, false, false);
	for (size_t i = 0; i < 7; ++i)
	{
		primitives::tesselateTriangles(va, ia);
		ia->resize(va->size());
		ia->linearize(va->size());
		va = primitives::buildLinearIndexArray(va, ia);
	}
	
	RawDataAcessor<vec3> pos = va->chunk(Usage_Position).accessData<vec3>(0);
	for (size_t i = 0, e = va->size(); i < e; ++i)
		pos[i] = normalize(pos[i]);
	
	_atmosphereVAO = rc->vertexBufferFactory().createVertexArrayObject("et~sky-sphere", va,
		BufferDrawType_Static, ia, BufferDrawType_Static);
}

void Atmosphere::renderAtmosphereWithGeometry(const Camera& cam)
{
	auto& rs = _rc->renderState();
	
	Camera adjustedCamera(cam);
	adjustedCamera.lookAt(_cameraPosition, _cameraPosition - cam.direction());

	rs.bindVertexArray(_atmosphereVAO);
	rs.setDepthMask(true);
	rs.setDepthTest(true);
	
	bool shouldDrawPlanet =
		adjustedCamera.frustum().containSphere(Sphere(vec3(0.0f), _parameters.floatForKey(kPlanetRadius)->content));

	if (shouldDrawPlanet)
	{
		rs.setBlend(false);
		rs.setCulling(true, CullState_Back);
		rs.bindProgram(_planetPerVertexProgram);
		setProgramParameters(_planetPerVertexProgram);
		_planetPerVertexProgram->setPrimaryLightPosition(_lightDirection);
		_planetPerVertexProgram->setCameraProperties(adjustedCamera);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
		
#if (DRAW_WIREFRAME)
		rs.setDepthFunc(DepthFunc_LessOrEqual);
		rs.setBlend(true, BlendState_Additive);
		rs.setWireframeRendering(true);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
		rs.setWireframeRendering(false);
#endif
	}
	
	rs.setBlend(true, BlendState_Additive);
	rs.setCulling(true, CullState_Front);
//	rs.bindVertexArray(_gridVAO);
	rs.bindProgram(_atmospherePerVertexProgram);
	setProgramParameters(_atmospherePerVertexProgram);

	_atmospherePerVertexProgram->setUniform("mInverseMVPMatrix", adjustedCamera.inverseModelViewProjectionMatrix());
	_atmospherePerVertexProgram->setPrimaryLightPosition(_lightDirection);
	_atmospherePerVertexProgram->setCameraProperties(adjustedCamera);
	_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	
#if (DRAW_WIREFRAME)
	rs.setWireframeRendering(true);
	_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	rs.setWireframeRendering(false);
	rs.setDepthFunc(DepthFunc_Less);
#endif
	
	rs.setCulling(true, CullState_Back);
	
	_parametersValid |= shouldDrawPlanet;
}

void Atmosphere::performRendering(bool shouldClear, bool renderPlanet)
{
/*
	auto& rs = _rc->renderState();

	auto boundFramebuffer = rs.boundFramebuffer();
	CullState cullState = rs.cullState();
	BlendState blendState = rs.blendState();
	bool blendEnabled = rs.blendEnabled();
	bool cullEnabled = rs.cullEnabled();
	bool depthTest = rs.depthTestEnabled();
	bool depthMask = rs.depthMask();
	vec4 clearColor = rs.clearColor();
	
	if (renderPlanet)
	{
		rs.bindProgram(_planetPerPixelProgram);
		setProgramParameters(_planetPerPixelProgram);
	}
	
	rs.bindProgram(_atmospherePerPixelProgram);
	setProgramParameters(_atmospherePerPixelProgram);
	
	if (shouldClear)
		rs.setClearColor(_ambientColor);
	
	rs.setDepthTest(true);
	rs.setBlend(true, BlendState_Additive);
	rs.setCulling(true, CullState_Front);
	rs.bindVertexArray(_atmosphereVAO);
	rs.bindFramebuffer(_framebuffer);
	for (uint32_t i = 0; i < 6; ++i)
	{
		_framebuffer->setCurrentCubemapFace(i);
		
		if (shouldClear)
		{
			rs.setDepthMask(true);
			_rc->renderer()->clear(true, true);
		}
		
		rs.setDepthMask(false);
		_atmospherePerPixelProgram->setMVPMatrix(_cubemapMatrices[i]);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
		
		if (renderPlanet)
		{
			rs.setDepthMask(true);
			rs.setBlend(false);
			rs.setCulling(true, CullState_Back);
			rs.bindProgram(_planetPerPixelProgram);
			_planetPerPixelProgram->setMVPMatrix(_cubemapMatrices[i]);
			_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
			rs.setBlend(true, BlendState_Additive);
			rs.bindProgram(_atmospherePerPixelProgram);
			rs.setCulling(true, CullState_Front);
		}
	}
	
	rs.bindFramebuffer(boundFramebuffer);
	rs.setBlend(blendEnabled, blendState);
	rs.setCulling(cullEnabled, cullState);
	rs.setDepthMask(depthMask);
	rs.setDepthTest(depthTest);
	
	if (shouldClear)
		rs.setClearColor(clearColor);
	
	_parametersValid = true;
*/
}

Texture Atmosphere::environmentTexture()
{
	return Texture(); // _framebuffer->renderTarget();
}

void Atmosphere::setLightDirection(const vec3& l)
{
	_lightDirection = l;
} 

void Atmosphere::setParameters(Dictionary d)
{
	_parameters->content = d->content;
	
	float innerRadius = _parameters.floatForKey(kPlanetRadius)->content;
	float atmosphereHeight = _parameters.floatForKey(kAtmosphereHeight)->content;
	float lat = _parameters.floatForKey(kLatitude)->content;
	float lon = _parameters.floatForKey(kLongitude)->content;
	float h = innerRadius + atmosphereHeight * _parameters.floatForKey(kHeightAboveSurface)->content;

	_ambientColor.x = _parameters.floatForKeyPath({kAmbientColor, "0"})->content;
	_ambientColor.y = _parameters.floatForKeyPath({kAmbientColor, "1"})->content;
	_ambientColor.z = _parameters.floatForKeyPath({kAmbientColor, "2"})->content;
	
	_cameraPosition = h * fromSpherical(lat, lon);
	
/*
	float outerRadius = innerRadius + atmosphereHeight;
	_cubemapCamera.perspectiveProjection(HALF_PI, 1.0f, 0.1f, 2.0f * outerRadius);
	_cubemapMatrices = cubemapMatrixProjectionArray(_cubemapCamera.modelViewProjectionMatrix(), _cameraPosition);
*/
	_parametersValid = false;
}

void Atmosphere::setPlanetFragmentShader(const std::string& shader)
{
	_parametersValid = false;
	
	_planetPerVertexProgram = _rc->programFactory().genProgram("et~planet~per-vertex~program",
		planetPerVertexVS, shader);
	
	setProgramParameters(_planetPerVertexProgram);
}

const std::string& Atmosphere::defaultPlanetFragmentShader()
{
	return planetPerVertexFS;
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
	uniform mat4 mModelViewProjection; \
	uniform vec4 ambientColor; \
	uniform vec3 vCamera; \
	uniform vec3 vPrimaryLight; \
	uniform vec3 vInvWavelength; \
	uniform float fOuterRadius; \
	uniform float fOuterRadius2; \
	uniform float fInnerRadius; \
	uniform float fInnerRadius2; \
	uniform float fKrESun; \
	uniform float fKmESun; \
	uniform float fKr4PI; \
	uniform float fKm4PI; \
	uniform float g; \
	uniform float g2; \
	uniform float fScale; \
	uniform float fScaleDepth; \
	uniform float fScaleOverScaleDepth; \
	uniform float fSamples; \
	uniform int nSamples; \

#define SCALE_FUNCTION \
	float scale(float fCos) \
	{ \
		float x = 1.0 - fCos; \
		return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287); \
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
		{ \
			float fHeight = length(vSamplePoint); \
			float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight)); \
			float fLightAngle = dot(vPrimaryLight, vSamplePoint) / fHeight; \
			float fCameraAngle = dot(vRay, vSamplePoint) / fHeight; \
			float fScatter = fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle)); \
			color += exp(-fScatter * vScatterScale) * (fDepth * scaledLength); \
			vSamplePoint += vSampleRay; \
		} \
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
	
	uniform mat4 mInverseMVPMatrix;
	
	etVertexIn vec3 Vertex;
	
	etVertexOut vec3 vVertex;
	etVertexOut vec3 vDirection;
	etVertexOut vec3 aFrontColor;
	etVertexOut vec3 aSecondaryColor;
	etVertexOut vec3 aSourceColor;
	
	void main(void)
	{
		/*
		vec4 farFarAway = mInverseMVPMatrix * vec4(Vertex, 1.0, 1.0);
		vec3 direction = normalize(farFarAway.xyz / farFarAway.w - vCamera);
		float b = 2.0f * dot(direction, vCamera);
		float c = dot(vCamera, vCamera) - fOuterRadius2;
		float d = sqrt(max(0.0, b * b - 4.0f * c));
		vVertex = vCamera + 0.5 * (-b + d) * direction;
		*/
		vVertex = normalize(Vertex) * fOuterRadius;
		
		float fCameraHeight = length(vCamera);
		vec3 vRay = vVertex - vCamera;
		float fFar = length(vRay);
		vRay /= fFar;

		float fStartAngle = dot(vRay, vCamera) / fCameraHeight;
		float fStartOffset = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight)) * scale(fStartAngle);
		
		vec3 vFrontColor = solveScatteringIntegral(vCamera, vRay, fFar, fStartOffset);
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
 PRECISION_STRING
 ATMOSPHERE_UNIFORMS

 etFragmentIn vec3 vVertex;
 etFragmentIn vec3 vDirection;
 etFragmentIn vec3 aFrontColor;
 etFragmentIn vec3 aSecondaryColor;
 
 void main (void)
 {
	 float fCos = dot(vPrimaryLight, vDirection) / length(vDirection);
	 float onePlusfCos2 = 1.0 + fCos * fCos;
	 float fRayleighPhase = 0.75 * onePlusfCos2;
	 float fMiePhase = max(0.0, 1.5 * ((1.0 - g2) / (2.0 + g2)) * onePlusfCos2 / pow(1.0 + g2 - 2.0 * g * fCos, 1.5));
	 vec3 aColor = fRayleighPhase * aFrontColor + fMiePhase * aSecondaryColor;
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

/*
 *
 * Planet fragment shader
 *
 */

const std::string planetPerVertexFS = ET_TO_CONST_CHAR
(
 PRECISION_STRING

 etFragmentIn vec3 aColor;
 
 void main (void)
 {
	 etFragmentOut = vec4(1.0 - exp(-aColor), 1.0);
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
