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

#include "win.h"
#include "winprefs.h"

#if 0
/*
 * winMWExtWMReorderWindows
 */

void
winMWExtWMReorderWindows (ScreenPtr pScreen)
{
  winScreenPriv(pScreen);
  HWND hwnd = NULL;
  win32RootlessWindowPtr pRLWin = NULL;
  win32RootlessWindowPtr pRLWinSib = NULL;
  DWORD dwCurrentProcessID = GetCurrentProcessId ();
  DWORD dwWindowProcessID = 0;
  XID vlist[2];

#if CYGMULTIWINDOW_DEBUG && FALSE
  ErrorF ("winMWExtWMReorderWindows\n");
#endif

  pScreenPriv->fRestacking = TRUE;

  if (pScreenPriv->fWindowOrderChanged)
    {
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winMWExtWMReorderWindows - Need to restack\n");
#endif
      hwnd = GetTopWindow (NULL);

      while (hwnd)
	{
	  GetWindowThreadProcessId (hwnd, &dwWindowProcessID);

	  if ((dwWindowProcessID == dwCurrentProcessID)
	      && GetProp (hwnd, WIN_WINDOW_PROP))
	    {
	      pRLWinSib = pRLWin;
	      pRLWin = (win32RootlessWindowPtr)GetProp (hwnd, WIN_WINDOW_PROP);
	      
	      if (pRLWinSib)
		{
		  vlist[0] = pRLWinSib->pFrame->win->drawable.id;
		  vlist[1] = Below;

		  ConfigureWindow (pRLWin->pFrame->win, CWSibling | CWStackMode,
				   vlist, wClient(pRLWin->pFrame->win));
		}
	      else
		{
		  /* 1st window - raise to the top */
		  vlist[0] = Above;

		  ConfigureWindow (pRLWin->pFrame->win, CWStackMode,
				   vlist, wClient(pRLWin->pFrame->win));
		}
	    }
	  hwnd = GetNextWindow (hwnd, GW_HWNDNEXT);
	}
    }

  pScreenPriv->fRestacking = FALSE;
  pScreenPriv->fWindowOrderChanged = FALSE;
}
#endif


/*
 * winMWExtWMMoveXWindow
 */

void
winMWExtWMMoveXWindow (WindowPtr pWin, int x, int y)
{
  XID *vlist = malloc(sizeof(long)*2);

  (CARD32*)vlist[0] = x;
  (CARD32*)vlist[1] = y;
  ConfigureWindow (pWin, CWX | CWY, vlist, wClient(pWin));
  free(vlist);
}


/*
 * winMWExtWMResizeXWindow
 */

void
winMWExtWMResizeXWindow (WindowPtr pWin, int w, int h)
{
  XID *vlist = malloc(sizeof(long)*2);

  (CARD32*)vlist[0] = w;
  (CARD32*)vlist[1] = h;
  ConfigureWindow (pWin, CWWidth | CWHeight, vlist, wClient(pWin));
  free(vlist);
}


/*
 * winMWExtWMMoveResizeXWindow
 */

void
winMWExtWMMoveResizeXWindow (WindowPtr pWin, int x, int y, int w, int h)
{
  XID *vlist = malloc(sizeof(long)*4);

  (CARD32*)vlist[0] = x;
  (CARD32*)vlist[1] = y;
  (CARD32*)vlist[2] = w;
  (CARD32*)vlist[3] = h;

  ConfigureWindow (pWin, CWX | CWY | CWWidth | CWHeight, vlist, wClient(pWin));
  free(vlist);
}


/*
 * winMWExtWMUpdateIcon
 * Change the Windows window icon
 */

void
winMWExtWMUpdateIcon (Window id)
{
  WindowPtr		pWin;
  HICON			hIcon, hiconOld;

  pWin = LookupIDByType (id, RT_WINDOW);
  hIcon = (HICON)winOverrideIcon ((unsigned long)pWin);

  if (!hIcon)
    hIcon = winXIconToHICON (pWin, GetSystemMetrics(SM_CXICON));

  if (hIcon)
    {
      win32RootlessWindowPtr pRLWinPriv
	= (win32RootlessWindowPtr) RootlessFrameForWindow (pWin, FALSE);

      if (pRLWinPriv->hWnd)
	{
	  hiconOld = (HICON) SetClassLong (pRLWinPriv->hWnd,
					   GCL_HICON,
					   (int) hIcon);
	  
          winDestroyIcon(hiconOld);
	}
    }
}
