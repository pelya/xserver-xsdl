/*
 * $Id$
 *
 * Copyright © 2004 Keith Packard
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
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include "xgl.h"

typedef struct _xglDepths {
    CARD8   depth;
    CARD8   bpp;
} XglDepths;

static XglDepths    xglDepths[] = {
    { 1, 1 },
    { 4, 4 },
    { 8, 8 },
    { 15, 16 },
    { 16, 16 },
    { 24, 32 },
    { 32, 32 }
};

#define NUM_XGL_DEPTHS (sizeof (xglDepths) / sizeof (xglDepths[0]))

static void
xglSetPixmapFormats (ScreenInfo *pScreenInfo)
{
    int	    i;
    
    pScreenInfo->imageByteOrder     = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad  = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder     = BITMAP_BIT_ORDER;
    
    pScreenInfo->numPixmapFormats = 0;
    for (i = 0; i < NUM_XGL_DEPTHS; i++)
    {
	PixmapFormatRec *format = &pScreenInfo->formats[pScreenInfo->numPixmapFormats++];
	format->depth = xglDepths[i].depth;
	format->bitsPerPixel = xglDepths[i].bpp;
        format->scanlinePad = BITMAP_SCANLINE_PAD;
    }
}

#define xglQueryBestSize	(void *) NoopDDA
#define xglSaveScreen		(void *) NoopDDA
#define miGetImage		(void *) NoopDDA
#define xglCreateWindow		(void *) NoopDDA
#define xglDestroyWindow	(void *) NoopDDA
#define xglPositionWindow	(void *) NoopDDA
#define xglChangeWindowAttributes	(void *) NoopDDA
#define xglRealizeWindow	(void *) NoopDDA
#define xglUnrealizeWindow	(void *) NoopDDA
#define xglPaintWindowBackground	(void *) NoopDDA
#define xglPaintWindowBorder	(void *) NoopDDA
#define xglCopyWindow		(void *) NoopDDA
#define xglRealizeFont		(void *) NoopDDA
#define xglUnrealizeFont	(void *) NoopDDA

#define xglConstrainCursor 	(void *) NoopDDA
#define xglCursorLimits 	(void *) NoopDDA
#define xglDisplayCursor 	(void *) NoopDDA
#define xglRealizeCursor 	(void *) NoopDDA
#define xglUnrealizeCursor 	(void *) NoopDDA
#define xglRecolorCursor 	(void *) NoopDDA
#define xglSetCursorPosition 	(void *) NoopDDA

#define xglCreateColormap	(void *) NoopDDA
#define xglDestroyColormap	(void *) NoopDDA
#define xglInstallColormap	(void *) NoopDDA
#define xglUninstallColormap	(void *) NoopDDA
#define xglListInstalledColormaps	(void *) NoopDDA
#define xglStoreColors		(void *) NoopDDA
#define xglResolveColor		(void *) NoopDDA
#define xglBitmapToRegion	(void *) NoopDDA

static PixmapPtr
xglGetWindowPixmap (WindowPtr pWin)
{
    return 0;
}

static void
xglSetWindowPixmap (WindowPtr pWin, PixmapPtr pPixmap)
{
}

static void
xglSetupScreen (ScreenPtr pScreen)
{
    pScreen->defColormap = FakeClientID (0);
    pScreen->blackPixel = pScreen->whitePixel = (Pixel) 0;
    
    pScreen->QueryBestSize = xglQueryBestSize;
    pScreen->SaveScreen = xglSaveScreen;
    pScreen->GetImage = miGetImage;
    pScreen->GetSpans = xglGetSpans;
    
    pScreen->CreateWindow = xglCreateWindow;
    pScreen->DestroyWindow = xglDestroyWindow;
    pScreen->PositionWindow = xglPositionWindow;
    pScreen->ChangeWindowAttributes = xglChangeWindowAttributes;
    pScreen->RealizeWindow = xglRealizeWindow;
    pScreen->UnrealizeWindow = xglUnrealizeWindow;
    pScreen->PaintWindowBackground = xglPaintWindowBackground;
    pScreen->PaintWindowBorder = xglPaintWindowBorder;
    pScreen->CopyWindow = xglCopyWindow;
    
    pScreen->CreatePixmap = xglCreatePixmap;
    pScreen->DestroyPixmap = xglDestroyPixmap;
    
    pScreen->RealizeFont = xglRealizeFont;
    pScreen->UnrealizeFont = xglUnrealizeFont;

    pScreen->ConstrainCursor = xglConstrainCursor;
    pScreen->CursorLimits = xglCursorLimits;
    pScreen->DisplayCursor = xglDisplayCursor;
    pScreen->RealizeCursor = xglRealizeCursor;
    pScreen->UnrealizeCursor = xglUnrealizeCursor;
    pScreen->RecolorCursor = xglRecolorCursor;
    pScreen->SetCursorPosition = xglSetCursorPosition;
    
    pScreen->CreateGC = xglCreateGC;
    
    pScreen->CreateColormap = miInitializeColormap;
    pScreen->DestroyColormap = xglDestroyColormap;
    pScreen->InstallColormap = miInstallColormap;
    pScreen->UninstallColormap = miUninstallColormap;
    pScreen->ListInstalledColormaps = miListInstalledColormaps;
    pScreen->StoreColors = xglStoreColors;
    pScreen->ResolveColor = miResolveColor;
    
    pScreen->BitmapToRegion = xglBitmapToRegion;

    pScreen->GetWindowPixmap = xglGetWindowPixmap;
    pScreen->SetWindowPixmap = xglSetWindowPixmap;
}

static Bool
xglScreenInit(int index, ScreenPtr pScreen, int argc, char **argv)
{
    VisualPtr	visuals;
    DepthPtr	depths;
    int		nvisuals;
    int		ndepth;
    int		rootDepth;
    VisualID	defaultVisual;
    
    rootDepth = 24;
    miInitVisuals (&visuals, &depths, &nvisuals, 
		   &ndepth, &rootDepth, &defaultVisual,
		   1 << 31, 8, -1);
    miScreenInit (pScreen, 0, 800, 600, 96, 96, 800, rootDepth,
		  ndepth, depths, defaultVisual, nvisuals, visuals);
    xglSetupScreen (pScreen);
    if (!miCreateDefColormap (pScreen))
	return FALSE;

    return TRUE;
}
    
void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
    xglSetPixmapFormats (pScreenInfo);

    AddScreen (xglScreenInit, argc, argv);
}

