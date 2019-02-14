#!/bin/sh

set -x

export TARGET_ARCH=x86
export TARGET_HOST=i686-linux-android16

../build.sh
