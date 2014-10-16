/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/core/conversion.h>
#include <et/app/application.h>
#include <et-ext/json/json.h>
#include <et-ext/scene2d/scenerenderer.h>
#include <et-ext/scene2d/element.h>

using namespace et;
using namespace et::s2d;

Element::Element(Element* parent, const std::string& name) :
	ElementHierarchy(parent), tag(0), _name(name) { }

void Element::setParent(Element* element)
{
	ElementHierarchy::setParent(element);
	invalidateContent();
	invalidateTransform();
}

void Element::removeAllChildren()
{
	removeChildren();
}

void Element::childRemoved(Element*)
{
	invalidateContent();
	invalidateTransform();
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
	setTransformValid(false);
	setInverseTransformValid(false);

	for (Element::List::iterator i = children().begin(), e = children().end(); i != e; ++i)
		(*i)->invalidateTransform();

	setInvalid();
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

void Element::broardcastMessage(const Message& msg)
{
	for (auto& c : children())
	{
		c->processMessage(msg);
		c->broardcastMessage(msg);
	}
}

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

void Element::setAutolayout(const vec2& pos, LayoutMode pMode, const vec2& sz,
	LayoutMode sMode, const vec2& pivot)
{
	_autoLayout.position = pos;
	_autoLayout.size = sz;
	_autoLayout.pivotPoint = pivot;
	_autoLayout.layoutPositionMode = pMode;
	_autoLayout.layoutSizeMode = sMode;
}

void Element::setAutolayoutPosition(const vec2& pos)
{
	_autoLayout.position = pos;
}

void Element::setAutolayoutSize(const vec2& sz)
{
	_autoLayout.size = sz;
}

void Element::autoLayout(const vec2& contextSize, float duration)
{
	willAutoLayout(duration);
	
	if (_autoLayout.layoutMask & LayoutMask_Size)
	{
		vec2 aSize = _autoLayout.size;
		
		switch (_autoLayout.layoutSizeMode)
		{
			case LayoutMode_RelativeToContext:
				aSize = contextSize * _autoLayout.size;
				break;
				
			case LayoutMode_RelativeToParent:
				aSize = _autoLayout.size * ((parent() == nullptr) ? contextSize : parent()->desiredSize());
				break;
		
			case LayoutMode_WrapContent:
				aSize = contentSize();
				break;
				
			default:
				break;
		}
		
		setSize(aSize, duration);
	}

	if (_autoLayout.layoutMask & LayoutMask_Position)
	{
		vec2 aPos = _autoLayout.position;
		
		switch (_autoLayout.layoutPositionMode)
		{
			case LayoutMode_RelativeToContext:
				aPos = contextSize * _autoLayout.position;
				break;
				
			case LayoutMode_RelativeToParent:
				aPos = _autoLayout.position * ((parent() == nullptr) ? contextSize : parent()->desiredSize());
				break;
				
			case LayoutMode_WrapContent:
				abort();
				break;
				
			default:
				break;
		}
		
		setPosition(aPos, duration);
	}
	
	if (_autoLayout.layoutMask & LayoutMask_Pivot)
		setPivotPoint(_autoLayout.pivotPoint, false);
	
	if (_autoLayout.layoutMask & LayoutMask_Angle)
		setAngle(_autoLayout.angle, duration);
	
	if (_autoLayout.layoutMask & LayoutMask_Scale)
		setScale(_autoLayout.scale, duration);
	
	if (!hasFlag(Flag_HandlesChildLayout))
	{
		for (auto aChild : children())
			aChild->autoLayout(contextSize, duration);
	}
	
	didAutoLayout(duration);
}

void Element::setLocationInParent(Location l, const vec2& offset)
{
	vec2 actualOffset(0.5f * static_cast<float>(l % 3), 0.5f * static_cast<float>(l / 3));
	setAutolayoutRelativeToParent(actualOffset + offset, vec2(0.0f), actualOffset);
	setAutolayoutMask(LayoutMask_NoSize);
}

void Element::setAutolayoutRelativeToParent(const vec2& pos, const vec2& sz, const vec2& pivot)
	{ setAutolayout(pos, LayoutMode_RelativeToParent, sz, LayoutMode_RelativeToParent, pivot); }

void Element::setAutolayoutSizeMode(LayoutMode mode)
	{ _autoLayout.layoutSizeMode = mode; }

void Element::setAutolayoutPositionMode(LayoutMode mode)
	{ _autoLayout.layoutPositionMode = mode; }

et::Dictionary Element::autoLayoutDictionary() const
{
	Dictionary result;
	
	result.setStringForKey("name", name());
	
	result.setArrayForKey("position", vec2ToArray(_autoLayout.position));
	result.setArrayForKey("size", vec2ToArray(_autoLayout.size));
	result.setArrayForKey("scale", vec2ToArray(_autoLayout.scale));
	result.setArrayForKey("pivotPoint", vec2ToArray(_autoLayout.pivotPoint));
	result.setFloatForKey("angle", _autoLayout.angle);
	
	result.setIntegerForKey("autolayout_position", (_autoLayout.layoutMask & LayoutMask_Position) ? 1 : 0);
	result.setIntegerForKey("autolayout_size", (_autoLayout.layoutMask & LayoutMask_Size) ? 1 : 0);
	result.setIntegerForKey("autolayout_pivot", (_autoLayout.layoutMask & LayoutMask_Pivot) ? 1 : 0);
	result.setIntegerForKey("autolayout_angle", (_autoLayout.layoutMask & LayoutMask_Angle) ? 1 : 0);
	result.setIntegerForKey("autolayout_scale", (_autoLayout.layoutMask & LayoutMask_Scale) ? 1 : 0);
	result.setIntegerForKey("autolayout_children", (_autoLayout.layoutMask & LayoutMask_Children) ? 1 : 0);
	
	result.setStringForKey("positionMode", layoutModeToString(_autoLayout.layoutPositionMode));
	result.setStringForKey("sizeMode", layoutModeToString(_autoLayout.layoutSizeMode));
	
	storeProperties(result);
	
	if ((_autoLayout.layoutMask & LayoutMask_Children) && (children().size() > 0))
	{
		Dictionary childrenValues;
		
		for (auto& c : children())
			childrenValues.setDictionaryForKey(c->name(), c->autoLayoutDictionary());
		
		result.setDictionaryForKey("children", childrenValues);
	}
	
	return result;
}

void Element::setAutolayout(const Dictionary& d)
{
	ElementLayout l = _autoLayout;
	
	if (d.hasKey("position"))
		l.position = arrayToVec2(d.arrayForKey("position"));
	
	if (d.hasKey("size"))
		l.size = arrayToVec2(d.arrayForKey("size"));
	
	if (d.hasKey("scale"))
		l.scale = arrayToVec2(d.arrayForKey("scale"));
	
	if (d.hasKey("pivotPoint"))
		l.pivotPoint = arrayToVec2(d.arrayForKey("pivotPoint"));
	
	if (d.hasKey("angle"))
		l.angle = d.floatForKey("angle", 0.0f)->content;
	
	if (d.hasKey("positionMode"))
		l.layoutPositionMode = layoutModeFromString(d.stringForKey("positionMode", "parent_relative")->content);
	
	if (d.hasKey("sizeMode"))
		l.layoutSizeMode = layoutModeFromString(d.stringForKey("sizeMode", "parent_relative")->content);
	
	l.layoutMask =
		(d.integerForKey("autolayout_position", l.layoutMask & LayoutMask_Position)->content ? LayoutMask_Position : 0) |
		(d.integerForKey("autolayout_size", l.layoutMask & LayoutMask_Size)->content ? LayoutMask_Size : 0) |
		(d.integerForKey("autolayout_pivot", l.layoutMask & LayoutMask_Pivot)->content ? LayoutMask_Pivot : 0) |
		(d.integerForKey("autolayout_angle", l.layoutMask & LayoutMask_Angle)->content ? LayoutMask_Angle : 0) |
		(d.integerForKey("autolayout_scale", l.layoutMask & LayoutMask_Scale)->content ? LayoutMask_Scale : 0) |
		(d.integerForKey("autolayout_children", l.layoutMask & LayoutMask_Children)->content ? LayoutMask_Children : 0);
	
	loadProperties(d);
	
	if (l.layoutMask & LayoutMask_Children)
	{
		Dictionary dChildren = d.dictionaryForKey("children");
		for (auto p : dChildren->content)
		{
			Element* el = baseChildWithName(p.first, false);
			if (el != nullptr)
				el->setAutolayout(p.second);
		}
	}
	
	setAutolayout(l);
}

void Element::autolayoutFromFile(const std::string& fileName)
{
	setOrigin(fileName);
	
	ValueClass c = ValueClass_Invalid;
	ValueBase::Pointer base = json::deserialize(loadTextFile(fileName), c);
	
	if (base.valid() && (c == ValueClass_Dictionary))
		setAutolayout(base);
	else
		log::error("Unable to load layout from file %s", fileName.c_str());
}

bool Element::enabled() const
	{ return _enabled; }

void Element::setEnabled(bool enabled)
	{ _enabled = enabled; }

SceneProgram Element::initProgram(SceneRenderer& r)
	{ return r.defaultProgram(); }

void Element::startUpdates(TimerPool* timerPool)
	{ TimedObject::startUpdates(timerPool); }

Element* Element::baseChildWithName(const std::string& name, bool recursive)
	{ return childWithNameCallback(name, this, recursive); }

void Element::setAutolayout(const et::s2d::ElementLayout& al)
	{ _autoLayout = al; }

void Element::setAutolayoutMask(size_t m)
	{ _autoLayout.layoutMask = m; }

void Element::fillParent()
	{ setAutolayoutRelativeToParent(vec2(0.0f), vec2(1.0f), vec2(0.0f)); }

TimerPool* Element::timerPool()
	{ return mainTimerPool().ptr(); }

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
