LOCAL_PATH := $(call my-dir)

# LXC components like lxc-start and lxc-monitord
# search for liblxc.so.1
# TODO make em look for liblxc.so
include $(CLEAR_VARS)
LOCAL_MODULE := liblxc
LOCAL_SRC_FILES := lib/liblxc.so.1.0.7
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so.1
include $(BUILD_PREBUILT)

# But perspectived needs liblxc.so to be linked
# with the AOSP build system (doesn't like .so.1)
include $(CLEAR_VARS)
LOCAL_MODULE := liblxc-1
LOCAL_SRC_FILES := lib/liblxc.so.1.0.7
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_SUFFIX := .so
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := lxc-start
LOCAL_SRC_FILES := bin/lxc-start
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := lxc-stop
LOCAL_SRC_FILES := bin/lxc-stop
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := lxc-info
LOCAL_SRC_FILES := bin/lxc-info
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := lxc-console
LOCAL_SRC_FILES := bin/lxc-console
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
