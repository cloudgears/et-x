/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/core/dictionary.h>

namespace et
{
	namespace json
	{
		std::string serialize(const et::Dictionary&, bool readableFormat = false);
		
		et::ValueBase::Pointer deserialize(const char*, et::ValueClass&);
		et::ValueBase::Pointer deserialize(const std::string&, et::ValueClass&);
	}
}