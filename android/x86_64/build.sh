#!/bin/sh

set -x

export TARGET_ARCH=x86_64
export TARGET_HOST=x86_64-linux-android21

../build.sh
