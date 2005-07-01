/* $XFree86$ */

#include <X11/X.h>
#include <X11/os.h>
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

int
xf86GetVerbosity()
{
    return xf86Verbose;
}

