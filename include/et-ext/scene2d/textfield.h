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
			
		public:
			TextField(const Image& background, const std::string& text, Font::Pointer font, 
				Element* parent, const std::string& name = std::string());

			void addToRenderQueue(RenderContext*, SceneRenderer&);

			const std::string& text() const;
			void setText(const std::string& s);

			void processMessage(const GuiMessage& msg);
			void setSecured(bool);

			void setFocus();
			void resignFocus(Element*);
			
			void setBackgroundColor(const vec4& color);
			
			ET_DECLARE_EVENT1(editingStarted, TextField*)
			ET_DECLARE_EVENT1(textChanged, TextField*)
			ET_DECLARE_EVENT1(editingFinished, TextField*)
			
		private:
			void buildVertices(RenderContext*, SceneRenderer&);
			void onCreateBlinkTimerExpired(NotifyTimer* t);

		private:
			Font::Pointer _font;
			Image _background;
			std::string _text;
			CharDescriptorList _charList;
			SceneVertexList _imageVertices;
			SceneVertexList _textVertices;
			SceneVertexList _backgroundVertices;
			NotifyTimer _caretBlinkTimer;
			vec4 _backgroundColor;
			bool _secured;
			bool _caretVisible;
		};
	}
}