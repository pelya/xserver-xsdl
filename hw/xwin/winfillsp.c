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
/* $XFree86: xc/programs/Xserver/hw/xwin/winfillsp.c,v 1.9 2001/11/01 12:19:40 alanh Exp $ */

#include "win.h"

/* See Porting Layer Definition - p. 54 */
void
winFillSpansNativeGDI (DrawablePtr	pDrawable,
		       GCPtr		pGC,
		       int		iSpans,
		       DDXPointPtr	pPoints,
		       int		*piWidths,
		       int		fSorted)
{
  winGCPriv(pGC);
  int			iSpan;
  DDXPointPtr		pPoint = NULL;
  int			*piWidth = 0;
  HBITMAP		hbmpOrig = NULL, hbmpOrigStipple = NULL;
  PixmapPtr		pPixmap = NULL;
  winPrivPixmapPtr	pPixmapPriv = NULL;
  HBITMAP		hbmpFilledStipple = NULL, hbmpMaskedForeground = NULL;
  PixmapPtr		pStipple = NULL;
  winPrivPixmapPtr	pStipplePriv = NULL;
  PixmapPtr		pTile = NULL;
  winPrivPixmapPtr	pTilePriv = NULL;
  HBRUSH		hbrushStipple = NULL;
  HDC			hdcStipple = NULL;
  BYTE			*pbbitsFilledStipple = NULL;
  int			iX;
  DEBUG_FN_NAME("winFillSpans");
  DEBUGVARS;
  DEBUGPROC_MSG;

  /* Branch on the type of drawable we have */
  switch (pDrawable->type)
    {
    case DRAWABLE_PIXMAP:
      ErrorF ("winFillSpans - DRAWABLE_PIXMAP\n");
#if 0
      /* Branch on the raster operation type */
      switch (pGC->alu)
	{
	case GXclear:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXclear\n");
	  break;
	case GXand:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXand\n");
	  break;
	case GXandReverse:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXandReverse\n");
	  break;
	case GXcopy:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXcopy\n");
	  break;
	case GXandInverted:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXandInverted\n");
	  break;
	case GXnoop:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXnoop\n");
	  break;
	case GXxor:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXxor\n");
	  break;
	case GXor:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXor\n");
	  break;
	case GXnor:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXnor\n");
	  break;
	case GXequiv:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXequiv\n");
	  break;
	case GXinvert:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXinvert\n");
	  break;
	case GXorReverse:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXorReverse\n");
	  break;
	case GXcopyInverted:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXcopyInverted\n");
	  break;
	case GXorInverted:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXorInverted\n");
	  break;
	case GXnand:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXnand\n");
	  break;
	case GXset:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - GXset\n");
	  break;
	default:
	  FatalError ("winFillSpans - DRAWABLE_PIXMAP - Unknown ROP\n");
	  break;
	}
#endif

      /* Get a pixmap pointer from the drawable pointer, and fetch privates  */
      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);

      /* Select the drawable pixmap into memory hdc */
      hbmpOrig = SelectObject (pGCPriv->hdcMem, pPixmapPriv->hBitmap);
      if (hbmpOrig == NULL)
	FatalError ("winFillSpans - DRAWABLE_PIXMAP - "
		    "SelectObject () failed on pPixmapPriv->hBitmap\n");
      
      /* Branch on the fill type */
      switch (pGC->fillStyle)
	{
	case FillSolid:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - FillSolid %08x\n",
		  pDrawable);
	  
	  /*
	   * REMOVE - Visual verification only.
	   */
	  BitBlt (pGCPriv->hdc, 
		  pDrawable->width, pDrawable->height,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0, 
		  SRCCOPY);
	  DEBUG_MSG ("FillSolid - Original bitmap");

	  /* Enumerate spans */
	  for (iSpan = 0; iSpan < iSpans; ++iSpan)
	    {
	      /* Get pointers to the current span location and width */
	      pPoint = pPoints + iSpan;
	      piWidth = piWidths + iSpan;

	      /* Display some useful information */
	      ErrorF ("(%dx%dx%d) (%d,%d) fg: %d bg: %d w: %d\n",
		      pDrawable->width, pDrawable->height, pDrawable->depth,
		      pPoint->x, pPoint->y,
		      pGC->fgPixel, pGC->bgPixel,
		      *piWidth);

	      /* Draw the requested line */
	      MoveToEx (pGCPriv->hdcMem, pPoint->x, pPoint->y, NULL);
	      LineTo (pGCPriv->hdcMem, pPoint->x + *piWidth, pPoint->y);
	    }

	  /*
	   * REMOVE - Visual verification only.
	   */
	  BitBlt (pGCPriv->hdc, 
		  pDrawable->width * 2, pDrawable->height,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0, 
		  SRCCOPY);
	  DEBUG_MSG ("FillSolid - Filled bitmap");

	  
	  /* Push the drawable pixmap out of the GC HDC */
	  SelectObject (pGCPriv->hdcMem, hbmpOrig);
	  break;

	case FillStippled:
	  ErrorF ("winFillSpans - DRAWABLE_PIXMAP - FillStippled %08x\n",
		  pDrawable);

	  /*
	   * FIXME: Assuming that pGC->patOrg.x = pGC->patOrg.y = 0
	   */

	  pStipple = pGC->stipple;
	  pStipplePriv = winGetPixmapPriv (pStipple);

	  /* Create a memory DC to hold the stipple */
	  hdcStipple = CreateCompatibleDC (pGCPriv->hdc);

	  /* Create a destination sized compatible bitmap */
	  hbmpFilledStipple = winCreateDIBNativeGDI (pDrawable->width,
						     pDrawable->height,
						     pDrawable->depth,
						     &pbbitsFilledStipple,
						     NULL);

	  /* Select the stipple bitmap into the stipple DC */
	  hbmpOrigStipple = SelectObject (hdcStipple, hbmpFilledStipple);
	  if (hbmpOrigStipple == NULL)
	    FatalError ("winFillSpans () - DRAWABLE_PIXMAP - FillStippled - "
			"SelectObject () failed on hbmpFilledStipple\n");

	  /* Create a pattern brush from the original stipple */
	  hbrushStipple = CreatePatternBrush (pStipplePriv->hBitmap);

	  /* Select the original stipple brush into the stipple DC */
	  SelectObject (hdcStipple, hbrushStipple);

	  /* PatBlt the original stipple to the filled stipple */
	  PatBlt (hdcStipple,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  PATCOPY);

	  /*
	   * REMOVE - Visual verification only.
	   */
	  BitBlt (pGCPriv->hdc,
		  pDrawable->width, 0,
		  pDrawable->width, pDrawable->height,
		  hdcStipple,
		  0, 0,
		  SRCCOPY);
	  DEBUG_MSG("Filled a drawable-sized stipple");

	  /*
	   * Mask out the bits from the drawable that are being preserved;
	   * hbmpFilledStipple now contains the preserved original bits.
	   */
	  BitBlt (hdcStipple,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0,
		  SRCERASE);

      	  /*
	   * REMOVE - Visual verification only.
	   */
	  BitBlt (pGCPriv->hdc,
		  pDrawable->width * 2, 0,
		  pDrawable->width, pDrawable->height,
		  hdcStipple,
		  0, 0,
		  SRCCOPY);
	  DEBUG_MSG("Preserved original bits");

	  /*
	   * Create a destination sized compatible bitmap to hold
	   * the masked foreground color.
	   */
	  hbmpMaskedForeground = winCreateDIBNativeGDI (pDrawable->width,
							pDrawable->height,
							pDrawable->depth,
							NULL,
							NULL);

	  /*
	   * Select the masked foreground bitmap into the default memory DC;
	   * this should pop the drawable bitmap out of the default DC.
	   */
	  if (SelectObject (pGCPriv->hdcMem, hbmpMaskedForeground) == NULL)
	    FatalError ("winFillSpans () - DRAWABLE_PIXMAP - FillStippled - "
			"SelectObject () failed on hbmpMaskedForeground\n");

	  /* Free the original drawable */
	  DeleteObject (pPixmapPriv->hBitmap);
	  pPixmapPriv->hBitmap = NULL;
	  pPixmapPriv->pbBits = NULL;

	  /* Select the stipple brush into the default memory DC */
	  SelectObject (pGCPriv->hdcMem, hbrushStipple);

	  /* Create the masked foreground bitmap using the original stipple */
	  PatBlt (pGCPriv->hdcMem,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  PATCOPY);
	  
      	  /*
	   * REMOVE - Visual verification only.
	   */
	  BitBlt (pGCPriv->hdc,
		  pDrawable->width * 3, 0,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0,
		  SRCCOPY);
	  DEBUG_MSG("Masked foreground bitmap");
      
	  /*
	   * Combine the masked foreground with the masked drawable;
	   * hbmpFilledStipple will contain the drawable stipple filled
	   * with the current foreground color
	   */
	  BitBlt (hdcStipple,
		  0, 0,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0,
		  SRCPAINT);

	  /*
	   * REMOVE - Visual verification only.
	   */
	  BitBlt (pGCPriv->hdc,
		  pDrawable->width * 4, 0,
		  pDrawable->width, pDrawable->height,
		  hdcStipple,
		  0, 0,
		  SRCCOPY);
	  DEBUG_MSG("Completed stipple");
	  
	  /* Release the stipple DC */
	  SelectObject (hdcStipple, hbmpOrig);
	  DeleteDC (hdcStipple);
      
	  /* Pop the stipple pattern brush out of the default mem DC */
	  SelectObject (pGCPriv->hdcMem, GetStockObject (WHITE_BRUSH));

	  /* Destroy the original stipple pattern brush */
	  DeleteObject (hbrushStipple);

	  /* Clear the memory DC */
	  SelectObject (pGCPriv->hdcMem, hbmpOrig);

	  /* Free the masked foreground bitmap */
	  DeleteObject (hbmpMaskedForeground);

	  /* Point the drawable to the new bitmap */
	  pPixmapPriv->hBitmap = hbmpFilledStipple;
	  pPixmapPriv->pbBits = pbbitsFilledStipple;
	  break;

	case FillTiled:
	  FatalError ("\nwinFillSpans - DRAWABLE_PIXMAP - FillTiled\n\n");
	  break;

	case FillOpaqueStippled:
	  FatalError ("winFillSpans - DRAWABLE_PIXMAP - OpaqueStippled\n");
	  break;

	default:
	  FatalError ("winFillSpans - DRAWABLE_PIXMAP - Unknown "
		      "fillStyle\n");
	  break;
	}
      break;
  
    case DRAWABLE_WINDOW:
      /* Branch on fill style */
      switch (pGC->fillStyle)
	{
	case FillSolid:
	  ErrorF ("\nwinFillSpans - DRAWABLE_WINDOW - FillSolid\n\n");

	  /* Enumerate spans */
	  for (iSpan = 0; iSpan < iSpans; ++iSpan)
	    {
	      /* Get pointers to the current span location and width */
	      pPoint = pPoints + iSpan;
	      piWidth = piWidths + iSpan;

	      /* Display some useful information */
	      ErrorF ("(%dx%dx%d) (%d,%d) fg: %d bg: %d w: %d\n",
		      pDrawable->width, pDrawable->height, pDrawable->depth,
		      pPoint->x, pPoint->y,
		      pGC->fgPixel, pGC->bgPixel,
		      *piWidth);

	      /* Draw the requested line */
	      MoveToEx (pGCPriv->hdc, pPoint->x, pPoint->y, NULL);
	      LineTo (pGCPriv->hdc, pPoint->x + *piWidth, pPoint->y);
	    }
	  break;

	case FillStippled:
	  FatalError ("winFillSpans - DRAWABLE_WINDOW - FillStippled\n\n");
	  break;

	case FillTiled:
	  ErrorF ("\nwinFillSpans - DRAWABLE_WINDOW - FillTiled\n\n");

	  /* Get a pixmap pointer from the tile pointer, and fetch privates  */
	  pTile = (PixmapPtr) pGC->tile.pixmap;
	  pTilePriv = winGetPixmapPriv (pTile);

	  /* Select the tile into a DC */
	  hbmpOrig = SelectObject (pGCPriv->hdcMem, pTilePriv->hBitmap);
	  if (hbmpOrig == NULL)
	    FatalError ("winFillSpans - DRAWABLE_WINDOW - FillTiled - "
			"SelectObject () failed on pTilePric->hBitmap\n");

	  /* Enumerate spans */
	  for (iSpan = 0; iSpan < iSpans; ++iSpan)
	    {
	      /* Get pointers to the current span location and width */
	      pPoint = pPoints + iSpan;
	      piWidth = piWidths + iSpan;
	  
	      for (iX = 0; iX < *piWidth; iX += pTile->drawable.width)
		{
		  /* Blit the tile to the screen, one line at a time */
		  BitBlt (pGCPriv->hdc,
			  pPoint->x + iX, pPoint->y,
			  pTile->drawable.width, 1,
			  pGCPriv->hdcMem,
			  0, pPoint->y % pTile->drawable.height,
			  SRCCOPY);
		}
	    }
      
	  /* Push the drawable pixmap out of the GC HDC */
	  SelectObject (pGCPriv->hdcMem, hbmpOrig);

#if 0
	  DEBUG_MSG("Completed tile fill");
#endif
	  break;

	case FillOpaqueStippled:
	  FatalError ("winFillSpans - DRAWABLE_WINDOW - "
		      "OpaqueStippled\n");
	  break;

	default:
	  FatalError ("winFillSpans - DRAWABLE_WINDOW - Unknown "
		      "fillStyle\n");
	}

      /* Branch on the raster operation type */
      switch (pGC->alu)
	{
	case GXclear:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXclear\n");
	  break;
	case GXand:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXand\n");
	  break;
	case GXandReverse:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXandReverse\n");
	  break;
	case GXcopy:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXcopy\n");
	  break;
	case GXandInverted:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXandInverted\n");
	  break;
	case GXnoop:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXnoop\n");
	  break;
	case GXxor:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXxor\n");
	  break;
	case GXor:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXor\n");
	  break;
	case GXnor:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXnor\n");
	  break;
	case GXequiv:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXequiv\n");
	  break;
	case GXinvert:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXinvert\n");
	  break;
	case GXorReverse:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXorReverse\n");
	  break;
	case GXcopyInverted:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXcopyInverted\n");
	  break;
	case GXorInverted:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXorInverted\n");
	  break;
	case GXnand:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXnand\n");
	  break;
	case GXset:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - GXset\n");
	  break;
	default:
	  ErrorF ("winFillSpans - DRAWABLE_WINDOW - Unknown ROP\n");
	  break;
	}
      break;
  
    case UNDRAWABLE_WINDOW:
      FatalError ("winFillSpans - UNDRAWABLE_WINDOW\n\n");

      switch (pGC->fillStyle)
	{
	case FillSolid:
	  ErrorF ("\nwinFillSpans - UNDRAWABLE_WINDOW - FillSolid\n\n");
	  break;

	case FillStippled:
	  ErrorF ("\nwinFillSpans - UNDRAWABLE_WINDOW - FillStippled\n\n");
	  break;

	case FillTiled:
	  ErrorF ("\nwinFillSpans - UNDRAWABLE_WINDOW - FillTiled\n\n");
	  break;

	case FillOpaqueStippled:
	  FatalError ("winFillSpans () - UNDRAWABLE_WINDOW - "
		      "OpaqueStippled\n");
	  break;

	default:
	  FatalError ("winFillSpans () - UNDRAWABLE_WINDOW - Unknown "
		      "fillStyle\n");
	}
      break;

    case DRAWABLE_BUFFER:
      FatalError ("winFillSpansNativeGDI - DRAWABLE_BUFFER\n");
      break;

    default:
      FatalError ("winFillSpansNativeGDI - Unknown drawable type\n");
      break;
    }
}
