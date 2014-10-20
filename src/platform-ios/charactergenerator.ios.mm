#include <TargetConditionals.h>

#include <UIKit/UIFont.h>
#include <UIKit/UIColor.h>
#include <UIKit/UIStringDrawing.h>
#include <UIKit/NSStringDrawing.h>
#include <UIKit/UIGraphics.h>

#include <et/rendering/rendercontext.h>
#include <et/geometry/rectplacer.h>
#include <et/imaging/imagewriter.h>
#include <et-ext/scene2d/charactergenerator.h>

using namespace et;
using namespace et::s2d;

class et::s2d::CharacterGeneratorPrivate
{
	public:
		CharacterGeneratorPrivate(const std::string& face, const std::string& boldFace, size_t size,
			size_t textureSize);
		~CharacterGeneratorPrivate();

		void updateTexture(RenderContext* rc, const vec2i& position, const vec2i& size,
			Texture texture, BinaryDataStorage& data);
	
		void renderCharacter(NSString* value, const vec2i& size, bool bold, BinaryDataStorage& data);

	public:
		std::string _fontFace;
		size_t _fontSize;
		size_t _textureSize;
	
		RectPlacer _placer;
   
        UIFont* font;
        UIFont* boldFont;
		UIColor* whiteColor;
		CGColorRef whiteColorRef;
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string& boldFace, size_t size, size_t texSize) : _rc(rc),
	_private(new CharacterGeneratorPrivate(face, boldFace, size, texSize)), _face(face), _size(size)
{
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_LUMINANCE, vec2i(static_cast<int>(texSize)),
		GL_LUMINANCE, GL_UNSIGNED_BYTE, BinaryDataStorage(sqr(texSize), 0), face + "font");
}

CharacterGenerator::~CharacterGenerator()
{
	delete _private;
}

CharDescriptor CharacterGenerator::generateCharacter(int value, bool)
{
	wchar_t string[2] = { value, 0 };
	
    NSString* wString = [[NSString alloc] initWithBytes:string length:sizeof(string)
		encoding:NSUTF32LittleEndianStringEncoding];
	
#if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_7_0)
	CGSize characterSize = [wString sizeWithAttributes:@{NSFontAttributeName:_private->font}];
#else
	CGSize characterSize = [wString sizeWithFont:_private->_font];
#endif
	
	vec2i charSize = vec2i(static_cast<int>(characterSize.width + 0.5f),
		static_cast<int>((value == ET_NEWLINE) || (value == ET_RETURN) ? lineHeight() : characterSize.height + 0.5f));
	
	CharDescriptor desc(value);
	if (charSize.square() > 0)
	{
		rect textureRect;
		
		BinaryDataStorage data(charSize.square(), 0);
		
		_private->_placer.place(charSize + vec2i(2), textureRect);
		
		_private->renderCharacter(wString, charSize, false, data);
		
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);
		
		desc.pixelsOrigin = textureRect.origin() + vec2(1.0f);
		desc.pixelsSize = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.pixelsOrigin);
		desc.uvSize = desc.pixelsSize / _texture->sizeFloat();
	}
	
#if (!ET_OBJC_ARC_ENABLED)
	[wString release];
#endif
	
	characterGenerated.invoke(value);
	
	return (_chars[value] = desc);
}

CharDescriptor CharacterGenerator::generateBoldCharacter(int value, bool)
{
	wchar_t string[2] = { value, 0 };
    NSString* wString = [[NSString alloc] initWithBytes:string length:sizeof(string)
		encoding:NSUTF32LittleEndianStringEncoding];
	
#if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_7_0)
	CGSize characterSize = [wString sizeWithAttributes:@{NSFontAttributeName:_private->boldFont}];
#else
	CGSize characterSize = [wString sizeWithFont:_private->_boldFont];
#endif

	vec2i charSize = vec2i(static_cast<int>(characterSize.width + 0.5f),
		static_cast<int>((value == ET_NEWLINE) || (value == ET_RETURN) ? lineHeight() : characterSize.height + 0.5f));
	
	CharDescriptor desc(value, CharParameter_Bold);
	if (charSize.square() > 0)
	{
		rect textureRect;
		
		BinaryDataStorage data(charSize.square(), 0);
		_private->_placer.place(charSize + vec2i(2), textureRect);
		_private->renderCharacter(wString, charSize, true, data);
		_private->updateTexture(_rc, vec2i(static_cast<int>(textureRect.left + 1.0f),
			static_cast<int>(textureRect.top + 1.0f)), charSize, _texture, data);
		
		desc.pixelsOrigin = textureRect.origin() + vec2(1.0f);
		desc.pixelsSize = textureRect.size() - vec2(2.0f);
		desc.uvOrigin = _texture->getTexCoord(desc.pixelsOrigin);
		desc.uvSize = desc.pixelsSize / _texture->sizeFloat();
	}
	
#if (!ET_OBJC_ARC_ENABLED)
	[wString release];
#endif
	
	characterGenerated.invoke(value);
	
	return (_boldChars[value] = desc);
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
	const std::string& boldFace, size_t size, size_t texSize) : _fontFace(face), _fontSize(size),
	_placer(vec2i(static_cast<int>(texSize)), false)
{
    NSString* cFace = [NSString stringWithCString:face.c_str() encoding:NSASCIIStringEncoding];
    NSString* cBoldFace = [NSString stringWithCString:boldFace.c_str() encoding:NSASCIIStringEncoding];
	
    font = [UIFont fontWithName:cFace size:static_cast<float>(size)];
	if (font == nil)
	{
		log::warning("Font %s not found, using system font.", face.c_str());
		font = [UIFont systemFontOfSize:static_cast<float>(size)];
	}
	ET_ASSERT(font);
	
    boldFont = [UIFont fontWithName:cBoldFace size:static_cast<float>(size)];
	if (boldFont == nil)
	{
		log::warning("Font %s not found, using system bold font.", boldFace.c_str());
		boldFont = [UIFont boldSystemFontOfSize:static_cast<float>(size)];
	}
	ET_ASSERT(boldFont);
	
	whiteColor = [UIColor whiteColor];
	
	whiteColorRef = [whiteColor CGColor];
	CGColorRetain(whiteColorRef);
	
#if (!ET_OBJC_ARC_ENABLED)
	[whiteColor retain];
	[font retain];
	[boldFont retain];
#endif
}

CharacterGeneratorPrivate::~CharacterGeneratorPrivate()
{
#if (!ET_OBJC_ARC_ENABLED)
    [font release];
	[boldFont release];
	[whiteColor release];
#endif
	
	CGColorRelease(whiteColorRef);
}

void CharacterGeneratorPrivate::updateTexture(RenderContext* rc, const vec2i& position,
	const vec2i& size, Texture texture, BinaryDataStorage& data)
{
	vec2i dest(position.x, _placer.contextSize().y - position.y - size.y);
	texture->updatePartialDataDirectly(rc, dest, size, data.binary(), data.dataSize());
}

void CharacterGeneratorPrivate::renderCharacter(NSString* value, const vec2i& size,
	bool bold, BinaryDataStorage& data)
{
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
	ET_ASSERT(colorSpace);
	
	CGContextRef context = CGBitmapContextCreateWithData(data.data(), size.x, size.y, 8,
		size.x, colorSpace, kCGImageAlphaNone, 0, 0);
	ET_ASSERT(context);
	
	CGContextSetTextDrawingMode(context, kCGTextFill);
	CGContextSetShouldAntialias(context, YES);
	CGContextSetShouldSmoothFonts(context, YES);
	CGContextSetShouldSubpixelPositionFonts(context, YES);
	CGContextSetShouldSubpixelQuantizeFonts(context, NO);
	UIGraphicsPushContext(context);

	UIFont* fontToDraw = (bold ? boldFont : font);
	
#if (__IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_7_0)
	[value drawAtPoint:CGPointZero withAttributes:@{NSFontAttributeName:fontToDraw,
		NSForegroundColorAttributeName:whiteColor}];
#else
	CGContextSetFillColorWithColor(context, whiteColor);
	[value drawAtPoint:CGPointZero withFont:fontToDraw];
#endif
	
	UIGraphicsPopContext();
	CGContextRelease(context);
	CGColorSpaceRelease(colorSpace);
}
