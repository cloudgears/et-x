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
	const std::string& boldFace, size_t size, size_t texSize) : _rc(rc),
	_private(new CharacterGeneratorPrivate(face, boldFace, size, texSize)), _face(face), _size(size)
{
	BinaryDataStorage emptyData(sqr(texSize), 0);
	
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RED, vec2i(static_cast<int>(texSize)),
		GL_RED, GL_UNSIGNED_BYTE, emptyData, face + "font");
}

CharacterGenerator::~CharacterGenerator()
{
	delete _private;
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
		static_cast<int>((value == ET_NEWLINE) || (value == ET_RETURN) ? lineHeight() : characterSize.height));
	
	CharDescriptor desc(value);
	
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square(), 0);
		
		rect textureRect;
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
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
	vec2i charSize = vec2i(static_cast<int>(characterSize.width + 0.5f),
		static_cast<int>((value == ET_NEWLINE) || (value == ET_RETURN) ? lineHeight() : characterSize.height + 0.5f));
	
	CharDescriptor desc(value, CharParameter_Bold);
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square(), 0);

		rect textureRect;
		
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
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
	
	_private->_placer.addPlacedRect(rect(desc.pixelsOrigin - vec2(1.0f), desc.pixelsSize + vec2(2.0f))
);
}

/*
 * Private
 */

CharacterGeneratorPrivate::CharacterGeneratorPrivate(const std::string& face,
	const std::string&, size_t size, size_t texSize) : fontFace(face), fontSize(size),
	_placer(vec2i(static_cast<int>(texSize)), false), textureSize(texSize)
{
    NSString* cFace = [NSString stringWithUTF8String:face.c_str()];
	
	font = [[NSFontManager sharedFontManager] fontWithFamily:cFace
		traits:0 weight:5 size:size];
	
	if (font == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		font = [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica" traits:0 weight:5 size:size];
	}
	ET_ASSERT(font);
	
	boldFont = [[NSFontManager sharedFontManager] fontWithFamily:cFace
		traits:NSBoldFontMask weight:5 size:size];
	
	if (boldFont == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		boldFont = [[NSFontManager sharedFontManager] fontWithFamily:@"Helvetica"
			traits:NSBoldFontMask weight:5 size:size];
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
