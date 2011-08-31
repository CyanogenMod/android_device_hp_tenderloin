ifneq ($(filter topaz,$(TARGET_DEVICE)),)
    include $(all-subdir-makefiles)
endif
