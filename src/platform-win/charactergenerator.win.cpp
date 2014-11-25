/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/geometry/rectplacer.h>
#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/charactergenerator.h>

#include <Windows.h>

using namespace et;
using namespace et::s2d;

class et::s2d::CharacterGeneratorImplementationPrivate
{
public:
	CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace);
	~CharacterGeneratorImplementationPrivate();

	bool startWithCharacter(const CharDescriptor& desc, vec2i& charSize, vec2i& canvasSize, BinaryDataStorage& charData);

private:
	HDC commonDC;
	HFONT font;
	HFONT boldFont;
};

CharacterGeneratorImplementation::CharacterGeneratorImplementation(const std::string& face, const std::string& boldFace)
	{ ET_PIMPL_INIT(CharacterGeneratorImplementation, face, boldFace) }

CharacterGeneratorImplementation::~CharacterGeneratorImplementation()
	{ ET_PIMPL_FINALIZE(CharacterGeneratorImplementation) }

bool CharacterGeneratorImplementation::processCharacter(const CharDescriptor& a, vec2i& b, vec2i& c, BinaryDataStorage& d)
	{ return _private->startWithCharacter(a, b, c, d); }

CharacterGeneratorImplementationPrivate::CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace)
{
	commonDC = CreateCompatibleDC(nullptr);
	SetTextColor(commonDC, RGB(255, 255, 255));
	SetBkMode(commonDC, TRANSPARENT);
	SetMapMode(commonDC, MM_TEXT);

	int pointSize = -static_cast<int>(CharacterGenerator::baseFontSize);

	font = CreateFont(pointSize, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_RASTER_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, face.c_str());

	boldFont = CreateFont(pointSize, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, OUT_RASTER_PRECIS,
		CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FF_DONTCARE, boldFace.c_str());
}

CharacterGeneratorImplementationPrivate::~CharacterGeneratorImplementationPrivate()
{
	DeleteObject(font);
	DeleteObject(boldFont);
	DeleteDC(commonDC);
}

bool CharacterGeneratorImplementationPrivate::startWithCharacter(const CharDescriptor& desc, vec2i& charSize, 
	vec2i& canvasSize, BinaryDataStorage& charData)
{
	if ((desc.value == ET_NEWLINE) || (desc.value == ET_RETURN))
		return false;

	bool isBold = (desc.flags & CharacterFlag_Bold) == CharacterFlag_Bold;
	
	SIZE characterSize = {};
	wchar_t string[2] = { static_cast<wchar_t>(desc.value), 0 };

	SelectObject(commonDC, isBold ? boldFont : font);
	GetTextExtentPointW(commonDC, string, 1, &characterSize);

	TEXTMETRICW textMetrics = { };
	GetTextMetricsW(commonDC, &textMetrics);
	auto leading = textMetrics.tmInternalLeading + textMetrics.tmExternalLeading;

	charSize = vec2i(static_cast<int>(characterSize.cx),  static_cast<int>(characterSize.cy - leading));

	if (charSize.dotSelf() <= 0) 
		return false;

	canvasSize = charSize + CharacterGenerator::charactersRenderingExtent;

	RECT r = 
	{ 
		CharacterGenerator::charactersRenderingExtent.x / 2, 
		CharacterGenerator::charactersRenderingExtent.y / 2 - leading, 
		canvasSize.x,
		canvasSize.y + leading
	};

	HBITMAP bitmap = CreateBitmap(canvasSize.x, canvasSize.y, 1, 32, nullptr);
	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapInfo.bmiHeader.biWidth = canvasSize.x;
	bitmapInfo.bmiHeader.biHeight = canvasSize.y;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;

	SelectObject(commonDC, bitmap);

	DrawTextW(commonDC, string, -1, &r, DT_SINGLELINE);

	BinaryDataStorage rgbData(4 * canvasSize.square(), 0);
	GetDIBits(commonDC, bitmap, 0, canvasSize.y, rgbData.binary(), &bitmapInfo, DIB_RGB_COLORS);

	charData.resize(canvasSize.square());

	size_t k = 0;
	unsigned int* ptr = reinterpret_cast<unsigned int*>(rgbData.data());
	uint32_t* ptrEnd = reinterpret_cast<unsigned int*>(rgbData.data() + rgbData.dataSize());
	while (ptr != ptrEnd)
		charData[k++] = (*ptr++) & 0xff;

	DeleteObject(bitmap);

	return true;
}
