# Copyright (C) 2010 Ricardo Cerqueira
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),tenderloin)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := serial.c

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_STATIC_LIBRARIES := libcutils libc

LOCAL_MODULE := serial
LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)

endif
