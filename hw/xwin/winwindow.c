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
/* $XFree86: xc/programs/Xserver/hw/xwin/winwindow.c,v 1.6 2003/02/12 15:01:38 alanh Exp $ */

#include "win.h"

/*
 * Prototypes for local functions
 */

static int
winAddRgn (WindowPtr pWindow, pointer data);

static
void
winUpdateRgn (WindowPtr pWindow);

#ifdef SHAPE
static
void
winReshape (WindowPtr pWin);
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
  ErrorF ("winCopyWindowNativeGDI ()\n");
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
  /*winUpdateRgn (pWin);*/
  
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
  
  winUpdateRgn (pWin);
  
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
  
  winUpdateRgn (pWin);
  
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
  
  winUpdateRgn (pWin);
  
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
  
  winUpdateRgn (pWin);
  
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
  
  winReshape (pWin);
  
  winUpdateRgn (pWin);
  
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
  
  winReshape (pWin);
  winUpdateRgn (pWin);
  
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
winUpdateRgn (WindowPtr pWin)
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
      ErrorF ("winUpdateRgn - CreateRectRgn failed.\n");
    }
}


#ifdef SHAPE
static
void
winReshape (WindowPtr pWin)
{
  int		nRects;
  ScreenPtr	pScreen = pWin->drawable.pScreen;
  RegionRec	rrNewShape;
  BoxPtr	pShape, pRects, pEnd;
  HRGN		hRgn, hRgnRect;
  winWindowPriv(pWin);

#if CYGDEBUG
  ErrorF ("winReshape ()\n");
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

  REGION_INIT(pScreen, &rrNewShape, NullBox, 0);
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
	      ErrorF("winReshape - CreateRectRgn() failed\n");
	    }

	  /* Merge the Windows region with the accumulated region */
	  if (CombineRgn (hRgn, hRgn, hRgnRect, RGN_OR) == ERROR)
	    {
	      ErrorF("winReshape - CombineRgn() failed\n");
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
