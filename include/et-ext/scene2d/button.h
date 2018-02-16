/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/textelement.h>
#include <et-ext/scene2d/font.h>

namespace et
{
namespace s2d
{
class Button : public TextElement
{
public:
	ET_DECLARE_POINTER(Button);
	typedef std::list<Pointer> List;

	enum Type
	{
		Type_PushButton,
		Type_CheckButton
	};

	enum ImageLayout
	{
		ImageLayout_Left,
		ImageLayout_Right,
		ImageLayout_Top,
		ImageLayout_Bottom
	};

	enum ContentMode
	{
		ContentMode_Fit,
		ContentMode_ScaleMaxToMin,
		ContentMode_Center,
	};

public:
	Button(const std::string& title, const Font::Pointer& font, float, Element2d* parent,
		const std::string& name = emptyString);

	void setImage(const Image& img);
	void setImageForState(const Image& img, State s);

	void setCommonBackground(const Image& img);

	void setBackground(const Image& img);
	void setBackgroundForState(const Image& img, State s);

	void adjustSize(float duration = 0.0f, bool vertical = true, bool horizontal = true);
	void adjustSizeForText(const std::string&, float duration = 0.0f, bool vertical = true, bool horizontal = true);
	vec2 sizeForText(const std::string&, const std::string& = "AA");

	void setContentMode(ContentMode);

	ET_DECLARE_EVENT1(clicked, Button*);
	ET_DECLARE_EVENT1(pressed, Button*);
	ET_DECLARE_EVENT1(released, Button*);
	ET_DECLARE_EVENT1(releasedInside, Button*);
	ET_DECLARE_EVENT1(releasedOutside, Button*);
	ET_DECLARE_EVENT1(cancelled, Button*);

	const Image& backgroundForState(State state) const
	{
		return _background[state];
	}

	bool capturePointer() const;

	const std::string& title() const
	{
		return _nextTitle.cachedText;
	}

	const std::string& titleKey() const
	{
		return _nextTitle.key;
	}

	void setTitle(const std::string&, float duration = 0.0f);

	const Image& imageForState(State s) const
	{
		return _image[s];
	}

	void setImageLayout(ImageLayout l);

	const vec2& textSize() const
	{
		return _maxTextSize;
	}

	void setTextColor(const vec4& color);
	const vec4& textColor() const;

	void setPressedColor(const vec4& color);
	const vec4& pressedColor() const;

	void setTextPressedColor(const vec4& color);
	const vec4& textPressedColor() const;

	void setBackgroundColor(const vec4&);
	const vec4& backgroundColor() const;

	void setBackgroundTintColor(const vec4&, float);
	const vec4& backgroundTintColor() const;

	void setCommonBackgroundTintColor(const vec4&, float);
	const vec4& commonBackgroundTintColor() const;

	bool pointerPressed(const PointerInputInfo&) override;
	bool pointerReleased(const PointerInputInfo&) override;
	bool pointerCancelled(const PointerInputInfo&) override;
	void pointerEntered(const PointerInputInfo&) override;
	void pointerLeaved(const PointerInputInfo&) override;

	Button::Type type() const
	{
		return _type;
	}

	void setType(Button::Type t);

	bool selected() const
	{
		return _selected;
	}

	void setSelected(bool s);

	void setContentOffset(const vec2& o);

	vec2 contentSize();

	void setShouldAdjustPressedBackground(bool);

	bool processMessage(const Message&) override;

	void setClickTreshold(float);
	void setShouldInvokeClickInRunLoop(bool);

	void setAction(Action a)
	{
		_action = a;
	}

	const vec2& imageOrigin() const
	{
		return _imageOrigin;
	}

	const vec2& imageSize() const
	{
		return _imageSize;
	}

	const vec2& textOrigin() const
	{
		return _textOrigin;
	}

protected:
	void performClick();
	bool respondsToMessage(const Message&) const override;

private:
	void buildVertices(RenderInterface::Pointer& rc, SceneRenderer& gr) override;
	void addToRenderQueue(RenderInterface::Pointer& rc, SceneRenderer& renderer) override;

	void setCurrentState(State s);

	void invalidateText() override;

private:
	LocalizedText _currentTitle;
	LocalizedText _nextTitle;

	SceneVertexList _bgVertices;
	SceneVertexList _textVertices;
	SceneVertexList _imageVertices;

	CharDescriptorList _currentTitleCharacters;
	CharDescriptorList _nextTitleCharacters;

	Image _commonBackground;
	StaticDataStorage<Image, State_max> _background;
	StaticDataStorage<Image, State_max> _image;

	vec4 _textColor = vec4(0.0f, 1.0f);
	vec4 _textPressedColor = vec4(0.0f, 1.0f);
	vec4 _pressedColor = vec4(0.5f, 1.0f);
	vec4 _backgroundColor = vec4(0.0f);
	vec4 _backgroundTintColor = vec4(1.0f);
	vec4 _commonBackgroundTintColor = vec4(1.0f);
	vec2 _currentTextSize = vec2(0.0f);
	vec2 _nextTextSize = vec2(0.0f);
	vec2 _maxTextSize = vec2(0.0f);
	vec2 _contentOffset = vec2(0.0f);
	vec2 _imageOrigin = vec2(0.0f);
	vec2 _imageSize = vec2(0.0f);
	vec2 _textOrigin = vec2(0.0f);

	FloatAnimator _titleAnimator;
	Vector4Animator _backgroundTintAnimator;
	Vector4Animator _commonBackgroundTintAnimator;

	Type _type = Type_PushButton;
	State _state = State_Default;
	ImageLayout _imageLayout = ImageLayout_Left;
	ContentMode _contentMode = ContentMode_Fit;
	Action _action = Action_None;

	float _lastClickTime = 0.0f;
	float _clickTreshold = 0.0f;

	bool _pressed = false;
	bool _selected = false;
	bool _adjustPressedBackground = false;
	bool _shouldInvokeClickInRunLoop = false;
};
}
}
