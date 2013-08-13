/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/imageview.h>

using namespace et;
using namespace s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(ImageView)

ImageView::ImageView(Element2d* parent, const std::string& name) :
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _contentMode(ImageView::ContentMode_Stretch)
{
}

ImageView::ImageView(const Texture& texture, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _texture(texture),
	_descriptor(ImageDescriptor(texture)), _contentMode(ImageView::ContentMode_Stretch)
{
	setSize(_descriptor.size);
}

ImageView::ImageView(const Texture& texture, const ImageDescriptor& i, Element2d* parent,
	const std::string& name) : Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _texture(texture),
	_descriptor(i), _contentMode(ImageView::ContentMode_Stretch)
{
	setSize(_descriptor.size, 0.0f);
}

ImageView::ImageView(const Image& img, Element2d* parent, const std::string& name) : 
	Element2d(parent, ET_GUI_PASS_NAME_TO_BASE_CLASS), _texture(img.texture), _descriptor(img.descriptor),
	_contentMode(ImageView::ContentMode_Stretch)
{
	setSize(_descriptor.size, 0.0f);
}

void ImageView::addToRenderQueue(RenderContext* rc, SceneRenderer& g)
{
	if (!contentValid() || !transformValid())
		buildVertices(rc, g);

	if (_vertices.offset())
		g.addVertices(_vertices, _texture);
}

void ImageView::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	_vertices.setOffset(0);
	
	if (_backgroundColor.w > 0.0f)
	{
		buildColorVertices(_vertices, rect(vec2(0.0f), size()), _backgroundColor,
			transform);
	}
	
	if (!_texture.valid()) return;

	if (_contentMode == ContentMode_Tile)
	{
		_actualImageSize = _descriptor.size;
		_actualImageSize = vec2(0.0f);

		size_t repeatsWidth = static_cast<size_t>(size().x / _descriptor.size.x);
		size_t repeatsHeight = static_cast<size_t>(size().y / _descriptor.size.y);

		_vertices.fitToSize(repeatsWidth * repeatsHeight * measuseVertexCountForImageDescriptor(_descriptor));

		for (size_t v = 0; v < repeatsHeight; ++v)
		{
			for (size_t u = 0; u < repeatsWidth; ++u)
			{
				float fx = static_cast<float>(u * _descriptor.size.x);
				float fy = static_cast<float>(v * _descriptor.size.y);
				buildImageVertices(_vertices, _texture, _descriptor,
					rect(vec2(fx, fy), _descriptor.size), color(), transform);
			}
		}
	}
	else
	{
		ImageDescriptor desc = calculateImageFrame();
		buildImageVertices(_vertices, _texture, desc, rect(_actualImageOrigin, _actualImageSize),
			color(), transform);
	}


	setContentValid();
}

void ImageView::setImageDescriptor(const ImageDescriptor& d)
{
	_descriptor = d;
	invalidateContent();
}

void ImageView::setContentMode(ImageView::ContentMode cm)
{
	if (cm != _contentMode)
	{
		_contentMode = cm;
		invalidateContent();
	}
}

void ImageView::setTexture(const Texture& t, bool updateDescriptor)
{
	_texture = t;
	if (updateDescriptor)
		_descriptor = ImageDescriptor(t);
	invalidateContent();
}

void ImageView::setImage(const Image& img)
{
	_texture = img.texture;
	_descriptor = img.descriptor;
	invalidateContent();
}

void ImageView::setBackgroundColor(const vec4& color)
{
	_backgroundColor = color;
	invalidateContent();
}

ImageDescriptor ImageView::calculateImageFrame()
{
	ImageDescriptor desc = _descriptor;

	switch (_contentMode)
	{
		case ContentMode_Center:
		{
			_actualImageSize = _descriptor.size;
			_actualImageOrigin = 0.5f * (size() - _descriptor.size);
			break;
		}

		case ContentMode_Fit:
		case ContentMode_FitAnyway:
		case ContentMode_Fill:
		{
			vec2 frameSize = size();
			vec2 descSize = absv(_descriptor.size);

			if ((_contentMode == ContentMode_FitAnyway) || (descSize.x > frameSize.x) ||
				(descSize.y > frameSize.y))
			{
				float imageAspect = descSize.aspect();
				float frameAspect = frameSize.aspect();
				if (frameAspect > 1.0f)
				{
					if (imageAspect > 1.0f)
					{
						float resultHeight = frameSize.x / imageAspect;
						if (resultHeight > frameSize.y)
						{
							float scale = frameSize.y / resultHeight;
							frameSize.x *= scale;
							frameSize.y = resultHeight * scale;
						}
						else
						{
							frameSize.y = resultHeight;
						}
					}
					else
					{
						frameSize.x = frameSize.x * imageAspect / frameAspect;
					}
				}
				else
				{
					frameSize.y = frameSize.y / imageAspect * frameAspect;
				}
			}
			else
			{
				frameSize = descSize;
			}

			if (_contentMode == ContentMode_Fill)
			{
				vec2 sizeAspect = frameSize / size();
				float minScale = etMin(sizeAspect.x, sizeAspect.y);
				frameSize /= minScale;
			}

			_actualImageSize = frameSize;
			_actualImageOrigin = 0.5f * (size() - _actualImageSize);
			break;
		}

		case ContentMode_Crop:
		{
			vec2 dSize = desc.size;
			desc.size.x = size().x < desc.size.x ? size().x : dSize.x;
			desc.size.y = size().y < desc.size.y ? size().y : dSize.y;
			vec2 cropped = dSize - desc.size;
			desc.origin += cropped * pivotPoint();
			_actualImageSize = desc.size;
			_actualImageOrigin = vec2(0.0f);
			break;
		}

		default:
		{
			_actualImageSize = size();
			_actualImageOrigin = vec2(0.0f);
		}
	}

	return desc;
}

vec2 ImageView::contentSize()
{
	return _descriptor.size;
}
