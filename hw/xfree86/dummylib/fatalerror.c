/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/fatalerror.c,v 1.1 2000/02/13 03:06:38 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
FatalError(const char *f, ...)
{
    va_list args;

    va_start(args, f);
    fprintf(stderr, "Fatal Error:\n");
    vfprintf(stderr, f, args);
    va_end(args);
    exit(1);
}

