/* $XFree86: xc/programs/Xserver/hw/xfree86/dummylib/xf86addrestolist.c,v 1.3 2002/12/24 15:50:43 tsi Exp $ */

#include "xf86.h"

resPtr
xf86AddResToList(resPtr rlist, resRange *Range, int entityIndex)
{
    return rlist;
}

void
xf86FreeResList(resPtr rlist)
{
    return;
}

resPtr
xf86DupResList(const resPtr rlist)
{
    return rlist;
}
