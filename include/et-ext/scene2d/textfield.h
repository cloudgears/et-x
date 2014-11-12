/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/timers/notifytimer.h>
#include <et-ext/scene2d/element2d.h>
#include <et-ext/scene2d/font.h>

namespace et
{
	namespace s2d
	{
		class TextField : public Element2d
		{
		public:
			ET_DECLARE_POINTER(TextField)
			
			enum
			{
				EditingFlag_ClearOnEscape = 0x0001,
				EditingFlag_ResignFocusOnReturn = 0x0002,
			};
			
		public:
			TextField(const Font::Pointer& font, Element2d* parent, const std::string& name = emptyString);
			
			TextField(const std::string& text, const Font::Pointer& font, Element2d* parent,
				const std::string& name = emptyString);

			TextField(const Image& background, const std::string& text, const Font::Pointer& font,
				Element2d* parent, const std::string& name = emptyString);

			const std::string& text() const;
			
			void setBackgroundImage(const Image&);
			
			const Image& backgroundImage() const
				{ return _background; }
			
			void setText(const std::string& s);
			void setPrefix(const std::string& s);
			
			void setPlaceholder(const std::string& s);
			
			void setEditingFlags(size_t);

			void setSecured(bool);
			void setBackgroundColor(const vec4& color);
			void setVerticalAlignment(Alignment);
			void setHorizontalAlignment(Alignment);
			void setContentOffset(const vec2&);

			void setFocus();
			void resignFocus(Element2d*);
			
			ET_DECLARE_EVENT1(editingStarted, TextField*)
			ET_DECLARE_EVENT1(textChanged, TextField*)
			ET_DECLARE_EVENT1(returnReceived, TextField*)
			ET_DECLARE_EVENT1(editingFinished, TextField*)
			
		private:
			void addToRenderQueue(RenderContext*, SceneRenderer&);
			void buildVertices(RenderContext*, SceneRenderer&);
			
			void processMessage(const Message& msg);
			void onCreateBlinkTimerExpired(NotifyTimer* t);

		private:
			Font::Pointer _font;
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
			Alignment _alignmentV;
			Alignment _alignmentH;
			bool _secured = false;
			bool _focused = false;
			bool _caretVisible = false;
		};
	}
}
