#
# Copyright (C) 2011 The Cyanogenmod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# To use this bootimg 
#  
#  Add to your BoardConfig.mk:
#    BOARD_CUSTOM_BOOTIMG_MK := device/common/uboot-bootimg.mk
#  If using uboot multiimage add:
#    BOARD_USES_UBOOT_MULTIIMAGE := true
# 

#
# Ramdisk/boot image
#
LOCAL_PATH := $(call my-dir)

PRODUCT_VERSION = CM-$(PRODUCT_VERSION_MAJOR).$(PRODUCT_VERSION_MINOR).$(PRODUCT_VERSION_MAINTENANCE)$(PRODUCT_VERSION_DEVICE_SPECIFIC)
INTERNAL_UBOOT_IMAGENAME := $(PRODUCT_VERSION) $(TARGET_DEVICE) Ramdisk

INTERNAL_URAMDISKIMAGE_ARGS := -A ARM -O Linux -T RAMDisk -C none -n "$(INTERNAL_UBOOT_IMAGENAME)" -d $(BUILT_RAMDISK_TARGET)
ZIP_SAVE_UBOOTIMG_ARGS := $(INTERNAL_URAMDISKIMAGE_ARGS)
BUILT_UBOOT_RAMDISK_TARGET := $(BUILT_RAMDISK_TARGET:%.img=%.ub)

$(BUILT_UBOOT_RAMDISK_TARGET): $(INSTALLED_RAMDISK_TARGET) $(MKIMAGE)
	$(MKIMAGE) $(INTERNAL_URAMDISKIMAGE_ARGS) $@
	@echo ----- Made uboot ramdisk -------- $@

INSTALLED_RAMDISK_TARGET := $(BUILT_UBOOT_RAMDISK_TARGET)

ifeq ($(BOARD_USES_UBOOT_MULTIIMAGE),true)
    ifneq ($(strip $(TARGET_NO_KERNEL)),true)

        INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img

        INTERNAL_UBOOT_MULTIIMAGENAME := $(PRODUCT_VERSION)  $(TARGET_DEVICE) Multiboot

        INTERNAL_UMULTIIMAGE_ARGS := -A ARM -O Linux -T multi -C none -n "$(INTERNAL_UBOOT_MULTIIMAGENAME)"

        BOARD_UBOOT_ENTRY := $(strip $(BOARD_UBOOT_ENTRY))
        ifdef BOARD_UBOOT_ENTRY
            INTERNAL_UMULTIIMAGE_ARGS += -e $(BOARD_UBOOT_ENTRY)
        endif

        BOARD_UBOOT_LOAD := $(strip $(BOARD_UBOOT_LOAD))
        ifdef BOARD_UBOOT_LOAD
            INTERNAL_UMULTIIMAGE_ARGS += -a $(BOARD_UBOOT_LOAD)
        endif

        INTERNAL_UMULTIIMAGE_ARGS += -d $(INSTALLED_KERNEL_TARGET):$(BUILT_UBOOT_RAMDISK_TARGET)
        ZIP_SAVE_UBOOTIMG_ARGS := $(INTERNAL_UMULTIIMAGE_ARGS)
$(INSTALLED_BOOTIMAGE_TARGET): $(MKIMAGE) $(INTERNAL_RAMDISK_FILES) $(BUILT_UBOOT_RAMDISK_TARGET) $(INSTALLED_KERNEL_TARGET)
			$(MKIMAGE) $(INTERNAL_UMULTIIMAGE_ARGS) $@
			@echo ----- Made uboot multiimage -------- $@

    endif #!TARGET_NO_KERNEL
else # Seperate uboot images kernel/ramdisk
    # HACK: Redefine the bootimage target to just build the ramdisk
$(INSTALLED_BOOTIMAGE_TARGET): $(BUILT_UBOOT_RAMDISK_TARGET)
endif

#
# Recovery Image
#
INSTALLED_RECOVERYIMAGE_TARGET := $(PRODUCT_OUT)/recovery.img
recovery_ramdisk := $(PRODUCT_OUT)/ramdisk-recovery.img
INTERNAL_RECOVERYRAMDISK_IMAGENAME := CWM $(TARGET_DEVICE) Ramdisk
INTERNAL_RECOVERYRAMDISKIMAGE_ARGS := -A ARM -O Linux -T RAMDisk -C none -n "$(INTERNAL_RECOVERYRAMDISK_IMAGENAME)" -d $(recovery_ramdisk)
recovery_uboot_ramdisk := $(recovery_ramdisk:%.img=%.ub)

$(recovery_uboot_ramdisk): $(MKIMAGE) $(recovery_ramdisk)
	@echo ----- Making recovery image ------
	$(MKIMAGE) $(INTERNAL_RECOVERYRAMDISKIMAGE_ARGS) $@
	@echo ----- Made recovery uboot ramdisk -------- $@

ifeq ($(BOARD_USES_UBOOT_MULTIIMAGE),true)
    $(warning We are here.)
    INTERNAL_RECOVERYIMAGE_IMAGENAME := CWM $(TARGET_DEVICE) Multiboot
    INTERNAL_RECOVERYIMAGE_ARGS := -A arm -T multi -C none -n "$(INTERNAL_RECOVERYIMAGE_IMAGENAME)"

    BOARD_UBOOT_ENTRY := $(strip $(BOARD_UBOOT_ENTRY))
    ifdef BOARD_UBOOT_ENTRY
        INTERNAL_RECOVERYIMAGE_ARGS += -e $(BOARD_UBOOT_ENTRY)
    endif

# XXX somehow even though we don't define BOARD_UBOOT_LOAD, it's still
# detected here and produces empty -a argument that confuses mkimage
#    BOARD_UBOOT_LOAD := $(strip $(BOARD_UBOOT_LOAD))    
#    ifdef BOARD_UBOOT_LOAD
#        INTERNAL_RECOVERYIMAGE_ARGS += -a $(BOARD_UBOOT_LOAD)
#    endif

    recovery_kernel := $(INSTALLED_KERNEL_TARGET) # hard-coded for tenderloin

    INTERNAL_RECOVERYIMAGE_ARGS += -d $(strip $(recovery_kernel)):$(strip $(recovery_uboot_ramdisk))

$(INSTALLED_RECOVERYIMAGE_TARGET): $(MKIMAGE) $(recovery_uboot_ramdisk) $(recovery_kernel)
	$(MKIMAGE) $(INTERNAL_RECOVERYIMAGE_ARGS) $@
	@echo ----- Made recovery uboot multiimage -------- $@

else #!BOARD_USES_UBOOT_MULTIIMAGE
    # If we are not on a multiimage platform lets zip the kernel with the ramdisk
    # for Rom Manager
$(INSTALLED_RECOVERYIMAGE_TARGET): $(recovery_uboot_ramdisk) $(recovery_kernel)
	$(hide) rm -f $@
	zip -qDj $@ $(recovery_uboot_ramdisk) $(recovery_kernel)
	@echo ----- Made recovery image \(zip\) -------- $@

endif
