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
 *		Kensuke Matsuzaki
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winwindow.c,v 1.10 2003/12/22 01:34:20 dickey Exp $ */

#include "win.h"

/*
 * Prototypes for local functions
 */

static int
winAddRgn (WindowPtr pWindow, pointer data);

static
void
winUpdateRgnPRootless (WindowPtr pWindow);

#ifdef SHAPE
static
void
winReshapePRootless (WindowPtr pWin);
#endif


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbCreateWindow() */

Bool
winCreateWindowNativeGDI (WindowPtr pWin)
{
  ErrorF ("winCreateWindowNativeGDI ()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbDestroyWindow() */

Bool
winDestroyWindowNativeGDI (WindowPtr pWin)
{
  ErrorF ("winDestroyWindowNativeGDI ()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbPositionWindow() */

Bool
winPositionWindowNativeGDI (WindowPtr pWin, int x, int y)
{
  ErrorF ("winPositionWindowNativeGDI ()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 39 */
/* See mfb/mfbwindow.c - mfbCopyWindow() */

void 
winCopyWindowNativeGDI (WindowPtr pWin,
			DDXPointRec ptOldOrg,
			RegionPtr prgnSrc)
{
  DDXPointPtr		pptSrc;
  DDXPointPtr		ppt;
  RegionPtr		prgnDst;
  BoxPtr		pBox;
  int			dx, dy;
  int			i, nbox;
  WindowPtr		pwinRoot;
  BoxPtr		pBoxDst;
  ScreenPtr		pScreen = pWin->drawable.pScreen;
  winScreenPriv(pScreen);

#if 0
  ErrorF ("winCopyWindow\n");
#endif

  /* Get a pointer to the root window */
  pwinRoot = WindowTable[pWin->drawable.pScreen->myNum];

  /* Create a region for the destination */
  prgnDst = REGION_CREATE(pWin->drawable.pScreen, NULL, 1);

  /* Calculate the shift from the source to the destination */
  dx = ptOldOrg.x - pWin->drawable.x;
  dy = ptOldOrg.y - pWin->drawable.y;

  /* Translate the region from the destination to the source? */
  REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);
  REGION_INTERSECT(pWin->drawable.pScreen, prgnDst, &pWin->borderClip,
		   prgnSrc);

  /* Get a pointer to the first box in the region to be copied */
  pBox = REGION_RECTS(prgnDst);
  
  /* Get the number of boxes in the region */
  nbox = REGION_NUM_RECTS(prgnDst);

  /* Allocate source points for each box */
  if(!(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL(nbox * sizeof(DDXPointRec))))
    return;

  /* Set an iterator pointer */
  ppt = pptSrc;

  /* Calculate the source point of each box? */
  for (i = nbox; --i >= 0; ppt++, pBox++)
    {
      ppt->x = pBox->x1 + dx;
      ppt->y = pBox->y1 + dy;
    }

  /* Setup loop pointers again */
  pBoxDst = REGION_RECTS(prgnDst);
  ppt = pptSrc;

#if 0
  ErrorF ("winCopyWindow - x1\tx2\ty1\ty2\tx\ty\n");
#endif

  /* BitBlt each source to the destination point */
  for (i = nbox; --i >= 0; pBoxDst++, ppt++)
    {
#if 0
      ErrorF ("winCopyWindow - %d\t%d\t%d\t%d\t%d\t%d\n",
	      pBoxDst->x1, pBoxDst->x2, pBoxDst->y1, pBoxDst->y2,
	      ppt->x, ppt->y);
#endif

      BitBlt (pScreenPriv->hdcScreen,
	      pBoxDst->x1, pBoxDst->y1,
	      pBoxDst->x2 - pBoxDst->x1, pBoxDst->y2 - pBoxDst->y1,
	      pScreenPriv->hdcScreen,
	      ppt->x, ppt->y,
	      SRCCOPY);
    }

  /* Cleanup the regions, etc. */
  DEALLOCATE_LOCAL(pptSrc);
  REGION_DESTROY(pWin->drawable.pScreen, prgnDst);
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbChangeWindowAttributes() */

Bool
winChangeWindowAttributesNativeGDI (WindowPtr pWin, unsigned long mask)
{
  ErrorF ("winChangeWindowAttributesNativeGDI ()\n");
  return TRUE;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as UnrealizeWindow
 */

Bool
winUnmapWindowNativeGDI (WindowPtr pWindow)
{
  ErrorF ("winUnmapWindowNativeGDI ()\n");
  /* This functions is empty in the CFB,
   * we probably won't need to do anything
   */
  return TRUE;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as RealizeWindow
 */

Bool
winMapWindowNativeGDI (WindowPtr pWindow)
{
  ErrorF ("winMapWindowNativeGDI ()\n");
  /* This function is empty in the CFB,
   * we probably won't need to do anything
   */
  return TRUE;

}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbCreateWindow() */

Bool
winCreateWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;
  winWindowPriv(pWin);

#if CYGDEBUG
  ErrorF ("winCreateWindowPRootless ()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->CreateWindow(pWin);
  
  pWinPriv->hRgn = NULL;
  
  return fResult;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbDestroyWindow() */

Bool
winDestroyWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;
  winWindowPriv(pWin);

#if CYGDEBUG
  ErrorF ("winDestroyWindowPRootless ()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->DestroyWindow(pWin);
  
  if (pWinPriv->hRgn != NULL)
    {
      DeleteObject(pWinPriv->hRgn);
      pWinPriv->hRgn = NULL;
    }
  
  winUpdateRgnPRootless (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbPositionWindow() */

Bool
winPositionWindowPRootless (WindowPtr pWin, int x, int y)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winPositionWindowPRootless ()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->PositionWindow(pWin, x, y);
  
  winUpdateRgnPRootless (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37 */
/* See mfb/mfbwindow.c - mfbChangeWindowAttributes() */

Bool
winChangeWindowAttributesPRootless (WindowPtr pWin, unsigned long mask)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winChangeWindowAttributesPRootless ()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->ChangeWindowAttributes(pWin, mask);
  
  winUpdateRgnPRootless (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as UnrealizeWindow
 */

Bool
winUnmapWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;
  winWindowPriv(pWin);

#if CYGDEBUG
  ErrorF ("winUnmapWindowPRootless ()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->UnrealizeWindow(pWin);
  
  if (pWinPriv->hRgn != NULL)
    {
      DeleteObject(pWinPriv->hRgn);
      pWinPriv->hRgn = NULL;
    }
  
  winUpdateRgnPRootless (pWin);
  
  return fResult;
}


/* See Porting Layer Definition - p. 37
 * Also referred to as RealizeWindow
 */

Bool
winMapWindowPRootless (WindowPtr pWin)
{
  Bool			fResult = FALSE;

#if CYGDEBUG
  ErrorF ("winMapWindowPRootless ()\n");
#endif

  fResult = winGetScreenPriv(pWin->drawable.pScreen)->RealizeWindow(pWin);
  
  winReshapePRootless (pWin);
  
  winUpdateRgnPRootless (pWin);
  
  return fResult;
}


#ifdef SHAPE
void
winSetShapePRootless (WindowPtr pWin)
{
#if CYGDEBUG
  ErrorF ("winSetShapePRootless ()\n");
#endif

  winGetScreenPriv(pWin->drawable.pScreen)->SetShape(pWin);
  
  winReshapePRootless (pWin);
  winUpdateRgnPRootless (pWin);
  
  return;
}
#endif


/*
 * Local function for adding a region to the Windows window region
 */

static
int
winAddRgn (WindowPtr pWin, pointer data)
{
  int		iX, iY, iWidth, iHeight, iBorder;
  HRGN		hRgn = *(HRGN*)data;
  HRGN		hRgnWin;
  winWindowPriv(pWin);
  
  /* If pWin is not Root */
  if (pWin->parent != NULL) 
    {
#if CYGDEBUG
      ErrorF ("winAddRgn ()\n");
#endif
      if (pWin->mapped)
	{
	  iBorder = wBorderWidth (pWin);
	  
	  iX = pWin->drawable.x - iBorder;
	  iY = pWin->drawable.y - iBorder;
	  
	  iWidth = pWin->drawable.width + iBorder * 2;
	  iHeight = pWin->drawable.height + iBorder * 2;
	  
	  hRgnWin = CreateRectRgn (0, 0, iWidth, iHeight);
	  
	  if (hRgnWin == NULL)
	    {
	      ErrorF ("winAddRgn - CreateRectRgn () failed\n");
	      ErrorF ("  Rect %d %d %d %d\n",
		      iX, iY, iX + iWidth, iY + iHeight);
	    }
	  
	  if (pWinPriv->hRgn)
	    {
	      if (CombineRgn (hRgnWin, hRgnWin, pWinPriv->hRgn, RGN_AND)
		  == ERROR)
		{
		  ErrorF ("winAddRgn - CombineRgn () failed\n");
		}
	    }
	  
	  OffsetRgn (hRgnWin, iX, iY);

	  if (CombineRgn (hRgn, hRgn, hRgnWin, RGN_OR) == ERROR)
	    {
	      ErrorF ("winAddRgn - CombineRgn () failed\n");
	    }
	  
	  DeleteObject (hRgnWin);
	}
      return WT_DONTWALKCHILDREN;
    }
  else
    {
      return WT_WALKCHILDREN;
    }
}


/*
 * Local function to update the Windows window's region
 */

static
void
winUpdateRgnPRootless (WindowPtr pWin)
{
  HRGN		hRgn = CreateRectRgn (0, 0, 0, 0);
  
  if (hRgn != NULL)
    {
      WalkTree (pWin->drawable.pScreen, winAddRgn, &hRgn);
      SetWindowRgn (winGetScreenPriv(pWin->drawable.pScreen)->hwndScreen,
		    hRgn, TRUE);
    }
  else
    {
      ErrorF ("winUpdateRgnPRootless - CreateRectRgn failed.\n");
    }
}


#ifdef SHAPE
static
void
winReshapePRootless (WindowPtr pWin)
{
  int		nRects;
  ScreenPtr	pScreen = pWin->drawable.pScreen;
  RegionRec	rrNewShape;
  BoxPtr	pShape, pRects, pEnd;
  HRGN		hRgn, hRgnRect;
  winWindowPriv(pWin);

#if CYGDEBUG
  ErrorF ("winReshapePRootless ()\n");
#endif

  /* Bail if the window is the root window */
  if (pWin->parent == NULL)
    return;

  /* Bail if the window is not top level */
  if (pWin->parent->parent != NULL)
    return;

  /* Free any existing window region stored in the window privates */
  if (pWinPriv->hRgn != NULL)
    {
      DeleteObject (pWinPriv->hRgn);
      pWinPriv->hRgn = NULL;
    }
  
  /* Bail if the window has no bounding region defined */
  if (!wBoundingShape (pWin))
    return;

  REGION_NULL(pScreen, &rrNewShape);
  REGION_COPY(pScreen, &rrNewShape, wBoundingShape(pWin));
  REGION_TRANSLATE(pScreen, &rrNewShape, pWin->borderWidth,
                   pWin->borderWidth);
  
  nRects = REGION_NUM_RECTS(&rrNewShape);
  pShape = REGION_RECTS(&rrNewShape);
  
  if (nRects > 0)
    {
      /* Create initial empty Windows region */
      hRgn = CreateRectRgn (0, 0, 0, 0);

      /* Loop through all rectangles in the X region */
      for (pRects = pShape, pEnd = pShape + nRects; pRects < pEnd; pRects++)
        {
	  /* Create a Windows region for the X rectangle */
	  hRgnRect = CreateRectRgn (pRects->x1, pRects->y1,
				    pRects->x2, pRects->y2);
	  if (hRgnRect == NULL)
	    {
	      ErrorF("winReshapePRootless - CreateRectRgn() failed\n");
	    }

	  /* Merge the Windows region with the accumulated region */
	  if (CombineRgn (hRgn, hRgn, hRgnRect, RGN_OR) == ERROR)
	    {
	      ErrorF("winReshapePRootless - CombineRgn() failed\n");
	    }

	  /* Delete the temporary Windows region */
	  DeleteObject (hRgnRect);
        }
      
      /* Save a handle to the composite region in the window privates */
      pWinPriv->hRgn = hRgn;
    }

  REGION_UNINIT(pScreen, &rrNewShape);
  
  return;
}
#endif
