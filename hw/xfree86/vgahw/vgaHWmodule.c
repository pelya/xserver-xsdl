/* $XFree86: xc/programs/Xserver/hw/xfree86/vgahw/vgaHWmodule.c,v 1.6 1999/01/26 05:54:18 dawes Exp $ */

/*
 * Copyright 1998 by The XFree86 Project, Inc
 */

#ifdef XFree86LOADER

#include "xf86Module.h"


static XF86ModuleVersionInfo VersRec = {
	"vgahw",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0, 1, 0,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	MOD_CLASS_NONE,
	{0, 0, 0, 0}
};

XF86ModuleData vgahwModuleData = { &VersRec, NULL, NULL };

#endif
