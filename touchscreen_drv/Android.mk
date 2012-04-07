LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
#
## TP Application
#
#
#LOCAL_C_INCLUDES:= uim.h

LOCAL_SRC_FILES:= \
	ts_srv.c \
	digitizer.c
LOCAL_CFLAGS:= -g -c -W -Wall -O2 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp -funsafe-math-optimizations -D_POSIX_SOURCE -I/home/green/touchpad/hp_tenderloin_kernel/include
LOCAL_MODULE:=ts_srv
LOCAL_MODULE_TAGS:= eng
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
include $(BUILD_EXECUTABLE)


## ts_srv_set application for changing modes of touchscreen operation
## used to set finger or stylus mode
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	ts_srv_set.c
LOCAL_CFLAGS:= -g -c -W -Wall -O2 -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp -funsafe-math-optimizations -D_POSIX_SOURCE -I/home/green/touchpad/hp_tenderloin_kernel/include
LOCAL_MODULE:=ts_srv_set
LOCAL_MODULE_TAGS:= eng
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog
include $(BUILD_EXECUTABLE)
