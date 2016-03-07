#!/system/bin/sh

MARU_SYSTEM_DIR="/system/maru"
MARU_DATA_DIR="/data/maru"

SRC_DIR="$MARU_SYSTEM_DIR/containers/jessie"
DST_DIR="$MARU_DATA_DIR/containers/jessie"
ROOTFS="$SRC_DIR/jessie-rootfs.tar.gz"

if [ ! -d "$DST_DIR" ] ; then
    echo "copying maru files to /data..."
    busybox cp -r "$MARU_SYSTEM_DIR" "/data/"

    # not needed since we read rootfs from /system
    busybox rm "$DST_DIR/jessie-rootfs.tar.gz"
fi

if [ -d "$DST_DIR/rootfs" ] ; then
    echo "rootfs already exists, nothing to be done" >&2
    exit 0
fi

if [ ! -f $ROOTFS ] ; then
    echo "can't find rootfs $ROOTFS" >&2
    exit 1
fi

echo "extracting $ROOTFS to $DST_DIR..."
busybox tar xzf "$ROOTFS" -C "$DST_DIR"

if [ $? -ne 0 ] ; then
    echo "failed to extract rootfs" >&2
    exit 1
fi

echo "success!"
