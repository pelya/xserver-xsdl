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
/* $XFree86: xc/programs/Xserver/hw/xwin/windialogs.c,v 1.1 2003/07/29 21:25:17 dawes Exp $ */

#include "win.h"

extern Bool			g_fCursor;

BOOL CALLBACK
winExitDlgProc (HWND hDialog, UINT message,
		WPARAM wParam, LPARAM lParam);

BOOL CALLBACK
winChangeDepthDlgProc (HWND hDialog, UINT message,
		       WPARAM wParam, LPARAM lParam);


/*
 * Display the Exit dialog box
 */

void
winDisplayExitDialog (winPrivScreenPtr pScreenPriv)
{
  /* Check if dialog already exists */
  if (g_hDlgExit != NULL)
    {
      /* Dialog box already exists, display it */
      ShowWindow (g_hDlgExit, SW_SHOWDEFAULT);

      /* User has lost the dialog.  Show them where it is. */
      SetForegroundWindow (g_hDlgExit);

      return;
    }

  /* Create dialog box */
  g_hDlgExit = CreateDialogParam (g_hInstance,
				  "EXIT_DIALOG",
				  pScreenPriv->hwndScreen,
				  winExitDlgProc,
				  (int) pScreenPriv);

  /* Drop minimize and maximize buttons */
  SetWindowLong (g_hDlgExit, GWL_STYLE,
		 GetWindowLong (g_hDlgExit, GWL_STYLE)
		 & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX));
  SetWindowLong (g_hDlgExit, GWL_EXSTYLE,
		 GetWindowLong (g_hDlgExit, GWL_EXSTYLE) & ~WS_EX_APPWINDOW );
  SetWindowPos (g_hDlgExit, 0, 0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE); 
 
  /* Show the dialog box */
  ShowWindow (g_hDlgExit, SW_SHOW);
  
  /* Needed to get keyboard controls (tab, arrows, enter, esc) to work */
  SetForegroundWindow (g_hDlgExit);
  
  /* Set focus to the Cancel buton */
  PostMessage (g_hDlgExit, WM_NEXTDLGCTL,
	       (int) GetDlgItem (g_hDlgExit, IDCANCEL), TRUE);
}


/*
 * Exit dialog window procedure
 */

BOOL CALLBACK
winExitDlgProc (HWND hDialog, UINT message,
		WPARAM wParam, LPARAM lParam)
{
  static winPrivScreenPtr	s_pScreenPriv = NULL;
  static winScreenInfo		*s_pScreenInfo = NULL;
  static ScreenPtr		s_pScreen = NULL;

  /* Branch on message type */
  switch (message)
    {
    case WM_INITDIALOG:
      /* Store pointers to private structures for future use */
      s_pScreenPriv = (winPrivScreenPtr) lParam;
      s_pScreenInfo = s_pScreenPriv->pScreenInfo;
      s_pScreen = s_pScreenInfo->pScreen;

      /* Set icon to standard app icon */
      PostMessage (hDialog,
		   WM_SETICON,
		   ICON_SMALL,
		   (LPARAM) LoadIcon (g_hInstance, MAKEINTRESOURCE(IDI_XWIN)));
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD (wParam))
	{
	case IDOK:
	  /* Send message to call the GiveUp function */
	  PostMessage (s_pScreenPriv->hwndScreen, WM_GIVEUP, 0, 0);
	  DestroyWindow (g_hDlgExit);
	  g_hDlgExit = NULL;

	  /* Fix to make sure keyboard focus isn't trapped */
	  PostMessage (s_pScreenPriv->hwndScreen, WM_NULL, 0, 0);
	  return TRUE;

	case IDCANCEL:
	  DestroyWindow (g_hDlgExit);
	  g_hDlgExit = NULL;

	  /* Fix to make sure keyboard focus isn't trapped */
	  PostMessage (s_pScreenPriv->hwndScreen, WM_NULL, 0, 0);
	  return TRUE;
	}
      break;

    case WM_MOUSEMOVE:
    case WM_NCMOUSEMOVE:
      /* Show the cursor if it is hidden */
      if (!g_fCursor)
	{
	  g_fCursor = TRUE;
	  ShowCursor (TRUE);
	}
      return TRUE;

    case WM_CLOSE:
      DestroyWindow (g_hDlgExit);
      g_hDlgExit = NULL;

      /* Fix to make sure keyboard focus isn't trapped */
      PostMessage (s_pScreenPriv->hwndScreen, WM_NULL, 0, 0);
      return TRUE;
    }

  return FALSE;
}


/*
 * Display the Depth Change dialog box
 */

void
winDisplayDepthChangeDialog (winPrivScreenPtr pScreenPriv)
{
  /* Check if dialog already exists */
  if (g_hDlgDepthChange != NULL)
    {
      /* Dialog box already exists, display it */
      ShowWindow (g_hDlgDepthChange, SW_SHOWDEFAULT);

      /* User has lost the dialog.  Show them where it is. */
      SetForegroundWindow (g_hDlgDepthChange);

      return;
    }

  /*
   * Display a notification to the user that the visual 
   * will not be displayed until the Windows display depth 
   * is restored to the original value.
   */
  g_hDlgDepthChange = CreateDialogParam (g_hInstance,
					 "DEPTH_CHANGE_BOX",
					 pScreenPriv->hwndScreen,
					 winChangeDepthDlgProc,
					 (int) pScreenPriv);
 
  /* Drop minimize and maximize buttons */
  SetWindowLong (g_hDlgDepthChange, GWL_STYLE,
		 GetWindowLong (g_hDlgDepthChange, GWL_STYLE)
		 & ~(WS_MAXIMIZEBOX | WS_MINIMIZEBOX));
  SetWindowLong (g_hDlgDepthChange, GWL_EXSTYLE,
		 GetWindowLong (g_hDlgDepthChange, GWL_EXSTYLE)
		 & ~WS_EX_APPWINDOW );
  SetWindowPos (g_hDlgDepthChange, 0, 0, 0, 0, 0,
		SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER | SWP_NOSIZE); 

  /* Show the dialog box */
  ShowWindow (g_hDlgDepthChange, SW_SHOW);
  
  ErrorF ("winDisplayDepthChangeDialog - DialogBox returned: %d\n",
	  g_hDlgDepthChange);
  ErrorF ("winDisplayDepthChangeDialog - GetLastError: %d\n", GetLastError ());
	      
  /* Minimize the display window */
  ShowWindow (pScreenPriv->hwndScreen, SW_MINIMIZE);
}


/*
 * Process messages for the dialog that is displayed for
 * disruptive screen depth changes. 
 */

BOOL CALLBACK
winChangeDepthDlgProc (HWND hwndDialog, UINT message,
		       WPARAM wParam, LPARAM lParam)
{
  static winPrivScreenPtr	s_pScreenPriv = NULL;
  static winScreenInfo		*s_pScreenInfo = NULL;
  static ScreenPtr		s_pScreen = NULL;

#if CYGDEBUG
  ErrorF ("winChangeDepthDlgProc\n");
#endif

  /* Branch on message type */
  switch (message)
    {
    case WM_INITDIALOG:
#if CYGDEBUG
      ErrorF ("winChangeDepthDlgProc - WM_INITDIALOG\n");
#endif

      /* Store pointers to private structures for future use */
      s_pScreenPriv = (winPrivScreenPtr) lParam;
      s_pScreenInfo = s_pScreenPriv->pScreenInfo;
      s_pScreen = s_pScreenInfo->pScreen;

#if CYGDEBUG
      ErrorF ("winChangeDepthDlgProc - WM_INITDIALG - s_pScreenPriv: %08x, "
	      "s_pScreenInfo: %08x, s_pScreen: %08x\n",
	      s_pScreenPriv, s_pScreenInfo, s_pScreen);
#endif

#if CYGDEBUG
      ErrorF ("winChangeDepthDlgProc - WM_INITDIALOG - orig bpp: %d, "
	      "last bpp: %d\n",
	      s_pScreenInfo->dwBPP,
	      s_pScreenPriv->dwLastWindowsBitsPixel);
#endif

      /* Set icon to standard app icon */
      PostMessage (hwndDialog,
		   WM_SETICON,
		   ICON_SMALL,
		   (LPARAM) LoadIcon (g_hInstance, MAKEINTRESOURCE(IDI_XWIN)));

      return TRUE;

    case WM_DISPLAYCHANGE:
#if CYGDEBUG
      ErrorF ("winChangeDepthDlgProc - WM_DISPLAYCHANGE - orig bpp: %d, "
	      "last bpp: %d, new bpp: %d\n",
	      s_pScreenInfo->dwBPP,
	      s_pScreenPriv->dwLastWindowsBitsPixel,
	      wParam);
#endif

      /* Dismiss the dialog if the display returns to the original depth */
      if (wParam == s_pScreenInfo->dwBPP)
	{
	  ErrorF ("winChangeDelthDlgProc - wParam == s_pScreenInfo->dwBPP\n");

	  /* Depth has been restored, dismiss dialog */
	  DestroyWindow (g_hDlgDepthChange);
	  g_hDlgDepthChange = NULL;

	  /* Flag that we have a valid screen depth */
	  s_pScreenPriv->fBadDepth = FALSE;
	}
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD (wParam))
	{
	case IDOK:
	case IDCANCEL:
	  ErrorF ("winChangeDepthDlgProc - WM_COMMAND - IDOK or IDCANCEL\n");

	  /* 
	   * User dismissed the dialog, hide it until the
	   * display mode is restored.
	   */
	  ShowWindow (g_hDlgDepthChange, SW_HIDE);
	  return TRUE;
	}
      break;

    case WM_CLOSE:
      ErrorF ("winChangeDepthDlgProc - WM_CLOSE\n");

      /* 
       * User dismissed the dialog, hide it until the
       * display mode is restored.
       */
      ShowWindow (g_hDlgDepthChange, SW_HIDE);
      return TRUE;
    }

  return FALSE;
}
