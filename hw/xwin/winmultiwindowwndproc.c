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
 * Authors:	Kensuke Matsuzaki
 *		Earle F. Philhower, III
 *		Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winmultiwindowwndproc.c,v 1.3 2003/10/08 11:13:03 eich Exp $ */

#include "win.h"
#include "dixevents.h"
#include "winmultiwindowclass.h"
#include "winprefs.h"

/*
 * External global variables
 */

extern Bool		g_fCursor;


/*
 * Global variables
 */

HICON			g_hiconX = NULL;


/*
 * Local globals
 */

static UINT_PTR		g_uipMousePollingTimerID = 0;


/*
 * Constant defines
 */

#define MOUSE_POLLING_INTERVAL		500
#define WIN_MULTIWINDOW_SHAPE		YES



/*
 * ConstrainSize - Taken from TWM sources - Respects hints for sizing
 */
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
static void
ConstrainSize (WinXSizeHints hints, int *widthp, int *heightp)
{
  int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
  int baseWidth, baseHeight;
  int dwidth = *widthp, dheight = *heightp;
  
  if (hints.flags & PMinSize)
    {
      minWidth = hints.min_width;
      minHeight = hints.min_height;
    }
  else if (hints.flags & PBaseSize)
    {
      minWidth = hints.base_width;
      minHeight = hints.base_height;
    }
  else
    minWidth = minHeight = 1;
  
  if (hints.flags & PBaseSize)
    {
      baseWidth = hints.base_width;
      baseHeight = hints.base_height;
    } 
  else if (hints.flags & PMinSize)
    {
      baseWidth = hints.min_width;
      baseHeight = hints.min_height;
    }
  else
    baseWidth = baseHeight = 0;

  if (hints.flags & PMaxSize)
    {
      maxWidth = hints.max_width;
      maxHeight = hints.max_height;
    }
  else
    {
      maxWidth = MAXINT;
      maxHeight = MAXINT;
    }

  if (hints.flags & PResizeInc)
    {
      xinc = hints.width_inc;
      yinc = hints.height_inc;
    }
  else
    xinc = yinc = 1;

  /*
   * First, clamp to min and max values
   */
  if (dwidth < minWidth)
    dwidth = minWidth;
  if (dheight < minHeight)
    dheight = minHeight;

  if (dwidth > maxWidth)
    dwidth = maxWidth;
  if (dheight > maxHeight)
    dheight = maxHeight;

  /*
   * Second, fit to base + N * inc
   */
  dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
  dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;
  
  /*
   * Third, adjust for aspect ratio
   */

  /*
   * The math looks like this:
   *
   * minAspectX    dwidth     maxAspectX
   * ---------- <= ------- <= ----------
   * minAspectY    dheight    maxAspectY
   *
   * If that is multiplied out, then the width and height are
   * invalid in the following situations:
   *
   * minAspectX * dheight > minAspectY * dwidth
   * maxAspectX * dheight < maxAspectY * dwidth
   * 
   */
  
  if (hints.flags & PAspect)
    {
      if (hints.min_aspect.x * dheight > hints.min_aspect.y * dwidth)
        {
	  delta = makemult(hints.min_aspect.x * dheight / hints.min_aspect.y - dwidth, xinc);
	  if (dwidth + delta <= maxWidth)
	    dwidth += delta;
	  else
            {
	      delta = makemult(dheight - dwidth*hints.min_aspect.y/hints.min_aspect.x, yinc);
	      if (dheight - delta >= minHeight)
		dheight -= delta;
            }
        }
      
      if (hints.max_aspect.x * dheight < hints.max_aspect.y * dwidth)
        {
	  delta = makemult(dwidth * hints.max_aspect.y / hints.max_aspect.x - dheight, yinc);
	  if (dheight + delta <= maxHeight)
	    dheight += delta;
	  else
            {
	      delta = makemult(dwidth - hints.max_aspect.x*dheight/hints.max_aspect.y, xinc);
	      if (dwidth - delta >= minWidth)
		dwidth -= delta;
            }
        }
    }
  
  /* Return computed values */
  *widthp = dwidth;
  *heightp = dheight;
}
#undef makemult



/*
 * ValidateSizing - Ensures size request respects hints
 */
static int
ValidateSizing (HWND hwnd, WindowPtr pWin,
		WPARAM wParam, LPARAM lParam)
{
  WinXSizeHints sizeHints;
  RECT *rect;
  int iWidth, iHeight, iTopBorder;
  POINT pt;

  /* Invalid input checking */
  if (pWin==NULL || lParam==0)
    return FALSE;

  /* No size hints, no checking */
  if (!winMultiWindowGetWMNormalHints (pWin, &sizeHints))
    return FALSE;
  
  /* Avoid divide-by-zero */
  if (sizeHints.flags & PResizeInc)
    {
      if (sizeHints.width_inc == 0) sizeHints.width_inc = 1;
      if (sizeHints.height_inc == 0) sizeHints.height_inc = 1;
    }
  
  rect = (RECT*)lParam;
  
  iWidth = rect->right - rect->left;
  iHeight = rect->bottom - rect->top;

  /* Get title bar height, there must be an easier way?! */
  pt.x = pt.y = 0;
  ClientToScreen(hwnd, &pt);
  iTopBorder = pt.y - rect->top;
  
  /* Now remove size of any borders */
  iWidth -= 2 * GetSystemMetrics(SM_CXSIZEFRAME);
  iHeight -= GetSystemMetrics(SM_CYSIZEFRAME) + iTopBorder;

  /* Constrain the size to legal values */
  ConstrainSize (sizeHints, &iWidth, &iHeight);

  /* Add back the borders */
  iWidth += 2 * GetSystemMetrics(SM_CXSIZEFRAME);
  iHeight += GetSystemMetrics(SM_CYSIZEFRAME) + iTopBorder;

  /* Adjust size according to where we're dragging from */
  switch(wParam) {
  case WMSZ_TOP:
  case WMSZ_TOPRIGHT:
  case WMSZ_BOTTOM:
  case WMSZ_BOTTOMRIGHT:
  case WMSZ_RIGHT:
    rect->right = rect->left + iWidth;
    break;
  default:
    rect->left = rect->right - iWidth;
    break;
  }
  switch(wParam) {
  case WMSZ_BOTTOM:
  case WMSZ_BOTTOMRIGHT:
  case WMSZ_BOTTOMLEFT:
  case WMSZ_RIGHT:
  case WMSZ_LEFT:
    rect->bottom = rect->top + iHeight;
    break;
  default:
    rect->top = rect->bottom - iHeight;
    break;
  }
  return TRUE;
}


/*
 * winTopLevelWindowProc - Window procedure for all top-level Windows windows.
 */

LRESULT CALLBACK
winTopLevelWindowProc (HWND hwnd, UINT message, 
		       WPARAM wParam, LPARAM lParam)
{
  POINT			ptMouse;
  HDC			hdcUpdate;
  PAINTSTRUCT		ps;
  WindowPtr		pWin = NULL;
  winPrivWinPtr	        pWinPriv = NULL;
  ScreenPtr		s_pScreen = NULL;
  winPrivScreenPtr	s_pScreenPriv = NULL;
  winScreenInfo		*s_pScreenInfo = NULL;
  HWND			hwndScreen = NULL;
  DrawablePtr		pDraw = NULL;
  int		        iX, iY, iWidth, iHeight, iBorder;
  winWMMessageRec	wmMsg;
  Bool                  fWMMsgInitialized = FALSE;
  static Bool		s_fTracking = FALSE;
  
  /* Check if the Windows window property for our X window pointer is valid */
  if ((pWin = GetProp (hwnd, WIN_WINDOW_PROP)) != NULL)
    {
      /* Our X window pointer is valid */

      /* Get pointers to the drawable and the screen */
      pDraw		= &pWin->drawable;
      s_pScreen		= pWin->drawable.pScreen;

      /* Get a pointer to our window privates */
      pWinPriv		= winGetWindowPriv(pWin);

      /* Get pointers to our screen privates and screen info */
      s_pScreenPriv	= pWinPriv->pScreenPriv;
      s_pScreenInfo	= s_pScreenPriv->pScreenInfo;

      /* Get the handle for our screen-sized window */
      hwndScreen	= s_pScreenPriv->hwndScreen;

      /* */
      wmMsg.msg		= 0;
      wmMsg.hwndWindow	= hwnd;
      wmMsg.iWindow	= (Window)GetProp (hwnd, WIN_WID_PROP);

      wmMsg.iX		= pWinPriv->iX;
      wmMsg.iY		= pWinPriv->iY;
      wmMsg.iWidth	= pWinPriv->iWidth;
      wmMsg.iHeight	= pWinPriv->iHeight;

      fWMMsgInitialized = TRUE;

#if 0
      /*
       * Print some debugging information
       */

      ErrorF ("hWnd %08X\n", hwnd);
      ErrorF ("pWin %08X\n", pWin);
      ErrorF ("pDraw %08X\n", pDraw);
      ErrorF ("\ttype %08X\n", pWin->drawable.type);
      ErrorF ("\tclass %08X\n", pWin->drawable.class);
      ErrorF ("\tdepth %08X\n", pWin->drawable.depth);
      ErrorF ("\tbitsPerPixel %08X\n", pWin->drawable.bitsPerPixel);
      ErrorF ("\tid %08X\n", pWin->drawable.id);
      ErrorF ("\tx %08X\n", pWin->drawable.x);
      ErrorF ("\ty %08X\n", pWin->drawable.y);
      ErrorF ("\twidth %08X\n", pWin->drawable.width);
      ErrorF ("\thenght %08X\n", pWin->drawable.height);
      ErrorF ("\tpScreen %08X\n", pWin->drawable.pScreen);
      ErrorF ("\tserialNumber %08X\n", pWin->drawable.serialNumber);
      ErrorF ("g_iWindowPrivateIndex %d\n", g_iWindowPrivateIndex);
      ErrorF ("pWinPriv %08X\n", pWinPriv);
      ErrorF ("s_pScreenPriv %08X\n", s_pScreenPriv);
      ErrorF ("s_pScreenInfo %08X\n", s_pScreenInfo);
      ErrorF ("hwndScreen %08X\n", hwndScreen);
#endif
    }

  /* Branch on message type */
  switch (message)
    {
    case WM_CREATE:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_CREATE\n");
#endif

      /* */
      SetProp (hwnd,
	       WIN_WINDOW_PROP,
	       (HANDLE)((LPCREATESTRUCT) lParam)->lpCreateParams);
      
      /* */
      SetProp (hwnd,
	       WIN_WID_PROP,
	       (HANDLE)winGetWindowID (((LPCREATESTRUCT) lParam)->lpCreateParams));

      return 0;


    case WM_INIT_SYS_MENU:
      /*
       * Add whatever the setup file wants to for this window
       */
      SetupSysMenu ((unsigned long)hwnd);
      return 0;

    case WM_SYSCOMMAND:
      /*
       * Any window menu items go through here
       */
      HandleCustomWM_COMMAND ((unsigned long)hwnd, LOWORD(wParam));
      break;

    case WM_INITMENU:
      /* Checks/Unchecks any menu items before they are displayed */
      HandleCustomWM_INITMENU ((unsigned long)hwnd, wParam);
      break;

    case WM_PAINT:
      /* Only paint if our window handle is valid */
      if (hwndScreen == NULL)
	break;

      /* BeginPaint gives us an hdc that clips to the invalidated region */
      hdcUpdate = BeginPaint (hwnd, &ps);

      /* Get the position and dimensions of the window */
      iBorder = wBorderWidth (pWin);
      iX = pWin->drawable.x;
      iY = pWin->drawable.y;
      iWidth = pWin->drawable.width;
      iHeight = pWin->drawable.height;

      /* Try to copy from the shadow buffer */
      if (!BitBlt (hdcUpdate,
		   0, 0,
		   iWidth, iHeight,
		   s_pScreenPriv->hdcShadow,
		   iX, iY,
		   SRCCOPY))
	{
	  LPVOID lpMsgBuf;
	  
	  /* Display a fancy error message */
	  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			 FORMAT_MESSAGE_FROM_SYSTEM | 
			 FORMAT_MESSAGE_IGNORE_INSERTS,
			 NULL,
			 GetLastError (),
			 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			 (LPTSTR) &lpMsgBuf,
			 0, NULL);

	  ErrorF ("winTopLevelWindowProc - BitBlt failed: %s\n",
		  (LPSTR)lpMsgBuf);
	  LocalFree (lpMsgBuf);
	}

      /* EndPaint frees the DC */
      EndPaint (hwndScreen, &ps);
      return 0;

    case WM_MOUSEMOVE:
      /* Unpack the client area mouse coordinates */
      ptMouse.x = GET_X_LPARAM(lParam);
      ptMouse.y = GET_Y_LPARAM(lParam);

      /* Translate the client area mouse coordinates to screen coordinates */
      ClientToScreen (hwnd, &ptMouse);

      /* Screen Coords from (-X, -Y) -> Root Window (0, 0) */
      ptMouse.x -= GetSystemMetrics (SM_XVIRTUALSCREEN);
      ptMouse.y -= GetSystemMetrics (SM_YVIRTUALSCREEN);

      /* We can't do anything without privates */
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /* Has the mouse pointer crossed screens? */
      if (s_pScreen != miPointerCurrentScreen ())
	miPointerSetNewScreen (s_pScreenInfo->dwScreen,
			       ptMouse.x - s_pScreenInfo->dwXOffset,
			       ptMouse.y - s_pScreenInfo->dwYOffset);

      /* Are we tracking yet? */
      if (!s_fTracking)
	{
	  TRACKMOUSEEVENT		tme;
	  
	  /* Setup data structure */
	  ZeroMemory (&tme, sizeof (tme));
	  tme.cbSize = sizeof (tme);
	  tme.dwFlags = TME_LEAVE;
	  tme.hwndTrack = hwnd;

	  /* Call the tracking function */
	  if (!(*g_fpTrackMouseEvent) (&tme))
	    ErrorF ("winTopLevelWindowProc - _TrackMouseEvent failed\n");

	  /* Flag that we are tracking now */
	  s_fTracking = TRUE;
	}
      
      /* Hide or show the Windows mouse cursor */
      if (g_fCursor)
	{
	  /* Hide Windows cursor */
	  g_fCursor = FALSE;
	  ShowCursor (FALSE);
	}

      /* Kill the timer used to poll mouse events */
      if (g_uipMousePollingTimerID != 0)
	{
	  KillTimer (s_pScreenPriv->hwndScreen, WIN_POLLING_MOUSE_TIMER_ID);
	  g_uipMousePollingTimerID = 0;
	}

      /* Deliver absolute cursor position to X Server */
      miPointerAbsoluteCursor (ptMouse.x - s_pScreenInfo->dwXOffset,
			       ptMouse.y - s_pScreenInfo->dwYOffset,
			       g_c32LastInputEventTime = GetTickCount ());
      return 0;
      
    case WM_NCMOUSEMOVE:
      /*
       * We break instead of returning 0 since we need to call
       * DefWindowProc to get the mouse cursor changes
       * and min/max/close button highlighting in Windows XP.
       * The Platform SDK says that you should return 0 if you
       * process this message, but it fails to mention that you
       * will give up any default functionality if you do return 0.
       */
      
      /* We can't do anything without privates */
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;

      /* Non-client mouse movement, show Windows cursor */
      if (!g_fCursor)
	{
	  g_fCursor = TRUE;
	  ShowCursor (TRUE);
	}

      /*
       * Timer to poll mouse events.  This is needed to make
       * programs like xeyes follow the mouse properly.
       */
      if (g_uipMousePollingTimerID == 0)
	g_uipMousePollingTimerID = SetTimer (s_pScreenPriv->hwndScreen,
					     WIN_POLLING_MOUSE_TIMER_ID,
					     MOUSE_POLLING_INTERVAL,
					     NULL);
      break;

    case WM_MOUSELEAVE:
      /* Mouse has left our client area */

      /* Flag that we are no longer tracking */
      s_fTracking = FALSE;

      /* Show the mouse cursor, if necessary */
      if (!g_fCursor)
	{
	  g_fCursor = TRUE;
	  ShowCursor (TRUE);
	}

      /*
       * Timer to poll mouse events.  This is needed to make
       * programs like xeyes follow the mouse properly.
       */
      if (g_uipMousePollingTimerID == 0)
	g_uipMousePollingTimerID = SetTimer (s_pScreenPriv->hwndScreen,
					     WIN_POLLING_MOUSE_TIMER_ID,
					     MOUSE_POLLING_INTERVAL,
					     NULL);
      return 0;

    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONDOWN:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonPress, Button1, wParam);
      
    case WM_LBUTTONUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonRelease, Button1, wParam);

    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonPress, Button2, wParam);
      
    case WM_MBUTTONUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonRelease, Button2, wParam);
      
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonPress, Button3, wParam);
      
    case WM_RBUTTONUP:
      if (s_pScreenPriv == NULL || s_pScreenInfo->fIgnoreInput)
	break;
      return winMouseButtonsHandle (s_pScreen, ButtonRelease, Button3, wParam);

    case WM_MOUSEWHEEL:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_MOUSEWHEEL\n");
#endif
      
      /* Pass the message to the root window */
      SendMessage (hwndScreen, message, wParam, lParam);
      return 0;

    case WM_KILLFOCUS:
      /* Pop any pressed keys since we are losing keyboard focus */
      winKeybdReleaseKeys ();
      return 0;

    case WM_SYSDEADCHAR:      
    case WM_DEADCHAR:
      /*
       * NOTE: We do nothing with WM_*CHAR messages,
       * nor does the root window, so we can just toss these messages.
       */
      return 0;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_*KEYDOWN\n");
#endif

      /*
       * Don't pass Alt-F4 key combo to root window,
       * let Windows translate to WM_CLOSE and close this top-level window.
       *
       * NOTE: We purposely don't check the fUseWinKillKey setting because
       * it should only apply to the key handling for the root window,
       * not for top-level window-manager windows.
       *
       * ALSO NOTE: We do pass Ctrl-Alt-Backspace to the root window
       * because that is a key combo that no X app should be expecting to
       * receive, since it has historically been used to shutdown the X server.
       * Passing Ctrl-Alt-Backspace to the root window preserves that
       * behavior, assuming that -unixkill has been passed as a parameter.
       */
      if (wParam == VK_F4 && (GetKeyState (VK_MENU) & 0x8000))
	  break;

      /* Pass the message to the root window */
      SendMessage (hwndScreen, message, wParam, lParam);
      return 0;

    case WM_SYSKEYUP:
    case WM_KEYUP:

#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_*KEYUP\n");
#endif

      /* Pass the message to the root window */
      SendMessage (hwndScreen, message, wParam, lParam);
      return 0;

    case WM_HOTKEY:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_HOTKEY\n");
#endif

      /* Pass the message to the root window */
      SendMessage (hwndScreen, message, wParam, lParam);
      return 0;

    case WM_ACTIVATE:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_ACTIVATE\n");
#endif

      /* Pass the message to the root window */
      SendMessage (hwndScreen, message, wParam, lParam);

      if (s_pScreenPriv != NULL)
	s_pScreenPriv->fWindowOrderChanged = TRUE;

      if (LOWORD(wParam) != WA_INACTIVE)
	{
	  /* Tell our Window Manager thread to activate the window */
	  wmMsg.msg = WM_WM_ACTIVATE;
	  if (fWMMsgInitialized)
	    winSendMessageToWM (s_pScreenPriv->pWMInfo, &wmMsg);
	}
      return 0;

    case WM_ACTIVATEAPP:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_ACTIVATEAPP\n");
#endif
      
      /* Pass the message to the root window */
      SendMessage (hwndScreen, message, wParam, lParam);
      return 0;

    case WM_CLOSE:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_CLOSE\n");
#endif
      /* Branch on if the window was killed in X already */
      if (pWinPriv->fXKilled)
        {
	  /* Window was killed, go ahead and destroy the window */
	  DestroyWindow (hwnd);
	}
      else
	{
	  /* Tell our Window Manager thread to kill the window */
	  wmMsg.msg = WM_WM_KILL;
	  if (fWMMsgInitialized)
	    winSendMessageToWM (s_pScreenPriv->pWMInfo, &wmMsg);
	}
      return 0;

    case WM_DESTROY:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_DESTROY\n");
#endif

      /* Branch on if the window was killed in X already */
      if (pWinPriv && !pWinPriv->fXKilled)
	{
	  ErrorF ("winTopLevelWindowProc - WM_DESTROY - WM_WM_KILL\n");
	  
	  /* Tell our Window Manager thread to kill the window */
	  wmMsg.msg = WM_WM_KILL;
	  if (fWMMsgInitialized)
	    winSendMessageToWM (s_pScreenPriv->pWMInfo, &wmMsg);
	}

#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_DESTROY\n");
#endif
      break;

    case WM_MOVE:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_MOVE - %d ms\n", GetTickCount ());
#endif
      
      /* Bail if Windows window is not actually moving */
      if (pWinPriv->iX == (short) LOWORD(lParam)
	  && pWinPriv->iY == (short) HIWORD(lParam))
	break;

      /* Also bail if we're maximizing, we'll do the whole thing in WM_SIZE */
      {
	WINDOWPLACEMENT windPlace;
	windPlace.length = sizeof (WINDOWPLACEMENT);

	/* Get current window placement */
	GetWindowPlacement (hwnd, &windPlace);

	/* Bail if maximizing */
	if (windPlace.showCmd == SW_MAXIMIZE
	    || windPlace.showCmd == SW_SHOWMAXIMIZED)
	  break;
      } 

      /* Get new position */
      pWinPriv->iX = (short) LOWORD(lParam);
      pWinPriv->iY = (short) HIWORD(lParam);

#if CYGMULTIWINDOW_DEBUG
      ErrorF ("\t(%d, %d)\n", pWinPriv->iX, pWinPriv->iY);
#endif

      winMoveXWindow (pWin,
		      (LOWORD(lParam) - wBorderWidth (pWin)
		       - GetSystemMetrics (SM_XVIRTUALSCREEN)),
		      (HIWORD(lParam) - wBorderWidth (pWin)
		       - GetSystemMetrics (SM_YVIRTUALSCREEN)));
      return 0;

    case WM_SHOWWINDOW:
      /* Bail out if the window is being hidden */
      if (!wParam)
	return 0;

      /* Tell X to map the window */
      MapWindow (pWin, wClient(pWin));

      /* */
      if (!pWin->overrideRedirect)
	{
	  DWORD		dwExStyle;
	  DWORD		dwStyle;
	  RECT		rcNew;
	  int		iDx, iDy;
	      
	  /* Flag that this window needs to be made active when clicked */
	  SetProp (hwnd, WIN_NEEDMANAGE_PROP, (HANDLE) 1);

	  /* Get the standard and extended window style information */
	  dwExStyle = GetWindowLongPtr (hwnd, GWL_EXSTYLE);
	  dwStyle = GetWindowLongPtr (hwnd, GWL_STYLE);

	  /* */
	  if (dwExStyle != WS_EX_APPWINDOW)
	    {
	      /* Setup a rectangle with the X window position and size */
	      SetRect (&rcNew,
		       pWinPriv->iX,
		       pWinPriv->iY,
		       pWinPriv->iX + pWinPriv->iWidth,
		       pWinPriv->iY + pWinPriv->iHeight);

#if 0
	      ErrorF ("winTopLevelWindowProc - (%d, %d)-(%d, %d)\n",
		      rcNew.left, rcNew.top,
		      rcNew.right, rcNew.bottom);
#endif

	      /* */
	      AdjustWindowRectEx (&rcNew,
				  WS_POPUP | WS_SIZEBOX | WS_OVERLAPPEDWINDOW,
				  FALSE,
				  WS_EX_APPWINDOW);

	      /* Calculate position deltas */
	      iDx = pWinPriv->iX - rcNew.left;
	      iDy = pWinPriv->iY - rcNew.top;

	      /* Calculate new rectangle */
	      rcNew.left += iDx;
	      rcNew.right += iDx;
	      rcNew.top += iDy;
	      rcNew.bottom += iDy;

#if 0
	      ErrorF ("winTopLevelWindowProc - (%d, %d)-(%d, %d)\n",
		      rcNew.left, rcNew.top,
		      rcNew.right, rcNew.bottom);
#endif

	      /* Set the window extended style flags */
	      SetWindowLongPtr (hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);

	      /* Set the window standard style flags */
	      SetWindowLongPtr (hwnd, GWL_STYLE,
				WS_POPUP | WS_SIZEBOX | WS_OVERLAPPEDWINDOW);

	      /* Position the Windows window */
	      SetWindowPos (hwnd, HWND_TOP,
			    rcNew.left, rcNew.top,
			    rcNew.right - rcNew.left, rcNew.bottom - rcNew.top,
			    SWP_NOMOVE | SWP_FRAMECHANGED
			    | SWP_SHOWWINDOW | SWP_NOACTIVATE);

	      /* Bring the Windows window to the foreground */
	      SetForegroundWindow (hwnd);
	    }
	}
	  
      /* Setup the Window Manager message */
      wmMsg.msg = WM_WM_MAP;
      wmMsg.iWidth = pWinPriv->iWidth;
      wmMsg.iHeight = pWinPriv->iHeight;

      /* Tell our Window Manager thread to map the window */
      if (fWMMsgInitialized)
	winSendMessageToWM (s_pScreenPriv->pWMInfo, &wmMsg);

      if (s_pScreenPriv != NULL)
	s_pScreenPriv->fWindowOrderChanged = TRUE;
      return 0;

    case WM_SIZING:
      /* Need to legalize the size according to WM_NORMAL_HINTS */
      /* for applications like xterm */
      return ValidateSizing (hwnd, pWin, wParam, lParam);

    case WM_WINDOWPOSCHANGED:
      {
	LPWINDOWPOS pwindPos = (LPWINDOWPOS) lParam;

	/* Bail if window z order was not changed */
	if (pwindPos->flags & SWP_NOZORDER)
	  break;

#if CYGMULTIWINDOW_DEBUG
	ErrorF ("winTopLevelWindowProc - hwndInsertAfter: %p\n",
		pwindPos->hwndInsertAfter);
#endif
	
	/* Pass the message to the root window */
	SendMessage (hwndScreen, message, wParam, lParam);
	
	if (s_pScreenPriv != NULL)
	  s_pScreenPriv->fWindowOrderChanged = TRUE;
      }
      return 0;

    case WM_SIZE:
      /* see dix/window.c */

#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_SIZE - %d ms\n", GetTickCount ());
#endif

      /* Branch on type of resizing occurring */
      switch (wParam)
	{
	case SIZE_MINIMIZED:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\tSIZE_MINIMIZED\n");
#endif
	  if (s_pScreenPriv != NULL)
	    s_pScreenPriv->fWindowOrderChanged = TRUE;
	  break;

	case SIZE_RESTORED:
	case SIZE_MAXIMIZED:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("SIZE_RESTORED || SIZE_MAXIMIZED\n");
#endif
	  /* Break out if nothing to do */
	  if (pWinPriv->iWidth == (short) LOWORD(lParam)
	      && pWinPriv->iHeight == (short) HIWORD(lParam))
	    break;
	  
	  /* Get the dimensions of the resized Windows window  */
	  pWinPriv->iWidth = (short) LOWORD(lParam);
	  pWinPriv->iHeight = (short) HIWORD(lParam);

#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\t(%d, %d)\n", pWinPriv->iWidth, pWinPriv->iHeight);
#endif

	  /*
	   * If we're maximizing the window has been moved to upper left
	   * of current screen.  Now it is safe for X to know about this.
	   */
	  if (wParam == SIZE_MAXIMIZED)
	    {
	      POINT		ptHome;

	      /* Flag that we are being maximized and store info for restore */
	      pWinPriv->fNeedRestore = TRUE;
	      pWinPriv->ptRestore.x = pWinPriv->iX;
	      pWinPriv->ptRestore.y = pWinPriv->iY;
	     
	      /* Get screen location of window root */
	      ptHome.x = 0;
	      ptHome.y = 0;
	      ClientToScreen (hwnd, &ptHome);

	      /* Map from screen (-X,-Y) to (0,0) root coords */
	      winMoveXWindow (pWin,
			      ptHome.x - wBorderWidth (pWin)
			      - GetSystemMetrics (SM_XVIRTUALSCREEN),
			      ptHome.y - wBorderWidth (pWin)
			      - GetSystemMetrics (SM_YVIRTUALSCREEN));
	    }
	  else if (wParam == SIZE_RESTORED && pWinPriv->fNeedRestore)
	    {
	      /* If need restore and !maximized then move to cached position */
	      WINDOWPLACEMENT windPlace;

	      windPlace.length = sizeof (WINDOWPLACEMENT);

	      GetWindowPlacement (hwnd, &windPlace);

	      if (windPlace.showCmd != SW_MAXIMIZE
		  && windPlace.showCmd != SW_SHOWMAXIMIZED)
		{
		  pWinPriv->fNeedRestore = FALSE;
		  winMoveXWindow (pWin,
				  pWinPriv->ptRestore.x  - wBorderWidth (pWin)
				  - GetSystemMetrics (SM_XVIRTUALSCREEN),
				  pWinPriv->ptRestore.y - wBorderWidth (pWin)
				  - GetSystemMetrics (SM_YVIRTUALSCREEN));
		}
	    }

	  /* Perform the resize and notify the X client */
	  winResizeXWindow (pWin,
			    (short) LOWORD(lParam),
			    (short) HIWORD(lParam));
	  break;

	default:
	  break;
	}
      return 0;

    case WM_MOUSEACTIVATE:
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winTopLevelWindowProc - WM_MOUSEACTIVATE\n");
#endif

      /* Check if this window needs to be made active when clicked */
      if (!GetProp (pWinPriv->hWnd, WIN_NEEDMANAGE_PROP))
	{
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("winTopLevelWindowProc - WM_MOUSEACTIVATE - "
		  "MA_NOACTIVATE\n");
#endif

	  /* */
	  return MA_NOACTIVATE;
	}
      break;

    default:
      break;
    }

  return DefWindowProc (hwnd, message, wParam, lParam);
}
