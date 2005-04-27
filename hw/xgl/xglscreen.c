/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
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
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"
#include "inputstr.h"
#include "mipointer.h"
#include "damage.h"
#include "fb.h"
#ifdef MITSHM
#include "shmint.h"
static ShmFuncs shmFuncs = { NULL, xglShmPutImage };
#endif
#ifdef RENDER
#include "glyphstr.h"
#endif
#ifdef COMPOSITE
#include "compint.h"
#endif

int xglScreenGeneration = -1;
int xglScreenPrivateIndex;
int xglGCPrivateIndex;
int xglPixmapPrivateIndex;
int xglWinPrivateIndex;

#ifdef RENDER
int xglGlyphPrivateIndex;
#endif

#define xglQueryBestSize	  (void *) NoopDDA
#define xglSaveScreen		  (void *) NoopDDA

#define xglConstrainCursor	  (void *) NoopDDA
#define xglCursorLimits		  (void *) NoopDDA
#define xglDisplayCursor	  (void *) NoopDDA
#define xglRealizeCursor	  (void *) NoopDDA
#define xglUnrealizeCursor	  (void *) NoopDDA
#define xglRecolorCursor	  (void *) NoopDDA
#define xglSetCursorPosition	  (void *) NoopDDA

static PixmapPtr
xglGetWindowPixmap (WindowPtr pWin)
{
    return XGL_GET_WINDOW_PIXMAP (pWin);
}

static void
xglSetWindowPixmap (WindowPtr pWin,
		    PixmapPtr pPixmap)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    XGL_SCREEN_PRIV (pScreen);
    
    XGL_SCREEN_UNWRAP (SetWindowPixmap);
    (*pScreen->SetWindowPixmap) (pWin, pPixmap);
    XGL_SCREEN_WRAP (SetWindowPixmap, xglSetWindowPixmap);
    
    XGL_GET_WINDOW_PRIV(pWin)->pPixmap = pPixmap;
}

static Bool
xglAllocatePrivates (ScreenPtr pScreen)
{
    xglScreenPtr pScreenPriv;

    if (xglScreenGeneration != serverGeneration)
    {
	xglScreenPrivateIndex = AllocateScreenPrivateIndex ();
	if (xglScreenPrivateIndex < 0)
	    return FALSE;
	
	xglGCPrivateIndex = AllocateGCPrivateIndex ();
	if (xglGCPrivateIndex < 0)
	    return FALSE;

	xglPixmapPrivateIndex = AllocatePixmapPrivateIndex ();
	if (xglPixmapPrivateIndex < 0)
	    return FALSE;

	xglWinPrivateIndex = AllocateWindowPrivateIndex ();
	if (xglWinPrivateIndex < 0)
	    return FALSE;

#ifdef RENDER	
	xglGlyphPrivateIndex = AllocateGlyphPrivateIndex ();
	if (xglGlyphPrivateIndex < 0)
	    return FALSE;
#endif	
	
 	xglScreenGeneration = serverGeneration;
    }

    if (!AllocateGCPrivate (pScreen, xglGCPrivateIndex, sizeof (xglGCRec)))
	return FALSE;

    if (!AllocatePixmapPrivate (pScreen, xglPixmapPrivateIndex,
				sizeof (xglPixmapRec)))
	return FALSE;

    if (!AllocateWindowPrivate (pScreen, xglWinPrivateIndex,
				sizeof (xglWinRec)))
	return FALSE;

    pScreenPriv = xalloc (sizeof (xglScreenRec));
    if (!pScreenPriv)
	return FALSE;
    
    XGL_SET_SCREEN_PRIV (pScreen, pScreenPriv);
    
    return TRUE;
}

Bool
xglScreenInit (ScreenPtr        pScreen,
	       xglScreenInfoPtr pScreenInfo)
{
    xglScreenPtr pScreenPriv;
    int		 depth, bpp;
    
#ifdef RENDER
    PictureScreenPtr pPictureScreen;
#endif
    
    if (!xglAllocatePrivates (pScreen))
	return FALSE;

    pScreenPriv = XGL_GET_SCREEN_PRIV (pScreen);

    pScreenPriv->pScreenPixmap = NULL;
    
    pScreenPriv->pVisual  = &xglVisuals[0];
    pScreenPriv->drawable = pScreenInfo->drawable;
    pScreenPriv->features =
	glitz_drawable_get_features (pScreenInfo->drawable);

    depth = pScreenPriv->pVisual->pPixel->depth;
    bpp   = pScreenPriv->pVisual->pPixel->masks.bpp;

    if (!xglInitOffscreen (pScreen, pScreenInfo))
	return FALSE;
    
    xglInitPixmapFormats (pScreen);
    if (!pScreenPriv->pixmapFormats[depth].format)
	return FALSE;
    
    pScreenPriv->geometryDataType = pScreenInfo->geometryDataType;
    pScreenPriv->geometryUsage    = pScreenInfo->geometryUsage;
    pScreenPriv->yInverted	  = pScreenInfo->yInverted;
    pScreenPriv->pboMask	  = pScreenInfo->pboMask;
    pScreenPriv->lines		  = pScreenInfo->lines;

    GEOMETRY_INIT (pScreen, &pScreenPriv->scratchGeometry,
		   GLITZ_GEOMETRY_TYPE_VERTEX,
		   pScreenPriv->geometryUsage, 0);
    
    pScreenPriv->surface =
	glitz_surface_create (pScreenPriv->drawable,
			      pScreenPriv->pixmapFormats[depth].format,
			      pScreenInfo->width, pScreenInfo->height,
			      0, NULL);
    if (!pScreenPriv->surface)
	return FALSE;

    glitz_surface_attach (pScreenPriv->surface,
			  pScreenPriv->drawable,
			  GLITZ_DRAWABLE_BUFFER_FRONT_COLOR,
			  0, 0);

    if (monitorResolution == 0)
	monitorResolution = XGL_DEFAULT_DPI;

    if (!fbSetupScreen (pScreen, NULL,
			pScreenInfo->width, pScreenInfo->height,
			monitorResolution, monitorResolution,
			pScreenInfo->width, bpp))
	return FALSE;

    pScreen->SaveScreen = xglSaveScreen;
    
    pScreen->CreatePixmap  = xglCreatePixmap;
    pScreen->DestroyPixmap = xglDestroyPixmap;

    if (!fbFinishScreenInit (pScreen, NULL,
			     pScreenInfo->width, pScreenInfo->height,
			     monitorResolution, monitorResolution,
			     pScreenInfo->width, bpp))
	return FALSE;

#ifdef MITSHM
    ShmRegisterFuncs (pScreen, &shmFuncs);
#endif

#ifdef RENDER
    if (!xglPictureInit (pScreen))
	return FALSE;
#endif

    XGL_SCREEN_WRAP (GetImage, xglGetImage);
    XGL_SCREEN_WRAP (GetSpans, xglGetSpans);
    
    XGL_SCREEN_WRAP (CopyWindow, xglCopyWindow);
    XGL_SCREEN_WRAP (CreateWindow, xglCreateWindow);
    XGL_SCREEN_WRAP (ChangeWindowAttributes, xglChangeWindowAttributes);
    XGL_SCREEN_WRAP (PaintWindowBackground, xglPaintWindowBackground);
    XGL_SCREEN_WRAP (PaintWindowBorder, xglPaintWindowBorder);

    XGL_SCREEN_WRAP (CreateGC, xglCreateGC);

    pScreen->ConstrainCursor   = xglConstrainCursor;
    pScreen->CursorLimits      = xglCursorLimits;
    pScreen->DisplayCursor     = xglDisplayCursor;
    pScreen->RealizeCursor     = xglRealizeCursor;
    pScreen->UnrealizeCursor   = xglUnrealizeCursor;
    pScreen->RecolorCursor     = xglRecolorCursor;
    pScreen->SetCursorPosition = xglSetCursorPosition;
    
    pScreen->ModifyPixmapHeader = xglModifyPixmapHeader;
    
    XGL_SCREEN_WRAP (BitmapToRegion, xglPixmapToRegion);

    pScreen->GetWindowPixmap = xglGetWindowPixmap;
    
    XGL_SCREEN_WRAP (SetWindowPixmap, xglSetWindowPixmap);

#ifdef RENDER
    pPictureScreen = GetPictureScreenIfSet (pScreen);
    if (pPictureScreen)
    {
	if (!AllocateGlyphPrivate (pScreen, xglGlyphPrivateIndex,
				   sizeof (xglGlyphRec)))
	    return FALSE;
	
	XGL_PICTURE_SCREEN_WRAP (RealizeGlyph, xglRealizeGlyph);
	XGL_PICTURE_SCREEN_WRAP (UnrealizeGlyph, xglUnrealizeGlyph);
	XGL_PICTURE_SCREEN_WRAP (Composite, xglComposite);
	XGL_PICTURE_SCREEN_WRAP (Glyphs, xglGlyphs);
	XGL_PICTURE_SCREEN_WRAP (Trapezoids, xglTrapezoids);
	XGL_PICTURE_SCREEN_WRAP (AddTraps, xglAddTraps);
	XGL_PICTURE_SCREEN_WRAP (AddTriangles, xglAddTriangles);
	XGL_PICTURE_SCREEN_WRAP (ChangePicture, xglChangePicture);
	XGL_PICTURE_SCREEN_WRAP (ChangePictureTransform,
				 xglChangePictureTransform);
	XGL_PICTURE_SCREEN_WRAP (ChangePictureFilter, xglChangePictureFilter);
    }
#endif

    XGL_SCREEN_WRAP (BackingStoreFuncs.SaveAreas, xglSaveAreas);
    XGL_SCREEN_WRAP (BackingStoreFuncs.RestoreAreas, xglRestoreAreas);

    if (!fbCreateDefColormap (pScreen))
	return FALSE;

#ifdef COMPOSITE
    if (!compScreenInit (pScreen))
	return FALSE;
#endif

#ifdef GLXEXT
    if (!xglInitVisualConfigs (pScreen))
	return FALSE;
#endif
    
    /* Damage is required */
    DamageSetup (pScreen);

    XGL_SCREEN_WRAP (CloseScreen, xglCloseScreen);

    return TRUE;
}

Bool
xglFinishScreenInit (ScreenPtr pScreen)
{
	
#ifdef RENDER
    glitz_vertex_format_t *format;
    static glitz_color_t  clearBlack = { 0x0, 0x0, 0x0, 0x0 };
    static glitz_color_t  solidWhite = { 0xffff, 0xffff, 0xffff, 0xffff };
    int			  i;
#endif

    XGL_SCREEN_PRIV (pScreen);
	
    pScreenPriv->solid =
	glitz_surface_create (pScreenPriv->drawable,
			      pScreenPriv->pixmapFormats[32].format,
			      1, 1, 0, NULL);
    if (!pScreenPriv->solid)
	return FALSE;
    
    glitz_surface_set_fill (pScreenPriv->solid, GLITZ_FILL_REPEAT);

#ifdef RENDER
    for (i = 0; i < 33; i++)
	pScreenPriv->glyphCache[i].pScreen = NULL;

    pScreenPriv->pSolidAlpha = NULL;

    pScreenPriv->trapInfo.mask =
	glitz_surface_create (pScreenPriv->drawable,
			      pScreenPriv->pixmapFormats[8].format,
			      2, 1, 0, NULL);
    if (!pScreenPriv->trapInfo.mask)
	return FALSE;

    glitz_set_rectangle (pScreenPriv->trapInfo.mask, &clearBlack, 0, 0, 1, 1);
    glitz_set_rectangle (pScreenPriv->trapInfo.mask, &solidWhite, 1, 0, 1, 1);
	
    glitz_surface_set_fill (pScreenPriv->trapInfo.mask, GLITZ_FILL_NEAREST);
    glitz_surface_set_filter (pScreenPriv->trapInfo.mask,
			      GLITZ_FILTER_BILINEAR,
			      NULL, 0);

    format = &pScreenPriv->trapInfo.format.vertex; 
    format->primitive  = GLITZ_PRIMITIVE_QUADS;
    format->attributes = GLITZ_VERTEX_ATTRIBUTE_MASK_COORD_MASK;

    format->mask.type	     = GLITZ_DATA_TYPE_FLOAT;
    format->mask.size	     = GLITZ_COORDINATE_SIZE_X;
    format->bytes_per_vertex = sizeof (glitz_float_t);

    if (pScreenPriv->geometryDataType)
    {
	format->type		  = GLITZ_DATA_TYPE_FLOAT;
	format->bytes_per_vertex += 2 * sizeof (glitz_float_t);
	format->mask.offset	  = 2 * sizeof (glitz_float_t);
    }
    else
    {
	format->type		  = GLITZ_DATA_TYPE_SHORT;
	format->bytes_per_vertex += 2 * sizeof (glitz_short_t);
	format->mask.offset	  = 2 * sizeof (glitz_short_t);
    }
#endif
    
    return TRUE;
}

Bool
xglCloseScreen (int	  index,
		ScreenPtr pScreen)
{
    XGL_SCREEN_PRIV (pScreen);
    XGL_PIXMAP_PRIV (pScreenPriv->pScreenPixmap);
    
#ifdef RENDER
    int i;
    
    for (i = 0; i < 33; i++)
	xglFiniGlyphCache (&pScreenPriv->glyphCache[i]);

    if (pScreenPriv->pSolidAlpha)
	FreePicture ((pointer) pScreenPriv->pSolidAlpha, 0);

    if (pScreenPriv->trapInfo.mask)
	glitz_surface_destroy (pScreenPriv->trapInfo.mask);
#endif

    xglFiniPixmap (pScreenPriv->pScreenPixmap);
    if (pPixmapPriv->pDamage)
	DamageDestroy (pPixmapPriv->pDamage);

    if (pScreenPriv->solid)
	glitz_surface_destroy (pScreenPriv->solid);

    if (pScreenPriv->surface)
	glitz_surface_destroy (pScreenPriv->surface);

    xglFiniOffscreen (pScreen);

    GEOMETRY_UNINIT (&pScreenPriv->scratchGeometry);

    XGL_SCREEN_UNWRAP (CloseScreen);
    xfree (pScreenPriv);

    return (*pScreen->CloseScreen) (index, pScreen);
}

#ifdef RENDER
void
xglCreateSolidAlphaPicture (ScreenPtr pScreen)
{
    static xRenderColor	solidWhite = { 0xffff, 0xffff, 0xffff, 0xffff };
    static xRectangle	one = { 0, 0, 1, 1 };
    PixmapPtr		pPixmap;
    PictFormatPtr	pFormat;
    int			error;
    Pixel		pixel;
    GCPtr		pGC;
    CARD32		tmpval[2];

    XGL_SCREEN_PRIV (pScreen);

    pFormat = PictureMatchFormat (pScreen, 32, PICT_a8r8g8b8);
    if (!pFormat)
	return;

    pGC = GetScratchGC (pFormat->depth, pScreen);
    if (!pGC)
	return;
	
    pPixmap = (*pScreen->CreatePixmap) (pScreen, 1, 1, pFormat->depth);
    if (!pPixmap)
	return;
	
    miRenderColorToPixel (pFormat, &solidWhite, &pixel);
    
    tmpval[0] = GXcopy;
    tmpval[1] = pixel;

    ChangeGC (pGC, GCFunction | GCForeground, tmpval);
    ValidateGC (&pPixmap->drawable, pGC);	
    (*pGC->ops->PolyFillRect) (&pPixmap->drawable, pGC, 1, &one);
    FreeScratchGC (pGC);
    
    tmpval[0] = xTrue;
    pScreenPriv->pSolidAlpha =
	CreatePicture (0, &pPixmap->drawable, pFormat,
		       CPRepeat, tmpval, 0, &error);
    (*pScreen->DestroyPixmap) (pPixmap);

    if (pScreenPriv->pSolidAlpha)
	ValidatePicture (pScreenPriv->pSolidAlpha);
}
#endif
