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

#include "xgl.h"
#include "inputstr.h"
#include "mipointer.h"
#include "damage.h"
#include "fb.h"

int xglScreenGeneration = -1;
int xglScreenPrivateIndex;
int xglGCPrivateIndex;
int xglPixmapPrivateIndex;
int xglWinPrivateIndex;

#define xglQueryBestSize	  (void *) NoopDDA
#define xglSaveScreen		  (void *) NoopDDA

#define xglRealizeFont		  (void *) NoopDDA
#define xglUnrealizeFont	  (void *) NoopDDA

#define xglConstrainCursor	  (void *) NoopDDA
#define xglCursorLimits		  (void *) NoopDDA
#define xglDisplayCursor	  (void *) NoopDDA
#define xglRealizeCursor	  (void *) NoopDDA
#define xglUnrealizeCursor	  (void *) NoopDDA
#define xglRecolorCursor	  (void *) NoopDDA
#define xglSetCursorPosition	  (void *) NoopDDA

#define xglCreateColormap	  (void *) NoopDDA
#define xglDestroyColormap	  (void *) NoopDDA
#define xglInstallColormap	  (void *) NoopDDA
#define xglUninstallColormap	  (void *) NoopDDA
#define xglListInstalledColormaps (void *) NoopDDA
#define xglStoreColors		  (void *) NoopDDA
#define xglResolveColor		  (void *) NoopDDA
#define xglBitmapToRegion	  (void *) NoopDDA

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
    xglScreenPtr   pScreenPriv;
    
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

    if (!xglInitOffscreen (pScreen, pScreenInfo))
	return FALSE;
    
    xglInitPixmapFormats (pScreen);
    if (!pScreenPriv->pixmapFormats[32].format)
	return FALSE;

    pScreenPriv->surface =
	glitz_surface_create (pScreenPriv->drawable,
			      pScreenPriv->pixmapFormats[32].format,
			      pScreenInfo->width, pScreenInfo->height);
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
			pScreenInfo->width,
			pScreenPriv->pVisual->pPixel->masks.bpp))
	return FALSE;

    pScreen->SaveScreen = xglSaveScreen;
    
    pScreen->CreatePixmap  = xglCreatePixmap;
    pScreen->DestroyPixmap = xglDestroyPixmap;

    if (!fbFinishScreenInit (pScreen, NULL,
			     pScreenInfo->width, pScreenInfo->height,
			     monitorResolution, monitorResolution,
			     pScreenInfo->width,
			     pScreenPriv->pVisual->pPixel->masks.bpp))
	return FALSE;

#ifdef RENDER
    if (!fbPictureInit (pScreen, 0, 0))
	return FALSE;
#endif

    XGL_SCREEN_WRAP (GetImage, xglGetImage);
    XGL_SCREEN_WRAP (GetSpans, xglGetSpans);
    
    XGL_SCREEN_WRAP (CopyWindow, xglCopyWindow);
    XGL_SCREEN_WRAP (CreateWindow, xglCreateWindow);
    XGL_SCREEN_WRAP (PaintWindowBackground, xglPaintWindowBackground);
    XGL_SCREEN_WRAP (PaintWindowBorder, xglPaintWindowBorder);

    /*
      pScreen->RealizeFont   = xglRealizeFont;
      pScreen->UnrealizeFont = xglUnrealizeFont;
    */
    
    XGL_SCREEN_WRAP (CreateGC, xglCreateGC);

    pScreen->ConstrainCursor   = xglConstrainCursor;
    pScreen->CursorLimits      = xglCursorLimits;
    pScreen->DisplayCursor     = xglDisplayCursor;
    pScreen->RealizeCursor     = xglRealizeCursor;
    pScreen->UnrealizeCursor   = xglUnrealizeCursor;
    pScreen->RecolorCursor     = xglRecolorCursor;
    pScreen->SetCursorPosition = xglSetCursorPosition;
    
    /*
      pScreen->CreateColormap	      = miInitializeColormap;
      pScreen->DestroyColormap	      = xglDestroyColormap;
      pScreen->InstallColormap	      = miInstallColormap;
      pScreen->UninstallColormap      = miUninstallColormap;
      pScreen->ListInstalledColormaps = miListInstalledColormaps;
      pScreen->StoreColors	      = xglStoreColors;
      pScreen->ResolveColor	      = miResolveColor;
    */
    
    /*
      pScreen->BitmapToRegion = xglBitmapToRegion;
    */

    pScreen->ModifyPixmapHeader = xglModifyPixmapHeader;

    pScreen->GetWindowPixmap = xglGetWindowPixmap;
    
    XGL_SCREEN_WRAP (SetWindowPixmap, xglSetWindowPixmap);

#ifdef RENDER
    pPictureScreen = GetPictureScreenIfSet (pScreen);
    if (pPictureScreen) {
	XGL_PICTURE_SCREEN_WRAP (Composite, xglComposite);
	XGL_PICTURE_SCREEN_WRAP (Glyphs, xglGlyphs);
	XGL_PICTURE_SCREEN_WRAP (RasterizeTrapezoid, xglRasterizeTrapezoid);
	XGL_PICTURE_SCREEN_WRAP (ChangePicture, xglChangePicture);
	XGL_PICTURE_SCREEN_WRAP (ChangePictureTransform,
				 xglChangePictureTransform);
	XGL_PICTURE_SCREEN_WRAP (ChangePictureFilter, xglChangePictureFilter);
    }
#endif

    XGL_SCREEN_WRAP (BackingStoreFuncs.SaveAreas, xglSaveAreas);
    XGL_SCREEN_WRAP (BackingStoreFuncs.RestoreAreas, xglRestoreAreas);

    /* Damage is required */
    DamageSetup (pScreen);

    XGL_SCREEN_WRAP (CloseScreen, xglCloseScreen);

    return TRUE;
}

Bool
xglFinishScreenInit (ScreenPtr pScreen)
{
    XGL_SCREEN_PRIV (pScreen);
	
    miInitializeBackingStore (pScreen);

    if (!fbCreateDefColormap (pScreen))
	return FALSE;

    pScreenPriv->solid =
	glitz_surface_create (pScreenPriv->drawable,
			      pScreenPriv->pixmapFormats[32].format,
			      1, 1);
    if (!pScreenPriv->solid)
	return FALSE;
    
    glitz_surface_set_fill (pScreenPriv->solid, GLITZ_FILL_REPEAT);
    
    return TRUE;
}

Bool
xglCloseScreen (int	  index,
		ScreenPtr pScreen)
{
    XGL_SCREEN_PRIV (pScreen);

    if (pScreenPriv->solid)
	glitz_surface_destroy (pScreenPriv->solid);

    if (pScreenPriv->surface)
	glitz_surface_destroy (pScreenPriv->surface);

    xglFiniOffscreen (pScreen);
    
    XGL_SCREEN_UNWRAP (CloseScreen);
    xfree (pScreenPriv);

    return (*pScreen->CloseScreen) (index, pScreen);
}
