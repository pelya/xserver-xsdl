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
/* $XFree86: xc/programs/Xserver/hw/xwin/wingetsp.c,v 1.7 2001/11/01 12:19:40 alanh Exp $ */

#include "win.h"

/* See Porting Layer Definition - p. 55 */
void
winGetSpansNativeGDI (DrawablePtr	pDrawable, 
		      int		nMax, 
		      DDXPointPtr	pPoints, 
		      int		*piWidths, 
		      int		iSpans, 
		      char		*pDsts)
{
  PixmapPtr		pPixmap = NULL;
  winPrivPixmapPtr	pPixmapPriv = NULL;
  int			iSpan;
  DDXPointPtr		pPoint = NULL;
  int			*piWidth = NULL;
  char			*pDst = pDsts;
  int			iBytesToCopy;
  HBITMAP		hbmpWindow, hbmpOrig;
  BYTE			*pbWindow = NULL;
  HDC			hdcMem;
  ScreenPtr		pScreen = pDrawable->pScreen;
  winScreenPriv(pScreen);
  int			iByteWidth;

  /* Branch on the drawable type */
  switch (pDrawable->type)
    {
    case DRAWABLE_PIXMAP:
      ErrorF ("winGetSpans - DRAWABLE_PIXMAP %08x\n",
	      pDrawable);

      pPixmap = (PixmapPtr) pDrawable;
      pPixmapPriv = winGetPixmapPriv (pPixmap);

      /* Loop through spans */
      for (iSpan = 0; iSpan < iSpans; ++iSpan)
	{
	  pPoint = pPoints + iSpan;
	  piWidth = piWidths + iSpan;
	  
	  iBytesToCopy = PixmapBytePad (*piWidth, pDrawable->depth);

	  memcpy (pDst,
		  pPixmapPriv->pbBits
		  + pPixmapPriv->dwScanlineBytes * pPoint->y,
		  iBytesToCopy);

	  ErrorF ("(%dx%dx%d) (%d,%d) w: %d\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, *piWidth);

	  /* Calculate offset of next bit destination */
	  pDst += 4 * ((*piWidth + 31) / 32);
	}
      break;

    case DRAWABLE_WINDOW:
      ErrorF ("winGetSpans - DRAWABLE_WINDOW\n");

      /*
       * FIXME: Making huge assumption here that we are copying the
       * area behind where the cursor will be displayed.  We already
       * know the size of the cursor, so this works, for now.
       */

      /* Create a bitmap to blit the window data to */
      hbmpWindow = winCreateDIBNativeGDI (*piWidths,
					  *piWidths,
					  pDrawable->depth,
					  &pbWindow,
					  NULL);

      /* Open a memory HDC */
      hdcMem = CreateCompatibleDC (NULL);

      /* Select the window bitmap */
      hbmpOrig = SelectObject (hdcMem, hbmpWindow);
			       
      /* Transfer the window bits to the window bitmap */
      BitBlt (hdcMem,
	      0, 0,
	      *piWidths, *piWidths, /* FIXME: Assuming square region */
	      pScreenPriv->hdcScreen,
	      pPoints->x, pPoints->y,
	      SRCCOPY);

      /* Pop the window bitmap out of the HDC */
      SelectObject (hdcMem, hbmpOrig);
      
      /* Delete the memory HDC */
      DeleteDC (hdcMem);
      hdcMem = NULL;

      iByteWidth = PixmapBytePad (*piWidths, pDrawable->depth);

      /* Loop through spans */
      for (iSpan = 0; iSpan < iSpans; ++iSpan)
	{
	  pPoint = pPoints + iSpan;
	  piWidth = piWidths + iSpan;

	  iBytesToCopy = PixmapBytePad (*piWidth, pDrawable->depth);

	  memcpy (pDst,
		  pbWindow + iByteWidth * (pPoint->y - pPoints->y),
		  iBytesToCopy);
	  
	  ErrorF ("(%dx%dx%d) (%d,%d) w: %d\n",
		  pDrawable->width, pDrawable->height, pDrawable->depth,
		  pPoint->x, pPoint->y, *piWidth);

	  /* Calculate offset of next bit destination */
	  pDst += 4 * ((*piWidth + 31) / 32);
	}

      /* Delete the window bitmap */
      DeleteObject (hbmpWindow);
      break;

    case UNDRAWABLE_WINDOW:
      FatalError ("winGetSpans - UNDRAWABLE_WINDOW\n");
      break;

    case DRAWABLE_BUFFER:
      FatalError ("winGetSpans - DRAWABLE_BUFFER\n");
      break;
      
    default:
      FatalError ("winGetSpans - Unknown drawable type\n");
      break;
    }
}
