/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <external/freetype/include/ft2build.h>

#include <et/core/filesystem.hpp>
#include <et/core/platform.hpp>
#include <et/rendering/rendercontext.hpp>
#include <et/scene2d/charactergenerator.hpp>

#include FT_FREETYPE_H

#if (ET_PLATFORM_APPLE)
#include <CoreFoundation/CFString.hpp>
#include <CoreFoundation/CFURL.hpp>
#include <CoreText/CTFontDescriptor.hpp>
#include <et/platform-apple/apple.hpp>
#elif (ET_PLATFORM_WIN)
#include <Windows.h>
#endif

namespace et {
namespace s2d {

class CharacterGeneratorImplementationPrivate {
 public:
  CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace, uint32_t faceIndex, uint32_t boldFaceIndex);
  ~CharacterGeneratorImplementationPrivate();

  bool startWithCharacter(const CharDescriptor& desc, vec2i& charSize, vec2i& canvasSize, BinaryDataStorage& charData);

 private:
  BinaryDataStorage regularFontData;
  BinaryDataStorage boldFontData;

  FT_Library library = nullptr;
  FT_Face regularFont = nullptr;
  FT_Face boldFont = nullptr;
};

CharacterGeneratorImplementation::CharacterGeneratorImplementation(const std::string& face, const std::string& boldFace, uint32_t faceIndex, uint32_t boldFaceIndex) {
  ET_PIMPL_INIT(CharacterGeneratorImplementation, face, boldFace, faceIndex, boldFaceIndex);
}

CharacterGeneratorImplementation::~CharacterGeneratorImplementation() {
  ET_PIMPL_FINALIZE(CharacterGeneratorImplementation);
}

bool CharacterGeneratorImplementation::processCharacter(const CharDescriptor& a, vec2i& b, vec2i& c, BinaryDataStorage& d) {
  return _private->startWithCharacter(a, b, c, d);
}

CharacterGeneratorImplementationPrivate::CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace, uint32_t faceIndex, uint32_t boldFaceIndex) {
  FT_Init_FreeType(&library);

  if (fileExists(face)) {
    FT_Error err = FT_New_Face(library, face.c_str(), FT_Long(faceIndex), &regularFont);
    if (regularFont == nullptr) {
      log::error("Failed to load FreeType font: %s [%u], error: %d (0x%08x)", face.c_str(), faceIndex, err, err);
    }
  } else {
#if (ET_PLATFORM_APPLE)
    StaticDataStorage<unsigned char, 1024> fontPath(0);
    CFStringRef faceString = CFStringCreateWithCString(kCFAllocatorDefault, face.c_str(), kCFStringEncodingUTF8);
    CTFontDescriptorRef fontRef = CTFontDescriptorCreateWithNameAndSize(faceString, 10.0f);
    if (fontRef) {
      CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(fontRef, kCTFontURLAttribute);
      if (url) {
        CFURLGetFileSystemRepresentation(url, 1, fontPath.data, fontPath.size());
        CFRelease(url);
      }
      CFRelease(fontRef);
    }
    CFRelease(faceString);
    FT_New_Face(library, fontPath.binary(), faceIndex, &regularFont);
#elif (ET_PLATFORM_WIN)
    auto commonDC = CreateCompatibleDC(nullptr);

    HFONT font = CreateFontA(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_EMBEDDED, PROOF_QUALITY, FF_DONTCARE, face.c_str());

    SelectObject(commonDC, font);
    auto fontDataSize = GetFontData(commonDC, 0, 0, nullptr, 0);
    if ((fontDataSize > 0) && (fontDataSize != GDI_ERROR)) {
      regularFontData.resize(fontDataSize);
      regularFontData.fill(0);

      if (GetFontData(commonDC, 0, 0, regularFontData.data(), DWORD(regularFontData.size())) != GDI_ERROR) {
        FT_Error err = FT_New_Memory_Face(library, regularFontData.data(), FT_Long(regularFontData.size()), FT_Long(faceIndex), &regularFont);
        if (regularFont == nullptr) {
          log::error("Failed to create FreeType font: %s [%u], error: %d (0x%08x)", face.c_str(), faceIndex, err, err);
        }
      }
    }

    DeleteObject(font);
    DeleteDC(commonDC);
#endif
  }

  /*
   * Load bold font
   */
  if (fileExists(boldFace)) {
    FT_Error err = FT_New_Face(library, boldFace.c_str(), FT_Long(boldFaceIndex), &boldFont);
    if (boldFont == nullptr) {
      log::error("Failed to load bold FreeType font: %s [%u], error: %d (0x%08x)", boldFace.c_str(), boldFaceIndex, err, err);
    }
  } else {
#if (ET_PLATFORM_APPLE)

    StaticDataStorage<unsigned char, 1024> fontPath(0);
    CFStringRef faceString = CFStringCreateWithCString(kCFAllocatorDefault, boldFace.c_str(), kCFStringEncodingUTF8);
    CTFontDescriptorRef fontRef = CTFontDescriptorCreateWithNameAndSize(faceString, 10.0f);
    if (fontRef) {
      CFURLRef url = (CFURLRef)CTFontDescriptorCopyAttribute(fontRef, kCTFontURLAttribute);
      if (url) {
        CFURLGetFileSystemRepresentation(url, 1, fontPath.data, fontPath.size());
        CFRelease(url);
      }
      CFRelease(fontRef);
    }
    FT_New_Face(library, fontPath.binary(), boldFaceIndex, &boldFont);
    CFRelease(faceString);

#elif (ET_PLATFORM_WIN)

    auto commonDC = CreateCompatibleDC(nullptr);

    HFONT font = CreateFontA(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_EMBEDDED, PROOF_QUALITY, FF_DONTCARE, boldFace.c_str());

    SelectObject(commonDC, font);
    auto fontDataSize = GetFontData(commonDC, 0, 0, nullptr, 0);
    if ((fontDataSize > 0) && (fontDataSize != GDI_ERROR)) {
      boldFontData.resize(fontDataSize);
      boldFontData.fill(0);

      if (GetFontData(commonDC, 0, 0, boldFontData.data(), DWORD(boldFontData.size())) != GDI_ERROR) {
        FT_Error err = FT_New_Memory_Face(library, boldFontData.data(), FT_Long(boldFontData.size()), FT_Long(boldFaceIndex), &boldFont);
        if (boldFont == nullptr) {
          log::error("Failed to create bold FreeType font: %s [%u], error: %d (0x%08x)", boldFace.c_str(), boldFaceIndex, err, err);
        }
      }
    }

    DeleteObject(font);
    DeleteDC(commonDC);

#endif
  }

  auto charSize = static_cast<int>(CharacterGenerator::baseFontSize) << 6;
  FT_Set_Char_Size(regularFont, charSize, charSize, 72, 72);
  FT_Set_Char_Size(boldFont, charSize, charSize, 72, 72);
}

CharacterGeneratorImplementationPrivate::~CharacterGeneratorImplementationPrivate() {
  FT_Done_Face(regularFont);
  FT_Done_FreeType(library);
}

bool CharacterGeneratorImplementationPrivate::startWithCharacter(const CharDescriptor& desc, vec2i& charSize, vec2i& canvasSize, BinaryDataStorage& charData) {
  auto font = (desc.flags & CharacterFlag_Bold) == CharacterFlag_Bold ? boldFont : regularFont;

  auto glyphIndex = FT_Get_Char_Index(font, desc.value);
  if (FT_Load_Glyph(font, glyphIndex, FT_LOAD_RENDER)) return false;

  auto glyph = font->glyph;

  vec2i bitmapSize(glyph->bitmap.width, glyph->bitmap.rows);

  long ascender = font->size->metrics.ascender >> 6;
  long descender = (-font->size->metrics.descender) >> 6;

  charSize.x = static_cast<int>(glyph->metrics.horiAdvance >> 6);
  charSize.y = static_cast<int>(ascender + descender);

  if (charSize.dotSelf() <= 0) return false;

  canvasSize = charSize + CharacterGenerator::charactersRenderingExtent;

  charData.resize(canvasSize.square());
  charData.fill(0);

  int ox = glyph->bitmap_left + CharacterGenerator::charactersRenderingExtent.x / 2;

  int oy = std::max(0, static_cast<int>(ascender) - glyph->bitmap_top + CharacterGenerator::charactersRenderingExtent.y / 2);

  uint32_t k = 0;
  for (int y = 0; y < bitmapSize.y; ++y) {
    for (int x = 0; x < bitmapSize.x; ++x) {
      int targetPos = (ox + x) + (canvasSize.y - oy - y - 1) * canvasSize.x;
      ET_ASSERT(targetPos >= 0);
      charData[targetPos] = glyph->bitmap.buffer[k++];
    }
  }

  return true;
}

}  // namespace s2d
}  // namespace et
