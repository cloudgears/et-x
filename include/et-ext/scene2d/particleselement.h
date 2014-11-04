//
//  ParticlesElement.h
//  Prime Elements
//
//  Created by Sergey Reznik on 3/11/2014.
//  Copyright (c) 2014 Sergey Reznik. All rights reserved.
//

#pragma once

#include <et/helpers/particles.h>

namespace et
{
	class RenderContext;
	
	namespace s2d
	{
		class ParticlesElement : public et::s2d::Element2d
		{
		public:
			ET_DECLARE_POINTER(ParticlesElement)
			
		public:
			ParticlesElement(size_t, Element2d*, const std::string& = emptyString);
			
			particles::PointSpriteEmitter& emitter()
				{ return _particles; }

			const particles::PointSpriteEmitter& emitter() const
				{ return _particles; }
			
			void setTexture(const et::Texture&);
			void setBaseAndVariationParticles(const particles::PointSprite&, const particles::PointSprite&);
			
			void start();
			void stop();
			void pause();
			
		private:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			
			et::s2d::SceneProgram program() const;
			et::s2d::SceneProgram initProgram(et::s2d::SceneRenderer&);
			void setProgramParameters(et::Program::Pointer&);
			
		private:
			et::Texture _texture;
			et::Texture _defaultTexture;
			et::s2d::SceneProgram _program;
			SceneVertexList _vertices;
			
			particles::PointSpriteEmitter _particles;
			
			NotifyTimer _updateTimer;
		};
	}
}
