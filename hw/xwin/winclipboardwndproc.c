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
/* $XFree86: xc/programs/Xserver/hw/xwin/winclipboardwndproc.c,v 1.1 2003/02/12 15:01:38 alanh Exp $ */

#include "winclipboard.h"


/*
 * Process a given Windows message
 */

LRESULT CALLBACK
winClipboardWindowProc (HWND hwnd, UINT message, 
			WPARAM wParam, LPARAM lParam)
{
  /* Branch on message type */
  switch (message)
    {
    case WM_DESTROY:
      PostQuitMessage (0);
      return 0;

    case WM_CREATE:
#if 0
      ErrorF ("WindowProc - WM_CREATE\n");
#endif
      return 0;
    }

  /* Let Windows perform default processing for unhandled messages */
  return DefWindowProc (hwnd, message, wParam, lParam);
}


/*
 * Process any pending Windows messages
 */

BOOL
winClipboardFlushWindowsMessageQueue (HWND hwnd)
{
  MSG			msg;

  /* Flush the messaging window queue */
  /* NOTE: Do not pass the hwnd of our messaging window to PeekMessage,
   * as this will filter out many non-window-specific messages that
   * are sent to our thread, such as WM_QUIT.
   */
  while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
    {
      /* Dispatch the message if not WM_QUIT */
      if (msg.message == WM_QUIT)
	return FALSE;
      else
	DispatchMessage (&msg);
    }
  
  return TRUE;
}
