/*
 * Copyright © 2004 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

#include "picturestr.h"

/*
 * One of these structures is allocated per GC that gets used with a window with
 * backing pixmap.
 */

typedef struct {
    GCPtr	    pBackingGC;	    /* Copy of the GC but with graphicsExposures
				     * set FALSE and the clientClip set to
				     * clip output to the valid regions of the
				     * backing pixmap. */
    int		    guarantee;      /* GuaranteeNothing, etc. */
    unsigned long   serialNumber;   /* clientClip computed time */
    unsigned long   stateChanges;   /* changes in parent gc since last copy */
    GCOps	    *wrapOps;	    /* wrapped ops */
    GCFuncs	    *wrapFuncs;	    /* wrapped funcs */
} cwGCRec, *cwGCPtr;

extern int cwGCIndex;

#define getCwGC(pGC)	((cwGCPtr)(pGC)->devPrivates[cwGCIndex].ptr)
#define setCwGC(pGC,p)	((pGC)->devPrivates[cwGCIndex].ptr = (pointer) (p))

typedef struct {
    PicturePtr	    pBackingPicture;
    unsigned long   serialNumber;   /* clientClip computed time */
    unsigned long   stateChanges;   /* changes in parent gc since last copy */
} cwPictureRec, *cwPicturePtr;

extern int  cwPictureIndex;

#define getCwPicture(pPicture)	    ((cwPicturePtr)(pPicture)->devPrivates[cwPictureIndex].ptr)
#define setCwPicture(pPicture,p)    ((pPicture)->devPrivates[cwPictureIndex].ptr = (pointer) (p))

#define cwDrawableIsRedirWindow(pDraw)	((pDraw)->type == DRAWABLE_WINDOW && \
					 ((WindowPtr)(pDraw))->redirectDraw)

typedef struct {
    /*
     * screen func wrappers
     */
    CloseScreenProcPtr	CloseScreen;
    GetImageProcPtr	GetImage;
    GetSpansProcPtr	GetSpans;
    CreateGCProcPtr	CreateGC;

    DestroyWindowProcPtr	DestroyWindow;

    StoreColorsProcPtr		StoreColors;

    InitIndexedProcPtr		InitIndexed;
    CloseIndexedProcPtr		CloseIndexed;
    UpdateIndexedProcPtr	UpdateIndexed;
    
#ifdef RENDER
    CreatePictureProcPtr	CreatePicture;
    DestroyPictureProcPtr	DestroyPicture;
    ChangePictureClipProcPtr	ChangePictureClip;
    DestroyPictureClipProcPtr	DestroyPictureClip;
    
    ChangePictureProcPtr	ChangePicture;
    ValidatePictureProcPtr	ValidatePicture;

    CompositeProcPtr		Composite;
    GlyphsProcPtr		Glyphs;
    CompositeRectsProcPtr	CompositeRects;

    TrapezoidsProcPtr		Trapezoids;
    TrianglesProcPtr		Triangles;
    TriStripProcPtr		TriStrip;
    TriFanProcPtr		TriFan;

    RasterizeTrapezoidProcPtr	RasterizeTrapezoid;
#if 0
    AddTrapsProcPtr		AddTraps;
#endif
#endif
} cwScreenRec, *cwScreenPtr;

extern int cwScreenIndex;

#define getCwScreen(pScreen)	((cwScreenPtr)(pScreen)->devPrivates[cwScreenIndex].ptr)
#define setCwScreen(pScreen,p)	((cwScreenPtr)(pScreen)->devPrivates[cwScreenIndex].ptr = (p))

#define CW_COPY_OFFSET_XYPOINTS(ppt_trans, ppt, npt) do { \
    short *_origpt = (short *)(ppt); \
    short *_transpt = (short *)(ppt_trans); \
    int _i; \
    for (_i = 0; _i < npt; _i++) { \
	*_transpt++ = *_origpt++ + dst_off_x; \
	*_transpt++ = *_origpt++ + dst_off_y; \
    } \
} while (0)

#define CW_COPY_OFFSET_RECTS(prect_trans, prect, nrect) do { \
    short *_origpt = (short *)(prect); \
    short *_transpt = (short *)(prect_trans); \
    int _i; \
    for (_i = 0; _i < nrect; _i++) { \
	*_transpt++ = *_origpt++ + dst_off_x; \
	*_transpt++ = *_origpt++ + dst_off_y; \
	_transpt += 2; \
	_origpt += 2; \
    } \
} while (0)

#define CW_COPY_OFFSET_ARCS(parc_trans, parc, narc) do { \
    short *_origpt = (short *)(parc); \
    short *_transpt = (short *)(parc_trans); \
    int _i; \
    for (_i = 0; _i < narc; _i++) { \
	*_transpt++ = *_origpt++ + dst_off_x; \
	*_transpt++ = *_origpt++ + dst_off_y; \
	_transpt += 4; \
	_origpt += 4; \
    } \
} while (0)

#define CW_COPY_OFFSET_XY_DST(bx, by, x, y) do { \
    bx = x + dst_off_x; \
    by = y + dst_off_y; \
} while (0)

#define CW_COPY_OFFSET_XY_SRC(bx, by, x, y) do { \
    bx = x + src_off_x; \
    by = y + src_off_y; \
} while (0)

/* cw.c */
DrawablePtr
cwGetBackingDrawable(DrawablePtr pDrawable, int *x_off, int *y_off);

/* cw_render.c */

Bool
cwInitializeRender (ScreenPtr pScreen);

/* cw.c */
void
miInitializeCompositeWrapper(ScreenPtr pScreen);
