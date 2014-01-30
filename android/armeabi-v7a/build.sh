#!/bin/sh

set -x

export TARGET_ARCH=armeabi-v7a
export TARGET_HOST=arm-linux-androideabi

../build.sh
