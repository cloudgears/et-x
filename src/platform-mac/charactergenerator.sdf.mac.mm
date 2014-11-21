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
#include <et/imaging/imageoperations.h>
#include <et-ext/scene2d/charactergenerator.h>
#include <et/imaging/imagewriter.h>

using namespace et;
using namespace et::s2d;

struct SDFPoint
{
	int dx = 0;
	int dy = 0;
	int f = 0;
	
	SDFPoint(int x, int y, int af) :
		dx(x), dy(y), f(af) { }
};

struct SDFGrid
{
	int w = 0;
	int h = 0;
	DataStorage<SDFPoint> grid;
};

class et::s2d::CharacterGeneratorPrivate
{
public:
	CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace);
	~CharacterGeneratorPrivate();
	
	void updateTexture(RenderContext* rc, const vec2i& position, const vec2i& size,
		Texture texture, BinaryDataStorage& data);
	
	void renderCharacter(NSAttributedString* value, const vec2i& offset, const vec2i& size,
		NSFont* font, BinaryDataStorage& data);
	
	void generateSignedDistanceField(BinaryDataStorage&, const vec2i&);
	BinaryDataStorage downsample(BinaryDataStorage&, vec2i size);
	
public:
	std::string fontFace;
	std::string boldFontFace;
	
	size_t textureSize = 0;
	
	RectPlacer _placer;
	
	NSFont* font = nil;
	NSFont* boldFont = nil;
	NSColor* whiteColor = nil;

	CGColorSpaceRef colorSpace = nullptr;
};

const int defaultTextureSize = 2048;
const int baseFontSize = 256.0f;
const SDFPoint pointEmpty = { 0, 0, 0 };
const SDFPoint pointInside = { 0, 0, 255 };

void GenerateSDF(SDFGrid &g);

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string& boldFace) : _rc(rc)
{
	ET_PIMPL_INIT(CharacterGenerator, face, boldFace)
	
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D,
#	if defined(GL_R8)
		GL_R8,
#	else
		GL_RED
#	endif
		vec2i(defaultTextureSize), GL_RED, GL_UNSIGNED_BYTE, BinaryDataStorage(sqr(defaultTextureSize), 0), face + "font");
}

CharacterGenerator::~CharacterGenerator()
{
	ET_PIMPL_FINALIZE(CharacterGenerator)
}

CharDescriptor CharacterGenerator::generateCharacter(int value, CharacterFlags flags)
{
	bool isBold = (flags & CharacterFlag_Bold) == CharacterFlag_Bold;
	
	wchar_t string[2] = { value, 0 };
	
	NSString* wString = [[NSString alloc] initWithBytesNoCopy:string length:sizeof(string)
		encoding:NSUTF32LittleEndianStringEncoding freeWhenDone:NO];

	NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString:wString];

	NSRange wholeString = NSMakeRange(0, [attrString length]);
	[attrString addAttribute:NSFontAttributeName value:isBold ? _private->boldFont : _private->font range:wholeString];
	[attrString addAttribute:NSForegroundColorAttributeName value:_private->whiteColor range:wholeString];

	NSSize characterSize = [attrString size];
	
	vec2i charSize = vec2i(static_cast<int>(characterSize.width),
		static_cast<int>((value == ET_NEWLINE) || (value == ET_RETURN) ? baseSize() : characterSize.height));
	
	CharDescriptor desc(value);
	
	if (charSize.square() > 0)
	{
		desc.originalSize = vector2ToFloat(charSize);
		
		const vec2i extent = vec2i(32);
		vec2i canvasSize = charSize + extent;
		
		BinaryDataStorage dataToProcess(canvasSize.square(), 0);
		_private->renderCharacter(attrString, extent / 2, canvasSize, isBold ? _private->boldFont : _private->font, dataToProcess);
		
		_private->generateSignedDistanceField(dataToProcess, canvasSize);
		
		vec2i topLeftOffset = canvasSize - vec2i(1);
		vec2i bottomRightOffset = vec2i(0);
		
		vec2i pixel;
		for (pixel.y = 0; pixel.y < canvasSize.y - 1; ++pixel.y)
		{
			for (pixel.x = 0; pixel.x < canvasSize.x - 1; ++pixel.x)
			{
				size_t i = pixel.x + (canvasSize.y - pixel.y - 1) * canvasSize.x;
				if (dataToProcess[i])
				{
					topLeftOffset = minv(topLeftOffset, pixel);
					bottomRightOffset = maxv(bottomRightOffset, pixel);
				}
			}
		}
		bottomRightOffset = minv(bottomRightOffset + vec2i(1), canvasSize);
		
		vec2i sizeToSave = bottomRightOffset - topLeftOffset;
		if ((sizeToSave.x > 0) && (sizeToSave.y > 0))
		{
			BinaryDataStorage dataToSave(sizeToSave.square(), 0);
			vec2 targetPixel;
			for (pixel.y = topLeftOffset.y; pixel.y < bottomRightOffset.y; ++pixel.y, ++targetPixel.y)
			{
				targetPixel.x = 0;
				for (pixel.x = topLeftOffset.x; pixel.x < bottomRightOffset.x; ++pixel.x, ++targetPixel.x)
				{
					size_t i = targetPixel.x + (sizeToSave.y - targetPixel.y - 1) * sizeToSave.x;
					size_t j = pixel.x + (canvasSize.y - pixel.y - 1) * canvasSize.x;
					dataToSave[i] = dataToProcess[j];
				}
			}
			
			vec2i downsampledSize = sizeToSave / 2;
			auto downsampled = _private->downsample(dataToSave, sizeToSave);
			
			recti textureRect;
			if (_private->_placer.place(downsampledSize, textureRect))
			{
				_private->updateTexture(_rc, textureRect.origin(), downsampledSize, _texture, downsampled);
				
				desc.contentRect = rect(vector2ToFloat(topLeftOffset - extent / 2), vector2ToFloat(sizeToSave));
				
				desc.uvRect = rect(_texture->getTexCoord(vector2ToFloat(textureRect.origin())),
					vector2ToFloat(textureRect.size()) / _texture->sizeFloat());
			}
			else
			{
				log::error("Failed to place character to the texture: %d (%c, %C)", value, static_cast<char>(value),
					static_cast<wchar_t>(value));
			}
		}
	}
	
#if (!ET_OBJC_ARC_ENABLED)
	[attrString release];
	[wString release];
#endif
	
	characterGenerated.invoke(value);
	
	_chars[value] = desc;
	return desc;
}

void CharacterGenerator::setTexture(Texture tex)
{
	_texture = tex;
}

void CharacterGenerator::pushCharacter(const et::s2d::CharDescriptor& desc)
{
	ET_FAIL("Implement me")
	
	/*
	if (desc.params & CharParameter_Bold)
		_boldChars[desc.value] = desc;
	else
		_chars[desc.value] = desc;

	_private->_placer.addPlacedRect(recti(static_cast<int>(desc.pixelsOrigin.x - 1.0f),
		static_cast<int>(desc.pixelsOrigin.y - 1.0f), static_cast<int>(desc.pixelsSize.x + 2.0f),
		static_cast<int>(desc.pixelsSize.y + 2.0f)));
	*/
}

const std::string& CharacterGenerator::face() const
	{ return _private->fontFace; }

const std::string& CharacterGenerator::boldFace() const
	{ return _private->boldFontFace; }

size_t CharacterGenerator::baseSize() const
	{ return static_cast<size_t>(baseFontSize); }

/*
 * Private
 */
CharacterGeneratorPrivate::CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace) :
	fontFace(face), boldFontFace(boldFace), textureSize(defaultTextureSize),
	_placer(vec2i(static_cast<int>(defaultTextureSize)), true)
{
	auto floatSize = static_cast<float>(baseFontSize);
	
	NSString* cFace = [NSString stringWithUTF8String:fontFace.c_str()];
	font = [[NSFontManager sharedFontManager] fontWithFamily:cFace traits:0 weight:5 size:floatSize];
	if (font == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", fontFace.c_str());
		font = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:0 weight:5 size:floatSize];
	}
	ET_ASSERT(font);
	
	NSString* cBoldFace = [NSString stringWithUTF8String:boldFontFace.c_str()];
	boldFont = [[NSFontManager sharedFontManager] fontWithFamily:cBoldFace traits:NSBoldFontMask weight:5 size:floatSize];
	if (boldFont == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", boldFontFace.c_str());
		boldFont = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:NSBoldFontMask weight:5 size:floatSize];
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
	vec2i dest(position.x, texture->size().y - position.y - size.y - 1);
	texture->updatePartialDataDirectly(rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGeneratorPrivate::renderCharacter(NSAttributedString* value,
	const vec2i& offset, const vec2i& size, NSFont*, BinaryDataStorage& data)
{
	CGContextRef context = CGBitmapContextCreateWithData(data.data(), size.x, size.y, 8, size.x,
		colorSpace, kCGImageAlphaNone, nil, nil);
	ET_ASSERT(context);

	NSGraphicsContext* aContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
	
	[NSGraphicsContext setCurrentContext:aContext];
	[value drawAtPoint:NSMakePoint(offset.x, offset.y)];
	[aContext flushGraphics];
	
	CGContextRelease(context);
}

#define get(g, x, y)		g.grid[(y) * (2 + g.w) + (x)]
#define put(g, x, y, p)		g.grid[(y) * (2 + g.w) + (x)] = p;

void CharacterGeneratorPrivate::generateSignedDistanceField(BinaryDataStorage& data, const vec2i& size)
{
	int x = 0, y = 0;
	int w = size.x;
	int h = size.y;
	
	SDFGrid grid[2];
	
	grid[0].w = grid[1].w = w;
	grid[0].h = grid[1].h = h;
	
	grid[0].grid.resize((w + 2) * (h + 2));
	grid[0].grid.fill(0);
	grid[1].grid.resize((w + 2) * (h + 2));
	grid[1].grid.fill(0);

	for (x = 0; x < w + 2; x++)
	{
		put(grid[0], x, 0, pointInside);
		put(grid[1], x, 0, pointInside);
		put(grid[0], x, h + 1, pointInside);
		put(grid[1], x, h + 1, pointInside);
	}
	
	for (y = 1; y <= h; y++)
	{
		put(grid[0], 0, y, pointInside);
		put(grid[1], 0, y, pointInside);
		put(grid[0], w + 1, y, pointInside);
		put(grid[1], w + 1, y, pointInside);
	}
	
	size_t k = 0;
	for (y = 1; y <= h; y++)
	{
		for (x = 1; x <= w; x++)
		{
			auto val = data[k++];
			put(grid[0], x, y, SDFPoint(val, val, val));
			put(grid[1], x, y, SDFPoint(0, 0, 255 - val));
		}
	}
	
	GenerateSDF(grid[0]);
	GenerateSDF(grid[1]);
	
	DataStorage<float> distances(w * h, 0);
	k = 0;
	for (y = 1; y <= h; y++)
	{
		for (x = 1; x <= w; x++)
		{
			float dist1 = std::sqrt(static_cast<float>(get(grid[0], x, y).f + 1));
			float dist2 = std::sqrt(static_cast<float>(get(grid[1], x, y).f + 1));
			distances[k++] = 127.0f + 8.4666666666666668f * (dist1 - dist2);
		}
	}
	
	/* */
	DataStorage<float> smooth(w * h, 0);
	for (int i = 0; i < 2; ++i)
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
	
	k = 0;
	for (y = 0; y < h; y++)
	{
		for (x = 0; x < w; x++)
		{
			data[k] = static_cast<unsigned char>(clamp(distances[k], 0.0f, 255.0f));
			++k;
		}
	}
}

BinaryDataStorage CharacterGeneratorPrivate::downsample(BinaryDataStorage& input, vec2i size)
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


/*
 * SDF Service
 */
void Compare(SDFGrid& g, SDFPoint& p, int x, int y, int offsetx, int offsety)
{
	int add = 0;
	
	SDFPoint other = get(g, x + offsetx, y + offsety);
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

void GenerateSDF(SDFGrid &g)
{
	for (int y = 1; y <= g.h; y++)
	{
		for (int x = 1; x <= g.w; x++)
		{
			SDFPoint p = get(g, x, y);
			Compare(g, p, x, y, -1,  0);
			Compare(g, p, x, y, 0, -1);
			Compare(g, p, x, y,  -1, -1);
			Compare(g, p, x, y,   1, -1);
			put(g, x, y, p);
		}
	}
	
	for(int y = g.h; y > 0; y--)
	{
		for(int x = g.w; x > 0; x--)
		{
			SDFPoint p = get(g, x, y);
			Compare(g, p, x, y,   1, 0);
			Compare(g, p, x, y,   0, 1);
			Compare(g, p, x, y,  -1, 1);
			Compare(g, p, x, y,   1, 1);
			put(g, x, y, p);
		}
	}
	
}
