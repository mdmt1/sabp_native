LOCAL_PATH := $(call my-dir)

ifeq ($(NDK_DEBUG),0)
    LIB_PREFIX = release
else ifeq ($(NDK_DEBUG),1)
    LIB_PREFIX = debug
endif

include $(CLEAR_VARS)
LOCAL_MODULE = libavutil
LOCAL_SRC_FILES = prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libavutil.a
LOCAL_EXPORT_C_INCLUDES = $(LOCAL_PATH)/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE = libswresample
LOCAL_SRC_FILES = prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libswresample.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES = libavutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavcodec
LOCAL_SRC_FILES = prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libavcodec.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES := libswresample libavutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavformat
LOCAL_SRC_FILES = prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libavformat.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_STATIC_LIBRARIES := libavcodec libswresample libavutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

ifeq ($(NDK_DEBUG),1)
    LOCAL_MODULE = n0
    LOCAL_LDLIBS += -llog
else ifeq ($(NDK_DEBUG),0)
    LOCAL_MODULE := $(RELEASE_LIB_NAME)
endif

#LOCAL_SRC_FILES := $(wildcard $(realpath $(LOCAL_PATH))/src/*.c) root.c
LOCAL_SRC_FILES = root.c

LOCAL_STATIC_LIBRARIES = libavutil libswresample libavformat libavcodec
LOCAL_LDLIBS += -lz

include $(BUILD_SHARED_LIBRARY)
