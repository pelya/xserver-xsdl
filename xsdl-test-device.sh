#!/bin/sh

#env DISPLAY=192.168.42.129:0.0 sh -c "x-window-manager & xlogo & xev"

adb forward tcp:6001 tcp:6000

env DISPLAY=127.0.0.1:1 sh -c "xfwm4 & xlogo & xev"
