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
/* $XFree86: xc/programs/Xserver/hw/xwin/winmultiwindowwindow.c,v 1.3 2003/10/02 13:30:10 eich Exp $ */

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
winCreateWindowsWindow (WindowPtr pWin);

static void
winDestroyWindowsWindow (WindowPtr pWin);

static void
winUpdateWindowsWindow (WindowPtr pWin);

static void
winFindWindow (pointer value, XID id, pointer cdata);

#if 0
static void
winRestackXWindow (WindowPtr pWin, int smode);
#endif


/*
 * Constant defines
 */

#define MOUSE_POLLING_INTERVAL		500


/*
 * Macros
 */

#define SubSend(pWin) \
    ((pWin->eventMask|wOtherEventMasks(pWin)) & SubstructureNotifyMask)

#define StrSend(pWin) \
    ((pWin->eventMask|wOtherEventMasks(pWin)) & StructureNotifyMask)

#define SubStrSend(pWin,pParent) (StrSend(pWin) || SubSend(pParent))


/*
 * CreateWindow - See Porting Layer Definition - p. 37
 */

Bool
winCreateWindowMultiWindow (WindowPtr pWin)
{
  Bool			fResult = TRUE;
  winWindowPriv(pWin);

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winCreateWindowMultiWindow - pWin: %08x\n", pWin);
#endif
  
  /* Call any wrapped CreateWindow function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->CreateWindow)
    fResult = winGetScreenPriv(pWin->drawable.pScreen)->CreateWindow (pWin);
  
  /* Initialize some privates values */
  pWinPriv->hRgn = NULL;
  pWinPriv->hWnd = NULL;
  pWinPriv->pScreenPriv = winGetScreenPriv(pWin->drawable.pScreen);
  pWinPriv->fXKilled = FALSE;
  pWinPriv->fNeedRestore = FALSE;
  
  return fResult;
}


/*
 * DestroyWindow - See Porting Layer Definition - p. 37
 */

Bool
winDestroyWindowMultiWindow (WindowPtr pWin)
{
  Bool			fResult = TRUE;
  winWindowPriv(pWin);

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winDestroyWindowMultiWindow - pWin: %08x\n", pWin);
#endif
  
  /* Call any wrapped DestroyWindow function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->DestroyWindow)
    fResult = winGetScreenPriv(pWin->drawable.pScreen)->DestroyWindow (pWin);
  
  /* Flag that the window has been destroyed */
  pWinPriv->fXKilled = TRUE;
  
  /* Kill the MS Windows window associated with this window */
  winDestroyWindowsWindow (pWin); 

  return fResult;
}


/*
 * PositionWindow - See Porting Layer Definition - p. 37
 */

Bool
winPositionWindowMultiWindow (WindowPtr pWin, int x, int y)
{
  Bool			fResult = TRUE;
  int		        iX, iY, iWidth, iHeight, iBorder;
  winWindowPriv(pWin);
  HWND hWnd = pWinPriv->hWnd;
  RECT rcNew;
  RECT rcOld;
#if CYGMULTIWINDOW_DEBUG
  RECT rcClient;
  RECT *lpRc;
#endif
  DWORD dwExStyle;
  DWORD dwStyle;

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winPositionWindowMultiWindow - pWin: %08x\n", pWin);
#endif
  
  /* Call any wrapped PositionWindow function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->PositionWindow)
    fResult = winGetScreenPriv(pWin->drawable.pScreen)->PositionWindow (pWin, x, y);
  
  /* Bail out if the Windows window handle is bad */
  if (!hWnd)
    return fResult;

  /* Get the Windows window style and extended style */
  dwExStyle = GetWindowLongPtr (hWnd, GWL_EXSTYLE);
  dwStyle = GetWindowLongPtr (hWnd, GWL_STYLE);

  /* Get the width of the X window border */
  iBorder = wBorderWidth (pWin);
  
  /* Get the X and Y location of the X window */
  iX = pWin->drawable.x + GetSystemMetrics (SM_XVIRTUALSCREEN);
  iY = pWin->drawable.y + GetSystemMetrics (SM_YVIRTUALSCREEN);

  /* Get the height and width of the X window */
  iWidth = pWin->drawable.width;
  iHeight = pWin->drawable.height;

  /* Store the origin, height, and width in a rectangle structure */
  SetRect (&rcNew, iX, iY, iX + iWidth, iY + iHeight);

#if CYGMULTIWINDOW_DEBUG
  lpRc = &rcNew;
  ErrorF ("winPositionWindowMultiWindow - (%d ms)drawable (%d, %d)-(%d, %d)\n",
	  GetTickCount (), lpRc->left, lpRc->top, lpRc->right, lpRc->bottom);
#endif

  /*
   * Calculate the required size of the Windows window rectangle,
   * given the size of the Windows window client area.
   */
  AdjustWindowRectEx (&rcNew, dwStyle, FALSE, dwExStyle);

  /* Get a rectangle describing the old Windows window */
  GetWindowRect (hWnd, &rcOld);

#if CYGMULTIWINDOW_DEBUG
  /* Get a rectangle describing the Windows window client area */
  GetClientRect (hWnd, &rcClient);

  lpRc = &rcNew;
  ErrorF ("winPositionWindowMultiWindow - (%d ms)rcNew (%d, %d)-(%d, %d)\n",
	  GetTickCount (), lpRc->left, lpRc->top, lpRc->right, lpRc->bottom);
      
  lpRc = &rcOld;
  ErrorF ("winPositionWindowMultiWindow - (%d ms)rcOld (%d, %d)-(%d, %d)\n",
	  GetTickCount (), lpRc->left, lpRc->top, lpRc->right, lpRc->bottom);
      
  lpRc = &rcClient;
  ErrorF ("(%d ms)rcClient (%d, %d)-(%d, %d)\n",
	  GetTickCount (), lpRc->left, lpRc->top, lpRc->right, lpRc->bottom);
#endif

  /* Check if the old rectangle and new rectangle are the same */
  if (!EqualRect (&rcNew, &rcOld))
    {
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winPositionWindowMultiWindow - Need to move\n");
#endif

      /* Change the position and dimensions of the Windows window */
      MoveWindow (hWnd,
		  rcNew.left, rcNew.top,
		  rcNew.right - rcNew.left, rcNew.bottom - rcNew.top,
		  TRUE);
    }
  else
    {
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winPositionWindowMultiWindow - Not need to move\n");
#endif
    }

  return fResult;
}


/*
 * ChangeWindowAttributes - See Porting Layer Definition - p. 37
 */

Bool
winChangeWindowAttributesMultiWindow (WindowPtr pWin, unsigned long mask)
{
  Bool			fResult = TRUE;

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winChangeWindowAttributesMultiWindow - pWin: %08x\n", pWin);
#endif
  
  /* Call any wrapped ChangeWindowAttributes function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->ChangeWindowAttributes)
    fResult = winGetScreenPriv(pWin->drawable.pScreen)->ChangeWindowAttributes (pWin, mask);
  
  /*
   * NOTE: We do not currently need to do anything here.
   */

  return fResult;
}


/*
 * UnmapWindow - See Porting Layer Definition - p. 37
 * Also referred to as UnrealizeWindow
 */

Bool
winUnmapWindowMultiWindow (WindowPtr pWin)
{
  Bool			fResult = TRUE;
  winWindowPriv(pWin);

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winUnmapWindowMultiWindow - pWin: %08x\n", pWin);
#endif
  
  /* Call any wrapped UnrealizeWindow function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->UnrealizeWindow)
    fResult = winGetScreenPriv(pWin->drawable.pScreen)->UnrealizeWindow (pWin);
  
  /* Flag that the window has been killed */
  pWinPriv->fXKilled = TRUE;
 
  /* Destroy the Windows window associated with this X window */
  winDestroyWindowsWindow (pWin);

  return fResult;
}


/*
 * MapWindow - See Porting Layer Definition - p. 37
 * Also referred to as RealizeWindow
 */

Bool
winMapWindowMultiWindow (WindowPtr pWin)
{
  Bool			fResult = TRUE;
  winWindowPriv(pWin);

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winMapWindowMultiWindow - pWin: %08x\n", pWin);
#endif
  
  /* Call any wrapped RealizeWindow function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->RealizeWindow)
    fResult = winGetScreenPriv(pWin->drawable.pScreen)->RealizeWindow (pWin);
  
  /* Flag that this window has not been destroyed */
  pWinPriv->fXKilled = FALSE;

  /* Refresh/redisplay the Windows window associated with this X window */
  winUpdateWindowsWindow (pWin);

#ifdef SHAPE
  /* Update the Windows window's shape */
  winReshapeMultiWindow (pWin);
  winUpdateRgnMultiWindow (pWin);
#endif

  return fResult;
}


/*
 * ReparentWindow - See Porting Layer Definition - p. 42
 */

void
winReparentWindowMultiWindow (WindowPtr pWin, WindowPtr pPriorParent)
{
#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winReparentMultiWindow - pWin: %08x\n", pWin);
#endif

  /* Call any wrapped ReparentWindow function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->ReparentWindow)
    winGetScreenPriv(pWin->drawable.pScreen)->ReparentWindow (pWin,
							      pPriorParent);
  
  /* Update the Windows window associated with this X window */
  winUpdateWindowsWindow (pWin);
}


/*
 * RestackWindow - Shuffle the z-order of a window
 */

void
winRestackWindowMultiWindow (WindowPtr pWin, WindowPtr pOldNextSib)
{
  WindowPtr		pPrevWin;
  UINT			uFlags;
  HWND			hInsertAfter;
  HWND                  hWnd = NULL;
  winWindowPriv(pWin);

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winRestackMultiWindow - %08x\n", pWin);
#endif
  
  /* Call any wrapped RestackWindow function */
  if (winGetScreenPriv(pWin->drawable.pScreen)->RestackWindow)
    winGetScreenPriv(pWin->drawable.pScreen)->RestackWindow (pWin,
							     pOldNextSib);
  
  if (winGetScreenPriv(pWin->drawable.pScreen)->fRestacking)
    return;

  /* Bail out if no window privates or window handle is invalid */
  if (!pWinPriv || !pWinPriv->hWnd)
    return;

  /* Get a pointer to our previous sibling window */
  pPrevWin = pWin->prevSib;

  /*
   * Look for a sibling window with
   * valid privates and window handle
   */
  while (pPrevWin
	 && !winGetWindowPriv(pPrevWin)
	 && !winGetWindowPriv(pPrevWin)->hWnd)
    pPrevWin = pPrevWin->prevSib;
      
  /* Check if we found a valid sibling */
  if (pPrevWin)
    {
      /* Valid sibling - get handle to insert window after */
      hInsertAfter = winGetWindowPriv(pPrevWin)->hWnd;
      uFlags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE;
  
      hWnd = GetNextWindow (pWinPriv->hWnd, GW_HWNDPREV);

      do
	{
	  if (GetProp (hWnd, WIN_WINDOW_PROP))
	    {
	      if (hWnd == winGetWindowPriv(pPrevWin)->hWnd)
		{
		  uFlags |= SWP_NOZORDER;
		}
	      break;
	    }
	  hWnd = GetNextWindow (hWnd, GW_HWNDPREV);
	}
      while (hWnd);
    }
  else
    {
      /* No valid sibling - make this window the top window */
      hInsertAfter = HWND_TOP;
      uFlags = SWP_NOMOVE | SWP_NOSIZE;
    }
      
  /* Perform the restacking operation in Windows */
  SetWindowPos (pWinPriv->hWnd,
		hInsertAfter,
		0, 0,
		0, 0,
		uFlags);
}


/*
 * winCreateWindowsWindow - Create a Windows window associated with an X window
 */

static void
winCreateWindowsWindow (WindowPtr pWin)
{
  int                   iX, iY;
  int			iWidth;
  int			iHeight;
  int                   iBorder;
  HWND			hWnd;
  WNDCLASS		wc;
  winWindowPriv(pWin);
  HICON                 hIcon;
#define CLASS_NAME_LENGTH 512
  char                  pszClass[CLASS_NAME_LENGTH], pszWindowID[12];
  char                  *res_name, *res_class, *res_role;
  static int		s_iWindowID = 0;

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winCreateWindowsWindow - pWin: %08x\n", pWin);
#endif

  iBorder = wBorderWidth (pWin);
  
  iX = pWin->drawable.x + GetSystemMetrics (SM_XVIRTUALSCREEN);
  iY = pWin->drawable.y + GetSystemMetrics (SM_YVIRTUALSCREEN);
  
  iWidth = pWin->drawable.width;
  iHeight = pWin->drawable.height;

  /* Load default X icon in case it's not ready yet */
  if (!g_hiconX)
    g_hiconX = (HICON)winOverrideDefaultIcon();
  
  if (!g_hiconX)
    g_hiconX = LoadIcon (g_hInstance, MAKEINTRESOURCE(IDI_XWIN));
  
  /* Try and get the icon from WM_HINTS */
  hIcon = winXIconToHICON (pWin);
  
  /* Use default X icon if no icon loaded from WM_HINTS */
  if (!hIcon)
    hIcon = g_hiconX;

  /* Set standard class name prefix so we can identify window easily */
  strncpy (pszClass, WINDOW_CLASS_X, strlen (WINDOW_CLASS_X));

  if (winMultiWindowGetClassHint (pWin, &res_name, &res_class))
    {
      strncat (pszClass, "-", 1);
      strncat (pszClass, res_name, CLASS_NAME_LENGTH - strlen (pszClass));
      strncat (pszClass, "-", 1);
      strncat (pszClass, res_class, CLASS_NAME_LENGTH - strlen (pszClass));
      
      /* Check if a window class is provided by the WM_WINDOW_ROLE property,
       * if not use the WM_CLASS information.
       * For further information see:
       * http://tronche.com/gui/x/icccm/sec-5.html
       */ 
      if (winMultiWindowGetWindowRole (pWin, &res_role) )
	{
	  strcat (pszClass, "-");
	  strcat (pszClass, res_role);
	  free (res_role);
	}

      free (res_name);
      free (res_class);
    }

  /* Add incrementing window ID to make unique class name */
  sprintf (pszWindowID, "-%x", s_iWindowID++);
  strcat (pszClass, pszWindowID);

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winCreateWindowsWindow - Creating class: %s\n", pszClass);
#endif

  /* Setup our window class */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winTopLevelWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = g_hInstance;
  wc.hIcon = hIcon;
  wc.hCursor = 0;
  wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = pszClass;
  RegisterClass (&wc);

  /* Create the window */
  hWnd = CreateWindowExA (WS_EX_TOOLWINDOW,	/* Extended styles */
			  pszClass,		/* Class name */
			  WINDOW_TITLE_X,	/* Window name */
			  WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
			  iX,			/* Horizontal position */
			  iY,			/* Vertical position */
			  iWidth,		/* Right edge */ 
			  iHeight,		/* Bottom edge */
			  (HWND) NULL,		/* No parent or owner window */
			  (HMENU) NULL,		/* No menu */
			  GetModuleHandle (NULL), /* Instance handle */
			  pWin);		/* ScreenPrivates */
  if (hWnd == NULL)
    {
      ErrorF ("winCreateWindowsWindow - CreateWindowExA () failed: %d\n",
	      GetLastError ());
    }
  
  pWinPriv->hWnd = hWnd;

  /* Cause the "Always On Top" to be added in main WNDPROC */
  PostMessage (hWnd, WM_INIT_SYS_MENU, 0, 0);
  
  SetProp (pWinPriv->hWnd, WIN_WID_PROP, (HANDLE) winGetWindowID(pWin));

  /* Flag that this Windows window handles its own activation */
  SetProp (pWinPriv->hWnd, WIN_NEEDMANAGE_PROP, (HANDLE) 0);
}


/*
 * winDestroyWindowsWindow - Destroy a Windows window associated
 * with an X window
 */

static void
winDestroyWindowsWindow (WindowPtr pWin)
{
  MSG			msg;
  winWindowPriv(pWin);
  HICON			hiconClass;
  HMODULE		hInstance;
  int			iReturn;
  char			pszClass[512];
  
#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winDestroyWindowsWindow\n");
#endif

  /* Bail out if the Windows window handle is invalid */
  if (pWinPriv->hWnd == NULL)
    return;

  SetProp (pWinPriv->hWnd, WIN_WINDOW_PROP, 0);

  /* Store the info we need to destroy after this window is gone */
  hInstance = (HINSTANCE) GetClassLong (pWinPriv->hWnd, GCL_HMODULE);
  hiconClass = (HICON) GetClassLong (pWinPriv->hWnd, GCL_HICON);
  iReturn = GetClassName (pWinPriv->hWnd, pszClass, 512);
  
  /* Destroy the Windows window */
  DestroyWindow (pWinPriv->hWnd);

  /* Null our handle to the Window so referencing it will cause an error */
  pWinPriv->hWnd = NULL;
  
  /* Process all messages on our queue */
  while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
    {
      if (g_hDlgDepthChange == 0 || !IsDialogMessage (g_hDlgDepthChange, &msg))
	{
	  DispatchMessage (&msg);
	}
    }

  /* Only if we were able to get the name */
  if (iReturn)
    { 
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winDestroyWindowsWindow - Unregistering %s: ", pszClass);
#endif
      iReturn = UnregisterClass (pszClass, hInstance);
      
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winDestroyWindowsWindow - %d Deleting Icon: ", iReturn);
#endif
      
      /* Only delete if it's not the default */
      if (hiconClass != g_hiconX &&
	  !winIconIsOverride((unsigned long)hiconClass))
	{ 
	  iReturn = DestroyIcon (hiconClass);
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("winDestroyWindowsWindow - %d\n", iReturn);
#endif
	}
    }

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("-winDestroyWindowsWindow\n");
#endif
}


/*
 * winUpdateWindowsWindow - Redisplay/redraw a Windows window
 * associated with an X window
 */

static void
winUpdateWindowsWindow (WindowPtr pWin)
{
  winWindowPriv(pWin);
  HWND			hWnd = pWinPriv->hWnd;

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winUpdateWindowsWindow\n");
#endif

  /* Check if the Windows window's parents have been destroyed */
  if (pWin->parent != NULL
      && pWin->parent->parent == NULL
      && pWin->mapped)
    {
      /* Create the Windows window if it has been destroyed */
      if (hWnd == NULL)
	{
	  winCreateWindowsWindow (pWin);
	  assert (pWinPriv->hWnd != NULL);
	}

      /* Display the window without activating it */
      ShowWindow (pWinPriv->hWnd, SW_SHOWNOACTIVATE);

      /* Send first paint message */
      UpdateWindow (pWinPriv->hWnd);
    }
  else if (hWnd != NULL)
    {
      /* Destroy the Windows window if its parents are destroyed */
      winDestroyWindowsWindow (pWin);
      assert (pWinPriv->hWnd == NULL);
    }

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("-winUpdateWindowsWindow\n");
#endif
}


/*
 * winGetWindowID - 
 */

XID
winGetWindowID (WindowPtr pWin)
{
  WindowIDPairRec	wi = {pWin, 0};
  ClientPtr		c = wClient(pWin);
  
  /* */
  FindClientResourcesByType (c, RT_WINDOW, winFindWindow, &wi);

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winGetWindowID - Window ID: %d\n", wi.id);
#endif

  return wi.id;
}


/*
 * winMoveXWindow - 
 */

void
winMoveXWindow (WindowPtr pWin, int x, int y)
{
  XID *vlist = malloc(sizeof(long)*2);

  (CARD32*)vlist[0] = x;
  (CARD32*)vlist[1] = y;
  ConfigureWindow (pWin, CWX | CWY, vlist, wClient(pWin));
  free(vlist);
}


/*
 * winResizeXWindow - 
 */

void
winResizeXWindow (WindowPtr pWin, int w, int h)
{
  XID *vlist = malloc(sizeof(long)*2);

  (CARD32*)vlist[0] = w;
  (CARD32*)vlist[1] = h;
  ConfigureWindow (pWin, CWWidth | CWHeight, vlist, wClient(pWin));
  free(vlist);
}


/*
 * winFindWindow - 
 */

static void
winFindWindow (pointer value, XID id, pointer cdata)
{
  WindowIDPairPtr	wi = (WindowIDPairPtr)cdata;

  if (value == wi->value)
    {
      wi->id = id;
    }
}


#if 0
/*
 * winRestackXWindow - 
 */

static void
winRestackXWindow (WindowPtr pWin, int smode)
{
  XID *vlist = malloc(sizeof(unsigned long));

  if (vlist == NULL)
    {
      ErrorF ("winRestackXWindow - malloc () failed\n");
      return;
    }

  if (pWin == NULL)
    {
      ErrorF ("winRestackXWindow - NULL window\n");
      free(vlist);
      return;
    }

  *((unsigned long*)vlist) = smode;
  ConfigureWindow (pWin, CWStackMode, vlist, wClient(pWin));

  free(vlist);
}
#endif


/*
 * winReorderWindowsMultiWindow - 
 */

void
winReorderWindowsMultiWindow (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  HWND hwnd = NULL;
  WindowPtr pWin = NULL;
  WindowPtr pWinSib = NULL;

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winOrderWindowsMultiWindow\n");
#endif

  pScreenPriv->fRestacking = TRUE;

  if (pScreenPriv->fWindowOrderChanged)
    {
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winOrderWindowsMultiWindow - Need to restack\n");
#endif
      hwnd = GetTopWindow (NULL);

      while (hwnd)
	{
	  if (GetProp (hwnd, WIN_WINDOW_PROP))
	    {
	      pWinSib = pWin;
	      pWin = GetProp (hwnd, WIN_WINDOW_PROP);
	      
	      if (pWinSib)
		{
		  XID *vlist = malloc (sizeof(long) * 2);
		  
		  if (vlist == NULL)
		    {
		      ErrorF ("winOrderWindowsMultiWindow - malloc () "
			      "failed\n");
		      return;
		    }
		  
		  ((long*)vlist)[0] = winGetWindowID (pWinSib);
		  ((long*)vlist)[1] = Below;

		  ConfigureWindow (pWin, CWSibling | CWStackMode,
				   vlist, wClient(pWin));
		  
		  free (vlist);
		}
	    }
	  hwnd = GetNextWindow (hwnd, GW_HWNDNEXT);
	}
    }

  pScreenPriv->fRestacking = FALSE;
  pScreenPriv->fWindowOrderChanged = FALSE;
}


/*
 * winMinimizeWindow - Minimize in response to WM_CHANGE_STATE
 */

void
winMinimizeWindow (Window id)
{
  WindowPtr		pWin;
  winPrivWinPtr	pWinPriv;
  
  pWin = LookupIDByType (id, RT_WINDOW);
  
  pWinPriv = winGetWindowPriv (pWin);
  
  ShowWindow (pWinPriv->hWnd, SW_MINIMIZE);
}
