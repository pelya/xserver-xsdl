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
/* $XFree86: xc/programs/Xserver/hw/xwin/winmisc.c,v 1.6 2002/07/05 09:19:26 alanh Exp $ */

#include "win.h"


/* See Porting Layer Definition - p. 33 */
/*
 * Called by clients, returns the best size for a cursor, tile, or
 * stipple, specified by class (sometimes called kind)
 */

void
winQueryBestSizeNativeGDI (int class, unsigned short *pWidth,
			   unsigned short *pHeight, ScreenPtr pScreen)
{
  ErrorF ("winQueryBestSizeNativeGDI\n");
}


/*
 * Count the number of one bits in a color mask.
 */

CARD8
winCountBits (DWORD dw)
{
  DWORD		dwBits = 0;

  while (dw)
    {
      dwBits += (dw & 1);
      dw >>= 1;
    }

  return dwBits;
}


/*
 * Modify the screen pixmap to point to the new framebuffer address
 */

Bool
winUpdateFBPointer (ScreenPtr pScreen, void *pbits)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;

  /* Location of shadow framebuffer has changed */
  pScreenInfo->pfb = pbits;

  /* Update the screen pixmap */
  if (!(*pScreen->ModifyPixmapHeader) (pScreen->devPrivate,
				       pScreen->width,
				       pScreen->height,
				       pScreen->rootDepth,
				       BitsPerPixel (pScreen->rootDepth),
				       PixmapBytePad (pScreenInfo->dwStride,
						      pScreenInfo->dwBPP),
				       pScreenInfo->pfb))
    {
      FatalError ("winUpdateFramebufferPointer - Failed modifying "\
		  "screen pixmap\n");
    }

  return TRUE;
}


/*
 * Paint the window background with the specified color
 */

BOOL
winPaintBackground (HWND hwnd, COLORREF colorref)
{
  HDC			hdc;
  HBRUSH		hbrush;
  RECT			rect;

  /* Create an hdc */
  hdc = GetDC (hwnd);
  if (hdc == NULL)
    {
      printf ("gdiWindowProc - GetDC failed\n");
      exit (1);
    }

  /* Create and select blue brush */
  hbrush = CreateSolidBrush (colorref);
  if (hbrush == NULL)
    {
      printf ("gdiWindowProc - CreateSolidBrush failed\n");
      exit (1);
    }

  /* Get window extents */
  if (GetClientRect (hwnd, &rect) == FALSE)
    {
      printf ("gdiWindowProc - GetClientRect failed\n");
      exit (1);
    }

  /* Fill window with blue brush */
  if (FillRect (hdc, &rect, hbrush) == 0)
    {
      printf ("gdiWindowProc - FillRect failed\n");
      exit (1);
    }

  /* Delete blue brush */
  DeleteObject (hbrush);

  /* Release the hdc */
  ReleaseDC (hwnd, hdc);

  return TRUE;
}
