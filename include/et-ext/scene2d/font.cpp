/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <stack>
#include <et/core/et.h>
#include <et/core/conversion.h>
#include <et/core/serialization.h>
#include <et/app/application.h>
#include <et/core/json.h>
#include <et/rendering/rendercontext.h>
#include <et/imaging/imagewriter.h>
#include <et-ext/scene2d/font.h>

namespace et
{
namespace s2d
{

size_t textLength(const wchar_t* text);
std::wstring subString(const wchar_t* begin, const wchar_t* end);
bool textBeginsFrom(const wchar_t* text, const wchar_t* entry);
const wchar_t* findClosingBracket(const wchar_t* text);
vec4 colorTagToColor(const std::wstring& colorTag);

Font::Font(const CharacterGenerator::Pointer& generator) :
	_generator(generator)
{
}

void Font::saveToFile(RenderInterface::Pointer& rc, const std::string& fileName)
{
	/*
	 * TODO : save to file
	 */
	std::ofstream fOut(fileName, std::ios::out);
	if (fOut.fail())
	{
		log::error("Unable save font to file %s", fileName.c_str());
		return;
	}

	auto textureFile = removeFileExt(getFileName(fileName)) + ".cache.png";

	Dictionary values;
	values.setStringForKey("face", _generator->face());
	values.setStringForKey("texture-file", textureFile);

	ArrayValue characters;

	const auto& chars = _generator->characters();
	for (const auto& c : chars)
	{
		Dictionary character;
		character.setIntegerForKey("value", c.second.value);
		character.setIntegerForKey("flags", c.second.flags);
		character.setArrayForKey("color", vec4ToArray(c.second.color));
		character.setArrayForKey("original-size", vec2ToArray(c.second.originalSize));
		character.setArrayForKey("parameters", vec4ToArray(c.second.parameters));
		character.setArrayForKey("content-rect", rectToArray(c.second.contentRect));
		character.setArrayForKey("uv-rect", rectToArray(c.second.uvRect));
		characters->content.push_back(character);
	}

	const auto& boldChars = _generator->boldCharacters();
	for (const auto& c : boldChars)
	{
		Dictionary character;
		character.setIntegerForKey("value", c.second.value);
		character.setIntegerForKey("flags", c.second.flags);
		character.setArrayForKey("color", vec4ToArray(c.second.color));
		character.setArrayForKey("original-size", vec2ToArray(c.second.originalSize));
		character.setArrayForKey("parameters", vec4ToArray(c.second.parameters));
		character.setArrayForKey("content-rect", rectToArray(c.second.contentRect));
		character.setArrayForKey("uv-rect", rectToArray(c.second.uvRect));
		characters->content.push_back(character);
	}

	values.setArrayForKey("characters", characters);

	fOut << json::serialize(values, json::SerializationFlag_ReadableFormat);
	fOut.flush();
	fOut.close();

	setCompressionLevelForImageFormat(ImageFormat_PNG, 1.0f / 9.0f);

	Texture::Pointer tex = _generator->texture();
	uint32_t dataSize = _generator->texture()->description().dataSizeForMipLevel(0);
	BinaryDataStorage imageData(tex->map(0, 0, Texture::MapOptions::Readable), dataSize);
	writeImageToFile(getFilePath(fileName) + textureFile, imageData, tex->size(0), 1, 8, ImageFormat_PNG, true);
	tex->unmap();
}

bool Font::loadFromDictionary(RenderInterface::Pointer& rc, const Dictionary& object, ObjectsCache& cache,
	const std::string& baseFileName)
{
	std::string fontFileDir = getFilePath(baseFileName);
	std::string textureFile = object.stringForKey("texture-file")->content;
	std::string textureFileName = fontFileDir + textureFile;
	std::string actualName = fileExists(textureFileName) ? textureFileName : textureFile;

	Texture::Pointer tex = rc->loadTexture(actualName, cache, [](TextureDescription::Pointer desc) {
		desc->flags |= Texture::Flags::Readback;
	});
	
	if (tex.invalid())
	{
		log::error("Unable to load texture for font %s. Missing file: %s", baseFileName.c_str(), textureFile.c_str());
		return false;
	}

	_generator->setTexture(tex);

	ArrayValue characters = object.arrayForKey("characters");
	for (Dictionary character : characters->content)
	{
		CharDescriptor desc;
		desc.value = character.integerForKey("value")->content & 0xffffffff;
		desc.flags = character.integerForKey("flags")->content & 0xffffffff;
		desc.color = arrayToVec4(character.arrayForKey("color"));
		desc.originalSize = arrayToVec2(character.arrayForKey("original-size"));
		desc.contentRect = arrayToRect(character.arrayForKey("content-rect"));
		desc.uvRect = arrayToRect(character.arrayForKey("uv-rect"));
		desc.parameters = arrayToVec4(character.arrayForKey("parameters"));
		_generator->pushCharacter(desc);
	}

	return false;
}

bool Font::loadFromFile(RenderInterface::Pointer& rc, const std::string& fileName, ObjectsCache& cache)
{
	std::string resolvedFileName = application().resolveFileName(fileName);

	auto loadedFile = loadTextFile(resolvedFileName);
	if (!loadedFile.empty())
	{
		VariantClass vc = VariantClass::Invalid;
		auto info = json::deserialize(loadedFile, vc, false);
		if (vc == VariantClass::Dictionary)
			return loadFromDictionary(rc, info, cache, resolvedFileName);
	}

	InputStream fontFile(resolvedFileName, StreamMode_Binary);

	if (fontFile.invalid())
		return false;

	std::string fontFileDir = getFilePath(resolvedFileName);

	uint32_t version = deserializeUInt32(fontFile.stream());
	ET_ASSERT(version >= FONT_VERSION_2);

	deserializeString(fontFile.stream());
	deserializeFloat(fontFile.stream());

	std::string textureFile = deserializeString(fontFile.stream());
	deserializeString(fontFile.stream()); // read layout file name, deprecated and not used anymore
	std::string textureFileName = fontFileDir + textureFile;
	std::string actualName = fileExists(textureFileName) ? textureFileName : textureFile;

	Texture::Pointer tex = rc->loadTexture(actualName, cache, [](TextureDescription::Pointer desc) {
		desc->flags |= Texture::Flags::Readback;
	});
	
	if (tex.invalid())
	{
		log::error("Unable to load texture for font %s. Missing file: %s", fileName.c_str(), textureFile.c_str());
		return false;
	}

	_generator->setTexture(tex);

	uint32_t charCount = deserializeUInt32(fontFile.stream());
	if (version == FONT_VERSION_2)
	{
		for (uint32_t i = 0; i < charCount; ++i)
		{
			CharDescriptor desc;
			desc.value = deserializeInt32(fontFile.stream());
			desc.flags = deserializeInt32(fontFile.stream());
			desc.color = deserializeVector<vec4>(fontFile.stream());
			desc.originalSize = deserializeVector<vec2>(fontFile.stream());
			desc.contentRect = deserializeVector<rectf>(fontFile.stream());
			desc.uvRect = deserializeVector<rectf>(fontFile.stream());
			desc.parameters = deserializeVector<vec4>(fontFile.stream());
			_generator->pushCharacter(desc);
		}
	}

	return true;
}

vec2 Font::measureStringSize(const CharDescriptorList& s)
{
	vec2 sz;
	vec2 lineSize;

	for (const auto& desc : s)
	{
		lineSize.y = std::max(lineSize.y, desc.originalSize.y);

		if ((desc.value == ET_RETURN) || (desc.value == ET_NEWLINE))
		{
			sz.x = std::max(sz.x, lineSize.x);
			sz.y += lineSize.y;
			lineSize = vec2(0.0f);
		}
		else
		{
			lineSize.x += desc.originalSize.x;
		}
	}

	sz.x = std::max(lineSize.x, sz.x);
	sz.y += lineSize.y;

	return sz;
}

vec2 Font::measureStringSize(const std::string& s, float size, float smoothing)
{
	return measureStringSize(buildString(utf8ToUnicode(s), size, smoothing));
}

vec2 Font::measureStringSize(const std::wstring& s, float size, float smoothing)
{
	return measureStringSize(buildString(s, size, smoothing));
}

CharDescriptorList Font::buildString(const std::string& s, float size, float smoothing)
{
	return buildString(utf8ToUnicode(s), size, smoothing);
}

CharDescriptorList Font::buildString(const std::wstring& s, float size, float smoothing)
{
	if (s.empty())
		return CharDescriptorList();

	float globalScale = size / CharacterGenerator::baseFontSize;

	CharDescriptorList result;
	result.reserve(s.size() + 1);

	static const wchar_t* boldTagStart = L"<b>";
	static const wchar_t* boldTagEnd = L"</b>";

	static const wchar_t* colorTagStart = L"<color=";
	static const wchar_t* colorTagEnd = L"</color>";

	static const wchar_t* scaleTagStart = L"<scale=";
	static const wchar_t* scaleTagEnd = L"</scale>";

	static const wchar_t* offsetTagStart = L"<offset=";
	static const wchar_t* offsetTagEnd = L"</offset>";

	size_t resultSize = 0;

	size_t boldTags = 0;
	std::vector<vec4> colorsStack;
	colorsStack.reserve(s.size() + 1);
	colorsStack.emplace_back(1.0f);

	std::vector<float> scaleStack;
	scaleStack.reserve(s.size() + 1);
	scaleStack.emplace_back(1.0f);
	
	std::vector<float> offsetStack;
	offsetStack.reserve(s.size() + 1);
	offsetStack.emplace_back(0.0f);

	auto* b = s.data();
	auto* e = b + s.size();
	while (b < e)
	{
		if (textBeginsFrom(b, boldTagStart))
		{
			++boldTags;
			b += textLength(boldTagStart);
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, boldTagEnd))
		{
			if (boldTags > 0)
				--boldTags;
			b += textLength(boldTagEnd);
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, colorTagStart))
		{
			auto closingBracket = findClosingBracket(b);
			colorsStack.emplace_back(colorTagToColor(subString(b, closingBracket)));
			b = closingBracket + 1;
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, colorTagEnd))
		{
			if (colorsStack.size() > 1)
				colorsStack.pop_back();

			b += textLength(colorTagEnd);
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, scaleTagStart))
		{
			auto closingBracket = findClosingBracket(b);
			scaleStack.emplace_back(std::wcstof(b + textLength(scaleTagStart), nullptr));
			b = closingBracket + 1;
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, scaleTagEnd))
		{
			if (scaleStack.size() > 1)
				scaleStack.pop_back();

			b += textLength(scaleTagEnd);
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, offsetTagStart))
		{
			auto closingBracket = findClosingBracket(b);
			offsetStack.emplace_back(std::wcstof(b + textLength(offsetTagStart), nullptr));
			b = closingBracket + 1;
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, offsetTagEnd))
		{
			if (offsetStack.size() > 1)
				offsetStack.pop_back();

			b += textLength(offsetTagEnd);
			if (b >= e)
				break;
		}
		else
		{
			result.emplace_back();
			CharDescriptor& cd = result.back();
			
			cd = (boldTags > 0) ? _generator->boldCharDescription(*b) : _generator->charDescription(*b);

			float localScale = scaleStack.back();
			float localOffset = offsetStack.back();
			float finalScale = globalScale * localScale;
			float smoothScale = 0.03f / std::pow(finalScale, 2.0f / 2.5f);

			cd.contentRect *= finalScale;
			cd.originalSize *= finalScale;
			cd.color = colorsStack.back();
			cd.parameters = vec4(0.5f, smoothing * smoothScale, 0.0f, 0.0f);
			cd.contentRect.top += localOffset;

			++resultSize;
			++b;
		}
	}

	return result;
}

/*
 * Service
 */
bool textBeginsFrom(const wchar_t* text, const wchar_t* entry)
{
	do
	{
		if (*entry != *text)
			return false;
		++text;
	}
	while (*(++entry));

	return true;
}

const wchar_t* findClosingBracket(const wchar_t* text)
{
	static const wchar_t closingBracket = L'>';
	do { if (*text == closingBracket) break; }
	while (*(++text));
	return text;
}

size_t textLength(const wchar_t* text)
{
	size_t result = 0;
	while (*text++)
		++result;
	return result;
}

std::wstring subString(const wchar_t* begin, const wchar_t* end)
{
	std::wstring result(end - begin + 1, 0);
	while (begin < end)
		result.push_back(*begin++);
	return result;
}

vec4 colorTagToColor(const std::wstring& colorTag)
{
	vec4 result(0.0f, 0.0f, 0.0f, 1.0f);

	std::wstring hex;
	for (auto i = colorTag.begin(), e = colorTag.end(); i != e; ++i)
	{
		auto l = tolower(*i);
		if (((l >= L'0') && (l <= L'9')) || ((l >= L'a') && (l <= L'f')))
			hex.push_back(*i);
	};

	int value = 0;
	uint32_t pos = 0;
	for (auto i = hex.rbegin(), e = hex.rend(); (i != e) && (pos <= 8); ++i, ++pos)
	{
		value += hexCharacterToInt(*i) * ((pos % 2 == 0) ? 1 : 16);

		if ((pos % 2) == 1)
		{
			result[pos / 2] = static_cast<float>(value) / 255.0f;
			value = 0;
		}
	}
	return result;
}
}
}
