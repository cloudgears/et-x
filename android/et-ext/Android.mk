LOCAL_PATH := $(call my-dir)

INCLUDE_PATH := ../../include/
SOURCE_PATH := ../../src/
LIB_PATH := ../../lib/android/

include $(CLEAR_VARS)
LOCAL_MODULE    := jansson
LOCAL_SRC_FILES := $(LIB_PATH)/libjansson.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := et-ext

LOCAL_C_INCLUDES := $(ENGINE_INCLUDES_PATH) $(LOCAL_PATH)/$(INCLUDE_PATH)

LOCAL_CPPFLAGS += --std=c++11 -Wno-extern-c-compat -DDEBUG -UNDEBUG
LOCAL_CFLAGS += -Wno-extern-c-compat -DDEBUG -UNDEBUG

LOCAL_SRC_FILES = $(SOURCE_PATH)json/json.cpp \
	$(SOURCE_PATH)scene2d/button.cpp \
	$(SOURCE_PATH)scene2d/element.cpp \
	$(SOURCE_PATH)scene2d/element2d.cpp \
	$(SOURCE_PATH)scene2d/font.cpp \
	$(SOURCE_PATH)scene2d/fullscreenelement.cpp \
	$(SOURCE_PATH)scene2d/imageview.cpp \
	$(SOURCE_PATH)scene2d/label.cpp \
	$(SOURCE_PATH)scene2d/layout.cpp \
	$(SOURCE_PATH)scene2d/line.cpp \
	$(SOURCE_PATH)scene2d/listbox.cpp \
	$(SOURCE_PATH)scene2d/messageview.cpp \
	$(SOURCE_PATH)scene2d/renderingelement.cpp \
	$(SOURCE_PATH)scene2d/scene.cpp \
	$(SOURCE_PATH)scene2d/scenerenderer.cpp \
	$(SOURCE_PATH)scene2d/scroll.cpp \
	$(SOURCE_PATH)scene2d/slider.cpp \
	$(SOURCE_PATH)scene2d/table.cpp \
	$(SOURCE_PATH)scene2d/textfield.cpp \
	$(SOURCE_PATH)scene2d/textureatlas.cpp \
	$(SOURCE_PATH)scene2d/textureatlaswriter.cpp \
	$(SOURCE_PATH)scene2d/vertexbuilder.cpp \
	$(SOURCE_PATH)platform-android/charactergenerator.android.cpp \
	$(SOURCE_PATH)/atmosphere/atmosphere.cpp
	
LOCAL_STATIC_LIBRARIES := et jansson

include $(BUILD_STATIC_LIBRARY)
