/*
 * Abstraction of the AGP GART interface.  Stubs for platforms without
 * AGP GART support.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/agp_noop.c,v 1.3 2001/05/19 00:26:46 dawes Exp $ */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

Bool
xf86GARTCloseScreen(int screenNum)
{
	return FALSE;
}

Bool
xf86AgpGARTSupported()
{
	return FALSE;
}

AgpInfoPtr
xf86GetAGPInfo(int screenNum)
{
	return NULL;
}

Bool
xf86AcquireGART(int screenNum)
{
	return FALSE;
}

Bool
xf86ReleaseGART(int screenNum)
{
	return FALSE;
}

int
xf86AllocateGARTMemory(int screenNum, unsigned long size, int type,
			unsigned long *physical)
{
	return -1;
}


Bool
xf86BindGARTMemory(int screenNum, int key, unsigned long offset)
{
	return FALSE;
}


Bool
xf86UnbindGARTMemory(int screenNum, int key)
{
	return FALSE;
}

Bool
xf86EnableAGP(int screenNum, CARD32 mode)
{
	return FALSE;
}
