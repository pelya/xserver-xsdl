/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86getpagesize.c,v 1.1 2000/02/13 03:06:41 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

int xf86getpagesize(void);

int
xf86getpagesize(void)
{
    return 4096;	/* not used */
}

