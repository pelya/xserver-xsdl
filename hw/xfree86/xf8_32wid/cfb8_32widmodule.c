/* $XFree86: xc/programs/Xserver/hw/xfree86/xf8_32wid/cfb8_32widmodule.c,v 1.1 2000/05/21 01:02:43 mvojkovi Exp $ */

#ifdef XFree86LOADER

#include "xf86Module.h"

static MODULESETUPPROTO(xf8_32widSetup);

static XF86ModuleVersionInfo VersRec =
{
        "xf8_32wid",
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

XF86ModuleData xf8_32widModuleData = { &VersRec, xf8_32widSetup, NULL };

static pointer
xf8_32widSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    if (!LoadSubModule(module, "mfb", NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    if (!LoadSubModule(module, "cfb", NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    if (!LoadSubModule(module, "cfb16", NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    if (!LoadSubModule(module, "cfb24", NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    if (!LoadSubModule(module, "cfb32", NULL, NULL, NULL, NULL,
			errmaj, errmin))
        return NULL;
    return (pointer)1;  /* non-NULL required to indicate success */
}

#endif
