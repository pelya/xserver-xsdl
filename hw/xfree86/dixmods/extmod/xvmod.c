/* $XFree86$ */

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

