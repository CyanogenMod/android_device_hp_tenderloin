ifeq ($(TARGET_BOOTLOADER_BOARD_NAME),topaz)
include $(call first-makefiles-under,$(call my-dir))
endif

