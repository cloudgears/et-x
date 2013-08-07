/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/gui/scroll.h>

namespace et
{
	namespace gui
	{
		class Table : public et::gui::Scroll
		{
		public:
			ET_DECLARE_POINTER(Table)
			
			Table(et::gui::Element2d*, const std::string& name = std::string());
		};
	}
}