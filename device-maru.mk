# init
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/init.maru.rc:root/init.maru.rc

# core
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/perspective/mcontainer-start.sh:system/xbin/mcontainer-start.sh

# input
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/excluded-input-devices.xml:system/etc/excluded-input-devices.xml

# container
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/prebuilts/jessie-rootfs.tar.gz:data/maru/containers/jessie/jessie-rootfs.tar.gz \
    device/lge/hammerhead/maru/container/jessie/config:data/maru/containers/jessie/config \
    device/lge/hammerhead/maru/container/jessie/fstab:data/maru/containers/jessie/fstab

# utilities
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/prebuilts/busybox:system/xbin/busybox

# LXC
PRODUCT_COPY_FILES += \
    device/lge/hammerhead/maru/prebuilts/lxc/bin/lxc-start:system/xbin/lxc-start \
    device/lge/hammerhead/maru/prebuilts/lxc/lib/liblxc.so.1.0.7:system/lib/liblxc.so.1 \
    device/lge/hammerhead/maru/prebuilts/lxc/lib/lxc/rootfs/README:data/maru/lxc/lib/lxc/rootfs/README \
    device/lge/hammerhead/maru/prebuilts/lxc/libexec/lxc/lxc-monitord:data/maru/lxc/libexec/lxc/lxc-monitord \
    device/lge/hammerhead/maru/prebuilts/lxc/share/lxc/config/common.seccomp:data/maru/lxc/share/lxc/config/common.seccomp \
    device/lge/hammerhead/maru/prebuilts/lxc/share/lxc/config/debian.common.conf:data/maru/lxc/share/lxc/config/debian.common.conf \
    device/lge/hammerhead/maru/prebuilts/lxc/share/lxc/config/debian.userns.conf:data/maru/lxc/share/lxc/config/debian.userns.conf


DEVICE_PACKAGE_OVERLAYS := \
    device/lge/hammerhead/maru/overlay/location
