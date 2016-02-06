/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/textelement.h>

using namespace et;
using namespace s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(TextElement)

extern const std::string textureSamplerName;
extern const std::string additionalOffsetAndAlphaUniform;
extern std::string et_scene2d_default_text_shader_vs_plain;
extern std::string et_scene2d_default_text_shader_fs_plain;

extern std::string et_scene2d_default_text_shader_sdf_vs_base;
extern std::string et_scene2d_default_text_shader_sdf_fs_plain;

extern std::string et_scene2d_default_text_shader_sdf_vs_shadow;
extern std::string et_scene2d_default_text_shader_sdf_fs_shadow;

extern std::string et_scene2d_default_text_shader_sdf_vs_bevel;
extern std::string et_scene2d_default_text_shader_sdf_fs_bevel;
extern std::string et_scene2d_default_text_shader_sdf_fs_inv_bevel;

const float maxShadowDistance = 1.0f;

TextElement::TextElement(Element2d* parent, const Font::Pointer& f, float fsz, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _font(f), _fontSize(fsz)
{
	setFlag(s2d::Flag_DynamicRendering);
}

void TextElement::setFont(const Font::Pointer& f)
{
	_font = f;
	invalidateText();
}

void TextElement::setFontSize(float fsz)
{
	_fontSize = fsz;
	invalidateText();
}

void TextElement::setFontSmoothing(float fsm)
{
	_fontSmoothing = fsm;
	invalidateText();
}

void TextElement::processMessage(const Message& msg)
{
	if (msg.type == Message::Type_SetFontSmoothing)
		setFontSmoothing(msg.paramf);
}

void TextElement::loadProperties(const Dictionary& d)
{
	if (d.hasKey("font_size"))
	{
		auto obj = d.objectForKey("font_size");
		
		if (obj->variantClass() == VariantClass::Float)
			setFontSize(FloatValue(obj)->content);
		else if (obj->variantClass() == VariantClass::Integer)
			setFontSize(static_cast<float>(IntegerValue(obj)->content));
	}
	
	if (d.hasKey("font_smoothing"))
	{
		auto obj = d.objectForKey("font_smoothing");
		
		if (obj->variantClass() == VariantClass::Float)
			setFontSmoothing(FloatValue(obj)->content);
		else if (obj->variantClass() == VariantClass::Integer)
			setFontSmoothing(static_cast<float>(IntegerValue(obj)->content));
	}
	
	if (d.hasKey("font_style"))
	{
		auto obj = d.objectForKey("font_style");
		if (obj->variantClass() == VariantClass::String)
		{
			if (StringValue(obj)->content == "shadow")
				setTextStyle(TextStyle_SignedDistanceFieldShadow);
			else
				setTextStyle(TextStyle_SignedDistanceField);
		}
	}
}

SceneProgram& TextElement::textProgram(SceneRenderer& r)
{
	if (_textProgram.invalid())
		initTextProgram(r);
	
	return _textProgram;
}

void TextElement::initTextProgram(SceneRenderer& r)
{
	static const std::string programNames[TextStyle_max] =
	{
		"et-default-text-program-sdf-plain",
		"et-default-text-program-sdf-shadow",
		"et-default-text-program-sdf-bevel",
		"et-default-text-program-plain",
	};
	
	const std::string vertexShaders[TextStyle_max] =
	{
		et_scene2d_default_text_shader_sdf_vs_base,
		et_scene2d_default_text_shader_sdf_vs_shadow,
		et_scene2d_default_text_shader_sdf_vs_bevel,
		et_scene2d_default_text_shader_sdf_vs_bevel,
		et_scene2d_default_text_shader_vs_plain,
	};
	
	const std::string fragmentShaders[TextStyle_max] =
	{
		et_scene2d_default_text_shader_sdf_fs_plain,
		et_scene2d_default_text_shader_sdf_fs_shadow,
		et_scene2d_default_text_shader_sdf_fs_bevel,
		et_scene2d_default_text_shader_sdf_fs_inv_bevel,
		et_scene2d_default_text_shader_fs_plain,
	};
	
	_textProgram = r.createProgramWithShaders(programNames[_textStyle], vertexShaders[_textStyle], fragmentShaders[_textStyle]);
	_shadowUniform = _textProgram.program->getUniform("shadowOffset");
}

void TextElement::setTextStyle(TextStyle style)
{
	if (style == _textStyle) return;
	
	_textStyle = style;
	_textProgram.program.reset(nullptr);
}

void TextElement::setShadowOffset(const vec2& o)
{
	_shadowOffset.x = +(std::abs(o.x) > maxShadowDistance ? maxShadowDistance : o.x);
	_shadowOffset.y = -(std::abs(o.y) > maxShadowDistance ? maxShadowDistance : o.y);
}

void TextElement::setProgramParameters(et::RenderContext*, et::Program::Pointer& p)
{
	if ((_textStyle == TextStyle_SignedDistanceFieldShadow) && (p == _textProgram.program))
		p->setUniform(_shadowUniform, _shadowOffset * _font->generator()->texture()->texel());
}

/*
 * SDF - plain
 */
std::string et_scene2d_default_text_shader_sdf_vs_base =
"uniform mat4 matWorld;"
"uniform vec3 additionalOffsetAndAlpha;"
"etVertexIn vec3 Vertex;"
"etVertexIn vec4 TexCoord0;"
"etVertexIn vec4 Color;"
"etVertexOut etHighp vec2 texCoord;"
"etVertexOut etLowp vec4 tintColor;"
"etVertexOut etLowp vec2 sdfParams;"
"void main()"
"{"
"	texCoord = TexCoord0.xy;"
"	tintColor = Color;"
"	tintColor.w *= additionalOffsetAndAlpha.z;"
"	sdfParams.x = TexCoord0.z - TexCoord0.w;"
"	sdfParams.y = TexCoord0.z + TexCoord0.w;"
"	vec4 vTransformed = matWorld * vec4(Vertex, 1.0);"
"	gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
"}";

std::string et_scene2d_default_text_shader_sdf_fs_plain =
"uniform etLowp sampler2D inputTexture;"
"etFragmentIn etHighp vec2 texCoord;"
"etFragmentIn etLowp vec4 tintColor;"
"etFragmentIn etLowp vec2 sdfParams;"
"void main()"
"{"
"	etFragmentOut = tintColor;"
"	etFragmentOut.w *= smoothstep(sdfParams.x, sdfParams.y, etTexture2D(inputTexture, texCoord).x);"
"}";

/*
 * Shadow
 */
std::string et_scene2d_default_text_shader_sdf_vs_shadow =
"uniform mat4 matWorld;"
"uniform vec3 additionalOffsetAndAlpha;"
"uniform vec2 shadowOffset;"
"etVertexIn vec3 Vertex;"
"etVertexIn vec4 TexCoord0;"
"etVertexIn vec4 Color;"
"etVertexOut etHighp vec2 texCoord;"
"etVertexOut etHighp vec2 shadowTexCoord;"
"etVertexOut etLowp vec4 tintColor;"
"etVertexOut etLowp vec2 sdfParams;"
"void main()"
"{"
"	texCoord = TexCoord0.xy;"
"	shadowTexCoord = TexCoord0.xy - shadowOffset;"
"	tintColor = Color;"
"	tintColor.w *= additionalOffsetAndAlpha.z;"
"	sdfParams.x = TexCoord0.z - TexCoord0.w;"
"	sdfParams.y = TexCoord0.z + TexCoord0.w;"
"	vec4 vTransformed = matWorld * vec4(Vertex, 1.0);"
"	gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
"}";

std::string et_scene2d_default_text_shader_sdf_fs_shadow =
"uniform etLowp sampler2D inputTexture;"
"etFragmentIn etHighp vec2 texCoord;"
"etFragmentIn etHighp vec2 shadowTexCoord;"
"etFragmentIn etLowp vec4 tintColor;"
"etFragmentIn etLowp vec2 sdfParams;"
"const etLowp vec4 shadowColor = vec4(0.0, 0.0, 0.0, 1.0);"
"void main()"
"{"
"	etLowp float sampledTextValue = etTexture2D(inputTexture, texCoord).x;"
"	etLowp float sampledShadowValue = etTexture2D(inputTexture, shadowTexCoord).x;"
"	etLowp float textAlpha = smoothstep(sdfParams.x, sdfParams.y, sampledTextValue);"
"	etFragmentOut = tintColor * vec4(textAlpha, textAlpha, textAlpha, max(textAlpha, sampledShadowValue));"
"}";

/*
 * Bevel
 */
std::string et_scene2d_default_text_shader_sdf_vs_bevel =
"uniform mat4 matWorld;"
"uniform vec3 additionalOffsetAndAlpha;"
"etVertexIn vec3 Vertex;"
"etVertexIn vec4 TexCoord0;"
"etVertexIn vec4 Color;"
"etVertexOut etHighp vec2 texCoord;"
"etVertexOut etHighp vec2 texCoordDx;"
"etVertexOut etHighp vec2 texCoordDy;"
"etVertexOut etLowp vec4 tintColor;"
"etVertexOut etLowp vec2 sdfParams;"
"void main()"
"{"
"	texCoord = TexCoord0.xy;"
"	texCoordDx = texCoord + vec2(1.0 / 1024.0, 0.0);"
"	texCoordDy = texCoord + vec2(0.0, 1.0 / 1024.0);"
"	tintColor = Color;"
"	tintColor.w *= additionalOffsetAndAlpha.z;"
"	sdfParams.x = TexCoord0.z - TexCoord0.w;"
"	sdfParams.y = TexCoord0.z + TexCoord0.w;"
"	vec4 vTransformed = matWorld * vec4(Vertex, 1.0);"
"	gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
"}";

std::string et_scene2d_default_text_shader_sdf_fs_bevel =
R"(
uniform etLowp sampler2D inputTexture;
etFragmentIn etHighp vec2 texCoord;
etFragmentIn etHighp vec2 texCoordDx;
etFragmentIn etHighp vec2 texCoordDy;
etFragmentIn etLowp vec4 tintColor;
etFragmentIn etLowp vec2 sdfParams;

const etLowp vec3 lightDirection = vec3(-0.577350269, 0.577350269, 0.577350269);

#define BEVEL_SCALE 0.002

void main()
{
	etLowp float c0 = etTexture2D(inputTexture, texCoord).x;
	etLowp float c1 = etTexture2D(inputTexture, texCoordDx).x;
	etLowp float c2 = etTexture2D(inputTexture, texCoordDy).x;
	etHighp vec3 v0 = vec3(texCoordDy.x, BEVEL_SCALE * (c2 - c0), texCoordDy.y);
	etHighp vec3 v1 = vec3(texCoordDx.x, BEVEL_SCALE * (c2 - c0), texCoordDx.y);
	etLowp float l = clamp(1.0 - lightDirection.y + dot(normalize(cross(v0, v1)), lightDirection), 0.0, 1.0);
	etLowp float t = smoothstep(sdfParams.x, sdfParams.y, c0);
	etFragmentOut = vec4(l, l, l, t) * tintColor;
})";

std::string et_scene2d_default_text_shader_sdf_fs_inv_bevel =
R"(
uniform etLowp sampler2D inputTexture;
etFragmentIn etHighp vec2 texCoord;
etFragmentIn etHighp vec2 texCoordDx;
etFragmentIn etHighp vec2 texCoordDy;
etFragmentIn etLowp vec4 tintColor;
etFragmentIn etLowp vec2 sdfParams;

const etLowp vec3 lightDirection = vec3(-0.577350269, 0.577350269, 0.577350269);

#define BEVEL_SCALE 0.002

void main()
{
	etLowp float c0 = etTexture2D(inputTexture, texCoord).x;
	etLowp float c1 = etTexture2D(inputTexture, texCoordDx).x;
	etLowp float c2 = etTexture2D(inputTexture, texCoordDy).x;
	etHighp vec3 v0 = vec3(texCoordDy.x, BEVEL_SCALE * (c0 - c2), texCoordDy.y);
	etHighp vec3 v1 = vec3(texCoordDx.x, BEVEL_SCALE * (c0 - c1), texCoordDx.y);
	etLowp float l = clamp(1.0 - lightDirection.y + dot(normalize(cross(v0, v1)), lightDirection), 0.0, 1.0);
	etLowp float t = smoothstep(sdfParams.x, sdfParams.y, c0);
	etFragmentOut = vec4(l, l, l, t) * tintColor;
})";

/*
 * Plain
 */
std::string et_scene2d_default_text_shader_vs_plain =
"uniform mat4 matWorld;"
"uniform vec3 additionalOffsetAndAlpha;"
"etVertexIn vec3 Vertex;"
"etVertexIn vec4 TexCoord0;"
"etVertexIn vec4 Color;"
"etVertexOut etHighp vec2 texCoord;"
"etVertexOut etLowp vec4 tintColor;"
"void main()"
"{"
"	texCoord = TexCoord0.xy;"
"	tintColor = Color;"
"	tintColor.w *= additionalOffsetAndAlpha.z;"
"	vec4 vTransformed = matWorld * vec4(Vertex, 1.0);"
"	gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
"}";

std::string et_scene2d_default_text_shader_fs_plain =
"uniform etLowp sampler2D inputTexture;"
"etFragmentIn etHighp vec2 texCoord;"
"etFragmentIn etLowp vec4 tintColor;"
"void main()"
"{"
"	etFragmentOut = tintColor;"
"	etFragmentOut.w *= etTexture2D(inputTexture, texCoord).x;"
"}";
