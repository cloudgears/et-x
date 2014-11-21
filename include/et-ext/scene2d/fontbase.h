/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/containers.h>

#define FONT_VERSION_1			0x0001
#define FONT_VERSION_2			0x0002
#define FONT_VERSION_CURRENT	FONT_VERSION_2

namespace et
{
	namespace s2d
	{
		enum CharacterFlags
		{
			CharacterFlag_Default = 0x0000,
			CharacterFlag_Bold = 0x0001
		};

		struct CharDescriptor
		{
			int value = 0;
			int flags = CharacterFlag_Default;
			
			vec4 color = vec4(1.0f);
			vec2 originalSize;
			rect contentRect;
			rect uvRect;
			vec4 parameters;
			
			CharDescriptor()
				{ }
			
			CharDescriptor(int v) :
				value(v) { }
		};

		typedef std::vector<CharDescriptor> CharDescriptorList;
		typedef std::map<int, CharDescriptor> CharDescriptorMap;
	}
}
