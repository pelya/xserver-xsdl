/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/wingc.c,v 1.10 2001/10/30 15:39:09 alanh Exp $ */

#include "win.h"

#if 0
/* GC Handling Routines */
const GCFuncs winGCFuncs = {
  winValidateGCNativeGDI,
  winChangeGCNativeGDI,
  winCopyGCNativeGDI,
  winDestroyGCNativeGDI,
  winChangeClipNativeGDI,
  winDestroyClipNativeGDI,
  winCopyClipNativeGDI,
};
#else
const GCFuncs winGCFuncs = {
  winValidateGCNativeGDI,
  winChangeGCNativeGDI,
  winCopyGCNativeGDI,
  winDestroyGCNativeGDI,
  miChangeClip,
  miDestroyClip,
  miCopyClip,
};
#endif

/* Drawing Primitives */
const GCOps winGCOps = {
  winFillSpansNativeGDI,
  winSetSpansNativeGDI,
  miPutImage,
  miCopyArea,
  miCopyPlane,
  miPolyPoint,
  winPolyLineNativeGDI,
  miPolySegment,
  miPolyRectangle,
  miPolyArc,
  miFillPolygon,
  miPolyFillRect,
  miPolyFillArc,
  miPolyText8,
  miPolyText16,
  miImageText8,
  miImageText16,
  miImageGlyphBlt,
  miPolyGlyphBlt,
  miPushPixels
#ifdef NEED_LINEHELPER
  ,NULL
#endif
};


/* See Porting Layer Definition - p. 45 */
/* See mfb/mfbgc.c - mfbCreateGC() */
/* See Strategies for Porting - pp. 15, 16 */
Bool
winCreateGCNativeGDI (GCPtr pGC)
{
  winPrivGCPtr		pGCPriv = NULL;
  winPrivScreenPtr	pScreenPriv = NULL;

  ErrorF ("winCreateGCNativeGDI () depth: %d\n",
	  pGC->depth);

  pGC->clientClip = NULL;
  pGC->clientClipType = CT_NONE;

  pGC->ops = (GCOps *) &winGCOps;
  pGC->funcs = (GCFuncs *) &winGCFuncs;

  /* 
   * Setting miTranslate to 1 causes the coordinates passed to
   * FillSpans, GetSpans, and SetSpans to be screen relative, rather
   * than drawable relative.
   *
   * miTranslate was set to 0 prior to 2001-08-17.
   */
  pGC->miTranslate = 1;

  /* Allocate privates for this GC */
  pGCPriv = winGetGCPriv (pGC);
  if (pGCPriv == NULL)
    {
      ErrorF ("winCreateGCNativeGDI () - Privates pointer was NULL\n");
      return FALSE;
    }

  /* Copy the screen DC to the local privates */
  pScreenPriv = winGetScreenPriv (pGC->pScreen);
  pGCPriv->hdc = pScreenPriv->hdcScreen;

  /* Allocate a memory DC for the GC */
  pGCPriv->hdcMem = CreateCompatibleDC (pGCPriv->hdc);

  return TRUE;
}


/* See Porting Layer Definition - p. 45 */
void
winChangeGCNativeGDI (GCPtr pGC, unsigned long ulChanges)
{
#if CYGDEBUG
  ErrorF ("winChangeGCNativeGDI () - Doing nothing\n");
#endif
}


void
winValidateGCNativeGDI (GCPtr pGC,
			unsigned long ulChanges,
			DrawablePtr pDrawable)
{
  winGCPriv(pGC);
  HBITMAP		hbmpOrig = NULL;
  PixmapPtr		pPixmap = NULL;
  winPrivPixmapPtr	pPixmapPriv = NULL;
  RGBQUAD		rgbColors[2] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
  PixmapPtr		pStipple = NULL;
  winPrivPixmapPtr	pStipplePriv = NULL;
  int			i;
  DEBUG_FN_NAME("winValidateGC");
  DEBUGVARS;
  DEBUGPROC_MSG;

  /* Branch on drawable type */
  switch (pDrawable->type)
    {
    case DRAWABLE_PIXMAP:
      /* Branch on the fill style */
      switch (pGC->fillStyle)
	{
	case FillSolid:
	  ErrorF ("winValidateGC - DRAWABLE_PIXMAP - FillSolid\n");

	  /* Select a stock pen */
	  if (pDrawable->depth == 1 && pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting WHITE_PEN\n");
	      SelectObject (pGCPriv->hdcMem, GetStockObject (WHITE_PEN));
	    }
	  else if (pDrawable->depth == 1 && !pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting BLACK_PEN\n");
	      SelectObject (pGCPriv->hdcMem, GetStockObject (BLACK_PEN));
	    }
	  else if (pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting custom pen: %d\n",
		      pGC->fgPixel);
	      /*
	       * FIXME: So far I've only seen a white pen selected here.
	       */
#if 1	     
	      SelectObject (pGCPriv->hdcMem, GetStockObject (WHITE_PEN));
#else
	      /* FIXME: This leaks a pen */
	      SelectObject (pGCPriv->hdcMem, 
			    CreatePen (PS_SOLID, 0, pGC->fgPixel));
#endif
	    }
	  else
	    {
	      ErrorF ("winValidateGC - Selecting BLACK_PEN\n");
	      SelectObject (pGCPriv->hdcMem, GetStockObject (BLACK_PEN));
	    }
	  break;
	  
	case FillStippled:
	  ErrorF ("winValidateGC - DRAWABLE_PIXMAP - FillStippled\n");
	  /*
	   * NOTE: Setting the brush color has no effect on DIB fills.
	   * You need to set the stipple bitmap's color table instead.
	   */
#if 1
	  /* Pick the white color index */
	  if (pGC->fgPixel)
	    i = 1;
	  else
	    i = 0;

	  /* Set the white color, black is default */
	  rgbColors[i].rgbRed = 255;
	  rgbColors[i].rgbGreen = 255;
	  rgbColors[i].rgbBlue = 255;
	  
	  /* Get stipple and privates pointers */
	  pStipple = pGC->stipple;
	  pStipplePriv = winGetPixmapPriv (pStipple);

	  /* Select the stipple bitmap */
	  hbmpOrig = SelectObject (pGCPriv->hdcMem, pStipplePriv->hBitmap);

	  /* Set the stipple color table */
	  SetDIBColorTable (pGCPriv->hdcMem, 0, 2, rgbColors);

	  /* Pop the stipple out of the hdc */
	  SelectObject (pGCPriv->hdcMem, hbmpOrig);
	  
#else
	  /* Set the foreground color for the stipple fill */
	  if (pGC->fgPixel == 0x1)
	    {
	      SetTextColor (pGCPriv->hdcMem, RGB(0x00, 0x00, 0x00));
	    }
	  else if (pGC->fgPixel == 0xFFFF)
	    {
	      SetTextColor (pGCPriv->hdcMem, RGB(0xFF, 0xFF, 0xFF));
	    }
	  else
	    {
	      SetTextColor (pGCPriv->hdcMem, RGB(0x00, 0x00, 0x00));
	    }
	  SetBkColor (pGCPriv->hdcMem, RGB(0x00, 0x00, 0x00));
#endif
	  break;
	  
	case FillOpaqueStippled:
	  FatalError ("winValidateGC - DRAWABLE_PIXMAP - "
		      "FillOpaqueStippled\n");
	  break;

	case FillTiled:
	  FatalError ("winValidateGC - DRAWABLE_PIXMAP - FillTiled\n");
	  break;

	default:
	  FatalError ("winValidateGC - DRAWABLE_PIXMAP - Unknown fill "
		      "style\n");
	  break;
	}
      break;

    case DRAWABLE_WINDOW:
      /* Branch on the fill style */
      switch (pGC->fillStyle)
	{
	case FillTiled:
	  ErrorF ("winValidateGC - DRAWABLE_WINDOW - FillTiled\n");
	  /* 
	   * Do nothing here for now.  Select the tile bitmap into the
	   * appropriate DC in the drawing function.
	   */

	  /*
	   * BEGIN REMOVE - Visual verification only.
	   */
	  /* Get pixmap and privates pointers for the tile */
	  pPixmap = pGC->tile.pixmap;
	  pPixmapPriv = winGetPixmapPriv (pPixmap);

	  /* Push the tile into the GC's DC */
	  hbmpOrig = SelectObject (pGCPriv->hdcMem, pPixmapPriv->hBitmap);
	  if (hbmpOrig == NULL)
	    FatalError ("winValidateGC - DRAWABLE_WINDOW - FillTiled - "
			"SelectObject () failed on pPixmapPriv->hBitmap\n");

	  /* Blit the tile to a remote area of the screen */
	  BitBlt (pGCPriv->hdc, 
		  64, 64,
		  pGC->tile.pixmap->drawable.width,
		  pGC->tile.pixmap->drawable.height,
		  pGCPriv->hdcMem,
		  0, 0, 
		  SRCCOPY);
	  DEBUG_MSG ("Blitted the tile to a remote area of the screen");
	  
	  /* Pop the tile out of the GC's DC */
	  SelectObject (pGCPriv->hdcMem, hbmpOrig);
	  /*
	   * END REMOVE - Visual verification only.
	   */
	  break;

	case FillStippled:
	  FatalError ("winValidateGC - DRAWABLE_WINDOW - FillStippled\n");
	  break;

	case FillOpaqueStippled:
	  FatalError ("winValidateGC - DRAWABLE_WINDOW - "
		      "FillOpaqueStippled\n");
	  break;

	case FillSolid:
	  ErrorF ("winValidateGC - DRAWABLE_WINDOW - FillSolid\n");

	  /* Select a stock pen */
	  if (pDrawable->depth == 1 && pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting WHITE_PEN\n");
	      SelectObject (pGCPriv->hdc, GetStockObject (WHITE_PEN));
	    }
	  else if (pDrawable->depth == 1 && !pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting BLACK_PEN\n");
	      SelectObject (pGCPriv->hdc, GetStockObject (BLACK_PEN));
	    }
	  else if (pGC->fgPixel)
	    {
	      ErrorF ("winValidateGC - Selecting custom pen: %d\n",
		      pGC->fgPixel);
	      /*
	       * FIXME: So far I've only seen a white pen selected here.
	       */
	      SelectObject (pGCPriv->hdc, GetStockObject (WHITE_PEN));
	    }
	  else
	    {
	      ErrorF ("winValidateGC - Selecting BLACK_PEN\n");
	      SelectObject (pGCPriv->hdc, GetStockObject (BLACK_PEN));
	    }
	  break;

	default:
	  FatalError ("winValidateGC - DRAWABLE_WINDOW - Unknown fill "
		      "style\n");
	  break;
	}
      break;

    case UNDRAWABLE_WINDOW:
      ErrorF ("\nwinValidateGC - UNDRAWABLE_WINDOW\n\n");
      break;

    case DRAWABLE_BUFFER:
      FatalError ("winValidateGC - DRAWABLE_BUFFER\n");
      break;

    default:
      FatalError ("winValidateGC - Unknown drawable type\n");
      break;
    }
}


/* See Porting Layer Definition - p. 46 */
void
winCopyGCNativeGDI (GCPtr pGCsrc, unsigned long ulMask, GCPtr pGCdst)
{

}


/* See Porting Layer Definition - p. 46 */
void
winDestroyGCNativeGDI (GCPtr pGC)
{
  winGCPriv(pGC);

  /* Free the memory DC */
  if (pGCPriv->hdcMem != NULL)
    {
      DeleteDC (pGCPriv->hdcMem);
      pGCPriv->hdcMem = NULL;
    }

  /* Invalidate the screen DC pointer */
  pGCPriv->hdc = NULL;

  /* Invalidate the GC privates pointer */
  winSetGCPriv (pGC, NULL);
}


/* See Porting Layer Definition - p. 46 */
void
winChangeClipNativeGDI (GCPtr pGC, int nType, pointer pValue, int nRects)
{

}


/* See Porting Layer Definition - p. 47 */
void
winDestroyClipNativeGDI (GCPtr pGC)
{

}


/* See Porting Layer Definition - p. 47 */
void
winCopyClipNativeGDI (GCPtr pGCdst, GCPtr pGCsrc)
{

}
