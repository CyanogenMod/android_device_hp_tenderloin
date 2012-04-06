$(call inherit-product, device/hp/tenderloin/device_tenderloin.mk)

PRODUCT_RELEASE_NAME := Touchpad
TARGET_BOOTANIMATION_NAME := horizontal-1024x768

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

PRODUCT_BUILD_PROP_OVERRIDES += PRODUCT_NAME=touchpad BUILD_FINGERPRINT=hp/hp_tenderloin/tenderloin:4.0.4/IMM76D/223971:user/release-keys PRIVATE_BUILD_DESC="tenderloin-user 4.0.4 IMM76D 223971 release-keys"

PRODUCT_NAME := cm_tenderloin
PRODUCT_DEVICE := tenderloin
