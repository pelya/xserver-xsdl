/*
 *Copyright (C) 1994-2001 The XFree86 Project, Inc. All Rights Reserved.
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
/* $XFree86: xc/programs/Xserver/hw/xwin/wincreatewnd.c,v 1.5 2003/02/12 15:01:38 alanh Exp $ */

#include "win.h"
#include "shellapi.h"


/*
 * Local function prototypes
 */

static Bool
winGetWorkArea (RECT *prcWorkArea, winScreenInfo *pScreenInfo);

static Bool
winAdjustForAutoHide (RECT *prcWorkArea);


/*
 * Create a full screen window
 */

Bool
winCreateBoundingWindowFullScreen (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  int			iWidth = pScreenInfo->dwWidth;
  int			iHeight = pScreenInfo->dwHeight;
  HWND			*phwnd = &pScreenPriv->hwndScreen;
  WNDCLASS		wc;

#if CYGDEBUG
  ErrorF ("winCreateBoundingWindowFullScreen\n");
#endif

  /* Setup our window class */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = g_hInstance;
  wc.hIcon = LoadIcon (g_hInstance, MAKEINTRESOURCE(IDI_XWIN));
  wc.hCursor = 0;
  wc.hbrBackground = 0;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WINDOW_CLASS;
  RegisterClass (&wc);

  /* Create the window */
  *phwnd = CreateWindowExA (WS_EX_TOPMOST,	/* Extended styles */
			    WINDOW_CLASS,	/* Class name */
			    WINDOW_TITLE,	/* Window name */
			    WS_POPUP,
			    0,			/* Horizontal position */
			    0,			/* Vertical position */
			    iWidth,		/* Right edge */ 
			    iHeight,		/* Bottom edge */
			    (HWND) NULL,	/* No parent or owner window */
			    (HMENU) NULL,	/* No menu */
			    GetModuleHandle (NULL),/* Instance handle */
			    pScreenPriv);	/* ScreenPrivates */

  /* Branch on the server engine */
  switch (pScreenInfo->dwEngine)
    {
    case WIN_SERVER_SHADOW_GDI:
      /* Show the window */
      ShowWindow (*phwnd, SW_SHOWMAXIMIZED);
      break;

    default:
      /* Hide the window */
      ShowWindow (*phwnd, SW_SHOWNORMAL);
      break;
    }
  
  /* Send first paint message */
  UpdateWindow (*phwnd);

  /* Attempt to bring our window to the top of the display */
  BringWindowToTop (*phwnd);

  return TRUE;
}


/*
 * Create our primary Windows display window
 */

Bool
winCreateBoundingWindowWindowed (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  winScreenInfo		*pScreenInfo = pScreenPriv->pScreenInfo;
  int			iWidth = pScreenInfo->dwUserWidth;
  int			iHeight = pScreenInfo->dwUserHeight;
  HWND			*phwnd = &pScreenPriv->hwndScreen;
  WNDCLASS		wc;
  RECT			rcClient, rcWorkArea;
  DWORD			dwWindowStyle;
  
  ErrorF ("winCreateBoundingWindowWindowed - User w: %d h: %d\n",
	  pScreenInfo->dwUserWidth, pScreenInfo->dwUserHeight);
  ErrorF ("winCreateBoundingWindowWindowed - Current w: %d h: %d\n",
	  pScreenInfo->dwWidth, pScreenInfo->dwHeight);
  
  /* Set the common window style flags */
  dwWindowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX;
  
  /* Decorated or undecorated window */
  if (pScreenInfo->fDecoration
      && !pScreenInfo->fRootless
      && !pScreenInfo->fMultiWindow)
    {
      dwWindowStyle |= WS_CAPTION;
      if (pScreenInfo->fScrollbars)
	dwWindowStyle |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    }
  else
    dwWindowStyle |= WS_POPUP;

  /* Setup our window class */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = g_hInstance;
  wc.hIcon = LoadIcon (g_hInstance, MAKEINTRESOURCE(IDI_XWIN));
  wc.hCursor = 0;
  wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WINDOW_CLASS;
  RegisterClass (&wc);

  /* Get size of work area */
  winGetWorkArea (&rcWorkArea, pScreenInfo);

  /* Adjust for auto-hide taskbars */
  winAdjustForAutoHide (&rcWorkArea);

  /* Did the user specify a height and width? */
  if (pScreenInfo->fUserGaveHeightAndWidth)
    {
      /* User gave a desired height and width, try to accomodate */
#if CYGDEBUG
      ErrorF ("winCreateBoundingWindowWindowed - User gave height "
	      "and width\n");
#endif
      
      /* Adjust the window width and height for borders and title bar */
      if (pScreenInfo->fDecoration
	  && !pScreenInfo->fRootless
	  && !pScreenInfo->fMultiWindow)
	{
#if CYGDEBUG
	  ErrorF ("winCreateBoundingWindowWindowed - Window has decoration\n");
#endif
	  /* Are we using scrollbars? */
	  if (pScreenInfo->fScrollbars)
	    {
#if CYGDEBUG
	      ErrorF ("winCreateBoundingWindowWindowed - Window has "
		      "scrollbars\n");
#endif

	      iWidth += 2 * GetSystemMetrics (SM_CXSIZEFRAME);
	      iHeight += 2 * GetSystemMetrics (SM_CYSIZEFRAME) 
		+ GetSystemMetrics (SM_CYCAPTION);
	    }
	  else
	    {
#if CYGDEBUG
	      ErrorF ("winCreateBoundingWindowWindowed - Window does not have "
		      "scrollbars\n");
#endif

	      iWidth += 2 * GetSystemMetrics (SM_CXFIXEDFRAME);
	      iHeight += 2 * GetSystemMetrics (SM_CYFIXEDFRAME) 
		+ GetSystemMetrics (SM_CYCAPTION);
	    }
	}
      else
	{
	  /*
	   * User gave a width and height but also said no decoration.
	   * In this case we have to ignore the requested width and height
	   * and instead use the largest possible window that we can.
	   */
	  if (pScreenInfo->fMultipleMonitors)
	    {
	      iWidth = GetSystemMetrics (SM_CXVIRTUALSCREEN);
	      iHeight = GetSystemMetrics (SM_CYVIRTUALSCREEN);
	    }
	  else
	    {
	      iWidth = GetSystemMetrics (SM_CXSCREEN);
	      iHeight = GetSystemMetrics (SM_CYSCREEN);
	    }
	}
    }
  else
    {
      /* By default, we are creating a window that is as large as possible */
#if CYGDEBUG
      ErrorF ("winCreateBoundingWindowWindowed - User did not give "
	      "height and width\n");
#endif
      /* Defaults are wrong if we have multiple monitors */
      if (pScreenInfo->fMultipleMonitors)
	{
	  iWidth = GetSystemMetrics (SM_CXVIRTUALSCREEN);
	  iHeight = GetSystemMetrics (SM_CYVIRTUALSCREEN);
	}
    }

  /* Clean up the scrollbars flag, if necessary */
  if ((!pScreenInfo->fDecoration
       || pScreenInfo->fRootless
       || pScreenInfo->fMultiWindow)
      && pScreenInfo->fScrollbars)
    {
      /* We cannot have scrollbars if we do not have a window border */
      pScreenInfo->fScrollbars = FALSE;
    }
  
  /* Trim window width to fit work area */
  if (iWidth > (rcWorkArea.right - rcWorkArea.left))
    iWidth = rcWorkArea.right - rcWorkArea.left;
  
  /* Trim window height to fit work area */
  if (iHeight >= (rcWorkArea.bottom - rcWorkArea.top))
    iHeight = rcWorkArea.bottom - rcWorkArea.top;
  
#if CYGDEBUG
  ErrorF ("winCreateBoundingWindowWindowed - Adjusted width: %d "\
	  "height: %d\n",
	  iWidth, iHeight);
#endif

  /* Create the window */
  *phwnd = CreateWindowExA (0,			/* Extended styles */
			    WINDOW_CLASS,	/* Class name */
			    WINDOW_TITLE,	/* Window name */
			    dwWindowStyle,
			    rcWorkArea.left,	/* Horizontal position */
			    rcWorkArea.top,	/* Vertical position */
			    iWidth,		/* Right edge */
			    iHeight,		/* Bottom edge */
			    (HWND) NULL,	/* No parent or owner window */
			    (HMENU) NULL,	/* No menu */
			    GetModuleHandle (NULL),/* Instance handle */
			    pScreenPriv);	/* ScreenPrivates */
  if (*phwnd == NULL)
    {
      ErrorF ("winCreateBoundingWindowWindowed - CreateWindowEx () failed\n");
      return FALSE;
    }

#if CYGDEBUG
  ErrorF ("winCreateBoundingWindowWindowed - CreateWindowEx () returned\n");
#endif

  /* Get the client area coordinates */
  if (!GetClientRect (*phwnd, &rcClient))
    {
      ErrorF ("winCreateBoundingWindowWindowed - GetClientRect () "
	      "failed\n");
      return FALSE;
    }

  ErrorF ("winCreateBoundingWindowWindowed - WindowClient "
	  "w %ld h %ld r %ld l %ld b %ld t %ld\n",
	  rcClient.right - rcClient.left,
	  rcClient.bottom - rcClient.top,
	  rcClient.right, rcClient.left,
	  rcClient.bottom, rcClient.top);
  
  /* We adjust the visual size if the user did not specify it */
  if (!(pScreenInfo->fScrollbars && pScreenInfo->fUserGaveHeightAndWidth))
    {
      /*
       * User did not give a height and width with scrollbars enabled,
       * so we will resize the underlying visual to be as large as
       * the initial view port (page size).  This way scrollbars will
       * not appear until the user shrinks the window, if they ever do.
       *
       * NOTE: We have to store the viewport size here because
       * the user may have an autohide taskbar, which would
       * cause the viewport size to be one less in one dimension
       * than the viewport size that we calculated by subtracting
       * the size of the borders and caption.
       */
      pScreenInfo->dwWidth = rcClient.right - rcClient.left;
      pScreenInfo->dwHeight = rcClient.bottom - rcClient.top;
    }

#if 0
  /*
   * NOTE: For the uninitiated, the page size is the number of pixels
   * that we can display in the x or y direction at a time and the
   * range is the total number of pixels in the x or y direction that we
   * have available to display.  In other words, the page size is the
   * size of the window area minus the space the caption, borders, and
   * scrollbars (if any) occupy, and the range is the size of the
   * underlying X visual.  Notice that, contrary to what some of the
   * MSDN Library arcticles lead you to believe, the windows
   * ``client area'' size does not include the scrollbars.  In other words,
   * the whole client area size that is reported to you is drawable by
   * you; you do not have to subtract the size of the scrollbars from
   * the client area size, and if you did it would result in the size
   * of the scrollbars being double counted.
   */

  /* Setup scrollbar page and range, if scrollbars are enabled */
  if (pScreenInfo->fScrollbars)
    {
      SCROLLINFO		si;
      
      /* Initialize the scrollbar info structure */
      si.cbSize = sizeof (si);
      si.fMask = SIF_RANGE | SIF_PAGE;
      si.nMin = 0;
      
      /* Setup the width range and page size */
      si.nMax = pScreenInfo->dwWidth - 1;
      si.nPage = rcClient.right - rcClient.left;
      ErrorF ("winCreateBoundingWindowWindowed - HORZ nMax: %d nPage :%d\n",
	      si.nMax, si.nPage);
      SetScrollInfo (*phwnd, SB_HORZ, &si, TRUE);
      
      /* Setup the height range and page size */
      si.nMax = pScreenInfo->dwHeight - 1;
      si.nPage = rcClient.bottom - rcClient.top;
      ErrorF ("winCreateBoundingWindowWindowed - VERT nMax: %d nPage :%d\n",
	      si.nMax, si.nPage);
      SetScrollInfo (*phwnd, SB_VERT, &si, TRUE);
    }
#endif

  /* Show the window */
  if (pScreenInfo->fMultiWindow)
    ShowWindow (*phwnd, SW_SHOWMINNOACTIVE);
  else
    ShowWindow (*phwnd, SW_SHOWNORMAL);
  if (!UpdateWindow (*phwnd))
    {
      ErrorF ("winCreateBoundingWindowWindowed - UpdateWindow () failed\n");
      return FALSE;
    }
  
  /* Attempt to bring our window to the top of the display */
  if (!BringWindowToTop (*phwnd))
    {
      ErrorF ("winCreateBoundingWindowWindowed - BringWindowToTop () "
	      "failed\n");
      return FALSE;
    }

  /* Paint window background blue */
  if (pScreenInfo->dwEngine == WIN_SERVER_NATIVE_GDI)
    winPaintBackground (*phwnd, RGB (0x00, 0x00, 0xFF));

  ErrorF ("winCreateBoundingWindowWindowed -  Returning\n");

  return TRUE;
}


/*
 * Find the work area of all attached monitors
 */

static Bool
winGetWorkArea (RECT *prcWorkArea, winScreenInfo *pScreenInfo)
{
  int			iPrimaryWidth, iPrimaryHeight;
  int			iWidth, iHeight;
  int			iLeft, iTop;
  int			iPrimaryNonWorkAreaWidth, iPrimaryNonWorkAreaHeight;

  /* SPI_GETWORKAREA only gets the work area of the primary screen. */
  SystemParametersInfo (SPI_GETWORKAREA, 0, prcWorkArea, 0);

  /* Bail out here if we aren't using multiple monitors */
  if (!pScreenInfo->fMultipleMonitors)
    return TRUE;
  
  ErrorF ("winGetWorkArea - Original WorkArea: %d %d %d %d\n",
	  prcWorkArea->top, prcWorkArea->left,
	  prcWorkArea->bottom, prcWorkArea->right);

  /* Get size of full virtual screen */
  iWidth = GetSystemMetrics (SM_CXVIRTUALSCREEN);
  iHeight = GetSystemMetrics (SM_CYVIRTUALSCREEN);

  ErrorF ("winGetWorkArea - Virtual screen is %d x %d\n", iWidth, iHeight);

  /* Get origin of full virtual screen */
  iLeft = GetSystemMetrics (SM_XVIRTUALSCREEN);
  iTop = GetSystemMetrics (SM_YVIRTUALSCREEN);

  ErrorF ("winGetWorkArea - Virtual screen origin is %d, %d\n", iLeft, iTop);
  
  /* Get size of primary screen */
  iPrimaryWidth = GetSystemMetrics (SM_CXSCREEN);
  iPrimaryHeight = GetSystemMetrics (SM_CYSCREEN);

  ErrorF ("winGetWorkArea - Primary screen is %d x %d\n",
	 iPrimaryWidth, iPrimaryHeight);
  
  /* Work out how much of the primary screen we aren't using */
  iPrimaryNonWorkAreaWidth = iPrimaryWidth - (prcWorkArea->right -
					      prcWorkArea->left);
  iPrimaryNonWorkAreaHeight = iPrimaryHeight - (prcWorkArea->bottom
						- prcWorkArea->top);
  
  /* Update the rectangle to include all monitors */
  if (iLeft < 0) 
    {
      prcWorkArea->left = iLeft;
    }
  if (iTop < 0) 
    {
      prcWorkArea->top = iTop;
    }
  prcWorkArea->right = prcWorkArea->left + iWidth -
    iPrimaryNonWorkAreaWidth;
  prcWorkArea->bottom = prcWorkArea->top + iHeight -
    iPrimaryNonWorkAreaHeight;
  
  ErrorF ("winGetWorkArea - Adjusted WorkArea for multiple "
	  "monitors: %d %d %d %d\n",
	  prcWorkArea->top, prcWorkArea->left,
	  prcWorkArea->bottom, prcWorkArea->right);
  
  return TRUE;
}


/*
 * Adjust the client area so that any auto-hide toolbars
 * will work correctly.
 */

static Bool
winAdjustForAutoHide (RECT *prcWorkArea)
{
  APPBARDATA		abd;
  HWND			hwndAutoHide;

  ErrorF ("winAdjustForAutoHide - Original WorkArea: %d %d %d %d\n",
	  prcWorkArea->top, prcWorkArea->left,
	  prcWorkArea->bottom, prcWorkArea->right);

  /* Find out if the Windows taskbar is set to auto-hide */
  ZeroMemory (&abd, sizeof (abd));
  abd.cbSize = sizeof (abd);
  if (SHAppBarMessage (ABM_GETSTATE, &abd) & ABS_AUTOHIDE)
    ErrorF ("winAdjustForAutoHide - Taskbar is auto hide\n");

  /* Look for a TOP auto-hide taskbar */
  abd.uEdge = ABE_TOP;
  hwndAutoHide = (HWND) SHAppBarMessage (ABM_GETAUTOHIDEBAR, &abd);
  if (hwndAutoHide != NULL)
    {
      ErrorF ("winAdjustForAutoHide - Found TOP auto-hide taskbar\n");
      prcWorkArea->top += 1;
    }

  /* Look for a LEFT auto-hide taskbar */
  abd.uEdge = ABE_LEFT;
  hwndAutoHide = (HWND) SHAppBarMessage (ABM_GETAUTOHIDEBAR, &abd);
  if (hwndAutoHide != NULL)
    {
      ErrorF ("winAdjustForAutoHide - Found LEFT auto-hide taskbar\n");
      prcWorkArea->left += 1;
    }

  /* Look for a BOTTOM auto-hide taskbar */
  abd.uEdge = ABE_BOTTOM;
  hwndAutoHide = (HWND) SHAppBarMessage (ABM_GETAUTOHIDEBAR, &abd);
  if (hwndAutoHide != NULL)
    {
      ErrorF ("winAdjustForAutoHide - Found BOTTOM auto-hide taskbar\n");
      prcWorkArea->bottom -= 1;
    }

  /* Look for a RIGHT auto-hide taskbar */
  abd.uEdge = ABE_RIGHT;
  hwndAutoHide = (HWND) SHAppBarMessage (ABM_GETAUTOHIDEBAR, &abd);
  if (hwndAutoHide != NULL)
    {
      ErrorF ("winAdjustForAutoHide - Found RIGHT auto-hide taskbar\n");
      prcWorkArea->right -= 1;
    }

  ErrorF ("winAdjustForAutoHide - Adjusted WorkArea: %d %d %d %d\n",
	  prcWorkArea->top, prcWorkArea->left,
	  prcWorkArea->bottom, prcWorkArea->right);
  
#if 0
  /* Obtain the task bar window dimensions */
  abd.hWnd = hwndAutoHide;
  hwndAutoHide = (HWND) SHAppBarMessage (ABM_GETTASKBARPOS, &abd);
  ErrorF ("hwndAutoHide %08x abd.hWnd %08x %d %d %d %d\n",
	  hwndAutoHide, abd.hWnd,
	  abd.rc.top, abd.rc.left, abd.rc.bottom, abd.rc.right);
#endif

  return TRUE;
}
