#!/bin/sh

set -x

export TARGET_ARCH=armeabi
export TARGET_HOST=arm-linux-androideabi

../build.sh
