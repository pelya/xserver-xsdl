/* $XFree86: xc/programs/Xserver/ilbm/ilbm.h,v 3.2 1998/04/05 16:42:23 robin Exp $ */
/* Combined Purdue/PurduePlus patches, level 2.0, 1/17/89 */
/***********************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


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
/* $XConsortium: ilbm.h,v 5.31 94/04/17 20:28:15 dpw Exp $ */
/* Monochrome Frame Buffer definitions
   written by drewry, september 1986
*/

/* Modified jun 95 by Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
   to use interleaved bitplanes instead of normal bitplanes */

#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "miscstruct.h"
#include "mibstore.h"

extern int ilbmInverseAlu[];
extern int ilbmScreenPrivateIndex;
/* warning: PixelType definition duplicated in maskbits.h */
#ifndef PixelType
#define PixelType unsigned long
#endif /* PixelType */

#define AFB_MAX_DEPTH 8

/* ilbmbitblt.c */

extern void ilbmDoBitblt(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
#endif
);

extern RegionPtr ilbmBitBlt(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	GCPtr /*pGC*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*width*/,
	int /*height*/,
	int /*dstx*/,
	int /*dsty*/,
	void (*doBitBlt)(),
	unsigned long /*planemask*/
#endif
);

extern RegionPtr ilbmCopyArea(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrcDrawable*/,
	DrawablePtr /*pDstDrawable*/,
	GCPtr/*pGC*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*width*/,
	int /*height*/,
	int /*dstx*/,
	int /*dsty*/
#endif
);

extern RegionPtr ilbmCopyPlane(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrcDrawable*/,
	DrawablePtr /*pDstDrawable*/,
	GCPtr/*pGC*/,
	int /*srcx*/,
	int /*srcy*/,
	int /*width*/,
	int /*height*/,
	int /*dstx*/,
	int /*dsty*/,
	unsigned long /*plane*/
#endif
);

extern void ilbmCopy1ToN(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmbltC.c */

extern void ilbmDoBitbltCopy(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmbltCI.c */

extern void ilbmDoBitbltCopyInverted(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmbltG.c */

extern void ilbmDoBitbltGeneral(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmbltO.c */

extern void ilbmDoBitbltOr(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmbltX.c */

extern void ilbmDoBitbltXor(
#if NeedFunctionPrototypes
	DrawablePtr /*pSrc*/,
	DrawablePtr /*pDst*/,
	int /*alu*/,
	RegionPtr /*prgnDst*/,
	DDXPointPtr /*pptSrc*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmbres.c */

extern void ilbmBresS(
#if NeedFunctionPrototypes
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*signdx*/,
	int /*signdy*/,
	int /*axis*/,
	int /*x1*/,
	int /*y1*/,
	int /*e*/,
	int /*e1*/,
	int /*e2*/,
	int /*len*/,
	unsigned char * /*rrops*/
#endif
);
/* ilbmbresd.c */

extern void ilbmBresD(
#if NeedFunctionPrototypes
	int * /*pdashIndex*/,
	unsigned char * /*pDash*/,
	int /*numInDashList*/,
	int * /*pdashOffset*/,
	int /*isDoubleDash*/,
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*signdx*/,
	int /*signdy*/,
	int /*axis*/,
	int /*x1*/,
	int /*y1*/,
	int /*e*/,
	int /*e1*/,
	int /*e2*/,
	int /*len*/,
	unsigned char * /*rrops*/,
	unsigned char * /*bgrrops*/
#endif
);
/* ilbmbstore.c */

extern void ilbmSaveAreas(
#if NeedFunctionPrototypes
	PixmapPtr /*pPixmap*/,
	RegionPtr /*prgnSave*/,
	int /*xorg*/,
	int /*yorg*/,
	WindowPtr /*pWin*/
#endif
);

extern void ilbmRestoreAreas(
#if NeedFunctionPrototypes
	PixmapPtr /*pPixmap*/,
	RegionPtr /*prgnRestore*/,
	int /*xorg*/,
	int /*yorg*/,
	WindowPtr /*pWin*/
#endif
);
/* ilbmclip.c */

extern RegionPtr ilbmPixmapToRegion(
#if NeedFunctionPrototypes
	PixmapPtr /*pPix*/
#endif
);

/* ilbmcmap.c */

extern Bool ilbmInitializeColormap(
#if NeedFunctionPrototypes
	ColormapPtr /*pmap*/
#endif
);

extern void ilbmResolveColor(
#if NeedFunctionPrototypes
	unsigned short * /*pred*/,
	unsigned short * /*pgreen*/,
	unsigned short * /*pblue*/,
	VisualPtr /*pVisual*/
#endif
);

extern Bool ilbmSetVisualTypes(
#if NeedFunctionPrototypes
	int /*depth*/,
	int /*visuals*/,
	int /*bitsPerRGB*/
#endif
);

/* ilbmfillarc.c */

extern void ilbmPolyFillArcSolid(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	GCPtr /*pGC*/,
	int /*narcs*/,
	xArc * /*parcs*/
#endif
);
/* ilbmfillrct.c */

extern void ilbmPolyFillRect(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nrectFill*/,
	xRectangle * /*prectInit*/
#endif
);

/* ilbmply1rct.c */
extern void ilbmFillPolygonSolid(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*shape*/,
	int /*count*/,
	DDXPointPtr /*ptsIn*/
#endif
);

/* ilbmfillsp.c */

extern void ilbmSolidFS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
);

extern void ilbmStippleFS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
);

extern void ilbmTileFS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
);

extern void ilbmUnnaturalTileFS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
);

extern void ilbmUnnaturalStippleFS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
);

extern void ilbmOpaqueStippleFS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
);

extern void ilbmUnnaturalOpaqueStippleFS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*nInit*/,
	DDXPointPtr /*pptInit*/,
	int * /*pwidthInit*/,
	int /*fSorted*/
#endif
);

/* ilbmfont.c */

extern Bool ilbmRealizeFont(
#if NeedFunctionPrototypes
	ScreenPtr /*pscr*/,
	FontPtr /*pFont*/
#endif
);

extern Bool ilbmUnrealizeFont(
#if NeedFunctionPrototypes
	ScreenPtr /*pscr*/,
	FontPtr /*pFont*/
#endif
);
/* ilbmgc.c */

extern Bool ilbmCreateGC(
#if NeedFunctionPrototypes
	GCPtr /*pGC*/
#endif
);

extern void ilbmValidateGC(
#if NeedFunctionPrototypes
	GCPtr /*pGC*/,
	unsigned long /*changes*/,
	DrawablePtr /*pDrawable*/
#endif
);

extern void ilbmDestroyGC(
#if NeedFunctionPrototypes
	GCPtr /*pGC*/
#endif
);

extern void ilbmReduceRop(
#if NeedFunctionPrototypes
	int /*alu*/,
	Pixel /*src*/,
	unsigned long /*planemask*/,
	int /*depth*/,
	unsigned char * /*rrops*/
#endif
);

extern void ilbmReduceOpaqueStipple (
#if NeedFunctionPrototypes
	Pixel /*fg*/,
	Pixel /*bg*/,
	unsigned long /*planemask*/,
	int /*depth*/,
	unsigned char * /*rrops*/
#endif
);

extern void ilbmComputeCompositeClip(
#if NeedFunctionPrototypes
   GCPtr /*pGC*/,
   DrawablePtr /*pDrawable*/
#endif
);

/* ilbmgetsp.c */

extern void ilbmGetSpans(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	int /*wMax*/,
	DDXPointPtr /*ppt*/,
	int * /*pwidth*/,
	int /*nspans*/,
	char * /*pdstStart*/
#endif
);
/* ilbmhrzvert.c */

extern int ilbmHorzS(
#if NeedFunctionPrototypes
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*x1*/,
	int /*y1*/,
	int /*len*/,
	unsigned char * /*rrops*/
#endif
);

extern int ilbmVertS(
#if NeedFunctionPrototypes
	PixelType * /*addrl*/,
	int /*nlwidth*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*x1*/,
	int /*y1*/,
	int /*len*/,
	unsigned char * /*rrops*/
#endif
);
/* ilbmigbblak.c */

extern void ilbmImageGlyphBlt (
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*x*/,
	int /*y*/,
	unsigned int /*nglyph*/,
	CharInfoPtr * /*ppci*/,
	pointer /*pglyphBase*/
#endif
);
/* ilbmigbwht.c */

/* ilbmimage.c */

extern void ilbmPutImage(
#if NeedFunctionPrototypes
	DrawablePtr /*dst*/,
	GCPtr /*pGC*/,
	int /*depth*/,
	int /*x*/,
	int /*y*/,
	int /*w*/,
	int /*h*/,
	int /*leftPad*/,
	int /*format*/,
	char * /*pImage*/
#endif
);

extern void ilbmGetImage(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	int /*sx*/,
	int /*sy*/,
	int /*w*/,
	int /*h*/,
	unsigned int /*format*/,
	unsigned long /*planeMask*/,
	char * /*pdstLine*/
#endif
);
/* ilbmline.c */

extern void ilbmLineSS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*npt*/,
	DDXPointPtr /*pptInit*/
#endif
);

extern void ilbmLineSD(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*npt*/,
	DDXPointPtr /*pptInit*/
#endif
);

/* ilbmmisc.c */

extern void ilbmQueryBestSize(
#if NeedFunctionPrototypes
	int /*class*/,
	unsigned short * /*pwidth*/,
	unsigned short * /*pheight*/,
	ScreenPtr /*pScreen*/
#endif
);
/* ilbmpntarea.c */

extern void ilbmSolidFillArea(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	unsigned char * /*rrops*/
#endif
);

extern void ilbmStippleAreaPPW(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	PixmapPtr /*pstipple*/,
	unsigned char * /*rrops*/
#endif
);
extern void ilbmStippleArea(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	PixmapPtr /*pstipple*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned char * /*rrops*/
#endif
);
/* ilbmplygblt.c */

extern void ilbmPolyGlyphBlt(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*x*/,
	int /*y*/,
	unsigned int /*nglyph*/,
	CharInfoPtr * /*ppci*/,
	pointer /*pglyphBase*/
#endif
);

/* ilbmpixmap.c */

extern PixmapPtr ilbmCreatePixmap(
#if NeedFunctionPrototypes
	ScreenPtr /*pScreen*/,
	int /*width*/,
	int /*height*/,
	int /*depth*/
#endif
);

extern Bool ilbmDestroyPixmap(
#if NeedFunctionPrototypes
	PixmapPtr /*pPixmap*/
#endif
);

extern PixmapPtr ilbmCopyPixmap(
#if NeedFunctionPrototypes
	PixmapPtr /*pSrc*/
#endif
);

extern void ilbmPadPixmap(
#if NeedFunctionPrototypes
	PixmapPtr /*pPixmap*/
#endif
);

extern void ilbmXRotatePixmap(
#if NeedFunctionPrototypes
	PixmapPtr /*pPix*/,
	int /*rw*/
#endif
);

extern void ilbmYRotatePixmap(
#if NeedFunctionPrototypes
	PixmapPtr /*pPix*/,
	int /*rh*/
#endif
);

extern void ilbmCopyRotatePixmap(
#if NeedFunctionPrototypes
	PixmapPtr /*psrcPix*/,
	PixmapPtr * /*ppdstPix*/,
	int /*xrot*/,
	int /*yrot*/
#endif
);
extern void ilbmPaintWindow(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/,
	RegionPtr /*pRegion*/,
	int /*what*/
#endif
);
/* ilbmpolypnt.c */

extern void ilbmPolyPoint(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*mode*/,
	int /*npt*/,
	xPoint * /*pptInit*/
#endif
);
/* ilbmpushpxl.c */

extern void ilbmPushPixels(
#if NeedFunctionPrototypes
	GCPtr /*pGC*/,
	PixmapPtr /*pBitMap*/,
	DrawablePtr /*pDrawable*/,
	int /*dx*/,
	int /*dy*/,
	int /*xOrg*/,
	int /*yOrg*/
#endif
);
/* ilbmscrclse.c */

extern Bool ilbmCloseScreen(
#if NeedFunctionPrototypes
	int /*index*/,
	ScreenPtr /*pScreen*/
#endif
);
/* ilbmscrinit.c */

extern Bool ilbmAllocatePrivates(
#if NeedFunctionPrototypes
	ScreenPtr /*pScreen*/,
	int * /*pWinIndex*/,
	int * /*pGCIndex*/
#endif
);

extern Bool ilbmScreenInit(
#if NeedFunctionPrototypes
	ScreenPtr /*pScreen*/,
	pointer /*pbits*/,
	int /*xsize*/,
	int /*ysize*/,
	int /*dpix*/,
	int /*dpiy*/,
	int /*width*/
#endif
);

extern PixmapPtr ilbmGetWindowPixmap(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/
#endif
);

extern void ilbmSetWindowPixmap(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/,
	PixmapPtr /*pPix*/
#endif
);

/* ilbmseg.c */

extern void ilbmSegmentSS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nseg*/,
	xSegment * /*pSeg*/
#endif
);

extern void ilbmSegmentSD(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	int /*nseg*/,
	xSegment * /*pSeg*/
#endif
);
/* ilbmsetsp.c */

extern int ilbmSetScanline(
#if NeedFunctionPrototypes
	int /*y*/,
	int /*xOrigin*/,
	int /*xStart*/,
	int /*xEnd*/,
	PixelType * /*psrc*/,
	int /*alu*/,
	PixelType * /*pdstBase*/,
	int /*widthDst*/,
	int /*sizeDst*/,
	int /*depthDst*/,
	int /*sizeSrc*/
#endif
);

extern void ilbmSetSpans(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr /*pGC*/,
	char * /*psrc*/,
	DDXPointPtr /*ppt*/,
	int * /*pwidth*/,
	int /*nspans*/,
	int /*fSorted*/
#endif
);
/* ilbmtegblt.c */

extern void ilbmTEGlyphBlt(
#if NeedFunctionPrototypes
	DrawablePtr /*pDrawable*/,
	GCPtr/*pGC*/,
	int /*x*/,
	int /*y*/,
	unsigned int /*nglyph*/,
	CharInfoPtr * /*ppci*/,
	pointer /*pglyphBase*/
#endif
);
/* ilbmtileC.c */

extern void ilbmTileAreaPPWCopy(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmtileG.c */

extern void ilbmTileAreaPPWGeneral(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned long /*planemask*/
#endif
);

extern void ilbmTileAreaCopy(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmtileG.c */

extern void ilbmTileAreaGeneral(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned long /*planemask*/
#endif
);

extern void ilbmOpaqueStippleAreaPPWCopy(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmtileG.c */

extern void ilbmOpaqueStippleAreaPPWGeneral(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
#endif
);

extern void ilbmOpaqueStippleAreaCopy(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
#endif
);
/* ilbmtileG.c */

extern void ilbmOpaqueStippleAreaGeneral(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	int /*nbox*/,
	BoxPtr /*pbox*/,
	int /*alu*/,
	PixmapPtr /*ptile*/,
	int /*xOff*/,
	int /*yOff*/,
	unsigned char */*rropsOS*/,
	unsigned long /*planemask*/
#endif
);

/* ilbmwindow.c */

extern Bool ilbmCreateWindow(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/
#endif
);

extern Bool ilbmDestroyWindow(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/
#endif
);

extern Bool ilbmMapWindow(
#if NeedFunctionPrototypes
	WindowPtr /*pWindow*/
#endif
);

extern Bool ilbmPositionWindow(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/,
	int /*x*/,
	int /*y*/
#endif
);

extern Bool ilbmUnmapWindow(
#if NeedFunctionPrototypes
	WindowPtr /*pWindow*/
#endif
);

extern void ilbmCopyWindow(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/,
	DDXPointRec /*ptOldOrg*/,
	RegionPtr /*prgnSrc*/
#endif
);

extern Bool ilbmChangeWindowAttributes(
#if NeedFunctionPrototypes
	WindowPtr /*pWin*/,
	unsigned long /*mask*/
#endif
);
/* ilbmzerarc.c */

extern void ilbmZeroPolyArcSS(
#if NeedFunctionPrototypes
	DrawablePtr /*pDraw*/,
	GCPtr /*pGC*/,
	int /*narcs*/,
	xArc * /*parcs*/
#endif
);

/*
	private field of pixmap
	pixmap.devPrivate = (PixelType *)pointer_to_bits
	pixmap.devKind = width_of_pixmap_in_bytes

	private field of screen
	a pixmap, for which we allocate storage.  devPrivate is a pointer to
the bits in the hardware framebuffer.  note that devKind can be poked to
make the code work for framebuffers that are wider than their
displayable screen (e.g. the early vsII, which displayed 960 pixels
across, but was 1024 in the hardware.)

	private field of GC
*/

typedef struct {
	unsigned char rrops[AFB_MAX_DEPTH];		/* reduction of rasterop to 1 of 3 */
	unsigned char rropOS[AFB_MAX_DEPTH];	/* rop for opaque stipple */
} ilbmPrivGC;
typedef ilbmPrivGC *ilbmPrivGCPtr;

extern int ilbmGCPrivateIndex;			/* index into GC private array */
extern int ilbmWindowPrivateIndex;		/* index into Window private array */
#ifdef PIXMAP_PER_WINDOW
extern int frameWindowPrivateIndex;		/* index into Window private array */
#endif

#define ilbmGetGCPrivate(pGC) \
	((ilbmPrivGC *)((pGC)->devPrivates[ilbmGCPrivateIndex].ptr))

/* private field of window */
typedef struct {
	unsigned char fastBorder;	/* non-zero if border tile is 32 bits wide */
	unsigned char fastBackground;
	unsigned short unused; /* pad for alignment with Sun compiler */
	DDXPointRec oldRotate;
	PixmapPtr pRotatedBackground;
	PixmapPtr pRotatedBorder;
} ilbmPrivWin;

/* Common macros for extracting drawing information */

#define ilbmGetPixelWidthAuxDepthAndPointer(pDrawable, width, aux, dep, pointer) {\
	PixmapPtr _pPix; \
	if ((pDrawable)->type == DRAWABLE_WINDOW) \
		_pPix = (PixmapPtr)(pDrawable)->pScreen->devPrivates[ilbmScreenPrivateIndex].ptr; \
	else \
		_pPix = (PixmapPtr)(pDrawable); \
	(pointer) = (PixelType *)_pPix->devPrivate.ptr; \
	(width) = ((int)_pPix->devKind)/sizeof(PixelType); \
	(dep) = _pPix->drawable.depth; \
	(aux) = (width)*(dep); \
}

/*  ilbm uses the following macros to calculate addresses in drawables.
 *  To support banked framebuffers, the macros come in four flavors.
 *  All four collapse into the same definition on unbanked devices.
 *
 *  ilbmScanlineFoo - calculate address and do bank switching
 *  ilbmScanlineFooNoBankSwitch - calculate address, don't bank switch
 *  ilbmScanlineFooSrc - calculate address, switch source bank
 *  ilbmScanlineFooDst - calculate address, switch destination bank
 */

/* The NoBankSwitch versions are the same for banked and unbanked cases */

#define ilbmScanlineIncNoBankSwitch(_ptr, _off) _ptr += (_off)
#define ilbmScanlineOffsetNoBankSwitch(_ptr, _off) ((_ptr)+(_off))
#define ilbmScanlineDeltaNoBankSwitch(_ptr, _y, _w) \
	ilbmScanlineOffsetNoBankSwitch(_ptr, (_y)*(_w))
#define ilbmScanlineNoBankSwitch(_ptr, _x, _y, _w) \
	ilbmScanlineOffsetNoBankSwitch(_ptr, (_y)*(_w)+((_x)>>MFB_PWSH))

#ifdef MFB_LINE_BANK

#include "ilbmlinebank.h" /* get macro definitions from this file */

#else /* !MFB_LINE_BANK - unbanked case */

#define ilbmScanlineInc(_ptr, _off)				ilbmScanlineIncNoBankSwitch(_ptr, _off)
#define ilbmScanlineIncSrc(_ptr, _off)			ilbmScanlineInc(_ptr, _off)
#define ilbmScanlineIncDst(_ptr, _off)			ilbmScanlineInc(_ptr, _off)

#define ilbmScanlineOffset(_ptr, _off)			ilbmScanlineOffsetNoBankSwitch(_ptr, _off)
#define ilbmScanlineOffsetSrc(_ptr, _off)		ilbmScanlineOffset(_ptr, _off)
#define ilbmScanlineOffsetDst(_ptr, _off)		ilbmScanlineOffset(_ptr, _off)

#define ilbmScanlineSrc(_ptr, _x, _y, _w)		ilbmScanline(_ptr, _x, _y, _w)
#define ilbmScanlineDst(_ptr, _x, _y, _w)		ilbmScanline(_ptr, _x, _y, _w)

#define ilbmScanlineDeltaSrc(_ptr, _y, _w)	ilbmScanlineDelta(_ptr, _y, _w)
#define ilbmScanlineDeltaDst(_ptr, _y, _w)	ilbmScanlineDelta(_ptr, _y, _w)

#endif /* MFB_LINE_BANK */

#define ilbmScanlineDelta(_ptr, _y, _w) \
	ilbmScanlineOffset(_ptr, (_y)*(_w))

#define ilbmScanline(_ptr, _x, _y, _w) \
	ilbmScanlineOffset(_ptr, (_y)*(_w)+((_x)>>MFB_PWSH))

/* precomputed information about each glyph for GlyphBlt code.
   this saves recalculating the per glyph information for each box.
*/

typedef struct _ilbmpos{
	int xpos;					/* xposition of glyph's origin */
	int xchar;					/* x position mod 32 */
	int leftEdge;
	int rightEdge;
	int topEdge;
	int bottomEdge;
	PixelType *pdstBase;		/* longword with character origin */
	int widthGlyph;			/* width in bytes of this glyph */
} ilbmTEXTPOS;

/* reduced raster ops for ilbm */
#define RROP_BLACK			GXclear
#define RROP_WHITE			GXset
#define RROP_NOP				GXnoop
#define RROP_INVERT			GXinvert
#define RROP_COPY				GXcopy

/* macros for ilbmbitblt.c, ilbmfillsp.c
	these let the code do one switch on the rop per call, rather
	than a switch on the rop per item (span or rectangle.)
*/

#define fnCLEAR(src, dst)				(0)
#define fnAND(src, dst)					(src & dst)
#define fnANDREVERSE(src, dst)		(src & ~dst)
#define fnCOPY(src, dst)				(src)
#define fnANDINVERTED(src, dst)		(~src & dst)
#define fnNOOP(src, dst)				(dst)
#define fnXOR(src, dst)					(src ^ dst)
#define fnOR(src, dst)					(src | dst)
#define fnNOR(src, dst)					(~(src | dst))
#define fnEQUIV(src, dst)				(~src ^ dst)
#define fnINVERT(src, dst)				(~dst)
#define fnORREVERSE(src, dst)			(src | ~dst)
#define fnCOPYINVERTED(src, dst)		(~src)
#define fnORINVERTED(src, dst)		(~src | dst)
#define fnNAND(src, dst)				(~(src & dst))
#define fnSET(src, dst)					(~0)

/*  Using a "switch" statement is much faster in most cases
 *  since the compiler can do a look-up table or multi-way branch
 *  instruction, depending on the architecture.  The result on
 *  A Sun 3/50 is at least 2.5 times faster, assuming a uniform
 *  distribution of RasterOp operation types.
 *
 *  However, doing some profiling on a running system reveals
 *  GXcopy is the operation over 99.5% of the time and
 *  GXxor is the next most frequent (about .4%), so we make special
 *  checks for those first.
 *
 *  Note that this requires a change to the "calling sequence"
 *  since we can't engineer a "switch" statement to have an lvalue.
 */
#define DoRop(result, alu, src, dst) \
{ \
	if (alu == GXcopy) \
		result = fnCOPY (src, dst); \
	else if (alu == GXxor) \
		result = fnXOR (src, dst); \
	else \
		switch (alu) { \
			case GXclear: \
				result = fnCLEAR (src, dst); \
				break; \
			case GXand: \
				result = fnAND (src, dst); \
				break; \
			case GXandReverse: \
				result = fnANDREVERSE (src, dst); \
				break; \
			case GXandInverted: \
				result = fnANDINVERTED (src, dst); \
				break; \
			case GXnoop: \
				result = fnNOOP (src, dst); \
				break; \
			case GXor: \
				result = fnOR (src, dst); \
				break; \
			case GXnor: \
				result = fnNOR (src, dst); \
				break; \
			case GXequiv: \
				result = fnEQUIV (src, dst); \
				break; \
			case GXinvert: \
				result = fnINVERT (src, dst); \
				break; \
			case GXorReverse: \
				result = fnORREVERSE (src, dst); \
				break; \
			case GXcopyInverted: \
				result = fnCOPYINVERTED (src, dst); \
				break; \
			case GXorInverted: \
				result = fnORINVERTED (src, dst); \
				break; \
			case GXnand: \
				result = fnNAND (src, dst); \
				break; \
			case GXset: \
				result = fnSET (src, dst); \
				break; \
	} \
}


/*  C expression fragments for various operations.  These get passed in
 *  as -D's on the compile command line.  See ilbm/Imakefile.  This
 *  fixes XBUG 6319.
 *
 *  This seems like a good place to point out that ilbm's use of the
 *  words black and white is an unfortunate misnomer.  In ilbm code, black
 *  means zero, and white means one.
 */
#define MFB_OPEQ_WHITE				|=
#define MFB_OPEQ_BLACK				&=~
#define MFB_OPEQ_INVERT				^=
#define MFB_EQWHOLEWORD_WHITE		=~0
#define MFB_EQWHOLEWORD_BLACK		=0
#define MFB_EQWHOLEWORD_INVERT	^=~0
#define MFB_OP_WHITE					/* nothing */
#define MFB_OP_BLACK					~
