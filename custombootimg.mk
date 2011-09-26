LOCAL_PATH := $(call my-dir)

INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
$(INSTALLED_BOOTIMAGE_TARGET): $(TARGET_PREBUILT_KERNEL) $(recovery_ramdisk) $(INSTALLED_RAMDISK_TARGET) $(PRODUCT_OUT)/utilities/flash_image $(PRODUCT_OUT)/utilities/busybox $(PRODUCT_OUT)/utilities/erase_image $(MKBOOTIMG) $(MINIGZIP) $(INTERNAL_BOOTIMAGE_FILES)
	$(call pretty,"Boot image: $@")
	##FIXME# uMulti needs to be passed to the OTA package
	$(hide) touch $(PRODUCT_OUT)/boot.img
	$(hide) mkdir -p $(PRODUCT_OUT)/uMulti/ramdisk/
	$(hide) cp -r $(PRODUCT_OUT)/root/* $(PRODUCT_OUT)/uMulti/ramdisk/
	$(hide) find $(PRODUCT_OUT)/uMulti/ramdisk/ | cpio -R 0:0 -H newc -o --quiet | gzip -9c > $(PRODUCT_OUT)/uMulti/andr-init.img
	$(hide) $(MKIMAGE) -A ARM -T RAMDisk -C none -n Image -d $(PRODUCT_OUT)/uMulti/andr-init.img $(PRODUCT_OUT)/uMulti/uRamdisk
	$(hide) $(MKIMAGE) -A arm -T multi -C none -n 'test-multi-image' -d $(TARGET_PREBUILT_KERNEL):$(PRODUCT_OUT)/uMulti/uRamdisk $(PRODUCT_OUT)/cm-uMulti

INSTALLED_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/recovery.img
$(INSTALLED_RECOVERYIMAGE_TARGET): $(MKBOOTIMG) \
	$(recovery_ramdisk) \
	$(recovery_kernel)
	@echo ----- Making recovery image ------
	$(hide) mkdir -p $(PRODUCT_OUT)/recovery-uMulti/ramdisk/
	$(hide) cp -r $(PRODUCT_OUT)/recovery/root/* $(PRODUCT_OUT)/recovery-uMulti/ramdisk/
	$(hide) find $(PRODUCT_OUT)/recovery-uMulti/ramdisk/ | cpio -R 0:0 -H newc -o --quiet | gzip -9c > $(PRODUCT_OUT)/recovery-uMulti/andr-init.img
	$(hide) $(MKIMAGE) -A ARM -T RAMDisk -C none -n Image -d $(PRODUCT_OUT)/recovery-uMulti/andr-init.img $(PRODUCT_OUT)/recovery-uMulti/uRamdisk
	$(hide) $(MKIMAGE) -A arm -T multi -C none -n 'test-multi-image' -d $(TARGET_PREBUILT_KERNEL):$(PRODUCT_OUT)/recovery-uMulti/uRamdisk $(PRODUCT_OUT)/recovery-uMulti
	@echo ----- Made recovery image -------- $@
	$(hide) $(call assert-max-image-size,$@,$(BOARD_RECOVERYIMAGE_PARTITION_SIZE),raw)
