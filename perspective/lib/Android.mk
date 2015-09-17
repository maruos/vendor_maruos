LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libperspective

LOCAL_SRC_FILES := IPerspectiveService.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbinder \
    libutils

include $(BUILD_SHARED_LIBRARY)
