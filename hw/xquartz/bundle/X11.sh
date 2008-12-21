#!/bin/bash --login

if [ -x ~/.x11run ]; then
	exec ~/.x11run "$(dirname "$0")"/X11.bin "${@}"
else
	exec "$(dirname "$0")"/X11.bin "${@}"
fi

