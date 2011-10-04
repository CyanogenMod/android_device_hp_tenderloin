#!/bin/bash
#
#  This is a manual way to make cm-uMulti and cwm-uMulti for quick testing and whatnot.
#
#  Default behavior is to create cm-uMulti (from $OUT/root)
#
#  use -r to make a clockworkmod recovery (from $OUT/recovery/root)

if [ -d "$HOME/android/system/out/target/product/tenderloin" ]; then
   OUT=~/android/system/out/target/product/tenderloin
   echo "setting \$OUT to $OUT..."
fi

if [ ! -d "$OUT" ]; then
   echo -e "\$OUT is unset or set incorrectly.  Re-build CM, or type\nexport OUT=/path/to/out\nQuitting."
   exit 0
fi

# OS X users change the line below to point to mkimage
MKIMAGE=$OUT/../../../host/linux-x86/bin/mkimage
KERNEL=~/gh/CyanogenMod/hp-kernel-tenderloin/
ROOT=$OUT/root
TARGET=cm

if [ "$1" = "-r" ]; then
   ROOT=$OUT/recovery/root
   TARGET=cwm
fi

if [ ! -d "$ROOT" ]; then
   echo -e "$ROOT is not an existing folder.  Re-build CM, or type\nexport OUT=/path/to/out\nQuitting."
   exit 0
fi

cd $ROOT
echo  "Making /tmp/andr-init.img into /tmp/uRamdisk..."
find . |cpio -R 0:0 -H newc -o --quiet |gzip -9c > /tmp/andr-init.img
$MKIMAGE -A ARM -T RAMDisk -C none -n Image -d /tmp/andr-init.img /tmp/uRamdisk
echo  "Making $OUT/$TARGET-uMulti from $OUT/kernel and /tmp/uRamdisk..."
$MKIMAGE -A arm -T multi -C none -n 'test-multi-image' -d $OUT/kernel:/tmp/uRamdisk $OUT/$TARGET-uMulti
echo "Result is $OUT/$TARGET-uMulti."
