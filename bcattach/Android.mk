LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#
## TP Application
#
#
#LOCAL_C_INCLUDES:= uim.h

LOCAL_SRC_FILES:= \
	main.c
LOCAL_CFLAGS:= -g -c -W -Wall -D_POSIX_SOURCE -I../include
LOCAL_MODULE:=bcattach
LOCAL_MODULE_TAGS:= optional

LOCAL_SHARED_LIBRARIES := \
	libc

include $(BUILD_EXECUTABLE)

