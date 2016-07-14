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
BOARD_SEPOLICY_DIRS += \
       vendor/maruos/sepolicy

BOARD_SEPOLICY_UNION += \
       service.te \
       surfaceflinger.te \
       perspectived.te \
       mflinger.te \
       mcprepare.te \
       maru_init.te \
       maru_files.te \
       file_contexts \
       service_contexts
