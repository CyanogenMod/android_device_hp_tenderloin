#
# Copyright (C) 2011 The CyanogenMod Project
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

# The gps config appropriate for this device
#PRODUCT_COPY_FILES += \
#    device/hp/topaz/gps.conf:system/etc/gps.conf

## (1) First, the most specific values, i.e. the aspects that are specific to GSM
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=131 \
    ro.com.google.clientidbase=android-hp \
    ro.com.google.locationfeatures=1 \
    ro.com.google.networklocation=1 \
    ro.setupwizard.enable_bypass=1 \
    dalvik.vm.lockprof.threshold=500 \
    dalvik.vm.dexopt-flags=m=y

PRODUCT_COPY_FILES += \
    device/hp/topaz/init.topaz.rc:root/init.topaz.rc \
    device/hp/topaz/init.rc:root/init.rc \
    device/hp/topaz/ueventd.topaz.rc:root/ueventd.topaz.rc

## (2) Also get non-open-source GSM-specific aspects if available
$(call inherit-product-if-exists, vendor/hp/topaz/topaz-vendor.mk)

## (3)  Finally, the least specific parts, i.e. the non-GSM-specific aspects

DEVICE_PACKAGE_OVERLAYS += device/hp/topaz/overlay

PRODUCT_COPY_FILES += \
    frameworks/base/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/base/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
    frameworks/base/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/base/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/base/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/base/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distinct.xml \
    frameworks/base/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml

#PRODUCT_PACKAGES += \
#    gps.topaz \
#    librs_jni \
#    gralloc.msm8660 \
#    copybit.msm8660 \
#    overlay.default \
#    com.android.future.usb.accessory

PRODUCT_PACKAGES += \
    librs_jni \
    libaudio \
    gralloc.msm8660 \
    overlay.default \
    copybit.msm8660 \
    com.android.future.usb.accessory


#    libOmxCore \
#    libOmxVenc \
#    libOmxVdec


#  from encore... just delete

PRODUCT_PACKAGES += \
    libreference-ril

# Keylayouts
#PRODUCT_COPY_FILES += \
#    device/hp/topaz/keychars/qwerty2.kcm.bin:system/usr/keychars/qwerty2.kcm.bin \
#    device/hp/topaz/keychars/qwerty.kcm.bin:system/usr/keychars/qwerty.kcm.bin \
#    device/hp/topaz/keychars/topaz-keypad.kcm.bin:system/usr/keychars/topaz-keypad.kcm.bin \
#    device/hp/topaz/keychars/BT_HID.kcm.bin:system/usr/keychars/BT_HID.kcm.bin \
#    device/hp/topaz/keylayout/h2w_headset.kl:system/usr/keylayout/h2w_headset.kl \
#    device/hp/topaz/keylayout/qwerty.kl:system/usr/keylayout/qwerty.kl \
#    device/hp/topaz/keylayout/topaz-keypad.kl:system/usr/keylayout/topaz-keypad.kl \
#    device/hp/topaz/keylayout/BT_HID.kl:system/usr/keylayout/BT_HID.kl \
#    device/hp/topaz/keylayout/AVRCP.kl:system/usr/keylayout/AVRCP.kl
# Firmware

#PRODUCT_COPY_FILES += \
#    device/hp/topaz/firmware/BCM4329B1_002.002.023.0589.0632.hcd:system/etc/firmware/BCM4329B1_002.002.023.0589.0632.hcd \
#    device/hp/topaz/firmware/fw_bcm4329.bin:system/etc/firmware/fw_bcm4329.bin \
#    device/hp/topaz/firmware/fw_bcm4329_apsta.bin:system/etc/firmware/fw_bcm4329_apsta.bin \
#    device/hp/topaz/firmware/vidc_1080p.fw:system/etc/firmware/vidc_1080p.fw \
#    device/hp/topaz/firmware/leia_pfp_470.fw:system/etc/firmware/leia_pfp_470.fw \
#    device/hp/topaz/firmware/leia_pm4_470.fw:system/etc/firmware/leia_pm4_470.fw
    
# Audio DSP Profiles
#PRODUCT_COPY_FILES += \
#    device/hp/topaz/dsp/AIC3254_REG.csv:system/etc/AIC3254_REG.csv \
#    device/hp/topaz/dsp/AIC3254_REG_DualMic.csv:system/etc/AIC3254_REG_DualMic.csv \
#    device/hp/topaz/dsp/AdieHWCodec.csv:system/etc/AdieHWCodec.csv \
#    device/hp/topaz/dsp/AudioBTID.csv:system/etc/AudioBTID.csv \
#    device/hp/topaz/dsp/CodecDSPID.txt:system/etc/CodecDSPID.txt \
#    device/hp/topaz/dsp/CodecDSPID_WB.txt:system/etc/CodecDSPID_WB.txt \
#    device/hp/topaz/dsp/TPA2051_CFG.csv:system/etc/TPA2051_CFG.csv \
#    device/hp/topaz/dsp/TPA2051_CFG_XC.csv:system/etc/TPA2051_CFG_XC.csv \
#    device/hp/topaz/dsp/soundimage/Sound_Beats.txt:system/etc/soundimage/Sound_Beats.txt \
#    device/hp/topaz/dsp/soundimage/Sound_MFG.txt:system/etc/soundimage/Sound_MFG.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Original_Recording.txt:system/etc/soundimage/Sound_Original_Recording.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Original_SPK.txt:system/etc/soundimage/Sound_Original_SPK.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Original.txt:system/etc/soundimage/Sound_Original.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Phone_Original_HP_WB.txt:system/etc/soundimage/Sound_Phone_Original_HP_WB.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Phone_Original_HP.txt:system/etc/soundimage/Sound_Phone_Original_HP.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Phone_Original_REC_WB.txt:system/etc/soundimage/Sound_Phone_Original_REC_WB.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Phone_Original_REC.txt:system/etc/soundimage/Sound_Phone_Original_REC.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Phone_Original_SPK_WB.txt:system/etc/soundimage/Sound_Phone_Original_SPK_WB.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Phone_Original_SPK.txt:system/etc/soundimage/Sound_Phone_Original_SPK.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Rec_Landscape.txt:system/etc/soundimage/Sound_Rec_Landscape.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Rec_Portrait.txt:system/etc/soundimage/Sound_Rec_Portrait.txt \
#    device/hp/topaz/dsp/soundimage/Sound_Recording.txt:system/etc/soundimage/Sound_Recording.txt \
#    device/hp/topaz/dsp/soundimage/srs_geq10.cfg:system/etc/soundimage/srs_geq10.cfg \
#    device/hp/topaz/dsp/soundimage/srsfx_trumedia_51.cfg:system/etc/soundimage/srsfx_trumedia_51.cfg \
#    device/hp/topaz/dsp/soundimage/srsfx_trumedia_movie.cfg:system/etc/soundimage/srsfx_trumedia_movie.cfg \
#    device/hp/topaz/dsp/soundimage/srsfx_trumedia_music.cfg:system/etc/soundimage/srsfx_trumedia_music.cfg \
#    device/hp/topaz/prebuilt/snd3254:system/bin/snd3254

# Wifi Module
PRODUCT_COPY_FILES += \
    device/hp/topaz/modules/ar6000.ko:system/lib/modules/ar6000.ko

# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

# device uses high-density artwork where available
PRODUCT_LOCALES += hdpi

PRODUCT_COPY_FILES += \
    device/hp/topaz/vold.fstab:system/etc/vold.fstab


$(call inherit-product, $(SRC_TARGET_DIR)/product/languages_full.mk)

# The gps config appropriate for this device
#$(call inherit-product, device/common/gps/gps_eu_supl.mk)

DEVICE_PACKAGE_OVERLAYS += device/hp/topaz/overlay


ifeq ($(TARGET_PREBUILT_KERNEL),)
	LOCAL_KERNEL := device/hp/topaz/kernel
else
	LOCAL_KERNEL := $(TARGET_PREBUILT_KERNEL)
endif

PRODUCT_COPY_FILES += \
    $(LOCAL_KERNEL):kernel

# media profiles and capabilities spec
#$(call inherit-product, device/hp/topaz/media_a1026.mk)

# hp audio settings
#$(call inherit-product, device/hp/topaz/media_hpaudio.mk)

# stuff common to all HTC phones
#$(call inherit-product, device/hp/common/common.mk)

$(call inherit-product, build/target/product/full_base.mk)

PRODUCT_NAME := full_topaz
PRODUCT_DEVICE := topaz
PRODUCT_MODEL := HP Touchpad
PRODUCT_MANUFACTURER := HP
