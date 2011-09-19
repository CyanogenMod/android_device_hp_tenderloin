#!/bin/sh

# Copyright (C) 2010 The Android Open Source Project
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

DEVICE=tenderloin;
MANUFACTURER=hp;

if [ $1 ]; then
    ZIPFILE=$1
else
    ZIPFILE=../../../${DEVICE}_update.zip
fi

if [ ! -f "$1" ]; then
    echo "Cannot find $ZIPFILE.  Try specifify the stock update.zip with $0 <zipfilename>"
    exit 1
fi

mkdir -p ../../../vendor/$MANUFACTURER/$DEVICE/proprietary

DIRS="
bin
etc
etc/firmware
lib
lib/hw
lib/egl
"

for DIR in $DIRS; do
	mkdir -p ../../../vendor/$MANUFACTURER/$DEVICE/proprietary/$DIR
done

#SHARED OBJECT LIBRARIES
unzip -j -o $ZIPFILE -d ../../../vendor/$MANUFACTURER/$DEVICE/proprietary/lib \
    system/lib/libcamera.so \
    system/lib/libaudioalsa.so \
    system/lib/libaudcal.so \
    system/lib/libdiag.so \
    system/lib/libgsl.so \
    system/lib/libmmipl.so \
    system/lib/libmmjpeg.so \
    system/lib/libOpenVG.so \
    system/lib/libqdp.so \
    system/lib/libqmi.so \
    system/lib/libqmiservices.so 

#EGL
unzip -j -o $ZIPFILE -d ../../../vendor/$MANUFACTURER/$DEVICE/proprietary/lib/egl \
    system/lib/egl/libEGL_adreno200.so \
    system/lib/egl/libGLESv1_CM_adreno200.so \
    system/lib/egl/libGLESv2_adreno200.so \
    system/lib/egl/libq3dtools_adreno200.so

#HW
unzip -j -o $ZIPFILE -d ../../../vendor/$MANUFACTURER/$DEVICE/proprietary/lib/hw \
    system/lib/hw/lights.msm8660.so \

#BIN
unzip -j -o $ZIPFILE -d ../../../vendor/$MANUFACTURER/$DEVICE/proprietary/bin \
    system/bin/dcvs \
    system/bin/dcvsd \
    system/bin/mpdecision \
    system/bin/mpld \
    system/bin/sensord \
    system/bin/thermald \
    system/bin/usbhub \
    system/bin/usbhub_init \

#Firmware
unzip -j -o $ZIPFILE -d ../../../vendor/$MANUFACTURER/$DEVICE/proprietary/etc/firmware \
    system/etc/firmware/leia_pfp_470.fw \
    system/etc/firmware/leia_pm4_470.fw \
    system/etc/firmware/vidc_1080p.fw \
    system/etc/firmware/yamato_pfp.fw \
    system/etc/firmware/yamato_pm4.fw
