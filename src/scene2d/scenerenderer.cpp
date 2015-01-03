/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/scenerenderer.h>
#include <et/opengl/openglcaps.h>

using namespace et;
using namespace et::s2d;

extern std::string et_scene2d_default_shader_vs;

extern std::string et_scene2d_default_shader_fs;

const std::string SceneRenderer::defaultProgramName = "et-scene2d-default-shader";

extern const std::string textureSamplerName = "inputTexture";
extern const std::string additionalOffsetAndAlphaUniform = "additionalOffsetAndAlpha";

SceneRenderer::SceneRenderer(RenderContext* rc) :
	_rc(rc), _additionalOffsetAndAlpha(0.0f, 0.0f, 1.0f)
{
	pushClipRect(recti(vec2i(0), rc->sizei()));
	
	_defaultProgram = createProgramWithFragmentshader(defaultProgramName, et_scene2d_default_shader_fs);
	_defaultProgram.program->setUniform(textureSamplerName, 0);
	
	_defaultTexture = rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::RGBA,
		vec2i(1), TextureFormat::RGBA, DataType::UnsignedChar, BinaryDataStorage(4, 0), "scene-default-texture");
	_defaultTexture->setFiltration(rc, TextureFiltration::Nearest, TextureFiltration::Nearest);
	
	setProjectionMatrices(rc->size());
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
	
	_defaultTransform = identityMatrix;
	
	_defaultTransform[0][0] =  2.0f / contextSize.x;
	_defaultTransform[1][1] = -2.0f / contextSize.y;
	_defaultTransform[3][0] = -1.0f;
	_defaultTransform[3][1] = 1.0f;
	_defaultTransform[3][3] = 1.0f;
}

SceneVertex* s2d::SceneRenderer::allocateVertices(size_t count, const Texture& inTexture,
	const SceneProgram& inProgram, Element2d* object, PrimitiveType pt)
{
	ET_ASSERT(_renderingElement.valid());
	
	if (object && !object->hasFlag(Flag_DynamicRendering))
		object = nullptr;

	const Texture& actualTexture = inTexture.valid() ? inTexture : _defaultTexture;
	
	bool isDynamicObject = (object != nullptr) && (object->hasFlag(Flag_DynamicRendering));
	bool shouldAdd = isDynamicObject || _renderingElement->chunks.empty();
	if ((shouldAdd == false) && _renderingElement->chunks.size())
	{
		RenderChunk& lastChunk = _renderingElement->chunks.back();
		
		bool sameConfiguration = (lastChunk.clip == _clip.top()) && (lastChunk.texture == actualTexture) &&
			(lastChunk.program.program == inProgram.program) && (lastChunk.primitiveType == pt);
		
		if (sameConfiguration)
			lastChunk.count += count;
		else
			shouldAdd = true;
	}

	if (shouldAdd)
	{
		_lastProgram = inProgram;
		_lastTexture = actualTexture;
		
		_renderingElement->chunks.emplace_back(_renderingElement->allocatedVertices, count,
			_clip.top(), _lastTexture, _lastProgram, object, pt);
	}
	
	return _renderingElement->allocateVertices(count);
}

void SceneRenderer::addVertices(const SceneVertexList& vertices, const Texture& texture,
	const SceneProgram& program, Element2d* owner, PrimitiveType pt)
{
	size_t count = vertices.lastElementIndex();
	ET_ASSERT((count > 0) && _renderingElement.valid() && program.valid());
	
	auto target = allocateVertices(count, texture, program, owner, pt);
	etCopyMemory(target, vertices.data(), count * vertices.typeSize());
}

void s2d::SceneRenderer::setRendernigElement(const RenderingElement::Pointer& r)
{
	_renderingElement = r;
	_lastTexture = _defaultTexture;
}

void s2d::SceneRenderer::beginRender(RenderContext* rc)
{
	_depthMaskEnabled = rc->renderState().depthMask();
	_blendEnabled = rc->renderState().blendEnabled();
	_lastBlendState = rc->renderState().blendState();
	_depthTestEnabled = rc->renderState().depthTestEnabled();
	_clipeEnabled = rc->renderState().clipEnabled();
	_latestClipRect = rc->renderState().clipRect();
	
	if (rc->renderState().depthMask())
	{
		rc->renderer()->clear(false, true);
		rc->renderState().setDepthMask(false);
	}
	
	rc->renderState().setDepthTest(false);
	rc->renderState().setBlend(true, BlendState::Default);
}

void s2d::SceneRenderer::render(RenderContext* rc)
{
	if (!_renderingElement.valid()) return;

	RenderState& rs = rc->renderState();
	Renderer* renderer = rc->renderer();
	const IndexBuffer& indexBuffer = _renderingElement->vertexArrayObject()->indexBuffer();
	
	Program::Pointer lastBoundProgram;
	for (auto& i : _renderingElement->chunks)
	{
		if (lastBoundProgram != i.program.program)
		{
			lastBoundProgram = i.program.program;
			rs.bindProgram(lastBoundProgram);
			lastBoundProgram->setUniform(i.program.additionalOffsetAndAlpha, _additionalOffsetAndAlpha);
			lastBoundProgram->setTransformMatrix(_defaultTransform);
		}
		
		if (i.object != nullptr)
			i.object->setProgramParameters(rc, lastBoundProgram);
		
		rs.bindTexture(0, i.texture);
		rs.setClip(true, i.clip + _additionalWindowOffset);
		
		renderer->drawElements(i.primitiveType, indexBuffer, i.first, i.count);
	}
}

void SceneRenderer::endRender(RenderContext* rc)
{
	auto& rs = rc->renderState();
	rs.setClip(_clipeEnabled, _latestClipRect);
	rs.setDepthMask(_depthMaskEnabled);
	rs.setDepthTest(_depthTestEnabled);
	rs.setBlend(_blendEnabled, _lastBlendState);
}

void SceneRenderer::setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha)
{
	_additionalOffsetAndAlpha = vec3(2.0f * offsetAndAlpha.xy(), offsetAndAlpha.z);
	_additionalWindowOffset.left = static_cast<int>(offsetAndAlpha.x * _rc->size().x);
	_additionalWindowOffset.top = static_cast<int>(offsetAndAlpha.y * _rc->size().y);
}

SceneProgram SceneRenderer::createProgramWithShaders(const std::string& name, const std::string& vs, const std::string& fs)
{
	Program::Pointer existingProgram = _programsCache.findAnyObject(name);
	
	SceneProgram program;
	if (existingProgram.invalid())
	{
		program.program = _rc->programFactory().genProgram(name, vs, fs);
		_programsCache.manage(program.program, ObjectLoader::Pointer());
	}
	else
	{
		program.program = existingProgram;
		_rc->renderState().bindProgram(program.program);
	}
	
	program.additionalOffsetAndAlpha = program.program->getUniform(additionalOffsetAndAlphaUniform);
	
	ET_ASSERT((program.additionalOffsetAndAlpha.location != -1) &&
		"Program should contain uniform for additional offset and alpha, of type vec3 and named additionalOffsetAndAlpha");
	
	return program;
}

SceneProgram SceneRenderer::createProgramWithFragmentshader(const std::string& name, const std::string& fs)
{
	return createProgramWithShaders(name, et_scene2d_default_shader_vs, fs);
}

std::string et_scene2d_default_shader_vs =
	"uniform mat4 mTransform;"
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
		"vec4 vTransformed = mTransform * vec4(Vertex, 1.0);"
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
