#!/system/bin/sh

#
# Copyright 2015-2016 Preetam J. D'Souza
# Copyright 2016 The Maru OS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

MARU_SYSTEM_DIR="/system/maru"
MARU_DATA_DIR="/data/maru"

SRC_DIR="$MARU_SYSTEM_DIR/containers/default"
DST_DIR="$MARU_DATA_DIR/containers/default"
ROOTFS="$SRC_DIR/rootfs.tar.gz"

if [ ! -d "$DST_DIR" ] ; then
    echo "copying maru files to /data..."
    busybox cp -r "$MARU_SYSTEM_DIR" "/data/"

    # not needed since we read rootfs from /system
    busybox rm "$DST_DIR/rootfs.tar.gz"
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
