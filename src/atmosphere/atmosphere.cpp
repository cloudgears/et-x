#include <et/camera/camera.h>
#include <et/geometry/geometry.h>
#include <et/primitives/primitives.h>
#include <et-ext/atmosphere/atmosphere.h>

using namespace et;

extern const std::string atmospherePerVertexVS;
extern const std::string atmospherePerPixelVS;

extern const std::string planetPerVertexVS;
extern const std::string planetPerPixelVS;

extern const std::string atmospherePerVertexFS;
extern const std::string atmospherePerPixelFS;

extern const std::string planetPerVertexFS;
extern const std::string planetPerPixelFS;

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

	_atmospherePerPixelProgram = rc->programFactory().genProgram("et~atmosphere~per-pixel~program",
		atmospherePerPixelVS, atmospherePerPixelFS);
	setProgramParameters(_atmospherePerPixelProgram);

	_planetPerPixelProgram = rc->programFactory().genProgram("et~planet~per-pixel~program",
		planetPerPixelVS, planetPerPixelFS);
	setProgramParameters(_planetPerPixelProgram);
		
	_framebuffer = rc->framebufferFactory().createCubemapFramebuffer(textureSize, "et~atmosphere~cubemap",
		GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT);
	
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
		va = primitives::buildIndexArray(va, ia);
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

	bool shouldDrawPlanet =
		adjustedCamera.frustum().containSphere(Sphere(vec3(0.0f), _parameters.floatForKey(kPlanetRadius)->content));
	
	rs.bindVertexArray(_atmosphereVAO);
	rs.setBlend(false);
	rs.setDepthMask(true);
	rs.setDepthTest(true);

	if (shouldDrawPlanet)
	{
		rs.setCulling(true, CullState_Back);
		rs.bindProgram(_planetPerVertexProgram);
		_planetPerVertexProgram->setPrimaryLightPosition(_lightDirection);
		_planetPerVertexProgram->setCameraProperties(adjustedCamera);
		_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	}
	
	rs.setCulling(true, CullState_Front);
	rs.bindProgram(_atmospherePerVertexProgram);
	setProgramParameters(_atmospherePerVertexProgram);
	_atmospherePerVertexProgram->setPrimaryLightPosition(_lightDirection);
	_atmospherePerVertexProgram->setCameraProperties(adjustedCamera);
	_rc->renderer()->drawAllElements(_atmosphereVAO->indexBuffer());
	rs.setCulling(true, CullState_Back);
	
	_parametersValid |= shouldDrawPlanet;
}

void Atmosphere::performRendering(bool shouldClear, bool renderPlanet)
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
	
	float innerRadius = _parameters.floatForKey(kPlanetRadius)->content;
	float atmosphereHeight = _parameters.floatForKey(kAtmosphereHeight)->content;
	float outerRadius = innerRadius + atmosphereHeight;
	float lat = _parameters.floatForKey(kLatitude)->content;
	float lon = _parameters.floatForKey(kLongitude)->content;
	float h = innerRadius + atmosphereHeight * _parameters.floatForKey(kHeightAboveSurface)->content;

	_ambientColor.x = _parameters.floatForKeyPath({kAmbientColor, "0"})->content;
	_ambientColor.y = _parameters.floatForKeyPath({kAmbientColor, "1"})->content;
	_ambientColor.z = _parameters.floatForKeyPath({kAmbientColor, "2"})->content;
	
	_cameraPosition = h * fromSpherical(lat, lon);
	
	_cubemapCamera.perspectiveProjection(HALF_PI, 1.0f, 0.1f, 2.0f * outerRadius);
	_cubemapMatrices = cubemapMatrixProjectionArray(_cubemapCamera.modelViewProjectionMatrix(), _cameraPosition);
	
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

////////////////////////////////////////////////////////////////
//
// Atmosphere per vertex
//
////////////////////////////////////////////////////////////////
#if (ET_PLATFORM_WIN)
#	define PRECISION_STRING
#else
#	define PRECISION_STRING precision highp float;
#endif

const std::string atmospherePerVertexVS = ET_TO_CONST_CHAR
   (
	uniform mat4 mModelViewProjection;
	uniform vec3 vCamera;
	uniform vec3 vPrimaryLight;
	uniform vec3 vInvWavelength;
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
	etVertexOut vec3 vDirection;
	etVertexOut vec3 aFrontColor;
	etVertexOut vec3 aSecondaryColor;

	float scale(float fCos)
	{
		float x = 1.0 - fCos;
		return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287);
	}
	
	void main(void)
	{
		vec3 scaledVertex = fOuterRadius * Vertex;
		
		float fCameraHeight = length(vCamera);
		vec3 vRay = scaledVertex - vCamera;
		float fFar = length(vRay);
		vRay /= fFar;
/*
 * from space
 *
		{
			float B = 2.0 * dot(vCamera, vRay);
			float C = fCameraHeight * fCameraHeight - fOuterRadius2;
			float fDet = max(0.0, B * B - 4.0 * C);
			float fNear = -0.5 * (B + sqrt(fDet));
			fFar -= fNear;
			
			vStart += vRay * fNear;
			fStartAngle = dot(vRay, vStart) / fOuterRadius;
			fStartOffset = exp(-1.0 / fScaleDepth) * scale(fStartAngle);
		}
 */
		vec3 vStart = vCamera;
		float fStartAngle = dot(vRay, vStart) / length(vStart);
		float fStartOffset = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight)) * scale(fStartAngle);
		
		float fSampleLength = fFar / fSamples;
		float fScaledLength = fSampleLength * fScale;
		vec3 vSampleRay = vRay * fSampleLength;
		vec3 vSamplePoint = vStart;
		
		vec3 vFrontColor = vec3(0.0);
		for(int i = 0; i < nSamples; i++)
		{
			float fHeight = length(vSamplePoint);
			float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
			float fLightAngle = dot(vPrimaryLight, vSamplePoint) / fHeight;
			float fCameraAngle = dot(vRay, vSamplePoint) / fHeight;
			float fScatter = fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle));
			vec3 vAttenuate = exp(-fScatter * (vInvWavelength * fKr4PI + fKm4PI));
			vFrontColor += vAttenuate * (fDepth * fScaledLength);
			vSamplePoint += vSampleRay;
		}
		
		vDirection = vCamera - scaledVertex;
		aFrontColor = fKrESun * (vFrontColor * vInvWavelength);
		aSecondaryColor = fKmESun * vFrontColor;
		
		gl_Position = mModelViewProjection * vec4(scaledVertex, 1.0);
	}
);

const std::string atmospherePerVertexFS = ET_TO_CONST_CHAR
(
 PRECISION_STRING
 
 uniform vec3 vPrimaryLight;
 uniform vec3 vCamera;
 
 uniform float g;
 uniform float g2;
 
 etFragmentIn vec3 vDirection;
 etFragmentIn vec3 aFrontColor;
 etFragmentIn vec3 aSecondaryColor;
 
 void main (void)
 {
	 float fCos = dot(vPrimaryLight, vDirection) / length(vDirection);
	 float fRayleighPhase = 0.75 * (1.0 + fCos*fCos);
	 float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + fCos*fCos) / pow(1.0 + g2 - 2.0*g*fCos, 1.5);
	 vec4 aColor = vec4(fRayleighPhase * aFrontColor + fMiePhase * aSecondaryColor, 1.0);
	 etFragmentOut = 1.0 - exp(-aColor);
 }
 );

const std::string planetPerVertexVS = ET_TO_CONST_CHAR
(
 uniform mat4 mModelViewProjection;
 uniform vec3 vCamera;
 uniform vec3 vPrimaryLight;
 uniform vec3 vInvWavelength;
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
 
 etVertexOut vec3 vVertexWS;
 etVertexOut vec3 aColor;
 
 float scale(float fCos)
 {
	 float x = 1.0 - fCos;
	 return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287);
 }
 
 void main(void)
 {
	 vVertexWS = fInnerRadius * Vertex;
	 vec3 vRay = vVertexWS - vCamera;
	 float fFar = length(vRay);
	 vRay /= fFar;
	 
	 vec3 vStart = vCamera;
	 float fCameraHeight = length(vCamera);
	 float fDepth = exp((fInnerRadius - fCameraHeight) / fScaleDepth);
	 float fCameraAngle = -dot(vRay, vVertexWS) / fInnerRadius;
	 float fLightAngle = dot(vPrimaryLight, vVertexWS) / fInnerRadius;
	 float fCameraScale = scale(fCameraAngle);
	 float fLightScale = scale(fLightAngle);
	 float fCameraOffset = fDepth * fCameraScale;
	 float fTemp = (fLightScale + fCameraScale);
	 
	 float fSampleLength = fFar / fSamples;
	 float fScaledLength = fSampleLength * fScale;
	 
	 vec3 vInvWavelengthScaled = vInvWavelength * fKr4PI + fKm4PI;
	 vec3 vSampleRay = vRay * fSampleLength;
	 vec3 vSamplePoint = vStart;
	 
	 vec3 vFrontColor = vec3(0.0);
	 
	 for (int i = 0; i < nSamples; ++i)
	 {
		 float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - length(vSamplePoint)));
		 float fScatter = fDepth * fTemp - fCameraOffset;
		 vec3 vAttenuate = exp(-fScatter * vInvWavelengthScaled);
		 vFrontColor += vAttenuate * (fDepth * fScaledLength);
		 vSamplePoint += vSampleRay;
	 }
	 
	 aColor = fKrESun * vFrontColor * vInvWavelength + fKmESun * vFrontColor;
	 
	 gl_Position = mModelViewProjection * vec4(vVertexWS, 1.0);
 }
 );

const std::string planetPerVertexFS = ET_TO_CONST_CHAR
(
 PRECISION_STRING

 etFragmentIn vec3 aColor;
 
 void main (void)
 {
	 etFragmentOut = vec4(1.0 - exp(-aColor), 1.0);
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

const std::string planetPerPixelVS = ET_TO_CONST_CHAR
   (
	uniform mat4 mModelViewProjection;
	uniform float fInnerRadius;
	etVertexIn vec3 Vertex;
	etVertexOut vec3 vVertex;
	void main(void)
	{
		vVertex = fInnerRadius * Vertex;
		gl_Position = mModelViewProjection * vec4(vVertex, 1.0);
	}
);

const std::string atmospherePerPixelFS = ET_TO_CONST_CHAR
   (
	PRECISION_STRING

	uniform vec3 vCamera;
	uniform vec3 vPrimaryLight;
	uniform vec3 vInvWavelength;
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
		vec3 vRay = vVertex - vCamera;
		float fFar = length(vRay);
		vRay /= fFar;
		
/*
 * from space
 *
		{
			float B = 2.0 * dot(vCamera, vRay);
			float C = fCameraHeight * fCameraHeight - fOuterRadius2;
			float fDet = max(0.0, B * B - 4.0 * C);
			float fNear = -0.5 * (B + sqrt(fDet));
			fFar -= fNear;
			
			vec3 vStart = vCamera + vRay * fNear;
			float fStartAngle = dot(vRay, vStart) / fOuterRadius;
			float fStartOffset = exp(-1.0 / fScaleDepth) * scale(fStartAngle);
		}
 */
		
		float fStartAngle = dot(vRay, vCamera) / fCameraHeight;
		float fStartOffset = exp(fScaleOverScaleDepth * (fInnerRadius - fCameraHeight)) * scale(fStartAngle);
		float fSampleLength = fFar / fSamples;
		float fScaledLength = fSampleLength * fScale;
		vec3 vSampleRay = vRay * fSampleLength;
		vec3 vSamplePoint = vCamera;
		
		vec3 vFrontColor = vec3(0.0);
		for(int i = 0; i < nSamples; i++)
		{
			float fHeight = length(vSamplePoint);
			float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
			float fLightAngle = dot(vPrimaryLight, vSamplePoint) / fHeight;
			float fCameraAngle = dot(vRay, vSamplePoint) / fHeight;
			float fScatter = fStartOffset + fDepth * (scale(fLightAngle) - scale(fCameraAngle));
			vec3 vAttenuate = exp(-fScatter * (vInvWavelength * fKr4PI + fKm4PI));
			vFrontColor += vAttenuate * (fDepth * fScaledLength);
			vSamplePoint += vSampleRay;
		}
		
		float fCos = dot(vPrimaryLight, vRay);
		float fOnePlusCos2 = 1.0 + fCos * fCos;
		
		float fMiePhase = 1.5 * ((1.0 - g2) / (2.0 + g2)) * fOnePlusCos2 / pow(1.0 + g2 + 2.0 * g * fCos, 1.5);
		float fRayleighPhase = 0.75 * fOnePlusCos2;
		
		vec3 aColor = (fRayleighPhase * fKrESun) * (vFrontColor * vInvWavelength) +
			(fMiePhase * fKmESun) * vFrontColor;
		
		etFragmentOut = vec4(1.0 - exp(-aColor), 1.0);
	}
);

const std::string planetPerPixelFS = ET_TO_CONST_CHAR
(
 PRECISION_STRING
 
 uniform vec3 vCamera;
 uniform vec3 vPrimaryLight;
 uniform vec3 vInvWavelength;
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
 
 etFragmentIn vec3 vVertex;
 
 float scale(float fCos)
 {
	 float x = 1.0 - fCos;
	 return fScaleDepth * exp(x * (0.459 + x * (3.83 + x * (x * 5.25 - 6.80))) - 0.00287);
 }
 
 void main(void)
 {
	 vec3 vPos = vVertex;
	 vec3 vRay = vPos - vCamera;
	 float fFar = length(vRay);
	 vRay /= fFar;
	 
	 vec3 vStart = vCamera;
	 float fCameraHeight = length(vCamera);
	 float fDepth = exp((fInnerRadius - fCameraHeight) / fScaleDepth);
	 float fCameraAngle = -dot(vRay, vPos) / length(vPos);
	 float fLightAngle = dot(vPrimaryLight, vPos) / length(vPos);
	 float fCameraScale = scale(fCameraAngle);
	 float fLightScale = scale(fLightAngle);
	 float fCameraOffset = fDepth * fCameraScale;
	 float fTemp = (fLightScale + fCameraScale);
	 float fSampleLength = fFar / fSamples;
	 float fScaledLength = fSampleLength * fScale;
	 vec3 vSampleRay = vRay * fSampleLength;
	 vec3 vSamplePoint = vStart;
	 vec3 vAttenuate = vec3(0.0);
	 vec3 vFrontColor = vec3(0.0);
	 
	 for (int i = 0; i < nSamples; ++i)
	 {
		 float fHeight = length(vSamplePoint);
		 float fDepth = exp(fScaleOverScaleDepth * (fInnerRadius - fHeight));
		 float fScatter = fDepth * fTemp - fCameraOffset;
		 vAttenuate = exp(-fScatter * (vInvWavelength * fKr4PI + fKm4PI));
		 vFrontColor += vAttenuate * (fDepth * fScaledLength);
		 vSamplePoint += vSampleRay;
	 }
	 
	 vec3 aColor = (vFrontColor + 0.25 * vAttenuate) * (vInvWavelength * fKrESun + fKmESun);
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
