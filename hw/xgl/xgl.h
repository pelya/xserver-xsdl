/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the names of
 * David Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * David Reveman makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@freedesktop.org>
 */

#ifndef _XGL_H_
#define _XGL_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#define NEED_EVENTS
#include <X11/Xproto.h>
#include <X11/Xos.h>
#include <glitz.h>

#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "servermd.h"
#include "mi.h"
#include "dix.h"
#include "damage.h"
#include "gc.h"
/* I'd like gc.h to provide this */
typedef struct _GCFuncs *GCFuncsPtr;

#ifdef RENDER
#include "mipict.h"
#endif

extern WindowPtr *WindowTable;

typedef struct _xglScreenInfo {
    glitz_drawable_t *drawable;
    unsigned int     width;
    unsigned int     height;
    unsigned int     widthMm;
    unsigned int     heightMm;
    Bool	     fullscreen;
} xglScreenInfoRec, *xglScreenInfoPtr;

typedef struct _xglPixelFormat {
    CARD8		depth, bitsPerRGB;
    glitz_pixel_masks_t masks;
} xglPixelFormatRec, *xglPixelFormatPtr;

typedef struct _xglVisual {
    glitz_drawable_format_t *format;
    xglPixelFormatPtr       pPixel;
    unsigned long           visuals;
} xglVisualRec, *xglVisualPtr;

typedef struct _xglPixmapFormat {
    glitz_format_t    *format;
    xglPixelFormatPtr pPixel;
} xglPixmapFormatRec, *xglPixmapFormatPtr;

extern xglVisualPtr xglVisuals;
extern int	    nxglVisuals;

extern xglVisualPtr xglPbufferVisuals;
extern int	    nxglPbufferVisuals;

#define xglOffscreenAreaAvailable 0
#define xglOffscreenAreaDivided   1
#define xglOffscreenAreaOccupied  2

typedef struct _xglOffscreen *xglOffscreenPtr;
typedef struct _xglPixmap *xglPixmapPtr;

typedef struct _xglOffscreenArea {
    int			     level;
    int			     state;
    int			     x, y;
    xglPixmapPtr	     pPixmapPriv;
    struct _xglOffscreenArea *pArea[4];
    xglOffscreenPtr	     pOffscreen;
} xglOffscreenAreaRec, *xglOffscreenAreaPtr;

typedef struct _xglOffscreen {
    glitz_drawable_t	    *drawable;
    glitz_drawable_format_t *format;
    glitz_drawable_buffer_t buffer;
    int			    width, height;
    xglOffscreenAreaPtr     pArea;
} xglOffscreenRec;

typedef struct _xglScreen {
    xglVisualPtr		  pVisual;
    xglPixmapFormatRec		  pixmapFormats[33];
    glitz_drawable_t		  *drawable;
    glitz_surface_t		  *surface;
    glitz_surface_t		  *solid;
    PixmapPtr			  pScreenPixmap;
    unsigned long		  features;
    xglOffscreenPtr		  pOffscreen;
    int				  nOffscreen;
    
    GetImageProcPtr		  GetImage;
    GetSpansProcPtr		  GetSpans;
    CreateWindowProcPtr		  CreateWindow;
    PaintWindowBackgroundProcPtr  PaintWindowBackground;
    PaintWindowBorderProcPtr	  PaintWindowBorder;
    CopyWindowProcPtr		  CopyWindow;
    CreateGCProcPtr		  CreateGC;
    CloseScreenProcPtr		  CloseScreen;
    SetWindowPixmapProcPtr	  SetWindowPixmap;

#ifdef RENDER
    CompositeProcPtr		  Composite;
    RasterizeTrapezoidProcPtr	  RasterizeTrapezoid;
    GlyphsProcPtr		  Glyphs;
    ChangePictureProcPtr	  ChangePicture;
    ChangePictureTransformProcPtr ChangePictureTransform;
    ChangePictureFilterProcPtr	  ChangePictureFilter;
#endif

    BSFuncRec			  BackingStoreFuncs;
    
} xglScreenRec, *xglScreenPtr;

extern int xglScreenPrivateIndex;

#define XGL_GET_SCREEN_PRIV(pScreen) 				       \
    ((xglScreenPtr) (pScreen)->devPrivates[xglScreenPrivateIndex].ptr)

#define XGL_SET_SCREEN_PRIV(pScreen, v)				      \
    ((pScreen)->devPrivates[xglScreenPrivateIndex].ptr = (pointer) v)

#define XGL_SCREEN_PRIV(pScreen)			     \
    xglScreenPtr pScreenPriv = XGL_GET_SCREEN_PRIV (pScreen)

#define XGL_SCREEN_WRAP(field, wrapper)	 \
    pScreenPriv->field = pScreen->field; \
    pScreen->field     = wrapper

#define XGL_SCREEN_UNWRAP(field)	\
    pScreen->field = pScreenPriv->field

#ifdef RENDER
#define XGL_PICTURE_SCREEN_WRAP(field, wrapper)	   \
    pScreenPriv->field    = pPictureScreen->field; \
    pPictureScreen->field = wrapper

#define XGL_PICTURE_SCREEN_UNWRAP(field)       \
    pPictureScreen->field = pScreenPriv->field
#endif

#define xglGCSoftwareDrawableFlag (1L << 0)
#define xglGCReadOnlyDrawableFlag (1L << 1)
#define xglGCBadFunctionFlag	  (1L << 2)
#define xglGCPlaneMaskFlag	  (1L << 3)
    
typedef struct _xglGC {
    glitz_color_t    fg;
    glitz_color_t    bg;
    glitz_operator_t op;
    unsigned long    flags;
    GCFuncsPtr	     funcs;
    GCOpsPtr	     ops;
} xglGCRec, *xglGCPtr;

extern int xglGCPrivateIndex;

#define XGL_GET_GC_PRIV(pGC)				   \
    ((xglGCPtr) (pGC)->devPrivates[xglGCPrivateIndex].ptr)

#define XGL_GC_PRIV(pGC)		     \
    xglGCPtr pGCPriv = XGL_GET_GC_PRIV (pGC)

#define XGL_GC_WRAP(field, wrapper) \
    pGCPriv->field = pGC->field;    \
    pGC->field     = wrapper

#define XGL_GC_UNWRAP(field)    \
    pGC->field = pGCPriv->field


#define xglPCFillMask		(1L << 0)
#define xglPCFilterMask		(1L << 1)
#define xglPCTransformMask 	(1L << 2)
#define xglPCComponentAlphaMask (1L << 3)

#define xglPFFilterMask		(1L << 8)

#define xglPixmapTargetNo  0
#define xglPixmapTargetOut 1
#define xglPixmapTargetIn  2

typedef struct _xglPixmap {
    xglPixelFormatPtr	pPixel;
    glitz_format_t	*format;
    glitz_surface_t	*surface;
    glitz_buffer_t	*buffer;
    int			target;
    xglOffscreenAreaPtr pArea;
    int			score;
    Bool		acceleratedTile;
    pointer		bits;
    unsigned int	stride;
    DamagePtr		pDamage;
    BoxRec		damageBox;
    BoxRec		bitBox;
    Bool		allBits;
    unsigned long	pictureMask;
} xglPixmapRec;

extern int xglPixmapPrivateIndex;

#define XGL_GET_PIXMAP_PRIV(pPixmap)				       \
    ((xglPixmapPtr) (pPixmap)->devPrivates[xglPixmapPrivateIndex].ptr)

#define XGL_PIXMAP_PRIV(pPixmap)			     \
    xglPixmapPtr pPixmapPriv = XGL_GET_PIXMAP_PRIV (pPixmap)

#define XGL_PICTURE_CHANGES(pictureMask)  (pictureMask & 0x0000ffff)
#define XGL_PICTURE_FAILURES(pictureMask) (pictureMask & 0xffff0000)

typedef struct _xglWin {
    PixmapPtr pPixmap;
} xglWinRec, *xglWinPtr;

extern int xglWinPrivateIndex;

#define XGL_GET_WINDOW_PRIV(pWin)			      \
    ((xglWinPtr) (pWin)->devPrivates[xglWinPrivateIndex].ptr)

#define XGL_WINDOW_PRIV(pWin)			    \
    xglWinPtr pWinPriv = XGL_GET_WINDOW_PRIV (pWin)

#define XGL_GET_WINDOW_PIXMAP(pWin)		       \
    (XGL_GET_WINDOW_PRIV((WindowPtr) (pWin))->pPixmap)


#define XGL_GET_DRAWABLE_PIXMAP(pDrawable)   \
    (((pDrawable)->type == DRAWABLE_WINDOW)? \
     XGL_GET_WINDOW_PIXMAP (pDrawable):	     \
     (PixmapPtr) (pDrawable))

#define XGL_DRAWABLE_PIXMAP(pDrawable)			    \
    PixmapPtr pPixmap = XGL_GET_DRAWABLE_PIXMAP (pDrawable)

#define XGL_GET_DRAWABLE_PIXMAP_PRIV(pDrawable)		      \
    XGL_GET_PIXMAP_PRIV (XGL_GET_DRAWABLE_PIXMAP (pDrawable))

#define XGL_DRAWABLE_PIXMAP_PRIV(pDrawable)			        \
    xglPixmapPtr pPixmapPriv = XGL_GET_DRAWABLE_PIXMAP_PRIV (pDrawable)


typedef struct xglGeometry {
    glitz_buffer_t	       *buffer;
    pointer		       *data;
    glitz_geometry_primitive_t primitive;
    Bool		       broken;
    glitz_fixed16_16_t	       xOff, yOff;
    int			       dataType;
    int			       usage;
    int			       size, endOffset;
} xglGeometryRec, *xglGeometryPtr;


#ifdef COMPOSITE
#define __XGL_OFF_X_WIN(pPix) (-(pPix)->screen_x)
#define __XGL_OFF_Y_WIN(pPix) (-(pPix)->screen_y)
#else
#define __XGL_OFF_X_WIN(pPix) (0)
#define __XGL_OFF_Y_WIN(pPix) (0)
#endif

#define XGL_GET_DRAWABLE(pDrawable, pSurface, xOff, yOff)  \
    {							   \
	PixmapPtr _pPix;				   \
	if ((pDrawable)->type != DRAWABLE_PIXMAP) {	   \
	    _pPix = XGL_GET_WINDOW_PIXMAP (pDrawable);	   \
	    (xOff) = __XGL_OFF_X_WIN (_pPix);		   \
	    (yOff) = __XGL_OFF_Y_WIN (_pPix);		   \
	} else {					   \
	    _pPix = (PixmapPtr) (pDrawable);		   \
	    (yOff) = (xOff) = 0;			   \
	}						   \
	(pSurface) = XGL_GET_PIXMAP_PRIV (_pPix)->surface; \
    }

#define XGL_DEFAULT_DPI 96

#define XGL_INTERNAL_SCANLINE_ORDER GLITZ_PIXEL_SCANLINE_ORDER_TOP_DOWN

#define XGL_INTERNAL_SCANLINE_ORDER_UPSIDE_DOWN				  \
    (XGL_INTERNAL_SCANLINE_ORDER == GLITZ_PIXEL_SCANLINE_ORDER_BOTTOM_UP)

#define XGL_SW_FAILURE_STRING "software fall-back failure"

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define POWER_OF_TWO(v) ((v & (v - 1)) == 0)

#define MOD(a,b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))

#define BOX_NOTEMPTY(pBox)	      \
    (((pBox)->x2 - (pBox)->x1) > 0 && \
     ((pBox)->y2 - (pBox)->y1) > 0)

#define BOX_EXTENTS(pBox, nBox, pExt)	   \
    {					   \
	int i;				   \
	(pExt)->x1 = (pExt)->y1 = 32767;   \
	(pExt)->x2 = (pExt)->y2 = -32767;  \
	for (i = 0; i < (nBox); i++)	   \
	{				   \
	    if ((pBox)[i].x1 < (pExt)->x1) \
		(pExt)->x1 = (pBox)[i].x1; \
	    if ((pBox)[i].y1 < (pExt)->y1) \
		(pExt)->y1 = (pBox)[i].y1; \
	    if ((pBox)[i].x2 > (pExt)->x2) \
		(pExt)->x2 = (pBox)[i].x2; \
	    if ((pBox)[i].y2 > (pExt)->y2) \
		(pExt)->y2 = (pBox)[i].y2; \
	}				   \
	if (((pExt)->x2 - (pExt)->x1) < 0) \
	    (pExt)->x1 = (pExt)->x2 = 0;   \
	if (((pExt)->y2 - (pExt)->y1) < 0) \
	    (pExt)->y1 = (pExt)->y2 = 0;   \
    }

#define XGL_MAX_PIXMAP_SCORE  32768
#define XGL_MIN_PIXMAP_SCORE -32768

#define XGL_INCREMENT_PIXMAP_SCORE(pPixmapPriv, incr)	 \
    {							 \
	(pPixmapPriv)->score += (incr);			 \
	if ((pPixmapPriv)->score > XGL_MAX_PIXMAP_SCORE) \
	    (pPixmapPriv)->score = XGL_MAX_PIXMAP_SCORE; \
    }

#define XGL_DECREMENT_PIXMAP_SCORE(pPixmapPriv, decr)	 \
    {							 \
	(pPixmapPriv)->score -= (decr);			 \
	if ((pPixmapPriv)->score < XGL_MIN_PIXMAP_SCORE) \
	    (pPixmapPriv)->score = XGL_MIN_PIXMAP_SCORE; \
    }


/* xglinput.c */

int
xglMouseProc (DeviceIntPtr pDevice,
	      int	   onoff);

int
xglKeybdProc (DeviceIntPtr pDevice,
	      int	   onoff);

void
xglBell (int	      volume,
	 DeviceIntPtr pDev,
	 pointer      ctrl,
	 int	      something);

void
xglKbdCtrl (DeviceIntPtr pDevice,
	    KeybdCtrl	 *ctrl);

void
xglInitInput (int argc, char **argv);


/* xgloutput.c */

void
xglSetPixmapFormats (ScreenInfo *pScreenInfo);


/* xglcmap.c */

void
xglSetVisualTypesAndMasks (ScreenInfo	           *pScreenInfo,
			   glitz_drawable_format_t *format,
			   unsigned long           visuals);

void
xglInitVisuals (ScreenInfo *pScreenInfo);

void
xglClearVisualTypes (void);

void
xglInitPixmapFormats (ScreenPtr pScreen);

void
xglPixelToColor (xglPixelFormatPtr pFormat,
		 CARD32		   pixel,
		 glitz_color_t	   *color);


/* xglparse.c */

char *
xglParseFindNext (char *cur,
		  char *delim,
		  char *save,
		  char *last);

void
xglParseScreen (xglScreenInfoPtr pScreenInfo,
		char	         *arg);

void
xglUseMsg (void);

int
xglProcessArgument (xglScreenInfoPtr pScreenInfo,
		    int		     argc,
		    char	     **argv,
		    int		     i);


/* xglscreen.c */

Bool
xglScreenInit (ScreenPtr        pScreen,
	       xglScreenInfoPtr pScreenInfo);

Bool
xglFinishScreenInit (ScreenPtr pScreen);

Bool
xglCloseScreen (int	  index,
		ScreenPtr pScreen);


/* xgloffscreen.c */

Bool
xglInitOffscreen (ScreenPtr	   pScreen,
		  xglScreenInfoPtr pScreenInfo);

void
xglFiniOffscreen (ScreenPtr pScreen);

Bool
xglFindOffscreenArea (ScreenPtr pScreen,
		      PixmapPtr	pPixmap);

void
xglWithdrawOffscreenArea (xglOffscreenAreaPtr pArea);


/* xglgeometry.c */

#define GEOMETRY_DATA_TYPE_SHORT 0
#define GEOMETRY_DATA_TYPE_FLOAT 1

#define GEOMETRY_USAGE_STREAM  0
#define GEOMETRY_USAGE_STATIC  1
#define GEOMETRY_USAGE_DYNAMIC 2
#define GEOMETRY_USAGE_USERMEM 3

#define GEOMETRY_INIT(pScreen, pGeometry, _size)		 \
    {								 \
	(pGeometry)->dataType  = GEOMETRY_DATA_TYPE_FLOAT;	 \
	(pGeometry)->usage     = GEOMETRY_USAGE_USERMEM;	 \
	(pGeometry)->primitive = GLITZ_GEOMETRY_PRIMITIVE_QUADS; \
	(pGeometry)->size      = 0;				 \
	(pGeometry)->endOffset = 0;				 \
	(pGeometry)->data      = (pointer) 0;			 \
	(pGeometry)->buffer    = NULL;				 \
	(pGeometry)->broken    = FALSE;				 \
	(pGeometry)->xOff      = 0;				 \
	(pGeometry)->yOff      = 0;				 \
	xglGeometryResize (pScreen, pGeometry, _size);		 \
    }

#define GEOMETRY_UNINIT(pGeometry)			\
    {							\
	if ((pGeometry)->buffer)			\
	    glitz_buffer_destroy ((pGeometry)->buffer); \
	if ((pGeometry)->data)				\
	    xfree ((pGeometry)->data);			\
    }

#define GEOMETRY_SET_PRIMITIVE(pScreen, pGeometry, _primitive) \
    (pGeometry)->primitive = _primitive

#define GEOMETRY_RESIZE(pScreen, pGeometry, size) \
    xglGeometryResize (pScreen, pGeometry, size)

#define GEOMETRY_TRANSLATE(pGeometry, tx, ty) \
    {				              \
	(pGeometry)->xOff += (tx) << 16;      \
	(pGeometry)->yOff += (ty) << 16;      \
    }

#define GEOMETRY_TRANSLATE_FIXED(pGeometry, ftx, fty) \
    {						      \
	(pGeometry)->xOff += (ftx);		      \
	(pGeometry)->yOff += (fty);		      \
    }

#define GEOMETRY_ADD_RECT(pScreen, pGeometry, pRect, nRect) \
    xglGeometryAddRect (pScreen, pGeometry, pRect, nRect)

#define GEOMETRY_ADD_BOX(pScreen, pGeometry, pBox, nBox) \
    xglGeometryAddBox (pScreen, pGeometry, pBox, nBox)

#define GEOMETRY_ADD_REGION(pScreen, pGeometry, pRegion) \
    xglGeometryAddBox (pScreen, pGeometry,		 \
		       REGION_RECTS (pRegion),		 \
		       REGION_NUM_RECTS (pRegion))

#define GEOMETRY_ADD_SPAN(pScreen, pGeometry, ppt, pwidth, n) \
    xglGeometryAddSpan (pScreen, pGeometry, ppt, pwidth, n)

#define GEOMETRY_ADD_LINE(pScreen, pGeometry, loop, mode, npt, ppt) \
    xglGeometryAddLine (pScreen, pGeometry, loop, mode, npt, ppt)

#define GEOMETRY_ADD_SEGMENT(pScreen, pGeometry, nsegInit, pSegInit) \
    xglGeometryAddSegment (pScreen, pGeometry, nsegInit, pSegInit)

#define GEOMETRY_ENABLE(pGeometry, surface, first, count) \
    xglSetGeometry (pGeometry, surface, first, count);

#define GEOMETRY_ENABLE_ALL_VERTICES(pGeometry, surface)	       \
    xglSetGeometry (pGeometry, surface, 0, (pGeometry)->endOffset / 2)

#define GEOMETRY_DISABLE(surface)		   \
    glitz_set_geometry (surface, 0, 0, NULL, NULL)

void
xglGeometryResize (ScreenPtr	  pScreen,
		   xglGeometryPtr pGeometry,
		   int		  size);

void
xglGeometryAddRect (ScreenPtr	   pScreen,
		    xglGeometryPtr pGeometry,
		    xRectangle	   *pRect,
		    int		   nRect);

void
xglGeometryAddBox (ScreenPtr	  pScreen,
		   xglGeometryPtr pGeometry,
		   BoxPtr	  pBox,
		   int		  nBox);

void
xglGeometryAddSpan (ScreenPtr	   pScreen,
		    xglGeometryPtr pGeometry,
		    DDXPointPtr	   ppt,
		    int		   *pwidth,
		    int		   n);

void
xglGeometryAddLine (ScreenPtr	   pScreen,
		    xglGeometryPtr pGeometry,
		    int		   loop,
		    int		   mode,
		    int		   npt,
		    DDXPointPtr    ppt);

void
xglGeometryAddSegment (ScreenPtr      pScreen,
		       xglGeometryPtr pGeometry,
		       int	      nsegInit,
		       xSegment       *pSegInit);

Bool
xglSetGeometry (xglGeometryPtr 	pGeometry,
		glitz_surface_t *surface,
		int		first,
		int		count);


/* xglpixmap.c */

PixmapPtr
xglCreatePixmap (ScreenPtr  pScreen,
		 int	    width,
		 int	    height, 
		 int	    depth);

Bool
xglDestroyPixmap (PixmapPtr pPixmap);

Bool
xglModifyPixmapHeader (PixmapPtr pPixmap,
		       int	 width,
		       int	 height,
		       int	 depth,
		       int	 bitsPerPixel,
		       int	 devKind,
		       pointer	 pPixData);

Bool
xglCreatePixmapSurface (PixmapPtr pPixmap);

Bool
xglAllocatePixmapBits (PixmapPtr pPixmap);

Bool
xglMapPixmapBits (PixmapPtr pPixmap);

Bool
xglUnmapPixmapBits (PixmapPtr pPixmap);


/* xglsync.c */

Bool
xglSyncBits (DrawablePtr pDrawable,
	     BoxPtr	 pExtents);

void
xglSyncDamageBoxBits (DrawablePtr pDrawable);

Bool
xglSyncSurface (DrawablePtr pDrawable);

Bool
xglPrepareTarget (DrawablePtr pDrawable);

void
xglAddSurfaceDamage (DrawablePtr pDrawable);

void
xglAddBitDamage (DrawablePtr pDrawable);


/* xglsolid.c */

Bool
xglSolid (DrawablePtr	   pDrawable,
	  glitz_operator_t op,
	  glitz_color_t	   *color,
	  xglGeometryPtr   pGeometry,
	  BoxPtr	   pBox,
	  int		   nBox);


/* xgltile.c */

Bool
xglTile (DrawablePtr	  pDrawable,
	 glitz_operator_t op,
	 PixmapPtr	  pTile,
	 int		  tileX,
	 int		  tileY,
	 xglGeometryPtr	  pGeometry,
	 BoxPtr		  pBox,
	 int		  nBox);

#define TILE_SOURCE 0
#define TILE_MASK   1

void
xglSwTile (glitz_operator_t op,
	   glitz_surface_t  *srcSurface,
	   glitz_surface_t  *maskSurface,
	   glitz_surface_t  *dstSurface,
	   int		    xSrc,
	   int		    ySrc,
	   int		    xMask,
	   int		    yMask,
	   int		    what,
	   BoxPtr	    pBox,
	   int		    nBox,
	   int		    xOff,
	   int		    yOff);


/* xglpixel.c */

Bool
xglSetPixels (DrawablePtr pDrawable,
	      char        *src,
	      int	  stride,
	      int	  x,
	      int	  y,
	      int	  width,
	      int	  height,
	      BoxPtr	  pBox,
	      int	  nBox);


/* xglcopy.c */

Bool
xglCopy (DrawablePtr pSrc,
	 DrawablePtr pDst,
	 int	     dx,
	 int	     dy,
	 BoxPtr	     pBox,
	 int	     nBox);

void
xglCopyProc (DrawablePtr pSrc,
	     DrawablePtr pDst,
	     GCPtr	 pGC,
	     BoxPtr	 pBox,
	     int	 nBox,
	     int	 dx,
	     int	 dy,
	     Bool	 reverse,
	     Bool	 upsidedown,
	     Pixel	 bitplane,
	     void	 *closure);


/* xglfill.c */

Bool
xglFill (DrawablePtr    pDrawable,
	 GCPtr	        pGC,
	 xglGeometryPtr pGeometry);

Bool
xglFillSpan (DrawablePtr pDrawable,
	     GCPtr	 pGC,
	     int	 n,
	     DDXPointPtr ppt,
	     int	 *pwidth);

Bool
xglFillRect (DrawablePtr pDrawable, 
	     GCPtr	 pGC, 
	     int	 nrect,
	     xRectangle  *prect);

Bool
xglFillLine (DrawablePtr pDrawable,
	     GCPtr       pGC,
	     int	 mode,
	     int	 npt,
	     DDXPointPtr ppt);

Bool
xglFillSegment (DrawablePtr pDrawable,
		GCPtr	    pGC, 
		int	    nsegInit,
		xSegment    *pSegInit);


/* xglwindow.c */

Bool
xglCreateWindow (WindowPtr pWin);

void 
xglCopyWindow (WindowPtr   pWin, 
	       DDXPointRec ptOldOrg, 
	       RegionPtr   prgnSrc);

void
xglPaintWindowBackground (WindowPtr pWin,
			  RegionPtr pRegion,
			  int	    what);

void
xglPaintWindowBorder (WindowPtr pWin,
		      RegionPtr pRegion,
		      int	what);


/* xglbstore.c */

void
xglSaveAreas (PixmapPtr	pPixmap,
	      RegionPtr	prgnSave,
	      int	xorg,
	      int	yorg,
	      WindowPtr	pWin);

void
xglRestoreAreas (PixmapPtr pPixmap,
		 RegionPtr prgnRestore,
		 int	   xorg,
		 int	   yorg,
		 WindowPtr pWin);


/* xglget.c */

void
xglGetImage (DrawablePtr   pDrawable,
	     int	   x,
	     int	   y,
	     int	   w,
	     int	   h,
	     unsigned int  format,
	     unsigned long planeMask,
	     char	   *d);

void
xglGetSpans (DrawablePtr pDrawable, 
	     int	 wMax, 
	     DDXPointPtr ppt, 
	     int	 *pwidth, 
	     int	 nspans, 
	     char	 *pchardstStart);


/* xglgc.c */

Bool
xglCreateGC (GCPtr pGC);

void
xglValidateGC (GCPtr	     pGC,
	       unsigned long changes,
	       DrawablePtr   pDrawable);

void
xglFillSpans  (DrawablePtr pDrawable,
	       GCPtr	   pGC,
	       int	   nspans,
	       DDXPointPtr ppt,
	       int	   *pwidth,
	       int	   fSorted);

void
xglSetSpans (DrawablePtr pDrawable,
	     GCPtr	 pGC,
	     char	 *psrc,
	     DDXPointPtr ppt,
	     int	 *pwidth,
	     int	 nspans,
	     int	 fSorted);

void
xglPutImage (DrawablePtr pDrawable,
	     GCPtr	 pGC,
	     int	 depth,
	     int	 x,
	     int	 y,
	     int	 w,
	     int	 h,
	     int	 leftPad,
	     int	 format,
	     char	 *bits);

RegionPtr
xglCopyArea (DrawablePtr pSrc,
	     DrawablePtr pDst,
	     GCPtr	 pGC,
	     int	 srcX,
	     int	 srcY,
	     int	 w,
	     int	 h,
	     int	 dstX,
	     int	 dstY);

RegionPtr
xglCopyPlane (DrawablePtr   pSrc,
	      DrawablePtr   pDst,
	      GCPtr	    pGC,
	      int	    srcX,
	      int	    srcY,
	      int	    w,
	      int	    h,
	      int	    dstX,
	      int	    dstY,
	      unsigned long bitPlane);

void
xglPolyPoint (DrawablePtr pDrawable,
	      GCPtr       pGC,
	      int	  mode,
	      int	  npt,
	      DDXPointPtr pptInit);

void
xglPolylines (DrawablePtr pDrawable,
	      GCPtr       pGC,
	      int	  mode,
	      int	  npt,
	      DDXPointPtr ppt);

void
xglPolySegment (DrawablePtr pDrawable,
		GCPtr	    pGC, 
		int	    nsegInit,
		xSegment    *pSegInit);

void
xglPolyArc (DrawablePtr pDrawable,
	    GCPtr	pGC, 
	    int		narcs,
	    xArc	*pArcs);

void
xglPolyFillRect (DrawablePtr pDrawable,
		 GCPtr	     pGC,
		 int	     nrect,
		 xRectangle  *prect);

void
xglPolyFillArc (DrawablePtr pDrawable,
		GCPtr	    pGC, 
		int	    narcs,
		xArc	    *pArcs);

void
xglImageGlyphBlt (DrawablePtr  pDrawable,
		  GCPtr	       pGC,
		  int	       x,
		  int	       y,
		  unsigned int nglyph,
		  CharInfoPtr  *ppci,
		  pointer      pglyphBase);

void
xglPolyGlyphBlt (DrawablePtr  pDrawable,
		 GCPtr	      pGC,
		 int	      x,
		 int	      y,
		 unsigned int nglyph,
		 CharInfoPtr  *ppci,
		 pointer      pglyphBase);
void
xglPushPixels (GCPtr	   pGC,
	       PixmapPtr   pBitmap,
	       DrawablePtr pDrawable,
	       int	   w,
	       int	   h,
	       int	   x,
	       int	   y);


#ifdef RENDER

/* xglcomp.c */

Bool
xglComp (CARD8	    op,
	 PicturePtr pSrc,
	 PicturePtr pMask,
	 PicturePtr pDst,
	 INT16	    xSrc,
	 INT16	    ySrc,
	 INT16	    xMask,
	 INT16	    yMask,
	 INT16	    xDst,
	 INT16	    yDst,
	 CARD16	    width,
	 CARD16	    height);


/* xglpict.c */

void
xglComposite (CARD8	 op,
	      PicturePtr pSrc,
	      PicturePtr pMask,
	      PicturePtr pDst,
	      INT16	 xSrc,
	      INT16	 ySrc,
	      INT16	 xMask,
	      INT16	 yMask,
	      INT16	 xDst,
	      INT16	 yDst,
	      CARD16	 width,
	      CARD16	 height);

void
xglGlyphs (CARD8	 op,
	   PicturePtr	 pSrc,
	   PicturePtr	 pDst,
	   PictFormatPtr maskFormat,
	   INT16	 xSrc,
	   INT16	 ySrc,
	   int		 nlist,
	   GlyphListPtr	 list,
	   GlyphPtr	 *glyphs);

void
xglRasterizeTrapezoid (PicturePtr pDst,
		       xTrapezoid *trap,
		       int	  xOff,
		       int	  yOff);

void
xglChangePicture (PicturePtr pPicture,
		  Mask	     mask);

int
xglChangePictureTransform (PicturePtr    pPicture,
			   PictTransform *transform);

int
xglChangePictureFilter (PicturePtr pPicture,
			int	   filter,
			xFixed	   *params,
			int	   nparams);

void
xglUpdatePicture (PicturePtr pPicture);

#endif

#endif /* _XGL_H_ */
