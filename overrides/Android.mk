#
# Copyright 2018 The Maru OS Project
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

include $(CLEAR_VARS)

LOCAL_MODULE := LineagePackageRemovals
LOCAL_MODULE_TAGS := optional

# LOCAL_OVERRIDES_PACKAGES does not work with phony packages so we explicitly
# set this make variable.
PACKAGES.$(LOCAL_MODULE).OVERRIDES := \
	LineageSetupWizard \
	AudioFX \
	Recorder \
	Trebuchet \
	Updater

include $(BUILD_PHONY_PACKAGE)
