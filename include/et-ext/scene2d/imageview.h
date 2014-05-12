/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>
#include <et-ext/scene2d/scenerenderer.h>

namespace et
{
	namespace s2d
	{
		class ImageView : public Element2d
		{
		public:
			ET_DECLARE_POINTER(ImageView)
			
		public:
			enum ContentMode
			{
				ContentMode_Stretch,
				ContentMode_Center,
				ContentMode_Fit,
				ContentMode_FitAnyway,
				ContentMode_Fill,
				ContentMode_Tile,
				ContentMode_Crop,
			};

		public:
			ImageView(Element2d* parent,
				const std::string& name = std::string());
			
			ImageView(const Texture& texture, Element2d* parent,
				const std::string& name = std::string());

			ImageView(const Texture& texture, const ImageDescriptor& i, Element2d* parent,
				const std::string& name = std::string());
			
			ImageView(const Image&, Element2d* parent,
				const std::string& name = std::string());

			const Texture& texture() const
				{ return _texture; }

			Texture& texture() 
				{ return _texture; }

			const ImageDescriptor& imageDescriptor() const
				{ return _descriptor; }

			const vec4& backgroundColor() const
				{ return _backgroundColor; }
			
			void setImageDescriptor(const ImageDescriptor& d);
			void setContentMode(ImageView::ContentMode cm);
			void setTexture(const Texture& t, bool updateDescriptor);
			void setImage(const Image& img);
			void setBackgroundColor(const vec4& color, float duration = 0.0f);
			
			vec2 contentSize();

			ImageDescriptor calculateImageFrame();

			ET_DECLARE_EVENT1(onPointerReleased, const PointerInputInfo&)
			
			ET_DECLARE_PROPERTY_GET_REF(vec2, actualImageOrigin)
			ET_DECLARE_PROPERTY_GET_REF(vec2, actualImageSize)
			
		public:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			
		private:
			bool pointerReleased(const PointerInputInfo&);
			
			void buildVertices(RenderContext*, SceneRenderer&);

		private:
			Texture _texture;
			SceneVertexList _vertices;
			ImageDescriptor _descriptor;
			ImageView::ContentMode _contentMode;
			vec4 _backgroundColor;
			Vector4Animator _backgroundColorAnimator;
		};

		typedef std::vector<ImageView::Pointer> ImageViewList;
	}
}
