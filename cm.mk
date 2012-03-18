$(call inherit-product, device/hp/tenderloin/device_tenderloin.mk)

PRODUCT_RELEASE_NAME := Touchpad
TARGET_BOOTANIMATION_NAME := horizontal-1024x768

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

PRODUCT_BUILD_PROP_OVERRIDES += PRODUCT_NAME=touchpad BUILD_ID=IML74K BUILD_FINGERPRINT=hp/hp_tenderloin/tenderloin:4.0.3/IML74K/223971:user/release-keys PRIVATE_BUILD_DESC="tenderloin-user 4.0.3 IML74K 223971 release-keys" BUILD_NUMBER=189904

PRODUCT_NAME := cm_tenderloin
PRODUCT_DEVICE := tenderloin
