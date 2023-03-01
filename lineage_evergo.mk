#
# Copyright (C) 2023 The Lineage Project
#
# SPDX-License-Identifier: Apache-2.0
#

# Inherit from those products. Most specific first.
$(call inherit-product, $(SRC_TARGET_DIR)/product/core_64_bit.mk)
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base_telephony.mk)

# Inherit some common Lineage stuff.
$(call inherit-product, vendor/lineage/config/common_full_phone.mk)

# Inherit Vendor GApps
$(call inherit-product-if-exists, vendor/gapps/arm64/arm64-vendor.mk)

# Inherit from evergo device
$(call inherit-product, device/xiaomi/evergo/device.mk)

# Lineage Stuff
TARGET_BOOT_ANIMATION_RES := 1080
TARGET_GAPPS_ARCH := arm64
TARGET_SUPPORTS_QUICK_TAP := true
TARGET_USES_AOSP_RECOVERY := true

# Device identifier. This must come after all inclusions
PRODUCT_DEVICE := evergo
PRODUCT_NAME := lineage_evergo
PRODUCT_BRAND := Redmi
PRODUCT_MODEL := Redmi Note 11T 5G
PRODUCT_MANUFACTURER := Xiaomi

# Build info
PRODUCT_GMS_CLIENTID_BASE := android-xiaomi

BUILD_FINGERPRINT := Redmi/evergo_in/evergo:12/SP1A.210812.016/V13.0.5.0.SGBINXM:user/release-keys
