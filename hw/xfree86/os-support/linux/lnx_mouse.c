/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_mouse.c,v 1.1 1999/05/17 13:17:18 dawes Exp $ */

/*
 * Copyright 1999 by The XFree86 Project, Inc.
 */

#include "X.h"
#include "xf86.h"
#include "xf86Xinput.h"
#include "xf86OSmouse.h"

static int
SupportedInterfaces(void)
{
    return MSE_SERIAL | MSE_BUS | MSE_PS2 | MSE_XPS2 | MSE_AUTO;
}

OSMouseInfoPtr
xf86OSMouseInit(int flags)
{
    OSMouseInfoPtr p;

    p = xcalloc(sizeof(OSMouseInfoRec), 1);
    if (!p)
	return NULL;
    p->SupportedInterfaces = SupportedInterfaces;
    return p;
}

