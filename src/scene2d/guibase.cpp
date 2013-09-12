/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/app/application.h>
#include <et-ext/scene2d/guibase.h>
#include <et-ext/scene2d/scenerenderer.h>

using namespace et;
using namespace et::s2d;

Element::Element(Element* parent, const std::string& name) :
	ElementHierarchy(parent), tag(0), _name(name), _enabled(true), _transformValid(false),
	_inverseTransformValid(false), _contentValid(false)
{
	
}

void Element::setParent(Element* element)
{
	ElementHierarchy::setParent(element);
	invalidateContent();
	invalidateTransform();
}

bool Element::enabled() const
{
	return _enabled;
}

void Element::setEnabled(bool enabled)
{
	_enabled = enabled;
}

void Element::invalidateContent()
{ 
	_contentValid = false; 

	for (Element::List::iterator i = children().begin(), e = children().end(); i != e; ++i)
		(*i)->invalidateContent();

	setInvalid();
}

void Element::invalidateTransform()
{ 
	_transformValid = false; 
	_inverseTransformValid = false;

	for (Element::List::iterator i = children().begin(), e = children().end(); i != e; ++i)
		(*i)->invalidateTransform();

	setInvalid();
}

void Element::addToRenderQueue(RenderContext*, SceneRenderer&) 
{
}

void Element::addToOverlayRenderQueue(RenderContext*, SceneRenderer&)
{
}

SceneProgram Element::initProgram(SceneRenderer& r)
{
	return r.defaultProgram();
}

void Element::setProgramParameters(et::Program::Pointer&)
{
}

void Element::startUpdates(TimerPool* timerPool)
{
	TimedObject::startUpdates(timerPool);
}

void Element::startUpdates()
{
	TimedObject::startUpdates();
}

TimerPool::Pointer Element::timerPool()
{
	return mainTimerPool();
}

void Element::layoutChildren()
{
	for (Element::List::iterator i = children().begin(), e = children().end(); i != e; ++i)
		(*i)->layout(size());
}

void Element::bringToFront(Element* c)
{
	ElementHierarchy::bringToFront(c);
	invalidateContent();
}

void Element::sendToBack(Element* c)
{
	ElementHierarchy::sendToBack(c);
	invalidateContent();
}

void Element::broardcastMessage(const GuiMessage& msg)
{
	for (auto& c : children())
	{
		c->processMessage(msg);
		c->broardcastMessage(msg);
	}
}

Element* Element::baseChildWithName(const std::string& name, bool recursive)
	{ return childWithNameCallback(name, this, recursive); }

Element* Element::childWithNameCallback(const std::string& name, Element* root, bool recursive)
{
	if (root->name() == name)
		return root;

	if (recursive)
	{
		for (auto& c : root->children())
		{
			Element* aElement = childWithNameCallback(name, c.ptr(), recursive);
			if (aElement != nullptr)
				return aElement;
		}
	}
	else
	{
		for (auto& c : root->children())
		{
			if (c->name() == name)
				return c.ptr();
		}
	}

	return nullptr;
}



void Element::setAutolayout(const et::s2d::ElementLayout& al)
{
	_autoLayout = al;
}

void Element::setAutolayout(const vec2& pos, LayoutMode pMode, const vec2& sz,
	LayoutMode sMode, const vec2& pivot)
{
	_autoLayout.position = pos;
	_autoLayout.size = sz;
	_autoLayout.pivotPoint = pivot;
	_autoLayout.positionMode = pMode;
	_autoLayout.sizeMode = sMode;
}

void Element::setAutolayoutMask(size_t m)
{
	_autoLayout.mask = m;
}

void Element::autoLayout(const vec2& contextSize, float duration)
{
	willAutoLayout(duration);
	
	if (_autoLayout.mask & LayoutMask_Pivot)
		setPivotPoint(_autoLayout.pivotPoint);

	vec2 aSize = size();
	vec2 aPos = position();

	if (_autoLayout.mask & LayoutMask_Size)
	{
		if (_autoLayout.sizeMode == LayoutMode_RelativeToContext)
			aSize = contextSize * _autoLayout.size;
		else if ((_autoLayout.sizeMode == LayoutMode_RelativeToParent) && (parent() != nullptr))
			aSize = parent()->desiredSize() * _autoLayout.size;
		else if (_autoLayout.sizeMode == LayoutMode_WrapContent)
			aSize = contentSize();
		else
			aSize = _autoLayout.size;
	}

	if (_autoLayout.mask & LayoutMask_Position)
	{
		if (_autoLayout.positionMode == LayoutMode_RelativeToContext)
			aPos = contextSize * _autoLayout.position;
		else if ((_autoLayout.positionMode == LayoutMode_RelativeToParent) && (parent() != nullptr))
			aPos = parent()->desiredSize() * _autoLayout.position;
		else if (_autoLayout.positionMode == LayoutMode_WrapContent)
			abort();
		else
			aPos = _autoLayout.position;
	}

	if (_autoLayout.mask & LayoutMask_Frame)
		setFrame(aPos, aSize, duration);
	
	didAutoLayout(duration);

	if (!hasFlag(Flag_HandlesChildLayout))
	{
		for (auto aChild : children())
			aChild->autoLayout(contextSize, duration);
	}
}

void Element::fillParent()
{
	setAutolayout(vec2(0.0f), LayoutMode_RelativeToParent, vec2(1.0f),
		LayoutMode_RelativeToParent, vec2(0.0f));
}

void Element::setLocationInParent(Location l, const vec2& offset)
{
	vec2 actualOffset(0.5f * static_cast<float>(l % 3), 0.5f * static_cast<float>(l / 3));
	setAutolayoutRelativeToParent(actualOffset + offset, vec2(0.0f), actualOffset);
	setAutolayoutMask(LayoutMask_PositionPivot);
}

void Element::setAutolayoutRelativeToParent(const vec2& pos, const vec2& sz, const vec2& pivot)
{
	setAutolayout(pos, LayoutMode_RelativeToParent, sz, LayoutMode_RelativeToParent, pivot);
}

void Element::setAutolayoutSizeMode(LayoutMode mode)
{
	_autoLayout.sizeMode = mode;
}

et::Dictionary Element::autoLayoutDictionary() const
{
	Dictionary result;
	
	result.setStringForKey("name", name());
	result.setFloatForKey("position.x", _autoLayout.position.x);
	result.setFloatForKey("position.y", _autoLayout.position.y);
	result.setFloatForKey("size.x", _autoLayout.size.x);
	result.setFloatForKey("size.y", _autoLayout.size.y);
	result.setFloatForKey("scale.x", _autoLayout.scale.x);
	result.setFloatForKey("scale.y", _autoLayout.scale.y);
	result.setFloatForKey("pivotPoint.x", _autoLayout.pivotPoint.x);
	result.setFloatForKey("pivotPoint.y", _autoLayout.pivotPoint.y);
	result.setFloatForKey("angle", _autoLayout.angle);
	result.setIntegerForKey("autolayout_position", (_autoLayout.mask & LayoutMask_Position) ? 1 : 0);
	result.setIntegerForKey("autolayout_size", (_autoLayout.mask & LayoutMask_Size) ? 1 : 0);
	result.setIntegerForKey("autolayout_pivot", (_autoLayout.mask & LayoutMask_Pivot) ? 1 : 0);
	result.setStringForKey("positionMode", layoutModeToString(_autoLayout.positionMode));
	result.setStringForKey("sizeMode", layoutModeToString(_autoLayout.sizeMode));
	
	storeProperties(result);
	
	if (children().size() > 0 )
	{
		Dictionary childrenValues;
		
		for (auto& c : children())
			childrenValues.setDictionaryForKey(c->name(), c->autoLayoutDictionary());
		
		result.setDictionaryForKey("children", childrenValues);
	}
	
	return result;
}

void Element::setAutolayout(Dictionary d)
{
	std::string dName = d.stringForKey("name")->content;
	if (dName != name())
	{
		log::warning("Trying to set autolayout from %s to %s", dName.c_str(), name().c_str());
		return;
	}
	
	ElementLayout l = { };
	
	l.position.x = d.floatForKey("position.x", 0.0f)->content;
	l.position.y = d.floatForKey("position.y", 0.0f)->content;
	l.size.x = d.floatForKey("size.x", 0.0f)->content;
	l.size.y = d.floatForKey("size.y", 0.0f)->content;
	l.scale.x = d.floatForKey("scale.x", 1.0f)->content;
	l.scale.y = d.floatForKey("scale.y", 1.0f)->content;
	l.pivotPoint.x = d.floatForKey("pivotPoint.x", 0.0f)->content;
	l.pivotPoint.y = d.floatForKey("pivotPoint.y", 0.0f)->content;
	l.angle = d.floatForKey("angle", 0.0f)->content;
	
	l.positionMode = layoutModeFromString(d.stringForKey("positionMode", "absolute")->content);
	l.sizeMode = layoutModeFromString(d.stringForKey("sizeMode", "absolute")->content);
	
	l.mask = (d.integerForKey("autolayout_position", 1)->content ? LayoutMask_Position : 0) |
		(d.integerForKey("autolayout_size", 1)->content ? LayoutMask_Size : 0) |
		(d.integerForKey("autolayout_pivot", 1)->content ? LayoutMask_Pivot : 0);
	
	loadProperties(d);
	
	Dictionary dChildren = d.dictionaryForKey("children");
	for (auto p : dChildren->content)
	{
		Element* el = baseChildWithName(p.first, false);
		if (el == nullptr)
			log::warning("Can't find child %s in %s", p.first.c_str(), name().c_str());
		else
			el->setAutolayout(p.second);
	}
	
	setAutolayout(l);
}


/*
 * Service functions
 */
float et::s2d::alignmentFactor(Alignment a)
{
	static const float alignmentValues[Alignment_max] = { 0.0f, 0.5f, 1.0f };
	return alignmentValues[a];
}

State et::s2d::adjustState(State s)
{
	bool isHovered = (s == State_Hovered) || (s == State_SelectedHovered);
	return (!Input::canGetCurrentPointerInfo() && isHovered) ? static_cast<State>(s - 1) : s;
}

static const std::string kLayoutMode_Absolute = "absolute";
static const std::string kLayoutMode_RelativeToContext = "context_relative";
static const std::string kLayoutMode_RelativeToParent = "parent_relative";
static const std::string kLayoutMode_WrapContent = "wrap_content";

#define RETURN_STR_IF(V) case V: return k##V;
#define RETURN_VAL_IF(V) if (s == k##V) return V;

std::string et::s2d::layoutModeToString(LayoutMode m)
{
	switch (m)
	{
		RETURN_STR_IF(LayoutMode_Absolute)
		RETURN_STR_IF(LayoutMode_RelativeToContext)
		RETURN_STR_IF(LayoutMode_RelativeToParent)
		RETURN_STR_IF(LayoutMode_WrapContent)
			
		default:
			log::error("Invalid layout mode");
	}
	
	return kLayoutMode_Absolute;
}

LayoutMode et::s2d::layoutModeFromString(const std::string& s)
{
	RETURN_VAL_IF(LayoutMode_Absolute);
	RETURN_VAL_IF(LayoutMode_RelativeToContext);
	RETURN_VAL_IF(LayoutMode_RelativeToParent);
	RETURN_VAL_IF(LayoutMode_WrapContent);
	
	log::error("Invalid layout mode string %s", s.c_str());
	return LayoutMode_Absolute;
}
