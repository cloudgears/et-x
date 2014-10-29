/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <AppKit/NSFont.h>
#include <AppKit/NSColor.h>
#include <AppKit/NSAttributedString.h>
#include <AppKit/NSStringDrawing.h>
#include <AppKit/NSGraphicsContext.h>

#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/geometry/rectplacer.h>
#include <et-ext/scene2d/charactergenerator.h>
#include <et/imaging/imagewriter.h>

using namespace et;
using namespace et::s2d;

class et::s2d::CharacterGeneratorPrivate
{
public:
	CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace, size_t size, size_t texSize);
	~CharacterGeneratorPrivate();
	
	void updateTexture(RenderContext* rc, const vec2i& position, const vec2i& size,
		Texture texture, BinaryDataStorage& data);
	
	void renderCharacter(NSAttributedString* value, const vec2i& size,
		NSFont* font, BinaryDataStorage& data);
	
	void processImageData(BinaryDataStorage& data, const vec2i& size);
	
public:
	std::string fontFace;
	size_t fontSize;
	size_t textureSize;
	
	RectPlacer _placer;
	
	NSFont* font;
	NSFont* boldFont;
	NSColor* whiteColor;

	CGColorSpaceRef colorSpace;
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string& boldFace, size_t size, size_t texSize) : _rc(rc), _face(face), _size(size)
{
	ET_PIMPL_INIT(CharacterGenerator, face, boldFace, size, texSize)
	
	BinaryDataStorage emptyData(sqr(texSize), 0);
	
#if defined(GL_R8)
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_R8, vec2i(static_cast<int>(texSize)),
		GL_RED, GL_UNSIGNED_BYTE, emptyData, face + "font");
#else
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RED, vec2i(static_cast<int>(texSize)),
		GL_RED, GL_UNSIGNED_BYTE, emptyData, face + "font");
#endif
}

CharacterGenerator::~CharacterGenerator()
{
	ET_PIMPL_FINALIZE(CharacterGenerator)
}

CharDescriptor CharacterGenerator::generateCharacter(int value, bool)
{
	wchar_t string[2] = { value, 0 };
	
	NSString* wString = [[NSString alloc] initWithBytesNoCopy:string length:sizeof(string)
		encoding:NSUTF32LittleEndianStringEncoding freeWhenDone:NO];

	NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString:wString];

	NSRange wholeString = NSMakeRange(0, [attrString length]);
	[attrString addAttribute:NSFontAttributeName value:_private->font range:wholeString];
	[attrString addAttribute:NSForegroundColorAttributeName value:_private->whiteColor range:wholeString];

	NSSize characterSize = [attrString size];
	vec2i charSize = vec2i(static_cast<int>(characterSize.width),
		static_cast<int>(characterSize.height));
	
	CharDescriptor desc(value);
	
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square(), 0);
		
		rect textureRect;
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
		_private->processImageData(data, charSize);
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);
		
		desc.pixelsOrigin = textureRect.origin() + vec2(1.0f);
		desc.pixelsSize = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.pixelsOrigin);
		desc.uvSize = desc.pixelsSize / _texture->sizeFloat();
		
		std::string imageName = application().environment().applicationDocumentsFolder() + "char-" + intToStr(value) + ".png";
		ImageWriter::writeImageToFile(imageName, data, charSize, 1, 8, ImageFormat_PNG, true);
		
		imageName = application().environment().applicationDocumentsFolder() + "result.png";
		BinaryDataStorage image(sqr(_private->textureSize), 0);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, image.data());
		ImageWriter::writeImageToFile(imageName, image, vec2i(_private->textureSize), 1, 8, ImageFormat_PNG, true);
	}
	
#if (!ET_OBJC_ARC_ENABLED)
	[attrString release];
	[wString release];
#endif
	
	characterGenerated.invoke(value);
	
	_chars[value] = desc;
	return desc;
}

CharDescriptor CharacterGenerator::generateBoldCharacter(int value, bool)
{
	wchar_t string[2] = { value, 0 };

	NSString* wString = [[NSString alloc] initWithBytesNoCopy:string length:sizeof(string)
		encoding:NSUTF32LittleEndianStringEncoding freeWhenDone:NO];

	NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString:wString];

	NSRange wholeString = NSMakeRange(0, [attrString length]);
	[attrString addAttribute:NSFontAttributeName value:_private->boldFont range:wholeString];
	[attrString addAttribute:NSForegroundColorAttributeName value:_private->whiteColor range:wholeString];

	NSSize characterSize = [attrString size];
	vec2i charSize = vec2i(static_cast<int>(characterSize.width),
		static_cast<int>(characterSize.height));
	
	CharDescriptor desc(value, CharParameter_Bold);
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square(), 0);

		rect textureRect;
		
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
		_private->processImageData(data, charSize);
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);

		desc.pixelsOrigin = textureRect.origin() + vec2(1.0f);
		desc.pixelsSize = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.pixelsOrigin);
		desc.uvSize = desc.pixelsSize / _texture->sizeFloat();
	}

#if (!ET_OBJC_ARC_ENABLED)
	[attrString release];
	[wString release];
#endif
	
	characterGenerated.invoke(value);

	_boldChars[value] = desc;
	return desc;
}

void CharacterGenerator::setTexture(Texture tex)
{
	_texture = tex;
}

void CharacterGenerator::pushCharacter(const et::s2d::CharDescriptor& desc)
{
	if (desc.params & CharParameter_Bold)
		_boldChars[desc.value] = desc;
	else
		_chars[desc.value] = desc;
	
	_private->_placer.addPlacedRect(rect(desc.pixelsOrigin - vec2(1.0f), desc.pixelsSize + vec2(2.0f)));
}

/*
 * Private
 */

CharacterGeneratorPrivate::CharacterGeneratorPrivate(const std::string& face,
	const std::string&, size_t size, size_t texSize) : fontFace(face), fontSize(size),
	_placer(vec2i(static_cast<int>(texSize)), false), textureSize(texSize)
{
	NSString* cFace = [NSString stringWithUTF8String:face.c_str()];
	
	font = [[NSFontManager sharedFontManager] fontWithFamily:cFace traits:0 weight:5 size:128.0f];
	if (font == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		font = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:0 weight:5 size:128.0f];
	}
	ET_ASSERT(font);
	
	boldFont = [[NSFontManager sharedFontManager] fontWithFamily:cFace traits:NSBoldFontMask weight:5 size:128.0f];
	if (boldFont == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		boldFont = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:NSBoldFontMask weight:5 size:128.0f];
	}
	ET_ASSERT(boldFont);
	
	whiteColor = [NSColor whiteColor];
	colorSpace = CGColorSpaceCreateDeviceGray();
	ET_ASSERT(colorSpace);
	
#if (!ET_OBJC_ARC_ENABLED)
	[font retain];
	[boldFont retain];
	[whiteColor retain];
#endif
}

CharacterGeneratorPrivate::~CharacterGeneratorPrivate()
{
	CGColorSpaceRelease(colorSpace);

#if (!ET_OBJC_ARC_ENABLED)
	[whiteColor release];
	[font release];
	[boldFont release];
#endif
}

void CharacterGeneratorPrivate::updateTexture(RenderContext* rc, const vec2i& position,
	const vec2i& size, Texture texture, BinaryDataStorage& data)
{
	vec2i dest(position.x, static_cast<int>(textureSize) - position.y - size.y);
	texture->updatePartialDataDirectly(rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGeneratorPrivate::renderCharacter(NSAttributedString* value,
	const vec2i& size, NSFont*, BinaryDataStorage& data)
{
	CGContextRef context = CGBitmapContextCreateWithData(data.data(), size.x, size.y, 8, size.x,
		colorSpace, kCGImageAlphaNone, nil, nil);
	ET_ASSERT(context);

	NSGraphicsContext* aContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
	
	[NSGraphicsContext setCurrentContext:aContext];
	[value drawAtPoint:NSMakePoint(0.0f, 0.0f)];
	[aContext flushGraphics];
	
	CGContextRelease(context);
}

struct SDFPoint
{
	short dx = 0;
	short dy = 0;
	int f = 0;
	
	SDFPoint()
		{ }
	
	SDFPoint(int x, int y, float af) :
		dx(x & 0xffff), dy(y & 0xffff), f(static_cast<int>(af)) { }
};

struct Grid
{
	int w;
	int h;
	DataStorage<SDFPoint> grid;
};

const SDFPoint pointInside = { 0, 0, 255 };
const SDFPoint pointEmpty = { 0, 0, 0 };

static inline SDFPoint Get(Grid& g, int x, int y)
{
	return g.grid[y * (g.w + 2) + x];
}

static inline void Put(Grid& g, int x, int y, const SDFPoint &p)
{
	g.grid[y * (g.w + 2) + x] = p;
}

static void Compare(Grid& g, SDFPoint& p, int x, int y, int offsetx, int offsety)
{
	int add = 0;
	
	SDFPoint other = Get(g, x + offsetx, y + offsety);
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
		if (offsety == 0) {
			p.dx = other.dx + 1;
			p.dy = other.dy;
		}
		else if(offsetx == 0) {
			p.dy = other.dy + 1;
			p.dx = other.dx;
		}
		else {
			p.dy = other.dy + 1;
			p.dx = other.dx + 1;
		}
	}
}

static void GenerateSDF(Grid &g)
{
	for (int y = 1; y <= g.h; y++)
	{
		for (int x = 1; x <= g.w; x++)
		{
			SDFPoint p = Get(g, x, y);
			Compare(g, p, x, y, -1,  0);
			Compare(g, p, x, y, 0, -1);
			Compare(g, p, x, y,  -1, -1);
			Compare(g, p, x, y,   1, -1);
			Put(g, x, y, p);
		}
	}
	
	for(int y = g.h; y > 0; y--)
	{
		for(int x = g.w; x > 0; x--)
		{
			SDFPoint p = Get(g, x, y);
			Compare(g, p, x, y,   1, 0);
			Compare(g, p, x, y,   0, 1);
			Compare(g, p, x, y,  -1, 1);
			Compare(g, p, x, y,   1, 1);
			Put(g, x, y, p);
		}
	}
	
}

void CharacterGeneratorPrivate::processImageData(BinaryDataStorage& data, const vec2i& size)
{
	int x = 0, y = 0;
	int w = size.x;
	int h = size.y;
	
	Grid grid[2];
	
	grid[0].w = grid[1].w = w;
	grid[0].h = grid[1].h = h;
	
	grid[0].grid.resize((w + 2) * (h + 2));
	grid[1].grid.resize((w + 2) * (h + 2));

	for(x = 0; x < w + 2; x++)
	{
		Put(grid[0], x, 0, pointEmpty);
		Put(grid[1], x, 0, pointInside);
		Put(grid[0], x, h + 1, pointEmpty);
		Put(grid[1], x, h + 1, pointInside);
	}
	
	for (y = 1; y <= h; y++)
	{
		Put(grid[0], 0, y, pointEmpty);
		Put(grid[1], 0, y, pointInside);
		Put(grid[0], w + 1, y, pointEmpty);
		Put(grid[1], w + 1, y, pointInside);
	}
	
	for (y = 1; y <= h; y++)
	{
		for (x = 1; x <= w; x++)
		{
			auto val = data[(x - 1) + size.x * (y - 1)];
			Put(grid[0], x, y, SDFPoint(val, val, val));
			Put(grid[1], x, y, SDFPoint(0, 0, 255 - val));
		}
	}
	
	GenerateSDF(grid[0]);
	GenerateSDF(grid[1]);
	
	DataStorage<float> distances(w * h, 0);
	for (y = 1; y <= h; y++)
	{
		for (x = 1; x <= w; x++)
		{
			float dist1 = std::sqrt(static_cast<float>(Get(grid[0], x, y).f + 1));
			float dist2 = std::sqrt(static_cast<float>(Get(grid[1], x, y).f + 1));
			distances[(x-1) + size.x * (y-1)] = 127.0f + 8.0f * (dist1 - dist2);// 127.0f + 8.0f * (dist1 - dist2);
		}
	}

	/* */
	DataStorage<float> smooth(w * h, 0);
	for (int i = 0; i < 5; ++i)
	{
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			int index = x + size.x * y;
			int prev = etMax(0, x-1) + size.x * y;
			int next = etMin(w-1, x+1) + size.x * y;
			smooth[index] = (distances[prev] + distances[index] + distances[next]) / 3.0f;
		}
	}

	for (x = 0; x < w; x++)
	{
		for (y = 0; y < h; y++)
		{
			int index = x + size.x * y;
			int prev = x + size.x * etMax(0, y-1);
			int next = x + size.x * etMin(h-1, y+1);
			distances[index] = (smooth[prev] + smooth[index] + smooth[next]) / 3.0f;
		}
	}
	}
	// */
	
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			int index = x + size.x * y;
			data[index] = static_cast<unsigned char>(clamp(distances[index], 0.0f, 255.0f));
		}
	}
}
