#!/bin/bash
#
#  This is a convenience tool to make new uMulti images for quick testing and custom boot images.
#
#  Default behavior is to create normal boot.img (from $OUT/root)
#
#  use -r to make a clockworkmod recovery.img (from $OUT/recovery/root)

if [ -d "$HOME/android/system/out/target/product/tenderloin" ]; then
   OUT=~/android/system/out/target/product/tenderloin
   echo "setting \$OUT to $OUT..."
fi

if [ ! -d "$OUT" ]; then
   echo -e "\$OUT is unset or set incorrectly.  Re-build CM or type\nexport OUT=/path/to/out"
   exit 0
fi

# OS X users change the line below to point to mkimage
MKIMAGE=$OUT/../../../host/linux-x86/bin/mkimage
KERNEL=~/gh/CyanogenMod/hp-kernel-tenderloin/
ROOT=$OUT/root
CPIO_TARGET=ramdisk.img
UBOOTED_RAMDISK=ramdisk.ub
TARGET=boot.img

if [ "$1" = "-r" ]; then
   ROOT=$OUT/recovery/root
   CPIO_TARGET=recovery-ramdisk.img
   UBOOTED_RAMDISK=recovery-ramdisk.ub
   TARGET=recovery.img
fi

if [ ! -d "$ROOT" ]; then
   echo -e "$ROOT is not an existing folder.  Re-build CM or type\nexport OUT=/path/to/out"
   exit 0
fi

cd $ROOT
echo  "Making $CPIO_TARGET..."
find . |cpio -R 0:0 -H newc -o --quiet |gzip -9c > $OUT/$CPIO_TARGET
echo  "Making $UBOOTED_RAMDISK..."
$MKIMAGE -A ARM -T RAMDisk -C none -n Image -d $OUT/$CPIO_TARGET $OUT/$UBOOTED_RAMDISK
echo  "Making $TARGET from kernel and $UBOOTED_RAMDISK..."
$MKIMAGE -A arm -T multi -C none -n 'test-multi-image' -d $OUT/kernel:$OUT/$UBOOTED_RAMDISK $OUT/$TARGET
echo "Result is $OUT/$TARGET."
