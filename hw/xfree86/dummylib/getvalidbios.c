/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/getvalidbios.c,v 1.3 2001/05/15 10:19:41 eich Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/*
 * Utility functions required by libxf86_os. 
 */

memType
getValidBIOSBase(PCITAG tag, int num)
{
    return 0;
}
