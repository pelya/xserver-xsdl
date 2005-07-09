/*
 *
 * Copyright (C) 2000 Keith Packard, member of The XFree86 Project, Inc.
 *               2005 Zack Rusin, Trolltech
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef EXAPRIV_H
#define EXAPRIV_H

#include "exa.h"

#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mibstore.h"
#include "colormapst.h"
#include "gcstruct.h"
#include "input.h"
#include "mipointer.h"
#include "mi.h"
#include "dix.h"
#include "fb.h"
#include "fboverlay.h"
#ifdef RENDER
#include "fbpict.h"
#endif

#ifndef EXA_MAX_FB
#define EXA_MAX_FB   FB_OVERLAY_MAX
#endif

typedef void (*EnableDisableFBAccessProcPtr)(int, Bool);
typedef struct {
    ExaDriverPtr info;
    CreateGCProcPtr 		 SavedCreateGC;
    CloseScreenProcPtr 		 SavedCloseScreen;
    GetImageProcPtr 		 SavedGetImage;
    GetSpansProcPtr 		 SavedGetSpans;
    PaintWindowBackgroundProcPtr SavedPaintWindowBackground;
    CreatePixmapProcPtr 	 SavedCreatePixmap;
    DestroyPixmapProcPtr 	 SavedDestroyPixmap;
    PaintWindowBorderProcPtr 	 SavedPaintWindowBorder;
    CopyWindowProcPtr 		 SavedCopyWindow;
#ifdef RENDER
    CompositeProcPtr             SavedComposite;
#endif
    EnableDisableFBAccessProcPtr SavedEnableDisableFBAccess;
    Bool			 wrappedEnableDisableFB;
    Bool			 swappedOut;
} ExaScreenPrivRec, *ExaScreenPrivPtr;

/*
 * This is the only completely portable way to
 * compute this info.
 */
#ifndef BitsPerPixel
#define BitsPerPixel(d) (\
    PixmapWidthPaddingInfo[d].notPower2 ? \
    (PixmapWidthPaddingInfo[d].bytesPerPixel * 8) : \
    ((1 << PixmapWidthPaddingInfo[d].padBytesLog2) * 8 / \
    (PixmapWidthPaddingInfo[d].padRoundUp+1)))
#endif

extern int exaScreenPrivateIndex;
extern int exaPixmapPrivateIndex;
#define ExaGetScreenPriv(s)	((ExaScreenPrivPtr)(s)->devPrivates[exaScreenPrivateIndex].ptr)
#define ExaScreenPriv(s)	ExaScreenPrivPtr    pExaScr = ExaGetScreenPriv(s)

#define ExaGetPixmapPriv(p)	((ExaPixmapPrivPtr)(p)->devPrivates[exaPixmapPrivateIndex].ptr)
#define ExaSetPixmapPriv(p,a)	((p)->devPrivates[exaPixmapPrivateIndex].ptr = (pointer) (a))
#define ExaPixmapPriv(p)	ExaPixmapPrivPtr pExaPixmap = ExaGetPixmapPriv(p)

typedef struct {
    ExaOffscreenArea *area;
    int		    score;
    int		    devKind;
    DevUnion	    devPrivate;
    Bool	    dirty;
} ExaPixmapPrivRec, *ExaPixmapPrivPtr;


/* exaasync.c */
void
ExaCheckFillSpans  (DrawablePtr pDrawable, GCPtr pGC, int nspans,
		   DDXPointPtr ppt, int *pwidth, int fSorted);

void
ExaCheckSetSpans (DrawablePtr pDrawable, GCPtr pGC, char *psrc,
		 DDXPointPtr ppt, int *pwidth, int nspans, int fSorted);

void
ExaCheckPutImage (DrawablePtr pDrawable, GCPtr pGC, int depth,
		 int x, int y, int w, int h, int leftPad, int format,
		 char *bits);

RegionPtr
ExaCheckCopyArea (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		 int srcx, int srcy, int w, int h, int dstx, int dsty);

RegionPtr
ExaCheckCopyPlane (DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		  int srcx, int srcy, int w, int h, int dstx, int dsty,
		  unsigned long bitPlane);

void
ExaCheckPolyPoint (DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr pptInit);

void
ExaCheckPolylines (DrawablePtr pDrawable, GCPtr pGC,
		  int mode, int npt, DDXPointPtr ppt);

void
ExaCheckPolySegment (DrawablePtr pDrawable, GCPtr pGC,
		    int nsegInit, xSegment *pSegInit);

void
ExaCheckPolyRectangle (DrawablePtr pDrawable, GCPtr pGC,
		      int nrects, xRectangle *prect);

void
ExaCheckPolyArc (DrawablePtr pDrawable, GCPtr pGC,
		int narcs, xArc *pArcs);

#define ExaCheckFillPolygon	miFillPolygon

void
ExaCheckPolyFillRect (DrawablePtr pDrawable, GCPtr pGC,
		     int nrect, xRectangle *prect);

void
ExaCheckPolyFillArc (DrawablePtr pDrawable, GCPtr pGC,
		    int narcs, xArc *pArcs);

void
ExaCheckImageGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		      int x, int y, unsigned int nglyph,
		      CharInfoPtr *ppci, pointer pglyphBase);

void
ExaCheckPolyGlyphBlt (DrawablePtr pDrawable, GCPtr pGC,
		     int x, int y, unsigned int nglyph,
		     CharInfoPtr *ppci, pointer pglyphBase);

void
ExaCheckPushPixels (GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable,
		   int w, int h, int x, int y);

void
ExaCheckGetImage (DrawablePtr pDrawable,
		 int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask,
		 char *d);

void
ExaCheckGetSpans (DrawablePtr pDrawable,
		 int wMax,
		 DDXPointPtr ppt,
		 int *pwidth,
		 int nspans,
		 char *pdstStart);

void
ExaCheckSaveAreas (PixmapPtr	pPixmap,
		  RegionPtr	prgnSave,
		  int		xorg,
		  int		yorg,
		  WindowPtr	pWin);

void
ExaCheckRestoreAreas (PixmapPtr	pPixmap,
		     RegionPtr	prgnSave,
		     int	xorg,
		     int    	yorg,
		     WindowPtr	pWin);

void
ExaCheckPaintWindow (WindowPtr pWin, RegionPtr pRegion, int what);

extern const GCOps	exaAsyncPixmapGCOps;

#ifdef RENDER
void
ExaCheckComposite (CARD8      op,
		  PicturePtr pSrc,
		  PicturePtr pMask,
		  PicturePtr pDst,
		  INT16      xSrc,
		  INT16      ySrc,
		  INT16      xMask,
		  INT16      yMask,
		  INT16      xDst,
		  INT16      yDst,
		  CARD16     width,
		  CARD16     height);
#endif

/* exaoffscreen.c */
void
ExaOffscreenMarkUsed (PixmapPtr pPixmap);

void
ExaOffscreenSwapOut (ScreenPtr pScreen);

void
ExaOffscreenSwapIn (ScreenPtr pScreen);

void
ExaOffscreenFini (ScreenPtr pScreen);

void
exaEnableDisableFBAccess (int index, Bool enable);

/* exa.c */
void
exaPixmapUseScreen (PixmapPtr pPixmap);

void
exaPixmapUseMemory (PixmapPtr pPixmap);

void
exaDrawableDirty(DrawablePtr pDrawable);

Bool
exaDrawableIsOffscreen (DrawablePtr pDrawable);

Bool
exaPixmapIsOffscreen(PixmapPtr p);

PixmapPtr
exaGetOffscreenPixmap (DrawablePtr pDrawable, int *xp, int *yp);

void
exaMoveInPixmap (PixmapPtr pPixmap);

void
exaCopyNtoN (DrawablePtr    pSrcDrawable,
	     DrawablePtr    pDstDrawable,
	     GCPtr	    pGC,
	     BoxPtr	    pbox,
	     int	    nbox,
	     int	    dx,
	     int	    dy,
	     Bool	    reverse,
	     Bool	    upsidedown,
	     Pixel	    bitplane,
	     void	    *closure);

void
exaComposite(CARD8	op,
	     PicturePtr pSrc,
	     PicturePtr pMask,
	     PicturePtr pDst,
	     INT16	xSrc,
	     INT16	ySrc,
	     INT16	xMask,
	     INT16	yMask,
	     INT16	xDst,
	     INT16	yDst,
	     CARD16	width,
	     CARD16	height);

#endif /* EXAPRIV_H */
