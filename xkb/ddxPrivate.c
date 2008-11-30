
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define NEED_EVENTS
#include <X11/X.h>
#include "windowstr.h"
#include <xkbsrv.h>

_X_EXPORT int
XkbDDXPrivate(DeviceIntPtr dev,KeyCode key,XkbAction *act)
{
    return 0;
}
