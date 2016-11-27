/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#pragma once

#include <et/rendering/interface/texture.h>
#include <et/app/events.h>
#include <et/geometry/rectplacer.h>
#include <et-ext/scene2d/fontbase.h>

namespace et
{
class RenderContext;

namespace s2d
{
namespace sdf
{
struct Grid
{
	int32_t w = 0;
	int32_t h = 0;
	vec2i grid[512 * 512];
};
}

class CharacterGeneratorImplementationPrivate;
class CharacterGeneratorImplementation
{
public:
	CharacterGeneratorImplementation(const std::string&, const std::string&, uint32_t, uint32_t);
	~CharacterGeneratorImplementation();

public:
	bool processCharacter(const CharDescriptor&, vec2i&, vec2i&, BinaryDataStorage&);

private:
	ET_DECLARE_PIMPL(CharacterGeneratorImplementation, 192);
};

class CharacterGenerator : public Object
{
public:
	ET_DECLARE_POINTER(CharacterGenerator);

	static const float baseFontSize;
	static const vec2i charactersRenderingExtent;

public:
	CharacterGenerator(RenderContext*, const std::string& face, const std::string& boldFace,
		uint32_t faceIndex = 0, uint32_t boldFaceIndex = 0);

	const Texture::Pointer& texture() const;
	const std::string& face() const;
	const std::string& boldFace() const;
	uint32_t charactersCount() const;
	const CharDescriptor& charDescription(wchar_t c);
	const CharDescriptor& boldCharDescription(wchar_t c);
	const CharDescriptorMap& characters() const;
	const CharDescriptorMap& boldCharacters() const;
	void setTexture(Texture::Pointer);
	void pushCharacter(const CharDescriptor&);

	ET_DECLARE_EVENT1(characterGenerated, int)

private:
	const CharDescriptor& generateCharacter(wchar_t, CharacterFlags);

	void generateSignedDistanceField(BinaryDataStorage&, int32_t, int32_t);
	void generateSignedDistanceFieldOnGrid(sdf::Grid&);

	bool performCropping(const BinaryDataStorage&, const vec2i&, BinaryDataStorage&, vec2i&, vec2i&);

	void updateTexture(const vec2i&, const vec2i&, BinaryDataStorage&);

	BinaryDataStorage downsample(BinaryDataStorage&, const vec2i&);

private:
	CharacterGeneratorImplementation _impl;
	et::Texture::Pointer _texture;
	std::string _fontFace;
	std::string _fontBoldFace;
	RectPlacer _placer;
	sdf::Grid _grid0;
	sdf::Grid _grid1;

	CharDescriptorMap _chars;
	CharDescriptorMap _boldChars;
};

inline const Texture::Pointer& CharacterGenerator::texture() const
{
	return _texture;
}

inline const std::string& CharacterGenerator::face() const
{
	return _fontFace;
}

inline const std::string& CharacterGenerator::boldFace() const
{
	return _fontBoldFace;
}

inline uint32_t CharacterGenerator::charactersCount() const
{
	return static_cast<uint32_t>(_chars.size() + _boldChars.size());
}

inline const CharDescriptor& CharacterGenerator::charDescription(wchar_t c)
{
	auto i = _chars.find(c);
	return (i != _chars.end()) ? i->second : generateCharacter(c, CharacterFlag_Default);
}

inline const CharDescriptor& CharacterGenerator::boldCharDescription(wchar_t c)
{
	auto i = _boldChars.find(c);
	return (i != _boldChars.end()) ? i->second : generateCharacter(c, CharacterFlag_Bold);
}

inline const CharDescriptorMap& CharacterGenerator::characters() const
{
	return _chars;
}

inline const CharDescriptorMap& CharacterGenerator::boldCharacters() const
{
	return _boldChars;
}

}
}
