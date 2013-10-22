LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := jansson
LOCAL_SRC_FILES := dump.c error.c hashtable.c load.c memory.c pack_unpack.c strbuffer.c strconv.c utf.c value.c
include $(BUILD_STATIC_LIBRARY)
