/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <sstream>
#include <et/core/conversion.h>
#include <et/app/application.h>
#include <et/rendering/rendercontext.h>
#include <et/imaging/textureloader.h>
#include <et/json/json.h>
#include <et-ext/scene2d/textureatlas.h>

using namespace et;
using namespace et::s2d;

static const Image _emptyImage;

rect parseRectString(std::string& s);

TextureAtlas::TextureAtlas() : _loaded(false)
{
}

TextureAtlas::TextureAtlas(RenderContext* rc, const std::string& filename, ObjectsCache& cache) :
	_loaded(false)
{
	loadFromFile(rc, filename, cache);
}

void TextureAtlas::loadFromFile(RenderContext* rc, const std::string& filename, ObjectsCache& cache, bool async)
{
	std::string resolvedFileName = application().resolveFileName(filename);
	if (!fileExists(resolvedFileName))
	{
		log::error("Unable to load texture atlas from %s", filename.c_str());
		return;
	}
	
	application().pushSearchPath(getFilePath(resolvedFileName));

#if (ET_DISABLE_JSON)
#
#	warning ET_DISABLE_JSON is defined, JSON texture atlases is not available
#
#else
	ValueClass vc = ValueClass_Invalid;
	Dictionary atlas = json::deserialize(loadTextFile(resolvedFileName), vc, false);
	
	if (vc == ValueClass_Dictionary)
	{
		ArrayValue textures = atlas.arrayForKey("textures");
		for (const Dictionary& tex : textures->content)
		{
			auto textureId = tex.stringForKey("id")->content;
			auto textureFile = tex.stringForKey("filename")->content;
			_textures[textureId] = rc->textureFactory().loadTexture(textureFile, cache, async);
			if (_textures[textureId].valid())
				_textures[textureId]->setWrap(rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
		}
		
		ArrayValue images = atlas.arrayForKey("images");
		for (const Dictionary& img : images->content)
		{
			auto name = img.stringForKey("name")->content;
			auto tex = img.stringForKey("texture")->content;
			auto r = arrayToRect(img.arrayForKey("rect"));
			vec4 offset = arrayToVec4(img.arrayForKey("offset"));
			
			_images[name] = Image(_textures[tex],
				ImageDescriptor(r.origin(), r.size(), ContentOffset(offset.x, offset.y, offset.z, offset.w)));
		}
	}
	else
#endif
	{
		InputStream descFile(resolvedFileName, StreamMode_Text);
		if (descFile.valid())
		{
			int lineNumber = 1;
			while (!descFile.stream().eof())
			{
				std::string token;
				std::string line;
				
				descFile.stream() >> token;
				std::getline(descFile.stream(), line);
				
				if (token == "texture:")
				{
					std::string textureId = trim(line);
					std::string textureName = application().resolveFileName(textureId);
					
					if (!fileExists(textureName))
						textureName = application().resolveFileName(textureId);
					
					_textures[textureId] = rc->textureFactory().loadTexture(textureName, cache, async);
					
					if (_textures[textureId].valid())
						_textures[textureId]->setWrap(rc, TextureWrap_ClampToEdge, TextureWrap_ClampToEdge);
				}
				else if (token == "image:")
				{
					trim(line);
					if ((*line.begin() == '{') && (*line.rbegin() == '}'))
					{
						line = line.substr(1, line.length() - 2);
						trim(line);
						std::istringstream parser(line);
						
						std::string imageName;
						std::string textureName;
						rect sourceRect;
						rect contentOffset;
						
						while (!parser.eof())
						{
							std::string aToken;
							parser >> aToken;
							if (aToken == "name:")
							{
								parser >> imageName;
								if (*imageName.begin() == '"')
									imageName.erase(0, 1);
								if (*imageName.rbegin() == '"')
									imageName.erase(imageName.length() - 1);
							}
							else if (aToken == "texture:")
							{
								parser >> textureName;
								if (*textureName.begin() == '"')
									textureName.erase(0, 1);
								if (*textureName.rbegin() == '"')
									textureName.erase(textureName.length() - 1);
							}
							else if (aToken == "rect:")
							{
								std::string sRect;
								parser >> sRect;
								sourceRect = parseRectString(sRect);
							}
							else if (aToken == "offset:")
							{
								std::string offset;
								parser >> offset;
								contentOffset = parseRectString(offset);
							}
							else
							{
								log::warning("Unknown token at line %d : %s", lineNumber, aToken.c_str());
							}
						}
						
						ImageDescriptor desc(sourceRect.origin(), sourceRect.size());
						
						desc.contentOffset = ContentOffset(contentOffset[0], contentOffset[1],
														   contentOffset[2], contentOffset[3]);
						
						_images[imageName] = Image(_textures[textureName], desc);
					}
					else
					{
						log::warning("Unable to parse image token at line %d : %s", lineNumber, line.c_str());
					}
				}
				else
				{
					if (token.length() && line.length())
						log::warning("Unknown token at line %d : %s", lineNumber, token.c_str());
				}
				
				++lineNumber;
			}
		}
	}
	application().popSearchPaths();
	
	_loaded = true;
}

bool TextureAtlas::hasImage(const std::string& key) const
{
	return _images.count(key) > 0;
}

const s2d::Image& TextureAtlas::image(const std::string& key) const
{
	ET_ASSERT(key.length() > 0);
	
	if (_images.count(key) == 0)
		return _emptyImage;
	
	return _images.at(key);
/*
	auto i = _images.find(key);
	if (i == _images.end())
		return _emptyImage;
	return i->second;
*/
}

std::vector<Image> TextureAtlas::imagesForTexture(Texture t) const
{
	std::vector<Image> result;
	
	for (auto& i : _images)
	{
		if (i.second.texture == t)
			result.push_back(i.second);
	}
			
	return result;
}

void TextureAtlas::unload()
{
	_images.clear();
	_textures.clear();
	_loaded = false;
}

Texture TextureAtlas::firstTexture() const
{
	return _textures.size() ? _textures.begin()->second : Texture();
}

/*
 * Helper funcitons
 */
rect parseRectString(std::string& s)
{
	rect result;
	int values[4] = { };
	int vIndex = 0;

	if (*s.begin() == '"')
		s.erase(0, 1);

	if (*s.rbegin() == '"')
		s.erase(s.length() - 1);

	while (s.length())
	{
		std::string::size_type dpos = s.find_first_of(";");
		if (dpos == std::string::npos)
		{
			values[vIndex++] = strToInt(s);
			break;
		}
		else
		{
			std::string value = s.substr(0, dpos);
			values[vIndex++] = strToInt(value);
			s.erase(0, dpos+1);
		}
	}

	result.left = static_cast<float>(values[0]);
	result.top = static_cast<float>(values[1]);
	result.width = static_cast<float>(values[2]);
	result.height = static_cast<float>(values[3]);

	return result;
}
