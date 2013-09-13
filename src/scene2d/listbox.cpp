/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et-ext/scene2d/listbox.h>
#include <et-ext/scene2d/layout.h>
#include <et-ext/scene2d/scenerenderer.h>

using namespace et;
using namespace et::s2d;

const float popupAppearTime = 0.1f;
const float textRevealDuration = 0.1f;

ET_DECLARE_SCENE_ELEMENT_CLASS(ListboxPopup)

ListboxPopup::ListboxPopup(Listbox* owner, const std::string& name) :
	Element2d(owner, ET_S2D_PASS_NAME_TO_BASE_CLASS), _owner(owner), _textAlphaAnimator(0),
	_selectedIndex(-1), _textAlpha(0.0f), _pressed(false)
{
	setFlag(Flag_RenderTopmost);
}

void ListboxPopup::buildVertices(SceneRenderer&)
{
	mat4 transform = finalTransform();
	_backgroundVertices.setOffset(0);
	_textVertices.setOffset(0);
	_selectionVertices.setOffset(0);

	const Image& background = _owner->_background;
	const Image& selection = _owner->_selection;

	if (background.texture.valid())
	{
		buildImageVertices(_backgroundVertices, background.texture, background.descriptor, 
			rect(vec2(0.0), size()), color(), transform);
	}

	const StringList& values = _owner->_values;
	if (values.size() && (_textAlpha > 0.0f))
	{
		bool selectionValid = selection.texture.valid();

		float row = 0.0f;
		float rowSize = _owner->size().y;
		float y0 = floorf(0.5f * (rowSize - _owner->_font->lineHeight()));
		float dy = floorf(size().y / static_cast<float>(values.size()));

		vec4 drawColor = color();
		drawColor.w *= _textAlpha;

		vec2 textPos = _owner->_contentOffset + vec2(0.0f, y0);
		
		int index = 0;
		for (auto i = values.begin(), e = values.end(); i != e; ++i, ++index, row += 1.0f)
		{
			if (selectionValid && (_selectedIndex == index))
			{
				buildImageVertices(_selectionVertices, selection.texture, selection.descriptor, 
					rect(vec2(0.0f, row * rowSize), vec2(size().x, rowSize)), drawColor, transform);
			}

			buildStringVertices(_textVertices, _owner->_font->buildString(*i), Alignment_Near,
				Alignment_Near, textPos, drawColor, transform);
			textPos.y += dy;
		}
	}

	setContentValid();
}

void ListboxPopup::revealText()
{
	hideText();

	if (_textAlphaAnimator)
		_textAlphaAnimator->destroy();

	_textAlphaAnimator = new FloatAnimator(this, &_textAlpha, _textAlpha, 1.0f,
		textRevealDuration, 0, timerPool());
}

void ListboxPopup::hideText()
{
	_textAlpha = 0.0f;
	invalidateContent();
}

void ListboxPopup::animatorUpdated(BaseAnimator* a)
{
	if (a == _textAlphaAnimator)
		invalidateContent();

	Element2d::animatorUpdated(a);
}

void ListboxPopup::animatorFinished(BaseAnimator* a)
{
	if (a == _textAlphaAnimator)
	{
		a->destroy();
		_textAlphaAnimator = 0;
		_owner->popupDidOpen();
	}
	else
		Element2d::animatorFinished(a);
}

void ListboxPopup::addToRenderQueue(RenderContext*, SceneRenderer& r)
{
	if (!contentValid() || !transformValid())
		buildVertices(r);

	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, _owner->_background.texture, r.defaultProgram(), this);

	if (_selectionVertices.lastElementIndex() > 0)
		r.addVertices(_selectionVertices, _owner->_selection.texture, r.defaultProgram(), this);

	if (_textVertices.lastElementIndex() > 0)
		r.addVertices(_textVertices, _owner->_font->texture(), r.defaultProgram(), this);
}

bool ListboxPopup::pointerPressed(const PointerInputInfo&)
{
	_pressed = true;
	return true;
}

bool ListboxPopup::pointerMoved(const PointerInputInfo& p)
{
	_selectedIndex = static_cast<int>(p.pos.y / _owner->size().y);
	invalidateContent();
	return true;
}

bool ListboxPopup::pointerReleased(const PointerInputInfo& p)
{
	if (_pressed)
	{
		_selectedIndex = static_cast<int>(p.pos.y / _owner->size().y);
		_owner->setSelectedIndex(_selectedIndex);
		_owner->hidePopup();
	}
	_pressed = false;

	return true;
}

void ListboxPopup::pointerEntered(const PointerInputInfo&)
{

}

void ListboxPopup::pointerLeaved(const PointerInputInfo&)
{
	_selectedIndex = -1;
	invalidateContent();
}

/*
 * Listbox
 */

ET_DECLARE_SCENE_ELEMENT_CLASS(Listbox)

Listbox::Listbox(Font::Pointer font, Element2d* parent, const std::string& name) :
	Element2d(parent, ET_S2D_PASS_NAME_TO_BASE_CLASS), _font(font), _state(ListboxState_Default),
	_contentOffset(0.0f), _selectedIndex(-1), _direction(ListboxPopupDirection_Bottom),
	_popupOpened(false), _popupOpening(false), _popupValid(false)
{
	_popup = ListboxPopup::Pointer::create(this);
	_popup->setVisible(false);
	
	ET_CONNECT_EVENT(_popup->elementAnimationFinished, Listbox::onPopupAnimationFinished)
}

void Listbox::setImage(const Image& img, ListboxState state)
{
	if (state == ListboxState_Default)
	{
		_contentOffset.x = floorf(2.0f / 3.0f * img.descriptor.contentOffset.left);
		_contentOffset.y = 0;
	}

	_images[state] = img;
	invalidateContent();
}

void Listbox::setBackgroundImage(const Image& img)
{
	_background = img;
	invalidateContent();
}

void Listbox::setSelectionImage(const Image& img)
{
	_selection = img;
	invalidateContent();
}

void Listbox::buildVertices(SceneRenderer&)
{
	mat4 transform = finalTransform();
	_backgroundVertices.setOffset(0);
	_textVertices.setOffset(0);

	if (_images[_state].texture.valid())
	{
		buildImageVertices(_backgroundVertices, _images[_state].texture, _images[_state].descriptor, 
			rect(vec2(0.0), size()), color(), transform);
	}

	if (shouldDrawText())
	{
		std::string textToDraw = _prefix + _values[_selectedIndex];
		vec2 textPos = _contentOffset + vec2(0.0f, 0.5f * (size().y - _font->lineHeight()));
		buildStringVertices(_textVertices, _font->buildString(textToDraw), Alignment_Near, Alignment_Near,
								textPos, color(), transform);
	}

	setContentValid();
}

void Listbox::addToRenderQueue(RenderContext*, SceneRenderer& r)
{
	if (!contentValid())
		buildVertices(r);

	if (_images[_state].texture.valid())
		r.addVertices(_backgroundVertices, _images[_state].texture, r.defaultProgram(), this);

	if (shouldDrawText())
		r.addVertices(_textVertices, _font->texture(), r.defaultProgram(), this);
}

bool Listbox::shouldDrawText()
{
	return !(_popupOpened || (_selectedIndex == -1)) && (_textVertices.lastElementIndex() > 0);
}

bool Listbox::containsPoint(const vec2& p, const vec2& np)
{
	bool inPopup = _popupOpened && _popup->containsPoint(p, np);
	return Element2d::containsPoint(p, np) || inPopup;
}

void Listbox::didChangeFrame()
{
	configurePopup();
}

bool Listbox::pointerPressed(const PointerInputInfo& p)
{
	if (_popupOpened || _popupOpening)
	{
		return _popup->pointerPressed(p);
	}
	else 
	{
		if (!_popupOpening)
			showPopup();

		return true;
	}
}

bool Listbox::pointerMoved(const PointerInputInfo& p)
{
	return _popupOpened ? _popup->pointerMoved(p) : true;
}

bool Listbox::pointerReleased(const PointerInputInfo& p)
{
	return _popupOpened ? _popup->pointerReleased(p) : true;
}

void Listbox::pointerEntered(const PointerInputInfo& p)
{
	_mouseIn = true;

	if (_popupOpened || _popupOpening)
		_popup->pointerEntered(p);
	else
		setState(ListboxState_Highlighted);
}

void Listbox::pointerLeaved(const PointerInputInfo& p)
{
	_mouseIn = false;
	if (_popupOpened || _popupOpening)
		_popup->pointerLeaved(p);
	else
		setState(ListboxState_Default);
}

void Listbox::showPopup()
{
	assert(false && "Need to move from frame to position+size");
/*
	setState(ListboxState_Opened);
	if (_popupOpening) return;

	if (!_popupValid)
		configurePopup();

	rect destSize = _popup->position + size instead of frame();
	rect currentSize = destSize;

	destSize.top = 0.0f;
	if (_direction == ListboxPopupDirection_Center)
		destSize.top = floorf(0.5f * size().y * (1.0f - static_cast<float>(_values.size())));
	else if (_direction == ListboxPopupDirection_Top)
		destSize.top = floorf(-size().y * static_cast<float>(_values.size() / 2.0f));

	currentSize.top = 0.0f;
	currentSize.height = size().y;

	_popupOpened = false;
	_popupOpening = true;

	_popup->hideText();
	_popup->setAlpha(0.0f);
	_popup->setPosition + Size instead of Frame(currentSize);
	_popup->setPosition + Size instead of Frame(destSize, popupAppearTime);
	_popup->setAlpha(1.0f, popupAppearTime);
	_popup->setVisible(true);

	owner()->setActiveElement(this);
*/ 
}

void Listbox::hidePopup()
{
	if (_popupOpening) return;

	setState(_mouseIn ? ListboxState_Highlighted : ListboxState_Default);
	_popupOpened = false;
	_popup->setVisible(false);
	popupClosed.invoke(this);
}

void Listbox::resignFocus(Element* e)
{
	if (e != _popup.ptr())
		hidePopup();
}

void Listbox::setValues(const StringList& v)
{
	_values = v;
	_popupValid = false;
}

void Listbox::addValue(const std::string& v)
{
	_values.push_back(v);
	_popupValid = false;
}

void Listbox::configurePopup()
{
	vec2 ownSize = size();
	size_t numElements = _values.size();
	float scale = (numElements < 1) ? 1.0f : static_cast<float>(numElements);

	_popup->setPosition(0.0f, 0.0f);
	_popup->setSize(ownSize.x, ownSize.y * scale);
	_popupValid = true;
}

void Listbox::setState(ListboxState s)
{
	if (_state != s)
	{
		_state = s;
		invalidateContent();
	}
}

void Listbox::setSelectedIndex(int value)
{
	_selectedIndex = (value >= 0) && (value < static_cast<int>(_values.size())) ? value : -1;
	invalidateContent();
}

void Listbox::onPopupAnimationFinished(Element2d*, AnimatedPropery)
{
	_popup->revealText();
	popupOpened.invoke(this);
}

void Listbox::popupDidOpen()
{
	_popupOpening = false;
	_popupOpened = true;
}

void Listbox::setPrefix(const std::string& prefix)
{
	_prefix = prefix;
	invalidateContent();
}

void Listbox::setPopupDirection(ListboxPopupDirection d)
{
	_direction = d;
}