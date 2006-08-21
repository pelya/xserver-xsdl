
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
