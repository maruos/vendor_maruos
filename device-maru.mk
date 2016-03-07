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
PRODUCT_COPY_FILES += \
    vendor/maru/prebuilts/lxc/lib/lxc/rootfs/README:system/maru/lxc/lib/lxc/rootfs/README \
    vendor/maru/prebuilts/lxc/libexec/lxc/lxc-monitord:system/maru/lxc/libexec/lxc/lxc-monitord \
    vendor/maru/prebuilts/lxc/share/lxc/config/common.seccomp:system/maru/lxc/share/lxc/config/common.seccomp \
    vendor/maru/prebuilts/lxc/share/lxc/config/debian.common.conf:system/maru/lxc/share/lxc/config/debian.common.conf \
    vendor/maru/prebuilts/lxc/share/lxc/config/debian.userns.conf:system/maru/lxc/share/lxc/config/debian.userns.conf

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
