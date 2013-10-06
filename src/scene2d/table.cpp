/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/table.h>

using namespace et;
using namespace et::s2d;

ET_DECLARE_SCENE_ELEMENT_CLASS(Table)

Table::Section::Section() :
	headerOffset(0.0f), footerOffset(0.0f), headerSize(0.0f), itemsSize(0.0f),
	footerSize(0.0f), sectionSize(0.0f) { }

Table::Table(et::s2d::Element2d* parent, const std::string& name) :
	Scroll(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS)
{
	setBounce(Bounce_Horizontal);
	setFlag(Flag_HandlesChildLayout);
}

Table::~Table()
{
	clean();
}

void Table::didAutoLayout(float)
{
	setOffsetDirectly(lastElementIndex());
	layoutChildren(size());
}

void Table::layoutChildren(const vec2& ownSize)
{
	float x = 0.0f;
	
	for (auto s : _sections)
	{
		if (s->header.valid())
		{
			s->header->setPosition(x + s->headerOffset, 0.0f);
			s->header->setSize(s->header->size().x, ownSize.y);
			x += s->header->size().x;
		}
		
		for (auto i : s->items)
		{
			i->setPosition(x, 0.0f);
			i->setSize(i->size().x, ownSize.y);
			x += i->size().x;
		}
		
		if (s->footer.valid())
		{
			s->footer->setPosition(x + s->footerOffset, 0.0f);
			s->footer->setSize(s->footer->size().x, ownSize.y);
			x += s->footer->size().x;
		}
	}

	adjustContentSize();
}

Table::Section* Table::addSection(Element2d::Pointer header, const Element2d::List& items,
	Element2d::Pointer footer)
{
	Section* section = new Section;
	
	if (header.valid())
	{
		section->header = header;
		section->header->setParent(this);
		section->headerSize = section->header->size().x;
		section->sectionSize += section->headerSize;
	}
	
	section->items = items;
	for (auto i : section->items)
	{
		i->setParent(this);
		section->itemsSize += i->size().x;
	}
	section->sectionSize += section->itemsSize;
	
	if (footer.valid())
	{
		section->footer = footer;
		section->footer->setParent(this);
		section->footerSize = section->footer->size().x;
		section->sectionSize += section->footerSize;
	}
	
	if (section->header.valid())
		bringToFront(section->header.ptr());

	if (section->footer.valid())
		bringToFront(section->footer.ptr());
	
	_sections.push_back(section);
	
	adjustContentSize();
	setOffsetDirectly(lastElementIndex());
	
	return section;
}

void Table::setOffsetDirectly(const vec2& o)
{
	Scroll::setOffsetDirectly(o);
	
	float width = size().x;
	float sectionStart = 0.0f;
	float off = -lastElementIndex().x;
	for (auto s : _sections)
	{
		s->headerOffset = clamp(off - sectionStart, 0.0f, s->itemsSize);
		s->footerOffset = clamp(width - s->sectionSize - sectionStart + off, -s->itemsSize, 0.0f);
		sectionStart += s->sectionSize;
	}
	
	layoutChildren(size());
}

void Table::clean()
{
	for (auto s : _sections)
	{
		s->header.reset(nullptr);
		
		for (auto& item : s->items)
		{
			item->setParent(nullptr);
			item.reset(nullptr);
		}
		
		s->items.clear();
		s->footer.reset(nullptr);
		delete s;
	}
	
	_sections.clear();
}
