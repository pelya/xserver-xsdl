/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/logvwrite.c,v 1.1 2003/09/09 03:20:38 dawes Exp $ */

#include "X.h"
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

