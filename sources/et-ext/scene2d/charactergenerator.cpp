/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#define ET_SHOULD_SAVE_GENERATED_CHARS 0
#define ET_DISABLE_SDF_GENERATION 0

#include <et-ext/scene2d/charactergenerator.hpp>
#include <et/core/filesystem.hpp>
#include <et/rendering/rendercontext.hpp>

#if (ET_SHOULD_SAVE_GENERATED_CHARS)
#include <et/app/application.hpp>
#include <et/imaging/imagewriter.hpp>
#endif

#include <external/freetype/include/ft2build.h>

#include FT_FREETYPE_H

#include <Windows.h>

namespace et {
namespace s2d {

enum GridProperies : int {
  defaultTextureSize = 1024,
  baseFontIntegerSize = 192,
  initialGridDimensions = 2 * (baseFontIntegerSize + 2) * (baseFontIntegerSize + 2),
};

const float CharacterGenerator::baseFontSize = static_cast<float>(baseFontIntegerSize);
const vec2i CharacterGenerator::charactersRenderingExtent = vec2i(128);

CharacterGenerator::CharacterGenerator(RenderInterface::Pointer& rc, const std::string& face, const std::string& boldFace, uint32_t faceIndex, uint32_t boldFaceIndex)
  : _impl(face, boldFace, faceIndex, boldFaceIndex)
  , _fontFace(face)
  , _fontBoldFace(boldFace)
  , _placer(vec2i(static_cast<int>(defaultTextureSize)), true) {
  _fontFace = fileExists(face) ? getFileName(face) : face;
  _fontBoldFace = fileExists(boldFace) ? getFileName(boldFace) : boldFace;

  TextureDescription::Pointer desc = TextureDescription::Pointer::create();
  desc->format = TextureFormat::R8;
  desc->size = vec2i(defaultTextureSize);
  desc->data = BinaryDataStorage(desc->size.square(), 0);
  desc->flags = Texture::Flags::ShaderResource | Texture::Flags::Readback;
  _texture = rc->createTexture(desc);
}

bool CharacterGenerator::performCropping(const BinaryDataStorage& renderedCharacterData, const vec2i& canvasSize, BinaryDataStorage& dataToSave, vec2i& sizeToSave, vec2i& topLeftOffset) {
  topLeftOffset = canvasSize - vec2i(1);
  vec2i bottomRightOffset = vec2i(0);

  vec2i pixel;
  for (pixel.y = 0; pixel.y < canvasSize.y - 1; ++pixel.y) {
    for (pixel.x = 0; pixel.x < canvasSize.x - 1; ++pixel.x) {
      uint32_t i = pixel.x + (canvasSize.y - pixel.y - 1) * canvasSize.x;
      if (renderedCharacterData[i]) {
        topLeftOffset = minv(topLeftOffset, pixel);
        bottomRightOffset = maxv(bottomRightOffset, pixel);
      }
    }
  }
  bottomRightOffset = minv(bottomRightOffset + vec2i(1), canvasSize);
  sizeToSave = bottomRightOffset - topLeftOffset;

  if ((sizeToSave.x <= 0) || (sizeToSave.y <= 0)) return false;

  vec2i additional;
  additional.x = sizeToSave.x % 4;
  additional.y = sizeToSave.y % 4;
  sizeToSave += additional;

  dataToSave.resize(sizeToSave.square());
  dataToSave.fill(0);

  vec2i targetPixel(0, additional.y / 2);
  for (int32_t py = topLeftOffset.y; py < bottomRightOffset.y; ++py, ++targetPixel.y) {
    targetPixel.x = additional.x / 2;
    for (int32_t px = topLeftOffset.x; px < bottomRightOffset.x; ++px, ++targetPixel.x) {
      uint32_t i = targetPixel.x + (sizeToSave.y - targetPixel.y - 1) * sizeToSave.x;
      uint32_t j = px + (canvasSize.y - py - 1) * canvasSize.x;
      dataToSave[i] = renderedCharacterData[j];
    }
  }

  return true;
}

void CharacterGenerator::generateCharacter(wchar_t value, CharacterFlags flags, CharDescriptor& result) {
  ET_ASSERT(value != 0);

  result.value = value;
  result.flags = flags;

  vec2i charSize;
  vec2i canvasSize;
  vec2i topLeftOffset;
  BinaryDataStorage renderedCharacterData;
  if (_impl.processCharacter(result, charSize, canvasSize, renderedCharacterData)) {
    generateSignedDistanceField(renderedCharacterData, canvasSize.x, canvasSize.y);

    vec2i sizeToSave;
    BinaryDataStorage dataToSave;

    if (performCropping(renderedCharacterData, canvasSize, dataToSave, sizeToSave, topLeftOffset)) {
#if (ET_SHOULD_SAVE_GENERATED_CHARS)
      auto path = application().environment().applicationDocumentsFolder() + "char_" + intToStr(value) + ".png";
      writeImageToFile(path, dataToSave, sizeToSave, 1, 8, ImageFormat::ImageFormat_PNG, true);
#endif

      vec2i downsampledSize = sizeToSave / 2;
      auto downsampled = downsample(dataToSave, sizeToSave);
      downsampled = downsample(downsampled, downsampledSize);
      downsampledSize /= 2;

      recti textureRect;
      if (_placer.place(downsampledSize, textureRect)) {
        vec2i invPos(textureRect.left, _texture->size(0).y - textureRect.top - downsampledSize.y - 1);
        vec2 fOrigin = _texture->getTexCoord(vector2ToFloat(textureRect.origin()), 0);
        result.contentRect = rect(vector2ToFloat(topLeftOffset - charactersRenderingExtent / 2), vector2ToFloat(sizeToSave));
        result.uvRect = rect(fOrigin, vector2ToFloat(textureRect.size()) / vector2ToFloat(_texture->size(0)));
        updateTexture(invPos, downsampledSize, downsampled);
      } else {
        log::error("Failed to place character to the texture: %d (%c, %C)", result.value, static_cast<char>(result.value), static_cast<wchar_t>(result.value));
      }
    }

    result.originalSize = vector2ToFloat(charSize);
  }

  characterGenerated.invoke(value);
}

void CharacterGenerator::updateTexture(const vec2i& position, const vec2i& size, BinaryDataStorage& data) {
  _texture->updateRegion(position, size, data);
}

void CharacterGenerator::setTexture(Texture::Pointer tex) {
  _texture = tex;
}

void CharacterGenerator::pushCharacter(const s2d::CharDescriptor& desc) {
  CharDescriptorMap& mapToInsert = ((desc.flags & CharacterFlag_Bold) == CharacterFlag_Bold) ? _boldChars : _chars;
  mapToInsert.insert(std::make_pair(desc.value, desc));

  vec2 size = _texture->sizeFloat(0) * desc.uvRect.size();
  vec2 origin = _texture->sizeFloat(0) * vec2(desc.uvRect.origin().x, 1.0f - desc.uvRect.origin().y);

  _placer.addPlacedRect(recti(static_cast<int>(origin.x), static_cast<int>(origin.y), static_cast<int>(size.x), static_cast<int>(size.y)));
}

BinaryDataStorage CharacterGenerator::downsample(BinaryDataStorage& input, const vec2i& size) {
  vec2i downsampledSize = size / 2;
  BinaryDataStorage result(downsampledSize.square(), 0);

  uint32_t k = 0;
  for (int32_t y = 0; y < downsampledSize.y; ++y) {
    int32_t thisRow = 2 * y;
    int32_t nextRow = std::min(thisRow + 1, size.y - 1);
    for (int32_t x = 0; x < downsampledSize.x; ++x) {
      int32_t thisCol = 2 * x;
      int32_t nextCol = std::min(thisCol + 1, size.x - 1);
      int32_t in00 = thisRow * size.x + thisCol;
      int32_t in01 = thisRow * size.x + nextCol;
      int32_t in10 = nextRow * size.x + thisCol;
      int32_t in11 = nextRow * size.x + nextCol;
      result[k++] = (input[in00] + input[in01] + input[in10] + input[in11]) / 4;
    }
  }

  return result;
}

inline const vec2i& sdf_get(const sdf::Grid& g, int32_t x, int32_t y) {
  ET_ASSERT((x >= 0) && (y >= 0) & (x < g.w) && (y < g.h));
  return g.grid[y * g.w + x];
}

inline void sdf_put(sdf::Grid& g, int32_t x, int32_t y, const vec2i& p) {
  ET_ASSERT((x >= 0) && (y >= 0) & (x < g.w) && (y < g.h));
  g.grid[y * g.w + x] = p;
}

inline void internal_sdf_compare(const sdf::Grid& g, vec2i& p, int32_t x, int32_t y, int32_t offsetx, int32_t offsety) {
  vec2i other = sdf_get(g, x + offsetx, y + offsety) + vec2i(offsetx, offsety);
  if ((other.dotSelf() - p.dotSelf()) & 0x80000000) p = other;
}

void CharacterGenerator::generateSignedDistanceFieldOnGrid(sdf::Grid& g) {
#if (ET_DISABLE_SDF_GENERATION)
  // do nothing
#else
  for (int32_t y = 1; y < g.h - 1; ++y) {
    for (int32_t x = 1; x < g.w - 1; ++x) {
      vec2i p = sdf_get(g, x, y);
      internal_sdf_compare(g, p, x, y, -1, 0);
      internal_sdf_compare(g, p, x, y, 0, -1);
      internal_sdf_compare(g, p, x, y, -1, -1);
      internal_sdf_compare(g, p, x, y, 1, -1);
      sdf_put(g, x, y, p);
    }
    for (int32_t x = g.w - 2; x > 0; --x) {
      vec2i p = sdf_get(g, x, y);
      internal_sdf_compare(g, p, x, y, 1, 0);
      sdf_put(g, x, y, p);
    }
  }

  for (int32_t y = g.h - 2; y > 0; --y) {
    for (int32_t x = g.w - 2; x > 0; --x) {
      vec2i p = sdf_get(g, x, y);
      internal_sdf_compare(g, p, x, y, 1, 0);
      internal_sdf_compare(g, p, x, y, 0, 1);
      internal_sdf_compare(g, p, x, y, -1, 1);
      internal_sdf_compare(g, p, x, y, 1, 1);
      sdf_put(g, x, y, p);
    }
    for (int32_t x = 1; x < g.w - 1; ++x) {
      vec2i p = sdf_get(g, x, y);
      internal_sdf_compare(g, p, x, y, -1, 0);
      sdf_put(g, x, y, p);
    }
  }
#endif
}

void CharacterGenerator::generateSignedDistanceField(BinaryDataStorage& data, int32_t w, int32_t h) {
  uint32_t requiredSize = static_cast<uint32_t>(sizeof(vec2i) * w * h);
  uint32_t providedSize = static_cast<uint32_t>(sizeof(sdf::Grid::grid));
  if (requiredSize > providedSize) {
    ET_FAIL_FMT("Insufficient storage for SDF generator. Required: %d (%d x %d), provided: %d", requiredSize, w, h, providedSize);
  }

  _grid0.w = w;
  _grid1.w = w;
  _grid0.h = h;
  _grid1.h = h;
  memset(_grid0.grid, 0, sizeof(_grid0.grid));
  memset(_grid1.grid, 0, sizeof(_grid1.grid));

  const vec2i pointOutside = vec2i(255);

  for (int32_t y = 0; y < _grid0.h; ++y) {
    sdf_put(_grid1, 0, y, pointOutside);
    sdf_put(_grid1, _grid0.w - 1, y, pointOutside);
  }

  for (int32_t x = 0; x < _grid0.w; ++x) {
    sdf_put(_grid1, x, 0, pointOutside);
    sdf_put(_grid1, x, _grid0.h - 1, pointOutside);
  }

  for (int32_t y = 1; y < h - 1; ++y) {
    for (int32_t x = 1; x < w - 1; ++x) {
      auto& targetGrid = (data[x + y * _grid0.w] == 0) ? _grid1 : _grid0;
      sdf_put(targetGrid, x, y, pointOutside);
    }
  }

  generateSignedDistanceFieldOnGrid(_grid0);
  generateSignedDistanceFieldOnGrid(_grid1);

  for (int32_t y = 1; y < h - 1; ++y) {
    for (int32_t x = 1; x < w - 1; ++x) {
      float p1 = static_cast<float>(sdf_get(_grid0, x, y).dotSelf());
      float p2 = static_cast<float>(sdf_get(_grid1, x, y).dotSelf());
      float value = 3.0f * (std::sqrt(p1) - std::sqrt(p2)) + 128.0f;
      data[x + y * _grid0.w] = static_cast<unsigned char>(clamp(value, 0.0f, 255.0f));
    }
  }
}

/*
 * Private implementation
 */
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
