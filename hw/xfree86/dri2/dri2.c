/*
 * Copyright © 2007, 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <xf86drm.h>
#include "xf86Module.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "dri2.h"

#include "xf86.h"

static int dri2ScreenPrivateKeyIndex;
static DevPrivateKey dri2ScreenPrivateKey = &dri2ScreenPrivateKeyIndex;
static int dri2WindowPrivateKeyIndex;
static DevPrivateKey dri2WindowPrivateKey = &dri2WindowPrivateKeyIndex;
static int dri2PixmapPrivateKeyIndex;
static DevPrivateKey dri2PixmapPrivateKey = &dri2PixmapPrivateKeyIndex;

typedef struct _DRI2Drawable {
    unsigned int	 refCount;
    int			 width;
    int			 height;
    DRI2BufferPtr	 buffers;
    int			 bufferCount;
    unsigned int	 pendingSequence;
} DRI2DrawableRec, *DRI2DrawablePtr;

typedef struct _DRI2Screen {
    const char			*driverName;
    const char			*deviceName;
    int				 fd;
    unsigned int		 lastSequence;
    DRI2CreateBuffersProcPtr	 CreateBuffers;
    DRI2DestroyBuffersProcPtr	 DestroyBuffers;
    DRI2CopyRegionProcPtr	 CopyRegion;

    HandleExposuresProcPtr       HandleExposures;
} DRI2ScreenRec, *DRI2ScreenPtr;

static DRI2ScreenPtr
DRI2GetScreen(ScreenPtr pScreen)
{
    return dixLookupPrivate(&pScreen->devPrivates, dri2ScreenPrivateKey);
}

static DRI2DrawablePtr
DRI2GetDrawable(DrawablePtr pDraw)
{
    WindowPtr		  pWin;
    PixmapPtr		  pPixmap;

    if (pDraw->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr) pDraw;
	return dixLookupPrivate(&pWin->devPrivates, dri2WindowPrivateKey);
    }
    else
    {
	pPixmap = (PixmapPtr) pDraw;
	return dixLookupPrivate(&pPixmap->devPrivates, dri2PixmapPrivateKey);
    }
}

int
DRI2CreateDrawable(DrawablePtr pDraw)
{
    WindowPtr	    pWin;
    PixmapPtr	    pPixmap;
    DRI2DrawablePtr pPriv;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv != NULL)
    {
	pPriv->refCount++;
	return Success;
    }

    pPriv = xalloc(sizeof *pPriv);
    if (pPriv == NULL)
	return BadAlloc;

    pPriv->refCount = 1;
    pPriv->width = pDraw->width;
    pPriv->height = pDraw->height;
    pPriv->buffers = NULL;
    pPriv->bufferCount = 0;

    if (pDraw->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr) pDraw;
	dixSetPrivate(&pWin->devPrivates, dri2WindowPrivateKey, pPriv);
    }
    else
    {
	pPixmap = (PixmapPtr) pDraw;
	dixSetPrivate(&pPixmap->devPrivates, dri2PixmapPrivateKey, pPriv);
    }

    return Success;
}

DRI2BufferPtr
DRI2GetBuffers(DrawablePtr pDraw, int *width, int *height,
	       unsigned int *attachments, int count, int *out_count)
{
    DRI2ScreenPtr   ds = DRI2GetScreen(pDraw->pScreen);
    DRI2DrawablePtr pPriv = DRI2GetDrawable(pDraw);
    DRI2BufferPtr   buffers;

    if (pPriv->buffers == NULL ||
	pDraw->width != pPriv->width || pDraw->height != pPriv->height)
    {
	buffers = (*ds->CreateBuffers)(pDraw, attachments, count);
	(*ds->DestroyBuffers)(pDraw, pPriv->buffers, pPriv->bufferCount);
	pPriv->buffers = buffers;
	pPriv->bufferCount = count;
	pPriv->width = pDraw->width;
	pPriv->height = pDraw->height;
    }

    *width = pPriv->width;
    *height = pPriv->height;
    *out_count = pPriv->bufferCount;

    return pPriv->buffers;
}

int
DRI2CopyRegion(DrawablePtr pDraw, RegionPtr pRegion,
	       unsigned int dest, unsigned int src)
{
    DRI2ScreenPtr   ds = DRI2GetScreen(pDraw->pScreen);
    DRI2DrawablePtr pPriv;
    DRI2BufferPtr   pDestBuffer, pSrcBuffer;
    int		    i;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
	return BadDrawable;

    pDestBuffer = NULL;
    pSrcBuffer = NULL;
    for (i = 0; i < pPriv->bufferCount; i++)
    {
	if (pPriv->buffers[i].attachment == dest)
	    pDestBuffer = &pPriv->buffers[i];
	if (pPriv->buffers[i].attachment == src)
	    pSrcBuffer = &pPriv->buffers[i];
    }
    if (pSrcBuffer == NULL || pDestBuffer == NULL)
	return BadValue;
		
    (*ds->CopyRegion)(pDraw, pRegion, pDestBuffer, pSrcBuffer);

    return Success;
}

void
DRI2DestroyDrawable(DrawablePtr pDraw)
{
    DRI2ScreenPtr   ds = DRI2GetScreen(pDraw->pScreen);
    DRI2DrawablePtr pPriv;
    WindowPtr  	    pWin;
    PixmapPtr	    pPixmap;

    pPriv = DRI2GetDrawable(pDraw);
    if (pPriv == NULL)
	return;
    
    pPriv->refCount--;
    if (pPriv->refCount > 0)
	return;

    (*ds->DestroyBuffers)(pDraw, pPriv->buffers, pPriv->bufferCount);
    xfree(pPriv);

    if (pDraw->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr) pDraw;
	dixSetPrivate(&pWin->devPrivates, dri2WindowPrivateKey, NULL);
    }
    else
    {
	pPixmap = (PixmapPtr) pDraw;
	dixSetPrivate(&pPixmap->devPrivates, dri2PixmapPrivateKey, NULL);
    }
}

Bool
DRI2Connect(ScreenPtr pScreen, unsigned int driverType, int *fd,
	    const char **driverName, const char **deviceName)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);

    if (ds == NULL)
	return FALSE;

    if (driverType != DRI2DriverDRI)
	return BadValue;

    *fd = ds->fd;
    *driverName = ds->driverName;
    *deviceName = ds->deviceName;

    return TRUE;
}

Bool
DRI2Authenticate(ScreenPtr pScreen, drm_magic_t magic)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);

    if (ds == NULL || drmAuthMagic(ds->fd, magic))
	return FALSE;

    return TRUE;
}

Bool
DRI2ScreenInit(ScreenPtr pScreen, DRI2InfoPtr info)
{
    DRI2ScreenPtr ds;

    ds = xalloc(sizeof *ds);
    if (!ds)
	return FALSE;

    ds->fd	       = info->fd;
    ds->driverName     = info->driverName;
    ds->deviceName     = info->deviceName;
    ds->CreateBuffers  = info->CreateBuffers;
    ds->DestroyBuffers = info->DestroyBuffers;
    ds->CopyRegion     = info->CopyRegion;

    dixSetPrivate(&pScreen->devPrivates, dri2ScreenPrivateKey, ds);

    xf86DrvMsg(pScreen->myNum, X_INFO, "[DRI2] Setup complete\n");

    return TRUE;
}

void
DRI2CloseScreen(ScreenPtr pScreen)
{
    DRI2ScreenPtr ds = DRI2GetScreen(pScreen);

    xfree(ds);
    dixSetPrivate(&pScreen->devPrivates, dri2ScreenPrivateKey, NULL);
}

extern ExtensionModule dri2ExtensionModule;

static pointer
DRI2Setup(pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone)
    {
	setupDone = TRUE;
	LoadExtension(&dri2ExtensionModule, FALSE);
    }
    else
    {
	if (errmaj)
	    *errmaj = LDR_ONCEONLY;
    }

    return (pointer) 1;
}

static XF86ModuleVersionInfo DRI2VersRec =
{
    "dri2",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_EXTENSION,
    ABI_EXTENSION_VERSION,
    MOD_CLASS_NONE,
    { 0, 0, 0, 0 }
};

_X_EXPORT XF86ModuleData dri2ModuleData = { &DRI2VersRec, DRI2Setup, NULL };

