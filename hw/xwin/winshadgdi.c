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
/* $XFree86: xc/programs/Xserver/hw/xwin/winshadgdi.c,v 1.22 2003/02/12 15:01:38 alanh Exp $ */

#include "win.h"

/*
 * Local function prototypes
 */

BOOL CALLBACK
winRedrawAllProcShadowGDI (HWND hwnd, LPARAM lParam);


/*
 * Internal function to get the DIB format that is compatible with the screen
 */

static
Bool
winQueryScreenDIBFormat (ScreenPtr pScreen, BITMAPINFOHEADER *pbmih)
{
  winScreenPriv(pScreen);
  HBITMAP		hbmp;
#if CYGDEBUG
  LPDWORD		pdw = NULL;
#endif
  
  /* Create a memory bitmap compatible with the screen */
  hbmp = CreateCompatibleBitmap (pScreenPriv->hdcScreen, 1, 1);
  if (hbmp == NULL)
    {
      ErrorF ("winQueryScreenDIBFormat - CreateCompatibleBitmap failed\n");
      return FALSE;
    }
  
  /* Initialize our bitmap info header */
  ZeroMemory (pbmih, sizeof (BITMAPINFOHEADER) + 256 * sizeof (RGBQUAD));
  pbmih->biSize = sizeof (BITMAPINFOHEADER);

  /* Get the biBitCount */
  if (!GetDIBits (pScreenPriv->hdcScreen,
		  hbmp,
		  0, 1,
		  NULL,
		  (BITMAPINFO*) pbmih,
		  DIB_RGB_COLORS))
    {
      ErrorF ("winQueryScreenDIBFormat - First call to GetDIBits failed\n");
      DeleteObject (hbmp);
      return FALSE;
    }

#if CYGDEBUG
  /* Get a pointer to bitfields */
  pdw = (DWORD*) ((CARD8*)pbmih + sizeof (BITMAPINFOHEADER));

  ErrorF ("winQueryScreenDIBFormat - First call masks: %08x %08x %08x\n",
	  pdw[0], pdw[1], pdw[2]);
#endif

  /* Get optimal color table, or the optimal bitfields */
  if (!GetDIBits (pScreenPriv->hdcScreen,
		  hbmp,
		  0, 1,
		  NULL,
		  (BITMAPINFO*)pbmih,
		  DIB_RGB_COLORS))
    {
      ErrorF ("winQueryScreenDIBFormat - Second call to GetDIBits "
	      "failed\n");
      DeleteObject (hbmp);
      return FALSE;
    }

  /* Free memory */
  DeleteObject (hbmp);
  
  return TRUE;
}


/*
 * Internal function to determine the GDI bits per rgb and bit masks
 */

static
Bool
winQueryRGBBitsAndMasks (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  BITMAPINFOHEADER	*pbmih = NULL;
  Bool			fReturn = TRUE;
  LPDWORD		pdw = NULL;
  DWORD			dwRedBits, dwGreenBits, dwBlueBits;

  /* Color masks for 8 bpp are standardized */
  if (GetDeviceCaps (pScreenPriv->hdcScreen, RASTERCAPS) & RC_PALETTE)
    {
      /* 
       * RGB BPP for 8 bit palletes is always 8
       * and the color masks are always 0.
       */
      pScreenPriv->dwBitsPerRGB = 8;
      pScreenPriv->dwRedMask = 0x0L;
      pScreenPriv->dwGreenMask = 0x0L;
      pScreenPriv->dwBlueMask = 0x0L;
      return TRUE;
    }

  /* Color masks for 24 bpp are standardized */
  if (GetDeviceCaps (pScreenPriv->hdcScreen, PLANES)
      * GetDeviceCaps (pScreenPriv->hdcScreen, BITSPIXEL) == 24)
    {
      ErrorF ("winQueryRGBBitsAndMasks - GetDeviceCaps (BITSPIXEL) "
	      "returned 24 for the screen.  Using default 24bpp masks.\n");

      /* 8 bits per primary color */
      pScreenPriv->dwBitsPerRGB = 8;

      /* Set screen privates masks */
      pScreenPriv->dwRedMask = WIN_24BPP_MASK_RED;
      pScreenPriv->dwGreenMask = WIN_24BPP_MASK_GREEN;
      pScreenPriv->dwBlueMask = WIN_24BPP_MASK_BLUE;
      
      return TRUE;
    }

  /* Allocate a bitmap header and color table */
  pbmih = (BITMAPINFOHEADER*) malloc (sizeof (BITMAPINFOHEADER)
				      + 256  * sizeof (RGBQUAD));
  if (pbmih == NULL)
    {
      ErrorF ("winQueryRGBBitsAndMasks - malloc failed\n");
      return FALSE;
    }

  /* Get screen description */
  if (winQueryScreenDIBFormat (pScreen, pbmih))
    {
      /* Get a pointer to bitfields */
      pdw = (DWORD*) ((CARD8*)pbmih + sizeof (BITMAPINFOHEADER));
      
#if CYGDEBUG
      ErrorF ("winQueryRGBBitsAndMasks - Masks: %08x %08x %08x\n",
	      pdw[0], pdw[1], pdw[2]);
#endif

      /* Count the number of bits in each mask */
      dwRedBits = winCountBits (pdw[0]);
      dwGreenBits = winCountBits (pdw[1]);
      dwBlueBits = winCountBits (pdw[2]);

      /* Find maximum bits per red, green, blue */
      if (dwRedBits > dwGreenBits && dwRedBits > dwBlueBits)
	pScreenPriv->dwBitsPerRGB = dwRedBits;
      else if (dwGreenBits > dwRedBits && dwGreenBits > dwBlueBits)
	pScreenPriv->dwBitsPerRGB = dwGreenBits;
      else
	pScreenPriv->dwBitsPerRGB = dwBlueBits;

      /* Set screen privates masks */
      pScreenPriv->dwRedMask = pdw[0];
      pScreenPriv->dwGreenMask = pdw[1];
      pScreenPriv->dwBlueMask = pdw[2];
    }
  else
    {
      ErrorF ("winQueryRGBBitsAndMasks - winQueryScreenDIBFormat failed\n");
      free (pbmih);
      fReturn = FALSE;
    }

  /* Free memory */
  free (pbmih);

  return fReturn;
}


/*
 * Redraw all ---?
 */

BOOL CALLBACK
winRedrawAllProcShadowGDI (HWND hwnd, LPARAM lParam)
{
  char strClassName[100];

  if (GetClassName (hwnd, strClassName, 100))
    {
      if(strcmp (WINDOW_CLASS_X, strClassName) == 0)
	{
	  InvalidateRect (hwnd, NULL, FALSE);
	  UpdateWindow (hwnd);
	}
    }
  return TRUE;
}


/*
 * Allocate a DIB for the shadow framebuffer GDI server
 */

Bool
winAllocateFBShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  BITMAPINFOHEADER	*pbmih = NULL;
  DIBSECTION		dibsection;
  Bool			fReturn = TRUE;

  /* Get device contexts for the screen and shadow bitmap */
  pScreenPriv->hdcScreen = GetDC (pScreenPriv->hwndScreen);
  pScreenPriv->hdcShadow = CreateCompatibleDC (pScreenPriv->hdcScreen);

  /* Allocate bitmap info header */
  pbmih = (BITMAPINFOHEADER*) malloc (sizeof (BITMAPINFOHEADER)
				      + 256 * sizeof (RGBQUAD));
  if (pbmih == NULL)
    {
      ErrorF ("winAllocateFBShadowGDI - malloc () failed\n");
      return FALSE;
    }

  /* Query the screen format */
  fReturn = winQueryScreenDIBFormat (pScreen, pbmih);

  /* Describe shadow bitmap to be created */
  pbmih->biWidth = pScreenInfo->dwWidth;
  pbmih->biHeight = -pScreenInfo->dwHeight;
  
  ErrorF ("winAllocateFBShadowGDI - Creating DIB with width: %d height: %d "
	  "depth: %d\n",
	  pbmih->biWidth, -pbmih->biHeight, pbmih->biBitCount);

  /* Create a DI shadow bitmap with a bit pointer */
  pScreenPriv->hbmpShadow = CreateDIBSection (pScreenPriv->hdcScreen,
					      (BITMAPINFO *) pbmih,
					      DIB_RGB_COLORS,
					      (VOID**) &pScreenInfo->pfb,
					      NULL,
					      0);
  if (pScreenPriv->hbmpShadow == NULL || pScreenInfo->pfb == NULL)
    {
      ErrorF ("winAllocateFBShadowGDI - CreateDIBSection failed\n");
      return FALSE;
    }
  else
    {
#if CYGDEBUG
      ErrorF ("winAllocateFBShadowGDI - Shadow buffer allocated\n");
#endif
    }

  /* Get information about the bitmap that was allocated */
  GetObject (pScreenPriv->hbmpShadow,
	     sizeof (dibsection),
	     &dibsection);

#if CYGDEBUG || YES
  /* Print information about bitmap allocated */
  ErrorF ("winAllocateFBShadowGDI - Dibsection width: %d height: %d "
	  "depth: %d size image: %d\n",
	  dibsection.dsBmih.biWidth, dibsection.dsBmih.biHeight,
	  dibsection.dsBmih.biBitCount,
	  dibsection.dsBmih.biSizeImage);
#endif

  /* Select the shadow bitmap into the shadow DC */
  SelectObject (pScreenPriv->hdcShadow,
		pScreenPriv->hbmpShadow);

#if CYGDEBUG
  ErrorF ("winAllocateFBShadowGDI - Attempting a shadow blit\n");
#endif

  /* Do a test blit from the shadow to the screen, I think */
  fReturn = BitBlt (pScreenPriv->hdcScreen,
		    0, 0,
		    pScreenInfo->dwWidth, pScreenInfo->dwHeight,
		    pScreenPriv->hdcShadow,
		    0, 0,
		    SRCCOPY);
  if (fReturn)
    {
#if CYGDEBUG
      ErrorF ("winAllocateFBShadowGDI - Shadow blit success\n");
#endif
    }
  else
    {
      ErrorF ("winAllocateFBShadowGDI - Shadow blit failure\n");
      return FALSE;
    }

  /* Set screeninfo stride */
  pScreenInfo->dwStride = ((dibsection.dsBmih.biSizeImage
			    / dibsection.dsBmih.biHeight)
			   * 8) / pScreenInfo->dwBPP;

#if CYGDEBUG || YES
  ErrorF ("winAllocateFBShadowGDI - Created shadow stride: %d\n",
	  pScreenInfo->dwStride);
#endif

  /* See if the shadow bitmap will be larger than the DIB size limit */
  if (pScreenInfo->dwWidth * pScreenInfo->dwHeight * pScreenInfo->dwBPP
      >= WIN_DIB_MAXIMUM_SIZE)
    {
      ErrorF ("winAllocateFBShadowGDI - Requested DIB (bitmap) "
	      "will be larger than %d MB.  The surface may fail to be "
	      "allocated on Windows 95, 98, or Me, due to a %d MB limit in "
	      "DIB size.  This limit does not apply to Windows NT/2000, and "
	      "this message may be ignored on those platforms.\n",
	      WIN_DIB_MAXIMUM_SIZE_MB, WIN_DIB_MAXIMUM_SIZE_MB);
    }

  /* Determine our color masks */
  if (!winQueryRGBBitsAndMasks (pScreen))
    {
      ErrorF ("winAllocateFBShadowGDI - winQueryRGBBitsAndMasks failed\n");
      return FALSE;
    }

  /* Redraw all windows */
  if (pScreenInfo->fMultiWindow) EnumWindows(winRedrawAllProcShadowGDI, 0);

  return fReturn;
}


/*
 * Blit the damaged regions of the shadow fb to the screen
 */

void
winShadowUpdateGDI (ScreenPtr pScreen, 
		    shadowBufPtr pBuf)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  RegionPtr		damage = &pBuf->damage;
  DWORD			dwBox = REGION_NUM_RECTS (damage);
  BoxPtr		pBox = REGION_RECTS (damage);
  int			x, y, w, h;
  HRGN			hrgnTemp = NULL, hrgnCombined = NULL;
#if WIN_UPDATE_STATS
  static DWORD		s_dwNonUnitRegions = 0;
  static DWORD		s_dwTotalUpdates = 0;
  static DWORD		s_dwTotalBoxes = 0;
#endif

  /*
   * Return immediately if the app is not active
   * and we are fullscreen, or if we have a bad display depth
   */
  if ((!pScreenPriv->fActive && pScreenInfo->fFullScreen)
      || pScreenPriv->fBadDepth) return;

#if WIN_UPDATE_STATS
  ++s_dwTotalUpdates;
  s_dwTotalBoxes += dwBox;

  if (dwBox != 1)
    {
      ++s_dwNonUnitRegions;
      ErrorF ("winShadowUpdatGDI - dwBox: %d\n", dwBox);
    }
  
  if ((s_dwTotalUpdates % 100) == 0)
    ErrorF ("winShadowUpdateGDI - %d%% non-unity regions, avg boxes: %d "
	    "nu: %d tu: %d\n",
	    (s_dwNonUnitRegions * 100) / s_dwTotalUpdates,
	    s_dwTotalBoxes / s_dwTotalUpdates,
	    s_dwNonUnitRegions, s_dwTotalUpdates);
#endif /* WIN_UPDATE_STATS */

  /*
   * Handle small regions with multiple blits,
   * handle large regions by creating a clipping region and 
   * doing a single blit constrained to that clipping region.
   */
  if (pScreenInfo->dwClipUpdatesNBoxes == 0
      || dwBox < pScreenInfo->dwClipUpdatesNBoxes)
    {
      /* Loop through all boxes in the damaged region */
      while (dwBox--)
	{
	  /*
	   * Calculate x offset, y offset, width, and height for
	   * current damage box
	   */
	  x = pBox->x1;
	  y = pBox->y1;
	  w = pBox->x2 - pBox->x1;
	  h = pBox->y2 - pBox->y1;
	  
	  BitBlt (pScreenPriv->hdcScreen,
		  x, y,
		  w, h,
		  pScreenPriv->hdcShadow,
		  x, y,
		  SRCCOPY);
	  
	  /* Get a pointer to the next box */
	  ++pBox;
	}
    }
  else
    {
      BoxPtr		pBoxExtents = REGION_EXTENTS (pScreen, damage);

      /* Compute a GDI region from the damaged region */
      hrgnCombined = CreateRectRgn (pBox->x1, pBox->y1, pBox->x2, pBox->y2);
      dwBox--;
      pBox++;
      while (dwBox--)
	{
	  hrgnTemp = CreateRectRgn (pBox->x1, pBox->y1, pBox->x2, pBox->y2);
	  CombineRgn (hrgnCombined, hrgnCombined, hrgnTemp, RGN_OR);
	  DeleteObject (hrgnTemp);
	  pBox++;
	}
      
      /* Install the GDI region as a clipping region */
      SelectClipRgn (pScreenPriv->hdcScreen, hrgnCombined);
      DeleteObject (hrgnCombined);
      hrgnCombined = NULL;
      
      /*
       * Blit the shadow buffer to the screen,
       * constrained to the clipping region.
       */
      BitBlt (pScreenPriv->hdcScreen,
	      pBoxExtents->x1, pBoxExtents->y1,
	      pBoxExtents->x2 - pBoxExtents->x1,
	      pBoxExtents->y2 - pBoxExtents->y1,
	      pScreenPriv->hdcShadow,
	      pBoxExtents->x1, pBoxExtents->y1,
	      SRCCOPY);

      /* Reset the clip region */
      SelectClipRgn (pScreenPriv->hdcScreen, NULL);
    }

  /* Redraw all windows */
  if (pScreenInfo->fMultiWindow) EnumWindows(winRedrawAllProcShadowGDI, 0);
}


/* See Porting Layer Definition - p. 33 */
/*
 * We wrap whatever CloseScreen procedure was specified by fb;
 * a pointer to said procedure is stored in our privates.
 */

Bool
winCloseScreenShadowGDI (int nIndex, ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  Bool			fReturn;

#if CYGDEBUG
  ErrorF ("winCloseScreenShadowGDI - Freeing screen resources\n");
#endif

  /* Flag that the screen is closed */
  pScreenPriv->fClosed = TRUE;
  pScreenPriv->fActive = FALSE;

  /* Call the wrapped CloseScreen procedure */
  pScreen->CloseScreen = pScreenPriv->CloseScreen;
  fReturn = (*pScreen->CloseScreen) (nIndex, pScreen);

  /* Delete the window property */
  RemoveProp (pScreenPriv->hwndScreen, WIN_SCR_PROP);

  /* Free the shadow DC; which allows the bitmap to be freed */
  DeleteDC (pScreenPriv->hdcShadow);
  
  /* Free the shadow bitmap */
  DeleteObject (pScreenPriv->hbmpShadow);

  /* Free the screen DC */
  ReleaseDC (pScreenPriv->hwndScreen, pScreenPriv->hdcScreen);

  /* Kill our window */
  if (pScreenPriv->hwndScreen)
    {
      DestroyWindow (pScreenPriv->hwndScreen);
      pScreenPriv->hwndScreen = NULL;
    }

  /* Destroy the thread startup mutex */
  pthread_mutex_destroy (&pScreenPriv->pmServerStarted);

  /* Invalidate our screeninfo's pointer to the screen */
  pScreenInfo->pScreen = NULL;

  /* Invalidate the ScreenInfo's fb pointer */
  pScreenInfo->pfb = NULL;

  /* Free the screen privates for this screen */
  free ((pointer) pScreenPriv);

  return fReturn;
}


/*
 * Tell mi what sort of visuals we need.
 * 
 * Generally we only need one visual, as our screen can only
 * handle one format at a time, I believe.  You may want
 * to verify that last sentence.
 */

Bool
winInitVisualsShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;

  /* Display debugging information */
  ErrorF ("winInitVisualsShadowGDI - Masks %08x %08x %08x BPRGB %d d %d "
	  "bpp %d\n",
	  pScreenPriv->dwRedMask,
	  pScreenPriv->dwGreenMask,
	  pScreenPriv->dwBlueMask,
	  pScreenPriv->dwBitsPerRGB,
	  pScreenInfo->dwDepth,
	  pScreenInfo->dwBPP);

  /* Create a single visual according to the Windows screen depth */
  switch (pScreenInfo->dwDepth)
    {
    case 24:
    case 16:
    case 15:
#if defined(XFree86Server)
      /* Setup the real visual */
      if (!miSetVisualTypesAndMasks (pScreenInfo->dwDepth,
				     TrueColorMask,
				     pScreenPriv->dwBitsPerRGB,
				     -1,
				     pScreenPriv->dwRedMask,
				     pScreenPriv->dwGreenMask,
				     pScreenPriv->dwBlueMask))
	{
	  ErrorF ("winInitVisualsShadowGDI - miSetVisualTypesAndMasks "
		  "failed\n");
	  return FALSE;
	}

#if WIN_EMULATE_PSEUDO_SUPPORT
      if (!pScreenInfo->fEmulatePseudo)
	break;

      /* Setup a pseudocolor visual */
      if (!miSetVisualTypesAndMasks (8,
				     PseudoColorMask,
				     8,
				     -1,
				     0,
				     0,
				     0))
	{
	  ErrorF ("winInitVisualsShadowGDI - miSetVisualTypesAndMasks "
		  "failed for PseudoColor\n");
	  return FALSE;
	}
#endif
#else /* XFree86Server */
      /* Setup the real visual */
      if (!fbSetVisualTypesAndMasks (pScreenInfo->dwDepth,
				     TrueColorMask,
				     pScreenPriv->dwBitsPerRGB,
				     pScreenPriv->dwRedMask,
				     pScreenPriv->dwGreenMask,
				     pScreenPriv->dwBlueMask))
	{
	  ErrorF ("winInitVisualsShadowGDI - fbSetVisualTypesAndMasks "
		  "failed for TrueColor\n");
	  return FALSE;
	}

#if WIN_EMULATE_PSEUDO_SUPPORT
      if (!pScreenInfo->fEmulatePseudo)
	break;

      /* Setup a pseudocolor visual */
      if (!fbSetVisualTypesAndMasks (8,
				     PseudoColorMask,
				     8,
				     0,
				     0,
				     0))
	{
	  ErrorF ("winInitVisualsShadowGDI - fbSetVisualTypesAndMasks "
		  "failed for PseudoColor\n");
	  return FALSE;
	}
#endif
#endif /* XFree86Server */
      break;

    case 8:
#if defined(XFree86Server)
      if (!miSetVisualTypesAndMasks (pScreenInfo->dwDepth,
				     PseudoColorMask,
				     pScreenPriv->dwBitsPerRGB,
				     PseudoColor,
				     pScreenPriv->dwRedMask,
				     pScreenPriv->dwGreenMask,
				     pScreenPriv->dwBlueMask))
	{
	  ErrorF ("winInitVisualsShadowGDI - miSetVisualTypesAndMasks "
		  "failed\n");
	  return FALSE;
	}
#else /* XFree86Server */
      if (!fbSetVisualTypesAndMasks (pScreenInfo->dwDepth,
				     PseudoColorMask,
				     pScreenPriv->dwBitsPerRGB,
				     pScreenPriv->dwRedMask,
				     pScreenPriv->dwGreenMask,
				     pScreenPriv->dwBlueMask))
	{
	  ErrorF ("winInitVisualsShadowGDI - fbSetVisualTypesAndMasks "
		  "failed\n");
	  return FALSE;
	}
#endif
      break;

    default:
      ErrorF ("winInitVisualsShadowGDI - Unknown screen depth\n");
      return FALSE;
    }

#if CYGDEBUG
  ErrorF ("winInitVisualsShadowGDI - Returning\n");
#endif

  return TRUE;
}


/*
 * Adjust the proposed video mode
 */

Bool
winAdjustVideoModeShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  HDC			hdc;
  DWORD			dwBPP;
  
  hdc = GetDC (NULL);

  /* We're in serious trouble if we can't get a DC */
  if (hdc == NULL)
    {
      ErrorF ("winAdjustVideoModeShadowGDI - GetDC () failed\n");
      return FALSE;
    }

  /* Query GDI for current display depth */
  dwBPP = GetDeviceCaps (hdc, BITSPIXEL);

  /* GDI cannot change the screen depth */
  if (pScreenInfo->dwBPP == WIN_DEFAULT_BPP)
    {
      /* No -depth parameter passed, let the user know the depth being used */
      ErrorF ("winAdjustVideoModeShadowGDI - Using Windows display "
	      "depth of %d bits per pixel\n", dwBPP);

      /* Use GDI's depth */
      pScreenInfo->dwBPP = dwBPP;
    }
  else if (dwBPP != pScreenInfo->dwBPP)
    {
      /* Warn user if GDI depth is different than -depth parameter */
      ErrorF ("winAdjustVideoModeShadowGDI - Command line bpp: %d, "\
	      "using bpp: %d\n", pScreenInfo->dwBPP, dwBPP);

      /* We'll use GDI's depth */
      pScreenInfo->dwBPP = dwBPP;
    }
  
  /* Release our DC */
  ReleaseDC (NULL, hdc);
  hdc = NULL;

  return TRUE;
}


/*
 * Blt exposed regions to the screen
 */

Bool
winBltExposedRegionsShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  winPrivCmapPtr	pCmapPriv = NULL;
  HDC			hdcUpdate;
  PAINTSTRUCT		ps;

  /* BeginPaint gives us an hdc that clips to the invalidated region */
  hdcUpdate = BeginPaint (pScreenPriv->hwndScreen, &ps);

  /* Realize the palette, if we have one */
  if (pScreenPriv->pcmapInstalled != NULL)
    {
      pCmapPriv = winGetCmapPriv (pScreenPriv->pcmapInstalled);
      
      SelectPalette (hdcUpdate, pCmapPriv->hPalette, FALSE);
      RealizePalette (hdcUpdate);
    }

  /* Our BitBlt will be clipped to the invalidated region */
  BitBlt (hdcUpdate,
	  0, 0,
	  pScreenInfo->dwWidth, pScreenInfo->dwHeight,
	  pScreenPriv->hdcShadow,
	  0, 0,
	  SRCCOPY);

  /* EndPaint frees the DC */
  EndPaint (pScreenPriv->hwndScreen, &ps);

  /* Redraw all windows */
  if (pScreenInfo->fMultiWindow) EnumWindows(winRedrawAllProcShadowGDI, 0);

  return TRUE;
}


/*
 * Do any engine-specific appliation-activation processing
 */

Bool
winActivateAppShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;

#if CYGDEBUG
  ErrorF ("winActivateAppShadowGDI\n");
#endif

  /*
   * Are we active?
   * Are we fullscreen?
   */
  if (pScreenPriv->fActive
      && pScreenInfo->fFullScreen)
    {
      /*
       * Activating, attempt to bring our window 
       * to the top of the display
       */
      ShowWindow (pScreenPriv->hwndScreen, SW_RESTORE);
    }
  else if (!pScreenPriv->fActive
	   && pScreenInfo->fFullScreen)
    {
      /*
       * Deactivating, stuff our window onto the
       * task bar.
       */
      ShowWindow (pScreenPriv->hwndScreen, SW_MINIMIZE);
    }

#if CYGDEBUG
  ErrorF ("winActivateAppShadowGDI - Returning\n");
#endif

  return TRUE;
}


/*
 * Reblit the shadow framebuffer to the screen.
 */

Bool
winRedrawScreenShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;

  /* Redraw the whole window, to take account for the new colors */
  BitBlt (pScreenPriv->hdcScreen,
	  0, 0,
	  pScreenInfo->dwWidth, pScreenInfo->dwHeight,
	  pScreenPriv->hdcShadow,
	  0, 0,
	  SRCCOPY);

  /* Redraw all windows */
  if (pScreenInfo->fMultiWindow) EnumWindows(winRedrawAllProcShadowGDI, 0);
  return TRUE;
}


/*
 * Realize the currently installed colormap
 */

Bool
winRealizeInstalledPaletteShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winPrivCmapPtr	pCmapPriv = NULL;

#if CYGDEBUG
  ErrorF ("winRealizeInstalledPaletteShadowGDI\n");
#endif

  /* Don't do anything if there is not a colormap */
  if (pScreenPriv->pcmapInstalled == NULL)
    {
#if CYGDEBUG
      ErrorF ("winRealizeInstalledPaletteShadowGDI - No colormap "
	      "installed\n");
#endif
      return TRUE;
    }

  pCmapPriv = winGetCmapPriv (pScreenPriv->pcmapInstalled);
  
  /* Realize our palette for the screen */
  if (RealizePalette (pScreenPriv->hdcScreen) == GDI_ERROR)
    {
      ErrorF ("winRealizeInstalledPaletteShadowGDI - RealizePalette () "
	      "failed\n");
      return FALSE;
    }
  
  /* Set the DIB color table */
  if (SetDIBColorTable (pScreenPriv->hdcShadow,
			0,
			WIN_NUM_PALETTE_ENTRIES,
			pCmapPriv->rgbColors) == 0)
    {
      ErrorF ("winRealizeInstalledPaletteShadowGDI - SetDIBColorTable () "
	      "failed\n");
      return FALSE;
    }
  
  return TRUE;
}


/*
 * Install the specified colormap
 */

Bool
winInstallColormapShadowGDI (ColormapPtr pColormap)
{
  ScreenPtr		pScreen = pColormap->pScreen;
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  winCmapPriv(pColormap);

  /*
   * Tell Windows to install the new colormap
   */
  if (SelectPalette (pScreenPriv->hdcScreen,
		     pCmapPriv->hPalette,
		     FALSE) == NULL)
    {
      ErrorF ("winInstallColormapShadowGDI - SelectPalette () failed\n");
      return FALSE;
    }
      
  /* Realize the palette */
  if (GDI_ERROR == RealizePalette (pScreenPriv->hdcScreen))
    {
      ErrorF ("winInstallColormapShadowGDI - RealizePalette () failed\n");
      return FALSE;
    }

  /* Set the DIB color table */
  if (SetDIBColorTable (pScreenPriv->hdcShadow,
			0,
			WIN_NUM_PALETTE_ENTRIES,
			pCmapPriv->rgbColors) == 0)
    {
      ErrorF ("winInstallColormapShadowGDI - SetDIBColorTable () failed\n");
      return FALSE;
    }

  /* Redraw the whole window, to take account for the new colors */
  BitBlt (pScreenPriv->hdcScreen,
	  0, 0,
	  pScreenInfo->dwWidth, pScreenInfo->dwHeight,
	  pScreenPriv->hdcShadow,
	  0, 0,
	  SRCCOPY);

  /* Save a pointer to the newly installed colormap */
  pScreenPriv->pcmapInstalled = pColormap;

  /* Redraw all windows */
  if (pScreenInfo->fMultiWindow) EnumWindows(winRedrawAllProcShadowGDI, 0);

  return TRUE;
}


/*
 * Store the specified colors in the specified colormap
 */

Bool
winStoreColorsShadowGDI (ColormapPtr pColormap,
			 int ndef,
			 xColorItem *pdefs)
{
  ScreenPtr		pScreen = pColormap->pScreen;
  winScreenPriv(pScreen);
  winCmapPriv(pColormap);
  ColormapPtr curpmap = pScreenPriv->pcmapInstalled;
  
  /* Put the X colormap entries into the Windows logical palette */
  if (SetPaletteEntries (pCmapPriv->hPalette,
			 pdefs[0].pixel,
			 ndef,
			 pCmapPriv->peColors + pdefs[0].pixel) == 0)
    {
      ErrorF ("winStoreColorsShadowGDI - SetPaletteEntries () failed\n");
      return FALSE;
    }

  /* Don't install the Windows palette if the colormap is not installed */
  if (pColormap != curpmap)
    {
      return TRUE;
    }

  /* Try to install the newly modified colormap */
  if (!winInstallColormapShadowGDI (pColormap))
    {
      ErrorF ("winInstallColormapShadowGDI - winInstallColormapShadowGDI "
	      "failed\n");
      return FALSE;
    }

#if 0
  /* Tell Windows that the palette has changed */
  RealizePalette (pScreenPriv->hdcScreen);
  
  /* Set the DIB color table */
  if (SetDIBColorTable (pScreenPriv->hdcShadow,
			pdefs[0].pixel,
			ndef,
			pCmapPriv->rgbColors + pdefs[0].pixel) == 0)
    {
      ErrorF ("winInstallColormapShadowGDI - SetDIBColorTable () failed\n");
      return FALSE;
    }

  /* Save a pointer to the newly installed colormap */
  pScreenPriv->pcmapInstalled = pColormap;
#endif

  return TRUE;
}


/*
 * Colormap initialization procedure
 */

Bool
winCreateColormapShadowGDI (ColormapPtr pColormap)
{
  LPLOGPALETTE		lpPaletteNew = NULL;
  DWORD			dwEntriesMax;
  VisualPtr		pVisual;
  HPALETTE		hpalNew = NULL;
  winCmapPriv(pColormap);

  /* Get a pointer to the visual that the colormap belongs to */
  pVisual = pColormap->pVisual;

  /* Get the maximum number of palette entries for this visual */
  dwEntriesMax = pVisual->ColormapEntries;

  /* Allocate a Windows logical color palette with max entries */
  lpPaletteNew = malloc (sizeof (LOGPALETTE)
			 + (dwEntriesMax - 1) * sizeof (PALETTEENTRY));
  if (lpPaletteNew == NULL)
    {
      ErrorF ("winCreateColormapShadowGDI - Couldn't allocate palette "
	      "with %d entries\n",
	      dwEntriesMax);
      return FALSE;
    }

  /* Zero out the colormap */
  ZeroMemory (lpPaletteNew, sizeof (LOGPALETTE)
	      + (dwEntriesMax - 1) * sizeof (PALETTEENTRY));
  
  /* Set the logical palette structure */
  lpPaletteNew->palVersion = 0x0300;
  lpPaletteNew->palNumEntries = dwEntriesMax;

  /* Tell Windows to create the palette */
  hpalNew = CreatePalette (lpPaletteNew);
  if (hpalNew == NULL)
    {
      ErrorF ("winCreateColormapShadowGDI - CreatePalette () failed\n");
      free (lpPaletteNew);
      return FALSE;
    }

  /* Save the Windows logical palette handle in the X colormaps' privates */
  pCmapPriv->hPalette = hpalNew;

  /* Free the palette initialization memory */
  free (lpPaletteNew);

  return TRUE;
}


/*
 * Colormap destruction procedure
 */

Bool
winDestroyColormapShadowGDI (ColormapPtr pColormap)
{
  winScreenPriv(pColormap->pScreen);
  winCmapPriv(pColormap);

  /*
   * Is colormap to be destroyed the default?
   *
   * Non-default colormaps should have had winUninstallColormap
   * called on them before we get here.  The default colormap
   * will not have had winUninstallColormap called on it.  Thus,
   * we need to handle the default colormap in a special way.
   */
  if (pColormap->flags & IsDefault)
    {
#if CYGDEBUG
      ErrorF ("winDestroyColormapShadowGDI - Destroying default "
	      "colormap\n");
#endif
      
      /*
       * FIXME: Walk the list of all screens, popping the default
       * palette out of each screen device context.
       */
      
      /* Pop the palette out of the device context */
      SelectPalette (pScreenPriv->hdcScreen,
		     GetStockObject (DEFAULT_PALETTE),
		     FALSE);

      /* Clear our private installed colormap pointer */
      pScreenPriv->pcmapInstalled = NULL;
    }
  
  /* Try to delete the logical palette */
  if (DeleteObject (pCmapPriv->hPalette) == 0)
    {
      ErrorF ("winDestroyColormap - DeleteObject () failed\n");
      return FALSE;
    }
  
  /* Invalidate the colormap privates */
  pCmapPriv->hPalette = NULL;

  return TRUE;
}


/*
 * Set engine specific funtions
 */

Bool
winSetEngineFunctionsShadowGDI (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  
  /* Set our pointers */
  pScreenPriv->pwinAllocateFB = winAllocateFBShadowGDI;
  pScreenPriv->pwinShadowUpdate = winShadowUpdateGDI;
  pScreenPriv->pwinCloseScreen = winCloseScreenShadowGDI;
  pScreenPriv->pwinInitVisuals = winInitVisualsShadowGDI;
  pScreenPriv->pwinAdjustVideoMode = winAdjustVideoModeShadowGDI;
  if (pScreenInfo->fFullScreen)
    pScreenPriv->pwinCreateBoundingWindow = winCreateBoundingWindowFullScreen;
  else
    pScreenPriv->pwinCreateBoundingWindow = winCreateBoundingWindowWindowed;
  pScreenPriv->pwinFinishScreenInit = winFinishScreenInitFB;
  pScreenPriv->pwinBltExposedRegions = winBltExposedRegionsShadowGDI;
  pScreenPriv->pwinActivateApp = winActivateAppShadowGDI;
  pScreenPriv->pwinRedrawScreen = winRedrawScreenShadowGDI;
  pScreenPriv->pwinRealizeInstalledPalette = 
    winRealizeInstalledPaletteShadowGDI;
  pScreenPriv->pwinInstallColormap = winInstallColormapShadowGDI;
  pScreenPriv->pwinStoreColors = winStoreColorsShadowGDI;
  pScreenPriv->pwinCreateColormap = winCreateColormapShadowGDI;
  pScreenPriv->pwinDestroyColormap = winDestroyColormapShadowGDI;
  pScreenPriv->pwinHotKeyAltTab = (winHotKeyAltTabProcPtr) (void (*)())NoopDDA;
  pScreenPriv->pwinCreatePrimarySurface
    = (winCreatePrimarySurfaceProcPtr) (void (*)())NoopDDA;
  pScreenPriv->pwinReleasePrimarySurface
    = (winReleasePrimarySurfaceProcPtr) (void (*)())NoopDDA;

  return TRUE;
}
