XCOMM!/bin/sh

XCOMM This script is used by XDarwin to start X clients when XDarwin is
XCOMM launched from the Finder.
XCOMM
XCOMM $XFree86: xc/programs/Xserver/hw/darwin/bundle/startXClients.cpp,v 1.1 2001/10/18 05:03:42 torrey Exp $

userclientrc=$HOME/.xinitrc
sysclientrc=XINITDIR/xinitrc
clientargs=""

if [ -f $userclientrc ]; then
    clientargs=$userclientrc
else if [ -f $sysclientrc ]; then
    clientargs=$sysclientrc
fi
fi

if [ "x$2" != "x" ]; then
    PATH="$PATH:$2"
    export PATH
fi

exec xinit $clientargs -- XBINDIR/XDarwinStartup "$1" -idle
