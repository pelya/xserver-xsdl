/* $XFree86: xc/programs/Xserver/GL/dri/dri.c,v 1.38 2002/11/20 18:10:24 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
Copyright 2000 VA Linux Systems, Inc.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Jens Owen <jens@tungstengraphics.com>
 *   Rickard E. (Rik) Faith <faith@valinux.com>
 *
 */

#ifdef XFree86LOADER
#include "xf86.h"
#include "xf86_ansic.h"
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#define NEED_REPLIES
#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "servermd.h"
#define _XF86DRI_SERVER_
#include "xf86dristr.h"
#include "swaprep.h"
#include "dri.h"
#include "sarea.h"
#include "dristruct.h"
#include "xf86.h"
#include "xf86drm.h"
#include "glxserver.h"
#include "mi.h"
#include "mipointer.h"

#if defined(XFree86LOADER) && !defined(PANORAMIX)
extern Bool noPanoramiXExtension;
#endif

static int DRIScreenPrivIndex = -1;
static int DRIWindowPrivIndex = -1;
static unsigned long DRIGeneration = 0;
static unsigned int DRIDrawableValidationStamp = 0;
static int lockRefCount=0;

				/* Support cleanup for fullscreen mode,
                                   independent of the DRICreateDrawable
                                   resource management. */
static Bool    _DRICloseFullScreen(pointer pResource, XID id);
static RESTYPE DRIFullScreenResType;

static RESTYPE DRIDrawablePrivResType;
static RESTYPE DRIContextPrivResType;
static void    DRIDestroyDummyContext(ScreenPtr pScreen, Bool hasCtxPriv);

				/* Wrapper just like xf86DrvMsg, but
				   without the verbosity level checking.
				   This will make it easy to turn off some
				   messages later, based on verbosity
				   level. */

/*
 * Since we're already referencing things from the XFree86 common layer in
 * this file, we'd might as well just call xf86VDrvMsgVerb, and have
 * consistent message formatting.  The verbosity of these messages can be
 * easily changed here.
 */
#define DRI_MSG_VERBOSITY 1
static void
DRIDrvMsg(int scrnIndex, MessageType type, const char *format, ...)
{
    va_list     ap;

    va_start(ap, format);
    xf86VDrvMsgVerb(scrnIndex, type, DRI_MSG_VERBOSITY, format, ap);
    va_end(ap);
}

Bool
DRIScreenInit(ScreenPtr pScreen, DRIInfoPtr pDRIInfo, int *pDRMFD)
{
    DRIScreenPrivPtr    pDRIPriv;
    drmContextPtr       reserved;
    int                 reserved_count;
    int                 i, fd, drmWasAvailable;
    Bool                xineramaInCore = FALSE;
    int                 err = 0;

    if (DRIGeneration != serverGeneration) {
	if ((DRIScreenPrivIndex = AllocateScreenPrivateIndex()) < 0)
	    return FALSE;
	DRIGeneration = serverGeneration;
    }

    /*
     * If Xinerama is on, don't allow DRI to initialise.  It won't be usable
     * anyway.
     */
#if defined(PANORAMIX) && !defined(XFree86LOADER)
    xineramaInCore = TRUE;
#elif defined(XFree86LOADER)
    if (xf86LoaderCheckSymbol("noPanoramiXExtension"))
	xineramaInCore = TRUE;
#endif

#if defined(PANORAMIX) || defined(XFree86LOADER)
    if (xineramaInCore) {
	if (!noPanoramiXExtension) {
	    DRIDrvMsg(pScreen->myNum, X_WARNING,
		"Direct rendering is not supported when Xinerama is enabled\n");
	    return FALSE;
	}
    }
#endif

    drmWasAvailable = drmAvailable();

    /* Note that drmOpen will try to load the kernel module, if needed. */
    fd = drmOpen(pDRIInfo->drmDriverName, NULL );
    if (fd < 0) {
        /* failed to open DRM */
        pScreen->devPrivates[DRIScreenPrivIndex].ptr = NULL;
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[drm] drmOpen failed\n");
        return FALSE;
    }

    if (!drmWasAvailable) {
       /* drmOpen loaded the kernel module, print a message to say so */
       DRIDrvMsg(pScreen->myNum, X_INFO,
                 "[drm] loaded kernel module for \"%s\" driver\n",
                 pDRIInfo->drmDriverName);
    }

    pDRIPriv = (DRIScreenPrivPtr) xcalloc(1, sizeof(DRIScreenPrivRec));
    if (!pDRIPriv) {
        pScreen->devPrivates[DRIScreenPrivIndex].ptr = NULL;
        return FALSE;
    }

    pScreen->devPrivates[DRIScreenPrivIndex].ptr = (pointer) pDRIPriv;
    pDRIPriv->drmFD = fd;
    pDRIPriv->directRenderingSupport = TRUE;
    pDRIPriv->pDriverInfo = pDRIInfo;
    pDRIPriv->nrWindows = 0;
    pDRIPriv->fullscreen = NULL;

    pDRIPriv->createDummyCtx     = pDRIInfo->createDummyCtx;
    pDRIPriv->createDummyCtxPriv = pDRIInfo->createDummyCtxPriv;

    pDRIPriv->grabbedDRILock = FALSE;
    pDRIPriv->drmSIGIOHandlerInstalled = FALSE;

    if ((err = drmSetBusid(pDRIPriv->drmFD, pDRIPriv->pDriverInfo->busIdString)) < 0) {
	pDRIPriv->directRenderingSupport = FALSE;
	pScreen->devPrivates[DRIScreenPrivIndex].ptr = NULL;
	drmClose(pDRIPriv->drmFD);
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[drm] drmSetBusid failed (%d, %s), %s\n",
                  pDRIPriv->drmFD, pDRIPriv->pDriverInfo->busIdString, strerror(-err));
	return FALSE;
    }

    *pDRMFD = pDRIPriv->drmFD;
    DRIDrvMsg(pScreen->myNum, X_INFO,
	      "[drm] created \"%s\" driver at busid \"%s\"\n",
	      pDRIPriv->pDriverInfo->drmDriverName,
	      pDRIPriv->pDriverInfo->busIdString);

    if (drmAddMap( pDRIPriv->drmFD,
		   0,
		   pDRIPriv->pDriverInfo->SAREASize,
		   DRM_SHM,
		   DRM_CONTAINS_LOCK,
		   &pDRIPriv->hSAREA) < 0)
    {
	pDRIPriv->directRenderingSupport = FALSE;
	pScreen->devPrivates[DRIScreenPrivIndex].ptr = NULL;
	drmClose(pDRIPriv->drmFD);
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[drm] drmAddMap failed\n");
	return FALSE;
    }
    DRIDrvMsg(pScreen->myNum, X_INFO,
	      "[drm] added %d byte SAREA at 0x%08lx\n",
	      pDRIPriv->pDriverInfo->SAREASize, pDRIPriv->hSAREA);

    if (drmMap( pDRIPriv->drmFD,
		pDRIPriv->hSAREA,
		pDRIPriv->pDriverInfo->SAREASize,
		(drmAddressPtr)(&pDRIPriv->pSAREA)) < 0)
    {
	pDRIPriv->directRenderingSupport = FALSE;
	pScreen->devPrivates[DRIScreenPrivIndex].ptr = NULL;
	drmClose(pDRIPriv->drmFD);
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[drm] drmMap failed\n");
	return FALSE;
    }
    memset(pDRIPriv->pSAREA, 0, pDRIPriv->pDriverInfo->SAREASize);
    DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] mapped SAREA 0x%08lx to %p\n",
	      pDRIPriv->hSAREA, pDRIPriv->pSAREA);

    if (drmAddMap( pDRIPriv->drmFD,
		   (drmHandle)pDRIPriv->pDriverInfo->frameBufferPhysicalAddress,
		   pDRIPriv->pDriverInfo->frameBufferSize,
		   DRM_FRAME_BUFFER,
		   0,
		   &pDRIPriv->hFrameBuffer) < 0)
    {
	pDRIPriv->directRenderingSupport = FALSE;
	pScreen->devPrivates[DRIScreenPrivIndex].ptr = NULL;
	drmUnmap(pDRIPriv->pSAREA, pDRIPriv->pDriverInfo->SAREASize);
	drmClose(pDRIPriv->drmFD);
        DRIDrvMsg(pScreen->myNum, X_INFO,
                  "[drm] drmAddMap failed\n");
	return FALSE;
    }
    DRIDrvMsg(pScreen->myNum, X_INFO, "[drm] framebuffer handle = 0x%08lx\n",
	      pDRIPriv->hFrameBuffer);

				/* Add tags for reserved contexts */
    if ((reserved = drmGetReservedContextList(pDRIPriv->drmFD,
					      &reserved_count))) {
	int  i;
	void *tag;

	for (i = 0; i < reserved_count; i++) {
	    tag = DRICreateContextPrivFromHandle(pScreen,
						 reserved[i],
						 DRI_CONTEXT_RESERVED);
	    drmAddContextTag(pDRIPriv->drmFD, reserved[i], tag);
	}
	drmFreeReservedContextList(reserved);
	DRIDrvMsg(pScreen->myNum, X_INFO,
		  "[drm] added %d reserved context%s for kernel\n",
		  reserved_count, reserved_count > 1 ? "s" : "");
    }

    /* validate max drawable table entry set by driver */
    if ((pDRIPriv->pDriverInfo->maxDrawableTableEntry <= 0) ||
        (pDRIPriv->pDriverInfo->maxDrawableTableEntry > SAREA_MAX_DRAWABLES)) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "Invalid max drawable table size set by driver: %d\n",
		      pDRIPriv->pDriverInfo->maxDrawableTableEntry);
    }

    /* Initialize drawable tables (screen private and SAREA) */
    for( i=0; i < pDRIPriv->pDriverInfo->maxDrawableTableEntry; i++) {
	pDRIPriv->DRIDrawables[i] = NULL;
	pDRIPriv->pSAREA->drawableTable[i].stamp = 0;
	pDRIPriv->pSAREA->drawableTable[i].flags = 0;
    }

    return TRUE;
}

Bool
DRIFinishScreenInit(ScreenPtr pScreen)
{
    DRIScreenPrivPtr  pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr        pDRIInfo = pDRIPriv->pDriverInfo;
    DRIContextFlags   flags    = 0;
    DRIContextPrivPtr pDRIContextPriv;

				/* Set up flags for DRICreateContextPriv */
    switch (pDRIInfo->driverSwapMethod) {
    case DRI_KERNEL_SWAP:    flags = DRI_CONTEXT_2DONLY;    break;
    case DRI_HIDE_X_CONTEXT: flags = DRI_CONTEXT_PRESERVED; break;
    }

    if (!(pDRIContextPriv = DRICreateContextPriv(pScreen,
						 &pDRIPriv->myContext,
						 flags))) {
	DRIDrvMsg(pScreen->myNum, X_ERROR,
		  "failed to create server context\n");
	return FALSE;
    }
    pDRIPriv->myContextPriv = pDRIContextPriv;

    DRIDrvMsg(pScreen->myNum, X_INFO,
	      "X context handle = 0x%08lx\n", pDRIPriv->myContext);

    /* Now that we have created the X server's context, we can grab the
     * hardware lock for the X server.
     */
    DRILock(pScreen, 0);
    pDRIPriv->grabbedDRILock = TRUE;

    /* pointers so that we can prevent memory leaks later */
    pDRIPriv->hiddenContextStore    = NULL;
    pDRIPriv->partial3DContextStore = NULL;

    switch(pDRIInfo->driverSwapMethod) {
    case DRI_HIDE_X_CONTEXT:
	/* Server will handle 3D swaps, and hide 2D swaps from kernel.
	 * Register server context as a preserved context.
	 */

	/* allocate memory for hidden context store */
	pDRIPriv->hiddenContextStore
	    = (void *)xcalloc(1, pDRIInfo->contextSize);
	if (!pDRIPriv->hiddenContextStore) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "failed to allocate hidden context\n");
	    DRIDestroyContextPriv(pDRIContextPriv);
	    return FALSE;
	}

	/* allocate memory for partial 3D context store */
	pDRIPriv->partial3DContextStore
	    = (void *)xcalloc(1, pDRIInfo->contextSize);
	if (!pDRIPriv->partial3DContextStore) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "[DRI] failed to allocate partial 3D context\n");
	    xfree(pDRIPriv->hiddenContextStore);
	    DRIDestroyContextPriv(pDRIContextPriv);
	    return FALSE;
	}

	/* save initial context store */
	if (pDRIInfo->SwapContext) {
	    (*pDRIInfo->SwapContext)(
		pScreen,
		DRI_NO_SYNC,
		DRI_2D_CONTEXT,
		pDRIPriv->hiddenContextStore,
		DRI_NO_CONTEXT,
		NULL);
	}
	/* fall through */

    case DRI_SERVER_SWAP:
        /* For swap methods of DRI_SERVER_SWAP and DRI_HIDE_X_CONTEXT
         * setup signal handler for receiving swap requests from kernel
	 */
	if (!(pDRIPriv->drmSIGIOHandlerInstalled =
	      drmInstallSIGIOHandler(pDRIPriv->drmFD, DRISwapContext))) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "[drm] failed to setup DRM signal handler\n");
	    if (pDRIPriv->hiddenContextStore)
		xfree(pDRIPriv->hiddenContextStore);
	    if (pDRIPriv->partial3DContextStore)
		xfree(pDRIPriv->partial3DContextStore);
	    DRIDestroyContextPriv(pDRIContextPriv);
	    return FALSE;
	} else {
	    DRIDrvMsg(pScreen->myNum, X_INFO,
		      "[drm] installed DRM signal handler\n");
	}

    default:
	break;
    }

    /* Wrap DRI support */
    if (pDRIInfo->wrap.ValidateTree) {
	pDRIPriv->wrap.ValidateTree     = pScreen->ValidateTree;
	pScreen->ValidateTree           = pDRIInfo->wrap.ValidateTree;
    }
    if (pDRIInfo->wrap.PostValidateTree) {
	pDRIPriv->wrap.PostValidateTree = pScreen->PostValidateTree;
	pScreen->PostValidateTree       = pDRIInfo->wrap.PostValidateTree;
    }
    if (pDRIInfo->wrap.WindowExposures) {
	pDRIPriv->wrap.WindowExposures  = pScreen->WindowExposures;
	pScreen->WindowExposures        = pDRIInfo->wrap.WindowExposures;
    }
    if (pDRIInfo->wrap.CopyWindow) {
	pDRIPriv->wrap.CopyWindow       = pScreen->CopyWindow;
	pScreen->CopyWindow             = pDRIInfo->wrap.CopyWindow;
    }
    if (pDRIInfo->wrap.ClipNotify) {
	pDRIPriv->wrap.ClipNotify       = pScreen->ClipNotify;
	pScreen->ClipNotify             = pDRIInfo->wrap.ClipNotify;
    }
    if (pDRIInfo->wrap.AdjustFrame) {
	ScrnInfoPtr pScrn               = xf86Screens[pScreen->myNum];
	pDRIPriv->wrap.AdjustFrame      = pScrn->AdjustFrame;
	pScrn->AdjustFrame              = pDRIInfo->wrap.AdjustFrame;
    }
    pDRIPriv->wrapped = TRUE;

    DRIDrvMsg(pScreen->myNum, X_INFO, "[DRI] installation complete\n");

    return TRUE;
}

void
DRICloseScreen(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr       pDRIInfo;
    drmContextPtr    reserved;
    int              reserved_count;

    if (pDRIPriv && pDRIPriv->directRenderingSupport) {

        pDRIInfo = pDRIPriv->pDriverInfo;

	if (pDRIPriv->wrapped) {
	    /* Unwrap DRI Functions */
	    if (pDRIInfo->wrap.ValidateTree) {
		pScreen->ValidateTree           = pDRIPriv->wrap.ValidateTree;
		pDRIPriv->wrap.ValidateTree     = NULL;
	    }
	    if (pDRIInfo->wrap.PostValidateTree) {
		pScreen->PostValidateTree       = pDRIPriv->wrap.PostValidateTree;
		pDRIPriv->wrap.PostValidateTree = NULL;
	    }
	    if (pDRIInfo->wrap.WindowExposures) {
		pScreen->WindowExposures        = pDRIPriv->wrap.WindowExposures;
		pDRIPriv->wrap.WindowExposures  = NULL;
	    }
	    if (pDRIInfo->wrap.CopyWindow) {
		pScreen->CopyWindow             = pDRIPriv->wrap.CopyWindow;
		pDRIPriv->wrap.CopyWindow       = NULL;
	    }
	    if (pDRIInfo->wrap.ClipNotify) {
		pScreen->ClipNotify             = pDRIPriv->wrap.ClipNotify;
		pDRIPriv->wrap.ClipNotify       = NULL;
	    }
	    if (pDRIInfo->wrap.AdjustFrame) {
		ScrnInfoPtr pScrn               = xf86Screens[pScreen->myNum];
		pScrn->AdjustFrame              = pDRIPriv->wrap.AdjustFrame;
		pDRIPriv->wrap.AdjustFrame      = NULL;
	    }
	    pDRIPriv->wrapped = FALSE;
	}

	if (pDRIPriv->drmSIGIOHandlerInstalled) {
	    if (!drmRemoveSIGIOHandler(pDRIPriv->drmFD)) {
		DRIDrvMsg(pScreen->myNum, X_ERROR,
			  "[drm] failed to remove DRM signal handler\n");
	    }
	}

        if (pDRIPriv->dummyCtxPriv && pDRIPriv->createDummyCtx) {
	    DRIDestroyDummyContext(pScreen, pDRIPriv->createDummyCtxPriv);
	}

	if (!DRIDestroyContextPriv(pDRIPriv->myContextPriv)) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "failed to destroy server context\n");
	}

				/* Remove tags for reserved contexts */
	if ((reserved = drmGetReservedContextList(pDRIPriv->drmFD,
						  &reserved_count))) {
	    int  i;

	    for (i = 0; i < reserved_count; i++) {
		DRIDestroyContextPriv(drmGetContextTag(pDRIPriv->drmFD,
						       reserved[i]));
	    }
	    drmFreeReservedContextList(reserved);
	    DRIDrvMsg(pScreen->myNum, X_INFO,
		      "[drm] removed %d reserved context%s for kernel\n",
		      reserved_count, reserved_count > 1 ? "s" : "");
	}

	/* Make sure signals get unblocked etc. */
	drmUnlock(pDRIPriv->drmFD, pDRIPriv->myContext);
	lockRefCount=0;
	DRIDrvMsg(pScreen->myNum, X_INFO,
		  "[drm] unmapping %d bytes of SAREA 0x%08lx at %p\n",
		  pDRIInfo->SAREASize,
		  pDRIPriv->hSAREA,
		  pDRIPriv->pSAREA);
	if (drmUnmap(pDRIPriv->pSAREA, pDRIInfo->SAREASize)) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "[drm] unable to unmap %d bytes"
		      " of SAREA 0x%08lx at %p\n",
		      pDRIInfo->SAREASize,
		      pDRIPriv->hSAREA,
		      pDRIPriv->pSAREA);
	}

	drmClose(pDRIPriv->drmFD);

	xfree(pDRIPriv);
	pScreen->devPrivates[DRIScreenPrivIndex].ptr = NULL;
    }
}

Bool
DRIExtensionInit(void)
{
    int		    	i;
    ScreenPtr		pScreen;

    if (DRIScreenPrivIndex < 0) {
	return FALSE;
    }

    /* Allocate a window private index with a zero sized private area for
     * each window, then should a window become a DRI window, we'll hang
     * a DRIWindowPrivateRec off of this private index.
     */
    if ((DRIWindowPrivIndex = AllocateWindowPrivateIndex()) < 0)
	return FALSE;

    DRIDrawablePrivResType = CreateNewResourceType(DRIDrawablePrivDelete);
    DRIContextPrivResType = CreateNewResourceType(DRIContextPrivDelete);
    DRIFullScreenResType = CreateNewResourceType(_DRICloseFullScreen);

    for (i = 0; i < screenInfo.numScreens; i++)
    {
	pScreen = screenInfo.screens[i];
	if (!AllocateWindowPrivate(pScreen, DRIWindowPrivIndex, 0))
	    return FALSE;
    }

    RegisterBlockAndWakeupHandlers(DRIBlockHandler, DRIWakeupHandler, NULL);

    return TRUE;
}

void
DRIReset(void)
{
    /*
     * This stub routine is called when the X Server recycles, resources
     * allocated by DRIExtensionInit need to be managed here.
     *
     * Currently this routine is a stub because all the interesting resources
     * are managed via the screen init process.
     */
}

Bool
DRIQueryDirectRenderingCapable(ScreenPtr pScreen, Bool* isCapable)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIPriv)
	*isCapable = pDRIPriv->directRenderingSupport;
    else
	*isCapable = FALSE;

    return TRUE;
}

Bool
DRIOpenConnection(ScreenPtr pScreen, drmHandlePtr hSAREA, char **busIdString)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    *hSAREA           = pDRIPriv->hSAREA;
    *busIdString      = pDRIPriv->pDriverInfo->busIdString;

    return TRUE;
}

Bool
DRIAuthConnection(ScreenPtr pScreen, drmMagic magic)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (drmAuthMagic(pDRIPriv->drmFD, magic)) return FALSE;
    return TRUE;
}

Bool
DRICloseConnection(ScreenPtr pScreen)
{
    return TRUE;
}

Bool
DRIGetClientDriverName(ScreenPtr pScreen,
                       int *ddxDriverMajorVersion,
                       int *ddxDriverMinorVersion,
                       int *ddxDriverPatchVersion,
                       char **clientDriverName)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    *ddxDriverMajorVersion = pDRIPriv->pDriverInfo->ddxDriverMajorVersion;
    *ddxDriverMinorVersion = pDRIPriv->pDriverInfo->ddxDriverMinorVersion;
    *ddxDriverPatchVersion = pDRIPriv->pDriverInfo->ddxDriverPatchVersion;
    *clientDriverName      = pDRIPriv->pDriverInfo->clientDriverName;

    return TRUE;
}

/* DRICreateContextPriv and DRICreateContextPrivFromHandle are helper
   functions that layer on drmCreateContext and drmAddContextTag.

   DRICreateContextPriv always creates a kernel drmContext and then calls
   DRICreateContextPrivFromHandle to create a DRIContextPriv structure for
   DRI tracking.  For the SIGIO handler, the drmContext is associated with
   DRIContextPrivPtr.  Any special flags are stored in the DRIContextPriv
   area and are passed to the kernel (if necessary).

   DRICreateContextPriv returns a pointer to newly allocated
   DRIContextPriv, and returns the kernel drmContext in pHWContext. */

DRIContextPrivPtr
DRICreateContextPriv(ScreenPtr pScreen,
		     drmContextPtr pHWContext,
		     DRIContextFlags flags)
{
    DRIScreenPrivPtr  pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (drmCreateContext(pDRIPriv->drmFD, pHWContext)) {
	return NULL;
    }

    return DRICreateContextPrivFromHandle(pScreen, *pHWContext, flags);
}

DRIContextPrivPtr
DRICreateContextPrivFromHandle(ScreenPtr pScreen,
			       drmContext hHWContext,
			       DRIContextFlags flags)
{
    DRIScreenPrivPtr  pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIContextPrivPtr pDRIContextPriv;
    int 	      contextPrivSize;

    contextPrivSize = sizeof(DRIContextPrivRec) +
			    pDRIPriv->pDriverInfo->contextSize;
    if (!(pDRIContextPriv = xcalloc(1, contextPrivSize))) {
	return NULL;
    }
    pDRIContextPriv->pContextStore = (void *)(pDRIContextPriv + 1);

    drmAddContextTag(pDRIPriv->drmFD, hHWContext, pDRIContextPriv);

    pDRIContextPriv->hwContext = hHWContext;
    pDRIContextPriv->pScreen   = pScreen;
    pDRIContextPriv->flags     = flags;
    pDRIContextPriv->valid3D   = FALSE;

    if (flags & DRI_CONTEXT_2DONLY) {
	if (drmSetContextFlags(pDRIPriv->drmFD,
			       hHWContext,
			       DRM_CONTEXT_2DONLY)) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "[drm] failed to set 2D context flag\n");
	    DRIDestroyContextPriv(pDRIContextPriv);
	    return NULL;
	}
    }
    if (flags & DRI_CONTEXT_PRESERVED) {
	if (drmSetContextFlags(pDRIPriv->drmFD,
			       hHWContext,
			       DRM_CONTEXT_PRESERVED)) {
	    DRIDrvMsg(pScreen->myNum, X_ERROR,
		      "[drm] failed to set preserved flag\n");
	    DRIDestroyContextPriv(pDRIContextPriv);
	    return NULL;
	}
    }
    return pDRIContextPriv;
}

Bool
DRIDestroyContextPriv(DRIContextPrivPtr pDRIContextPriv)
{
    DRIScreenPrivPtr pDRIPriv;

    if (!pDRIContextPriv) return TRUE;

    pDRIPriv = DRI_SCREEN_PRIV(pDRIContextPriv->pScreen);

    if (!(pDRIContextPriv->flags & DRI_CONTEXT_RESERVED)) {
				/* Don't delete reserved contexts from
                                   kernel area -- the kernel manages its
                                   reserved contexts itself. */
	if (drmDestroyContext(pDRIPriv->drmFD, pDRIContextPriv->hwContext))
	    return FALSE;
    }

				/* Remove the tag last to prevent a race
                                   condition where the context has pending
                                   buffers.  The context can't be re-used
                                   while in this thread, but buffers can be
                                   dispatched asynchronously. */
    drmDelContextTag(pDRIPriv->drmFD, pDRIContextPriv->hwContext);
    xfree(pDRIContextPriv);
    return TRUE;
}

static Bool
DRICreateDummyContext(ScreenPtr pScreen, Bool needCtxPriv)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    __GLXscreenInfo *pGLXScreen = &__glXActiveScreens[pScreen->myNum];
    __GLXvisualConfig *pGLXVis = pGLXScreen->pGlxVisual;
    void **pVisualConfigPriv = pGLXScreen->pVisualPriv;
    DRIContextPrivPtr pDRIContextPriv;
    void *contextStore;
    VisualPtr visual;
    int visNum;

    visual = pScreen->visuals;

    /* Find the X visual that corresponds the the first GLX visual */
    for (visNum = 0;
	 visNum < pScreen->numVisuals;
	 visNum++, visual++) {
	if (pGLXVis->vid == visual->vid)
	    break;
    }
    if (visNum == pScreen->numVisuals) return FALSE;

    if (!(pDRIContextPriv =
	  DRICreateContextPriv(pScreen,
			       &pDRIPriv->pSAREA->dummy_context, 0))) {
	return FALSE;
    }

    contextStore = DRIGetContextStore(pDRIContextPriv);
    if (pDRIPriv->pDriverInfo->CreateContext && needCtxPriv) {
	if (!pDRIPriv->pDriverInfo->CreateContext(pScreen, visual,
						  pDRIPriv->pSAREA->dummy_context,
						  *pVisualConfigPriv,
						  (DRIContextType)(long)contextStore)) {
	    DRIDestroyContextPriv(pDRIContextPriv);
	    return FALSE;
	}
    }

    pDRIPriv->dummyCtxPriv = pDRIContextPriv;
    return TRUE;
}

static void
DRIDestroyDummyContext(ScreenPtr pScreen, Bool hasCtxPriv)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIContextPrivPtr pDRIContextPriv = pDRIPriv->dummyCtxPriv;
    void *contextStore;

    if (!pDRIContextPriv) return;
    if (pDRIPriv->pDriverInfo->DestroyContext && hasCtxPriv) {
	contextStore = DRIGetContextStore(pDRIContextPriv);
	pDRIPriv->pDriverInfo->DestroyContext(pDRIContextPriv->pScreen,
					      pDRIContextPriv->hwContext,
					      (DRIContextType)(long)contextStore);
    }

    DRIDestroyContextPriv(pDRIPriv->dummyCtxPriv);
    pDRIPriv->dummyCtxPriv = NULL;
}

Bool
DRICreateContext(ScreenPtr pScreen, VisualPtr visual,
                 XID context, drmContextPtr pHWContext)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    __GLXscreenInfo *pGLXScreen = &__glXActiveScreens[pScreen->myNum];
    __GLXvisualConfig *pGLXVis = pGLXScreen->pGlxVisual;
    void **pVisualConfigPriv = pGLXScreen->pVisualPriv;
    DRIContextPrivPtr pDRIContextPriv;
    void *contextStore;
    int visNum;

    if (pDRIPriv->createDummyCtx && !pDRIPriv->dummyCtxPriv) {
        if (!DRICreateDummyContext(pScreen, pDRIPriv->createDummyCtxPriv)) {
	    DRIDrvMsg(pScreen->myNum, X_INFO,
		      "[drm] Could not create dummy context\n");
	    return FALSE;
	}
    }

    /* Find the GLX visual associated with the one requested */
    for (visNum = 0;
	 visNum < pGLXScreen->numVisuals;
	 visNum++, pGLXVis++, pVisualConfigPriv++)
	if (pGLXVis->vid == visual->vid)
	    break;
    if (visNum == pGLXScreen->numVisuals) {
	/* No matching GLX visual found */
	return FALSE;
    }

    if (!(pDRIContextPriv = DRICreateContextPriv(pScreen, pHWContext, 0))) {
	return FALSE;
    }

    contextStore = DRIGetContextStore(pDRIContextPriv);
    if (pDRIPriv->pDriverInfo->CreateContext) {
	if (!((*pDRIPriv->pDriverInfo->CreateContext)(pScreen, visual,
		*pHWContext, *pVisualConfigPriv,
		(DRIContextType)(long)contextStore))) {
	    DRIDestroyContextPriv(pDRIContextPriv);
	    return FALSE;
	}
    }

    /* track this in case the client dies before cleanup */
    AddResource(context, DRIContextPrivResType, (pointer)pDRIContextPriv);

    return TRUE;
}

Bool
DRIDestroyContext(ScreenPtr pScreen, XID context)
{
    FreeResourceByType(context, DRIContextPrivResType, FALSE);

    return TRUE;
}

/* DRIContextPrivDelete is called by the resource manager. */
Bool
DRIContextPrivDelete(pointer pResource, XID id)
{
    DRIContextPrivPtr pDRIContextPriv = (DRIContextPrivPtr)pResource;
    DRIScreenPrivPtr pDRIPriv;
    void *contextStore;

    pDRIPriv = DRI_SCREEN_PRIV(pDRIContextPriv->pScreen);
    if (pDRIPriv->pDriverInfo->DestroyContext) {
      contextStore = DRIGetContextStore(pDRIContextPriv);
      pDRIPriv->pDriverInfo->DestroyContext(pDRIContextPriv->pScreen,
					    pDRIContextPriv->hwContext,
					    (DRIContextType)(long)contextStore);
    }
    return DRIDestroyContextPriv(pDRIContextPriv);
}


/* This walks the drawable timestamp array and invalidates all of them
 * in the case of transition from private to shared backbuffers.  It's
 * not necessary for correctness, because DRIClipNotify gets called in
 * time to prevent any conflict, but the transition from
 * shared->private is sometimes missed if we don't do this.
 */
static void
DRIClipNotifyAllDrawables(ScreenPtr pScreen)
{
   int i;
   DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

   for( i=0; i < pDRIPriv->pDriverInfo->maxDrawableTableEntry; i++) {
      pDRIPriv->pSAREA->drawableTable[i].stamp = DRIDrawableValidationStamp++;
   }
}


static void
DRITransitionToSharedBuffers(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables( pScreen );

    if (pDRIInfo->TransitionSingleToMulti3D)
	pDRIInfo->TransitionSingleToMulti3D( pScreen );
}


static void
DRITransitionToPrivateBuffers(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables( pScreen );

    if (pDRIInfo->TransitionMultiToSingle3D)
	pDRIInfo->TransitionMultiToSingle3D( pScreen );
}


static void
DRITransitionTo3d(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables( pScreen );

    if (pDRIInfo->TransitionTo3d)
	pDRIInfo->TransitionTo3d( pScreen );
}

static void
DRITransitionTo2d(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIInfoPtr pDRIInfo = pDRIPriv->pDriverInfo;

    DRIClipNotifyAllDrawables( pScreen );

    if (pDRIInfo->TransitionTo2d)
	pDRIInfo->TransitionTo2d( pScreen );
}


Bool
DRICreateDrawable(ScreenPtr pScreen, Drawable id,
                  DrawablePtr pDrawable, drmDrawablePtr hHWDrawable)
{
    DRIScreenPrivPtr	pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr	pDRIDrawablePriv;
    WindowPtr		pWin;

    if (pDrawable->type == DRAWABLE_WINDOW) {
	pWin = (WindowPtr)pDrawable;
	if ((pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin))) {
	    pDRIDrawablePriv->refCount++;
	}
	else {
	    /* allocate a DRI Window Private record */
	    if (!(pDRIDrawablePriv = xalloc(sizeof(DRIDrawablePrivRec)))) {
		return FALSE;
	    }

	    /* Only create a drmDrawable once */
	    if (drmCreateDrawable(pDRIPriv->drmFD, hHWDrawable)) {
		xfree(pDRIDrawablePriv);
		return FALSE;
	    }

	    /* add it to the list of DRI drawables for this screen */
	    pDRIDrawablePriv->hwDrawable = *hHWDrawable;
	    pDRIDrawablePriv->pScreen = pScreen;
	    pDRIDrawablePriv->refCount = 1;
	    pDRIDrawablePriv->drawableIndex = -1;

	    /* save private off of preallocated index */
	    pWin->devPrivates[DRIWindowPrivIndex].ptr =
						(pointer)pDRIDrawablePriv;

	    switch (++pDRIPriv->nrWindows) {
	    case 1:
	       DRITransitionTo3d( pScreen );
	       break;
	    case 2:
	       DRITransitionToSharedBuffers( pScreen );
	       break;
	    default:
	       break;
	    }

	    /* track this in case this window is destroyed */
	    AddResource(id, DRIDrawablePrivResType, (pointer)pWin);
	}
    }
    else { /* pixmap (or for GLX 1.3, a PBuffer) */
	/* NOT_DONE */
	return FALSE;
    }

    return TRUE;
}

Bool
DRIDestroyDrawable(ScreenPtr pScreen, Drawable id, DrawablePtr pDrawable)
{
    DRIDrawablePrivPtr	pDRIDrawablePriv;
    WindowPtr		pWin;


    if (pDrawable->type == DRAWABLE_WINDOW) {
	pWin = (WindowPtr)pDrawable;
	pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);
	pDRIDrawablePriv->refCount--;
	if (pDRIDrawablePriv->refCount <= 0) {
	    /* This calls back DRIDrawablePrivDelete which frees private area */
	    FreeResourceByType(id, DRIDrawablePrivResType, FALSE);
	}
    }
    else { /* pixmap (or for GLX 1.3, a PBuffer) */
	/* NOT_DONE */
	return FALSE;
    }

    return TRUE;
}

Bool
DRIDrawablePrivDelete(pointer pResource, XID id)
{
    DrawablePtr		pDrawable = (DrawablePtr)pResource;
    DRIScreenPrivPtr	pDRIPriv = DRI_SCREEN_PRIV(pDrawable->pScreen);
    DRIDrawablePrivPtr	pDRIDrawablePriv;
    WindowPtr		pWin;

    if (pDrawable->type == DRAWABLE_WINDOW) {
	pWin = (WindowPtr)pDrawable;
	pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

	if (pDRIDrawablePriv->drawableIndex != -1) {
	    /* bump stamp to force outstanding 3D requests to resync */
	    pDRIPriv->pSAREA->drawableTable[pDRIDrawablePriv->drawableIndex].stamp
		= DRIDrawableValidationStamp++;

	    /* release drawable table entry */
	    pDRIPriv->DRIDrawables[pDRIDrawablePriv->drawableIndex] = NULL;
	}

	if (drmDestroyDrawable(pDRIPriv->drmFD,
			       pDRIDrawablePriv->hwDrawable)) {
	    return FALSE;
	}
	xfree(pDRIDrawablePriv);
	pWin->devPrivates[DRIWindowPrivIndex].ptr = NULL;

	switch (--pDRIPriv->nrWindows) {
	case 0:
	   DRITransitionTo2d( pDrawable->pScreen );
	   break;
	case 1:
	   DRITransitionToPrivateBuffers( pDrawable->pScreen );
	   break;
	default:
	   break;
	}
    }
    else { /* pixmap (or for GLX 1.3, a PBuffer) */
	/* NOT_DONE */
	return FALSE;
    }

    return TRUE;
}

Bool
DRIGetDrawableInfo(ScreenPtr pScreen,
                   DrawablePtr pDrawable,
                   unsigned int* index,
                   unsigned int* stamp,
                   int* X,
                   int* Y,
                   int* W,
                   int* H,
                   int* numClipRects,
                   XF86DRIClipRectPtr* pClipRects,
                   int* backX,
                   int* backY,
                   int* numBackClipRects,
                   XF86DRIClipRectPtr* pBackClipRects)
{
    DRIScreenPrivPtr    pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr	pDRIDrawablePriv, pOldDrawPriv;
    WindowPtr		pWin, pOldWin;
    int			i;

    printf("maxDrawableTableEntry = %d\n", pDRIPriv->pDriverInfo->maxDrawableTableEntry);

    if (pDrawable->type == DRAWABLE_WINDOW) {
	pWin = (WindowPtr)pDrawable;
	if ((pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin))) {

	    /* Manage drawable table */
	    if (pDRIDrawablePriv->drawableIndex == -1) { /* load SAREA table */

		/* Search table for empty entry */
		i = 0;
		while (i < pDRIPriv->pDriverInfo->maxDrawableTableEntry) {
		    if (!(pDRIPriv->DRIDrawables[i])) {
			pDRIPriv->DRIDrawables[i] = pDrawable;
			pDRIDrawablePriv->drawableIndex = i;
			pDRIPriv->pSAREA->drawableTable[i].stamp =
					    DRIDrawableValidationStamp++;
			break;
		    }
		    i++;
		}

		/* Search table for oldest entry */
		if (i == pDRIPriv->pDriverInfo->maxDrawableTableEntry) {
                    unsigned int oldestStamp = ~0;
                    int oldestIndex = 0;
		    i = pDRIPriv->pDriverInfo->maxDrawableTableEntry;
		    while (i--) {
			if (pDRIPriv->pSAREA->drawableTable[i].stamp <
								oldestStamp) {
			    oldestIndex = i;
			    oldestStamp =
				pDRIPriv->pSAREA->drawableTable[i].stamp;
			}
		    }
		    pDRIDrawablePriv->drawableIndex = oldestIndex;

		    /* release oldest drawable table entry */
		    pOldWin = (WindowPtr)pDRIPriv->DRIDrawables[oldestIndex];
		    pOldDrawPriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pOldWin);
		    pOldDrawPriv->drawableIndex = -1;

		    /* claim drawable table entry */
		    pDRIPriv->DRIDrawables[oldestIndex] = pDrawable;

		    /* validate SAREA entry */
		    pDRIPriv->pSAREA->drawableTable[oldestIndex].stamp =
					DRIDrawableValidationStamp++;

		    /* check for stamp wrap around */
		    if (oldestStamp > DRIDrawableValidationStamp) {

			/* walk SAREA table and invalidate all drawables */
			for( i=0;
                             i < pDRIPriv->pDriverInfo->maxDrawableTableEntry;
                             i++) {
				pDRIPriv->pSAREA->drawableTable[i].stamp =
					DRIDrawableValidationStamp++;
			}
		    }
		}

		/* If the driver wants to be notified when the index is
		 * set for a drawable, let it know now.
		 */
		if (pDRIPriv->pDriverInfo->SetDrawableIndex)
			pDRIPriv->pDriverInfo->SetDrawableIndex(pWin,
				pDRIDrawablePriv->drawableIndex);

		/* reinit drawable ID if window is visible */
		if ((pWin->viewable) &&
		    (pDRIPriv->pDriverInfo->bufferRequests != DRI_NO_WINDOWS))
		{
		    (*pDRIPriv->pDriverInfo->InitBuffers)(pWin,
			    &pWin->clipList, pDRIDrawablePriv->drawableIndex);
		}
	    }

	    *index = pDRIDrawablePriv->drawableIndex;
	    *stamp = pDRIPriv->pSAREA->drawableTable[*index].stamp;
	    *X = (int)(pWin->drawable.x);
	    *Y = (int)(pWin->drawable.y);
#if 0
	    *W = (int)(pWin->winSize.extents.x2 - pWin->winSize.extents.x1);
	    *H = (int)(pWin->winSize.extents.y2 - pWin->winSize.extents.y1);
#endif
	    *W = (int)(pWin->drawable.width);
	    *H = (int)(pWin->drawable.height);
	    *numClipRects = REGION_NUM_RECTS(&pWin->clipList);
	    *pClipRects = (XF86DRIClipRectPtr)REGION_RECTS(&pWin->clipList);

	    if (!*numClipRects && pDRIPriv->fullscreen) {
				/* use fake full-screen clip rect */
		pDRIPriv->fullscreen_rect.x1 = *X;
		pDRIPriv->fullscreen_rect.y1 = *Y;
		pDRIPriv->fullscreen_rect.x2 = *X + *W;
		pDRIPriv->fullscreen_rect.y2 = *Y + *H;

		*numClipRects = 1;
		*pClipRects   = &pDRIPriv->fullscreen_rect;
	    }

	    *backX = *X;
	    *backY = *Y;

	    if (pDRIPriv->nrWindows == 1 && *numClipRects) {
	       /* Use a single cliprect. */

	       int x0 = *X;
	       int y0 = *Y;
	       int x1 = x0 + *W;
	       int y1 = y0 + *H;

	       if (x0 < 0) x0 = 0;
	       if (y0 < 0) y0 = 0;
	       if (x1 > pScreen->width-1) x1 = pScreen->width-1;
	       if (y1 > pScreen->height-1) y1 = pScreen->height-1;

	       pDRIPriv->private_buffer_rect.x1 = x0;
	       pDRIPriv->private_buffer_rect.y1 = y0;
	       pDRIPriv->private_buffer_rect.x2 = x1;
	       pDRIPriv->private_buffer_rect.y2 = y1;

	       *numBackClipRects = 1;
	       *pBackClipRects = &(pDRIPriv->private_buffer_rect);
	    } else {
	       /* Use the frontbuffer cliprects for back buffers.  */
	       *numBackClipRects = 0;
	       *pBackClipRects = 0;
	    }
	}
	else {
	    /* Not a DRIDrawable */
	    return FALSE;
	}
    }
    else { /* pixmap (or for GLX 1.3, a PBuffer) */
	/* NOT_DONE */
	return FALSE;
    }

    return TRUE;
}

Bool
DRIGetDeviceInfo(ScreenPtr pScreen,
                 drmHandlePtr hFrameBuffer,
                 int* fbOrigin,
                 int* fbSize,
                 int* fbStride,
                 int* devPrivateSize,
                 void** pDevPrivate)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    *hFrameBuffer = pDRIPriv->hFrameBuffer;
    *fbOrigin = 0;
    *fbSize = pDRIPriv->pDriverInfo->frameBufferSize;
    *fbStride = pDRIPriv->pDriverInfo->frameBufferStride;
    *devPrivateSize = pDRIPriv->pDriverInfo->devPrivateSize;
    *pDevPrivate = pDRIPriv->pDriverInfo->devPrivate;

    return TRUE;
}

DRIInfoPtr
DRICreateInfoRec(void)
{
    DRIInfoPtr inforec = (DRIInfoPtr)xcalloc(1, sizeof(DRIInfoRec));
    if (!inforec) return NULL;

    /* Initialize defaults */
    inforec->busIdString = NULL;

    /* Wrapped function defaults */
    inforec->wrap.WakeupHandler         = DRIDoWakeupHandler;
    inforec->wrap.BlockHandler          = DRIDoBlockHandler;
    inforec->wrap.WindowExposures       = DRIWindowExposures;
    inforec->wrap.CopyWindow            = DRICopyWindow;
    inforec->wrap.ValidateTree          = DRIValidateTree;
    inforec->wrap.PostValidateTree      = DRIPostValidateTree;
    inforec->wrap.ClipNotify            = DRIClipNotify;
    inforec->wrap.AdjustFrame           = DRIAdjustFrame;

    inforec->TransitionTo2d = 0;
    inforec->TransitionTo3d = 0;
    inforec->SetDrawableIndex = 0;

    return inforec;
}

void
DRIDestroyInfoRec(DRIInfoPtr DRIInfo)
{
    if (DRIInfo->busIdString) xfree(DRIInfo->busIdString);
    xfree((char*)DRIInfo);
}


void
DRIWakeupHandler(pointer wakeupData, int result, pointer pReadmask)
{
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
	ScreenPtr        pScreen  = screenInfo.screens[i];
	DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

	if (pDRIPriv &&
	    pDRIPriv->pDriverInfo->wrap.WakeupHandler)
	    (*pDRIPriv->pDriverInfo->wrap.WakeupHandler)(i, wakeupData,
							 result, pReadmask);
    }
}

void
DRIBlockHandler(pointer blockData, OSTimePtr pTimeout, pointer pReadmask)
{
    int i;

    for (i = 0; i < screenInfo.numScreens; i++) {
	ScreenPtr        pScreen  = screenInfo.screens[i];
	DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

	if (pDRIPriv &&
	    pDRIPriv->pDriverInfo->wrap.BlockHandler)
	    (*pDRIPriv->pDriverInfo->wrap.BlockHandler)(i, blockData,
							pTimeout, pReadmask);
    }
}

void
DRIDoWakeupHandler(int screenNum, pointer wakeupData,
                   unsigned long result, pointer pReadmask)
{
    ScreenPtr pScreen = screenInfo.screens[screenNum];
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    DRILock(pScreen, 0);
    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
	/* hide X context by swapping 2D component here */
	(*pDRIPriv->pDriverInfo->SwapContext)(pScreen,
					      DRI_3D_SYNC,
					      DRI_2D_CONTEXT,
					      pDRIPriv->partial3DContextStore,
					      DRI_2D_CONTEXT,
					      pDRIPriv->hiddenContextStore);
    }
}

void
DRIDoBlockHandler(int screenNum, pointer blockData,
                  pointer pTimeout, pointer pReadmask)
{
    ScreenPtr pScreen = screenInfo.screens[screenNum];
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
	/* hide X context by swapping 2D component here */
	(*pDRIPriv->pDriverInfo->SwapContext)(pScreen,
					      DRI_2D_SYNC,
					      DRI_NO_CONTEXT,
					      NULL,
					      DRI_2D_CONTEXT,
					      pDRIPriv->partial3DContextStore);
    }
    DRIUnlock(pScreen);
}

void
DRISwapContext(int drmFD, void *oldctx, void *newctx)
{
    DRIContextPrivPtr oldContext      = (DRIContextPrivPtr)oldctx;
    DRIContextPrivPtr newContext      = (DRIContextPrivPtr)newctx;
    ScreenPtr         pScreen         = newContext->pScreen;
    DRIScreenPrivPtr  pDRIPriv        = DRI_SCREEN_PRIV(pScreen);
    void*             oldContextStore = NULL;
    DRIContextType    oldContextType;
    void*             newContextStore = NULL;
    DRIContextType    newContextType;
    DRISyncType       syncType;
#ifdef DEBUG
    static int        count = 0;
#endif

    if (!newContext) {
	DRIDrvMsg(pScreen->myNum, X_ERROR,
		  "[DRI] Context Switch Error: oldContext=%x, newContext=%x\n",
		  oldContext, newContext);
	return;
    }

#ifdef DEBUG
    /* usefull for debugging, just print out after n context switches */
    if (!count || !(count % 1)) {
	DRIDrvMsg(pScreen->myNum, X_INFO,
		  "[DRI] Context switch %5d from %p/0x%08x (%d)\n",
		  count,
		  oldContext,
		  oldContext ? oldContext->flags : 0,
		  oldContext ? oldContext->hwContext : -1);
	DRIDrvMsg(pScreen->myNum, X_INFO,
		  "[DRI] Context switch %5d to   %p/0x%08x (%d)\n",
		  count,
		  newContext,
		  newContext ? newContext->flags : 0,
		  newContext ? newContext->hwContext : -1);
    }
    ++count;
#endif

    if (!pDRIPriv->pDriverInfo->SwapContext) {
	DRIDrvMsg(pScreen->myNum, X_ERROR,
		  "[DRI] DDX driver missing context swap call back\n");
	return;
    }

    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {

        /* only 3D contexts are swapped in this case */
	if (oldContext) {
	    oldContextStore     = DRIGetContextStore(oldContext);
	    oldContext->valid3D = TRUE;
	    oldContextType      = DRI_3D_CONTEXT;
	} else {
	    oldContextType      = DRI_NO_CONTEXT;
	}
	newContextStore = DRIGetContextStore(newContext);
	if ((newContext->valid3D) &&
	  (newContext->hwContext != pDRIPriv->myContext)) {
	    newContextType = DRI_3D_CONTEXT;
	}
	else {
	    newContextType = DRI_2D_CONTEXT;
	}
	syncType = DRI_3D_SYNC;
    }
    else /* default: driverSwapMethod == DRI_SERVER_SWAP */ {

        /* optimize 2D context swaps */

	if (newContext->flags & DRI_CONTEXT_2DONLY) {
	    /* go from 3D context to 2D context and only save 2D
             * subset of 3D state
             */
	    oldContextStore = DRIGetContextStore(oldContext);
	    oldContextType = DRI_2D_CONTEXT;
	    newContextStore = DRIGetContextStore(newContext);
	    newContextType = DRI_2D_CONTEXT;
	    syncType = DRI_3D_SYNC;
	    pDRIPriv->lastPartial3DContext = oldContext;
	}
	else if (oldContext->flags & DRI_CONTEXT_2DONLY) {
	    if (pDRIPriv->lastPartial3DContext == newContext) {
		/* go from 2D context back to previous 3D context and
		 * only restore 2D subset of previous 3D state
		 */
		oldContextStore = DRIGetContextStore(oldContext);
		oldContextType = DRI_2D_CONTEXT;
		newContextStore = DRIGetContextStore(newContext);
		newContextType = DRI_2D_CONTEXT;
		syncType = DRI_2D_SYNC;
	    }
	    else {
		/* go from 2D context to a different 3D context */

		/* call DDX driver to do partial restore */
		oldContextStore = DRIGetContextStore(oldContext);
		newContextStore =
			DRIGetContextStore(pDRIPriv->lastPartial3DContext);
		(*pDRIPriv->pDriverInfo->SwapContext)(pScreen,
						      DRI_2D_SYNC,
						      DRI_2D_CONTEXT,
						      oldContextStore,
						      DRI_2D_CONTEXT,
						      newContextStore);

		/* now setup for a complete 3D swap */
		oldContextStore = newContextStore;
		oldContext->valid3D = TRUE;
		oldContextType = DRI_3D_CONTEXT;
		newContextStore = DRIGetContextStore(newContext);
		if ((newContext->valid3D) &&
		  (newContext->hwContext != pDRIPriv->myContext)) {
		    newContextType = DRI_3D_CONTEXT;
		}
		else {
		    newContextType = DRI_2D_CONTEXT;
		}
		syncType = DRI_NO_SYNC;
	    }
	}
	else {
	    /* now setup for a complete 3D swap */
	    oldContextStore = newContextStore;
	    oldContext->valid3D = TRUE;
	    oldContextType = DRI_3D_CONTEXT;
	    newContextStore = DRIGetContextStore(newContext);
	    if ((newContext->valid3D) &&
	      (newContext->hwContext != pDRIPriv->myContext)) {
		newContextType = DRI_3D_CONTEXT;
	    }
	    else {
		newContextType = DRI_2D_CONTEXT;
	    }
	    syncType = DRI_3D_SYNC;
	}
    }

    /* call DDX driver to perform the swap */
    (*pDRIPriv->pDriverInfo->SwapContext)(pScreen,
					  syncType,
					  oldContextType,
					  oldContextStore,
					  newContextType,
					  newContextStore);
}

void* 
DRIGetContextStore(DRIContextPrivPtr context)
{
    return((void *)context->pContextStore);
}

void
DRIWindowExposures(WindowPtr pWin, RegionPtr prgn, RegionPtr bsreg)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

    if(pDRIDrawablePriv) {
         (*pDRIPriv->pDriverInfo->InitBuffers)(pWin, prgn,
                                               pDRIDrawablePriv->drawableIndex);
    }

    /* call lower wrapped functions */
    if (pDRIPriv && pDRIPriv->wrap.WindowExposures) {

	/* unwrap */
	pScreen->WindowExposures = pDRIPriv->wrap.WindowExposures;

	/* call lower layers */
	(*pScreen->WindowExposures)(pWin, prgn, bsreg);

	/* rewrap */
	pDRIPriv->wrap.WindowExposures = pScreen->WindowExposures;
	pScreen->WindowExposures = DRIWindowExposures;
    }
}


static int
DRITreeTraversal(WindowPtr pWin, pointer data)
{
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);

    if(pDRIDrawablePriv) {
        ScreenPtr pScreen = pWin->drawable.pScreen;
        DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
        RegionPtr reg = (RegionPtr)data;

        REGION_UNION(pScreen, reg, reg, &(pWin->clipList));

        if(pDRIPriv->nrWindows == 1)
	   return WT_STOPWALKING;
    }
    return WT_WALKCHILDREN;
}

void
DRICopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if(!pDRIPriv) return;

    if(pDRIPriv->nrWindows > 0) {
       RegionRec reg;

       REGION_INIT(pScreen, &reg, NullBox, 0);
       TraverseTree(pWin, DRITreeTraversal, (pointer)(&reg));

       if(REGION_NOTEMPTY(pScreen, &reg)) {
           REGION_TRANSLATE(pScreen, &reg, ptOldOrg.x - pWin->drawable.x,  
                                        ptOldOrg.y - pWin->drawable.y);
           REGION_INTERSECT(pScreen, &reg, &reg, prgnSrc);

           /* The MoveBuffers interface is not ideal */
           (*pDRIPriv->pDriverInfo->MoveBuffers)(pWin, ptOldOrg, &reg,
				pDRIPriv->pDriverInfo->ddxDrawableTableEntry);
       }

       REGION_UNINIT(pScreen, &reg);
    }

    /* call lower wrapped functions */
    if(pDRIPriv->wrap.CopyWindow) {
	/* unwrap */
	pScreen->CopyWindow = pDRIPriv->wrap.CopyWindow;

	/* call lower layers */
	(*pScreen->CopyWindow)(pWin, ptOldOrg, prgnSrc);

	/* rewrap */
	pDRIPriv->wrap.CopyWindow = pScreen->CopyWindow;
	pScreen->CopyWindow = DRICopyWindow;
    }
}

static void
DRIGetSecs(long *secs, long *usecs)
{
#ifdef XFree86LOADER
    getsecs(secs,usecs);
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);

    *secs  = tv.tv_sec;
    *usecs = tv.tv_usec;
#endif
}

static unsigned long
DRIComputeMilliSeconds(unsigned long s_secs, unsigned long s_usecs,
		       unsigned long f_secs, unsigned long f_usecs)
{
    if (f_usecs < s_usecs) {
	--f_secs;
	f_usecs += 1000000;
    }
    return (f_secs - s_secs) * 1000 + (f_usecs - s_usecs) / 1000;
}

static void
DRISpinLockTimeout(drmLock *lock, int val, unsigned long timeout /* in mS */)
{
    int  count = 10000;
#if !defined(__alpha__) && !defined(__powerpc__)
    char ret;
#else
    int ret;
#endif
    long s_secs, s_usecs;
    long f_secs, f_usecs;
    long msecs;
    long prev  = 0;

    DRIGetSecs(&s_secs, &s_usecs);

    do {
	DRM_SPINLOCK_COUNT(lock, val, count, ret);
	if (!ret) return;	/* Got lock */
	DRIGetSecs(&f_secs, &f_usecs);
	msecs = DRIComputeMilliSeconds(s_secs, s_usecs, f_secs, f_usecs);
	if (msecs - prev < 250) count *= 2; /* Not more than 0.5S */
    } while (msecs < timeout);

				/* Didn't get lock, so take it.  The worst
                                   that can happen is that there is some
                                   garbage written to the wrong part of the
                                   framebuffer that a refresh will repair.
                                   That's undesirable, but better than
                                   locking the server.  This should be a
                                   very rare event. */
    DRM_SPINLOCK_TAKE(lock, val);
}

static void
DRILockTree(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    if(!pDRIPriv) return;

    /* Restore the last known 3D context if the X context is hidden */
    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
	(*pDRIPriv->pDriverInfo->SwapContext)(pScreen,
					      DRI_2D_SYNC,
					      DRI_NO_CONTEXT,
					      NULL,
					      DRI_2D_CONTEXT,
					      pDRIPriv->partial3DContextStore);
    }

    /* Call kernel to release lock */
    DRIUnlock(pScreen);

    /* Grab drawable spin lock: a time out between 10 and 30 seconds is
       appropriate, since this should never time out except in the case of
       client death while the lock is being held.  The timeout must be
       greater than any reasonable rendering time. */
    DRISpinLockTimeout(&pDRIPriv->pSAREA->drawable_lock, 1, 10000); /*10 secs*/

    /* Call kernel flush outstanding buffers and relock */
    DRILock(pScreen, DRM_LOCK_QUIESCENT|DRM_LOCK_FLUSH_ALL);

    /* Switch back to our 2D context if the X context is hidden */
    if (pDRIPriv->pDriverInfo->driverSwapMethod == DRI_HIDE_X_CONTEXT) {
	/* hide X context by swapping 2D component here */
	(*pDRIPriv->pDriverInfo->SwapContext)(pScreen,
					      DRI_3D_SYNC,
					      DRI_2D_CONTEXT,
					      pDRIPriv->partial3DContextStore,
					      DRI_2D_CONTEXT,
					      pDRIPriv->hiddenContextStore);
    }
}

/* It appears that somebody is relying on the lock being set even 
   if we aren't touching 3D windows */ 

#define DRI_BROKEN

static Bool DRIWindowsTouched = FALSE;

int
DRIValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{
    ScreenPtr pScreen = pParent->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    int returnValue = 1; /* always return 1, not checked by dix/window.c */

    if(!pDRIPriv) return returnValue;

    DRIWindowsTouched = FALSE;

#ifdef DRI_BROKEN
    if(!DRIWindowsTouched) {
        DRILockTree(pScreen);
        DRIWindowsTouched = TRUE;
    }
#endif

    /* call lower wrapped functions */
    if(pDRIPriv->wrap.ValidateTree) {
	/* unwrap */
	pScreen->ValidateTree = pDRIPriv->wrap.ValidateTree;

	/* call lower layers */
	returnValue = (*pScreen->ValidateTree)(pParent, pChild, kind);

	/* rewrap */
	pDRIPriv->wrap.ValidateTree = pScreen->ValidateTree;
	pScreen->ValidateTree = DRIValidateTree;
    }

    return returnValue;
}

void
DRIPostValidateTree(WindowPtr pParent, WindowPtr pChild, VTKind kind)
{
    ScreenPtr pScreen;
    DRIScreenPrivPtr pDRIPriv;

    if (pParent) {
	pScreen = pParent->drawable.pScreen;
    } else {
	pScreen = pChild->drawable.pScreen;
    }
    if(!(pDRIPriv = DRI_SCREEN_PRIV(pScreen))) return;

    if (pDRIPriv->wrap.PostValidateTree) {
	/* unwrap */
	pScreen->PostValidateTree = pDRIPriv->wrap.PostValidateTree;

	/* call lower layers */
	(*pScreen->PostValidateTree)(pParent, pChild, kind);

	/* rewrap */
	pDRIPriv->wrap.PostValidateTree = pScreen->PostValidateTree;
	pScreen->PostValidateTree = DRIPostValidateTree;
    }

    if (DRIWindowsTouched) {
	/* Release spin lock */
	DRM_SPINUNLOCK(&pDRIPriv->pSAREA->drawable_lock, 1);
        DRIWindowsTouched = FALSE;
    }
}

void
DRIClipNotify(WindowPtr pWin, int dx, int dy)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr	pDRIDrawablePriv;

    if(!pDRIPriv) return;

    if ((pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin))) {

#ifndef DRI_BROKEN
        if(!DRIWindowsTouched) {
            DRILockTree(pScreen);
            DRIWindowsTouched = TRUE;
        }
#endif

	pDRIPriv->pSAREA->drawableTable[pDRIDrawablePriv->drawableIndex].stamp
	    = DRIDrawableValidationStamp++;
    }

    /* call lower wrapped functions */
    if(pDRIPriv->wrap.ClipNotify) {

	/* unwrap */
        pScreen->ClipNotify = pDRIPriv->wrap.ClipNotify;

	/* call lower layers */
        (*pScreen->ClipNotify)(pWin, dx, dy);

	/* rewrap */
        pDRIPriv->wrap.ClipNotify = pScreen->ClipNotify;
        pScreen->ClipNotify = DRIClipNotify;
    }
}

CARD32
DRIGetDrawableIndex(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    DRIDrawablePrivPtr pDRIDrawablePriv = DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin);
    CARD32 index;

    if (pDRIDrawablePriv) {
	index = pDRIDrawablePriv->drawableIndex;
    }
    else {
	index = pDRIPriv->pDriverInfo->ddxDrawableTableEntry;
    }

    return index;
}

unsigned int
DRIGetDrawableStamp(ScreenPtr pScreen, CARD32 drawable_index)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    return pDRIPriv->pSAREA->drawableTable[drawable_index].stamp;
}


void
DRIPrintDrawableLock(ScreenPtr pScreen, char *msg)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);

    ErrorF("%s: %d\n", msg,  pDRIPriv->pSAREA->drawable_lock.lock);
}

void
DRILock(ScreenPtr pScreen, int flags)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    if(!pDRIPriv) return;

    if (!lockRefCount)
        DRM_LOCK(pDRIPriv->drmFD, pDRIPriv->pSAREA, pDRIPriv->myContext, flags);
    lockRefCount++;
}

void
DRIUnlock(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    if(!pDRIPriv) return;

    if (lockRefCount > 0) {
        lockRefCount--;
    }
    else {
        ErrorF("DRIUnlock called when not locked\n");
        return;
    }
    if (!lockRefCount)
        DRM_UNLOCK(pDRIPriv->drmFD, pDRIPriv->pSAREA, pDRIPriv->myContext);
}

void *
DRIGetSAREAPrivate(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    if (!pDRIPriv) return 0;

    return (void *)(((char*)pDRIPriv->pSAREA)+sizeof(XF86DRISAREARec));
}

drmContext
DRIGetContext(ScreenPtr pScreen)
{
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    if (!pDRIPriv) return 0;

    return pDRIPriv->myContext;
}

/* This lets get at the unwrapped functions so that they can correctly
 * call the lowerlevel functions, and choose whether they will be
 * called at every level of recursion (eg in validatetree).
 */
DRIWrappedFuncsRec *
DRIGetWrappedFuncs(ScreenPtr pScreen)
{
    return &(DRI_SCREEN_PRIV(pScreen)->wrap);
}

void
DRIQueryVersion(int *majorVersion,
                int *minorVersion,
                int *patchVersion)
{
    *majorVersion = XF86DRI_MAJOR_VERSION;
    *minorVersion = XF86DRI_MINOR_VERSION;
    *patchVersion = XF86DRI_PATCH_VERSION;
}

static void
_DRIAdjustFrame(ScrnInfoPtr pScrn, DRIScreenPrivPtr pDRIPriv, int x, int y)
{
    pDRIPriv->pSAREA->frame.x      = x;
    pDRIPriv->pSAREA->frame.y      = y;
    pDRIPriv->pSAREA->frame.width  = pScrn->frameX1 - x + 1;
    pDRIPriv->pSAREA->frame.height = pScrn->frameY1 - y + 1;
}

void
DRIAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    ScreenPtr        pScreen  = screenInfo.screens[scrnIndex];
    DRIScreenPrivPtr pDRIPriv = DRI_SCREEN_PRIV(pScreen);
    ScrnInfoPtr      pScrn    = xf86Screens[pScreen->myNum];
    int              px, py;

    if (!pDRIPriv || !pDRIPriv->pSAREA) {
	DRIDrvMsg(scrnIndex, X_ERROR, "[DRI] No SAREA (%p %p)\n",
		  pDRIPriv, pDRIPriv ? pDRIPriv->pSAREA : NULL);
	return;
    }

    if (pDRIPriv->fullscreen) {
				/* Fix up frame */
	pScrn->frameX0 = pDRIPriv->pSAREA->frame.x;
	pScrn->frameY0 = pDRIPriv->pSAREA->frame.y;
	pScrn->frameX1 = pScrn->frameX0 + pDRIPriv->pSAREA->frame.width - 1;
	pScrn->frameY1 = pScrn->frameY0 + pDRIPriv->pSAREA->frame.height - 1;

				/* Fix up cursor */
	miPointerPosition(&px, &py);
	if (px < pScrn->frameX0) px = pScrn->frameX0;
	if (px > pScrn->frameX1) px = pScrn->frameX1;
	if (py < pScrn->frameY0) py = pScrn->frameY0;
	if (py > pScrn->frameY1) py = pScrn->frameY1;
	pScreen->SetCursorPosition(pScreen, px, py, TRUE);
	return;
    }

    if (pDRIPriv->wrap.AdjustFrame) {
	/* unwrap */
	pScrn->AdjustFrame = pDRIPriv->wrap.AdjustFrame;
	/* call lower layers */
	(*pScrn->AdjustFrame)(scrnIndex, x, y, flags);
	/* rewrap */
	pDRIPriv->wrap.AdjustFrame = pScrn->AdjustFrame;
	pScrn->AdjustFrame         = DRIAdjustFrame;
    }

    _DRIAdjustFrame(pScrn, pDRIPriv, x, y);
}

/* WARNING WARNING WARNING: Just like every other function call in this
   file, the DRIOpenFullScreen and DRICloseFullScreen calls are for
   internal use only!  They should be used only by GLX internals and
   should NEVER be called from a GL application.

   Some time in the future, there will be a (proposed) standard GLX
   extension that performs expanded functionality, that is designed for
   used by application-level programs, and that should be portable
   across multiple GLX implementations. */
Bool
DRIOpenFullScreen(ScreenPtr pScreen, DrawablePtr pDrawable)
{
    DRIScreenPrivPtr   pDRIPriv    = DRI_SCREEN_PRIV(pScreen);
    ScrnInfoPtr        pScrn       = xf86Screens[pScreen->myNum];
    WindowPtr	       pWin        = (WindowPtr)pDrawable;
    XF86DRIClipRectPtr pClipRects  = (void *)REGION_RECTS(&pWin->clipList);

    _DRIAdjustFrame(pScrn, pDRIPriv, pScrn->frameX0, pScrn->frameY0);

    if (pDrawable->type != DRAWABLE_WINDOW) return FALSE;

    if (!pScrn->vtSema) return FALSE; /* switched away */

    if (pDrawable->x != pScrn->frameX0
	|| pDrawable->y != pScrn->frameY0
	|| pDrawable->width != pScrn->frameX1 - pScrn->frameX0 + 1
	|| pDrawable->height != pScrn->frameY1 - pScrn->frameY0 + 1) {
	return FALSE;
    }

    if (REGION_NUM_RECTS(&pWin->clipList) != 1) return FALSE;
    if (pDrawable->x != pClipRects[0].x1
	|| pDrawable->y != pClipRects[0].y1
	|| pDrawable->width != pClipRects[0].x2 - pClipRects[0].x1
	|| pDrawable->height != pClipRects[0].y2 - pClipRects[0].y1) {
	return FALSE;
    }

    AddResource(pDrawable->id, DRIFullScreenResType, (pointer)pWin);

    xf86EnableVTSwitch(FALSE);
    pScrn->EnableDisableFBAccess(pScreen->myNum, FALSE);
    pScrn->vtSema        = FALSE;
    pDRIPriv->fullscreen = pDrawable;
    DRIClipNotify(pWin, 0, 0);

    if (pDRIPriv->pDriverInfo->OpenFullScreen)
	pDRIPriv->pDriverInfo->OpenFullScreen(pScreen);

    pDRIPriv->pSAREA->frame.fullscreen = 1;
    return TRUE;
}

static Bool
_DRICloseFullScreen(pointer pResource, XID id)
{
    DrawablePtr      pDrawable = (DrawablePtr)pResource;
    ScreenPtr        pScreen   = pDrawable->pScreen;
    DRIScreenPrivPtr pDRIPriv  = DRI_SCREEN_PRIV(pScreen);
    ScrnInfoPtr      pScrn     = xf86Screens[pScreen->myNum];
    WindowPtr	     pWin      = (WindowPtr)pDrawable;
    WindowOptPtr     optional  = pWin->optional;
    Mask             mask      = pWin->eventMask;

    if (pDRIPriv->pDriverInfo->CloseFullScreen)
	pDRIPriv->pDriverInfo->CloseFullScreen(pScreen);

    pDRIPriv->fullscreen = NULL;
    pScrn->vtSema        = TRUE;

				/* Turn off expose events for the top window */
    pWin->eventMask &= ~ExposureMask;
    pWin->optional   = NULL;
    pScrn->EnableDisableFBAccess(pScreen->myNum, TRUE);
    pWin->eventMask  = mask;
    pWin->optional   = optional;

    xf86EnableVTSwitch(TRUE);
    pDRIPriv->pSAREA->frame.fullscreen = 0;
    return TRUE;
}

Bool
DRICloseFullScreen(ScreenPtr pScreen, DrawablePtr pDrawable)
{
    FreeResourceByType(pDrawable->id, DRIFullScreenResType, FALSE);
    return TRUE;
}


/* 
 * DRIMoveBuffersHelper swaps the regions rects in place leaving you
 * a region with the rects in the order that you need to blit them,
 * but it is possibly (likely) an invalid region afterwards.  If you
 * need to use the region again for anything you have to call 
 * REGION_VALIDATE on it, or better yet, save a copy first.
 */

void
DRIMoveBuffersHelper(
   ScreenPtr pScreen, 
   int dx,
   int dy,
   int *xdir, 
   int *ydir, 
   RegionPtr reg
)
{
   BoxPtr extents, pbox, firstBox, lastBox;
   BoxRec tmpBox;
   int y, nbox;

   extents = REGION_EXTENTS(pScreen, reg);
   nbox = REGION_NUM_RECTS(reg);
   pbox = REGION_RECTS(reg);

   if((dy > 0) && (dy < (extents->y2 - extents->y1))) {
     *ydir = -1;
     if(nbox > 1) {
        firstBox = pbox;
        lastBox = pbox + nbox - 1;
        while((unsigned long)firstBox < (unsigned long)lastBox) {
           tmpBox = *firstBox;
           *firstBox = *lastBox;
           *lastBox = tmpBox;
           firstBox++;
           lastBox--;
        }
     }
   } else *ydir = 1;

   if((dx > 0) && (dx < (extents->x2 - extents->x1))) {
     *xdir = -1;
     if(nbox > 1) {
        firstBox = lastBox = pbox;
        y = pbox->y1;
        while(--nbox) {
           pbox++;
           if(pbox->y1 == y) lastBox++;
           else {
              while((unsigned long)firstBox < (unsigned long)lastBox) {
                 tmpBox = *firstBox;
                 *firstBox = *lastBox;
                 *lastBox = tmpBox;
                 firstBox++;
                 lastBox--;
              }

              firstBox = lastBox = pbox;
              y = pbox->y1;
           }
         }
         while((unsigned long)firstBox < (unsigned long)lastBox) {
           tmpBox = *firstBox;
           *firstBox = *lastBox;
           *lastBox = tmpBox;
           firstBox++;
           lastBox--;
        }
     }
   } else *xdir = 1;

}
