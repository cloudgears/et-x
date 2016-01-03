/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/texture.h>
#include <et/rendering/program.h>
#include <et/locale/locale.hpp>
#include <et/timers/animator.h>
#include <et-ext/scene2d/baseconst.h>

namespace et
{
	namespace s2d
	{
		class SceneRenderer;

		template <typename T>
		struct ElementClass
		{
			static const std::string className;
			static AtomicCounter instanceConter;
			
			static std::string uniqueName(const std::string&);
		};

#		define ET_DECLARE_SCENE_ELEMENT_CLASS(CLASS) template <> \
			const std::string et::s2d::ElementClass<et::s2d::CLASS*>::className = std::string(#CLASS);\
			template <> et::AtomicCounter et::s2d::ElementClass<et::s2d::CLASS*>::instanceConter = { };\
			template <> std::string et::s2d::ElementClass<et::s2d::CLASS*>::uniqueName(const std::string& inputName)\
			{ return (inputName.empty()) ? className + intToStr(instanceConter.retain()) : inputName; }

#		define ET_S2D_PASS_NAME_TO_BASE_CLASS ElementClass<decltype(this)>::uniqueName(name)

		struct AnimationDescriptor
		{
		public:
			size_t flags;
			float duration;

		public:
			AnimationDescriptor() :
				flags(AnimationFlag_None), duration(0.0f) { }

			AnimationDescriptor(size_t aFlags, float aDuration) :
				flags(aFlags), duration(aDuration) { }
		};

		struct ContentOffset
		{
		public:
			float left = 0.0f;
			float top = 0.0f;
			float right = 0.0f;
			float bottom = 0.0f;

		public:
			ContentOffset(float value = 0.0f) :
				left(value), top(value), right(value), bottom(value) { }

			ContentOffset(const vec2& values) :
				left(values.x), top(values.y), right(values.x), bottom(values.y) { }

			ContentOffset(float l, float t, float r, float b) :	
				left(l), top(t), right(r), bottom(b) { }

			const vec2 origin() const
				{ return vec2(left, top); }

			ContentOffset& operator *= (float t)
			{
				left *= t;
				top *= t;
				right *= t;
				bottom *= t;
				return *this;
			}

			ContentOffset& operator += (const ContentOffset& r)
			{
				left += r.left;
				top += r.top;
				right += r.right;
				bottom += r.bottom;
				return *this;
			}
		};

		struct ImageDescriptor
		{
			vec2 origin;
			vec2 size;
			ContentOffset contentOffset;

			ImageDescriptor() : 
				origin(0.0f), size(0.0f) { } 

			ImageDescriptor(const Texture::Pointer& tex) :
				origin(0.0f), size(tex.valid() ? tex->sizeFloat() : vec2(0.0f)) { }

			ImageDescriptor(const Texture::Pointer& tex, const ContentOffset& offset) : 
				origin(0.0f), size(tex.valid() ? tex->sizeFloat() : vec2(0.0f)), contentOffset(offset) { }

			ImageDescriptor(const vec2& aOrigin, const vec2& aSize, const ContentOffset& offset = ContentOffset()) :
				origin(aOrigin), size(aSize), contentOffset(offset) { }

			vec2 centerPartTopLeft() const 
				{ return origin + contentOffset.origin(); }

			vec2 centerPartTopRight() const 
				{ return origin + vec2(size.x - contentOffset.right, contentOffset.top); }

			vec2 centerPartBottomLeft() const 
				{ return origin + vec2(contentOffset.left, size.y - contentOffset.bottom); }

			vec2 centerPartBottomRight() const 
				{ return origin + size - vec2(contentOffset.right, contentOffset.bottom); }

			rect rectangle() const
				{ return rect(origin, size); }

			ImageDescriptor operator * (float t) const
			{
				ImageDescriptor result(*this);
				result.origin *= t;
				result.size *= t;
				result.contentOffset *= t;
				return result;
			}

			ImageDescriptor operator + (const ImageDescriptor& r) const
			{
				ImageDescriptor result(*this);
				result.origin += r.origin;
				result.size += r.size;
				result.contentOffset += r.contentOffset;
				return result;
			}

		};

		struct Image
		{
			Texture::Pointer texture;
			ImageDescriptor descriptor;

			Image()
				{ }

			Image(const Texture::Pointer& t) :
				texture(t), descriptor(ImageDescriptor(t)) { }

			Image(const Texture::Pointer& t, const ImageDescriptor& d) :
				texture(t), descriptor(d) { }
		};

		struct SceneVertex
		{
		public:
			SceneVertex() 
				{ }

			SceneVertex(const vec2& pos, const vec4& tc, const vec4& c = vec4(1.0f)) : 
				position(pos, 0.0f), texCoord(tc), color(c) { }

			SceneVertex(const vec3& pos, const vec4& tc, const vec4& c = vec4(1.0f)) : 
				position(pos), texCoord(tc), color(c) { }

		public:
			vec4 texCoord = vec4(0.0f);;
			vec4 color = vec4(0.0f);
			vec3 position = vec3(0.0f);
		};

		struct ElementDragInfo
		{
			vec2 currentPosition = vec2(0.0f);
			vec2 initialPosition = vec2(0.0f);
			vec2 normalizedPointerPosition = vec2(0.0f);
			
			ElementDragInfo()
				{ }
			
			ElementDragInfo(const vec2& c, const vec2& i, const vec2& npp) : 
				currentPosition(c), initialPosition(i), normalizedPointerPosition(npp) { }
		};

		enum Action : size_t
		{
			Action_None = 0,
			Action_Confirm,
			Action_Cancel,
			Action_max
		};
		
		struct Message
		{
			enum Type : size_t
			{
				Type_None,
				Type_TextInput,
				Type_TextFieldControl,
				Type_SetText,
				Type_UpdateText,
				Type_PerformAction,
				Type_SetFontSmoothing
			};
			
			size_t type = 0;
			
			union
			{
				size_t param = 0;
				float paramf;
			};
			
			std::string text;
			float duration = 0.0f;

			explicit Message(size_t aType) :
				type(aType) { }
			
			Message(size_t aType, size_t aParam) :
				type(aType), param(aParam) { }
			
			Message(size_t aType, const std::string& aText) :
				type(aType), param(0), text(aText) { }
		};

		struct ElementLayout
		{
			vec2 position = vec2(0.0f);
			vec2 size = vec2(0.0f);
			vec2 scale = vec2(1.0f);
			vec2 pivotPoint = vec2(0.0f);
			float angle = 0.0f;
			
			size_t layoutMask = LayoutMask_All;
			LayoutMode layoutPositionMode = LayoutMode_Absolute;
			LayoutMode layoutSizeMode = LayoutMode_Absolute;

			ElementLayout() : scale(1.0f), angle(0.0f), layoutMask(LayoutMask_All),
				layoutPositionMode(LayoutMode_Absolute), layoutSizeMode(LayoutMode_Absolute) { }

			ElementLayout(const rect& frame) :
				position(frame.origin()), size(frame.size()), scale(1.0f), angle(0.0f),
				layoutMask(LayoutMask_All), layoutPositionMode(LayoutMode_Absolute),
				layoutSizeMode(LayoutMode_Absolute) { }
			
			ElementLayout(const vec2& pos, const vec2& sz, LayoutMode pMode,
				LayoutMode sMode) : position(pos), size(sz), scale(1.0f), angle(0.0f),
				layoutMask(LayoutMask_All), layoutPositionMode(pMode), layoutSizeMode(sMode) { }
		};

		struct SceneProgram
		{
			Program::Pointer program;
			Program::Uniform additionalOffsetAndAlpha;
			
			bool valid() const
				{ return program.valid(); }
			
			bool invalid() const
				{ return program.invalid(); }
		};

		struct LocalizedText
		{
			std::string key;
			std::string cachedText;
			
			void setKey(const std::string& aKey)
				{ key = aKey; updateCache(); }
			
			void updateCache()
				{ cachedText = localized(key); }
			
			void clear()
				{ key.clear(); cachedText.clear(); }
		};
		
		typedef DataStorage<SceneVertex> SceneVertexList;
	}
}
