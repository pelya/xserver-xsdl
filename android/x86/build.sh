#!/bin/sh

set -x

export TARGET_ARCH=mips
export TARGET_HOST=i686-linux-android

../build.sh
