/* $XFree86$ */

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

