#
# Copyright 2015-2016 Preetam J. D'Souza
# Copyright 2016 The Maru OS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

# -----------------------------------------------------------------------------
# perspectived

include $(CLEAR_VARS)

LOCAL_MODULE := perspectived

LOCAL_SRC_FILES := \
	PerspectiveService.cpp \
	LXCContainerManager.cpp

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../include \
	external/lxc/src

LOCAL_CFLAGS := -DLOG_TAG=\"perspectived\"

LOCAL_SHARED_LIBRARIES := \
    libhardware_legacy \
    liblog \
    libbinder \
    libutils \
    libperspective \
    liblxc

include $(BUILD_EXECUTABLE)

# -----------------------------------------------------------------------------
# libperspective

include $(CLEAR_VARS)

LOCAL_MODULE := libperspective

LOCAL_SRC_FILES := lib/IPerspectiveService.cpp

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libbinder \
    libutils

include $(BUILD_SHARED_LIBRARY)
