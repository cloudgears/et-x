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
#include <et/imaging/imagewriter.h>
#include <et-ext/scene2d/font.h>

using namespace et;
using namespace et::s2d;

Font::Font(const CharacterGenerator::Pointer& generator) :
	_generator(generator)
{
	
}

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
	
	BinaryDataStorage imageData(4 * texture()->size().square());
	
#if (ET_PLATFORM_MAC || ET_PLATFORM_WIN)
	
	rc->renderState().bindTexture(0, texture());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
	checkOpenGLError("glGetTexImage");
	
#elif (ET_PLATFORM_IOS || ET_PLATFORM_ANDROID)
	
	auto fbo = rc->framebufferFactory().createFramebuffer(texture()->size(), "temp-buffer",
		GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE, 0, 0, 0);
	
	bool blendEnabled = rc->renderState().blendEnabled();
	auto currentBuffer = rc->renderState().boundFramebuffer();
	
	rc->renderState().bindFramebuffer(fbo);
	rc->renderState().setBlend(false, BlendState_Current);
	rc->renderer()->renderFullscreenTexture(texture());
	
	glReadPixels(0, 0, fbo->size().x, fbo->size().y, GL_RGBA, GL_UNSIGNED_BYTE, imageData.data());
	checkOpenGLError("glReadPixels");
	
	rc->renderState().bindFramebuffer(currentBuffer);
	rc->renderState().setBlend(blendEnabled, BlendState_Current);
	
	fbo.reset(nullptr);
	
#endif
	
	ImageWriter::writeImageToFile(getFilePath(fileName) + textureFile, imageData,
		texture()->size(), 4, 8, ImageFormat_PNG, true);
	
}

void Font::loadFromFile(RenderContext* rc, const std::string& fileName, ObjectsCache& cache)
{
	std::string resolvedFileName =
		application().environment().resolveScalableFileName(fileName, rc->screenScaleFactor());

	InputStream fontFile(resolvedFileName, StreamMode_Binary);
	if (fontFile.invalid()) return;

	std::string fontFileDir = getFilePath(resolvedFileName);

	int version = deserializeInt(fontFile.stream());
	ET_ASSERT(version >= FONT_VERSION_2)
	
	std::string face = deserializeString(fontFile.stream());
	deserializeInt(fontFile.stream()); // -> size
	
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
		lineSize.y = etMax(lineSize.y, desc.size.y);
		
		if ((desc.value == ET_RETURN) || (desc.value == ET_NEWLINE))
		{
			sz.x = etMax(sz.x, lineSize.x);
			sz.y += lineSize.y;
			lineSize = vec2(0.0f);
		}
		else 
		{
			lineSize.x += desc.size.x;
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
	ET_ITERATE(s, auto, i, result.push_back(charDescription(i)));
	return result;
}

CharDescriptorList Font::buildString(const std::wstring& s, bool formatted)
{
	if (formatted)
		return parseString(s);

	CharDescriptorList result;
	ET_ITERATE(s, auto, i, result.push_back(charDescription(i)));
	return result;
}

CharDescriptorList Font::parseString(const std::string& s)
{
	CharDescriptorList result;
	
	const char tagOpening = '<';
	const char tagClosing = '>';
	const char tagClosingId = '/';
	const std::string tagBold("b");
	const std::string tagColor("color");
	const std::string tagColorOpen("color=#");

	std::deque<vec4> colors;
	colors.push_front(vec4(1.0f));

	int nBoldTags = 0;
	int nColorTags = 0;
	bool readingTag = false;
	bool closingTag = false;
	std::string tag;

	for (auto& c : s)
	{
		if (c == tagOpening)
		{
			readingTag = true;
			closingTag = false;
			tag = std::string();
		}
		else if (c == tagClosing)
		{
			if (readingTag)
			{
				if (closingTag)
				{
					if (tag == tagBold)
					{
						if (nBoldTags)
							--nBoldTags;
					}
					else if (tag.find_first_of(tagColor) == 0)
					{
						if (nColorTags)
						{
							--nColorTags;
							colors.pop_front();
						}
					}
					else 
					{
						log::warning("Unknown tag `%s` passed in string: %s", tag.c_str(), s.c_str());
					}
				}
				else
				{
					if (tag == tagBold)
					{
						nBoldTags++;
					}
					else if (tag.find(tagColorOpen) == 0)
					{
						nColorTags++;
						tag.erase(0, tagColorOpen.size());
						colors.push_front(strHexToVec4(tag));
					}
					else
					{
						log::warning("Unknown tag `%s` passed in string: %s", tag.c_str(), s.c_str());
					}
				}
				readingTag = false;
				closingTag = false;
			}
		}
		else 
		{
			if (readingTag)
			{
				if (c == tagClosingId)
				{
					closingTag = true;
				}
				else if (!isWhitespaceChar(c))
				{
					tag += c;
				}
			}
			else
			{
				result.push_back(nBoldTags ? boldCharDescription(c) : charDescription(c));
				result.back().color = colors.front();
			}
		}
	}

	return result;
}

CharDescriptorList Font::parseString(const std::wstring& s)
{
	CharDescriptorList result;
	
	const wchar_t tagOpening = L'<';
	const wchar_t tagClosing = L'>';
	const wchar_t tagClosingId = L'/';
	const std::wstring tagBold(L"b");
	const std::wstring tagColor(L"color");
	const std::wstring tagColorOpen(L"color=#");

	std::deque<vec4> colors;
	colors.push_front(vec4(1.0f));

	int nBoldTags = 0;
	int nColorTags = 0;
	bool readingTag = false;
	bool closingTag = false;
	std::wstring tag;

	for (auto& c : s)
	{
		if (c == tagOpening)
		{
			readingTag = true;
			closingTag = false;
			tag = std::wstring();
		}
		else if (c == tagClosing)
		{
			if (readingTag)
			{
				if (closingTag)
				{
					if (tag == tagBold)
					{
						if (nBoldTags)
							--nBoldTags;
					}
					else if (tag.find_first_of(tagColor) == 0)
					{
						if (nColorTags)
						{
							--nColorTags;
							colors.pop_front();
						}
					}
					else 
					{
						log::warning("Unknown tag `%S` passed in string: %S", tag.c_str(), s.c_str());
					}
				}
				else
				{
					if (tag == tagBold)
					{
						nBoldTags++;
					}
					else if (tag.find(tagColorOpen) == 0)
					{
						nColorTags++;
						tag.erase(0, tagColorOpen.size());
						colors.push_front(strHexToVec4(tag));
					}
					else
					{
						log::warning("Unknown tag `%S` passed in string: %S", tag.c_str(), s.c_str());
					}
				}
				readingTag = false;
				closingTag = false;
			}
		}
		else 
		{
			if (readingTag)
			{
				if (c == tagClosingId)
				{
					closingTag = true;
				}
				else if (!isWhitespaceChar(c))
				{
					tag += c;
				}
			}
			else
			{
				result.push_back(nBoldTags ? boldCharDescription(c) : charDescription(c));
				result.back().color = colors.front();
			}
		}
	}

	return result;
}

bool Font::isUtf8String(const std::string& s) const
{
	for (auto c : s)
	{
		if (c & 0x80)
			return true;
	}
	
	return false;
}