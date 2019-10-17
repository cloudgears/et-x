/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.hpp>
#include <et/scene2d/particleselement.hpp>
#include <et/scene2d/scenerenderer.hpp>

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
  if (is_invalid(_defaultTexture)) {
    TextureDescription::Pointer desc = TextureDescription::make_pointer();
    desc->size = vec2i(1);
    desc->format = TextureFormat::RGBA8;
    desc->data = BinaryDataStorage(4, 255);
    _defaultTexture = rc->createTexture(desc);

    if (is_invalid(_texture)) {
      _texture = _defaultTexture;
    }
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

void ParticlesElement::setTexture(const Texture::Pointer& t) {
  _texture = is_invalid(t) ? _defaultTexture : t;
}

}  // namespace s2d
}  // namespace et
