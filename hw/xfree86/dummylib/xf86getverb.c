/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86getverb.c,v 1.1 2000/02/13 03:06:41 dawes Exp $ */

#include "X.h"
#include "os.h"
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

