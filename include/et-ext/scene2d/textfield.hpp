/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/core/notifytimer.hpp>
#include <et/scene2d/font.hpp>
#include <et/scene2d/textelement.hpp>

namespace et {
namespace s2d {
class TextField : public TextElement {
 public:
  ET_DECLARE_POINTER(TextField);

  enum {
    EditingFlag_ClearOnEscape = 0x0001,
    EditingFlag_ResignFocusOnReturn = 0x0002,
  };

 public:
  TextField(const Font::Pointer&, float, Element2d*, const std::string& = emptyString);
  TextField(const std::string&, const Font::Pointer&, float, Element2d*, const std::string& = emptyString);
  TextField(const Image&, const std::string&, const Font::Pointer&, float, Element2d*, const std::string& = emptyString);

  const std::string& text() const;

  void setBackgroundImage(const Image&);

  const Image& backgroundImage() const {
    return _background;
  }

  void setText(const std::string& s);
  void setPrefix(const std::string& s);

  void setPlaceholder(const std::string& s);

  void setEditingFlags(size_t);

  void setSecured(bool);
  void setBackgroundColor(const vec4& color);
  void setContentOffset(const vec2&);

  void setFocus() override;
  void resignFocus(Element2d*) override;

  ET_DECLARE_EVENT1(editingStarted, TextField*);
  ET_DECLARE_EVENT1(textChanged, TextField*);
  ET_DECLARE_EVENT1(returnReceived, TextField*);
  ET_DECLARE_EVENT1(editingFinished, TextField*);

 private:
  void addToRenderQueue(RenderInterface::Pointer&, SceneRenderer&) override;
  void buildVertices(RenderInterface::Pointer&, SceneRenderer&) override;

  bool processMessage(const Message& msg) override;
  void onCreateBlinkTimerExpired(NotifyTimer* t);

 private:
  Image _background;

  std::string _text;
  std::string _prefix;
  std::string _actualText;

  LocalizedText _placeholder;

  CharDescriptorList _textCharacters;
  CharDescriptorList _placeholderCharacters;

  CharDescriptorList _caretChar;
  SceneVertexList _imageVertices;
  SceneVertexList _textVertices;
  SceneVertexList _backgroundVertices;
  NotifyTimer _caretBlinkTimer;

  vec4 _backgroundColor;
  vec2 _contentOffset;

  FlagsHolder _editingFlags;
  bool _secured = false;
  bool _focused = false;
  bool _caretVisible = false;
};
}  // namespace s2d
}  // namespace et
