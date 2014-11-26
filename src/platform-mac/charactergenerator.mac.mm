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
#include <et-ext/scene2d/charactergenerator.h>
#include <et/platform-apple/apple.h>

using namespace et;
using namespace et::s2d;

class et::s2d::CharacterGeneratorImplementationPrivate
{
public:
	CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace);
	~CharacterGeneratorImplementationPrivate();
	
	bool startWithCharacter(const CharDescriptor& desc, vec2i& charSize, vec2i& canvasSize, BinaryDataStorage& charData);
	
public:
	NSFont* font = nil;
	NSFont* boldFont = nil;
	NSColor* whiteColor = nil;
	CGColorSpaceRef colorSpace = nullptr;
};

CharacterGeneratorImplementation::CharacterGeneratorImplementation(const std::string& face, const std::string& boldFace)
	{ ET_PIMPL_INIT(CharacterGeneratorImplementation, face, boldFace) }

CharacterGeneratorImplementation::~CharacterGeneratorImplementation()
	{ ET_PIMPL_FINALIZE(CharacterGeneratorImplementation) }

bool CharacterGeneratorImplementation::processCharacter(const CharDescriptor& a, vec2i& b, vec2i& c, BinaryDataStorage& d)
	{ return _private->startWithCharacter(a, b, c, d); }

CharacterGeneratorImplementationPrivate::CharacterGeneratorImplementationPrivate(const std::string& face, const std::string& boldFace)
{
	colorSpace = CGColorSpaceCreateDeviceGray();
	
	font = [[NSFontManager sharedFontManager] fontWithFamily:[NSString stringWithUTF8String:face.c_str()]
		traits:0 weight:5 size:CharacterGenerator::baseFontSize];
	if (font == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		font = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:0 weight:5 size:CharacterGenerator::baseFontSize];
	}
	
	boldFont = [[NSFontManager sharedFontManager] fontWithFamily:[NSString stringWithUTF8String:boldFace.c_str()]
		traits:NSBoldFontMask weight:5 size:CharacterGenerator::baseFontSize];
	if (boldFont == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", boldFace.c_str());
		boldFont = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:NSBoldFontMask weight:5 size:CharacterGenerator::baseFontSize];
	}
		
	whiteColor = [NSColor whiteColor];
	
	(void)ET_OBJC_RETAIN(font);
	(void)ET_OBJC_RETAIN(boldFont);
	(void)ET_OBJC_RETAIN(whiteColor);
}

CharacterGeneratorImplementationPrivate::~CharacterGeneratorImplementationPrivate()
{
	CGColorSpaceRelease(colorSpace);
	
	ET_OBJC_RELEASE(whiteColor);
	ET_OBJC_RELEASE(font);
	ET_OBJC_RELEASE(boldFont);
}

bool CharacterGeneratorImplementationPrivate::startWithCharacter(const CharDescriptor& desc, vec2i& charSize,
	vec2i& canvasSize, BinaryDataStorage& charData)
{
	if ((desc.value == ET_NEWLINE) || (desc.value == ET_RETURN))
		return false;
	
	unichar currentChar = static_cast<unichar>(desc.value);
	NSString* nsString = [[NSString alloc] initWithCharactersNoCopy:&currentChar length:1 freeWhenDone:NO];
	
	if (nsString == nil)
		return false;
	
	NSMutableAttributedString* attrString = [[NSMutableAttributedString alloc] initWithString:nsString attributes:
	@{
		NSFontAttributeName : (desc.flags & CharacterFlag_Bold) ? boldFont : font,
		NSForegroundColorAttributeName : whiteColor
	}];
	
	NSSize characterSize = [attrString size];
	charSize = vec2i(static_cast<int>(characterSize.width), static_cast<int>(characterSize.height));
	if (charSize.dotSelf() <= 0)
		return false;
	
	canvasSize = charSize + CharacterGenerator::charactersRenderingExtent;
	charData.resize(canvasSize.square());
	charData.fill(0);
	
	CGContextRef context = CGBitmapContextCreateWithData(charData.data(), canvasSize.x, canvasSize.y, 8,
		canvasSize.x, colorSpace, kCGImageAlphaNone, nil, nil);
	if (context == nil) return false;
	
	NSGraphicsContext* aContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
	[NSGraphicsContext setCurrentContext:aContext];
	
	[attrString drawAtPoint:NSMakePoint(CharacterGenerator::charactersRenderingExtent.x / 2,
		CharacterGenerator::charactersRenderingExtent.x / 2)];
	
	[aContext flushGraphics];
	CGContextRelease(context);
	
	return true;
}
