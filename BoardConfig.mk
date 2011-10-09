# inherit from the proprietary version
-include vendor/hp/tenderloin/BoardConfigVendor.mk

TARGET_SPECIFIC_HEADER_PATH := device/hp/tenderloin/include 

# We have so much memory 3:1 split is detrimental to us.
TARGET_USES_2G_VM_SPLIT := true

TARGET_NO_BOOTLOADER := true
TARGET_NO_KERNEL := false

TARGET_BOARD_PLATFORM := msm8660
TARGET_BOARD_PLATFORM_GPU := qcom-adreno200

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true

TARGET_BOOTLOADER_BOARD_NAME := tenderloin
TARGET_HAVE_HDMI_OUT := false
TARGET_USES_OVERLAY := true
TARGET_NO_RADIOIMAGE := true
TARGET_HAVE_TSLIB := false
TARGET_GLOBAL_CFLAGS += -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp 
TARGET_GLOBAL_CPPFLAGS += -mtune=cortex-a9 -mfpu=neon -mfloat-abi=softfp 


# Wifi related defines
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
WPA_SUPPLICANT_VERSION      := VER_0_6_X
BOARD_WLAN_DEVICE           := ar6000
WIFI_DRIVER_MODULE_PATH     := "/system/lib/modules/ar6000.ko"
WIFI_DRIVER_MODULE_NAME     := "ar6000"
BOARD_WEXT_NO_COMBO_SCAN	:= true

# Audio 
BOARD_USES_GENERIC_AUDIO := false
TARGET_PROVIDES_LIBAUDIO := true
BOARD_USES_ALSA_AUDIO := true
BOARD_WITH_ALSA_UTILS := true

#Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_CSR := true
#BOARD_HAVE_BLUETOOTH_BCM := true

# Define egl.cfg location
BOARD_EGL_CFG := device/hp/tenderloin/egl.cfg

BOARD_USES_QCOM_LIBS := true
BOARD_USES_QCOM_LIBRPC := true
#BOARD_USES_QCOM_GPS := true
BOARD_USE_QCOM_PMEM := true
BOARD_CAMERA_USE_GETBUFFERINFO := true
BOARD_USE_FROYO_LIBCAMERA := true
BOARD_USE_LARGE_CAMHAL := true
BOARD_FIRST_CAMERA_FRONT_FACING := true

BOARD_OVERLAY_FORMAT_YCbCr_420_SP := true
BOARD_USES_ADRENO_200 := true
#BOARD_NO_RGBX_8888 := true

#BOARD_HAVE_FM_RADIO := true
#BOARD_GLOBAL_CFLAGS += -DHAVE_FM_RADIO

TARGET_BOOTLOADER_BOARD_NAME := tenderloin

#BOARD_USE_NEW_LIBRIL_HTC := true
#TARGET_PROVIDES_LIBRIL := vendor/hp/tenderloin/proprietary/libril.so

# tenderloin- these kernel settings are temporary to complete build
BOARD_KERNEL_CMDLINE := console=ttyHSL0,115200,n8 androidboot.hardware=qcom
BOARD_KERNEL_BASE := 0x40200000
BOARD_PAGE_SIZE := 2048

TARGET_USE_SCORPION_BIONIC_OPTIMIZATION := true

#BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE := tenderloin
#BOARD_VENDOR_QCOM_GPS_LOC_API_AMSS_VERSION := 50000

BOARD_HAVE_FAKE_GPS := true

TARGET_USES_C2D_COMPOSITION := true
TARGET_HAVE_HDMI_OUT := false
#TARGET_HARDWARE_3D := true

TARGET_RELEASETOOL_IMG_FROM_TARGET_SCRIPT := device/hp/tenderloin/releasetools/tenderloin_img_from_target_files
TARGET_RELEASETOOL_OTA_FROM_TARGET_SCRIPT := device/hp/tenderloin/releasetools/tenderloin_ota_from_target_files

BOARD_USES_UBOOT := true
BOARD_USES_UBOOT_MULTIIMAGE := true

# Define Prebuilt kernel locations
TARGET_PREBUILT_KERNEL := device/hp/tenderloin/prebuilt/boot/kernel

# clockworkmod
BOARD_CUSTOM_RECOVERY_KEYMAPPING:= ../../device/hp/tenderloin/recovery_ui.c
TARGET_RECOVERY_INITRC := device/hp/tenderloin/init.recovery.rc
BOARD_RECOVERY_IGNORE_BOOTABLES := true

# tenderloin - these partition sizes are temporary to complete build
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 16777216
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 16776192
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 838860800
BOARD_USERDATAIMAGE_PARTITION_SIZE := 20044333056
BOARD_FLASH_BLOCK_SIZE := 131072

TARGET_RELEASETOOLS_EXTENSIONS := device/hp/common

BOARD_HAS_SDCARD_INTERNAL := false
#BOARD_SDCARD_DEVICE_PRIMARY := /dev/store/cm-sdcard
#BOARD_SDCARD_DEVICE_SECONDARY := /dev/store/cm-sdcard2
#BOARD_SDEXT_DEVICE := /dev/store/cm-sd-ext
BOARD_USES_MMCUTILS := true
BOARD_HAS_NO_MISC_PARTITION := true
BOARD_HAS_NO_SELECT_BUTTON := true
BOARD_CUSTOM_GRAPHICS:= ../../../device/hp/tenderloin/graphics.c
BOARD_USES_RECOVERY_CHARGEMODE := true

# Multiboot stuff
TARGET_RECOVERY_PRE_COMMAND := "/system/bin/rebootcmd recovery"
TARGET_ALTOS_PRE_COMMAND := "/system/bin/rebootcmd altos"
