LOCAL_PATH := $(call my-dir)

ifeq ($(SABP_RELEASE),1)
    LIB_PREFIX := release
else
    LIB_PREFIX := debug
endif

include $(CLEAR_VARS)
LOCAL_MODULE := libavutil
LOCAL_SRC_FILES := prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libavutil.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libswresample
LOCAL_SRC_FILES := prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libswresample.a
LOCAL_STATIC_LIBRARIES := libavutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavcodec
LOCAL_SRC_FILES := prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libavcodec.a
LOCAL_STATIC_LIBRARIES := libswresample libavutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := libavformat
LOCAL_SRC_FILES := prebuilt/$(LIB_PREFIX)/$(TARGET_ARCH_ABI)/libavformat.a
LOCAL_STATIC_LIBRARIES := libavcodec libswresample libavutil
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

ifeq ($(SABP_RELEASE),1)
	LOCAL_MODULE := $(RELEASE_LIB_NAME)
else
	LOCAL_MODULE := n0
	LOCAL_LDLIBS += -llog
endif

LOCAL_SRC_FILES := root.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_STATIC_LIBRARIES := libavutil libswresample libavformat libavcodec
LOCAL_LDLIBS += -lz -ldl

LOCAL_ARM_NEON := true

include $(BUILD_SHARED_LIBRARY)
