#include <et/geometry/rectplacer.h>
#include <et/rendering/rendercontext.h>
#include <et-ext/scene2d/charactergenerator.h>

using namespace et;
using namespace et::s2d;

class et::s2d::CharacterGeneratorPrivate
{
public:
	size_t texSize;
	RectPlacer placer;
	
	CharacterGeneratorPrivate(size_t ts) :
		texSize(ts), placer(vec2i(static_cast<int>(ts)), false)
	{
		
	}
};

CharacterGenerator::CharacterGenerator(RenderContext* rc, const std::string& face,
	const std::string&, size_t size, size_t textureSize) : _rc(rc),
	_private(new CharacterGeneratorPrivate(textureSize)), _face(face), _size(size)
{
	_texture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(textureSize),
		GL_RGBA, GL_UNSIGNED_BYTE, BinaryDataStorage(4 * textureSize * textureSize, 0), face + "font");
}

CharacterGenerator::~CharacterGenerator()
{
	delete _private;
}

CharDescriptor CharacterGenerator::generateCharacter(int value, bool)
{
	CharDescriptor desc(value);
	_chars[value] = desc;
	return desc;
}

CharDescriptor CharacterGenerator::generateBoldCharacter(int value, bool)
{
	CharDescriptor desc(value, CharParameter_Bold);
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
	
	_private->placer.addPlacedRect(rect(desc.pixelsOrigin - vec2(1.0f), desc.pixelsSize + vec2(2.0f)));
}
