//
//  particleselement.cpp
//  Prime Elements
//
//  Created by Sergey Reznik on 3/11/2014.
//  Copyright (c) 2014 Sergey Reznik. All rights reserved.
//

#include <et-ext/scene2d/particleselement.h>

using namespace et;
using namespace s2d;

extern const std::string particlesVertexShader;
extern const std::string particlesFragmentShader;

ET_DECLARE_SCENE_ELEMENT_CLASS(ParticlesElement)

ParticlesElement::ParticlesElement(size_t amount, Element2d* parent, const std::string& name) :
	s2d::Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _particles(amount)
{
	setFlag(s2d::Flag_DynamicRendering);
	
	_vertices.resize(amount);
	for (size_t i = 0; i < amount; ++i)
		_vertices[i].texCoord = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	
	fillParent();
	
	particles::PointSprite base;
	base.position = vec3(200.0f);
	base.velocity = vec3(70.0f, -15.0f, 0.0f);
	base.acceleration = vec3(0.0f);
	base.color = vec4(0.4f, 0.5f, 0.95f, 1.0f);
	base.size = 15.0;
	base.emitTime = actualTime();
	base.lifeTime = 3.0f;
	
	particles::PointSprite var;
	var.position = vec3(5.0f, 5.0f, 0.0f);
	var.velocity = vec3(35.0f, 10.0f, 0.0f);
	var.acceleration = vec3(randomFloat(0.0f, 2.0f), randomFloat(0.0f, 0.5f), 0.0f);
	var.color = vec4(0.15f, 0.1f, 0.05f, 0.0f);
	var.size = 5.0f;
	var.lifeTime = 2.5f;
	
	_particles.emit(amount, base, var);
	
	_updateTimer.expired.connect([this](NotifyTimer* timer)
	{
		_particles.update(timer->actualTime());
		invalidateContent();
	});
	_updateTimer.start(timerPool(), 0.0f, NotifyTimer::RepeatForever);
}

void ParticlesElement::addToRenderQueue(RenderContext*, SceneRenderer& gr)
{
	initProgram(gr);
	
	mat4 ft = finalTransform();

	for (size_t i = 0; i < _particles.activeParticlesCount(); ++i)
	{
		const auto& p = _particles.particle(i);
		_vertices[i].position = vec3(p.position.xy(), p.size);
		_vertices[i].color = p.color;
	}
	_vertices.setOffset(_particles.activeParticlesCount());
	
	gr.addVertices(_vertices, Texture(), program(), this, PrimitiveType_Points);
}

et::s2d::SceneProgram ParticlesElement::program() const
{
	return _program;
}

et::s2d::SceneProgram ParticlesElement::initProgram(et::s2d::SceneRenderer& r)
{
	if (_program.invalid())
	{
		_program = r.createProgramWithShaders("default-particles-shader", particlesVertexShader, particlesFragmentShader);
		setDefaultProgram(_program);
	}
	return _program;
}

void ParticlesElement::setProgramParameters(et::Program::Pointer& p)
{
	p->setUniform("finalTransform", finalTransform());
}

const std::string particlesVertexShader =
"uniform mat4 mTransform;"
"uniform mat4 finalTransform;"
"uniform vec3 additionalOffsetAndAlpha;"

"etVertexIn vec3 Vertex;"
"etVertexIn vec4 TexCoord0;"
"etVertexIn vec4 Color;"

"etVertexOut etHighp vec2 texCoord;"
"etVertexOut etLowp vec4 tintColor;"

"void main()"
"{"
"	vec4 vTransformed = mTransform * finalTransform * vec4(Vertex.xy, 0.0, 1.0);"
"	texCoord = TexCoord0.xy;"
"	tintColor = Color * vec4(1.0, 1.0, 1.0, additionalOffsetAndAlpha.z);"
"	gl_PointSize = Vertex.z;"
"	gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
"}";

const std::string particlesFragmentShader =
"etFragmentIn etLowp vec4 tintColor;"
"void main()"
"{"
"	etLowp vec2 dUdV = 2.0 * (gl_PointCoord - vec2(0.5));"
"	float alpha = tintColor.w * max(0.0, 1.0 - length(dUdV));"
"	etFragmentOut = vec4(tintColor.xyz, alpha);"
"}"
"";

