#!/system/bin/sh

DST="/data/maru/containers/jessie"
ROOTFS="$DST/jessie-rootfs.tar.gz"

if [ -d "$DST/rootfs" ] ; then
    echo "rootfs already exists, nothing to be done" >&2
    exit 0
fi

if [ ! -f $ROOTFS ] ; then
    echo "can't find rootfs $ROOTFS" >&2
    exit 1
fi

echo "extracting $ROOTFS to $DST..."

busybox tar xzf "$ROOTFS" -C "$DST"

if [ $? -ne 0 ] ; then
    echo "failed to extract rootfs" >&2
    exit 1
fi

echo "success!"
