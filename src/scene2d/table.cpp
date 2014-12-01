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
	setOffsetDirectly(contentOffset());
}

void Table::layoutChildren()
{
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

void Table::validateSections()
{
	for (auto section : _sections)
	{
		section->headerSize = section->header.valid() ? section->header->size().x : 0.0f;
		section->footerSize = section->footer.valid() ? section->footer->size().x : 0.0f;
		
		section->itemsSize = 0.0f;
		for (auto i : section->items)
			section->itemsSize += i->size().x;
		
		section->sectionSize = section->headerSize + section->itemsSize + section->footerSize;
	}
}

Table::Section* Table::addSection(Element2d::Pointer header, const Element2d::List& items,
	Element2d::Pointer footer)
{
	Section* section = sharedObjectFactory().createObject<Section>();
	
	section->items = items;
	for (auto i : section->items)
		i->setParent(this);
	
	if (header.valid())
	{
		section->header = header;
		section->header->setParent(this);
		bringToFront(section->header.ptr());
	}
	
	if (footer.valid())
	{
		section->footer = footer;
		section->footer->setParent(this);
		bringToFront(section->footer.ptr());
	}
	
	_sections.push_back(section);

	adjustContentSize();
	setOffsetDirectly(contentOffset());
	
	return section;
}

void Table::setOffsetDirectly(const vec2& o)
{
	Scroll::setOffsetDirectly(o);
	
	validateSections();
	
	float width = size().x;
	float sectionStart = 0.0f;
	float actualOffset = -contentOffset().x;
	for (auto s : _sections)
	{
		s->headerOffset = clamp(actualOffset - sectionStart, 0.0f, s->itemsSize);
		s->footerOffset = clamp(width - s->sectionSize + actualOffset - sectionStart, -s->itemsSize, 0.0f);
		sectionStart += s->sectionSize;
	}
	
	layoutChildren(size());
}

void Table::clean()
{
	for (auto s : _sections)
	{
		if (s->header.valid())
			s->header->setParent(nullptr);
		
		for (auto& item : s->items)
		{
			item->setParent(nullptr);
			item.reset(nullptr);
		}
		
		if (s->footer.valid())
			s->footer->setParent(nullptr);
		
		s->header.reset(nullptr);
		s->items.clear();
		s->footer.reset(nullptr);
		
		sharedObjectFactory().deleteObject(s);
	}
	
	_sections.clear();
}
