/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#define ET_SHOULD_SAVE_GENERATED_CHARS 0

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/charactergenerator.h>

#if (ET_SHOULD_SAVE_GENERATED_CHARS)
#	include <et/app/application.h>
#	include <et/imaging/imagewriter.h>
#endif

using namespace et;
using namespace et::s2d;

enum GridProperies : int
{
	defaultTextureSize = 1024,
	baseFontIntegerSize = 192,
	initialGridDimensions = 2 * (baseFontIntegerSize + 2) * (baseFontIntegerSize + 2)
};

const float CharacterGenerator::baseFontSize = static_cast<float>(baseFontIntegerSize);
const vec2i CharacterGenerator::charactersRenderingExtent = vec2i(128);

void internal_sdf_compare(sdf::Grid& g, vec2i& p, int x, int y, int offsetx, int offsety);

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face, const std::string& boldFace,
	size_t faceIndex, size_t boldFaceIndex) : _impl(face, boldFace, faceIndex, boldFaceIndex), _rc(rc),
	_fontFace(face), _fontBoldFace(boldFace), _placer(vec2i(static_cast<int>(defaultTextureSize)), true)
{
	_fontFace = fileExists(face) ? getFileName(face) : face;
	_fontBoldFace = fileExists(boldFace) ? getFileName(boldFace) : boldFace;
	
	_texture = rc->textureFactory().genTexture(TextureTarget::Texture_2D, TextureFormat::R,
		vec2i(defaultTextureSize), TextureFormat::R, DataFormat::UnsignedChar,
		BinaryDataStorage(defaultTextureSize * defaultTextureSize, 0), face + "font");
	
	_grid0.grid.resize(initialGridDimensions);
	_grid1.grid.resize(initialGridDimensions);
}

bool CharacterGenerator::performCropping(const BinaryDataStorage& renderedCharacterData, const vec2i& canvasSize,
	BinaryDataStorage& dataToSave, vec2i& sizeToSave, vec2i& topLeftOffset)
{
	topLeftOffset = canvasSize - vec2i(1);
	vec2i bottomRightOffset = vec2i(0);
	
	vec2i pixel;
	for (pixel.y = 0; pixel.y < canvasSize.y - 1; ++pixel.y)
	{
		for (pixel.x = 0; pixel.x < canvasSize.x - 1; ++pixel.x)
		{
			uint32_t i = pixel.x + (canvasSize.y - pixel.y - 1) * canvasSize.x;
			if (renderedCharacterData[i])
			{
				topLeftOffset = minv(topLeftOffset, pixel);
				bottomRightOffset = maxv(bottomRightOffset, pixel);
			}
		}
	}
	bottomRightOffset = minv(bottomRightOffset + vec2i(1), canvasSize);
	sizeToSave = bottomRightOffset - topLeftOffset;

	if ((sizeToSave.x <= 0) || (sizeToSave.y <= 0))
		return false;

	vec2i additional;
	additional.x = sizeToSave.x % 4;
	additional.y = sizeToSave.y % 4;
	sizeToSave += additional;

	dataToSave.resize(sizeToSave.square());
	dataToSave.fill(0);
	
	vec2i targetPixel(0, additional.y / 2);
	for (int py = topLeftOffset.y; py < bottomRightOffset.y; ++py, ++targetPixel.y)
	{
		targetPixel.x = additional.x / 2;
		for (int px = topLeftOffset.x; px < bottomRightOffset.x; ++px, ++targetPixel.x)
		{
			uint32_t i = targetPixel.x + (sizeToSave.y - targetPixel.y - 1) * sizeToSave.x;
			uint32_t j = px + (canvasSize.y - py - 1) * canvasSize.x;
			dataToSave[i] = renderedCharacterData[j];
		}
	}
	
	return true;
}

CharDescriptor CharacterGenerator::generateCharacter(int value, CharacterFlags flags)
{
	CharDescriptor result(value);
	result.flags = flags;
	
	vec2i charSize;
	vec2i canvasSize;
	vec2i topLeftOffset;
	BinaryDataStorage renderedCharacterData;
	
	if (_impl.processCharacter(result, charSize, canvasSize, renderedCharacterData))
	{
		generateSignedDistanceField(renderedCharacterData, canvasSize.x, canvasSize.y);

		vec2i sizeToSave;
		BinaryDataStorage dataToSave;
		
		if (performCropping(renderedCharacterData, canvasSize, dataToSave, sizeToSave, topLeftOffset))
		{
		#if (ET_SHOULD_SAVE_GENERATED_CHARS)
			auto path = application().environment().applicationDocumentsFolder() + "char_" + intToStr(value) + ".png";
			writeImageToFile(path, dataToSave, sizeToSave, 1, 8, et::ImageFormat::ImageFormat_PNG, true);
		#endif

			vec2i downsampledSize = sizeToSave / 2;
			auto downsampled = downsample(dataToSave, sizeToSave);
			downsampled = downsample(downsampled, downsampledSize);
			downsampledSize /= 2;

			recti textureRect;
			if (_placer.place(downsampledSize, textureRect))
			{
				updateTexture(textureRect.origin(), downsampledSize, downsampled);
				
				result.contentRect = rectf(vector2ToFloat(topLeftOffset - charactersRenderingExtent / 2), vector2ToFloat(sizeToSave));
				result.uvRect = rectf(_texture->getTexCoord(vector2ToFloat(textureRect.origin())),
					vector2ToFloat(textureRect.size()) / _texture->sizeFloat());
			}
			else
			{
				log::error("Failed to place character to the texture: %d (%c, %C)", result.value,
					static_cast<char>(result.value), static_cast<wchar_t>(result.value));
			}
		}
		
		result.originalSize = vector2ToFloat(charSize);
	}
	
	CharDescriptorMap& mapToInsert = ((flags & CharacterFlag_Bold) == CharacterFlag_Bold) ? _boldChars : _chars;
	mapToInsert.emplace(value, result);
					   
	characterGenerated.invoke(value);
	
	return result;
}

void CharacterGenerator::updateTexture(const vec2i& position, const vec2i& size, BinaryDataStorage& data)
{
	vec2i dest(position.x, _texture->size().y - position.y - size.y - 1);
	_texture->updatePartialDataDirectly(_rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGenerator::setTexture(Texture::Pointer tex)
{
	_texture = tex;
}

void CharacterGenerator::pushCharacter(const et::s2d::CharDescriptor& desc)
{
	CharDescriptorMap& mapToInsert = ((desc.flags & CharacterFlag_Bold) == CharacterFlag_Bold) ? _boldChars : _chars;
	mapToInsert.insert(std::make_pair(desc.value, desc));
	
	vec2 size = _texture->sizeFloat() * desc.uvRect.size();
	vec2 origin = _texture->sizeFloat() * vec2(desc.uvRect.origin().x, 1.0f - desc.uvRect.origin().y);
	
	_placer.addPlacedRect(recti(static_cast<int>(origin.x), static_cast<int>(origin.y),
		static_cast<int>(size.x), static_cast<int>(size.y)));
}

BinaryDataStorage CharacterGenerator::downsample(BinaryDataStorage& input, const vec2i& size)
{
	vec2i downsampledSize = size / 2;
	BinaryDataStorage result(downsampledSize.square(), 0);
	
	uint32_t k = 0;
	for (int y = 0; y < downsampledSize.y; ++y)
	{
		int thisRow = 2 * y;
		int nextRow = std::min(thisRow + 1, size.y - 1);
		for (int x = 0; x < downsampledSize.x; ++x)
		{
			int thisCol = 2 * x;
			int nextCol = std::min(thisCol + 1, size.x - 1);
			int in00 = thisRow * size.x + thisCol;
			int in01 = thisRow * size.x + nextCol;
			int in10 = nextRow * size.x + thisCol;
			int in11 = nextRow * size.x + nextCol;
			result[k++] = (input[in00] + input[in01] + input[in10] + input[in11]) / 4;
		}
	}
	
	return result;
}

inline const vec2i& sdf_get(sdf::Grid& g, int x, int y)
{
	ET_ASSERT((x >= 0) && (y >= 0) & (x < g.w) && (y < g.h));
	return g.grid[y * g.w + x];
}

inline void sdf_put(sdf::Grid& g, int x, int y, const vec2i& p)
{
	ET_ASSERT((x >= 0) && (y >= 0) & (x < g.w) && (y < g.h));
	g.grid[y * g.w + x] = p;
}

inline void internal_sdf_compare(sdf::Grid& g, vec2i& p, int x, int y, int offsetx, int offsety)
{
	vec2i other = sdf_get(g, x + offsetx, y + offsety) + vec2i(offsetx, offsety);
	if (other.dotSelf() < p.dotSelf())
		p = other;
}

void CharacterGenerator::generateSignedDistanceFieldOnGrid(sdf::Grid &g)
{
	for (int y = 1; y < g.h - 1; ++y)
	{
		for (int x = 1; x < g.w - 1; ++x)
		{
			vec2i p = sdf_get(g, x, y);
			internal_sdf_compare(g, p, x, y, -1,  0);
			internal_sdf_compare(g, p, x, y,  0, -1);
			internal_sdf_compare(g, p, x, y, -1, -1);
			internal_sdf_compare(g, p, x, y,  1, -1);
			sdf_put(g, x, y, p);
		}
		for (int x = g.w - 2; x > 0; --x)
		{
			vec2i p = sdf_get(g, x, y);
			internal_sdf_compare(g, p, x, y, 1, 0);
			sdf_put(g, x, y, p);
		}
	}

	for (int y = g.h - 2; y > 0; --y)
	{
		for (int x = g.w - 2; x > 0; --x)
		{
			vec2i p = sdf_get(g, x, y);
			internal_sdf_compare(g, p, x, y,  1,  0);
			internal_sdf_compare(g, p, x, y,  0,  1);
			internal_sdf_compare(g, p, x, y, -1,  1);
			internal_sdf_compare(g, p, x, y,  1,  1);
			sdf_put(g, x, y, p);
		}
		for (int x = 1; x < g.w - 1; ++x)
		{
			vec2i p = sdf_get(g, x, y);
			internal_sdf_compare(g, p, x, y, -1, 0);
			sdf_put(g, x, y, p);
		}
	}
}

void CharacterGenerator::generateSignedDistanceField(BinaryDataStorage& data, int w, int h)
{
	const vec2i pointOutside = vec2i(255);

	size_t targetGridSize = w * h;
	
	if (_grid0.grid.size() < targetGridSize)
		_grid0.grid.resize(targetGridSize);
	
	if (_grid1.grid.size() < targetGridSize)
		_grid1.grid.resize(targetGridSize);
	
	_grid0.w = w;
	_grid1.w = w;
	_grid0.h = h;
	_grid1.h = h;
	
	_grid0.grid.fill(0);
	_grid1.grid.fill(0);

	for (int y = 0; y < _grid0.h; ++y)
	{
		sdf_put(_grid1, 0, y, pointOutside);
		sdf_put(_grid1, _grid0.w - 1, y, pointOutside);
	}

	for (int x = 0; x < _grid0.w; ++x)
	{
		sdf_put(_grid1, x, 0, pointOutside);
		sdf_put(_grid1, x, _grid0.h - 1, pointOutside);
	}

	for (int y = 1; y < h - 1; ++y)
	{
		for (int x = 1; x < w - 1; ++x)
		{
			auto& targetGrid = (data[x + y * _grid0.w] == 0) ? _grid1 : _grid0;
			sdf_put(targetGrid, x, y, pointOutside);
		}
	}
	
	generateSignedDistanceFieldOnGrid(_grid0);
	generateSignedDistanceFieldOnGrid(_grid1);
	
	for (int y = 1; y < h - 1; ++y)
	{
		for (int x = 1; x < w - 1; ++x)
		{
			float p1 = static_cast<float>(sdf_get(_grid0, x, y).dotSelf());
			float p2 = static_cast<float>(sdf_get(_grid1, x, y).dotSelf());
			float value = 3.0f * (std::sqrt(p1) - std::sqrt(p2)) + 128.0f;
			data[x + y * _grid0.w] = static_cast<unsigned char>(clamp(value, 0.0f, 255.0f));
		}
	}
}
