# inherit from the proprietary version
-include vendor/hp/tenderloin/BoardConfigVendor.mk

TARGET_SPECIFIC_HEADER_PATH := device/hp/tenderloin/include

# We have so much memory 3:1 split is detrimental to us.
TARGET_USES_2G_VM_SPLIT := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := false

TARGET_BOOTLOADER_BOARD_NAME := tenderloin
TARGET_BOARD_PLATFORM := msm8660
TARGET_BOARD_PLATFORM_GPU := qcom-adreno200
BOARD_USES_ADRENO_200 := true

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true


TARGET_DISABLE_ARM_PIE := true
TARGET_NO_RADIOIMAGE := true
TARGET_HAVE_TSLIB := false
TARGET_GLOBAL_CFLAGS += -mfpu=neon -mfloat-abi=softfp
TARGET_GLOBAL_CPPFLAGS += -mfpu=neon -mfloat-abi=softfp
TARGET_USE_SCORPION_BIONIC_OPTIMIZATION := true
TARGET_USE_SCORPION_PLD_SET := true
TARGET_SCORPION_BIONIC_PLDOFFS := 6
TARGET_SCORPION_BIONIC_PLDSIZE := 128

COMMON_GLOBAL_CFLAGS += -DREFRESH_RATE=59 -DQCOM_HARDWARE -DUSES_AUDIO_LEGACY

# Boot animation
TARGET_SCREEN_HEIGHT := 768
TARGET_SCREEN_WIDTH := 1024

# Wifi related defines
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_ath6kl
WPA_SUPPLICANT_VERSION      := VER_0_8_X
BOARD_WLAN_DEVICE           := ath6kl
WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/ath6kl.ko"
WIFI_DRIVER_MODULE_NAME     := "ath6kl"

# Audio
BOARD_USES_AUDIO_LEGACY := true
BOARD_USES_GENERIC_AUDIO := false
TARGET_PROVIDES_LIBAUDIO := false
BOARD_USES_ALSA_AUDIO := false
BOARD_WITH_ALSA_UTILS := false

#Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_CSR := true

# Define egl.cfg location
BOARD_EGL_CFG := device/hp/tenderloin/egl.cfg
USE_OPENGL_RENDERER := true

# QCOM HAL
BOARD_USES_QCOM_HARDWARE := true
TARGET_NO_HW_VSYNC := true
TARGET_USES_OVERLAY := false
TARGET_HAVE_BYPASS  := false
TARGET_USES_SF_BYPASS := false
TARGET_USES_C2D_COMPOSITION := true
TARGET_USES_GENLOCK := true
BOARD_ADRENO_DECIDE_TEXTURE_TARGET := true

# Webkit workaround
TARGET_FORCE_CPU_UPLOAD := true

# Enable WEBGL in WebKit
ENABLE_WEBGL := true

BOARD_USES_QCOM_LIBS := true
BOARD_USES_QCOM_LIBRPC := true
BOARD_USE_QCOM_PMEM := true
BOARD_CAMERA_USE_GETBUFFERINFO := true
BOARD_FIRST_CAMERA_FRONT_FACING := true
BOARD_CAMERA_USE_ENCODEDATA := true
BOARD_NEEDS_MEMORYHEAPPMEM := true

BOARD_OVERLAY_FORMAT_YCbCr_420_SP := true
USE_CAMERA_STUB := false

# tenderloin- these kernel settings are temporary to complete build
BOARD_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.hardware=qcom
BOARD_KERNEL_BASE := 0x40200000
BOARD_PAGE_SIZE := 2048

BOARD_NEEDS_CUTILS_LOG := true

TARGET_PROVIDES_RELEASETOOLS := true
TARGET_RELEASETOOL_IMG_FROM_TARGET_SCRIPT := device/hp/tenderloin/releasetools/tenderloin_img_from_target_files
TARGET_RELEASETOOL_OTA_FROM_TARGET_SCRIPT := device/hp/tenderloin/releasetools/tenderloin_ota_from_target_files

BOARD_USES_UBOOT := true
BOARD_USES_UBOOT_MULTIIMAGE := true

# use dosfsck from dosfstools
BOARD_USES_CUSTOM_FSCK_MSDOS := true

# kernel has no ext4_lazyinit
# (esp. important for make_ext4fs in recovery)
BOARD_NO_EXT4_LAZYINIT := true

# Define kernel config for inline building
TARGET_KERNEL_CONFIG := tenderloin_android_defconfig

# Define Prebuilt kernel locations
TARGET_PREBUILT_KERNEL := device/hp/tenderloin/prebuilt/boot/kernel
BOARD_CUSTOM_RECOVERY_KEYMAPPING := ../../device/hp/tenderloin/recovery/recovery_ui.c
TARGET_RECOVERY_INITRC := device/hp/tenderloin/recovery/init.rc
BOARD_HAS_NO_SELECT_BUTTON := false

# tenderloin - these partition sizes are temporary to complete build
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16776192
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 838860800
BOARD_USERDATAIMAGE_PARTITION_SIZE := 20044333056
BOARD_FLASH_BLOCK_SIZE := 131072

TARGET_RELEASETOOLS_EXTENSIONS := device/hp/common

BOARD_HAS_SDCARD_INTERNAL := false
BOARD_USES_MMCUTILS := true
BOARD_HAS_NO_MISC_PARTITION := true
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_CUSTOM_GRAPHICS:= ../../../device/hp/tenderloin/graphics.c
BOARD_CUSTOM_BOOTIMG_MK := device/hp/tenderloin/uboot-bootimg.mk

# Multiboot stuff
TARGET_RECOVERY_PRE_COMMAND := "/system/bin/rebootcmd"

ADDITIONAL_DEFAULT_PROPERTIES += ro.secure=0
