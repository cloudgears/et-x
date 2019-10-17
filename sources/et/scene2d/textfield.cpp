/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene2d/layout.hpp>
#include <et/scene2d/textfield.hpp>

namespace et {
namespace s2d {

const std::string caret = "|";
const int securedChar = 0x2022;

TextField::TextField(const Font::Pointer& f, float fsz, Element2d* parent, const std::string& name)
  : TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS) {
  setEditingFlags(EditingFlag_ResignFocusOnReturn);
  setFlag(Flag_RequiresKeyboard | Flag_ClipToBounds);
  setTextAlignment(s2d::Alignment::Near, s2d::Alignment::Center);
  ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired);
}

TextField::TextField(const std::string& text, const Font::Pointer& f, float fsz, Element2d* parent, const std::string& name)
  : TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS) {
  setText(text);
  setEditingFlags(EditingFlag_ResignFocusOnReturn);
  setFlag(Flag_RequiresKeyboard | Flag_ClipToBounds);
  setTextAlignment(s2d::Alignment::Near, s2d::Alignment::Center);
  ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired);
}

TextField::TextField(const Image& background, const std::string& text, const Font::Pointer& f, float fsz, Element2d* parent, const std::string& name)
  : TextElement(parent, f, fsz, ET_S2D_PASS_NAME_TO_BASE_CLASS)
  , _background(background) {
  setSize(font()->measureStringSize(text, fontSize(), fontSmoothing()));

  setText(text);
  setEditingFlags(EditingFlag_ResignFocusOnReturn);
  setFlag(Flag_RequiresKeyboard | Flag_ClipToBounds);
  setTextAlignment(s2d::Alignment::Near, s2d::Alignment::Center);

  ET_CONNECT_EVENT(_caretBlinkTimer.expired, TextField::onCreateBlinkTimerExpired);
}

void TextField::addToRenderQueue(RenderInterface::Pointer& rc, SceneRenderer& r) {
  if (_backgroundVertices.lastElementIndex() > 0) {
    materialInstance()->setTexture(MaterialTexture::BaseColor, _background.texture);
    r.addVertices(_backgroundVertices, materialInstance(), this);
  }

  if (_imageVertices.lastElementIndex() > 0) {
    materialInstance()->setTexture(MaterialTexture::BaseColor, _background.texture);
    r.addVertices(_imageVertices, materialInstance(), this);
  }

  if (_textVertices.lastElementIndex() > 0) {
    textMaterial(r)->setTexture(MaterialTexture::BaseColor, font()->generator()->texture());
    r.addVertices(_textVertices, textMaterial(r), this);
  }
}

void TextField::buildVertices(RenderInterface::Pointer&, SceneRenderer&) {
  vec4 alphaScale = vec4(1.0f, finalAlpha());
  mat4 transform = finalTransform();
  rect wholeRect(vec2(0.0f), size());

  _backgroundVertices.setOffset(0);
  _imageVertices.setOffset(0);
  _textVertices.setOffset(0);

  if (_backgroundColor.w > 0.0f) buildColorVertices(_backgroundVertices, wholeRect, _backgroundColor * alphaScale, transform);

  if (is_valid(_background.texture)) {
    buildImageVertices(_imageVertices, _background.texture, _background.descriptor, wholeRect, alphaScale, transform);
  }

  vec2 textSize = font()->measureStringSize(_textCharacters);
  vec2 caretSize = font()->measureStringSize(_caretChar);

  if (_textCharacters.empty()) textSize = caretSize * vec2(0.0f, 1.0f);

  auto actualAlignment = textHorizontalAlignment();

  if (!_focused && !_placeholderCharacters.empty() && _textCharacters.empty()) {
    vec2 placeholderSize = font()->measureStringSize(_placeholderCharacters);

    vec2 placeholderOrigin = _contentOffset + vec2(alignmentFactor(actualAlignment), alignmentFactor(textVerticalAlignment())) * ((wholeRect.size() - 2.0f * _contentOffset) - placeholderSize);

    buildStringVertices(_textVertices, _placeholderCharacters, Alignment::Near, Alignment::Near, placeholderOrigin, finalColor() * vec4(1.0f, 0.5f), transform, 1.0f);
  }

  float widthAdjustment = 0.0f;
  actualAlignment = textHorizontalAlignment();
  if (_focused && (textSize.x + caretSize.x >= wholeRect.width - _contentOffset.x)) {
    actualAlignment = Alignment::Far;
    textSize.x += caretSize.x;
    textSize.y = std::max(caretSize.y, textSize.y);
    widthAdjustment = caretSize.x;
  }

  vec2 textOrigin = _contentOffset + vec2(alignmentFactor(actualAlignment), alignmentFactor(textVerticalAlignment())) * ((wholeRect.size() - 2.0f * _contentOffset) - textSize);

  if (!_textCharacters.empty()) {
    buildStringVertices(_textVertices, _textCharacters, Alignment::Near, Alignment::Near, textOrigin, finalColor(), transform, 1.0f);
  }

  if (_caretVisible) {
    buildStringVertices(_textVertices, _caretChar, s2d::Alignment::Near, Alignment::Near, textOrigin + vec2(textSize.x - widthAdjustment, 0.0f), finalColor(), transform);
  }

  setContentValid();
}

void TextField::setText(const std::string& s) {
  _text = s;
  _actualText = _prefix + _text;

  if (_secured) {
    _textCharacters = CharDescriptorList(_actualText.length(), font()->generator()->charDescription(securedChar));
  } else {
    font()->buildString(_actualText, fontSize(), fontSmoothing(), _textCharacters);
  }

  font()->buildString(caret, fontSize(), fontSmoothing(), _caretChar);

  invalidateContent();
}

bool TextField::processMessage(const Message& msg) {
  bool result = TextElement::processMessage(msg);

  if (msg.type == Message::Type_TextFieldControl) {
    switch (msg.param) {
      case ET_KEY_RETURN: {
        returnReceived.invoke(this);
        if (_editingFlags.hasFlag(EditingFlag_ResignFocusOnReturn)) {
          owner()->setFocusedElement(Element2d::Pointer());
          result = true;
        }
        break;
      }

      case ET_KEY_BACKSPACE: {
        if (_text.length() > 0) {
          size_t charToErase = 1;
          while (_text.size() > charToErase) {
            auto lastChar = _text[_text.length() - charToErase];
            if (lastChar & 0x80) {
              do {
                charToErase++;
                lastChar = _text[_text.length() - charToErase];
              } while ((_text.size() > charToErase) && (lastChar & 0x40) == 0);
              break;
            } else {
              break;
            }
          }

          setText(_text.substr(0, _text.length() - charToErase));
        }

        textChanged.invoke(this);
        result = true;
        break;
      }

      case ET_KEY_ESCAPE: {
        if (_editingFlags.hasFlag(EditingFlag_ClearOnEscape)) {
          setText(emptyString);
          textChanged.invoke(this);
          result = true;
        }
        break;
      }

      default:
        break;
    }
  } else if (msg.type == Message::Type_TextInput) {
    setText(_text + msg.text);
    textChanged.invoke(this);
    invalidateContent();
    result = true;
  } else if (msg.type == Message::Type_UpdateText) {
    setPlaceholder(_placeholder.key);
    result = true;
  }

  return result;
}

void TextField::setSecured(bool s) {
  _secured = s;
  invalidateContent();
}

void TextField::setFocus() {
  _caretBlinkTimer.start(timerPool(), 0.5f, NotifyTimer::RepeatForever);
  _focused = true;
  _caretVisible = true;
  invalidateContent();

  editingStarted.invoke(this);
}

void TextField::resignFocus(Element2d*) {
  _caretBlinkTimer.cancelUpdates();
  _focused = false;
  _caretVisible = false;
  invalidateContent();

  editingFinished.invoke(this);
}

void TextField::onCreateBlinkTimerExpired(NotifyTimer*) {
  _caretVisible = !_caretVisible;
  invalidateContent();
}

const std::string& TextField::text() const {
  return _text;
}

void TextField::setBackgroundColor(const vec4& color) {
  _backgroundColor = color;
  invalidateContent();
}

void TextField::setPrefix(const std::string& s) {
  _prefix = s;
  setText(_text);
  invalidateContent();
}

void TextField::setEditingFlags(size_t f) {
  _editingFlags.setFlags(f);
}

void TextField::setContentOffset(const vec2& o) {
  _contentOffset = o;
  invalidateContent();
}

void TextField::setBackgroundImage(const Image& img) {
  _background = img;
  invalidateContent();
}

void TextField::setPlaceholder(const std::string& s) {
  _placeholder.setKey(s);
  font()->buildString(_placeholder.cachedText, fontSize(), fontSmoothing(), _placeholderCharacters);
  invalidateContent();
}

}  // namespace s2d
}  // namespace et
