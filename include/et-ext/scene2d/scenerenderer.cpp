/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/scenerenderer.h>

namespace et {

extern const char* etExtWorkingFolder;

namespace s2d {

SceneRenderer::SceneRenderer(RenderContext* rc) :
	_rc(rc), _additionalOffsetAndAlpha(0.0f, 0.0f, 1.0f)
{
	_clip.emplace(); // default clip, which would be replaced with correct later

	TextureDescription::Pointer desc = TextureDescription::Pointer::create();
	desc->size = vec2i(1);
	desc->format = TextureFormat::RGBA8;
	
	desc->data = BinaryDataStorage(4, 255);
	_whiteTexture = rc->renderer()->createTexture(desc);
	
	desc->data.fill(0);
	_transparentTexture = rc->renderer()->createTexture(desc);

	application().pushSearchPath(etExtWorkingFolder);
	std::string scene2dMaterial = application().resolveFileName("engine_ext_data/materials/scene2d.json");
	ET_ASSERT(fileExists(scene2dMaterial));
	std::string fontMaterial = application().resolveFileName("engine_ext_data/materials/font.json");
	ET_ASSERT(fileExists(fontMaterial));
	application().popSearchPaths();

	_defaultMaterial = Material::Pointer::create(rc->renderer().pointer());
	_defaultMaterial->loadFromJson(loadTextFile(scene2dMaterial), getFilePath(scene2dMaterial));
	_defaultMaterial->setTexture(MaterialTexture::Albedo, _whiteTexture);

	_fontMaterial = Material::Pointer::create(rc->renderer().pointer());
	_fontMaterial->loadFromJson(loadTextFile(fontMaterial), getFilePath(fontMaterial));

	RenderPass::ConstructionInfo passInfo;
	passInfo.color[0].enabled = true;
	passInfo.color[0].loadOperation = FramebufferOperation::Load;
	passInfo.depth.enabled = true;
	passInfo.depth.loadOperation = FramebufferOperation::Clear;
	passInfo.camera = _sceneCamera;
	_renderPass = rc->renderer()->allocateRenderPass(passInfo);

	setProjectionMatrices(vector2ToFloat(rc->size()));
}

SceneRenderer::~SceneRenderer()
{
	_defaultMaterial->releaseInstances();
	_fontMaterial->releaseInstances();
}

void s2d::SceneRenderer::resetClipRect()
{
	while (_clip.size() > 1)
		_clip.pop();
}

void s2d::SceneRenderer::pushClipRect(const recti& value)
{
	_clip.push(value);
}

void s2d::SceneRenderer::popClipRect()
{
	ET_ASSERT(_clip.size() > 1);
	_clip.pop();
}

void s2d::SceneRenderer::setProjectionMatrices(const vec2& contextSize)
{
	std::stack<recti> tempClipStack;
	
	while (_clip.size() > 1)
	{
		tempClipStack.push(_clip.top());
		_clip.pop();
	}
	
	_clip.pop();
	_clip.push(recti(vec2i(0), vec2i(static_cast<int>(contextSize.x), static_cast<int>(contextSize.y))));
	
	while (tempClipStack.size())
	{
		_clip.push(tempClipStack.top());
		tempClipStack.pop();
	}
	
	mat4 transform = identityMatrix;
	transform[0][0] = 2.0f / contextSize.x;
	transform[1][1] = -Camera::renderingOriginTransform * 2.0f / contextSize.y;
	transform[3][0] = -1.0f;
	transform[3][1] = Camera::renderingOriginTransform;
	transform[3][3] = 1.0f;
	_sceneCamera->setProjectionMatrix(transform);
}

SceneVertex* s2d::SceneRenderer::allocateVertices(uint32_t count, const Texture::Pointer,
	const MaterialInstance::Pointer& inMaterial, Element2d* object, PrimitiveType pt)
{
	ET_ASSERT(inMaterial.valid());
	ET_ASSERT(_renderingElement.valid());

	if (object && !object->hasFlag(Flag_DynamicRendering))
		object = nullptr;

	bool isDynamicObject = (object != nullptr) && (object->hasFlag(Flag_DynamicRendering));
	bool shouldAdd = isDynamicObject || _renderingElement->chunks.empty();
	if ((shouldAdd == false) && _renderingElement->chunks.size())
	{
		RenderChunk& lastChunk = _renderingElement->chunks.back();
		
		bool sameConfiguration = (lastChunk.clip == _clip.top()) &&
			(lastChunk.material == inMaterial) && (lastChunk.primitiveType == pt);
		
		if (sameConfiguration)
			lastChunk.count += count;
		else
			shouldAdd = true;
	}

	if (shouldAdd)
	{
		_renderingElement->chunks.emplace_back(_renderingElement->allocatedVertices, count, _clip.top(), inMaterial, object, pt);
	}
	
	return _renderingElement->allocateVertices(count);
}

void SceneRenderer::addVertices(const SceneVertexList& vertices, const Texture::Pointer texture,
	const MaterialInstance::Pointer& material, Element2d* owner, PrimitiveType pt)
{
	uint32_t count = vertices.lastElementIndex();
	ET_ASSERT((count > 0) && _renderingElement.valid() && material.valid());
	
	SceneVertex* target = allocateVertices(count, texture, material, owner, pt);
	for (const SceneVertex& v : vertices)
		*target++ = v;
}

void s2d::SceneRenderer::setRenderingElement(const RenderingElement::Pointer& r)
{
	_renderingElement = r;
}

void s2d::SceneRenderer::beginRender(RenderContext* rc)
{
	/*
	 * TODO : beginrender
	 *
	auto& rs = rc->renderState();
	_lastBlendState = rs.actualState().blend;
	_lastDepthState = rs.actualState().depth;
	_lastRasterizerState = rs.actualState().rasterizer;
	rc->renderState().setDepthState({false, false});
	rc->renderState().setBlendConfiguration(BlendConfiguration::AlphaBlend);
	// */
	_renderPass->begin();
}

void s2d::SceneRenderer::render(RenderContext* rc)
{
	ET_ASSERT(_renderingElement.valid());

	VertexStream::Pointer vs = _renderingElement->vertexStream();
	for (const RenderChunk& chunk : _renderingElement->chunks)
	{
		// TODO : use _additionalOffsetAndAlpha
		_renderPass->pushRenderBatch(RenderBatch::Pointer::create(chunk.material, vs, identityMatrix, chunk.first, chunk.count));
	}

	/*
	 * TODO : render something
	 *
	RenderState& rs = rc->renderState();
	Renderer* renderer = rc->renderer();
	const IndexBuffer::Pointer& indexBuffer = _renderingElement->VertexStream()->indexBuffer();
	
	Program::Pointer lastBoundProgram;
	for (auto& i : _renderingElement->chunks)
	{
		if (lastBoundProgram != i.program.program)
		{
			lastBoundProgram = i.program.program;
			rs.bindProgram(lastBoundProgram);
			lastBoundProgram->setUniform(i.program.additionalOffsetAndAlpha, _additionalOffsetAndAlpha);
			lastBoundProgram->setTransformMatrix(transform);
		}
		
		if (i.object != nullptr)
			i.object->setProgramParameters(rc, lastBoundProgram);
		
		rs.bindTexture(0, i.texture);
		rs.setScissor(true, i.clip + _additionalWindowOffset);
		
		renderer->drawElements(i.primitiveType, indexBuffer, i.first, i.count);
	}
	*/
}

void SceneRenderer::endRender(RenderContext* rc)
{
	_renderPass->end();
	rc->renderer()->submitRenderPass(_renderPass);
}

void SceneRenderer::setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha)
{
	_additionalOffsetAndAlpha = vec3(2.0f * offsetAndAlpha.xy(), offsetAndAlpha.z);
	_additionalWindowOffset.left = static_cast<int>(offsetAndAlpha.x * _rc->size().x);
	_additionalWindowOffset.top = static_cast<int>(offsetAndAlpha.y * _rc->size().y);
}

/*
 *
std::string et_scene2d_default_shader_vs =
	"uniform mat4 matWorld;"
	"uniform vec3 " + additionalOffsetAndAlphaUniform + ";"
	"etVertexIn vec3 Vertex;"
	"etVertexIn vec4 TexCoord0;"
	"etVertexIn vec4 Color;"
	"etVertexOut etHighp vec2 texCoord;"
	"etVertexOut etLowp vec4 tintColor;"
	"etVertexOut etLowp vec4 additiveColor;"
	"void main()"
	"{"
		"texCoord = TexCoord0.xy;"
		"vec4 alphaScaledColor = Color * vec4(1.0, 1.0, 1.0, additionalOffsetAndAlpha.z);"
		"additiveColor = alphaScaledColor * TexCoord0.w;"
		"tintColor = alphaScaledColor * (1.0 - TexCoord0.w);"
		"vec4 vTransformed = matWorld * vec4(Vertex, 1.0);"
		"gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
	"}";

std::string et_scene2d_default_shader_vs_with_screen_pos =
	"uniform mat4 matWorld;"
	"uniform vec3 " + additionalOffsetAndAlphaUniform + ";"
	"etVertexIn vec3 Vertex;"
	"etVertexIn vec4 TexCoord0;"
	"etVertexIn vec4 Color;"
	"etVertexOut etHighp vec2 texCoord;"
	"etVertexOut etHighp vec2 screenSpaceTexCoord;"
	"etVertexOut etLowp vec4 tintColor;"
	"etVertexOut etLowp vec4 additiveColor;"
	"void main()"
	"{"
		"texCoord = TexCoord0.xy;"
		"vec4 alphaScaledColor = Color * vec4(1.0, 1.0, 1.0, additionalOffsetAndAlpha.z);"
		"additiveColor = alphaScaledColor * TexCoord0.w;"
		"tintColor = alphaScaledColor * (1.0 - TexCoord0.w);"
		"vec4 vTransformed = matWorld * vec4(Vertex, 1.0);"
		"gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
		"screenSpaceTexCoord = 0.5 + 0.5 * gl_Position.xy / gl_Position.w;"
	"}";

std::string et_scene2d_default_shader_fs =
	"uniform etLowp sampler2D " + textureSamplerName + ";"
	"etFragmentIn etHighp vec2 texCoord;"
	"etFragmentIn etLowp vec4 tintColor;"
	"etFragmentIn etLowp vec4 additiveColor;"
	"void main()"
	"{"
	"	etFragmentOut = etTexture2D(inputTexture, texCoord) * tintColor + additiveColor;"
	"}";

std::string et_scene2d_default_text_shader_fs =
	"uniform etLowp sampler2D " + textureSamplerName + ";"
	"etFragmentIn etHighp vec2 texCoord;"
	"etFragmentIn etLowp vec4 tintColor;"
	"etFragmentIn etLowp vec4 additiveColor;"
	"void main()"
	"{"
	"	etFragmentOut = tintColor + additiveColor;"
	"	etFragmentOut.w *= etTexture2D(inputTexture, texCoord).x;"
	"}";
// */
	
}
}
