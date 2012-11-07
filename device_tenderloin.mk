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

## (1) First, the most specific values, i.e. the aspects that are specific to GSM
PRODUCT_PROPERTY_OVERRIDES += \
    ro.sf.lcd_density=160 \
    ro.com.google.networklocation=1 \
    dalvik.vm.lockprof.threshold=500 \
    dalvik.vm.dexopt-flags=m=y

PRODUCT_CHARACTERISTICS := tablet

PRODUCT_AAPT_CONFIG := xlarge mdpi

PRODUCT_PACKAGES += \
	make_ext4fs

PRODUCT_COPY_FILES += \
    device/hp/tenderloin/init.tenderloin.usb.rc:root/init.tenderloin.usb.rc \
    device/hp/tenderloin/init.tenderloin.rc:root/init.tenderloin.rc \
    device/hp/tenderloin/ueventd.tenderloin.rc:root/ueventd.tenderloin.rc \
    device/hp/tenderloin/initlogo.rle:root/initlogo.rle \
    device/hp/tenderloin/prebuilt/wifi/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf \
    device/hp/tenderloin/init.qcom.bt.sh:system/etc/init.qcom.bt.sh \
    device/hp/tenderloin/HPTouchpad.idc:system/usr/idc/HPTouchpad.idc

# Dualboot Magic
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/moboot_control:system/bin/moboot_control

# media minor check boot script
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/etc/init.d/10check_media_minor:system/etc/init.d/10check_media_minor

## (2) Also get non-open-source GSM-specific aspects if available
$(call inherit-product-if-exists, vendor/hp/tenderloin/tenderloin-vendor.mk)
## (3)  Finally, the least specific parts, i.e. the non-GSM-specific aspects

DEVICE_PACKAGE_OVERLAYS += device/hp/tenderloin/overlay

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:system/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml \
    frameworks/native/data/etc/android.hardware.camera.autofocus.xml:system/etc/permissions/android.hardware.camera.autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.xml:system/etc/permissions/android.hardware.location.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.compass.xml:system/etc/permissions/android.hardware.sensor.compass.xml \
    frameworks/native/data/etc/android.hardware.sensor.accelerometer.xml:system/etc/permissions/android.hardware.sensor.accelerometer.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.distinct.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.distinct.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml

# QCOM Hal
PRODUCT_PACKAGES += \
    copybit.msm8660 \
    hwcomposer.msm8660 \
    gralloc.msm8660 \
    liboverlay \
    libmemalloc \
    libtilerenderer \
    libgenlock \
    libQcomUI

# QCOM OMX
PRODUCT_PACKAGES += \
	libstagefrighthw \
	libOmxCore \
	libmm-omxcore \
	libdivxdrmdecrypt \
	libOmxVdec

# QCOM OMX Video Encoding
PRODUCT_PACKAGES += \
	libOmxVenc

# QCOM OMX Video Tests
#PRODUCT_PACKAGES += \
#	mm-vdev-omx-test \
#	mm-video-driver-test \
#	mm-venc-omx-test720p \
#	mm-video-encdrv-test

# Audio
PRODUCT_PACKAGES += \
    audio.a2dp.default \
    libaudioutils \
    audio.primary.tenderloin \

# Prebuilt audio libs
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/audio/lib/liba2dp.so:system/lib/liba2dp.so \
    device/hp/tenderloin/prebuilt/audio/lib/libasound.so:system/lib/libasound.so \
    device/hp/tenderloin/prebuilt/audio/lib/libaudiopolicy.so:system/lib/libaudiopolicy.so \
    device/hp/tenderloin/prebuilt/audio/lib/libaudio.so:system/lib/libaudio.so \
    device/hp/tenderloin/prebuilt/audio/lib/hw/alsa.tenderloin.so:system/lib/hw/alsa.tenderloin.so

# Prebuilt audio libs needed to compile other libs
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/audio/lib/libaudio.so:obj/lib/libaudio.so \
    device/hp/tenderloin/prebuilt/audio/lib/libaudiopolicy.so:obj/lib/libaudiopolicy.so \
	device/hp/tenderloin/prebuilt/audio/lib/liba2dp.so:obj/lib/liba2dp.so

# Prebuilt alsa configs
PRODUCT_COPY_FILES += \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/dsnoop.conf:system/usr/share/alsa/pcm/dsnoop.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/dmix.conf:system/usr/share/alsa/pcm/dmix.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/dpl.conf:system/usr/share/alsa/pcm/dpl.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/modem.conf:system/usr/share/alsa/pcm/modem.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/surround40.conf:system/usr/share/alsa/pcm/surround40.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/iec958.conf:system/usr/share/alsa/pcm/iec958.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/center_lfe.conf:system/usr/share/alsa/pcm/center_lfe.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/default.conf:system/usr/share/alsa/pcm/default.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/surround50.conf:system/usr/share/alsa/pcm/surround50.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/rear.conf:system/usr/share/alsa/pcm/rear.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/surround41.conf:system/usr/share/alsa/pcm/surround41.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/side.conf:system/usr/share/alsa/pcm/side.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/surround51.conf:system/usr/share/alsa/pcm/surround51.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/front.conf:system/usr/share/alsa/pcm/front.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/pcm/surround71.conf:system/usr/share/alsa/pcm/surround71.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/alsa.conf:system/usr/share/alsa/alsa.conf \
	device/hp/tenderloin/prebuilt/audio/usr/share/alsa/cards/aliases.conf:system/usr/share/alsa/cards/aliases.conf

# Sensors, misc
PRODUCT_PACKAGES += \
    librs_jni \
    wpa_supplicant.conf \
    sensors.tenderloin \
    lights.tenderloin \
    ts_srv \
    ts_srv_set \
    dosfsck \
    bcattach \
    serial \
    com.android.future.usb.accessory \
    rebootcmd \
    TenderloinParts

# Keylayouts
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/usr/keychars/qwerty2.kcm.bin:system/usr/keychars/qwerty2.kcm.bin \
    device/hp/tenderloin/prebuilt/usr/keychars/qwerty.kcm.bin:system/usr/keychars/qwerty.kcm.bin \
    device/hp/tenderloin/prebuilt/usr/keychars/ffa-keypad_numeric.kcm.bin:system/usr/keychars/ffa-keypad_numeric.kcm.bin \
    device/hp/tenderloin/prebuilt/usr/keychars/ffa-keypad_qwerty.kcm.bin:system/usr/keychars/ffa-keypad_qwerty.kcm.bin \
    device/hp/tenderloin/prebuilt/usr/keylayout/qwerty.kl:system/usr/keylayout/qwerty.kl \
    device/hp/tenderloin/prebuilt/usr/keylayout/handset.kl:system/usr/keylayout/handset.kl \
    device/hp/tenderloin/prebuilt/usr/keylayout/gpio-keys.kl:system/usr/keylayout/gpio-keys.kl \
    device/hp/tenderloin/prebuilt/usr/keylayout/AVRCP.kl:system/usr/keylayout/AVRCP.kl \
    device/hp/tenderloin/prebuilt/usr/keylayout/pmic8058_pwrkey.kl:system/usr/keylayout/pmic8058_pwrkey.kl \
    device/hp/tenderloin/prebuilt/bluetooth/hciattach:system/bin/hciattach_awesome 
    
# Misc Modules
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/modules/cifs.ko:system/lib/modules/cifs.ko \
    device/hp/tenderloin/prebuilt/modules/ntfs.ko:system/lib/modules/ntfs.ko \
    device/hp/tenderloin/prebuilt/modules/nls_utf8.ko:system/lib/modules/nls_utf8.ko \
    device/hp/tenderloin/prebuilt/modules/tun.ko:system/lib/modules/tun.ko

# Wifi Modules
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/wifi/ath6kl.ko:system/lib/modules/ath6kl.ko \
    device/hp/tenderloin/prebuilt/wifi/ath.ko:system/lib/modules/ath.ko \
    device/hp/tenderloin/prebuilt/wifi/cfg80211.ko:system/lib/modules/cfg80211.ko \
    device/hp/tenderloin/prebuilt/wifi/mac80211.ko:system/lib/modules/mac80211.ko \
    device/hp/tenderloin/prebuilt/wifi/compat.ko:system/lib/modules/compat.ko \
    device/hp/tenderloin/prebuilt/wifi/sch_codel.ko:system/lib/modules/sch_codel.ko
    device/hp/tenderloin/prebuilt/wifi/sch_fq_codel.ko:system/lib/modules/sch_fw_codel.ko

#Wifi Firmware
# from kernel.org
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.0/data.patch.bin:/system/etc/firmware/ath6k/AR6003/hw2.0/data.patch.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.0/bdata.SD31.bin:/system/etc/firmware/ath6k/AR6003/hw2.0/bdata.SD31.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.0/athwlan.bin.z77:/system/etc/firmware/ath6k/AR6003/hw2.0/athwlan.bin.z77 \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.0/bdata.WB31.bin:/system/etc/firmware/ath6k/AR6003/hw2.0/bdata.WB31.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.0/otp.bin.z77:/system/etc/firmware/ath6k/AR6003/hw2.0/otp.bin.z77 \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.0/bdata.SD32.bin:/system/etc/firmware/ath6k/AR6003/hw2.0/bdata.SD32.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/endpointping.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/endpointping.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/data.patch.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/data.patch.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/bdata.SD31.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/bdata.SD31.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/bdata.WB31.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/bdata.WB31.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/athwlan.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/athwlan.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/bdata.SD32.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/bdata.SD32.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/otp.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/otp.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/fw-3.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/fw-3.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw1.0/data.patch.bin:/system/etc/firmware/ath6k/AR6003/hw1.0/data.patch.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw1.0/bdata.SD31.bin:/system/etc/firmware/ath6k/AR6003/hw1.0/bdata.SD31.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw1.0/athwlan.bin.z77:/system/etc/firmware/ath6k/AR6003/hw1.0/athwlan.bin.z77 \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw1.0/bdata.WB31.bin:/system/etc/firmware/ath6k/AR6003/hw1.0/bdata.WB31.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw1.0/otp.bin.z77:/system/etc/firmware/ath6k/AR6003/hw1.0/otp.bin.z77 \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw1.0/bdata.SD32.bin:/system/etc/firmware/ath6k/AR6003/hw1.0/bdata.SD32.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6002/eeprom.data:/system/etc/firmware/ath6k/AR6002/eeprom.data \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6002/data.patch.hw2_0.bin:/system/etc/firmware/ath6k/AR6002/data.patch.hw2_0.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6002/athwlan.bin.z77:/system/etc/firmware/ath6k/AR6002/athwlan.bin.z77 \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6002/eeprom.bin:/system/etc/firmware/ath6k/AR6002/eeprom.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/LICENSE.atheros_firmware:/system/etc/firmware/ath6k/LICENSE.atheros_firmware \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.0/bdata.SD32.bin:/system/etc/firmware/ath6k/AR6003/hw2.0/bdata.CUSTOM.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/bdata.SD32.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/bdata.CUSTOM.bin \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw1.0/bdata.SD32.bin:/system/etc/firmware/ath6k/AR6003/hw1.0/bdata.CUSTOM.bin

# Wifi Firmware (hack for selecting proper bdata.bin)
PRODUCT_COPY_FILES += \
    device/hp/tenderloin/prebuilt/wifi/ath6k/AR6003/hw2.1.1/bdata.SD32.bin:/system/etc/firmware/ath6k/AR6003/hw2.1.1/bdata.bin

# we have enough storage space to hold precise GC data
PRODUCT_TAGS += dalvik.gc.type-precise

# device uses high-density artwork where available
PRODUCT_LOCALES += en_US mdpi

PRODUCT_COPY_FILES += \
    device/hp/tenderloin/configs/media_profiles.xml:system/etc/media_profiles.xml \
    device/hp/tenderloin/configs/media_codecs.xml:system/etc/media_codecs.xml \
    device/hp/tenderloin/vold.fstab:system/etc/vold.fstab \
    device/hp/tenderloin/makemulti.sh:makemulti.sh \
    device/hp/tenderloin/prebuilt/boot/moboot.splash.CyanogenMod.tga:moboot.splash.CyanogenMod.tga

$(call inherit-product, frameworks/native/build/tablet-dalvik-heap.mk)
$(call inherit-product, build/target/product/full_base.mk)
