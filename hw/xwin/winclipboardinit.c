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
/* $XFree86: xc/programs/Xserver/hw/xwin/winclipboardinit.c,v 1.2 2003/07/29 21:25:16 dawes Exp $ */

#include "winclipboard.h"


/*
 * Intialize the Clipboard module
 */

Bool
winInitClipboard (pthread_t *ptClipboardProc,
		  pthread_mutex_t *ppmServerStarted,
		  DWORD dwScreen)
{
  ClipboardProcArgPtr		pArg;

  ErrorF ("winInitClipboard ()\n");

  /* Allocate the parameter structure */
  pArg = (ClipboardProcArgPtr) malloc (sizeof (ClipboardProcArgRec));
  if (pArg == NULL)
    {
      ErrorF ("winInitClipboard - malloc for ClipboardProcArgRec failed.\n");
      return FALSE;
    }
  
  /* Setup the argument structure for the thread function */
  pArg->dwScreen = dwScreen;
  pArg->ppmServerStarted = ppmServerStarted;

  /* Spawn a thread for the Clipboard module */
  if (pthread_create (ptClipboardProc, NULL, winClipboardProc, pArg))
    {
      /* Bail if thread creation failed */
      ErrorF ("winInitClipboard - pthread_create failed.\n");
      return FALSE;
    }

  return TRUE;
}


/*
 * Create the Windows window that we use to recieve Windows messages
 */

HWND
winClipboardCreateMessagingWindow ()
{
  WNDCLASS		wc;
  HWND			hwnd;

  /* Setup our window class */
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = winClipboardWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = GetModuleHandle (NULL);
  wc.hIcon = 0;
  wc.hCursor = 0;
  wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = WIN_CLIPBOARD_WINDOW_CLASS;
  RegisterClass (&wc);

  /* Create the window */
  hwnd = CreateWindowExA (0,			/* Extended styles */
			  WIN_CLIPBOARD_WINDOW_CLASS,/* Class name */
			  WIN_CLIPBOARD_WINDOW_TITLE,/* Window name */
			  WS_OVERLAPPED,	/* Not visible anyway */
			  CW_USEDEFAULT,	/* Horizontal position */
			  CW_USEDEFAULT,	/* Vertical position */
			  CW_USEDEFAULT,	/* Right edge */
			  CW_USEDEFAULT,	/* Bottom edge */
			  (HWND) NULL,		/* No parent or owner window */
			  (HMENU) NULL,		/* No menu */
			  GetModuleHandle (NULL),/* Instance handle */
			  NULL);		/* ScreenPrivates */
  assert (hwnd != NULL);

  /* I'm not sure, but we may need to call this to start message processing */
  ShowWindow (hwnd, SW_HIDE);

  /* Similarly, we may need a call to this even though we don't paint */
  UpdateWindow (hwnd);

  return hwnd;
}
