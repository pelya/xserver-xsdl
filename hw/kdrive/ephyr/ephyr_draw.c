/*
 * Copyright © 2006 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif

#include "ephyr.h"
#include "exa_priv.h"
#include "fbpict.h"

#define EPHYR_TRACE_DRAW 0

#if EPHYR_TRACE_DRAW
#define TRACE_DRAW() ErrorF("%s\n", __FUNCTION__);
#else
#define TRACE_DRAW() do { } while (0)
#endif

/* Use some oddball alignments, to expose issues in alignment handling in EXA. */
#define EPHYR_OFFSET_ALIGN	11
#define EPHYR_PITCH_ALIGN	9

#define EPHYR_OFFSCREEN_SIZE	(16 * 1024 * 1024)
#define EPHYR_OFFSCREEN_BASE	(1 * 1024 * 1024)

/**
 * Sets up a scratch GC for fbFill, and saves other parameters for the
 * ephyrSolid implementation.
 */
static Bool
ephyrPrepareSolid(PixmapPtr pPix, int alu, Pixel pm, Pixel fg)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;
    CARD32 tmpval[3];

    fakexa->pDst = pPix;
    fakexa->pGC = GetScratchGC(pPix->drawable.depth, pScreen);

    tmpval[0] = alu;
    tmpval[1] = pm;
    tmpval[2] = fg;
    ChangeGC(fakexa->pGC, GCFunction | GCPlaneMask | GCForeground, 
	     tmpval);

    ValidateGC(&pPix->drawable, fakexa->pGC);

    TRACE_DRAW();

    return TRUE;
}

/**
 * Does an fbFill of the rectangle to be drawn.
 */
static void
ephyrSolid(PixmapPtr pPix, int x1, int y1, int x2, int y2)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fbFill(&fakexa->pDst->drawable, fakexa->pGC, x1, y1, x2 - x1, y2 - y1);
}

/**
 * Cleans up the scratch GC created in ephyrPrepareSolid.
 */
static void
ephyrDoneSolid(PixmapPtr pPix)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    FreeScratchGC(fakexa->pGC);
}

/**
 * Sets up a scratch GC for fbCopyArea, and saves other parameters for the
 * ephyrCopy implementation.
 */
static Bool
ephyrPrepareCopy(PixmapPtr pSrc, PixmapPtr pDst, int dx, int dy, int alu,
		 Pixel pm)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;
    CARD32 tmpval[2];

    fakexa->pSrc = pSrc;
    fakexa->pDst = pDst;
    fakexa->pGC = GetScratchGC(pDst->drawable.depth, pScreen);

    tmpval[0] = alu;
    tmpval[1] = pm;
    ChangeGC (fakexa->pGC, GCFunction | GCPlaneMask, tmpval);

    ValidateGC(&pDst->drawable, fakexa->pGC);

    TRACE_DRAW();

    return TRUE;
}

/**
 * Does an fbCopyArea to take care of the requested copy.
 */
static void
ephyrCopy(PixmapPtr pDst, int srcX, int srcY, int dstX, int dstY, int w, int h)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fbCopyArea(&fakexa->pSrc->drawable, &fakexa->pDst->drawable, fakexa->pGC,
	       srcX, srcY, w, h, dstX, dstY);
}

/**
 * Cleans up the scratch GC created in ephyrPrepareCopy.
 */
static void
ephyrDoneCopy(PixmapPtr pDst)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    FreeScratchGC (fakexa->pGC);
}

/**
 * Reports that we can always accelerate the given operation.  This may not be
 * desirable from an EXA testing standpoint -- testing the fallback paths would
 * be useful, too.
 */
static Bool
ephyrCheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		    PicturePtr pDstPicture)
{
    return TRUE;
}

/**
 * Saves off the parameters for ephyrComposite.
 */
static Bool
ephyrPrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		      PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask,
		      PixmapPtr pDst)
{
    KdScreenPriv(pDst->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    op = op;
    fakexa->pSrcPicture = pSrcPicture;
    fakexa->pMaskPicture = pMaskPicture;
    fakexa->pDstPicture = pDstPicture;

    TRACE_DRAW();

    return TRUE;
}

/**
 * Does an fbComposite to complete the requested drawing operation.
 */
static void
ephyrComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
	       int dstX, int dstY, int w, int h)
{
    KdScreenPriv(pDst->drawable.pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fbComposite(fakexa->op, fakexa->pSrcPicture, fakexa->pMaskPicture,
		fakexa->pDstPicture, srcX, srcY, maskX, maskY, dstX, dstY,
		w, h);
}

static void
ephyrDoneComposite(PixmapPtr pDst)
{
}

/**
 * In fakexa, we currently only track whether we have synced to the latest
 * "accelerated" drawing that has happened or not.  This will be used by an
 * ephyrPrepareAccess for the purpose of reliably providing garbage when
 * reading/writing when we haven't synced.
 */
static int
ephyrMarkSync(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fakexa->is_synced = FALSE;

    return 0;
}

/**
 * Assumes that we're waiting on the latest marker.  When EXA gets smarter and
 * starts using markers in a fine-grained way (for example, waiting on drawing
 * to required pixmaps to complete, rather than waiting for all drawing to
 * complete), we'll want to make the ephyrMarkSync/ephyrWaitMarker
 * implementation fine-grained as well.
 */
static void
ephyrWaitMarker(ScreenPtr pScreen, int marker)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa = scrpriv->fakexa;

    fakexa->is_synced = TRUE;
}

/**
 * This function initializes EXA to use the fake acceleration implementation
 * which just falls through to software.  The purpose is to have a reliable,
 * correct driver with which to test changes to the EXA core.
 */
Bool
ephyrDrawInit(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    EphyrScrPriv *scrpriv = screen->driver;
    EphyrFakexaPriv *fakexa;
    Bool success;

    fakexa = xcalloc(1, sizeof(*fakexa));
    if (fakexa == NULL)
	return FALSE;

#if 0
    /* Currently, EXA isn't ready for what we want to do here.  We want one
     * pointer to the framebuffer (which is set in exaMapFramebuffer) to be
     * considered "in framebuffer", and a separate pointer to offscreen memory,
     * which is also considered to be in framebuffer.  The alternative would be
     * to extend the XImage data area set up in hostx.c from exaMapFramebuffer,
     * but that may be complicated.
     */
    fakexa->exa.card.memoryBase = xalloc(EPHYR_OFFSCREEN_SIZE);
    if (fakexa->exa.card.memoryBase == NULL) {
	xfree(fakexa);
	return FALSE;
    }
    fakexa->exa.card.memorySize = EPHYR_OFFSCREEN_SIZE;
    fakexa->exa.card.offScreenBase = EPHYR_OFFSCREEN_BASE;
#else
    /* Tell EXA that there's a single framebuffer area, which happens to cover
     * exactly what the front buffer is.
     */
    fakexa->exa.card.memoryBase = screen->memory_base;
    fakexa->exa.card.memorySize = screen->off_screen_base;
    fakexa->exa.card.offScreenBase = screen->off_screen_base;
#endif

    fakexa->exa.accel.PrepareSolid = ephyrPrepareSolid;
    fakexa->exa.accel.Solid = ephyrSolid;
    fakexa->exa.accel.DoneSolid = ephyrDoneSolid;

    fakexa->exa.accel.PrepareCopy = ephyrPrepareCopy;
    fakexa->exa.accel.Copy = ephyrCopy;
    fakexa->exa.accel.DoneCopy = ephyrDoneCopy;

    fakexa->exa.accel.CheckComposite = ephyrCheckComposite;
    fakexa->exa.accel.PrepareComposite = ephyrPrepareComposite;
    fakexa->exa.accel.Composite = ephyrComposite;
    fakexa->exa.accel.DoneComposite = ephyrDoneComposite;

    fakexa->exa.accel.MarkSync = ephyrMarkSync;
    fakexa->exa.accel.WaitMarker = ephyrWaitMarker;

    fakexa->exa.card.pixmapOffsetAlign = EPHYR_OFFSET_ALIGN;
    fakexa->exa.card.pixmapPitchAlign = EPHYR_PITCH_ALIGN;

    fakexa->exa.card.maxX = 1023;
    fakexa->exa.card.maxY = 1023;

    fakexa->exa.card.flags = EXA_OFFSCREEN_PIXMAPS;

    success = exaDriverInit(pScreen, &fakexa->exa);
    if (success) {
	ErrorF("Initialized fake EXA acceleration\n");
	scrpriv->fakexa = fakexa;
    } else {
	ErrorF("Failed to initialize EXA\n");
	xfree(fakexa->exa.card.memoryBase);
	xfree(fakexa);
    }

    return success;
}

void
ephyrDrawEnable(ScreenPtr pScreen)
{
}

void
ephyrDrawDisable(ScreenPtr pScreen)
{
}

void
ephyrDrawFini(ScreenPtr pScreen)
{
}

/**
 * exaDDXDriverInit is required by the top-level EXA module, and is used by
 * the xorg DDX to hook in its EnableDisableFB wrapper.  We don't need it, since
 * we won't be enabling/disabling the FB.
 */
void
exaDDXDriverInit(ScreenPtr pScreen)
{
}
