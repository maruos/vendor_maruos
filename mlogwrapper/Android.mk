#
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
# mlogwrapper - un-selinux-constrained logwrapper for maru

include $(CLEAR_VARS)

LOCAL_MODULE := mlogwrapper

LOCAL_SRC_FILES := mlogwrapper.c

LOCAL_CFLAGS := -DLOG_TAG=\"mlogwrapper\"

LOCAL_SHARED_LIBRARIES := \
    liblog

include $(BUILD_EXECUTABLE)
