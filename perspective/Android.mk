LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := perspectived

LOCAL_SRC_FILES := PerspectiveService.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include

LOCAL_CFLAGS := -DLOG_TAG=\"perspectived\"

LOCAL_SHARED_LIBRARIES := \
    libhardware_legacy \
    liblog \
    libbinder \
    libutils \
    libperspective \
    liblxc-1

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under, $(LOCAL_PATH))
