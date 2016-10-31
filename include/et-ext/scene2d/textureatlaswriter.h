/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/baseclasses.h>

namespace et
{
namespace s2d
{
class TextureAtlasWriter
{
public:
	struct ImageItem
	{
		TextureDescription::Pointer image;
		s2d::ImageDescriptor place;

		ImageItem(TextureDescription::Pointer t, const s2d::ImageDescriptor& p) : 
			image(t), place(p) { }
	};

	typedef std::vector<ImageItem> ImageItemList;

	struct TextureAtlasItem
	{
		TextureDescription::Pointer texture;
		ImageItemList images;
		int maxWidth = 0;
		int maxHeight = 0;
	};

	typedef std::vector<TextureAtlasItem> TextureAtlasItemList;

public:
	TextureAtlasWriter(bool addSpace = true) :
		_addSpace(addSpace) { }
	
	TextureAtlasItem& addItem(const vec2i& textureSize);
	bool placeImage(TextureDescription::Pointer image, TextureAtlasItem& item);

	const TextureAtlasItemList& items() const 
		{ return _items; }

	void writeToFile(const std::string& fileName, const char* textureNamePattern = "texture_%d.png");

private:
	TextureAtlasItemList _items;
	bool _addSpace;
};
}
}
