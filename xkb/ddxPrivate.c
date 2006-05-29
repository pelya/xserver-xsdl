/* $XFree86: xc/programs/Xserver/xkb/ddxPrivate.c,v 1.2 2003/04/03 16:20:22 dawes Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define NEED_EVENTS
#include <X11/X.h>
#include "windowstr.h"
#include <X11/extensions/XKBsrv.h>

int
XkbDDXPrivate(DeviceIntPtr dev,KeyCode key,XkbAction *act)
{
    return 0;
}
