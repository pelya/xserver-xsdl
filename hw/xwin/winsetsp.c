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
/* $XFree86: xc/programs/Xserver/hw/xwin/winsetsp.c,v 1.7 2001/11/01 12:19:42 alanh Exp $ */

#include "win.h"

/* See Porting Layer Definition - p. 55 */
void
winSetSpansNativeGDI (DrawablePtr	pDrawable,
		      GCPtr		pGC,
		      char		*pSrcs,
		      DDXPointPtr	pPoints,
		      int		*piWidths,
		      int		iSpans,
		      int		fSorted)
{
  winGCPriv(pGC);
  PixmapPtr		pPixmap = NULL;
  winPrivPixmapPtr	pPixmapPriv = NULL;
  int			iSpan;
  int			*piWidth = NULL;
  DDXPointPtr		pPoint = NULL;
  char			*pSrc = pSrcs;
  HDC			hdcMem;
  BITMAPINFOHEADER	*pbmih;
  HBITMAP		hBitmap = NULL;
  HBITMAP		hbmpOrig = NULL;
  DEBUG_FN_NAME("winSetSpans");
  DEBUGVARS;
  DEBUGPROC_MSG;

  /* Branch on the drawable type */
  switch (pDrawable->type)
    {
    case DRAWABLE_PIXMAP:
      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);
      
      /* Select the drawable pixmap into a DC */
      hbmpOrig = SelectObject (pGCPriv->hdcMem, pPixmapPriv->hBitmap);
      if (hbmpOrig == NULL)
	FatalError ("winSetSpans - DRAWABLE_PIXMAP - SelectObject () "
		    "failed on pPixmapPriv->hBitmap\n");

      /* Branch on the raster operation type */
      switch (pGC->alu)
	{
	case GXclear:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXclear\n");
	  break;

	case GXand:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXand\n");
	  break;

	case GXandReverse:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXandReverse\n");
	  break;

	case GXcopy:
	  ErrorF ("winSetSpans - DRAWABLE_PIXMAP - GXcopy %08x\n",
		  pDrawable);

	  /* Loop through spans */
	  for (iSpan = 0; iSpan < iSpans; ++iSpan)
	    {
	      piWidth = piWidths + iSpan;
	      pPoint = pPoints + iSpan;
	  
	      /* Blast the bits to the drawable */
	      SetDIBits (pGCPriv->hdcMem,
			 pPixmapPriv->hBitmap,
			 pPoint->y, 1, 
			 pSrc,
			 (BITMAPINFO *) pPixmapPriv->pbmih,
			 0);
	  
	      /* Display some useful information */
	      ErrorF ("(%dx%dx%d) (%d,%d) w: %d ps: %08x\n",
		      pDrawable->width, pDrawable->height, pDrawable->depth,
		      pPoint->x, pPoint->y, *piWidth, pSrc);

	      /* Calculate offset of next bit source */
	      pSrc += 4 * ((*piWidth  + 31) / 32);
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
	  DEBUG_MSG ("DRAWABLE_PIXMAP - GXcopy");
	  break;

	case GXandInverted:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXandInverted\n");
	  break;

	case GXnoop:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXnoop\n");
	  break;

	case GXxor:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXxor\n");
	  break;

	case GXor:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXor\n");
	  break;

	case GXnor:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXnor\n");
	  break;

	case GXequiv:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXequiv\n");
	  break;

	case GXinvert:
	  ErrorF ("winSetSpans - DRAWABLE_PIXMAP - GXinvert %08x\n",
		  pDrawable);

	  /* Create a temporary DC */
	  hdcMem = CreateCompatibleDC (NULL);
 
	  /* Loop through spans */
	  for (iSpan = 0; iSpan < iSpans; ++iSpan)
	    {
	      piWidth = piWidths + iSpan;
	      pPoint = pPoints + iSpan;

	      /* Create a one-line DIB for the bit data */
	      hBitmap = winCreateDIBNativeGDI (*piWidth, 1, pDrawable->depth,
					       NULL, (BITMAPINFO **) &pbmih);

	      /* Select the span line line bitmap into the temporary DC */
	      hbmpOrig = SelectObject (hdcMem, hBitmap);
	      
	      /* Blast bit data to the one-line DIB */
	      SetDIBits (hdcMem, hBitmap,
			 0, 1,
			 pSrc,
			 (BITMAPINFO *) pbmih,
			 DIB_RGB_COLORS);

	      /* Blit the span line to the drawable */
	      BitBlt (pGCPriv->hdcMem,
		      pPoint->x, pPoint->y,
		      *piWidth, 1,
		      hdcMem,
		      0, 0, 
		      NOTSRCCOPY);

	      /*
	       * REMOVE - Visual verification only.
	       */
	      BitBlt (pGCPriv->hdc,
		      pDrawable->width, pDrawable->height + pPoint->y,
		      *piWidth, 1,
		      hdcMem,
		      0, 0, 
		      SRCCOPY);

	      /* Display some useful information */
	      ErrorF ("(%dx%dx%d) (%d,%d) w: %d ps: %08x\n",
		      pDrawable->width, pDrawable->height, pDrawable->depth,
		      pPoint->x, pPoint->y, *piWidth, pSrc);

	      /* Calculate offset of next bit source */
	      pSrc += 4 * ((*piWidth + 31) / 32);

	      /* Pop the span line bitmap out of the memory DC */
	      SelectObject (hdcMem, hbmpOrig);

	      /* Free the temporary bitmap */
	      DeleteObject (hBitmap);
	      hBitmap = NULL;
	    }

	  /*
	   * REMOVE - Visual verification only.
	   */
	  DEBUG_MSG ("DRAWABLE_PIXMAP - GXinvert - Prior to invert");
	  BitBlt (pGCPriv->hdc, 
		  pDrawable->width * 2, pDrawable->height,
		  pDrawable->width, pDrawable->height,
		  pGCPriv->hdcMem,
		  0, 0, 
		  SRCCOPY);
	  DEBUG_MSG ("DRAWABLE_PIXMAP - GXinvert - Finished invert");

	  /* Release the scratch DC */
	  DeleteDC (hdcMem);
	  break;

	case GXorReverse:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXorReverse\n");
	  break;

	case GXcopyInverted:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXcopyInverted\n");
	  break;

	case GXorInverted:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXorInverted\n");
	  break;

	case GXnand:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXnand\n");
	  break;

	case GXset:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - GXset\n");
	  break;

	default:
	  FatalError ("winSetSpans - DRAWABLE_PIXMAP - Unknown ROP\n");
	  break;
	}

      /* Push the drawable pixmap out of the GC HDC */
      SelectObject (pGCPriv->hdcMem, hbmpOrig);
      break;

    case DRAWABLE_WINDOW:
      FatalError ("\nwinSetSpansNativeGDI - DRAWABLE_WINDOW\n\n");

      /* Branch on the raster operation type */
      switch (pGC->alu)
	{
	case GXclear:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXclear\n");
	  break;

	case GXand:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXand\n");
	  break;

	case GXandReverse:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXandReverse\n");
	  break;

	case GXcopy:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXcopy\n");
	  break;

	case GXandInverted:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXandInverted\n");
	  break;

	case GXnoop:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXnoop\n");
	  break;

	case GXxor:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXxor\n");
	  break;

	case GXor:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXor\n");
	  break;

	case GXnor:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXnor\n");
	  break;

	case GXequiv:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXequiv\n");
	  break;

	case GXinvert:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXinvert\n");
	  break;

	case GXorReverse:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXorReverse\n");
	  break;

	case GXcopyInverted:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXcopyInverted\n");
	  break;

	case GXorInverted:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXorInverted\n");
	  break;

	case GXnand:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXnand\n");
	  break;

	case GXset:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - GXset\n");
	  break;

	default:
	  ErrorF ("winSetSpans () - DRAWABLE_WINDOW - Unknown ROP\n");
	  break;
	}
      break;

    case UNDRAWABLE_WINDOW:
      FatalError ("\nwinSetSpansNativeGDI - UNDRAWABLE_WINDOW\n\n");
      break;

    case DRAWABLE_BUFFER:
      FatalError ("\nwinSetSpansNativeGDI - DRAWABLE_BUFFER\n\n");
      break;
      
    default:
      FatalError ("\nwinSetSpansNativeGDI - Unknown drawable type\n\n");
      break;
    }
}
