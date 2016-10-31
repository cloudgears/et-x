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
		class ParticlesElement : public et::s2d::Element2d
		{
		public:
			ET_DECLARE_POINTER(ParticlesElement);
			
		public:
			ParticlesElement(uint32_t, Element2d*, const std::string& = emptyString);
			
			particles::PointSpriteEmitter& emitter()
				{ return _particles; }

			const particles::PointSpriteEmitter& emitter() const
				{ return _particles; }
			
			void setTexture(const et::Texture::Pointer&);
			void setBaseAndVariationParticles(const particles::PointSprite&, const particles::PointSprite&);
			
			void start();
			void stop();
			void pause();
			
		private:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			
			et::s2d::SceneProgram program() const;
			et::s2d::SceneProgram initProgram(et::s2d::SceneRenderer&);
			void setProgramParameters(et::RenderContext*, et::Program::Pointer&);
			
		private:
			et::Texture::Pointer _texture;
			et::Texture::Pointer _defaultTexture;
			et::s2d::SceneProgram _program;
			SceneVertexList _vertices;
			
			particles::PointSpriteEmitter _particles;
			
			NotifyTimer _updateTimer;
		};
	}
}
