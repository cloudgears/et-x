/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/element2d.h>
#include <et-ext/scene2d/font.h>

namespace et
{
	namespace s2d
	{
		enum ListboxState
		{
			ListboxState_Default,
			ListboxState_Highlighted,
			ListboxState_Opened,
			ListboxState_max
		};

		enum ListboxPopupDirection
		{
			ListboxPopupDirection_Top,
			ListboxPopupDirection_Center,
			ListboxPopupDirection_Bottom,
		};

		class Listbox;

		class ListboxPopup : public Element2d
		{
		public:
			ET_DECLARE_POINTER(ListboxPopup)
			
		public:
			ListboxPopup(Listbox* owner, const std::string& name = emptyString);
			
			void setBackgroundImage(const Image& img);

			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			void pointerEntered(const PointerInputInfo&);
			void pointerLeaved(const PointerInputInfo&);

			void hideText();
			void revealText();

		private:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			void buildVertices(SceneRenderer& gr);

		private:
			Listbox* _owner;
			SceneVertexList _backgroundVertices;
			SceneVertexList _selectionVertices;
			SceneVertexList _textVertices;
			FloatAnimator _textAlphaAnimator;
			int _selectedIndex;
			float _textAlpha;
			bool _pressed;
		};

		class Listbox : public Element2d
		{
		public:
			ET_DECLARE_POINTER(Listbox)

		public:
			Listbox(const Font::Pointer& font, Element2d* parent, const std::string& name = emptyString);

			void setImage(const Image& img, ListboxState state);
			void setBackgroundImage(const Image& img);
			void setSelectionImage(const Image& img);
			void setPopupDirection(ListboxPopupDirection d);

			bool containsPoint(const vec2& p, const vec2&);

			bool pointerPressed(const PointerInputInfo&);
			bool pointerMoved(const PointerInputInfo&);
			bool pointerReleased(const PointerInputInfo&);
			void pointerEntered(const PointerInputInfo&);
			void pointerLeaved(const PointerInputInfo&);

			void showPopup();
			void hidePopup();

			void resignFocus(Element2d*);

			void setValues(const StringList& v);
			void addValue(const std::string& v);

			int selectedIndex() const
				{ return _selectedIndex; }

			void setSelectedIndex(int value);

			void setPrefix(const std::string& prefix);

			const std::string& prefix() const
				{ return _prefix; }

			const StringList& values() const 
				{ return _values; }

			ET_DECLARE_EVENT1(popupOpened, Listbox*)
			ET_DECLARE_EVENT1(popupClosed, Listbox*)

		private:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			void buildVertices(SceneRenderer& gr);
			
			void didChangeFrame();
			
			void configurePopup();

			void setState(ListboxState s);
			void onPopupAnimationFinished(Element2d*, AnimatedPropery);

			void popupDidOpen();

			bool shouldDrawText();
			
		private:
			friend class ListboxPopup;

			Font::Pointer _font;
			ListboxPopup::Pointer _popup;
			Image _images[ListboxState_max];
			Image _background;
			Image _selection;
			SceneVertexList _backgroundVertices;
			SceneVertexList _textVertices;
			std::string _prefix;
			StringList _values;
			ListboxState _state;
			vec2 _contentOffset;
			ListboxPopupDirection _direction;
			int _selectedIndex;
			bool _popupOpened;
			bool _popupOpening;
			bool _popupValid;
			bool _mouseIn;
		};
	}
}
