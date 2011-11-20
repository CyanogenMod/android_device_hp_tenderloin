# hardware/libaudio-alsa/Android.mk
#
# Copyright 2008 Wind River Systems
#

ifeq ($(strip $(TARGET_BOOTLOADER_BOARD_NAME)), tenderloin)

  LOCAL_PATH := $(call my-dir)

  include $(CLEAR_VARS)

  LOCAL_ARM_MODE := arm
  LOCAL_CFLAGS := -D_POSIX_SOURCE

    LOCAL_C_INCLUDES += external/alsa-lib/include

  LOCAL_SRC_FILES := \
	AudioHardwareALSA.cpp \
	AudioStreamOutALSA.cpp \
	AudioStreamInALSA.cpp \
	ALSAStreamOps.cpp \
	ALSAMixer.cpp \
	ALSAControl.cpp

  LOCAL_MODULE := libaudio
  LOCAL_MODULE_TAGS:= optional

  LOCAL_STATIC_LIBRARIES += libaudiointerface

  LOCAL_SHARED_LIBRARIES := \
    libasound \
    libcutils \
    libutils \
    libmedia \
    libhardware \
    libhardware_legacy \
    libc

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
  LOCAL_SHARED_LIBRARIES += liba2dp
endif

  include $(BUILD_SHARED_LIBRARY)

# This is the ALSA audio policy manager

  include $(CLEAR_VARS)

  LOCAL_CFLAGS := -D_POSIX_SOURCE

ifeq ($(BOARD_HAVE_BLUETOOTH),true)
    LOCAL_CFLAGS += -DWITH_A2DP
endif

  LOCAL_SRC_FILES := AudioPolicyManagerALSA.cpp

  LOCAL_MODULE := libaudiopolicy
  LOCAL_MODULE_TAGS:= optional

  LOCAL_WHOLE_STATIC_LIBRARIES += libaudiopolicybase

  LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libmedia

  include $(BUILD_SHARED_LIBRARY)

# This is the default ALSA module which behaves closely like the original

  include $(CLEAR_VARS)

  LOCAL_PRELINK_MODULE := false

  LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

  LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar

ifneq ($(ALSA_DEFAULT_SAMPLE_RATE),)
    LOCAL_CFLAGS += -DALSA_DEFAULT_SAMPLE_RATE=$(ALSA_DEFAULT_SAMPLE_RATE)
endif

  LOCAL_C_INCLUDES += external/alsa-lib/include

  LOCAL_SRC_FILES:= alsa_default.cpp

  LOCAL_SHARED_LIBRARIES := \
  	libasound \
  	liblog

  LOCAL_MODULE:= alsa.tenderloin
  LOCAL_MODULE_TAGS:= optional

  include $(BUILD_SHARED_LIBRARY)

# This is the default Acoustics module which is essentially a stub

  include $(CLEAR_VARS)

  LOCAL_PRELINK_MODULE := false

  LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

  LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar

  LOCAL_C_INCLUDES += external/alsa-lib/include

  LOCAL_SRC_FILES:= acoustics_default.cpp

  LOCAL_SHARED_LIBRARIES := liblog

  LOCAL_MODULE:= acoustics.default
  LOCAL_MODULE_TAGS:= optional

  include $(BUILD_SHARED_LIBRARY)

endif
