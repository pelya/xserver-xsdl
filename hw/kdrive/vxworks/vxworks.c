/*
 * Id: vxworks.c,v 1.1 1999/11/24 08:35:24 keithp Exp $
 *
 * Copyright © 1999 Network Computing Devices, Inc.  All rights reserved.
 *
 * Author: Keith Packard
 */

#include "kdrive.h"
#include <X11/keysym.h>

int
VxWorksInit (void)
{
    return 1;
}

void
VxWorksEnable (void)
{
}

Bool
VxWorksSpecialKey (KeySym sym)
{
    switch (sym) {
    case XK_Sys_Req:
	download(1, "setup", 0);
	return TRUE;
    case XK_Break:
	download(1, "launcher", 0);
	return TRUE;
    case XK_Delete:
	dispatchException |= DE_REBOOT;
	return TRUE;
    case XK_BackSpace:
	dispatchException |= DE_RESET;
	return TRUE;
    }
    return FALSE;
}

void
VxWorksDisable (void)
{
}

void
VxWorksFini (void)
{
}

KdOsFuncs   VxWorksFuncs = {
    VxWorksInit,
    VxWorksEnable,
    VxWorksSpecialKey,
    VxWorksDisable,
    VxWorksFini,
};

void
OsVendorInit (void)
{
    KdOsInit (&VxWorksFuncs);
}
