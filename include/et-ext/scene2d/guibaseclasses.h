#pragma once

#include <et/apiobjects/texture.h>
#include <et-ext/scene2d/guibaseconst.h>

namespace et
{
	namespace s2d
	{
		class SceneRenderer;

		enum ElementRepresentation
		{
			ElementRepresentation_2d,
			ElementRepresentation_3d,
			ElementRepresentation_max
		};

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
			float left;
			float top;
			float right;
			float bottom;

		public:
			ContentOffset(float value = 0.0f) :
				left(value), top(value), right(value), bottom(value) { }

			ContentOffset(const vec2& values) :
				left(values.x), top(values.y), right(values.x), bottom(values.y) { }

			ContentOffset(float l, float t, float r, float b) :	
				left(l), top(t), right(r), bottom(b) { }

			const vec2 origin() const
				{ return vec2(left, top); }
		};

		struct ImageDescriptor
		{
			vec2 origin;
			vec2 size;
			ContentOffset contentOffset;

			ImageDescriptor() : 
				origin(0.0f), size(0.0f) { } 

			ImageDescriptor(const Texture& tex) :
				origin(0.0f), size(tex.valid() ? tex->sizeFloat() : vec2(0.0f)) { }

			ImageDescriptor(const Texture& tex, const ContentOffset& offset) : 
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
		};

		struct Image
		{
			Texture texture;
			ImageDescriptor descriptor;

			Image()
				{ }

			Image(const Texture& t) :
				texture(t), descriptor(ImageDescriptor(t)) { }

			Image(const Texture& t, const ImageDescriptor& d) :
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
			vec3 position;
			vec4 texCoord;
			vec4 color; 
		};

		struct ElementDragInfo
		{
			vec2 currentPosition;
			vec2 initialPosition;
			vec2 normalizedPointerPosition;
			
			ElementDragInfo(const vec2& c, const vec2& i, const vec2& npp) : 
				currentPosition(c), initialPosition(i), normalizedPointerPosition(npp) { }
		};

		struct GuiMessage
		{
			enum Type
			{
				Type_None,
				Type_TextInput,
				Type_TextFieldControl,
			};

			size_t type;
			size_t param;
			std::string text;

			GuiMessage(size_t aType, size_t aParam) :
				type(aType), param(aParam) { }
			
			GuiMessage(size_t aType, const std::string& aText) :
				type(aType), param(0), text(aText) { }
		};

		struct ElementLayout
		{
			vec2 position;
			vec2 size;
			vec2 scale;
			vec2 pivotPoint;
			float angle;
			size_t mask;

			LayoutMode positionMode;
			LayoutMode sizeMode;

			ElementLayout() : scale(1.0f), angle(0.0f), mask(LayoutMask_All),
				positionMode(LayoutMode_Absolute), sizeMode(LayoutMode_Absolute) { }

			ElementLayout(const vec2& pos, const vec2& sz, LayoutMode pMode,
				LayoutMode sMode) : position(pos), size(sz), scale(1.0f), angle(0.0f),
				mask(LayoutMask_All), positionMode(pMode), sizeMode(sMode) { }
		};

		struct SceneProgram
		{
			Program::Pointer program;
			ProgramUniform additionalOffsetAndAlpha;
			
			bool valid() const
			{ return program.valid(); }
			
			bool invalid() const
			{ return program.invalid(); }
		};

		typedef DataStorage<SceneVertex> SceneVertexList;
		typedef std::vector<Image> ImageList;
		typedef std::map<std::string, Image> ImageMap;
	}
}
