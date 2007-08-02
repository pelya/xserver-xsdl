/*
 * Copyright © 1999 Network Computing Devices, Inc.  All rights reserved.
 *
 * Author: Keith Packard
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "kdrive.h"
#include <X11/keysym.h>

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
    }
    return FALSE;
}

void
KdOsAddInputDrivers (void)
{
    KdAddPointerDriver(&VxWorksMouseDriver);
    KdAddPointerDriver(&VxWorksKeyboardDriver);
}

KdOsFuncs   VxWorksFuncs = {
    .SpecialKey = VxWorksSpecialKey,
};

void
OsVendorInit (void)
{
    KdOsInit (&VxWorksFuncs);
}
