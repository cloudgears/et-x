/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/notifytimer.h>
#include <et-ext/helpers/particles.h>
#include <et-ext/scene2d/element2d.h>

namespace et
{
class RenderContext;

namespace s2d
{
class ParticlesElement : public s2d::Element2d
{
public:
	ET_DECLARE_POINTER(ParticlesElement);

public:
	ParticlesElement(uint32_t, Element2d*, const std::string& = emptyString);

	particles::PointSpriteEmitter& emitter()
	{
		return _particles;
	}

	const particles::PointSpriteEmitter& emitter() const
	{
		return _particles;
	}

	void setTexture(const Texture::Pointer&);
	void setBaseAndVariationParticles(const particles::PointSprite&, const particles::PointSprite&);

	void start();
	void stop();
	void pause();

private:
	void addToRenderQueue(RenderContext*, SceneRenderer&);

private:
	Texture::Pointer _texture;
	Texture::Pointer _defaultTexture;
	MaterialInstance::Pointer _material;
	SceneVertexList _vertices;

	particles::PointSpriteEmitter _particles;

	NotifyTimer _updateTimer;
};
}
}
