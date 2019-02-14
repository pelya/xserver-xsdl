#!/bin/sh

set -x

export TARGET_ARCH=armeabi-v7a
export TARGET_HOST=armv7a-linux-androideabi16

../build.sh
