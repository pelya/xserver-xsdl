/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/verrorfverb.c,v 1.2 2003/08/25 04:13:05 dawes Exp $ */

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
LogVWrite(int verb, const char *format, va_list ap)
{
    if (xf86Verbose >= verb)
	vfprintf(stderr, format, ap);
}

