#include <et/primitives/primitives.h>
#include "maincontroller.h"

using namespace et;
using namespace emb;

extern const std::string fullscreenVertexShader;

extern const std::string gaussianBlurLatShader;
extern const std::string gaussianBlurLonShader;

extern const std::string previewVertexShader;
extern const std::string previewFragmentShader;

const vec3 lightPosition = normalize(vec3(1.0f, 1.0f, 1.0f));

void MainController::setApplicationParameters(et::ApplicationParameters& p)
{
	p.windowStyle = WindowStyle_Caption | WindowStyle_Sizable;
}

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.supportedInterfaceOrientations = InterfaceOrientation_AnyLandscape;
	p.contextBaseSize = vec2i(1600, 900);
	p.contextSize = p.contextBaseSize;
	
	_gestures.pointerPressed.connect([this](PointerInputInfo p)
									 {
										 _ui->pointerPressed(p);
										 _cameraAngles.cancelInterpolation();
									 });
	_gestures.pointerReleased.connect([this](PointerInputInfo p)
									  {
										  _ui->pointerReleased(p);
									  });
	_gestures.pointerMoved.connect([this](PointerInputInfo p)
								   {
									   _ui->pointerMoved(p);
								   });
	_gestures.pointerCancelled.connect([this](PointerInputInfo p)
									   {
										   _ui->pointerCancelled(p);
									   });
	
	_gestures.drag.connect(this, &MainController::onDrag);
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	_rc = rc;
	_rc->renderState().setClearColor(vec4(0.5f));
	
	loadPrograms();
	loadGeometry();
	
	size_t verticalSamples = 12;
	size_t horizontalSamples = 12;
	size_t numSamples = horizontalSamples * verticalSamples;
	
	_offsetAndScales.reserve(numSamples);
	
	vec2 scale(1.0 / static_cast<float>(horizontalSamples), 1.0 / static_cast<float>(verticalSamples));
	
	vec2 realOffset;
	realOffset.y = -scale.y * static_cast<float>(verticalSamples - 1);
	for (size_t y = 0; y < verticalSamples; ++y)
	{
		realOffset.x = -scale.x * static_cast<float>(horizontalSamples - 1);
		for (size_t x = 0; x < horizontalSamples; ++x)
		{
			_offsetAndScales.push_back(std::make_pair(realOffset, scale));
			realOffset.x += 2.0f * scale.x;
		}
		
		realOffset.y += 2.0f * scale.y;
	}
	
	for (const auto& p : _offsetAndScales)
	{
		log::info("(%f %f)", p.first.x, p.first.y);
	}
	
	std::string fileToLoad;
	size_t numParams = application().launchParamtersCount();
	for (size_t i = 0; i < numParams - 1; ++i)
	{
		const auto& param = application().launchParameter(i);
		if (param == "--file")
		{
			fileToLoad = application().launchParameter(i+1);
			break;
		}
	}
	
	_rm.load(rc);
	_mainUi = MainUI::Pointer::create(_rm);
	_mainUi->fileSelected.connect([this](std::string name)
	{
		ObjectsCache localCache;
		
		_loadedTexture = _rc->textureFactory().loadTexture(name, localCache);
		_loadedTexture->setWrap(_rc, TextureWrap_Repeat, TextureWrap_ClampToEdge);
		
		_framebuffer = _rc->framebufferFactory().createFramebuffer(_loadedTexture->size(), "framebuffer",
			GL_RGBA32F, GL_RGBA, GL_FLOAT, 0, 0, 0);
		_framebuffer->addSameRendertarget();
		
		_framebuffer->renderTarget(0)->setWrap(_rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
		_framebuffer->renderTarget(1)->setWrap(_rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
		
		_rc->renderState().bindFramebuffer(_framebuffer);
		_framebuffer->setCurrentRenderTarget(0);
		_rc->renderer()->renderFullscreenTexture(_loadedTexture);
		_rc->renderState().bindDefaultFramebuffer();
		
		_mainUi->setImages(_loadedTexture, _framebuffer->renderTarget(0));
	});
	_mainUi->processSelected.connect([this]()
	{
		_shouldProcessPass1 = true;
		_shouldProcessPass2 = false;
		_processingSample = 0;
	});
	
	if (fileExists(fileToLoad))
		_mainUi->fileSelected.invokeInMainRunLoop(fileToLoad);
	
	_ui = s2d::Scene::Pointer::create(rc);
	_ui->pushLayout(_mainUi);
	
	_cameraAngles.setTargetValue(vec2(0.0f, 0.0f));
	_cameraAngles.finishInterpolation();
	_cameraAngles.updated.connect(this, &MainController::updateCamera);
	_cameraAngles.run();
	
	updateCamera();
}

void MainController::updateCamera()
{
	_camera.lookAt(5.0f * fromSpherical(_cameraAngles.value().y, _cameraAngles.value().x));
};

void MainController::onDrag(et::vec2 v, et::PointerType)
{
	const float maxTheta = HALF_PI - 10.0f * DEG_1;
	const vec2 anglesScale(0.125f, -0.25f);
	
	vec2 t = _cameraAngles.targetValue() + v * anglesScale;
	
	if (std::abs(t.y) >= maxTheta)
		t.y = maxTheta * signOrZero(t.y);
	
	_cameraAngles.setTargetValue(t);
}

void MainController::loadPrograms()
{
	programs.gaussianBlurLat = _rc->programFactory().genProgram("gaussian-blur-lat", fullscreenVertexShader, gaussianBlurLatShader);
	programs.gaussianBlurLat->setUniform("inputTexture", 0);
	programs.gaussianBlurLon = _rc->programFactory().genProgram("gaussian-blur-lon", fullscreenVertexShader, gaussianBlurLonShader);
	programs.gaussianBlurLon->setUniform("inputTexture", 0);
	programs.preview = _rc->programFactory().genProgram("preview", previewVertexShader, previewFragmentShader);
	programs.preview->setUniform("colorTexture", 0);
}

void MainController::loadGeometry()
{
	VertexDeclaration decl(false, Usage_Position, Type_Vec3);
	decl.push_back(Usage_Normal, Type_Vec3);
	
	vec2i gridDensity(100);
	size_t numIndexes = primitives::indexCountForRegularMesh(gridDensity, PrimitiveType_TriangleStrips);
	
	auto va = VertexArray::Pointer::create(decl, 0);
	auto ia = IndexArray::Pointer::create(IndexArrayFormat_32bit, numIndexes, PrimitiveType_TriangleStrips);
	
	primitives::createSphere(va, 1.0f, gridDensity);
	primitives::buildTriangleStripIndexes(ia, gridDensity, 0, 0);
	
	_vaoSphere = _rc->vertexBufferFactory().createVertexArrayObject("sphere", va, BufferDrawType_Static,
		ia, BufferDrawType_Static);
}

void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	vec2 fSize = vector2ToFloat(sz);
	_ui->layout(fSize);
	_camera.perspectiveProjection(QUARTER_PI, fSize.aspect(), 1.0f, 100.0f);
}

void MainController::render(et::RenderContext* rc)
{
	if (_shouldProcessPass1)
		processImage_Pass1();
	else if (_shouldProcessPass2)
		processImage_Pass2();
	
	rc->renderState().bindDefaultFramebuffer();
	rc->renderState().setDepthMask(true);
	rc->renderer()->clear(true, true);
	renderPreview();
	
	_ui->render(rc);
}

void MainController::renderPreview()
{
	auto& rs = _rc->renderState();
	
	rs.setBlend(false);
	rs.setDepthTest(true);
	rs.setCulling(true, CullState_Back);
	rs.bindVertexArray(_vaoSphere);
	rs.bindTexture(0, _framebuffer->renderTarget(0));
	rs.bindProgram(programs.preview);
	programs.preview->setPrimaryLightPosition(lightPosition);
	programs.preview->setCameraProperties(_camera);
	programs.preview->setUniform("exposure", _mainUi->exposureValue());
	_rc->renderer()->drawAllElements(_vaoSphere->indexBuffer());
}

void MainController::processImage_Pass1()
{
	auto& rs = _rc->renderState();
	const auto& p = _offsetAndScales.at(_processingSample);
	
	rs.bindFramebuffer(_framebuffer);
	rs.bindProgram(programs.gaussianBlurLat);
	programs.gaussianBlurLat->setUniform("radius", _mainUi->angleValue());
	programs.gaussianBlurLat->setUniform("vertexOffset", p.first);
	programs.gaussianBlurLat->setUniform("vertexScale", p.second);
	
	_framebuffer->setCurrentRenderTarget(1);
	rs.bindTexture(0, _loadedTexture);
	programs.gaussianBlurLat->setUniform("texel", _loadedTexture->texel() * vec2(1.0f, 0.0f));
	_rc->renderer()->fullscreenPass();

	_framebuffer->setCurrentRenderTarget(0);
	_rc->renderer()->renderFullscreenTexture(_framebuffer->renderTarget(1));
	
	_processingSample++;
	if (_processingSample >= _offsetAndScales.size())
	{
		_shouldProcessPass1 = false;
		_shouldProcessPass2 = true;
		_processingSample = 0;
	}
}

void MainController::processImage_Pass2()
{
	auto& rs = _rc->renderState();
	const auto& p = _offsetAndScales.at(_processingSample);
	
	rs.bindFramebuffer(_framebuffer);
	rs.bindProgram(programs.gaussianBlurLat);
	programs.gaussianBlurLat->setUniform("radius", _mainUi->angleValue());
	programs.gaussianBlurLat->setUniform("vertexOffset", p.first);
	programs.gaussianBlurLat->setUniform("vertexScale", p.second);
	
	_framebuffer->setCurrentRenderTarget(0);
	rs.bindTexture(0, _framebuffer->renderTarget(1));
	programs.gaussianBlurLat->setUniform("texel", _framebuffer->renderTarget(0)->texel() * vec2(0.0f, 1.0f));
	_rc->renderer()->fullscreenPass();
	
	_processingSample++;
	if (_processingSample >= _offsetAndScales.size())
	{
		_shouldProcessPass1 = false;
		_shouldProcessPass2 = false;
		_processingSample = 0;
	}
}

ApplicationIdentifier MainController::applicationIdentifier() const
{ return ApplicationIdentifier(applicationIdentifierForCurrentProject(), "Cheetek", "EnvMapBlur"); }

IApplicationDelegate* Application::initApplicationDelegate()
{ return new MainController; }

const std::string fullscreenVertexShader = ET_TO_CONST_CHAR
(
 uniform vec2 vertexScale;
 uniform vec2 vertexOffset;
 etVertexIn vec2 Vertex;
 etVertexOut vec2 vertexNormalized;
 void main()
 {
	 vec2 adjustedVertex = vertexOffset + vertexScale * Vertex;
	 
	 vertexNormalized = 0.5 + 0.5 * adjustedVertex;
	 gl_Position = vec4(adjustedVertex, 0.0, 1.0);
 }
 );

const std::string gaussianBlurLonShader = ET_TO_CONST_CHAR
(
 void main()
 {
	 etFragmentOut = vec4(1.0);
 }
);

const std::string gaussianBlurLatShader = ET_TO_CONST_CHAR
(
 uniform sampler2D inputTexture;
 uniform int intRadius;
 uniform float radius;
 uniform vec2 texel;
 
 etFragmentIn vec2 vertexNormalized;
 
 const float halfPi = 1.5707963268;
 const float pi = 3.1415926536;
 const float doublePi = 6.2831853072;
 
 float gauss(float x, float sigma)
 {
	 float x_sqr = x * x;
	 float sigma_sqr = sigma * sigma;
	 float sqrt_value = 1.0 / sqrt(doublePi * sigma_sqr);
	 float exp_value = exp(-x_sqr / (2.0 * sigma_sqr));
	 return sqrt_value * exp_value;
 }
 
 vec2 texCoordToAngles(in vec2 texCoord)
 {
	 vec2 result = vec2(0.0);
	 result.x = -pi + doublePi * texCoord.x;
	 result.y = -halfPi + pi * texCoord.y;
	 return result;
 }
 
 vec2 anglesToTexCoord(in vec2 angles)
 {
	 vec2 result = vec2(0.0);
	 result.x = (angles.x + pi) / doublePi;
	 result.y = (angles.y + halfPi) / pi;
	 return result;
 }
 
 vec3 anglesToVector(in vec2 angles)
 {
	 return normalize(vec3(cos(angles.y) * cos(angles.x), sin(angles.y), cos(angles.y) * sin(angles.x)));
 }
 
 vec2 vectorToAngles(in vec3 vector)
 {
	 vec3 n = normalize(vector);
	 return vec2(atan(n.z, n.x), asin(n.y));
 }
 
 vec3 slerp(in vec3 p0, in vec3 p1, float t)
 {
	 float cosTheta = dot(p0, p1);
	 
	 if (abs(cosTheta) > 0.999)
		 return mix(p0, p1, t);
	 
	 float theta = 1.0 - cosTheta * cosTheta;
	 return (p0 * sin(theta * (1.0 - t)) + p1 * sin(theta * t)) / sin(theta);
 }
 
 void main()
 {
	 vec2 baseAngles = texCoordToAngles(vertexNormalized);
	 vec3 baseVector = anglesToVector(baseAngles);
	 
	 vec3 forwardVector = anglesToVector(baseAngles + radius * texel);
	 vec2 forwardAngles = vectorToAngles(forwardVector);
	 
	 vec3 backwardVector = anglesToVector(baseAngles - radius * texel);
	 vec2 backwardAngles = vectorToAngles(backwardVector);
	 
	 vec3 forwardAxis = normalize(cross(baseVector, forwardVector));
	 vec3 backwardAxis = normalize(cross(baseVector, backwardVector));
	 
	 vec4 color = vec4(0.0);
	 float gain = 0.0;

	 float angle = 0.0;
	 float deltaAngle = 0.1 * dot(texel, vec2(pi, halfPi));
	 float sigma = radius / 3.333333;
	 float sum = 0.0;
	 while (angle <= radius)
	 {
		 float gaussianScale = gauss(angle, sigma);
		 sum += gaussianScale;
		 
		 float cosTheta = cos(angle);
		 float sinTheta = sin(angle);
		 float invCosTheta = 1.0 - cosTheta;
		 
		 mat3 matrix;
		 
		 float x = forwardAxis.x;
		 float y = forwardAxis.y;
		 float z = forwardAxis.z;
		 
		 matrix[0][0] = cosTheta + invCosTheta * x * x;
		 matrix[0][1] = invCosTheta * y * x - sinTheta * z;
		 matrix[0][2] = invCosTheta * x * z + sinTheta * y;
		 matrix[1][0] = invCosTheta * y * x + sinTheta * z;
		 matrix[1][1] = cosTheta + invCosTheta * y * y;
		 matrix[1][2] = invCosTheta * y * z - sinTheta * x;
		 matrix[2][0] = invCosTheta * x * z - sinTheta * y;
		 matrix[2][1] = invCosTheta * z * y + sinTheta * x;
		 matrix[2][2] = cosTheta + invCosTheta * z * z;
		 
		 vec3 rotatedVector = normalize(matrix * baseVector);
		 float scale = gaussianScale * dot(rotatedVector, baseVector);
		 gain += scale;
		 color += scale * etTexture2D(inputTexture, anglesToTexCoord(vectorToAngles(rotatedVector)));

		 x = backwardAxis.x;
		 y = backwardAxis.y;
		 z = backwardAxis.z;
		 
		 matrix[0][0] = cosTheta + invCosTheta * x * x;
		 matrix[0][1] = invCosTheta * y * x - sinTheta * z;
		 matrix[0][2] = invCosTheta * x * z + sinTheta * y;
		 matrix[1][0] = invCosTheta * y * x + sinTheta * z;
		 matrix[1][1] = cosTheta + invCosTheta * y * y;
		 matrix[1][2] = invCosTheta * y * z - sinTheta * x;
		 matrix[2][0] = invCosTheta * x * z - sinTheta * y;
		 matrix[2][1] = invCosTheta * z * y + sinTheta * x;
		 matrix[2][2] = cosTheta + invCosTheta * z * z;
		 
		 rotatedVector = normalize(matrix * baseVector);
		 scale = gaussianScale * dot(rotatedVector, baseVector);
		 gain += scale;
		 color += scale * etTexture2D(inputTexture, anglesToTexCoord(vectorToAngles(rotatedVector)));
		 
		 angle += deltaAngle;
	 }
	 	 
	 etFragmentOut = color / gain;
	 etFragmentOut.w = 1.0;
 }
 );

const std::string previewVertexShader = ET_TO_CONST_CHAR
(
 uniform mat4 mModelViewProjection;
 etVertexIn vec3 Vertex;
 etVertexIn vec3 Normal;
 etVertexOut vec3 vNormalWS;
 void main()
 {
	 vNormalWS = Normal;
 	 gl_Position = mModelViewProjection * vec4(Vertex, 1.0);
	 gl_Position.x += 0.5 * gl_Position.w;
 }
 );

const std::string previewFragmentShader = ET_TO_CONST_CHAR
(
 uniform sampler2D colorTexture;
 uniform vec3 vPrimaryLight;
 uniform float exposure;
 etFragmentIn vec3 vNormalWS;
 
 const float pi = 3.1415926536;
 
 void main()
 {
	 vec3 normal = normalize(vNormalWS);
	 
	 float x = 1.0 - (atan(normal.z, normal.x) + pi) / (2.0 * pi);
	 float y = asin(normal.y) / pi + 0.5;
	 
	 etFragmentOut = 1.0 - exp(-exposure * etTexture2D(colorTexture, vec2(x, y)));
 }
 );


