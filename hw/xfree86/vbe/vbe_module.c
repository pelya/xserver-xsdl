/* $XFree86: xc/programs/Xserver/hw/xfree86/vbe/vbe_module.c,v 1.1 2003/02/17 17:06:46 dawes Exp $ */

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
    XF86_VERSION_CURRENT,
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

