/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/pm_noop.c,v 1.1 2000/02/13 03:36:11 dawes Exp $ */

/* Stubs for the OS-support layer power-management functions. */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"

PMClose
xf86OSPMOpen(void)
{
	return NULL;
}


