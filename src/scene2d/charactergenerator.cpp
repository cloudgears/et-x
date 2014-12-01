/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/charactergenerator.h>

#define ET_SAVE_FONT_TO_FILE	0

#if (ET_SAVE_FONT_TO_FILE)
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
const vec2i CharacterGenerator::charactersRenderingExtent = vec2i(64);

void internal_sdf_compare(sdf::Grid& g, sdf::Point& p, int x, int y, int offsetx, int offsety);

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face, const std::string& boldFace,
	size_t faceIndex, size_t boldFaceIndex) : _impl(face, boldFace, faceIndex, boldFaceIndex), _rc(rc),
	_placer(vec2i(static_cast<int>(defaultTextureSize)), true), _fontFace(face), _fontBoldFace(boldFace)
{
	_fontFace = fileExists(face) ? getFileName(face) : face;
	_fontBoldFace = fileExists(boldFace) ? getFileName(boldFace) : boldFace;
	
#if defined(GL_R8)
	auto internalFormat = GL_R8;
#else
	auto internalFormat = GL_RED;
#endif
	
	_texture = rc->textureFactory().genTexture(GL_TEXTURE_2D, internalFormat, vec2i(defaultTextureSize),
		GL_RED, GL_UNSIGNED_BYTE, BinaryDataStorage(defaultTextureSize * defaultTextureSize, 0), face + "font");
	
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
			size_t i = pixel.x + (canvasSize.y - pixel.y - 1) * canvasSize.x;
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
	
	dataToSave.resize(sizeToSave.square());
	
	vec2i targetPixel;
	for (int py = topLeftOffset.y; py < bottomRightOffset.y; ++py, ++targetPixel.y)
	{
		targetPixel.x = 0;
		for (int px = topLeftOffset.x; px < bottomRightOffset.x; ++px, ++targetPixel.x)
		{
			size_t i = targetPixel.x + (sizeToSave.y - targetPixel.y - 1) * sizeToSave.x;
			size_t j = px + (canvasSize.y - py - 1) * canvasSize.x;
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

#	if (ET_SAVE_FONT_TO_FILE) && (ET_PLATFORM_WIN || ET_PLATFORM_MAC)
		{
			auto outputFile = application().environment().applicationDocumentsFolder() +
				_fontFace + " - " + intToStr(value) + " - original.png";
			ImageWriter::writeImageToFile(outputFile, renderedCharacterData, canvasSize, 1, 8, ImageFormat_PNG, true);
		}
#	endif

		generateSignedDistanceField(renderedCharacterData, canvasSize.x, canvasSize.y);
		
#	if (ET_SAVE_FONT_TO_FILE) && (ET_PLATFORM_WIN || ET_PLATFORM_MAC)
		{
			auto outputFile = application().environment().applicationDocumentsFolder() +
				_fontFace + " - " + intToStr(value) + " - sdf.png";
			ImageWriter::writeImageToFile(outputFile, renderedCharacterData, canvasSize, 1, 8, ImageFormat_PNG, true);
		}
#	endif

		vec2i sizeToSave;
		BinaryDataStorage dataToSave;
		
		if (performCropping(renderedCharacterData, canvasSize, dataToSave, sizeToSave, topLeftOffset))
		{

#		if (ET_SAVE_FONT_TO_FILE) && (ET_PLATFORM_WIN || ET_PLATFORM_MAC)
			{
				auto outputFile = application().environment().applicationDocumentsFolder() +
					_fontFace + " - " + intToStr(value) + " - crop.png";
				ImageWriter::writeImageToFile(outputFile, dataToSave, sizeToSave, 1, 8, ImageFormat_PNG, true);
			}
#		endif

			vec2i downsampledSize = sizeToSave / 2;
			auto downsampled = downsample(dataToSave, sizeToSave);
			downsampled = downsample(downsampled, downsampledSize);
			downsampledSize /= 2;
			
#		if (ET_SAVE_FONT_TO_FILE) && (ET_PLATFORM_WIN || ET_PLATFORM_MAC)
			{
				auto outputFile = application().environment().applicationDocumentsFolder() +
					_fontFace + " - " + intToStr(value) + " - downsampled.png";
				ImageWriter::writeImageToFile(outputFile, downsampled, downsampledSize, 1, 8, ImageFormat_PNG, true);
			}
#		endif

			recti textureRect;
			if (_placer.place(downsampledSize, textureRect))
			{
				updateTexture(textureRect.origin(), downsampledSize, downsampled);
				
#			if (ET_SAVE_FONT_TO_FILE) && (ET_PLATFORM_WIN || ET_PLATFORM_MAC)
				{
					_rc->renderState().bindTexture(0, _texture);
					BinaryDataStorage data(4 * _texture->size().square(), 0);
					glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
					checkOpenGLError("glGetTexImage")
					auto outputFile = application().environment().applicationDocumentsFolder() + _fontFace + ".png";
					ImageWriter::writeImageToFile(outputFile, data, _texture->size(), 4, 8, ImageFormat_PNG, true);
				}
#			endif
				
				result.contentRect = rect(vector2ToFloat(topLeftOffset - charactersRenderingExtent / 2), vector2ToFloat(sizeToSave));
				result.uvRect = rect(_texture->getTexCoord(vector2ToFloat(textureRect.origin())),
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
	mapToInsert.insert(std::make_pair(value, result));
					   
	characterGenerated.invoke(value);
	
	return result;
}

void CharacterGenerator::updateTexture(const vec2i& position, const vec2i& size, BinaryDataStorage& data)
{
	vec2i dest(position.x, _texture->size().y - position.y - size.y - 1);
	_texture->updatePartialDataDirectly(_rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGenerator::setTexture(Texture tex)
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
	
	size_t k = 0;
	for (int y = 0; y < downsampledSize.y; ++y)
	{
		int thisRow = 2 * y;
		int nextRow = etMin(thisRow + 1, size.y - 1);
		for (int x = 0; x < downsampledSize.x; ++x)
		{
			int thisCol = 2 * x;
			int nextCol = etMin(thisCol + 1, size.x - 1);
			int in00 = thisRow * size.x + thisCol;
			int in01 = thisRow * size.x + nextCol;
			int in10 = nextRow * size.x + thisCol;
			int in11 = nextRow * size.x + nextCol;
			result[k++] = (input[in00] + input[in01] + input[in10] + input[in11]) / 4;
		}
	}
	
	return result;
}

#define sdf_get(g, x, y)		g.grid[(y) * (2 + g.w) + (x)]
#define sdf_put(g, x, y, p)		g.grid[(y) * (2 + g.w) + (x)] = p;

inline void internal_sdf_compare(sdf::Grid& g, sdf::Point& p, int x, int y, int offsetx, int offsety)
{
	int add = 0;
	
	sdf::Point other = sdf_get(g, x + offsetx, y + offsety);
	
	if (offsety == 0)
	{
		add = 2 * other.dx + 1;
	}
	else if (offsetx == 0)
	{
		add = 2 * other.dy + 1;
	}
	else
	{
		add = 2 * (other.dy + other.dx + 1);
	}
	
	other.f += add;
	
	if (other.f < p.f)
	{
		p.f = other.f;
		
		if (offsety == 0)
		{
			p.dx = other.dx + 1;
			p.dy = other.dy;
		}
		else if (offsetx == 0)
		{
			p.dy = other.dy + 1;
			p.dx = other.dx;
		}
		else
		{
			p.dy = other.dy + 1;
			p.dx = other.dx + 1;
		}
	}
}

void CharacterGenerator::generateSignedDistanceFieldOnGrid(sdf::Grid &g)
{
	for (int y = 1; y <= g.h; y++)
	{
		for (int x = 1; x <= g.w; x++)
		{
			sdf::Point p = sdf_get(g, x, y);
			internal_sdf_compare(g, p, x, y, -1,  0);
			internal_sdf_compare(g, p, x, y, 0, -1);
			internal_sdf_compare(g, p, x, y,  -1, -1);
			internal_sdf_compare(g, p, x, y,   1, -1);
			sdf_put(g, x, y, p);
		}
	}
	
	for(int y = g.h; y > 0; y--)
	{
		for(int x = g.w; x > 0; x--)
		{
			sdf::Point p = sdf_get(g, x, y);
			internal_sdf_compare(g, p, x, y,   1, 0);
			internal_sdf_compare(g, p, x, y,   0, 1);
			internal_sdf_compare(g, p, x, y,  -1, 1);
			internal_sdf_compare(g, p, x, y,   1, 1);
			sdf_put(g, x, y, p);
		}
	}
}

void CharacterGenerator::generateSignedDistanceField(BinaryDataStorage& data, int w, int h)
{
	static const sdf::Point pointInside = { 0, 0, 255 };
		 
	size_t targetGridSize = (w + 2) * (h + 2);
	
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
	
	for (int x = 0; x < w + 2; x++)
	{
		sdf_put(_grid0, x, 0, pointInside);
		sdf_put(_grid1, x, 0, pointInside);
		sdf_put(_grid0, x, h + 1, pointInside);
		sdf_put(_grid1, x, h + 1, pointInside);
	}
	
	for (int y = 1; y <= h; y++)
	{
		sdf_put(_grid0, 0, y, pointInside);
		sdf_put(_grid1, 0, y, pointInside);
		sdf_put(_grid0, w + 1, y, pointInside);
		sdf_put(_grid1, w + 1, y, pointInside);
	}
	
	size_t k = 0;
	for (int y = 1; y <= h; y++)
	{
		for (int x = 1; x <= w; x++)
		{
			auto val = data[k++];
			sdf_put(_grid0, x, y, sdf::Point(val, val, val));
			sdf_put(_grid1, x, y, sdf::Point(0, 0, 255 - val));
		}
	}
	
	generateSignedDistanceFieldOnGrid(_grid0);
	generateSignedDistanceFieldOnGrid(_grid1);
	
	k = 0;
	
	DataStorage<float> distances(w * h, 0);
	for (int y = 1; y <= h; y++)
	{
		for (int x = 1; x <= w; x++)
		{
			float dist1 = std::sqrt(static_cast<float>(sdf_get(_grid0, x, y).f + 1));
			float dist2 = std::sqrt(static_cast<float>(sdf_get(_grid1, x, y).f + 1));
			distances[k++] = 127.0f + 8.4666666666666668f * (dist1 - dist2);
		}
	}
	
	DataStorage<float> smooth(w * h, 0);
	for (int i = 0; i < 2; ++i)
	{
		int row = 0;
		
		for (int y = 0; y < h; y++, row += w)
		{
			for (int x = 0; x < w; x++)
			{
				int index = x + row;
				int prev = etMax(0, x - 1) + row;
				int next = etMin(w - 1, x + 1) + row;
				smooth[index] = (distances[prev] + distances[index] + distances[next]) / 3.0f;
			}
		}
		
		for (int x = 0; x < w; x++)
		{
			for (int y = 0; y < h; y++)
			{
				int index = x + w * y;
				int prev = x + w * etMax(0, y-1);
				int next = x + w * etMin(h-1, y+1);
				distances[index] = (smooth[prev] + smooth[index] + smooth[next]) / 3.0f;
			}
		}
	}
	
	k = 0;
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			data[k] = static_cast<unsigned char>(clamp(distances[k], 0.0f, 255.0f));
			++k;
		}
	}
}
