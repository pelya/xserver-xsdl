/* $XFree86: xc/programs/Xserver/Xext/extmod/modinit.c,v 1.16 2002/03/06 21:12:33 mvojkovi Exp $ */

/*
 *
 * Copyright (c) 1997 Matthieu Herrb
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Matthieu Herrb not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Matthieu Herrb makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * MATTHIEU HERRB DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL MATTHIEU HERRB BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef XFree86LOADER
#include "xf86_ansic.h"

#include "xf86Module.h"
#include "xf86Opt.h"

#include "Xproto.h"

static MODULESETUPPROTO(extmodSetup);

extern Bool noTestExtensions;

#ifdef SHAPE
extern void ShapeExtensionInit(INITARGS);
#define _SHAPE_SERVER_  /* don't want Xlib structures */
#include "shapestr.h"
#endif

#ifdef MULTIBUFFER
extern void MultibufferExtensionInit(INITARGS);
#define _MULTIBUF_SERVER_	/* don't want Xlib structures */
#include "multibufst.h"
#endif

#ifdef MITMISC
extern void MITMiscExtensionInit(INITARGS);
#define _MITMISC_SERVER_
#include "mitmiscstr.h"
#endif

#ifdef XTEST
extern void XTestExtensionInit(INITARGS);
#define _XTEST_SERVER_
#include "XTest.h"
#include "xteststr.h"
#endif

#ifdef BIGREQS
extern void BigReqExtensionInit(INITARGS);
#include "bigreqstr.h"
#endif

#ifdef XSYNC
extern void SyncExtensionInit(INITARGS);
#define _SYNC_SERVER
#include "sync.h"
#include "syncstr.h"
#endif

#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (INITARGS);
#include "saver.h"
#endif

#ifdef XCMISC
extern void XCMiscExtensionInit(INITARGS);
#include "xcmiscstr.h"
#endif

#ifdef XF86VIDMODE
extern void	XFree86VidModeExtensionInit(INITARGS);
#define _XF86VIDMODE_SERVER_
#include "xf86vmstr.h"
#endif

#ifdef XF86MISC
extern void XFree86MiscExtensionInit(INITARGS);
#define _XF86MISC_SERVER_
#define _XF86MISC_SAVER_COMPAT_
#include "xf86mscstr.h"
#endif

#ifdef XFreeXDGA
extern void XFree86DGAExtensionInit(INITARGS);
extern void XFree86DGARegister(INITARGS);
#define _XF86DGA_SERVER_
#include "xf86dgastr.h"
#endif

#ifdef DPMSExtension
extern void DPMSExtensionInit(INITARGS);
#include "dpmsstr.h"
#endif

#ifdef FONTCACHE
extern void FontCacheExtensionInit(INITARGS);
#define _FONTCACHE_SERVER_
#include "fontcacheP.h"
#include "fontcachstr.h"
#endif

#ifdef TOGCUP
extern void XcupExtensionInit(INITARGS);
#define _XCUP_SERVER_
#include "Xcupstr.h"
#endif

#ifdef EVI
extern void EVIExtensionInit(INITARGS);
#define _XEVI_SERVER_
#include "XEVIstr.h"
#endif

#ifdef XV
extern void XvExtensionInit(INITARGS);
extern void XvMCExtensionInit(INITARGS);
extern void XvRegister(INITARGS);
#include "Xv.h"
#include "XvMC.h"
#endif

#ifdef RES
extern void ResExtensionInit(INITARGS);
#include "XResproto.h"
#endif

/*
 * Array describing extensions to be initialized
 */
ExtensionModule extensionModules[] = {
#ifdef SHAPE
    {
	ShapeExtensionInit,
	SHAPENAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef MULTIBUFFER
    {
	MultibufferExtensionInit,
	MULTIBUFFER_PROTOCOL_NAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef MITMISC
    {
	MITMiscExtensionInit,
	MITMISCNAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef notyet
    {
	XTestExtensionInit,
	XTestExtensionName,
	&noTestExtensions,
	NULL,
	NULL
    },
#endif
#ifdef BIGREQS
     {
	BigReqExtensionInit,
	XBigReqExtensionName,
	NULL,
	NULL,
	NULL
     },
#endif
#ifdef XSYNC
    {
	SyncExtensionInit,
	SYNC_NAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef SCREENSAVER
    {
	ScreenSaverExtensionInit,
	ScreenSaverName,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef XCMISC
    {
	XCMiscExtensionInit,
	XCMiscExtensionName,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef XF86VIDMODE
    {
	XFree86VidModeExtensionInit,
	XF86VIDMODENAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef XF86MISC
    {
	XFree86MiscExtensionInit,
	XF86MISCNAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef XFreeXDGA
    {
	XFree86DGAExtensionInit,
	XF86DGANAME,
	NULL,
	XFree86DGARegister,
	NULL
    },
#endif
#ifdef DPMSExtension
    {
	DPMSExtensionInit,
	DPMSExtensionName,
	NULL,
	NULL
    },
#endif
#ifdef FONTCACHE
    {
	FontCacheExtensionInit,
	FONTCACHENAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef TOGCUP
    {
	XcupExtensionInit,
	XCUPNAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef EVI
    {
	EVIExtensionInit,
	EVINAME,
	NULL,
	NULL,
	NULL
    },
#endif
#ifdef XV
    {
	XvExtensionInit,
	XvName,
	NULL,
	XvRegister,
	NULL
    },
    {
        XvMCExtensionInit,
        XvMCName,
        NULL,
        NULL,
        NULL
    },
#endif
#ifdef RES
    {
        ResExtensionInit,
        XRES_NAME, 
        NULL,
        NULL,
        NULL
    },
#endif
    {				/* DON'T delete this entry ! */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
    }
};

static XF86ModuleVersionInfo VersRec =
{
	"extmod",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_EXTENSION,
	ABI_EXTENSION_VERSION,
	MOD_CLASS_EXTENSION,
	{0,0,0,0}
};

/*
 * Data for the loader
 */
XF86ModuleData extmodModuleData = { &VersRec, extmodSetup, NULL };

static pointer
extmodSetup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    int i;

    /* XXX the option stuff here is largely a sample/test case */

    for (i = 0; extensionModules[i].name != NULL; i++) {
	if (opts) {
	    char *s;
	    s = (char *)xalloc(strlen(extensionModules[i].name) + 5);
	    if (s) {
		pointer o;
		strcpy(s, "omit");
		strcat(s, extensionModules[i].name);
		o = xf86FindOption(opts, s);
		xfree(s);
		if (o) {
		    xf86MarkOptionUsed(o);
		    continue;
		}
	    }
	}
	LoadExtension(&extensionModules[i], FALSE);
    }
    /* Need a non-NULL return */
    return (pointer)1;
}

#endif /* XFree86LOADER */
