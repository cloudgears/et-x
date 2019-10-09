/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/scene2d/font.hpp>
#include <et-ext/scene2d/textelement.hpp>

namespace et {
namespace s2d {
enum ListboxState { ListboxState_Default, ListboxState_Highlighted, ListboxState_Opened, ListboxState_max };

enum ListboxPopupDirection {
  ListboxPopupDirection_Top,
  ListboxPopupDirection_Center,
  ListboxPopupDirection_Bottom,
};

class Listbox : public TextElement {
 public:
  class Popup;
  ET_DECLARE_POINTER(Listbox);

 public:
  Listbox(const Font::Pointer& font, float fsz, Element2d* parent, const std::string& name = emptyString);

  void setImage(const Image& img, ListboxState state);
  void setBackgroundImage(const Image& img);
  void setSelectionImage(const Image& img);
  void setPopupDirection(ListboxPopupDirection d);

  bool containsPoint(const vec2& p, const vec2&) override;

  bool pointerPressed(const PointerInputInfo&) override;
  bool pointerMoved(const PointerInputInfo&) override;
  bool pointerReleased(const PointerInputInfo&) override;
  void pointerEntered(const PointerInputInfo&) override;
  void pointerLeaved(const PointerInputInfo&) override;

  void showPopup();
  void hidePopup();

  void resignFocus(Element2d*) override;

  void setValues(const StringList& v);
  void addValue(const std::string& v);
  const std::string& valueAtIndex(size_t) const;

  int selectedIndex() const {
    return _selectedIndex;
  }

  void setSelectedIndex(int value);

  void setPrefix(const std::string& prefix);

  const std::string& prefix() const {
    return _prefix;
  }

  const StringList& values() const {
    return _values;
  }

  ET_DECLARE_EVENT1(valueSelected, size_t);

 private:
  void addToRenderQueue(RenderInterface::Pointer&, SceneRenderer&) override;
  void buildVertices(RenderInterface::Pointer&, SceneRenderer& gr) override;

  void didChangeFrame() override;

  void configurePopup();

  void setState(ListboxState s);
  void onPopupAnimationFinished(Element2d*, AnimatedPropery);

  void popupDidOpen();

  bool shouldDrawText();

 private:
  Popup* _popup = nullptr;

  Image _images[ListboxState_max];
  Image _background;
  Image _selection;
  SceneVertexList _backgroundVertices;
  SceneVertexList _textVertices;
  std::string _prefix;
  StringList _values;
  ListboxState _state = ListboxState_Default;
  vec2 _contentOffset;
  ListboxPopupDirection _direction = ListboxPopupDirection_Bottom;
  int _selectedIndex = -1;
  bool _popupValid = false;
  bool _mouseIn = false;
};
}  // namespace s2d
}  // namespace et
