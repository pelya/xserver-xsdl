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
 * Authors:	Earle F. Philhower, III
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winmultiwindowicons.c,v 1.2 2003/10/02 13:30:10 eich Exp $ */

#include "win.h"
#include "dixevents.h"
#include "winmultiwindowclass.h"
#include "winprefs.h"


/*
 * External global variables
 */

extern HICON		g_hiconX;


/*
 * Prototypes for local functions
 */

static void
winScaleXBitmapToWindows (int iconSize, int effBPP,
			  PixmapPtr pixmap, unsigned char *image);


/*
 * Scale an X icon bitmap into a Windoze icon bitmap
 */

static void
winScaleXBitmapToWindows (int iconSize,
			  int effBPP,
			  PixmapPtr pixmap,
			  unsigned char *image)
{
  int			row, column, effXBPP, effXDepth;
  unsigned char		*outPtr;
  unsigned char		*iconData = 0;
  int			stride, xStride;
  float			factX, factY;
  int			posX, posY;
  unsigned char		*ptr;
  unsigned int		zero;
  unsigned int		color;


  if (pixmap->drawable.bitsPerPixel == 15)
    effXBPP = 16;
  else
    effXBPP = pixmap->drawable.bitsPerPixel;
  
  if (pixmap->drawable.depth == 15)
    effXDepth = 16;
  else
    effXDepth = pixmap->drawable.depth;

  /* Need 32-bit aligned rows */
  stride = ((iconSize * effBPP + 31) & (~31)) / 8;
  xStride  = ((pixmap->drawable.width * effXBPP + 31) & (~31)) / 8;
  
  iconData = malloc (xStride * pixmap->drawable.height);
  miGetImage ((DrawablePtr) &(pixmap->drawable), 0, 0,
	      pixmap->drawable.width, pixmap->drawable.height,
	      ZPixmap, 0xffffffff, iconData);
  
  /* Keep aspect ratio */
  factX = ((float)pixmap->drawable.width) / ((float)iconSize);
  factY = ((float)pixmap->drawable.height) / ((float)iconSize);
  if (factX > factY)
    factY = factX;
  else
    factX = factY;
  
  /* Out-of-bounds, fill icon with zero */
  zero = 0;
 
  for (row = 0; row < iconSize; row++)
    {
      outPtr = image + stride * row;
      for (column = 0; column < iconSize; column++)
	{
	  posX = factX * column;
	  posY = factY * row;
	  
	  ptr = iconData + posY*xStride;
	  if (effXBPP == 1)
	    {
	      ptr += posX / 8;
	      
	      /* Out of X icon bounds, leave space blank */
	      if (posX >= pixmap->drawable.width
		  || posY >= pixmap->drawable.height)
		ptr = (unsigned char *) &zero;
	      
	      if ((*ptr) & (1 << (posX & 7)))
		switch (effBPP)
		  {
		  case 32:
		    *(outPtr++) = 0;
		  case 24:
		    *(outPtr++) = 0;
		  case 16:
		    *(outPtr++) = 0;
		  case 8:
		    *(outPtr++) = 0;
		    break;
		  case 1:
		    outPtr[column / 8] &= ~(1 << (7 - (column & 7)));
		    break;
		  }
	      else
		switch (effBPP)
		  {
		  case 32:
		    *(outPtr++) = 255;
		    *(outPtr++) = 255;
		    *(outPtr++) = 255;
		    *(outPtr++) = 0;
		    break;
		  case 24:
		    *(outPtr++) = 255;
		  case 16:
		    *(outPtr++) = 255;
		  case 8: 
		    *(outPtr++) = 255;
		    break;
		  case 1:
		    outPtr[column / 8] |= (1 << (7 - (column & 7)));
		    break;
		  }
	    }
	  else if (effXDepth == 24 || effXDepth == 32)
	    {
	      ptr += posX * (effXBPP / 8);

	      /* Out of X icon bounds, leave space blank */
	      if (posX >= pixmap->drawable.width
		  || posY >= pixmap->drawable.height)
		ptr = (unsigned char *) &zero;
	      color = (((*ptr) << 16)
		       + ((*(ptr + 1)) << 8)
		       + ((*(ptr + 2)) << 0));
	      switch (effBPP)
		{
		case 32:
		  *(outPtr++) = *(ptr++); // b
		  *(outPtr++) = *(ptr++); // g
		  *(outPtr++) = *(ptr++); // r
		  *(outPtr++) = 0; // resvd
		  break;
		case 24:
		  *(outPtr++) = *(ptr++);
		  *(outPtr++) = *(ptr++);
		  *(outPtr++) = *(ptr++);
		  break;
		case 16:
		  color = ((((*ptr) >> 2) << 10)
			   + (((*(ptr + 1)) >> 2) << 5)
			   + (((*(ptr + 2)) >> 2)));
		  *(outPtr++) = (color >> 8);
		  *(outPtr++) = (color & 255);
		  break;
		case 8:
		  color = (((*ptr))) + (((*(ptr + 1)))) + (((*(ptr + 2))));
		  color /= 3;
		  *(outPtr++) = color;
		  break;
		case 1:
		  if (color)
		    outPtr[column / 8] |= (1 << (7 - (column & 7)));
		  else
		    outPtr[column / 8] &= ~(1 << (7 - (column & 7)));
		}
	    }
	  else if (effXDepth == 16)
	    {
	      ptr += posX * (effXBPP / 8);
	
	      /* Out of X icon bounds, leave space blank */
	      if (posX >= pixmap->drawable.width
		  || posY >= pixmap->drawable.height)
		ptr = (unsigned char *) &zero;
	      color = ((*ptr) << 8) + (*(ptr + 1));
	      switch (effBPP)
		{
		case 32:
		  *(outPtr++) = (color & 31) << 2;
		  *(outPtr++) = ((color >> 5) & 31) << 2;
		  *(outPtr++) = ((color >> 10) & 31) << 2;
		  *(outPtr++) = 0; // resvd
		  break;
		case 24:
		  *(outPtr++) = (color & 31) << 2;
		  *(outPtr++) = ((color >> 5) & 31) << 2;
		  *(outPtr++) = ((color >> 10) & 31) << 2;
		  break;
		case 16:
		  *(outPtr++) = *(ptr++);
		  *(outPtr++) = *(ptr++);
		  break;
		case 8:
		  *(outPtr++) = (((color & 31)
				  + ((color >> 5) & 31)
				  + ((color >> 10) & 31)) / 3) << 2;
		  break;
		case 1:
		  if (color)
		    outPtr[column / 8] |= (1 << (7 - (column & 7)));
		  else
		    outPtr[column / 8] &= ~(1 << (7 - (column & 7)));
		  break;
		} /* end switch(effbpp) */
	    } /* end if effxbpp==16) */
	} /* end for column */
    } /* end for row */
  free (iconData);
}


/*
 * Attempt to create a custom icon from the WM_HINTS bitmaps
 */

HICON
winXIconToHICON (WindowPtr pWin)
{
  unsigned char		*mask, *image, *imageMask;
  unsigned char		*dst, *src;
  PixmapPtr		iconPtr;
  PixmapPtr		maskPtr;
  int			iconSize, planes, bpp, effBPP, stride, maskStride, i;
  HDC			hDC;
  ICONINFO		ii;
  WinXWMHints		hints;
  HICON			hIcon;

  winMultiWindowGetWMHints (pWin, &hints);
  if (!hints.icon_pixmap) return NULL;

  iconPtr = LookupIDByType (hints.icon_pixmap, RT_PIXMAP);
  
  if (!iconPtr) return NULL;
  
  iconSize = 32;
  
  hDC = GetDC (GetDesktopWindow ());
  planes = GetDeviceCaps (hDC, PLANES);
  bpp = GetDeviceCaps (hDC, BITSPIXEL);
  ReleaseDC (GetDesktopWindow (), hDC);
  
  /* 15 BPP is really 16BPP as far as we care */
  if (bpp == 15)
    effBPP = 16;
  else
    effBPP = bpp;
  
  /* Need 32-bit aligned rows */
  stride = ((iconSize * effBPP + 31) & (~31)) / 8;

  /* Mask is 1-bit deep */
  maskStride = ((iconSize * 1 + 31) & (~31)) / 8; 

  image = (unsigned char * ) malloc (stride * iconSize);
  imageMask = (unsigned char *) malloc (stride * iconSize);
  mask = (unsigned char *) malloc (maskStride * iconSize);
  
  /* Default to a completely black mask */
  memset (mask, 0, maskStride * iconSize);
  
  winScaleXBitmapToWindows (iconSize, effBPP, iconPtr, image);
  maskPtr = LookupIDByType (hints.icon_mask, RT_PIXMAP);

  if (maskPtr) 
    {
      winScaleXBitmapToWindows (iconSize, 1, maskPtr, mask);
      
      winScaleXBitmapToWindows (iconSize, effBPP, maskPtr, imageMask);
      
      /* Now we need to set all bits of the icon which are not masked */
      /* on to 0 because Color is really an XOR, not an OR function */
      dst = image;
      src = imageMask;

      for (i = 0; i < (stride * iconSize); i++)
	if ((*(src++)))
	  *(dst++) = 0;
	else
	  dst++;
    }
  
  ii.fIcon = TRUE;
  ii.xHotspot = 0; /* ignored */
  ii.yHotspot = 0; /* ignored */
  
  /* Create Win32 mask from pixmap shape */
  ii.hbmMask = CreateBitmap (iconSize, iconSize, planes, 1, mask);

  /* Create Win32 bitmap from pixmap */
  ii.hbmColor = CreateBitmap (iconSize, iconSize, planes, bpp, image);

  /* Merge Win32 mask and bitmap into icon */
  hIcon = CreateIconIndirect (&ii);

  /* Release Win32 mask and bitmap */
  DeleteObject (ii.hbmMask);
  DeleteObject (ii.hbmColor);

  /* Free X mask and bitmap */
  free (mask);
  free (image);
  free (imageMask);

  return hIcon;
}



/*
 * Change the Windows window icon 
 */

void
winUpdateIcon (Window id)
{
  WindowPtr		pWin;
  HICON			hIcon, hiconOld;

  pWin = LookupIDByType (id, RT_WINDOW);
  hIcon = (HICON)winOverrideIcon ((unsigned long)pWin);

  if (!hIcon)
    hIcon = winXIconToHICON (pWin);

  if (hIcon)
    {
      winWindowPriv(pWin);

      if (pWinPriv->hWnd)
	{
	  hiconOld = (HICON) SetClassLong (pWinPriv->hWnd,
					   GCL_HICON,
					   (int) hIcon);
	  
	  /* Delete the icon if its not the default */
	  if (hiconOld != g_hiconX &&
	      !winIconIsOverride((unsigned long)hiconOld))
	    DeleteObject (hiconOld);
	}
    }
}
