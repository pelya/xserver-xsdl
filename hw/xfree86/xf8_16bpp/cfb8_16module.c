/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_16bpp/cfb8_16module.c,v 1.1 1999/01/31 12:22:16 dawes Exp $ */

#ifdef XFree86LOADER

#include "xf86Module.h"

static MODULESETUPPROTO(xf8_16bppSetup);

static XF86ModuleVersionInfo VersRec =
{
        "xf8_16bpp",
        MODULEVENDORSTRING,
        MODINFOSTRING1,
        MODINFOSTRING2,
        XF86_VERSION_CURRENT,
        1, 0, 0,
        ABI_CLASS_ANSIC,                /* Only need the ansic layer */
        ABI_ANSIC_VERSION,
        NULL,
        {0,0,0,0}       /* signature, to be patched into the file by a tool */
};

XF86ModuleData xf8_16bppModuleData = { &VersRec, xf8_16bppSetup, NULL };

static pointer
xf8_16bppSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    if (!LoadSubModule(module, "cfb", NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    if (!LoadSubModule(module, "cfb16", NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    return (pointer)1;  /* non-NULL required to indicate success */
}

#endif
