/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/xf86int10module.c,v 1.2 2002/09/16 18:06:09 eich Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include "xf86.h"
#include "xf86str.h"
#include "xf86Pci.h"
#include "xf86int10.h"


#ifdef XFree86LOADER


#ifndef MOD_NAME
# define MOD_NAME int10
#endif

#define stringify(x) #x
#define STRING(x) stringify(x)
#define concat(x,y) x ## y
#define combine(a,b) concat(a,b)
#define NAME(x) combine(MOD_NAME,x)

static MODULESETUPPROTO(NAME(Setup));

static XF86ModuleVersionInfo NAME(VersRec) =
{
    STRING(NAME( )),
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XF86_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_VIDEODRV,		/* needs the video driver ABI */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0,0,0,0}
};

XF86ModuleData NAME(ModuleData) = { &NAME(VersRec), NAME(Setup), NULL };

static pointer
NAME(Setup)(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;
    
    if (!setupDone) {
	setupDone = TRUE;
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
