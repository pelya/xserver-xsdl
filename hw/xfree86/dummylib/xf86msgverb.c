/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86msgverb.c,v 1.2 2003/09/09 03:20:38 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
xf86MsgVerb(MessageType type, int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVWrite(verb, format, ap);
    va_end(ap);
}

