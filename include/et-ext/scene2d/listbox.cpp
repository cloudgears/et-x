/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/scene2d/listbox.h>
#include <et-ext/scene2d/layout.h>
#include <et-ext/scene2d/scenerenderer.h>

namespace et {
namespace s2d {

namespace
{
	const float popupAppearTime = 0.3f;
}

class Listbox::Popup : public Element2d
{
public:
	Popup(Listbox* owner, const std::string& name = emptyString);
	
	void setBackgroundImage(const Image& img);
	
	bool pointerPressed(const PointerInputInfo&) override;
	bool pointerMoved(const PointerInputInfo&) override;
	bool pointerReleased(const PointerInputInfo&) override;
	void pointerEntered(const PointerInputInfo&) override;
	void pointerLeaved(const PointerInputInfo&) override;
	void hideText();
	void revealText();
	void addToRenderQueue(RenderContext*, SceneRenderer&) override;
	void buildVertices(SceneRenderer& gr);
	
private:
	Listbox* _owner = nullptr;
	SceneVertexList _backgroundVertices;
	SceneVertexList _selectionVertices;
	SceneVertexList _textVertices;
	int _selectedIndex = -1;
	bool _pressed = false;
};

Listbox::Popup::Popup(Listbox* owner, const std::string& name) :
	Element2d(owner, ET_S2D_PASS_NAME_TO_BASE_CLASS), _owner(owner)
{
	setFlag(Flag_RenderTopmost);
}

void Listbox::Popup::buildVertices(SceneRenderer&)
{
	mat4 transform = finalTransform();
	_backgroundVertices.setOffset(0);
	_textVertices.setOffset(0);
	_selectionVertices.setOffset(0);

	const Image& background = _owner->_background;
	const Image& selection = _owner->_selection;
    vec4 drawColor = finalColor();
	
	rectf wholeRect(vec2(0.0f), size());
	buildColorVertices(_backgroundVertices, wholeRect, vec4(0.25f, 1.0f) * finalColor(), transform);

	if (background.texture.valid())
	{
		buildImageVertices(_backgroundVertices, background.texture, background.descriptor, 
			wholeRect, finalColor(), transform);
	}
	
	const StringList& values = _owner->_values;
	if (!values.empty())
    {
		float rowSize = _owner->size().y;
		
		int index = 0;
		for (const auto& i : values)
		{
			float row = static_cast<float>(index) * rowSize;
            rectf rowRect(vec2(0.0f, row), vec2(size().x, rowSize));
			
			if (_selectedIndex == index)
			{
                buildColorVertices(_backgroundVertices, rowRect, vec4(0.5f, 1.0f) * drawColor, transform);
                if (selection.texture.valid())
                {
                    buildImageVertices(_selectionVertices, selection.texture, selection.descriptor,
                        rowRect, drawColor, transform);
                }
			}

            auto charList = _owner->font()->buildString(i, _owner->fontSize(), _owner->fontSmoothing());
            auto textSize = _owner->font()->measureStringSize(i, _owner->fontSize());
            auto textPos = rowRect.origin() + 0.5f * (rowRect.size() - textSize);
            buildStringVertices(_textVertices, charList, Alignment_Near, Alignment_Near, textPos, drawColor, transform);
            
            ++index;
		}
	}

	setContentValid();
}

void Listbox::Popup::revealText()
{
    invalidateContent();
}

void Listbox::Popup::hideText()
{
	invalidateContent();
}

void Listbox::Popup::addToRenderQueue(RenderContext*, SceneRenderer& r)
{
	validateMaterialInstance(r);

	if (!contentValid() || !transformValid())
		buildVertices(r);

	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, _owner->_background.texture, materialInstance(), this);

	if (_selectionVertices.lastElementIndex() > 0)
		r.addVertices(_selectionVertices, _owner->_selection.texture, materialInstance(), this);

	if (_textVertices.lastElementIndex() > 0)
		r.addVertices(_textVertices, _owner->font()->generator()->texture(), _owner->textMaterial(r), this);
}

bool Listbox::Popup::pointerPressed(const PointerInputInfo&)
{
	_pressed = true;
	return true;
}

bool Listbox::Popup::pointerMoved(const PointerInputInfo& p)
{
	_selectedIndex = static_cast<int>(p.pos.y / _owner->size().y);
	invalidateContent();
	return true;
}

bool Listbox::Popup::pointerReleased(const PointerInputInfo& p)
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

void Listbox::Popup::pointerEntered(const PointerInputInfo&)
{

}

void Listbox::Popup::pointerLeaved(const PointerInputInfo&)
{
	_selectedIndex = -1;
	invalidateContent();
}

/*
 * Listbox
 */

Listbox::Listbox(const Font::Pointer& f, float fsz, Element2d* parent, const std::string& name) :
	TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS)
{
	_popup = etCreateObject<Popup>(this);
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
	_backgroundVertices.setOffset(0);
	_textVertices.setOffset(0);

	mat4 transform = finalTransform();
	buildColorVertices(_backgroundVertices, rectf(vec2(0.0f), size()), vec4(1.0f, 0.5f), transform);
	
	if (_images[_state].texture.valid())
	{
		buildImageVertices(_backgroundVertices, _images[_state].texture, _images[_state].descriptor, 
			rectf(vec2(0.0f), size()), finalColor(), transform);
	}

	if (shouldDrawText())
	{
		std::string textToDraw = _prefix + _values[_selectedIndex];
        auto textSize = font()->measureStringSize(textToDraw, fontSize());
        auto charList = font()->buildString(textToDraw, fontSize(), fontSmoothing());
        vec2 textPos = 0.5f * (size() - textSize);
		buildStringVertices(_textVertices, charList, Alignment_Near, Alignment_Near, textPos, finalColor(), transform);
	}

	setContentValid();
}

void Listbox::addToRenderQueue(RenderContext*, SceneRenderer& r)
{
	validateMaterialInstance(r);

	if (!contentValid())
		buildVertices(r);

	if (_backgroundVertices.lastElementIndex() > 0)
		r.addVertices(_backgroundVertices, _images[_state].texture, materialInstance(), this);

	if (_textVertices.lastElementIndex() > 0)
		r.addVertices(_textVertices, font()->generator()->texture(), textMaterial(r), this);
}

bool Listbox::shouldDrawText()
{
    return !_values.empty();
}

bool Listbox::containsPoint(const vec2& p, const vec2& np)
{
	bool inPopup = _popup->visible() && _popup->containsPoint(p, np);
	return Element2d::containsPoint(p, np) || inPopup;
}

void Listbox::didChangeFrame()
{
	configurePopup();
}

bool Listbox::pointerPressed(const PointerInputInfo& p)
{
	if (_popup->visible())
		return _popup->pointerPressed(p);

    showPopup();
    return true;
}

bool Listbox::pointerMoved(const PointerInputInfo& p)
{
	return _popup->visible() ? _popup->pointerMoved(p) : true;
}

bool Listbox::pointerReleased(const PointerInputInfo& p)
{
	return _popup->visible() ? _popup->pointerReleased(p) : true;
}

void Listbox::pointerEntered(const PointerInputInfo& p)
{
	_mouseIn = true;
    if (_popup->visible())
        _popup->pointerEntered(p);
    else
		setState(ListboxState_Highlighted);
}

void Listbox::pointerLeaved(const PointerInputInfo& p)
{
	_mouseIn = false;
    if (_popup->visible())
		_popup->pointerLeaved(p);
	else
		setState(ListboxState_Default);
}

void Listbox::showPopup()
{
	setState(ListboxState_Opened);

	if (!_popupValid)
		configurePopup();

	vec2 relativeSize = vec2(1.0f, _values.empty() ? 1.0f : _values.size());
	
	_popup->hideText();
	_popup->setAutolayoutRelativeToParent(vec2(0.0f, 1.0f), relativeSize, vec2(0.0f, 1.0f));
	_popup->setVisible(true, popupAppearTime);
	_popup->autoLayout(owner()->size());
	
	owner()->setFocusedElement(s2d::Element2d::Pointer(_popup));
	invalidateContent();
}

void Listbox::hidePopup()
{
	setState(_mouseIn ? ListboxState_Highlighted : ListboxState_Default);
	_popup->setVisible(false);
    owner()->setFocusedElement(s2d::Element2d::Pointer(this));
}

void Listbox::resignFocus(Element2d* e)
{
	if (e != _popup)
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

const std::string& Listbox::valueAtIndex(size_t index) const
{
    ET_ASSERT(index < _values.size());
    return _values.at(index);
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
    
    size_t szValue = static_cast<size_t>(_selectedIndex);
    if (szValue < _values.size())
        valueSelected.invokeInMainRunLoop(szValue);
    
    invalidateContent();
}

void Listbox::onPopupAnimationFinished(Element2d*, AnimatedPropery)
{
}

void Listbox::popupDidOpen()
{
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

}
}
