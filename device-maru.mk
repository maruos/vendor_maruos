# init
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/init.maru.rc:root/init.maru.rc

# input
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/excluded-input-devices.xml:system/etc/excluded-input-devices.xml

# container
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/prebuilts/jessie.tar.gz:data/maru/containers/jessie/jessie-rootfs.tar.gz \
    device/lge/hammerhead/maru/container/jessie/config:data/maru/containers/jessie/config \
    device/lge/hammerhead/maru/container/jessie/fstab:data/maru/containers/jessie/fstab \
    device/lge/hammerhead/maru/container/mcprepare.sh:system/bin/mcprepare

# LXC
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/prebuilts/lxc/lib/lxc/rootfs/README:data/maru/lxc/lib/lxc/rootfs/README \
    device/lge/hammerhead/maru/prebuilts/lxc/libexec/lxc/lxc-monitord:data/maru/lxc/libexec/lxc/lxc-monitord \
    device/lge/hammerhead/maru/prebuilts/lxc/share/lxc/config/common.seccomp:data/maru/lxc/share/lxc/config/common.seccomp \
    device/lge/hammerhead/maru/prebuilts/lxc/share/lxc/config/debian.common.conf:data/maru/lxc/share/lxc/config/debian.common.conf \
    device/lge/hammerhead/maru/prebuilts/lxc/share/lxc/config/debian.userns.conf:data/maru/lxc/share/lxc/config/debian.userns.conf

PRODUCT_PACKAGES += liblxc

ifeq ($(TARGET_BUILD_VARIANT),userdebug)
    PRODUCT_PACKAGES += \
        lxc-start \
        lxc-stop \
        lxc-info \
        lxc-console
endif

# bootanim
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/prebuilts/mbootanim.zip:system/media/bootanimation.zip

DEVICE_PACKAGE_OVERLAYS := \
    device/lge/hammerhead/maru/overlay/location \
    device/lge/hammerhead/maru/overlay/apps \
    device/lge/hammerhead/maru/overlay/daydream

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
