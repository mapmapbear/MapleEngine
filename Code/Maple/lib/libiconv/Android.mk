LOCAL_PATH := $(call my-dir)
LOCAL_PATH := $(call my-dir)

#****************************************************************************



#****************************************************************************
# static lib, which will be built statically
#
include $(CLEAR_VARS)

LOCAL_MODULE := libiconv
LOCAL_SRC_FILES := libs/$(TARGET_ARCH_ABI)/libiconv.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_STATIC_LIBRARY)
