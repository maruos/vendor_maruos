# early build definitions for maru
include $(LOCAL_PATH)/maru_build.mk

# init
PRODUCT_COPY_FILES += \
    vendor/maru/init.maru.rc:root/init.maru.rc

# input
PRODUCT_COPY_FILES += \
    vendor/maru/excluded-input-devices.xml:system/etc/excluded-input-devices.xml

# container
PRODUCT_COPY_FILES += \
    vendor/maru/prebuilts/jessie-min.tar.gz:system/maru/containers/jessie/jessie-rootfs.tar.gz \
    vendor/maru/container/jessie/config:system/maru/containers/jessie/config \
    vendor/maru/container/jessie/fstab:system/maru/containers/jessie/fstab \
    vendor/maru/container/mcprepare.sh:system/bin/mcprepare

# LXC
PRODUCT_PACKAGES += liblxc

# bootanim
PRODUCT_COPY_FILES += \
    vendor/maru/prebuilts/mbootanim.zip:system/media/bootanimation.zip

# PerspectiveService
PRODUCT_PACKAGES += \
    libperspective \
    perspectived

# mflinger
PRODUCT_PACKAGES += \
    libmaru \
    mflinger

# busybox
PRODUCT_PACKAGES += busybox

# only defined for hammerhead
PRODUCT_PACKAGES += TimeService


PRODUCT_PROPERTY_OVERRIDES += \
    ro.build.maru.version=$(MARU_BUILD_VERSION)
