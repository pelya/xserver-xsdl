/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86drvmsgverb.c,v 1.1 2000/02/13 03:06:40 dawes Exp $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
xf86DrvMsgVerb(int i, MessageType type, int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVWrite(verb, format, ap);
    va_end(ap);
}

