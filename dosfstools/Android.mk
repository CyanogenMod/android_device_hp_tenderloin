ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/boot.c src/check.c src/common.c \
	src/fat.c src/file.c src/io.c src/lfn.c src/dosfsck.c
LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_SHARED_LIBRARIES := libc
LOCAL_CFLAGS += -D_USING_BIONIC_
LOCAL_CFLAGS += -DUSE_ANDROID_RETVALS
LOCAL_MODULE = dosfsck
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/boot.c src/check.c src/common.c src/fat.c \
	src/file.c src/io.c src/lfn.c src/dosfslabel.c
LOCAL_C_INCLUDES := $(KERNEL_HEADERS) \
	bionic/libc/kernel/common
LOCAL_SHARED_LIBRARIES := libc
LOCAL_CFLAGS += -D_USING_BIONIC_
LOCAL_MODULE = dosfslabel
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := src/mkdosfs.c
LOCAL_C_INCLUDES := $(KERNEL_HEADERS)
LOCAL_SHARED_LIBRARIES := libc
LOCAL_CFLAGS += -D_USING_BIONIC_
LOCAL_MODULE = mkdosfs
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

endif
