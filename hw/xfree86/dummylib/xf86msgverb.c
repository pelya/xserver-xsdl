/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86msgverb.c,v 1.1 2000/02/13 03:06:42 dawes Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "dummylib.h"

/*
 * Utility functions required by libxf86_os. 
 */

void
xf86MsgVerb(MessageType type, int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    VErrorFVerb(verb, format, ap);
    va_end(ap);
}

