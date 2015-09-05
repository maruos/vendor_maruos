LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := perspectived
LOCAL_SRC_FILES := perspectived.c
LOCAL_CFLAGS := -DLOG_TAG=\"perspectived\"
LOCAL_SHARED_LIBRARIES := libhardware_legacy liblog liblxc
include $(BUILD_EXECUTABLE)
