#!/bin/sh

set -x

export TARGET_ARCH=mips
export TARGET_HOST=mipsel-linux-android

../build.sh
