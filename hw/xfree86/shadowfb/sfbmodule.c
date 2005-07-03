/* $XFree86: xc/programs/Xserver/hw/xfree86/shadowfb/sfbmodule.c,v 1.1 1999/01/31 12:38:06 dawes Exp $ */


#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifdef XFree86LOADER

#include "xf86Module.h"

static XF86ModuleVersionInfo VersRec =
{
        "shadowfb",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XORG_VERSION_CURRENT,
        1, 0, 0,
        ABI_CLASS_ANSIC,                /* Only need the ansic layer */
        ABI_ANSIC_VERSION,
        MOD_CLASS_NONE,
        {0,0,0,0}       /* signature, to be patched into the file by a tool */
};

XF86ModuleData shadowfbModuleData = { &VersRec, NULL, NULL };

#endif
