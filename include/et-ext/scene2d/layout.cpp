/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/rendering/rendercontext.h>
#include <et/core/json.h>
#include <et-ext/scene2d/layout.h>

namespace et
{
namespace s2d
{

Layout::Layout(const std::string& name) :
	Element2d(nullptr, ET_S2D_PASS_NAME_TO_BASE_CLASS), _positionInterpolationFunction(linearInerpolation)
{
	setAutolayout(vec2(0.0f), LayoutMode_Absolute, vec2(1.0f),
		LayoutMode_RelativeToContext, vec2(0.0f));
}

void Layout::addElementToRenderQueue(Element2d::Pointer& element, RenderContext* rc, SceneRenderer& gr)
{
	if (!element->visible()) return;

	bool clipToBounds = element->hasFlag(Flag_ClipToBounds);

	if (clipToBounds)
	{
		const mat4& parentTransform = element->parent()->finalTransform();

		vec2 eSize = multiplyWithoutTranslation(element->size(), parentTransform);
		vec2 eOrigin = parentTransform * element->origin();

		gr.pushClipRect(recti(vec2i(static_cast<int>(eOrigin.x),
			static_cast<int>(rc->size().y - eOrigin.y - eSize.y)),
			vec2i(static_cast<int>(eSize.x), static_cast<int>(eSize.y))));
	}

	element->addToRenderQueue(rc, gr);
	for (auto& c : element->children())
	{
		if (!elementIsBeingDragged(c))
			addElementToRenderQueue(c, rc, gr);
	}
	element->addToOverlayRenderQueue(rc, gr);

	if (clipToBounds)
		gr.popClipRect();
}

void Layout::addToRenderQueue(RenderContext* rc, SceneRenderer& gr)
{
	gr.resetClipRect();

	for (auto& c : children())
	{
		if (!elementIsBeingDragged(c))
			addElementToRenderQueue(c, rc, gr);
	}

	for (auto& t : _topmostElements)
	{
		if (!elementIsBeingDragged(t))
			addElementToRenderQueue(t, rc, gr);
	}

	if (elementIsBeingDragged(_capturedElement))
		addElementToRenderQueue(_capturedElement, rc, gr);

	_valid = true;
}

bool Layout::pointerPressed(const et::PointerInputInfo& p)
{
	if (hasFlag(Flag_TransparentForPointer)) return false;

	if (_capturedElement.valid())
	{
		_capturedElement->pointerPressed(PointerInputInfo(p.type, _capturedElement->positionInElement(p.pos),
			p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		return true;
	}
	else
	{
		Element2d::Pointer active(activeElement(p));
		setHoveredElement(p, active);
		bool processed = false;
		if (active.invalid())
		{
			setFocusedElement(Element2d::Pointer());
		}
		else
		{
			vec2 elementPos = active->positionInElement(p.pos);

			processed = active->pointerPressed(PointerInputInfo(p.type, elementPos,
				p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

			if ((p.type == PointerTypeMask::General))
			{
				if (active->hasFlag(Flag_Dragable))
				{
					processed = true;

					_dragging = true;
					_capturedElement = active;
					_dragInitialPosition = active->position();
					_dragInitialOffset = elementPos;

					_capturedElement->dragStarted.invoke(_capturedElement.pointer(),
						ElementDragInfo(_dragInitialPosition, _dragInitialPosition, p.normalizedPos));

					if (Input::canGetCurrentPointerInfo())
						startUpdates();

					invalidateContent();
				}
				else if (!active->hasFlag(s2d::Flag_HandlesChildEvents))
				{
					setFocusedElement(active);
				}
			}

			if (processed || _dragging)
			{
				_capturedElement = active;
				invalidateContent();
			}
		}

		return processed;
	}
}

bool Layout::pointerMoved(const et::PointerInputInfo& p)
{
	if (hasFlag(Flag_TransparentForPointer)) return false;

	if (_capturedElement.valid())
	{
		if (!Input::canGetCurrentPointerInfo() && (p.type == PointerTypeMask::General) && _dragging)
			performDragging(p);

		_capturedElement->pointerMoved(PointerInputInfo(p.type, _capturedElement->positionInElement(p.pos),
			p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

		return true;
	}

	Element2d::Pointer active(activeElement(p));
	setHoveredElement(p, active);

	if (active.invalid())
		return false;

	return active->pointerMoved(PointerInputInfo(p.type, active->positionInElement(p.pos),
		p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
}

bool Layout::pointerReleased(const et::PointerInputInfo& p)
{
	if (hasFlag(Flag_TransparentForPointer)) return false;

	Element2d::Pointer active(activeElement(p));

	if (_capturedElement.valid())
	{
		bool processed = _capturedElement->pointerReleased(PointerInputInfo(p.type,
			_capturedElement->positionInElement(p.pos), p.normalizedPos, p.scroll, p.id,
			p.timestamp, p.origin));

		if ((p.type == PointerTypeMask::General) && _dragging)
		{
			if (Input::canGetCurrentPointerInfo())
				cancelUpdates();

			_capturedElement->dragFinished.invoke(_capturedElement.pointer(),
				ElementDragInfo(_capturedElement->parent()->positionInElement(p.pos),
					_dragInitialPosition, p.normalizedPos));

			_dragging = false;
			_capturedElement.reset(nullptr);
			invalidateContent();
		}
		else if (processed)
		{
			_capturedElement.reset(nullptr);
			invalidateContent();
		}

		return true;
	}
	else
	{
		setHoveredElement(p, active);
		bool processed = false;

		if (active.valid())
		{
			processed = active->pointerReleased(PointerInputInfo(p.type, active->positionInElement(p.pos),
				p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		}

		return processed;
	}
}

bool Layout::pointerCancelled(const et::PointerInputInfo& p)
{
	Element2d::Pointer active(activeElement(p));
	if (_capturedElement.valid())
	{
		bool processed = _capturedElement->pointerCancelled(PointerInputInfo(p.type,
			_capturedElement->positionInElement(p.pos), p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

		if ((p.type == PointerTypeMask::General) && _dragging)
		{
			if (Input::canGetCurrentPointerInfo())
				cancelUpdates();

			_capturedElement->dragFinished.invoke(_capturedElement.pointer(),
				ElementDragInfo(_capturedElement->parent()->positionInElement(p.pos), _dragInitialPosition, p.normalizedPos));

			_dragging = false;
			_capturedElement.reset(nullptr);
			invalidateContent();
		}
		else if (processed)
		{
			_capturedElement.reset(nullptr);
			invalidateContent();
		}

		return true;
	}
	else
	{
		setHoveredElement(p, active);
		bool processed = false;

		if (active.valid())
		{
			processed = active->pointerCancelled(PointerInputInfo(p.type, active->positionInElement(p.pos),
				p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));
		}

		return processed;
	}
}

bool Layout::pointerScrolled(const et::PointerInputInfo& p)
{
	if (hasFlag(Flag_TransparentForPointer)) return false;

	if (_capturedElement.valid())
	{
		_capturedElement->pointerScrolled(PointerInputInfo(p.type, _capturedElement->positionInElement(p.pos),
			p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

		return true;
	}
	else
	{
		Element2d::Pointer active(activeElement(p));

		setHoveredElement(p, active);

		bool processed = active.valid() && active->pointerScrolled(PointerInputInfo(p.type,
			active->positionInElement(p.pos), p.normalizedPos, p.scroll, p.id, p.timestamp, p.origin));

		return processed;
	}
}

Element2d::Pointer Layout::activeElement(const PointerInputInfo& p)
{
	_topmostElements.clear();
	for (auto& i : children())
		collectTopmostElements(i);

	Element2d::Pointer active;
	for (auto i = _topmostElements.rbegin(), e = _topmostElements.rend(); i != e; ++i)
	{
		active = getActiveElement(p, *i);
		if (active.valid())
		{
			active->name();
			break;
		}
	}

	if (active.invalid())
	{
		for (auto i = children().rbegin(), e = children().rend(); i != e; ++i)
		{
			active = getActiveElement(p, *i);
			if (active.valid())	break;
		}
	}

	return active;
}

Element2d::Pointer Layout::getActiveElement(const PointerInputInfo& p, Element2d::Pointer el)
{
	if (!el->visible() || !el->enabled() || !el->containsPoint(p.pos, p.normalizedPos))
		return Element2d::Pointer();

	if (el->hasFlag(Flag_HandlesChildEvents))
		return el;

	for (auto ei = el->children().rbegin(), ee = el->children().rend(); ei != ee; ++ei)
	{
		Element2d::Pointer element = getActiveElement(p, *ei);
		if (element.valid())
			return element;
	}

	return el->hasFlag(Flag_TransparentForPointer) ? Element2d::Pointer() : el;
}

void Layout::setHoveredElement(const PointerInputInfo& p, Element2d::Pointer e)
{
	if (e == _currentElement) return;

	if (_currentElement.valid())
	{
		_currentElement->pointerLeaved(p);
		_currentElement->hoverEnded.invoke(_currentElement.pointer());
	}

	_currentElement = e;

	if (_currentElement.valid())
	{
		_currentElement->pointerEntered(p);
		_currentElement->hoverStarted.invoke(_currentElement.pointer());
	}
}

void Layout::performDragging(const PointerInputInfo& p)
{
	ET_ASSERT(_dragging);

	vec2 currentPos = _capturedElement->positionInElement(p.pos);
	vec2 delta = currentPos - _dragInitialOffset;

	_capturedElement->setPosition(_capturedElement->position() + delta, 0.0f);

	_capturedElement->dragged.invoke(_capturedElement.pointer(),
		ElementDragInfo(_capturedElement->position(), _dragInitialPosition, p.normalizedPos));
}

bool Layout::elementIsBeingDragged(s2d::Element2d::Pointer e)
{
	return _dragging && e.valid() && (e == _capturedElement);
}

void Layout::update(float)
{
	if (_dragging && Input::canGetCurrentPointerInfo())
		performDragging(Input::currentPointer());
}

void Layout::cancelDragging(float returnDuration)
{
	if (_dragging)
	{
		_capturedElement->setPosition(_dragInitialPosition, returnDuration);
		_capturedElement.reset(nullptr);
		_dragging = false;

		invalidateContent();
	}
}

void Layout::setFocusedElement(Element2d::Pointer e)
{
	if (_focusedElement == e) return;

	if (_focusedElement.valid())
		_focusedElement->resignFocus(e.pointer());

	_focusedElement = e;

	bool needKeyboard = hasFlag(Flag_RequiresKeyboard) ||
		(_focusedElement.valid() && _focusedElement->hasFlag(Flag_RequiresKeyboard));

	if (_focusedElement.valid())
		_focusedElement->setFocus();

	if (needKeyboard)
		layoutRequiresKeyboard.invoke(this, _focusedElement.pointer());
	else
		layoutDoesntNeedKeyboard.invoke(this);

	focusedElementChanged(_focusedElement.pointer());
}

void Layout::setInvalid()
{
	_valid = false;
}

void Layout::collectPreRenderingObjects(Element2d::Pointer element, Element2d::Collection& elementList)
{
	if (element->visible())
	{
		if (element->hasFlag(Flag_RequiresPreRendering))
			elementList.push_back(Element2d::Pointer(element));

		for (auto& i : element->children())
			collectPreRenderingObjects(i, elementList);
	}
}

void Layout::collectTopmostElements(Element2d::Pointer element)
{
	if (element->visible())
	{
		if (element->hasFlag(Flag_RenderTopmost))
			_topmostElements.push_back(Element2d::Pointer(element));

		for (auto& i : element->children())
			collectTopmostElements(i);
	}
}

void Layout::initRenderingElement(et::RenderContext* rc)
{
	if (_renderingElement.invalid())
		_renderingElement = RenderingElement::Pointer::create(rc, RenderingElement::MaxCapacity);
}

vec2 Layout::contentSize()
{
	return vec2(0.0f);
}

void Layout::cancelInteractions()
{
	cancelInteractionsInElement(Element2d::Pointer(this), PointerInputInfo());
}

void Layout::cancelInteractionsInElement(Element2d::Pointer e, const PointerInputInfo& info)
{
	e->pointerCancelled(info);

	for (auto c : e->children())
		cancelInteractionsInElement(c, info);
}

Element2d::Pointer Layout::findFirstResponder(const Message& msg)
{
	return findFirstResponderStartingFrom(Element2d::Pointer(this), msg);
}

Element2d::Pointer Layout::findFirstResponderStartingFrom(Element2d::Pointer base, const Message& msg)
{
	if (base->enabled() && base->visible())
	{
		for (auto i = base->children().rbegin(), e = base->children().rend(); i != e; ++i)
		{
			auto el = findFirstResponderStartingFrom(*i, msg);
			if (el.valid())
				return el;
		}

		if (base->respondsToMessage(msg))
			return base;
	}

	return Element2d::Pointer();
}


/*
 *
 * Modal Layout
 *
 */

ModalLayout::ModalLayout(const std::string& name) :
	Layout(ET_S2D_PASS_NAME_TO_BASE_CLASS)
{
	_backgroundFade = ImageView::Pointer::create(Texture::Pointer(), this, "img_background_fade");
	_backgroundFade->setBackgroundColor(vec4(0.0f, 0.5f));
	_backgroundFade->fillParent();
}

bool ModalLayout::pointerPressed(const et::PointerInputInfo& p)
{
	Layout::pointerPressed(p);
	return true;
}

bool ModalLayout::pointerMoved(const et::PointerInputInfo& p)
{
	Layout::pointerMoved(p);
	return true;
}

bool ModalLayout::pointerReleased(const et::PointerInputInfo& p)
{
	Layout::pointerReleased(p);
	return true;
}

bool ModalLayout::pointerScrolled(const et::PointerInputInfo& p)
{
	Layout::pointerScrolled(p);
	return true;
}

}
}
