/* $XFree86$ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
VErrorF(const char *f, va_list args)
{
    vfprintf(stderr, f, args);
}

