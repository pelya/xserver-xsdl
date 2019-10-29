#!/bin/sh

cp -f ../../pulseaudio/android-pulseaudio.conf pulseaudio.conf
mkdir -p tmp
tar cvfz data-1.tgz usr tmp pulseaudio.conf
