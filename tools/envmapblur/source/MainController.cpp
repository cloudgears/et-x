#include <et/primitives/primitives.h>
#include <et/imaging/imagewriter.h>
#include "maincontroller.h"

using namespace et;
using namespace emb;

extern const std::string fullscreenVertexShader;

extern const std::string cubemapFragmentShader;

extern const std::string gaussianBlurShader;

extern const std::string previewVertexShader;
extern const std::string previewFragmentShader;

const vec3 lightPosition = normalize(vec3(1.0f, 1.0f, 1.0f));

void MainController::setApplicationParameters(et::ApplicationParameters& p)
{
	p.windowStyle = WindowStyle_Caption | WindowStyle_Sizable;
	p.shouldSuspendOnDeactivate = false;
}

void MainController::setRenderContextParameters(et::RenderContextParameters& p)
{
	p.multisamplingQuality = MultisamplingQuality_None;
	p.supportedInterfaceOrientations = InterfaceOrientation_AnyLandscape;
	p.contextBaseSize = vec2i(1600, 900);
	p.contextSize = p.contextBaseSize;
	
	_gestures.pointerPressed.connect([this](PointerInputInfo p)
	{
		_ui->pointerPressed(p);
		_cameraAngles.cancelInterpolation();
	});
	
	_gestures.pointerReleased.connect([this](PointerInputInfo p) { _ui->pointerReleased(p); });
	_gestures.pointerMoved.connect([this](PointerInputInfo p) { _ui->pointerMoved(p); });
	_gestures.pointerCancelled.connect([this](PointerInputInfo p) { _ui->pointerCancelled(p); });
	
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
	
	vec2 scale(1.0f / static_cast<float>(horizontalSamples), 1.0f / static_cast<float>(verticalSamples));
	
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
		if (_loadedTexture.valid())
		{
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
		}
		else
		{
			_framebuffer.reset(nullptr);
		}
		_mainUi->setImages(_loadedTexture, _framebuffer->renderTarget(0));
	});
	_mainUi->processSelected.connect([this]()
	{
		_shouldProcessPass1 = true;
		_shouldProcessPass2 = false;
		_processingSample = 0;
	});
	_mainUi->saveSelected.connect([this](std::string f)
	{
		_fileToSave = f;
		_shouldSaveToFile = true;
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
	programs.gaussianBlur = _rc->programFactory().genProgram("gaussian-blur", fullscreenVertexShader, gaussianBlurShader);
	programs.gaussianBlur->setUniform("inputTexture", 0);

	programs.cubemap = _rc->programFactory().genProgram("cubemap", fullscreenVertexShader, cubemapFragmentShader);
	programs.cubemap->setUniform("colorTexture", 0);
	programs.cubemap->setUniform("vertexOffset", vec2(0.0f));
	programs.cubemap->setUniform("vertexScale", vec2(1.0f));

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

	if (_shouldSaveToFile && _framebuffer.valid())
	{
		Camera cubemapCamera;
		cubemapCamera.perspectiveProjection(HALF_PI, 1.0f, 1.0f, 100.0f);
		cubemapCamera.setModelViewMatrix(identityMatrix);
		auto cm = cubemapMatrixProjectionArray(cubemapCamera.modelViewProjectionMatrix(), vec3(0.0f));

		vec2i textureSize = _framebuffer->renderTarget(0)->size() / 2;
		textureSize = vec2i(static_cast<int>(roundToHighestPowerOfTwo(etMin(textureSize.x, textureSize.y)) / 2));

		size_t mipLevel = 0;
		while (textureSize.x >= 4)
		{
			auto fb = rc->framebufferFactory().createCubemapFramebuffer(textureSize.x, "cb", GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 0);
			rc->renderState().bindFramebuffer(fb);
			rc->renderState().bindProgram(programs.cubemap);
			rc->renderState().bindTexture(0, _framebuffer->renderTarget(0));
			programs.cubemap->setUniform("exposure", _mainUi->exposureValue());

			BinaryDataStorage data(4 * textureSize.square(), 0);
			for (uint32_t i = 0; i < 6; ++i)
			{
				fb->setCurrentCubemapFace(i);
				rc->renderState().setClearColor(vec4(0.0f));
				rc->renderer()->clear(true, false);

				programs.cubemap->setMVPMatrix(cm[i].inverse());
				rc->renderer()->fullscreenPass();

				glReadPixels(0, 0, textureSize.x, textureSize.y, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

				ImageWriter::writeImageToFile(_fileToSave + "~level-" + intToStr(mipLevel) + "~face-" + intToStr(i) + ".png",
					data, textureSize, 4, 8, ImageFormat_PNG, false);
			}

			textureSize /= 2;
			mipLevel++;
		}
		rc->renderState().bindDefaultFramebuffer();
		rc->renderState().setClearColor(vec4(0.5f));
		_shouldSaveToFile = false;
	}
	
	_ui->render(rc);
}

void MainController::renderPreview()
{
	auto& rs = _rc->renderState();
	
	rs.setBlend(false);
	rs.setDepthTest(true);
	rs.setCulling(true, CullState_Back);
	rs.bindVertexArray(_vaoSphere);
	rs.bindTexture(0, _framebuffer.valid() ? _framebuffer->renderTarget(0) : Texture());
	rs.bindProgram(programs.preview);
	programs.preview->setPrimaryLightPosition(lightPosition);
	programs.preview->setCameraProperties(_camera);
	programs.preview->setUniform("exposure", _mainUi->exposureValue());
	_rc->renderer()->drawAllElements(_vaoSphere->indexBuffer());
}

void MainController::processImage_Pass1()
{
	if (_framebuffer.invalid()) return;
	
	auto& rs = _rc->renderState();
	const auto& p = _offsetAndScales.at(_processingSample);
	
	rs.bindFramebuffer(_framebuffer);
	rs.bindProgram(programs.gaussianBlur);
	
	programs.gaussianBlur->setUniform("radius", _mainUi->angleValue());
	programs.gaussianBlur->setUniform("vertexOffset", p.first);
	programs.gaussianBlur->setUniform("vertexScale", p.second);
	
	_framebuffer->setCurrentRenderTarget(1);
	rs.bindTexture(0, _loadedTexture);
	programs.gaussianBlur->setUniform("texel", _loadedTexture->texel() * vec2(1.0f, 0.0f));
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
	rs.bindProgram(programs.gaussianBlur);
	programs.gaussianBlur->setUniform("radius", _mainUi->angleValue());
	programs.gaussianBlur->setUniform("vertexOffset", p.first);
	programs.gaussianBlur->setUniform("vertexScale", p.second);
	
	_framebuffer->setCurrentRenderTarget(0);
	rs.bindTexture(0, _framebuffer->renderTarget(1));
	programs.gaussianBlur->setUniform("texel", _framebuffer->renderTarget(0)->texel() * vec2(0.0f, 1.0f));
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

const std::string cubemapFragmentShader = ET_TO_CONST_CHAR
(
	uniform sampler2D colorTexture;
	uniform mat4 mModelViewProjection;
	uniform float exposure;
	etFragmentIn vec2 vertexNormalized;
	void main()
	{
		vec3 normal = normalize((mModelViewProjection * vec4(2.0 * vertexNormalized - 1.0, 1.0, 1.0)).xyz);
	 
		etFragmentOut = 1.0 - exp(-exposure * etTexture2D(colorTexture,
			0.5 + vec2(-0.5 * atan(normal.z, normal.x), asin(normal.y)) / PI));
	}
);

const std::string gaussianBlurShader = ET_TO_CONST_CHAR
(
 uniform sampler2D inputTexture;
 uniform float radius;
 uniform vec2 texel;
 
 etFragmentIn vec2 vertexNormalized;
 
 float gauss(float x, float sigma)
 {
	 float sigma_sqr = sigma * sigma;
	 return exp(-0.5 * x * x / sigma_sqr) / sqrt(DOUBLE_PI * sigma_sqr);
 }
 
 vec2 texCoordToAngles(in vec2 texCoord)
	 { return PI * vec2(2.0 * texCoord.x - 1.0, texCoord.y - 0.5); }
 
 vec2 anglesToTexCoord(in vec2 angles)
	 { return vec2(angles.x / DOUBLE_PI + 0.5, angles.y / PI + 0.5); }
 
 vec3 anglesToVector(in vec2 angles)
	 { return normalize(vec3(cos(angles.y) * cos(angles.x), sin(angles.y), cos(angles.y) * sin(angles.x))); }
 
 vec2 vectorToAngles(in vec3 vector)
 {
	 vec3 n = normalize(vector);
	 return vec2(atan(n.z, n.x), asin(n.y));
 }
 
 void main()
 {
	 vec2 baseAngles = texCoordToAngles(vertexNormalized);
	 
	 vec3 baseVector = anglesToVector(baseAngles);
	 vec3 forwardVector = anglesToVector(baseAngles + radius * texel);
	 vec3 backwardVector = anglesToVector(baseAngles - radius * texel);
	 
	 vec3 forwardAxis = normalize(cross(baseVector, forwardVector));
	 vec3 backwardAxis = normalize(cross(baseVector, backwardVector));
	 
	 float x = forwardAxis.x;
	 float y = forwardAxis.y;
	 float z = forwardAxis.z;
	 
	 float deltaAngle = dot(texel, vec2(PI, HALF_PI));
	 
	 float cosTheta = cos(deltaAngle);
	 float sinTheta = sin(deltaAngle);
	 float invCosTheta = 1.0 - cosTheta;
	 
	 mat3 forwardMatrix;
	 forwardMatrix[0][0] = cosTheta + invCosTheta * x * x;
	 forwardMatrix[0][1] = invCosTheta * y * x - sinTheta * z;
	 forwardMatrix[0][2] = invCosTheta * x * z + sinTheta * y;
	 forwardMatrix[1][0] = invCosTheta * y * x + sinTheta * z;
	 forwardMatrix[1][1] = cosTheta + invCosTheta * y * y;
	 forwardMatrix[1][2] = invCosTheta * y * z - sinTheta * x;
	 forwardMatrix[2][0] = invCosTheta * x * z - sinTheta * y;
	 forwardMatrix[2][1] = invCosTheta * z * y + sinTheta * x;
	 forwardMatrix[2][2] = cosTheta + invCosTheta * z * z;
	 
	 x = backwardAxis.x;
	 y = backwardAxis.y;
	 z = backwardAxis.z;
	 
	 mat3 backwardMatrix;
	 backwardMatrix[0][0] = cosTheta + invCosTheta * x * x;
	 backwardMatrix[0][1] = invCosTheta * y * x - sinTheta * z;
	 backwardMatrix[0][2] = invCosTheta * x * z + sinTheta * y;
	 backwardMatrix[1][0] = invCosTheta * y * x + sinTheta * z;
	 backwardMatrix[1][1] = cosTheta + invCosTheta * y * y;
	 backwardMatrix[1][2] = invCosTheta * y * z - sinTheta * x;
	 backwardMatrix[2][0] = invCosTheta * x * z - sinTheta * y;
	 backwardMatrix[2][1] = invCosTheta * z * y + sinTheta * x;
	 backwardMatrix[2][2] = cosTheta + invCosTheta * z * z;
	 
	 float sigma = radius / 3.333333;
	 float gain = gauss(0.0, sigma);
	 
	 vec4 color = gain * etTexture2D(inputTexture, vertexNormalized);
	 
	 forwardVector = baseVector;
	 backwardVector = baseVector;
	 
	 float angle = deltaAngle;
	 while (angle <= radius)
	 {
		 float gaussianScale = gauss(angle, sigma);
		 
		 forwardVector = normalize(forwardMatrix * forwardVector);
		 backwardVector = normalize(backwardMatrix * backwardVector);
		 
		 float scale = gaussianScale * dot(forwardVector, baseVector);
		 color += scale * etTexture2D(inputTexture, anglesToTexCoord(vectorToAngles(forwardVector)));
		 gain += scale;
		 
		 scale = gaussianScale * dot(backwardVector, baseVector);
		 color += scale * etTexture2D(inputTexture, anglesToTexCoord(vectorToAngles(backwardVector)));
		 gain += scale;
		 
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
 uniform float exposure;
 
 etFragmentIn vec3 vNormalWS;
 
 void main()
 {
	 vec3 normal = normalize(vNormalWS);
	 
	 etFragmentOut = 1.0 - exp(-exposure * etTexture2D(colorTexture,
		0.5 + vec2(-0.5 * atan(normal.z, normal.x), asin(normal.y)) / PI));
 }
 );


