ifneq ($(filter tenderloin,$(TARGET_DEVICE)),)
    include $(all-subdir-makefiles)
endif
