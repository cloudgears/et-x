/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/charactergenerator.h>
#include <et/platform-apple/apple.h>

#include <freetype/ft2build.h>
#include FT_FREETYPE_H

#if (ET_PLATFORM_APPLE)
#	include <CoreText/CTFontDescriptor.h>
#	include <CoreFoundation/CFString.h>
#	include <CoreFoundation/CFURL.h>
#elif (ET_PLATFORM_WIN)
#	include <Windows.h>
#endif

using namespace et;
using namespace et::s2d;

class et::s2d::CharacterGeneratorImplementationPrivate
{
public:
	CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace);
	~CharacterGeneratorImplementationPrivate();
	
	bool startWithCharacter(const CharDescriptor& desc, vec2i& charSize, vec2i& canvasSize, BinaryDataStorage& charData);
	
private:
	FT_Library library = nullptr;
	FT_Face regularFont = nullptr;
	FT_Face boldFont = nullptr;
};

CharacterGeneratorImplementation::CharacterGeneratorImplementation(const std::string& face, const std::string& boldFace)
	{ ET_PIMPL_INIT(CharacterGeneratorImplementation, face, boldFace) }

CharacterGeneratorImplementation::~CharacterGeneratorImplementation()
	{ ET_PIMPL_FINALIZE(CharacterGeneratorImplementation) }

bool CharacterGeneratorImplementation::processCharacter(const CharDescriptor& a, vec2i& b, vec2i& c, BinaryDataStorage& d)
	{ return _private->startWithCharacter(a, b, c, d); }

CharacterGeneratorImplementationPrivate::CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace)
{
	FT_Init_FreeType(&library);

	if (fileExists(face))
	{
		FT_New_Face(library, face.c_str(), 0, &regularFont);
	}
	else
	{
#if (ET_PLATFORM_APPLE)
		StaticDataStorage<unsigned char, 1024> fontPath(0);
		CFStringRef faceString = CFStringCreateWithCString(kCFAllocatorDefault, face.c_str(), kCFStringEncodingUTF8);
		CTFontDescriptorRef fontRef = CTFontDescriptorCreateWithNameAndSize(faceString, 10.0f);
		if (fontRef)
		{
			CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(fontRef, kCTFontURLAttribute);
			if (url)
			{
				CFURLGetFileSystemRepresentation(url, 1, fontPath.data, fontPath.size());
				CFRelease(url);
			}
			CFRelease(fontRef);
		}
		CFRelease(faceString);
		FT_New_Face(library, fontPath.binary(), 0, &regularFont);
#endif
	}
	
	/*
	 * Load bold font
	 */
	if (fileExists(boldFace))
	{
		FT_New_Face(library, boldFace.c_str(), 0, &boldFont);
	}
	else
	{
#if (ET_PLATFORM_APPLE)
		StaticDataStorage<unsigned char, 1024> fontPath(0);
		CFStringRef faceString = CFStringCreateWithCString(kCFAllocatorDefault, boldFace.c_str(), kCFStringEncodingUTF8);
		CTFontDescriptorRef fontRef = CTFontDescriptorCreateWithNameAndSize(faceString, 10.0f);
		if (fontRef)
		{
			CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(fontRef, kCTFontURLAttribute);
			if (url)
			{
				CFURLGetFileSystemRepresentation(url, 1, fontPath.data, fontPath.size());
				CFRelease(url);
			}
			CFRelease(fontRef);
		}
		FT_New_Face(library, fontPath.binary(), 0, &regularFont);
#endif
	}

	auto charSize = static_cast<int>(CharacterGenerator::baseFontSize) << 6;
	FT_Set_Char_Size(regularFont, charSize, charSize, 72, 72);
	FT_Set_Char_Size(boldFont, charSize, charSize, 72, 72);
}

CharacterGeneratorImplementationPrivate::~CharacterGeneratorImplementationPrivate()
{
	FT_Done_Face(regularFont);
	FT_Done_FreeType(library);
}

bool CharacterGeneratorImplementationPrivate::startWithCharacter(const CharDescriptor& desc, vec2i& charSize,
	vec2i& canvasSize, BinaryDataStorage& charData)
{
	auto font = (desc.flags & CharacterFlag_Bold) == CharacterFlag_Bold ? boldFont : regularFont;

	auto glyphIndex = FT_Get_Char_Index(font, desc.value);
	if (FT_Load_Glyph(font, glyphIndex, FT_LOAD_RENDER)) return false;
	
	auto glyph = font->glyph;
	
	vec2i bitmapSize(glyph->bitmap.width, glyph->bitmap.rows);
	
	charSize.x = static_cast<int>(glyph->metrics.horiAdvance >> 6);
	charSize.y = static_cast<int>(glyph->metrics.vertAdvance >> 6);
	
	if (charSize.dotSelf() <= 0) return false;
	
	canvasSize = charSize + CharacterGenerator::charactersRenderingExtent;
	
	charData.resize(canvasSize.square());
	charData.fill(0);
	
	int ascender = static_cast<int>(font->size->metrics.ascender >> 6);
	
	int ox = glyph->bitmap_left + CharacterGenerator::charactersRenderingExtent.x / 2;
	int oy = etMax(0, ascender - glyph->bitmap_top + CharacterGenerator::charactersRenderingExtent.y / 2);
	
//	bitmapSize.y = etMin(bitmapSize.y, canvasSize.y - oy - 1);

	size_t k = 0;
	for (int y = 0; y < bitmapSize.y; ++y)
	{
		for (int x = 0; x < bitmapSize.x; ++x)
		{
			int targetPos = (ox + x) + (canvasSize.y - oy - y - 1) * canvasSize.x;
			if (targetPos >= 0)
				charData[targetPos] = glyph->bitmap.buffer[k++];
		}
	}
	
	return true;
}
