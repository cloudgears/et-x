/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <fstream>
#include <et/core/conversion.h>
#include <et/core/serialization.h>
#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/imaging/imagewriter.h>
#include <et-ext/scene2d/font.h>

using namespace et;
using namespace et::s2d;

size_t textLength(const wchar_t* text);
std::wstring subString(const wchar_t* begin, const wchar_t* end);
bool textBeginsFrom(const wchar_t* text, const wchar_t* entry);
const wchar_t* findClosingBracket(const wchar_t* text);
vec4 colorTagToColor(const std::wstring& colorTag);

Font::Font(const CharacterGenerator::Pointer& generator) :
	_generator(generator) { }

void Font::saveToFile(RenderContext* rc, const std::string& fileName)
{
	std::ofstream fOut(fileName, std::ios::out | std::ios::binary);
	if (fOut.fail())
	{
		log::error("Unable save font to file %s", fileName.c_str());
		return;
	}
	
	serializeInt(fOut, FONT_VERSION_CURRENT);
	serializeString(fOut, _generator->face());
	serializeInt(fOut, _generator->size());
	
	std::string textureFile = removeFileExt(getFileName(fileName)) + ".cache.png";
	std::string layoutFile = removeFileExt(getFileName(fileName)) + ".layout.png";
	
	serializeString(fOut, textureFile);
	serializeString(fOut, layoutFile);
	
	int charCount = static_cast<int>(_generator->charactersCount());
	serializeInt(fOut, charCount);
	
	const auto& chars = _generator->characters();
	const auto& boldChars = _generator->boldCharacters();
	
	for (const auto& c : chars)
		fOut.write(reinterpret_cast<const char*>(&c.second), sizeof(c.second));

	for (const auto& c : boldChars)
		fOut.write(reinterpret_cast<const char*>(&c.second), sizeof(c.second));
	
	fOut.flush();
	fOut.close();
	
#if (ET_PLATFORM_MAC || ET_PLATFORM_WIN)
	
	BinaryDataStorage imageData(texture()->size().square());
	
	rc->renderState().bindTexture(0, texture());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, imageData.data());
	checkOpenGLError("glGetTexImage");
	
	ImageWriter::writeImageToFile(getFilePath(fileName) + textureFile, imageData,
		texture()->size(), 1, 8, ImageFormat_PNG, true);
	
#elif (ET_PLATFORM_IOS || ET_PLATFORM_ANDROID)
	
	auto fbo = rc->framebufferFactory().createFramebuffer(texture()->size(), "temp-buffer",
		GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, 0, 0, 0);
	
	bool blendEnabled = rc->renderState().blendEnabled();
	auto currentBuffer = rc->renderState().boundFramebuffer();
	
	rc->renderState().bindFramebuffer(fbo);
	rc->renderState().setBlend(false, BlendState_Current);
	rc->renderer()->renderFullscreenTexture(texture());
	
	BinaryDataStorage imageData(4 * texture()->size().square());
	glReadPixels(0, 0, fbo->size().x, fbo->size().y, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
	checkOpenGLError("glReadPixels");
	
	rc->renderState().bindFramebuffer(currentBuffer);
	rc->renderState().setBlend(blendEnabled, BlendState_Current);
	
	fbo.reset(nullptr);
	
	auto ptr = imageData.begin();
	auto off = imageData.begin();
	auto end = imageData.end();
	while (off != end)
	{
		*ptr++ = *off;
		off += 4;
	}
	
	ImageWriter::writeImageToFile(getFilePath(fileName) + textureFile, imageData,
		texture()->size(), 1, 8, ImageFormat_PNG, true);
	
#endif
}

void Font::loadFromFile(RenderContext* rc, const std::string& fileName, ObjectsCache& cache)
{
	std::string resolvedFileName = application().resolveFileName(fileName);
	
	InputStream fontFile(resolvedFileName, StreamMode_Binary);
	if (fontFile.invalid()) return;

	std::string fontFileDir = getFilePath(resolvedFileName);

	int version = deserializeInt(fontFile.stream());
	ET_ASSERT(version >= FONT_VERSION_2)
	
	deserializeString(fontFile.stream());
	deserializeInt(fontFile.stream());
	
	std::string textureFile = deserializeString(fontFile.stream());
	std::string layoutFile = deserializeString(fontFile.stream());
	std::string textureFileName = fontFileDir + textureFile;
	std::string actualName = fileExists(textureFileName) ? textureFileName : textureFile;

	Texture tex = rc->textureFactory().loadTexture(actualName, cache);
	if (tex.invalid())
	{
		log::error("Unable to load texture for font %s. Missing file: %s", fileName.c_str(), textureFile.c_str());
		return;
	}
	
	_generator->setTexture(tex);
	
	int charCount = deserializeInt(fontFile.stream());
	if (version == FONT_VERSION_2)
	{
		for (int i = 0; i < charCount; ++i)
		{
			CharDescriptor desc;
			fontFile.stream().read(reinterpret_cast<char*>(&desc), sizeof(desc));
			_generator->pushCharacter(desc);
		}
	}
}

vec2 Font::measureStringSize(const CharDescriptorList& s)
{
	vec2 sz;
	vec2 lineSize;

	for (auto& desc : s)
	{
		lineSize.y = etMax(lineSize.y, desc.pixelsSize.y + desc.padding.y);
		
		if ((desc.value == ET_RETURN) || (desc.value == ET_NEWLINE))
		{
			sz.x = etMax(sz.x, lineSize.x);
			sz.y += lineSize.y;
			lineSize = vec2(0.0f);
		}
		else 
		{
			lineSize.x += desc.padding.x + desc.pixelsSize.x;
		}
	}
	
	sz.x = etMax(lineSize.x, sz.x);
	sz.y += lineSize.y;
	
	return sz;
}

vec2 Font::measureStringSize(const std::string& s, bool formatted)
{
	if (isUtf8String(s))
	{
		std::wstring ws = utf8ToUnicode(s);
		return measureStringSize(formatted ? parseString(ws) : buildString(ws, formatted));
	}
	
	return measureStringSize(formatted ? parseString(s) : buildString(s, formatted));
}

vec2 Font::measureStringSize(const std::wstring& s, bool formatted)
{
	return measureStringSize(formatted ? parseString(s) : buildString(s, formatted));
}

CharDescriptorList Font::buildString(const std::string& s, bool formatted)
{
	if (isUtf8String(s))
		return buildString(utf8ToUnicode(s), formatted);

	if (formatted)
		return parseString(s);

	CharDescriptorList result;
	
	for (const auto& i : s)
		result.push_back(charDescription(i));
	
	return result;
}

CharDescriptorList Font::buildString(const std::wstring& s, bool formatted)
{
	if (formatted)
		return parseString(s);

	CharDescriptorList result;
	for (const auto& i : s)
		result.push_back(charDescription(i));
	return result;
}

CharDescriptorList Font::parseString(const std::string& s)
{
	std::wstring ws;
	ws.assign(s.begin(), s.end());
	return parseString(ws);
}

CharDescriptorList Font::parseString(const std::wstring& s)
{
	if (s.empty())
		return CharDescriptorList();
	
	CharDescriptorList result(s.size());
	
	static const wchar_t* boldTagStart = L"<b>";
	static const wchar_t* boldTagEnd = L"</b>";
	
	static const wchar_t* colorTagStart = L"<color=";
	static const wchar_t* colorTagEnd = L"</color>";
	
	static const wchar_t* scaleTagStart = L"<scale=";
	static const wchar_t* scaleTagEnd = L"</scale>";
	
	size_t resultSize = 0;
	
	size_t boldTags = 0;
	std::stack<vec4> colorsStack;
	std::stack<float> scaleStack;
	colorsStack.push(vec4(1.0f));
	scaleStack.push(1.0f);
	
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
			colorsStack.push(colorTagToColor(subString(b, closingBracket)));
			b = closingBracket + 1;
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, colorTagEnd))
		{
			if (colorsStack.size() > 1)
				colorsStack.pop();
			b += textLength(colorTagEnd);
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, scaleTagStart))
		{
			auto closingBracket = findClosingBracket(b);
			scaleStack.push(std::wcstof(b + textLength(scaleTagStart), nullptr));
			b = closingBracket + 1;
			if (b >= e)
				break;
		}
		else if (textBeginsFrom(b, scaleTagEnd))
		{
			if (scaleStack.size() > 1)
				scaleStack.pop();
			b += textLength(scaleTagEnd);
			if (b >= e)
				break;
		}
		else
		{
			auto cd = (boldTags > 0) ? boldCharDescription(*b) : charDescription(*b);
			cd.color = colorsStack.top();
			cd.pixelsSize *= scaleStack.top();
			result[resultSize++] = cd;
			++b;
		}
	}
	
	result.shrink_to_fit();
	return result;
}

bool Font::isUtf8String(const std::string& s) const
{
	for (auto c : s)
	{
		if ((c & 0x80) != 0)
			return true;
	}
	
	return false;
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
	} while (*(++entry));
	return true;
}

const wchar_t* findClosingBracket(const wchar_t* text)
{
	static const wchar_t closingBracket = L'>';
	do { if (*text == closingBracket) break; } while (*(++text));
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
	size_t pos = 0;
	
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
