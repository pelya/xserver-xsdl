/* $XFree86: xc/programs/Xserver/ilbm/ilbmscrinit.c,v 3.5 1998/11/22 10:37:40 dawes Exp $ */
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
/* $XConsortium: ilbmscrinit.c,v 5.17 94/04/17 20:28:34 dpw Exp $ */

/* Modified jun 95 by Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
   to use interleaved bitplanes instead of normal bitplanes */

#include "X.h"
#include "Xproto.h"		/* for xColorItem */
#include "Xmd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "resource.h"
#include "colormap.h"
#include "ilbm.h"
#include "mistruct.h"
#include "dix.h"
#include "mi.h"
#include "mibstore.h"
#include "migc.h"
#include "servermd.h"

#ifdef PIXMAP_PER_WINDOW
int frameWindowPrivateIndex;
#endif
int ilbmWindowPrivateIndex;
int ilbmGCPrivateIndex;
int ilbmScreenPrivateIndex;

static unsigned long ilbmGeneration = 0;

BSFuncRec ilbmBSFuncRec = {
	ilbmSaveAreas,
	ilbmRestoreAreas,
	(BackingStoreSetClipmaskRgnProcPtr) 0,
	(BackingStoreGetImagePixmapProcPtr) 0,
	(BackingStoreGetSpansPixmapProcPtr) 0,
};

Bool
ilbmCloseScreen(index, pScreen)
	int index;
	ScreenPtr pScreen;
{
	int d;
	DepthPtr depths = pScreen->allowedDepths;

	for (d = 0; d < pScreen->numDepths; d++)
		xfree(depths[d].vids);
	xfree(depths);
	xfree(pScreen->visuals);
	xfree(pScreen->devPrivates[ilbmScreenPrivateIndex].ptr);
	return(TRUE);
}

Bool
ilbmCreateScreenResources(pScreen)
	ScreenPtr pScreen;
{
	Bool retval;

	pointer oldDevPrivate = pScreen->devPrivate;

	pScreen->devPrivate = pScreen->devPrivates[ilbmScreenPrivateIndex].ptr;
	retval = miCreateScreenResources(pScreen);

	/* Modify screen's pixmap devKind value stored off devPrivate to
	 * be the width of a single plane in longs rather than the width
	 * of a chunky screen in longs as incorrectly setup by the mi routine.
	 */
	((PixmapPtr)pScreen->devPrivate)->devKind = BitmapBytePad(pScreen->width);
	pScreen->devPrivates[ilbmScreenPrivateIndex].ptr = pScreen->devPrivate;
	pScreen->devPrivate = oldDevPrivate;
	return(retval);
}

Bool
ilbmAllocatePrivates(pScreen, pWinIndex, pGCIndex)
	ScreenPtr pScreen;
	int *pWinIndex, *pGCIndex;
{
	if (ilbmGeneration != serverGeneration) {
#ifdef PIXMAP_PER_WINDOW
		frameWindowPrivateIndex = AllocateWindowPrivateIndex();
#endif
		ilbmWindowPrivateIndex = AllocateWindowPrivateIndex();
		ilbmGCPrivateIndex = AllocateGCPrivateIndex();
		ilbmGeneration = serverGeneration;
	}
	if (pWinIndex)
		*pWinIndex = ilbmWindowPrivateIndex;
	if (pGCIndex)
		*pGCIndex = ilbmGCPrivateIndex;

	ilbmScreenPrivateIndex = AllocateScreenPrivateIndex();
	pScreen->GetWindowPixmap = ilbmGetWindowPixmap;
	pScreen->SetWindowPixmap = ilbmSetWindowPixmap;
	return(AllocateWindowPrivate(pScreen, ilbmWindowPrivateIndex, sizeof(ilbmPrivWin)) &&
	       AllocateGCPrivate(pScreen, ilbmGCPrivateIndex, sizeof(ilbmPrivGC)));
}

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
ilbmScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width)
	register ScreenPtr pScreen;
	pointer pbits;			/* pointer to screen bitmap */
	int xsize, ysize;		/* in pixels */
	int dpix, dpiy;			/* dots per inch */
	int width;			/* pixel width of frame buffer */
{
	VisualPtr visuals;
	DepthPtr depths;
	int nvisuals;
	int ndepths;
	int rootdepth;
	VisualID defaultVisual;
	pointer oldDevPrivate;

	rootdepth = 0;
	if (!ilbmInitVisuals(&visuals, &depths, &nvisuals, &ndepths, &rootdepth,
								&defaultVisual, 256, 8)) {
		ErrorF("ilbmInitVisuals: FALSE\n");
		return FALSE;
	}
	if (!ilbmAllocatePrivates(pScreen,(int *)NULL, (int *)NULL)) {
		ErrorF("ilbmAllocatePrivates: FALSE\n");
		return FALSE;
	}

	pScreen->defColormap = (Colormap)FakeClientID(0);
	/* whitePixel, blackPixel */
	pScreen->blackPixel = 0;
	pScreen->whitePixel = 0;
	pScreen->QueryBestSize = ilbmQueryBestSize;
	/* SaveScreen */
	pScreen->GetImage = ilbmGetImage;
	pScreen->GetSpans = ilbmGetSpans;
	pScreen->CreateWindow = ilbmCreateWindow;
	pScreen->DestroyWindow = ilbmDestroyWindow;
	pScreen->PositionWindow = ilbmPositionWindow;
	pScreen->ChangeWindowAttributes = ilbmChangeWindowAttributes;
	pScreen->RealizeWindow = ilbmMapWindow;
	pScreen->UnrealizeWindow = ilbmUnmapWindow;
	pScreen->PaintWindowBackground = ilbmPaintWindow;
	pScreen->PaintWindowBorder = ilbmPaintWindow;
	pScreen->CopyWindow = ilbmCopyWindow;
	pScreen->CreatePixmap = ilbmCreatePixmap;
	pScreen->DestroyPixmap = ilbmDestroyPixmap;
	pScreen->RealizeFont = ilbmRealizeFont;
	pScreen->UnrealizeFont = ilbmUnrealizeFont;
	pScreen->CreateGC = ilbmCreateGC;
	pScreen->CreateColormap = ilbmInitializeColormap;
	pScreen->DestroyColormap = (void (*)())NoopDDA;
	pScreen->InstallColormap = ilbmInstallColormap;
	pScreen->UninstallColormap = ilbmUninstallColormap;
	pScreen->ListInstalledColormaps = ilbmListInstalledColormaps;
	pScreen->StoreColors = (void (*)())NoopDDA;
	pScreen->ResolveColor = ilbmResolveColor;
	pScreen->BitmapToRegion = ilbmPixmapToRegion;
	oldDevPrivate = pScreen->devPrivate;
	if (!miScreenInit(pScreen, pbits, xsize, ysize, dpix, dpiy, width, rootdepth,
			ndepths, depths, defaultVisual, nvisuals, visuals)) {
		ErrorF("miScreenInit: FALSE\n");
		return FALSE;
	}

	pScreen->CloseScreen = ilbmCloseScreen;
	pScreen->CreateScreenResources = ilbmCreateScreenResources;
	pScreen->BackingStoreFuncs = ilbmBSFuncRec;

	pScreen->devPrivates[ilbmScreenPrivateIndex].ptr = pScreen->devPrivate;
	pScreen->devPrivate = oldDevPrivate;

	return TRUE;
}

PixmapPtr
ilbmGetWindowPixmap(pWin)
    WindowPtr pWin;
{
#ifdef PIXMAP_PER_WINDOW
    return (PixmapPtr)(pWin->devPrivates[frameWindowPrivateIndex].ptr);
#else
    ScreenPtr pScreen = pWin->drawable.pScreen;

    return (* pScreen->GetScreenPixmap)(pScreen);
#endif
}

void
ilbmSetWindowPixmap(pWin, pPix)
    WindowPtr pWin;
    PixmapPtr pPix;
{
#ifdef PIXMAP_PER_WINDOW
    pWin->devPrivates[frameWindowPrivateIndex].ptr = (pointer)pPix;
#else
    (* pWin->drawable.pScreen->SetScreenPixmap)(pPix);
#endif
}
