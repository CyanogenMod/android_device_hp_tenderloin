LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#
## reboot command
#
#
LOCAL_SRC_FILES:= \
	rebootcmd.c
LOCAL_CFLAGS:= -g -c -W -Wall -O2 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp -funsafe-math-optimizations -D_POSIX_SOURCE
LOCAL_MODULE:=rebootcmd
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog
LOCAL_MODULE_TAGS:= eng
include $(BUILD_EXECUTABLE)
