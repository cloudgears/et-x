#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>

#include <et/rendering/rendercontext.h>
#include <et/geometry/rectplacer.h>
#include <et-ext/scene2d/charactergenerator.h>
#include <et/imaging/imagewriter.h>

using namespace et;
using namespace et::s2d;

const size_t defaultTextureSize = 1024;

class et::s2d::CharacterGeneratorPrivate
{
public:
	CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace, size_t size);
	~CharacterGeneratorPrivate();
	
	void updateTexture(RenderContext* rc, const vec2i& position, const vec2i& size,
		Texture texture, BinaryDataStorage& data);
	
	void renderCharacter(NSAttributedString* value, const vec2i& size,
		NSFont* font, BinaryDataStorage& data);
	
public:
	std::string fontFace;
	size_t fontSize;
	
	RectPlacer _placer;
	
	NSFont* font;
	NSFont* boldFont;
	NSColor* whiteColor;

	CGColorSpaceRef colorSpace;
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string& boldFace, size_t size) : _rc(rc),
	_private(new CharacterGeneratorPrivate(face, boldFace, size)), _face(face), _size(size)
{
	BinaryDataStorage emptyData(defaultTextureSize * defaultTextureSize * 4, 0);
	
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(defaultTextureSize),
												GL_RGBA, GL_UNSIGNED_BYTE, emptyData, face + "font");
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
        static_cast<int>(characterSize.height));
	
	CharDescriptor desc(value);
	
	if (charSize.square() > 0)
	{
		BinaryDataStorage data(charSize.square() * 4, 0);
		
		rect textureRect;
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);

		desc.origin = textureRect.origin() + vec2(1.0f);
		desc.size = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.origin);
		desc.uvSize = desc.size / _texture->sizeFloat();
	}

	[attrString release];
	[wString release];
	
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
		BinaryDataStorage data(charSize.square() * 4, 0);

		rect textureRect;
		
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(attrString, charSize, _private->font, data);
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);

		desc.origin = textureRect.origin() + vec2(1.0f);
		desc.size = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.origin);
		desc.uvSize = desc.size / _texture->sizeFloat();
	}

	[attrString release];
	[wString release];
	
	_boldChars[value] = desc;
	return desc;
}

/*
 * Private
 */

CharacterGeneratorPrivate::CharacterGeneratorPrivate(const std::string& face,
	const std::string&, size_t size) : fontFace(face), fontSize(size),
	_placer(vec2i(defaultTextureSize), true)
{
    NSString* cFace = [NSString stringWithCString:face.c_str() encoding:NSUTF8StringEncoding];
	
	font = [[[NSFontManager sharedFontManager] fontWithFamily:cFace
		traits:0 weight:0 size:size] retain];
	
	if (font == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		font = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:0 weight:0 size:size];
	}
	assert(font);
	
	boldFont = [[[NSFontManager sharedFontManager] fontWithFamily:cFace
		traits:NSBoldFontMask weight:0 size:size] retain];
	
	if (boldFont == nil)
	{
		log::error("Font %s not found. Using default font (Arial)", face.c_str());
		boldFont = [[NSFontManager sharedFontManager] fontWithFamily:@"Arial" traits:0 weight:0 size:size];
	}
	assert(boldFont);
	
	whiteColor = [[NSColor whiteColor] retain];
	assert(whiteColor);
	
	colorSpace = CGColorSpaceCreateDeviceRGB();
	assert(colorSpace);
}

CharacterGeneratorPrivate::~CharacterGeneratorPrivate()
{
	CGColorSpaceRelease(colorSpace);

	[whiteColor release];
    [font release];
	[boldFont release];
}

void CharacterGeneratorPrivate::updateTexture(RenderContext* rc, const vec2i& position,
	const vec2i& size, Texture texture, BinaryDataStorage& data)
{
	vec2i dest(position.x, defaultTextureSize - position.y - size.y);
	texture->updatePartialDataDirectly(rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGeneratorPrivate::renderCharacter(NSAttributedString* value,
	const vec2i& size, NSFont*, BinaryDataStorage& data)
{
	CGContextRef context = CGBitmapContextCreateWithData(data.data(), size.x, size.y, 8, 4 * size.x,
		colorSpace, kCGImageAlphaPremultipliedLast, nil, nil);
	assert(context);

	NSGraphicsContext* aContext = [NSGraphicsContext graphicsContextWithGraphicsPort:context flipped:YES];
	[NSGraphicsContext setCurrentContext:aContext];

	[value drawAtPoint:NSMakePoint(0.0, 0.0)];
	
	[aContext flushGraphics];
	CGContextRelease(context);
	
	unsigned int* ptr = reinterpret_cast<unsigned int*>(data.data());
	unsigned int* endPtr = ptr + data.dataSize() / 4;
	while (ptr != endPtr)
		*ptr++ |= 0x00FFFFFF;
}
