#!/bin/bash

#
# Copyright 2018 The Maru OS Project
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

set -e
set -u

# Reference:
# https://developer.android.com/training/multiscreen/screendensities#TaskProvideAltBmp

help () {
    cat <<_EOF

Generate alternative bitmap drawables for different densities.
Usage: $(basename "$0") [IMAGE]
_EOF
}

if ! command -v convert >/dev/null ; then
    echo >&2 "[x] Missing dependency: 'convert'. Please install imagemagick!"
    exit 2
fi

src_img="$1"
if ! [ -f "$src_img" ] ; then
    echo >&2 "[x] Invalid wallpaper file: '$1'"
    help
    exit 2
fi
src_img="$(readlink -f "$src_img")"
dst_img="$(basename "$src_img")"

echo "[*] Generating alternative drawables for '$dst_img'..."

# 3/16 = 0.1875
mkdir drawable-ldpi
cd drawable-ldpi
convert -resize 18.75% "$src_img" "$dst_img"
cd - >/dev/null

# 4/16 = 0.25
mkdir drawable-mdpi
cd drawable-mdpi
convert -resize 25% "$src_img" "$dst_img"
cd - >/dev/null

# 6/16 = 0.375
mkdir drawable-hdpi
cd drawable-hdpi
convert -resize 37.5% "$src_img" "$dst_img"
cd - >/dev/null

# 8/16 = 0.50
mkdir drawable-xhdpi
cd drawable-xhdpi
convert -resize 50% "$src_img" "$dst_img"
cd - >/dev/null

# 12/16 = 0.75
mkdir drawable-xxhdpi
cd drawable-xxhdpi
convert -resize 75% "$src_img" "$dst_img"
cd - >/dev/null

# 16/16 = 1
mkdir drawable-xxxhdpi
cd drawable-xxxhdpi
cp "$src_img" "$dst_img"
cd - >/dev/null

echo "[*] All tasks completed successfully."
