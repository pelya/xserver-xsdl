/* $XFree86: xc/programs/Xserver/Xext/xvmod.c,v 1.1 1998/08/13 14:45:36 dawes Exp $ */

#include "X.h"
#include "misc.h"
#include "scrnintstr.h"
#include "gc.h"
#include "Xv.h"
#include "Xvproto.h"
#include "xvdix.h"
#include "xvmodproc.h"

void
XvRegister()
{
    XvScreenInitProc = XvScreenInit;
    XvGetScreenIndexProc = XvGetScreenIndex;
    XvGetRTPortProc = XvGetRTPort;
    XvMCScreenInitProc = XvMCScreenInit;
}

