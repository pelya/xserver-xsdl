/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/vbe/vbe_module.c,v 1.4 2002/09/16 18:06:15 eich Exp $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86str.h"
#include "vbe.h"

extern const char *vbe_ddcSymbols[];

#ifdef XFree86LOADER

static MODULESETUPPROTO(vbeSetup);

static XF86ModuleVersionInfo vbeVersRec =
{
    "vbe",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    1, 1, 0,
    ABI_CLASS_VIDEODRV,		/* needs the video driver ABI */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0,0,0,0}
};

XF86ModuleData vbeModuleData = { &vbeVersRec, vbeSetup, NULL };

static pointer
vbeSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;
    
    if (!setupDone) {
	setupDone = TRUE;
	LoaderRefSymLists(vbe_ddcSymbols,NULL);
	/*
	 * Tell the loader about symbols from other modules that this module
	 * might refer to.
	 */
    } 
    /*
     * The return value must be non-NULL on success even though there
     * is no TearDownProc.
     */
    return (pointer)1;
}

#endif

