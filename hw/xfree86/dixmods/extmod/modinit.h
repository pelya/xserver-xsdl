/* $XFree86: xc/programs/Xserver/Xext/extmod/modinit.h,v 1.2 2003/09/13 21:33:04 dawes Exp $ */

#ifndef INITARGS
#define INITARGS void
#endif

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

#if 1
extern void XTestExtension1Init(INITARGS);
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

#ifdef SHM
extern void ShmExtensionInit(INITARGS);
#include "shmstr.h"
extern void ShmSetPixmapFormat(
    ScreenPtr pScreen,
    int format);
extern void ShmRegisterFuncs(
    ScreenPtr pScreen,
    ShmFuncsPtr funcs);
#endif

#if 1
extern void SecurityExtensionInit(INITARGS);
#endif

#if 1
extern void XagExtensionInit(INITARGS);
#endif

#if 1
extern void XpExtensionInit(INITARGS);
#endif

#if 1
extern void PanoramiXExtensionInit(int argc, char *argv[]);
#endif

#if 1
extern void XkbExtensionInit(INITARGS);
#endif
