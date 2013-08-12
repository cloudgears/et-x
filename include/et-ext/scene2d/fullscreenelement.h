/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>

namespace et
{
	namespace s2d
	{
		class FullscreenElement : public Element2d
		{
		public:
			ET_DECLARE_POINTER(FullscreenElement)

		public:
			FullscreenElement(Element* parent, const std::string& name = std::string());
			void layout(const vec2& sz);
		};
	}
}
