/* $XFree86: xc/programs/Xserver/iplan2p4/ipl.h,v 3.5 2001/01/30 22:06:21 tsi Exp $ */
/* $XConsortium: ipl.h,v 5.37 94/04/17 20:28:38 dpw Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or X Consortium
not be used in advertising or publicity pertaining to 
distribution  of  the software  without specific prior 
written permission. Sun and X Consortium make no 
representations about the suitability of this software for 
any purpose. It is provided "as is" without any express or 
implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "X.h"
#include "pixmap.h"
#include "region.h"
#include "gc.h"
#include "colormap.h"
#include "miscstruct.h"
#include "servermd.h"
#include "windowstr.h"
#include "mfb.h"
#undef PixelType

#include "iplmap.h"

/*
   private filed of pixmap
   pixmap.devPrivate = (unsigned int *)pointer_to_bits
   pixmap.devKind = width_of_pixmap_in_bytes
*/

extern int  iplGCPrivateIndex;
extern int  iplWindowPrivateIndex;

/* private field of GC */
typedef struct {
    unsigned char       rop;            /* special case rop values */
    /* next two values unused in ipl, included for compatibility with mfb */
    unsigned char       ropOpStip;      /* rop for opaque stipple */
    /* this value is ropFillArea in mfb, usurped for ipl */
    unsigned char       oneRect;	/*  drawable has one clip rect */
    unsigned long	xor, and;	/* reduced rop values */
    unsigned short 	xorg[INTER_PLANES],andg[INTER_PLANES];
    } iplPrivGC;

typedef iplPrivGC	*iplPrivGCPtr;

#define iplGetGCPrivate(pGC)	((iplPrivGCPtr)\
	(pGC)->devPrivates[iplGCPrivateIndex].ptr)

#define iplGetCompositeClip(pGC) ((pGC)->pCompositeClip)

/* way to carry RROP info around */
typedef struct {
    unsigned char	rop;
    unsigned long	xor, and;
    unsigned short 	xorg[INTER_PLANES],andg[INTER_PLANES];
} iplRRopRec, *iplRRopPtr;

/* private field of window */
typedef struct {
    unsigned	char fastBorder; /* non-zero if border is 32 bits wide */
    unsigned	char fastBackground;
    unsigned short unused; /* pad for alignment with Sun compiler */
    DDXPointRec	oldRotate;
    PixmapPtr	pRotatedBackground;
    PixmapPtr	pRotatedBorder;
    } iplPrivWin;

#define iplGetWindowPrivate(_pWin) ((iplPrivWin *)\
	(_pWin)->devPrivates[iplWindowPrivateIndex].ptr)


/* ipl8bit.c */

extern int iplSetStipple(
#if NeedFunctionPrototypes
    int /*alu*/,
    unsigned long /*fg*/,
    unsigned long /*planemask*/
#endif
);

extern int iplSetOpaqueStipple(
#if NeedFunctionPrototypes
    int /*alu*/,
    unsigned long /*fg*/,
    unsigned long /*bg*/,
    unsigned long /*planemask*/
#endif
);

extern int iplComputeClipMasks32(
#if NeedFunctionPrototypes
    BoxPtr /*pBox*/,
    int /*numRects*/,
    int /*x*/,
    int /*y*/,
    int /*w*/,
    int /*h*/,
    CARD32 * /*clips*/
#endif
);
/* ipl8cppl.c */

extern void iplCopyImagePlane(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
#endif
);

extern void iplCopyPlane8to1(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
#endif
);
/* ipl8lineCO.c */

extern int ipl8LineSS1RectCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/
#endif
);

extern void ipl8LineSS1Rect(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
#endif
);

extern void ipl8ClippedLineCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
#endif
);
/* ipl8lineCP.c */

extern int ipl8LineSS1RectPreviousCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/

#endif
);
/* ipl8lineG.c */

extern int ipl8LineSS1RectGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/
#endif
);

extern void ipl8ClippedLineGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
#endif
);
/* ipl8lineX.c */

extern int ipl8LineSS1RectXor(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/,
    DDXPointPtr /*pptInitOrig*/,
    int * /*x1p*/,
    int * /*y1p*/,
    int * /*x2p*/,
    int * /*y2p*/
#endif
);

extern void ipl8ClippedLineXor(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*x1*/,
    int /*y1*/,
    int /*x2*/,
    int /*y2*/,
    BoxPtr /*boxp*/,
    Bool /*shorten*/
#endif
);
/* ipl8segC.c */

extern int ipl8SegmentSS1RectCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
#endif
);
/* ipl8segCS.c */

extern int ipl8SegmentSS1RectShiftCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
#endif
);

extern void ipl8SegmentSS1Rect(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
#endif
);
/* ipl8segG.c */

extern int ipl8SegmentSS1RectGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
#endif
);
/* iplsegX.c */

extern int ipl8SegmentSS1RectXor(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSegInit*/
#endif
);
/* iplallpriv.c */

extern Bool iplAllocatePrivates(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    int * /*window_index*/,
    int * /*gc_index*/
#endif
);
/* iplbitblt.c */

extern RegionPtr iplBitBlt(
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
    void (* /*doBitBlt*/)(),
    unsigned long /*bitPlane*/
#endif
);

extern void iplDoBitblt(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
#endif
);

extern RegionPtr iplCopyArea(
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

extern void iplCopyPlane1to8(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    int /*rop*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/,
    unsigned long /*bitPlane*/
#endif
);

extern RegionPtr iplCopyPlane(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrcDrawable*/,
    DrawablePtr /*pDstDrawable*/,
    GCPtr /*pGC*/,
    int /*srcx*/,
    int /*srcy*/,
    int /*width*/,
    int /*height*/,
    int /*dstx*/,
    int /*dsty*/,
    unsigned long /*bitPlane*/
#endif
);
/* iplbltC.c */

extern void iplDoBitbltCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
#endif
);
/* iplbltG.c */

extern void iplDoBitbltGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
#endif
);
/* iplbltO.c */

extern void iplDoBitbltOr(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
#endif
);
/* iplbltX.c */

extern void iplDoBitbltXor(
#if NeedFunctionPrototypes
    DrawablePtr /*pSrc*/,
    DrawablePtr /*pDst*/,
    int /*alu*/,
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*pptSrc*/,
    unsigned long /*planemask*/
#endif
);
/* iplbres.c */

extern void iplBresS(
#if NeedFunctionPrototypes
    int /*rop*/,
    unsigned short * /*and*/,
    unsigned short * /*xor*/,
    unsigned short * /*addrl*/,
    int /*nlwidth*/,
    int /*signdx*/,
    int /*signdy*/,
    int /*axis*/,
    int /*x1*/,
    int /*y1*/,
    int /*e*/,
    int /*e1*/,
    int /*e2*/,
    int /*len*/
#endif
);
/* iplbresd.c */

extern void iplBresD(
#if NeedFunctionPrototypes
    iplRRopPtr /*rrops*/,
    int * /*pdashIndex*/,
    unsigned char * /*pDash*/,
    int /*numInDashList*/,
    int * /*pdashOffset*/,
    int /*isDoubleDash*/,
    unsigned short * /*addrl*/,
    int /*nlwidth*/,
    int /*signdx*/,
    int /*signdy*/,
    int /*axis*/,
    int /*x1*/,
    int /*y1*/,
    int /*e*/,
    int /*e1*/,
    int /*e2*/,
    int /*len*/
#endif
);
/* iplbstore.c */

extern void iplSaveAreas(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnSave*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
#endif
);

extern void iplRestoreAreas(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/,
    RegionPtr /*prgnRestore*/,
    int /*xorg*/,
    int /*yorg*/,
    WindowPtr /*pWin*/
#endif
);
/* iplcmap.c */

extern int iplListInstalledColormaps(
#if NeedFunctionPrototypes
    ScreenPtr	/*pScreen*/,
    Colormap	* /*pmaps*/
#endif
);

extern void iplInstallColormap(
#if NeedFunctionPrototypes
    ColormapPtr	/*pmap*/
#endif
);

extern void iplUninstallColormap(
#if NeedFunctionPrototypes
    ColormapPtr	/*pmap*/
#endif
);

extern void iplResolveColor(
#if NeedFunctionPrototypes
    unsigned short * /*pred*/,
    unsigned short * /*pgreen*/,
    unsigned short * /*pblue*/,
    VisualPtr /*pVisual*/
#endif
);

extern Bool iplInitializeColormap(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/
#endif
);

extern int iplExpandDirectColors(
#if NeedFunctionPrototypes
    ColormapPtr /*pmap*/,
    int /*ndef*/,
    xColorItem * /*indefs*/,
    xColorItem * /*outdefs*/
#endif
);

extern Bool iplCreateDefColormap(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/
#endif
);

extern Bool iplSetVisualTypes(
#if NeedFunctionPrototypes
    int /*depth*/,
    int /*visuals*/,
    int /*bitsPerRGB*/
#endif
);

extern Bool iplInitVisuals(
#if NeedFunctionPrototypes
    VisualPtr * /*visualp*/,
    DepthPtr * /*depthp*/,
    int * /*nvisualp*/,
    int * /*ndepthp*/,
    int * /*rootDepthp*/,
    VisualID * /*defaultVisp*/,
    unsigned long /*sizes*/,
    int /*bitsPerRGB*/
#endif
);
/* iplfillarcC.c */

extern void iplPolyFillArcSolidCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
#endif
);
/* iplfillarcG.c */

extern void iplPolyFillArcSolidGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
#endif
);
/* iplfillrct.c */

extern void iplFillBoxTileOdd(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    BoxPtr /*rects*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/
#endif
);

extern void iplFillRectTileOdd(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void iplPolyFillRect(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nrectFill*/,
    xRectangle * /*prectInit*/
#endif
);
/* iplfillsp.c */

extern void iplUnnaturalTileFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void iplUnnaturalStippleFS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void ipl8Stipple32FS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);

extern void ipl8OpaqueStipple32FS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);
/* iplgc.c */

extern GCOpsPtr iplMatchCommon(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/,
    iplPrivGCPtr /*devPriv*/
#endif
);

extern Bool iplCreateGC(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/
#endif
);

extern void iplValidateGC(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/,
    unsigned long /*changes*/,
    DrawablePtr /*pDrawable*/
#endif
);

/* iplgetsp.c */

extern void iplGetSpans(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*wMax*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    char * /*pdstStart*/
#endif
);
/* iplglblt8.c */

extern void iplPolyGlyphBlt8(
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
/* iplglrop8.c */

extern void iplPolyGlyphRop8(
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
/* iplhrzvert.c */

extern int iplHorzS(
#if NeedFunctionPrototypes
    int /*rop*/,
    unsigned short * /*and*/,
    unsigned short * /*xor*/,
    unsigned short * /*addrg*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
#endif
);

extern int iplVertS(
#if NeedFunctionPrototypes
    int /*rop*/,
    unsigned short * /*and*/,
    unsigned short * /*xor*/,
    unsigned short * /*addrg*/,
    int /*nlwidth*/,
    int /*x1*/,
    int /*y1*/,
    int /*len*/
#endif
);
/* ipligblt8.c */

extern void iplImageGlyphBlt8(
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
/* iplimage.c */

extern void iplPutImage(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
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

extern void iplGetImage(
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
/* iplline.c */

extern void iplLineSS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
#endif
);

extern void iplLineSD(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    DDXPointPtr /*pptInit*/
#endif
);
/* iplmskbits.c */
/* iplpixmap.c */

extern PixmapPtr iplCreatePixmap(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/,
    int /*width*/,
    int /*height*/,
    int /*depth*/
#endif
);

extern Bool iplDestroyPixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/
#endif
);

extern PixmapPtr iplCopyPixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pSrc*/
#endif
);

extern void iplPadPixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPixmap*/
#endif
);

extern void iplXRotatePixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPix*/,
    int /*rw*/
#endif
);

extern void iplYRotatePixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPix*/,
    int /*rh*/
#endif
);

extern void iplCopyRotatePixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*psrcPix*/,
    PixmapPtr * /*ppdstPix*/,
    int /*xrot*/,
    int /*yrot*/
#endif
);
/* iplply1rctC.c */

extern void iplFillPoly1RectCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
#endif
);
/* iplply1rctG.c */

extern void iplFillPoly1RectGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*shape*/,
    int /*mode*/,
    int /*count*/,
    DDXPointPtr /*ptsIn*/
#endif
);
/* iplpntwin.c */

extern void iplPaintWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    RegionPtr /*pRegion*/,
    int /*what*/
#endif
);

extern void iplFillBoxSolid(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    unsigned long /*pixel*/
#endif
);

extern void iplFillBoxTile32(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/
#endif
);
/* iplpolypnt.c */

extern void iplPolyPoint(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*mode*/,
    int /*npt*/,
    xPoint * /*pptInit*/
#endif
);
/* iplpush8.c */

extern void iplPushPixels8(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/,
    PixmapPtr /*pBitmap*/,
    DrawablePtr /*pDrawable*/,
    int /*dx*/,
    int /*dy*/,
    int /*xOrg*/,
    int /*yOrg*/
#endif
);
/* iplrctstp8.c */

extern void ipl8FillRectOpaqueStippled32(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void ipl8FillRectTransparentStippled32(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void ipl8FillRectStippledUnnatural(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);
/* iplrrop.c */

extern int iplReduceRasterOp(
#if NeedFunctionPrototypes
    int /*rop*/,
    unsigned long /*fg*/,
    unsigned long /*pm*/,
    unsigned short * /*andp*/,
    unsigned short * /*xorp*/
#endif
);
/* iplscrinit.c */

extern Bool iplCloseScreen(
#if NeedFunctionPrototypes
    int /*index*/,
    ScreenPtr /*pScreen*/
#endif
);

extern Bool iplSetupScreen(
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

extern int iplFinishScreenInit(
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

extern Bool iplScreenInit(
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

extern PixmapPtr iplGetScreenPixmap(
#if NeedFunctionPrototypes
    ScreenPtr /*pScreen*/
#endif
);

extern void iplSetScreenPixmap(
#if NeedFunctionPrototypes
    PixmapPtr /*pPix*/
#endif
);

/* iplseg.c */

extern void iplSegmentSS(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
#endif
);

extern void iplSegmentSD(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nseg*/,
    xSegment * /*pSeg*/
#endif
);
/* iplsetsp.c */

extern int iplSetScanline(
#if NeedFunctionPrototypes
    int /*y*/,
    int /*xOrigin*/,
    int /*xStart*/,
    int /*xEnd*/,
    unsigned int * /*psrc*/,
    int /*alu*/,
    unsigned short * /*pdstBase*/,
    int /*widthDst*/,
    unsigned long /*planemask*/
#endif
);

extern void iplSetSpans(
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
/* iplsolidC.c */

extern void iplFillRectSolidCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void iplSolidSpansCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);
/* iplsolidG.c */

extern void iplFillRectSolidGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void iplSolidSpansGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);
/* iplsolidX.c */

extern void iplFillRectSolidXor(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void iplSolidSpansXor(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);
/* iplteblt8.c */

extern void iplTEGlyphBlt8(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr/*pGC*/,
    int /*xInit*/,
    int /*yInit*/,
    unsigned int /*nglyph*/,
    CharInfoPtr * /*ppci*/,
    pointer /*pglyphBase*/
#endif
);
/* ipltegblt.c */

extern void iplTEGlyphBlt(
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
/* ipltile32C.c */

extern void iplFillRectTile32Copy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void iplTile32FSCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);
/* ipltile32G.c */

extern void iplFillRectTile32General(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nBox*/,
    BoxPtr /*pBox*/
#endif
);

extern void iplTile32FSGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    GCPtr /*pGC*/,
    int /*nInit*/,
    DDXPointPtr /*pptInit*/,
    int * /*pwidthInit*/,
    int /*fSorted*/
#endif
);
/* ipltileoddC.c */

extern void iplFillBoxTileOddCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);

extern void iplFillSpanTileOddCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);

extern void iplFillBoxTile32sCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);

extern void iplFillSpanTile32sCopy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);
/* ipltileoddG.c */

extern void iplFillBoxTileOddGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);

extern void iplFillSpanTileOddGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);

extern void iplFillBoxTile32sGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*nBox*/,
    BoxPtr /*pBox*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);

extern void iplFillSpanTile32sGeneral(
#if NeedFunctionPrototypes
    DrawablePtr /*pDrawable*/,
    int /*n*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    PixmapPtr /*tile*/,
    int /*xrot*/,
    int /*yrot*/,
    int /*alu*/,
    unsigned long /*planemask*/
#endif
);
/* iplwindow.c */

extern Bool iplCreateWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/
#endif
);

extern Bool iplDestroyWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/
#endif
);

extern Bool iplMapWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWindow*/
#endif
);

extern Bool iplPositionWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    int /*x*/,
    int /*y*/
#endif
);

extern Bool iplUnmapWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWindow*/
#endif
);

extern void iplCopyWindow(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    DDXPointRec /*ptOldOrg*/,
    RegionPtr /*prgnSrc*/
#endif
);

extern Bool iplChangeWindowAttributes(
#if NeedFunctionPrototypes
    WindowPtr /*pWin*/,
    unsigned long /*mask*/
#endif
);
/* iplzerarcC.c */

extern void iplZeroPolyArcSS8Copy(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
#endif
);
/* iplzerarcG.c */

extern void iplZeroPolyArcSS8General(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
#endif
);
/* iplzerarcX.c */

extern void iplZeroPolyArcSS8Xor(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*narcs*/,
    xArc * /*parcs*/
#endif
);

/* Common macros for extracting drawing information */

#if (!defined(SINGLEDEPTH) && PSZ != 8) || defined(FORCE_SEPARATE_PRIVATE)

#define CFB_NEED_SCREEN_PRIVATE

extern int iplScreenPrivateIndex;
#endif

#define iplGetWindowPixmap(d) \
    ((* ((DrawablePtr)(d))->pScreen->GetWindowPixmap)((WindowPtr)(d)))

#define iplGetTypedWidth(pDrawable,wtype) (\
    (((pDrawable)->type != DRAWABLE_PIXMAP) ? \
     (int) (iplGetWindowPixmap(pDrawable)->devKind) : \
     (int)(((PixmapPtr)pDrawable)->devKind)) / sizeof (wtype))

#define iplGetByteWidth(pDrawable) iplGetTypedWidth(pDrawable, unsigned char)

#define iplGetPixelWidth(pDrawable) iplGetTypedWidth(pDrawable, PixelType)

#define iplGetLongWidth(pDrawable) iplGetTypedWidth(pDrawable, unsigned long)
    
#define iplGetTypedWidthAndPointer(pDrawable, width, pointer, wtype, ptype) {\
    PixmapPtr   _pPix; \
    if ((pDrawable)->type != DRAWABLE_PIXMAP) \
	_pPix = iplGetWindowPixmap(pDrawable); \
    else \
	_pPix = (PixmapPtr) (pDrawable); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define iplGetByteWidthAndPointer(pDrawable, width, pointer) \
    iplGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned char, unsigned char)

#define iplGetLongWidthAndPointer(pDrawable, width, pointer) \
    iplGetTypedWidthAndPointer(pDrawable, width, pointer, unsigned long, unsigned long)

#define iplGetPixelWidthAndPointer(pDrawable, width, pointer) \
    iplGetTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

#define iplGetWindowTypedWidthAndPointer(pWin, width, pointer, wtype, ptype) {\
    PixmapPtr	_pPix = iplGetWindowPixmap((DrawablePtr) (pWin)); \
    (pointer) = (ptype *) _pPix->devPrivate.ptr; \
    (width) = ((int) _pPix->devKind) / sizeof (wtype); \
}

#define iplGetWindowLongWidthAndPointer(pWin, width, pointer) \
    iplGetWindowTypedWidthAndPointer(pWin, width, pointer, unsigned long, unsigned long)

#define iplGetWindowByteWidthAndPointer(pWin, width, pointer) \
    iplGetWindowTypedWidthAndPointer(pWin, width, pointer, unsigned char, unsigned char)

#define iplGetWindowPixelWidthAndPointer(pDrawable, width, pointer) \
    iplGetWindowTypedWidthAndPointer(pDrawable, width, pointer, PixelType, PixelType)

/* Macros which handle a coordinate in a single register */

/* Most compilers will convert divide by 65536 into a shift, if signed
 * shifts exist.  If your machine does arithmetic shifts and your compiler
 * can't get it right, add to this line.
 */

/* mips compiler - what a joke - it CSEs the 65536 constant into a reg
 * forcing as to use div instead of shift.  Let's be explicit.
 */

#if defined(mips) || defined(sparc) || defined(__alpha) || defined(__alpha__)
#define GetHighWord(x) (((int) (x)) >> 16)
#else
#define GetHighWord(x) (((int) (x)) / 65536)
#endif

#if IMAGE_BYTE_ORDER == MSBFirst
#define intToCoord(i,x,y)   (((x) = GetHighWord(i)), ((y) = (int) ((short) (i))))
#define coordToInt(x,y)	(((x) << 16) | (y))
#define intToX(i)	(GetHighWord(i))
#define intToY(i)	((int) ((short) i))
#else
#define intToCoord(i,x,y)   (((x) = (int) ((short) (i))), ((y) = GetHighWord(i)))
#define coordToInt(x,y)	(((y) << 16) | (x))
#define intToX(i)	((int) ((short) (i)))
#define intToY(i)	(GetHighWord(i))
#endif
