#!/bin/sh

env DISPLAY=192.168.42.129:0.0 sh -c "x-window-manager & xlogo & xev & xclock -update 1"
