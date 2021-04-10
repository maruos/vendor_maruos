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

# TARGET_DESKTOP_ROOTFS can be set in vendor makefiles to override the default
# desktop rootfs image for Maru. Note that the path must be relative to the
# AOSP workspace root directory, which can easily be done by prefixing the path
# with $(LOCAL_PATH).
ifeq ($(TARGET_DESKTOP_ROOTFS),)
  TARGET_DESKTOP_ROOTFS := $(LOCAL_PATH)/desktop-rootfs.tar.gz
endif

include $(CLEAR_VARS)
LOCAL_MODULE := rootfs.tar.gz
LOCAL_MODULE_TAGS  := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/maru/containers/default

# Instead of using LOCAL_SRC_FILES and $(BUILD_PREBUILT), we explicitly set up
# the rule ourselves so that we can copy a rootfs path from outside of this
# directory if a vendor overrides TARGET_DESKTOP_ROOTFS.
include $(BUILD_SYSTEM)/base_rules.mk
$(LOCAL_BUILT_MODULE): $(TARGET_DESKTOP_ROOTFS)
	@mkdir -p $(dir $@)
	@cp $(TARGET_DESKTOP_ROOTFS) $@

include $(call all-makefiles-under, $(LOCAL_PATH))
