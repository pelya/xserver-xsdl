/* $XFree86: xc/programs/Xserver/mi/miinitext.c,v 3.68 2003/01/15 02:34:14 torrey Exp $ */
/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Xorg: miinitext.c,v 1.4 2001/02/09 02:05:21 xorgcvs Exp $ */

#include "misc.h"
#include "extension.h"
#include "micmap.h"

#ifdef NOPEXEXT /* sleaze for Solaris cpp building XsunMono */
#undef PEXEXT
#endif

#if defined(QNX4) /* sleaze for Watcom on QNX4 ... */
#undef PEXEXT
#undef XIE
#undef GLXEXT
#endif

#ifdef PANORAMIX
extern Bool noPanoramiXExtension;
#endif
extern Bool noTestExtensions;
#ifdef XKB
extern Bool noXkbExtension;
#endif

#ifndef XFree86LOADER
#define INITARGS void
typedef void (*InitExtension)(INITARGS);
#else /* XFree86Loader */
#include "loaderProcs.h"
#endif

#ifdef MITSHM
#define _XSHM_SERVER_
#include "shmstr.h"
#endif
#ifdef XTEST
#define _XTEST_SERVER_
#include "XTest.h"
#endif
#ifdef XKB
#include "XKB.h"
#endif
#ifdef LBX
#define _XLBX_SERVER_
#include "lbxstr.h"
#endif
#ifdef XPRINT
#include "Print.h"
#endif
#ifdef XAPPGROUP
#define _XAG_SERVER_
#include "Xagstr.h"
#endif
#ifdef XCSECURITY
#define _SECURITY_SERVER
#include "securstr.h"
#endif
#ifdef PANORAMIX
#include "panoramiXproto.h"
#endif
#ifdef XF86BIGFONT
#include "xf86bigfstr.h"
#endif
#ifdef RES
#include "XResproto.h"
#endif

/* FIXME: this whole block of externs should be from the appropriate headers */
#ifdef BEZIER
extern void BezierExtensionInit(INITARGS);
#endif
#ifdef XTESTEXT1
extern void XTestExtension1Init(INITARGS);
#endif
#ifdef SHAPE
extern void ShapeExtensionInit(INITARGS);
#endif
#ifdef EVI
extern void EVIExtensionInit(INITARGS);
#endif
#ifdef MITSHM
extern void ShmExtensionInit(INITARGS);
#endif
#ifdef PEXEXT
extern void PexExtensionInit(INITARGS);
#endif
#ifdef MULTIBUFFER
extern void MultibufferExtensionInit(INITARGS);
#endif
#ifdef PANORAMIX
extern void PanoramiXExtensionInit(INITARGS);
#endif
#ifdef XINPUT
extern void XInputExtensionInit(INITARGS);
#endif
#ifdef XTEST
extern void XTestExtensionInit(INITARGS);
#endif
#ifdef BIGREQS
extern void BigReqExtensionInit(INITARGS);
#endif
#ifdef MITMISC
extern void MITMiscExtensionInit(INITARGS);
#endif
#ifdef XIDLE
extern void XIdleExtensionInit(INITARGS);
#endif
#ifdef XTRAP
extern void DEC_XTRAPInit(INITARGS);
#endif
#ifdef SCREENSAVER
extern void ScreenSaverExtensionInit (INITARGS);
#endif
#ifdef XV
extern void XvExtensionInit(INITARGS);
extern void XvMCExtensionInit(INITARGS);
#endif
#ifdef XIE
extern void XieInit(INITARGS);
#endif
#ifdef XSYNC
extern void SyncExtensionInit(INITARGS);
#endif
#ifdef XKB
extern void XkbExtensionInit(INITARGS);
#endif
#ifdef XCMISC
extern void XCMiscExtensionInit(INITARGS);
#endif
#ifdef XRECORD
extern void RecordExtensionInit(INITARGS);
#endif
#ifdef LBX
extern void LbxExtensionInit(INITARGS);
#endif
#ifdef DBE
extern void DbeExtensionInit(INITARGS);
#endif
#ifdef XAPPGROUP
extern void XagExtensionInit(INITARGS);
#endif
#ifdef XCSECURITY
extern void SecurityExtensionInit(INITARGS);
#endif
#ifdef XPRINT
extern void XpExtensionInit(INITARGS);
#endif
#ifdef XF86BIGFONT
extern void XFree86BigfontExtensionInit(INITARGS);
#endif
#ifdef XF86VIDMODE
extern void XFree86VidModeExtensionInit(INITARGS);
#endif
#ifdef XF86MISC
extern void XFree86MiscExtensionInit(INITARGS);
#endif
#ifdef XFreeXDGA
extern void XFree86DGAExtensionInit(INITARGS);
#endif
#ifdef GLXEXT
#ifndef __DARWIN__
extern void GlxExtensionInit(INITARGS);
extern void GlxWrapInitVisuals(miInitVisualsProcPtr *);
#else
extern void DarwinGlxExtensionInit(INITARGS);
extern void DarwinGlxWrapInitVisuals(miInitVisualsProcPtr *);
#endif
#endif
#ifdef XF86DRI
extern void XFree86DRIExtensionInit(INITARGS);
#endif
#ifdef TOGCUP
extern void XcupExtensionInit(INITARGS);
#endif
#ifdef DPMSExtension
extern void DPMSExtensionInit(INITARGS);
#endif
#ifdef DPSEXT
extern void DPSExtensionInit(INITARGS);
#endif
#ifdef FONTCACHE
extern void FontCacheExtensionInit(INITARGS);
#endif
#ifdef RENDER
extern void RenderExtensionInit(INITARGS);
#endif
#ifdef RANDR
extern void RRExtensionInit(INITARGS);
#endif
#ifdef RES
extern void ResExtensionInit(INITARGS);
#endif

#ifndef XFree86LOADER

/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
#ifdef PANORAMIX
# if !defined(PRINT_ONLY_SERVER) && !defined(NO_PANORAMIX)
  if (!noPanoramiXExtension) PanoramiXExtensionInit();
# endif
#endif
#ifdef BEZIER
    BezierExtensionInit();
#endif
#ifdef XTESTEXT1
    if (!noTestExtensions) XTestExtension1Init();
#endif
#ifdef SHAPE
    ShapeExtensionInit();
#endif
#ifdef MITSHM
    ShmExtensionInit();
#endif
#ifdef EVI
    EVIExtensionInit();
#endif
#ifdef PEXEXT
    PexExtensionInit();
#endif
#ifdef MULTIBUFFER
    MultibufferExtensionInit();
#endif
#if defined(XINPUT) && !defined(NO_HW_ONLY_EXTS)
    XInputExtensionInit();
#endif
#ifdef XTEST
    if (!noTestExtensions) XTestExtensionInit();
#endif
#ifdef BIGREQS
    BigReqExtensionInit();
#endif
#ifdef MITMISC
    MITMiscExtensionInit();
#endif
#ifdef XIDLE
    XIdleExtensionInit();
#endif
#ifdef XTRAP
    if (!noTestExtensions) DEC_XTRAPInit();
#endif
#if defined(SCREENSAVER) && !defined(PRINT_ONLY_SERVER)
    ScreenSaverExtensionInit ();
#endif
#ifdef XV
    XvExtensionInit();
    XvMCExtensionInit();
#endif
#ifdef XIE
    XieInit();
#endif
#ifdef XSYNC
    SyncExtensionInit();
#endif
#if defined(XKB) && !defined(PRINT_ONLY_SERVER) && !defined(NO_HW_ONLY_EXTS)
    if (!noXkbExtension) XkbExtensionInit();
#endif
#ifdef XCMISC
    XCMiscExtensionInit();
#endif
#ifdef XRECORD
    if (!noTestExtensions) RecordExtensionInit(); 
#endif
#ifdef LBX
    LbxExtensionInit();
#endif
#ifdef DBE
    DbeExtensionInit();
#endif
#ifdef XAPPGROUP
    XagExtensionInit();
#endif
#ifdef XCSECURITY
    SecurityExtensionInit();
#endif
#ifdef XPRINT
    XpExtensionInit();
#endif
#ifdef TOGCUP
    XcupExtensionInit();
#endif
#if defined(DPMSExtension) && !defined(NO_HW_ONLY_EXTS)
    DPMSExtensionInit();
#endif
#ifdef FONTCACHE
    FontCacheExtensionInit();
#endif
#ifdef XF86BIGFONT
    XFree86BigfontExtensionInit();
#endif
#if !defined(PRINT_ONLY_SERVER) && !defined(NO_HW_ONLY_EXTS)
#if defined(XF86VIDMODE)
    XFree86VidModeExtensionInit();
#endif
#if defined(XF86MISC)
    XFree86MiscExtensionInit();
#endif
#if defined(XFreeXDGA)
    XFree86DGAExtensionInit();
#endif
#ifdef XF86DRI
    XFree86DRIExtensionInit();
#endif
#endif
#ifdef GLXEXT
#ifndef XPRINT	/* we don't want Glx in the Xprint server */
#ifndef __DARWIN__
    GlxExtensionInit();
#else
    DarwinGlxExtensionInit();
#endif
#endif
#endif
#ifdef DPSEXT
#ifndef XPRINT
    DPSExtensionInit();
#endif
#endif
#ifdef RENDER
    RenderExtensionInit();
#endif
#ifdef RANDR
    RRExtensionInit();
#endif
#ifdef RES
    ResExtensionInit();
#endif
}

void
InitVisualWrap()
{
    miResetInitVisuals();
#ifdef GLXEXT
#ifndef XPRINT
#ifndef __DARWIN__
    GlxWrapInitVisuals(&miInitVisualsProc);
#else
    DarwinGlxWrapInitVisuals(&miInitVisualsProc);
#endif
#endif
#endif
}

#else /* XFree86LOADER */
#if 0
/* FIXME:The names here must come from the headers. those with ?? are 
   not included in X11R6.3 sample implementation, so there's a problem... */
/* XXX use the correct #ifdefs for symbols not present when an extension
   is disabled */
ExtensionModule extension[] =
{
    { NULL, "BEZIER", NULL, NULL },	/* ?? */
    { NULL, "XTEST1", &noTestExtensions, NULL }, /* ?? */
    { NULL, "SHAPE", NULL, NULL },
    { NULL, "MIT-SHM", NULL, NULL },
    { NULL, "X3D-PEX", NULL, NULL },
    { NULL, "Multi-Buffering", NULL, NULL },
    { NULL, "XInputExtension", NULL, NULL },
    { NULL, "XTEST", &noTestExtensions, NULL },
    { NULL, "BIG-REQUESTS", NULL, NULL },
    { NULL, "MIT-SUNDRY-NONSTANDARD", NULL, NULL },
    { NULL, "XIDLE", NULL, NULL },	/* ?? */
    { NULL, "XTRAP", &noTestExtensions, NULL }, /* ?? */
    { NULL, "MIT-SCREEN-SAVER", NULL, NULL },
    { NULL, "XVideo", NULL, NULL },	/* ?? */
    { NULL, "XIE", NULL, NULL },
    { NULL, "SYNC", NULL, NULL },
#ifdef XKB
    { NULL, "XKEYBOARD", &noXkbExtension, NULL },
#else
    { NULL, "NOXKEYBOARD", NULL, NULL },
#endif
    { NULL, "XC-MISC", NULL, NULL },
    { NULL, "RECORD", &noTestExtensions, NULL },
    { NULL, "LBX", NULL, NULL },
    { NULL, "DOUBLE-BUFFER", NULL, NULL },
    { NULL, "XC-APPGROUP", NULL, NULL },
    { NULL, "SECURITY", NULL, NULL },
    { NULL, "XpExtension", NULL, NULL },
    { NULL, "XFree86-VidModeExtension", NULL, NULL },
    { NULL, "XFree86-Misc", NULL, NULL },
    { NULL, "XFree86-DGA", NULL, NULL },
    { NULL, "DPMS", NULL, NULL },
    { NULL, "GLX", NULL, NULL },
    { NULL, "TOG-CUP", NULL, NULL },
    { NULL, "Extended-Visual-Information", NULL, NULL },
#ifdef PANORAMIX
    { NULL, "XINERAMA", &noPanoramiXExtension, NULL },
#else
    { NULL, "NOXINERAMA", NULL, NULL },
#endif
    { NULL, "XFree86-Bigfont", NULL, NULL },
    { NULL, "XFree86-DRI", NULL, NULL },
    { NULL, "Adobe-DPS-Extension", NULL, NULL },
    { NULL, "FontCache", NULL, NULL },
    { NULL, "RENDER", NULL, NULL },
    { NULL, "RANDR", NULL, NULL },
    { NULL, "X-Resource", NULL, NULL },
    { NULL, NULL, NULL, NULL }
};
#endif

/* List of built-in (statically linked) extensions */
static ExtensionModule staticExtensions[] = {
#ifdef BEZIER
    { BezierExtensionInit, "BEZIER", NULL, NULL, NULL },
#endif
#ifdef XTESTEXT1
    { XTestExtension1Init, "XTEST1", &noTestExtensions, NULL, NULL },
#endif
#ifdef MITSHM
    { ShmExtensionInit, SHMNAME, NULL, NULL, NULL },
#endif
#ifdef XINPUT
    { XInputExtensionInit, "XInputExtension", NULL, NULL, NULL },
#endif
#ifdef XTEST
    { XTestExtensionInit, XTestExtensionName, &noTestExtensions, NULL, NULL },
#endif
#ifdef XIDLE
    { XIdleExtensionInit, "XIDLE", NULL, NULL, NULL },
#endif
#ifdef XKB
    { XkbExtensionInit, XkbName, &noXkbExtension, NULL, NULL },
#endif
#ifdef LBX
    { LbxExtensionInit, LBXNAME, NULL, NULL, NULL },
#endif
#ifdef XAPPGROUP
    { XagExtensionInit, XAGNAME, NULL, NULL, NULL },
#endif
#ifdef XCSECURITY
    { SecurityExtensionInit, SECURITY_EXTENSION_NAME, NULL, NULL, NULL },
#endif
#ifdef XPRINT
    { XpExtensionInit, XP_PRINTNAME, NULL, NULL, NULL },
#endif
#ifdef PANORAMIX
    { PanoramiXExtensionInit, PANORAMIX_PROTOCOL_NAME, &noPanoramiXExtension, NULL, NULL },
#endif
#ifdef XF86BIGFONT
    { XFree86BigfontExtensionInit, XF86BIGFONTNAME, NULL, NULL, NULL },
#endif
#ifdef RENDER
    { RenderExtensionInit, "RENDER", NULL, NULL, NULL },
#endif
#ifdef RANDR
    { RRExtensionInit, "RANDR", NULL, NULL, NULL },
#endif
    { NULL, NULL, NULL, NULL, NULL }
};
    
/*ARGSUSED*/
void
InitExtensions(argc, argv)
    int		argc;
    char	*argv[];
{
    int i;
    ExtensionModule *ext;
    static Bool listInitialised = FALSE;

    if (!listInitialised) {
	/* Add built-in extensions to the list. */
	for (i = 0; staticExtensions[i].name; i++)
	    LoadExtension(&staticExtensions[i], TRUE);

	/* Sort the extensions according the init dependencies. */
	LoaderSortExtensions();
	listInitialised = TRUE;
    }

    for (i = 0; ExtensionModuleList[i].name != NULL; i++) {
	ext = &ExtensionModuleList[i];
	if (ext->initFunc != NULL && 
	    (ext->disablePtr == NULL || 
	     (ext->disablePtr != NULL && !*ext->disablePtr))) {
	    (ext->initFunc)();
	}
    }
}

static void (*__miHookInitVisualsFunction)(miInitVisualsProcPtr *);

void
InitVisualWrap()
{
    miResetInitVisuals();
    if (__miHookInitVisualsFunction)
	(*__miHookInitVisualsFunction)(&miInitVisualsProc);
}

void miHookInitVisuals(void (**old)(miInitVisualsProcPtr *),
		       void (*new)(miInitVisualsProcPtr *))
{
    if (old)
	*old = __miHookInitVisualsFunction;
    __miHookInitVisualsFunction = new;
}

#endif /* XFree86LOADER */
