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
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/wincursor.c,v 1.6 2003/07/29 21:25:17 dawes Exp $ */

#include "win.h"

miPointerScreenFuncRec g_winPointerCursorFuncs =
{
  winCursorOffScreen,
  winCrossScreen,
  winPointerWarpCursor
};


void
winPointerWarpCursor (ScreenPtr pScreen, int x, int y)
{
  winScreenPriv(pScreen);
  RECT			rcClient;
  static Bool		s_fInitialWarp = TRUE;

  /* Discard first warp call */
  if (s_fInitialWarp)
    {
      /* First warp moves mouse to center of window, just ignore it */

      /* Don't ignore subsequent warps */
      s_fInitialWarp = FALSE;

      ErrorF ("winPointerWarpCursor - Discarding first warp: %d %d\n",
	      x, y);
      
      return;
    }

  /* Only update the Windows cursor position if we are active */
  if (pScreenPriv->hwndScreen == GetForegroundWindow ())
    {
      /* Get the client area coordinates */
      GetClientRect (pScreenPriv->hwndScreen, &rcClient);
      
      /* Translate the client area coords to screen coords */
      MapWindowPoints (pScreenPriv->hwndScreen,
		       HWND_DESKTOP,
		       (LPPOINT)&rcClient,
		       2);
      
      /* 
       * Update the Windows cursor position so that we don't
       * immediately warp back to the current position.
       */
      SetCursorPos (rcClient.left + x, rcClient.top + y);
    }

  /* Call the mi warp procedure to do the actual warping in X. */
  miPointerWarpCursor (pScreen, x, y);
}

Bool
winCursorOffScreen (ScreenPtr *ppScreen, int *x, int *y)
{
  return FALSE;
}

void
winCrossScreen (ScreenPtr pScreen, Bool fEntering)
{
}

