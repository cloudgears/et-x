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

const size_t BlockSize = 2048;

extern std::string et_scene2d_default_shader_vs;
extern std::string et_scene2d_default_shader_fs;

const std::string SceneRenderer::defaultProgramName = "et-scene2d-default-shader";

SceneRenderer::SceneRenderer(RenderContext* rc) :
	_rc(rc), _additionalOffsetAndAlpha(0.0f, 0.0f, 1.0f)
{
	pushClipRect(recti(vec2i(0), rc->sizei()));
	
	_defaultProgram = createProgramWithFragmentshader(defaultProgramName, et_scene2d_default_shader_fs);
	
	_defaultTexture = rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(1), GL_RGBA,
		GL_UNSIGNED_BYTE, BinaryDataStorage(4, 0), "gui-default-texture");
	_defaultTexture->setFiltration(rc, TextureFiltration_Nearest, TextureFiltration_Nearest);
	
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
	assert(_clip.size() > 1);
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

	_cameraFor3dElements.perspectiveProjection(DEG_30, contextSize.aspect(), 0.1f, 10.0f);
	_cameraFor3dElements.lookAt(vec3(0.0f, 0.0f, std::cos(DEG_15) / std::sin(DEG_15)), vec3(0.0f), unitY);
}

void s2d::SceneRenderer::alloc(size_t count)
{
	if (_renderingElement.invalid()) return;

	size_t currentOffset = _renderingElement->vertexList.offset();
	size_t currentSize = _renderingElement->vertexList.size();

	if (currentOffset + count >= currentSize)
	{
		size_t newSize = currentSize + BlockSize * (1 + count / BlockSize);
		_renderingElement->vertexList.resize(newSize);
		_renderingElement->indexArray->resize(newSize);
		_renderingElement->indexArray->linearize();
	}
}

GuiVertexPointer s2d::SceneRenderer::allocateVertices(size_t count, const Texture& inTexture,
	const SceneProgram& inProgram, ElementRepresentation rep)
{
	if (!_renderingElement.valid()) return 0;
	
	Texture texture = inTexture.invalid() ? _defaultTexture : inTexture;

	bool shouldAdd = _renderingElement->chunks.empty();
	
	_renderingElement->changed = true;
	size_t i0 = _renderingElement->vertexList.offset();

	if (_renderingElement->chunks.size())
	{
		RenderChunk& lastChunk = _renderingElement->chunks.back();
		
		bool sameConfiguration = (lastChunk.representation == rep) && (lastChunk.clip == _clip.top()) &&
			(lastChunk.texture == texture) && (lastChunk.program == inProgram);
		
		if (sameConfiguration)
			lastChunk.count += count;
		
		shouldAdd = !sameConfiguration;
	}

	if (shouldAdd)
	{
		_lastTexture = texture;
		_lastProgram = inProgram;
		_renderingElement->chunks.emplace_back(i0, count, _clip.top(), _lastTexture, _lastProgram, rep);
	}
	
	alloc(count);
	
	_renderingElement->vertexList.applyOffset(count);

	assert(i0 < _renderingElement->vertexList.size());
	assert(i0 * _renderingElement->vertexList.typeSize() < _renderingElement->vertexList.dataSize());

	return _renderingElement->vertexList.element_ptr(i0);
}

size_t SceneRenderer::addVertices(const GuiVertexList& vertices, const Texture& texture,
	ElementRepresentation cls)
{
	size_t current = 0;
	size_t count = vertices.offset();
	
	if (_renderingElement.valid() && (count > 0))
	{
		current = _renderingElement->vertexList.offset();
		GuiVertex* v0 = allocateVertices(count, texture, _defaultProgram, cls);
		etCopyMemory(v0, vertices.data(), count * vertices.typeSize());
	}

	return current;
}

size_t SceneRenderer::addVertices(const GuiVertexList& vertices, const Texture& texture)
{
	return addVertices(vertices, texture, ElementRepresentation_2d);
}

void s2d::SceneRenderer::setRendernigElement(const RenderingElement::Pointer& r)
{
	_renderingElement = r;
	_lastTexture = _defaultTexture;
}

void s2d::SceneRenderer::beginRender(RenderContext* rc)
{
	rc->renderer()->clear(false, true);

	_depthTestEnabled = rc->renderState().depthTestEnabled();
	_blendEnabled = rc->renderState().blendEnabled();
	_blendState = rc->renderState().blendState();
	_depthMask = rc->renderState().depthMask();
	_clipEnabled =  rc->renderState().clipEnabled();
	_clipRect = rc->renderState().clipRect();
	
	rc->renderState().setBlend(true, Blend_Default);
}

void SceneRenderer::endRender(RenderContext* rc)
{
	rc->renderState().setDepthTest(_depthTestEnabled);
	rc->renderState().setBlend(_blendEnabled, static_cast<BlendState>(_blendState));
	rc->renderState().setDepthMask(_depthMask);
	rc->renderState().setClip(_clipEnabled, _clipRect);
}

void s2d::SceneRenderer::render(RenderContext* rc)
{
	if (!_renderingElement.valid()) return;

	RenderState& rs = rc->renderState();
	Renderer* renderer = rc->renderer();

	ElementRepresentation representation = ElementRepresentation_max;
	
	const IndexBuffer& indexBuffer =
		_renderingElement->vertexArrayObject()->indexBuffer();
	
	Program::Pointer lastBoundProgram;
	for (auto& i : _renderingElement->chunks)
	{
		if (lastBoundProgram != i.program.program)
		{
			lastBoundProgram = i.program.program;
			rs.bindProgram(lastBoundProgram);
			lastBoundProgram->setUniform(i.program.additionalOffsetAndAlpha, _additionalOffsetAndAlpha);
		}
		
		rs.bindTexture(0, i.texture);
		rs.setClip(true, i.clip + _additionalWindowOffset);
		
		if (i.representation != representation)
		{
			representation = i.representation;
			bool is3D = (i.representation == ElementRepresentation_3d);
			rs.setDepthTest(is3D);
			rs.setDepthMask(is3D);
			lastBoundProgram->setTransformMatrix(is3D ? _cameraFor3dElements.modelViewProjectionMatrix() : _defaultTransform);
		}

		renderer->drawElements(indexBuffer, i.first, i.count);
	}
}

void SceneRenderer::setAdditionalOffsetAndAlpha(const vec3& offsetAndAlpha)
{
	_additionalOffsetAndAlpha = vec3(2.0f * offsetAndAlpha.xy(), offsetAndAlpha.z);
	_additionalWindowOffset.left = static_cast<int>(offsetAndAlpha.x * _rc->size().x);
	_additionalWindowOffset.top = static_cast<int>(offsetAndAlpha.y * _rc->size().y);
}

SceneProgram SceneRenderer::createProgramWithFragmentshader(const std::string& name, const std::string& fs)
{
	SceneProgram program;
	program.program = _rc->programFactory().genProgram(name, et_scene2d_default_shader_vs, std::string(),
		fs, _sharedCache, StringList());
	program.program->setUniform("layer0_texture", 0);
	program.additionalOffsetAndAlpha = program.program->getUniform("additionalOffsetAndAlpha");
	return program;
}

std::string et_scene2d_default_shader_vs =
	"uniform mat4 mTransform;"
	"uniform vec3 additionalOffsetAndAlpha;"

	"etVertexIn vec3 Vertex;"
	"etVertexIn vec4 TexCoord0;"
	"etVertexIn vec4 Color;"

	"etVertexOut vec2 vTexCoord;"
	"etVertexOut etLowp vec4 tintColor;"
	"etVertexOut etLowp vec4 additiveColor;"

	"void main()"
	"{"
		"vTexCoord = TexCoord0.xy;"
		"vec4 alphaScaledColor = Color * vec4(1.0, 1.0, 1.0, additionalOffsetAndAlpha.z);"
		"additiveColor = alphaScaledColor * TexCoord0.w;"
		"tintColor = alphaScaledColor * (1.0 - TexCoord0.w);"
		"vec4 vTransformed = mTransform * vec4(Vertex, 1.0);"
		"gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
	"}";

std::string et_scene2d_default_shader_fs =
	"uniform etLowp sampler2D layer0_texture;"
	"etFragmentIn etHighp vec2 vTexCoord;"
	"etFragmentIn etLowp vec4 tintColor;"
	"etFragmentIn etLowp vec4 additiveColor;"
	"void main()"
	"{"
	"	etFragmentOut = etTexture2D(layer0_texture, vTexCoord) * tintColor + additiveColor;"
	"}";
