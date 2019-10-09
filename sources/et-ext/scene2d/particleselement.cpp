/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.hpp>
#include <et-ext/scene2d/particleselement.hpp>
#include <et-ext/scene2d/scenerenderer.hpp>

namespace et {
namespace s2d {

ParticlesElement::ParticlesElement(uint32_t amount, Element2d* parent, const std::string& name)
  : s2d::Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS)
  , _vertices(amount, 0)
  , _particles(amount) {
  setLocationInParent(s2d::Location_Center);
  setFlag(s2d::Flag_DynamicRendering | s2d::Flag_TransparentForPointer);

  particles::PointSprite baseParticle;
  particles::PointSprite variationParticle;

  baseParticle.position = vec3(0.0f);
  baseParticle.velocity = vec3(0.0f, 0.0f, 0.0f);
  baseParticle.acceleration = vec3(0.0f);
  baseParticle.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
  baseParticle.size = 3.0;
  baseParticle.emitTime = actualTime();
  baseParticle.lifeTime = 3.0f;

  variationParticle.position = vec3(5.0f, 5.0f, 0.0f);
  variationParticle.velocity = vec3(15.0f, 15.0f, 0.0f);
  variationParticle.acceleration = vec3(0.0f);
  variationParticle.color = vec4(0.0f);
  variationParticle.size = 1.0f;
  variationParticle.lifeTime = 2.5f;

  _particles.setBase(baseParticle);
  _particles.setVariation(variationParticle);

  _updateTimer.expired.connect([this](NotifyTimer* timer) {
    _particles.update(timer->actualTime());
    invalidateContent();
  });
}

void ParticlesElement::setBaseAndVariationParticles(const particles::PointSprite& b, const particles::PointSprite& v) {
  _particles.setBase(b);
  _particles.setVariation(v);
}

void ParticlesElement::start() {
  _particles.emitMissingParticles(_updateTimer.actualTime());

  _updateTimer.start(timerPool(), 0.0f, NotifyTimer::RepeatForever);
  invalidateContent();
}

void ParticlesElement::stop() {
  _particles.clear();
  _updateTimer.cancelUpdates();
}

void ParticlesElement::pause() {
  _updateTimer.cancelUpdates();
}

void ParticlesElement::addToRenderQueue(RenderInterface::Pointer& rc, SceneRenderer& r) {
  if (_defaultTexture.invalid()) {
    TextureDescription::Pointer desc = TextureDescription::Pointer::create();
    desc->size = vec2i(1);
    desc->format = TextureFormat::RGBA8;
    desc->data = BinaryDataStorage(4, 255);
    _defaultTexture = rc->createTexture(desc);

    if (_texture.invalid()) _texture = _defaultTexture;
  }

  if (_particles.activeParticlesCount() > 0) {
    vec4 fc = finalColor();

    if (!contentValid()) {
      for (uint32_t i = 0; i < _particles.activeParticlesCount(); ++i) {
        const auto& p = _particles.particle(i);
        _vertices[i].position = vec4(p.position.xy(), p.size, 0.0f);
        _vertices[i].color = fc * p.color;
      }
      _vertices.setOffset(_particles.activeParticlesCount());
      setContentValid();
    }

    materialInstance()->setTexture(MaterialTexture::BaseColor, _texture);
    r.addVertices(_vertices, materialInstance(), this, PrimitiveType::Points);
  }
}

/*
s2d::SceneProgram ParticlesElement::program() const
{
    return _program;
}

s2d::SceneProgram ParticlesElement::initProgram(s2d::SceneRenderer& r)
{
    if (_program.invalid())
    {
        _program = r.createProgramWithShaders("default-particles-shader", particlesVertexShader, particlesFragmentShader);
        setDefaultProgram(_program);
    }
    return _program;
}

void ParticlesElement::setProgramParameters(RenderInterface::Pointer&, Program::Pointer& p)
{
    // TODO : set parameter
    // p->setUniform("finalTransform", finalTransform());
}
*/

void ParticlesElement::setTexture(const Texture::Pointer& t) {
  _texture = t.invalid() ? _defaultTexture : t;
}

}  // namespace s2d

}  // namespace et

/*
const std::string particlesVertexShader =
"uniform mat4 matWorld;"
"uniform mat4 finalTransform;"
"uniform vec3 additionalOffsetAndAlpha;"

"etVertexIn vec3 Vertex;"
"etVertexIn vec4 TexCoord0;"
"etVertexIn vec4 Color;"

"etVertexOut etHighp vec2 texCoord;"
"etVertexOut etLowp vec4 tintColor;"

"void main()"
"{"
"	vec4 vTransformed = matWorld * finalTransform * vec4(Vertex.xy, 0.0, 1.0);"
"	texCoord = TexCoord0.xy;"
"	tintColor = Color * vec4(1.0, 1.0, 1.0, additionalOffsetAndAlpha.z);"
"	gl_PointSize = Vertex.z;"
"	gl_Position = vTransformed + vec4(vTransformed.w * additionalOffsetAndAlpha.xy, 0.0, 0.0);"
"}";

const std::string particlesFragmentShader =
"uniform sampler2D inputTexture;"
"etFragmentIn etLowp vec4 tintColor;"
"void main()"
"{"
"	etFragmentOut = tintColor * etTexture2D(inputTexture, gl_PointCoord);"
"}"
"";
*/