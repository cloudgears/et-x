/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et-ext/scene2d/layout.h>
#include <et-ext/scene2d/imageview.h>
#include <et-ext/scene2d/label.h>
#include <et-ext/scene2d/button.h>

namespace et
{
	namespace s2d
	{
		enum MessageViewButton
		{
			MessageViewButton_First = 0x01,
			MessageViewButton_Second = 0x02,
			MessageViewButton_Any = 0xf0,
		};
		
		class MessageView : public ModalLayout
		{
		public:
			ET_DECLARE_POINTER(MessageView)
			
		public:
			MessageView(const std::string& title, const std::string& text, const Font::Pointer& font, const Image& bgImage = Image(),
						const std::string& button1title = "Close", const std::string& button2title = "Ok");

			void setContentOffset(const vec2& offset)
				{ _contentOffset = offset; }
			
			void setImage(const Image& img);
			void setBackgroundImage(const Image& img);
			
			// void layout(const vec2&); - to be replaced
			
			void setText(const std::string& text);
			void setTitle(const std::string& text);
			
			void setButton1Title(const std::string&);
			void setButton2Title(const std::string&);
			void setButtonsBackground(const Image& img, State s);
			void setButtonsTextColor(const vec4& color);
			void setButtonsPressedTextColor(const vec4& color);
			
			void setButtonsEnabled(bool b1, bool b2, bool animated);

			ET_DECLARE_EVENT2(messageViewButtonSelected, MessageView*, MessageViewButton)
			
		private:
			void buttonClicked(Button* btn);
			
			bool hasFirstButton() const;
			bool hasSecondButton() const;
			
		private:
			ImageView::Pointer _imgBackground;
			ImageView::Pointer _imgImage;
			Label::Pointer _title;
			Label::Pointer _text;
			Button::Pointer _button1;
			Button::Pointer _button2;
			Button::Pointer _buttonCommon;
			
			vec2 _contentOffset;
		};
	}
}
