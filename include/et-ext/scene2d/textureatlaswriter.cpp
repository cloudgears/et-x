/*
* This file is part of `et engine`
* Copyright 2009-2015 by Sergey Reznik
* Please, modify content only if you know what are you doing.
*
*/

#include <fstream>
#include <et/core/json.h>
#include <et/core/conversion.h>
#include <et/imaging/pngloader.h>
#include <et/imaging/imagewriter.h>
#include <et/imaging/imageoperations.h>
#include <et-ext/scene2d/textureatlaswriter.h>

namespace et
{
namespace s2d
{

const int defaultSpacing = 1;

TextureAtlasWriter::TextureAtlasItem& TextureAtlasWriter::addItem(const vec2i& textureSize)
{
	_items.push_back(TextureAtlasItem());

	TextureAtlasItem& item = _items.back();
	item.texture = et::TextureDescription::Pointer::create();
	item.texture->size = textureSize;

	return item;
}

bool TextureAtlasWriter::placeImage(TextureDescription::Pointer image, TextureAtlasItem& item)
{
	vec2i sz = image->size;
	vec2i offset;

	if (_addSpace)
	{
		if (sz.x + defaultSpacing < item.texture->size.x)
		{
			sz.x += defaultSpacing;
			offset.x = defaultSpacing;
		}

		if (sz.y + defaultSpacing < item.texture->size.y)
		{
			sz.y += defaultSpacing;
			offset.y = defaultSpacing;
		}
	}
	ImageDescriptor desc(vec2(0.0f), vector2ToFloat(sz));

	if (item.images.size() == 0)
	{
		item.images.push_back(ImageItem(image, desc));

		if (desc.origin.x + desc.size.x > item.maxWidth)
			item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - offset.x;

		if (desc.origin.y + desc.size.y > item.maxHeight)
			item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - offset.y;

		return true;
	}

	for (ImageItemList::iterator i = item.images.begin(), e = item.images.end(); i != e; ++i)
	{
		desc.origin = i->place.origin + vec2(i->place.size.x, 0.0f);

		bool placed = (desc.origin.x + sz.x <= item.texture->size.x) && (desc.origin.y + sz.y <= item.texture->size.y);
		if (placed)
		{
			for (ImageItemList::iterator ii = item.images.begin(); ii != e; ++ii)
			{
				if ((ii != i) && ii->place.rectangle().intersects(desc.rectangle()))
				{
					placed = false;
					break;
				}
			}
		}

		if (placed)
		{
			item.images.push_back(ImageItem(image, desc));

			if (desc.origin.x + desc.size.x > item.maxWidth)
				item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - offset.x;

			if (desc.origin.y + desc.size.y > item.maxHeight)
				item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - offset.y;

			return true;
		}
	}

	for (ImageItemList::iterator i = item.images.begin(), e = item.images.end(); i != e; ++i)
	{
		desc.origin = i->place.origin + vec2(i->place.size.x, 0.0f);
		desc.origin = i->place.origin + vec2(0.0f, i->place.size.y);

		bool placed = (desc.origin.x + sz.x <= item.texture->size.x) && (desc.origin.y + sz.y <= item.texture->size.y);
		if (placed)
		{
			for (ImageItemList::iterator ii = item.images.begin(); ii != e; ++ii)
			{
				if ((ii != i) && ii->place.rectangle().intersects(desc.rectangle()))
				{
					placed = false;
					break;
				}
			}
		}

		if (placed)
		{
			item.images.push_back(ImageItem(image, desc));

			if (desc.origin.x + desc.size.x > item.maxWidth)
				item.maxWidth = static_cast<int>(desc.origin.x + desc.size.x) - offset.x;

			if (desc.origin.y + desc.size.y > item.maxHeight)
				item.maxHeight = static_cast<int>(desc.origin.y + desc.size.y) - offset.y;

			return true;
		}
	}

	return false;
}

void TextureAtlasWriter::writeToFile(const std::string& fileName, const char* textureNamePattern)
{
	vec2 spacing = _addSpace ? vector2ToFloat(vec2i(defaultSpacing)) : vec2(0.0f);

	std::string path = addTrailingSlash(getFilePath(fileName));
	ArrayValue textures;
	ArrayValue images;

	int textureIndex = 0;
	for (TextureAtlasItemList::iterator i = _items.begin(), e = _items.end(); i != e; ++i, ++textureIndex)
	{
		BinaryDataStorage data(i->texture->size.square() * 4);
		data.fill(0);

		char textureName[1024] = {};
		sprintf(textureName, textureNamePattern, textureIndex);
		std::string texName = path + std::string(textureName);
		std::string texId = removeFileExt(textureName);

		Dictionary texture;
		texture.setStringForKey("filename", textureName);
		texture.setStringForKey("id", texId);
		textures->content.push_back(texture);

		int index = 0;
		for (const auto& ii : i->images)
		{
			TextureDescription image;
			png::loadFromFile(ii.image->origin(), image, true);

			std::string sIndex = intToStr(index);

			if (sIndex.length() < 2)
				sIndex = "0" + sIndex;

			std::string name = removeFileExt(getFileName(ii.image->origin()));

			vec4 offset;
			size_t delimPos = name.find_first_of("~");
			if (delimPos != std::string::npos)
			{
				std::string params = name.substr(delimPos + 1);
				name.erase(delimPos);
				while (params.length())
				{
					size_t dPos = params.find_first_of("-");
					if (dPos == std::string::npos) break;

					std::string token = params.substr(0, dPos + 1);
					params.erase(0, dPos + 1);

					if (token == "offset-")
					{
						offset = strToVector4(params);
						for (uint32_t q = 0; q < 4; ++q)
						{
							if (offset[q] < 1.0f)
							{
								offset[q] *= (q % 2 == 0) ? static_cast<float>(image.size.x) :
									static_cast<float>(image.size.y);
							}
						}
					}
					else
					{
						log::warning("Unrecognized token: %s", token.c_str());
						break;
					}
				}
			}

			Dictionary imageDictionary;
			imageDictionary.setStringForKey("name", name);
			imageDictionary.setStringForKey("texture", texId);
			imageDictionary.setArrayForKey("rect", rectToArray(rectf(ii.place.origin, ii.place.size - spacing)));
			imageDictionary.setArrayForKey("offset", vec4ToArray(offset));
			images->content.push_back(imageDictionary);

			if (image.format == TextureFormat::RGBA8)
			{
				ImageOperations::transfer(image.data, image.size, 4, data, i->texture->size, 4,
					vec2i(static_cast<int>(ii.place.origin.x), static_cast<int>(ii.place.origin.y)));
			}
			else
			{
				ET_FAIL("Unsupported image format!");
			}

			++index;
		}

		writeImageToFile(texName, data, i->texture->size, 4, 8, ImageFormat_PNG, true);
	}

	Dictionary output;
	output.setArrayForKey("textures", textures);
	output.setArrayForKey("images", images);

	auto serialized = json::serialize(output, json::SerializationFlag_ReadableFormat);
	std::ofstream fOut(fileName, std::ios::out);
	if (fOut.good())
	{
		fOut << serialized;
		fOut.close();
	}
	else
	{
		log::error("Unable to create output file: %s\nOutputting result to console:", fileName.c_str());
		output.printContent();
	}
}

}
}
