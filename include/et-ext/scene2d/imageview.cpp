/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/scene2d/imageview.h>

using namespace et;
using namespace s2d;

ImageView::ImageView(Element2d* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _descriptor(timerPool()),
	_backgroundColorAnimator(timerPool()), _contentMode(ImageView::ContentMode_Stretch)
{
	connectEvents();
}

ImageView::ImageView(const Texture::Pointer& texture, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _texture(texture),
	_descriptor(ImageDescriptor(texture), timerPool()), _backgroundColorAnimator(timerPool()),
	_contentMode(ImageView::ContentMode_Stretch)
{
	connectEvents();
	setSize(_descriptor.value().size);
}

ImageView::ImageView(const Texture::Pointer& texture, const ImageDescriptor& i, Element2d* parent,
	const std::string& name) : Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _texture(texture),
	_descriptor(i, timerPool()), _backgroundColorAnimator(timerPool()), _contentMode(ImageView::ContentMode_Stretch)
{
	connectEvents();
	setSize(_descriptor.value().size, 0.0f);
}

ImageView::ImageView(const Image& img, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _texture(img.texture),
	_descriptor(img.descriptor, timerPool()), _backgroundColorAnimator(timerPool()),
	_contentMode(ImageView::ContentMode_Stretch)
{
	connectEvents();
	setSize(_descriptor.value().size, 0.0f);
}

void ImageView::connectEvents()
{
	_descriptor.updated.connect([this]() { invalidateContent(); });
	_backgroundColorAnimator.updated.connect([this]() { invalidateContent(); });
}

void ImageView::addToRenderQueue(RenderContext* rc, SceneRenderer& r)
{
	validateMaterialInstance(r);

	if (!contentValid() || !transformValid())
		buildVertices(rc, r);

	if (_vertices.lastElementIndex())
	{
		materialInstance()->setTexture(MaterialTexture::BaseColor, _texture);
		r.addVertices(_vertices, _texture, materialInstance(), this);
	}
}

void ImageView::buildVertices(RenderContext*, SceneRenderer&)
{
	mat4 transform = finalTransform();
	_vertices.setOffset(0);

	vec4 alphaScale = vec4(1.0f, finalAlpha());

	if (_backgroundColor.w > 0.0f)
	{
		buildColorVertices(_vertices, rectf(vec2(0.0f), size()), _backgroundColor * alphaScale, transform);
	}

	if (_texture.valid() && (std::abs(_descriptor.value().size.square()) > 0.0f))
	{
		if (_contentMode == ContentMode_Tile)
		{
			_actualImageSize = _descriptor.value().size;

			uint32_t repeatsWidth = std::max(1u, static_cast<uint32_t>(size().x / _descriptor.value().size.x));
			uint32_t repeatsHeight = std::max(1u, static_cast<uint32_t>(size().y / _descriptor.value().size.y));

			_vertices.fitToSize(repeatsWidth * repeatsHeight * measuseVertexCountForImageDescriptor(_descriptor.value()));

			for (size_t v = 0; v < repeatsHeight; ++v)
			{
				for (size_t u = 0; u < repeatsWidth; ++u)
				{
					float fx = static_cast<float>(u * _descriptor.value().size.x);
					float fy = static_cast<float>(v * _descriptor.value().size.y);
					buildImageVertices(_vertices, _texture, _descriptor.value(),
						rectf(vec2(fx, fy), _descriptor.value().size), finalColor(), transform);
				}
			}
		}
		else
		{
			auto frame = calculateImageFrame();
			buildImageVertices(_vertices, _texture, frame,
				rectf(_actualImageOrigin, _actualImageSize), finalColor(), transform);
		}
	}

	setContentValid();
}

void ImageView::setImageDescriptor(const ImageDescriptor& d, float duration)
{
	_descriptor.animate(d, duration);
}

void ImageView::setContentMode(ImageView::ContentMode cm)
{
	if (cm != _contentMode)
	{
		_contentMode = cm;
		invalidateContent();
	}
}

void ImageView::setTexture(const Texture::Pointer& t, bool updateDescriptor)
{
	_texture = t;

	if (updateDescriptor)
		_descriptor.animate(ImageDescriptor(t), 0.0f);
	else
		invalidateContent();
}

void ImageView::setImage(const Image& img)
{
	_texture = img.texture;
	_descriptor.animate(img.descriptor, 0.0f);
}

void ImageView::setBackgroundColor(const vec4& color, float duration)
{
	if (duration == 0.0f)
	{
		_backgroundColor = color;
		invalidateContent();
	}
	else
	{
		_backgroundColorAnimator.animate(&_backgroundColor, _backgroundColor, color, duration);
	}
}

ImageDescriptor ImageView::calculateImageFrame()
{
	ImageDescriptor desc = _descriptor.value();

	switch (_contentMode)
	{
	case ContentMode_Center:
	{
		_actualImageSize = absv(desc.size);
		_actualImageOrigin = 0.5f * (size() - desc.size);
		break;
	}

	case ContentMode_Fit:
	case ContentMode_FitAnyway:
	case ContentMode_Fill:
	{
		vec2 frameSize = size();
		vec2 descSize = absv(desc.size);

		if (_contentMode == ContentMode_Fill)
		{
			vec2 sizeAspect = frameSize / descSize;
			float minScale = std::max(sizeAspect.x, sizeAspect.y);
			frameSize = descSize * minScale;
		}
		else
		{
			vec2 sizeAspect = frameSize / descSize;
			float minScale = std::min(sizeAspect.x, sizeAspect.y);

			if (_contentMode == ContentMode_Fit)
				minScale = std::min(minScale, 1.0f);

			frameSize = descSize * minScale;
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
	return _descriptor.value().size;
}
