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
 * Authors:	Harold Hunt
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winclipboard.h,v 1.1 2003/02/12 15:01:38 alanh Exp $ */


#ifndef _WINCLIPBOARD_H_
#define _WINCLIPBOARD_H_

/* Standard library headers */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <setjmp.h>
#include <pthread.h>

/* X headers */
#include "X.h"
#include "Xatom.h"
/* NOTE: For some unknown reason, including Xproto.h solves
 * tons of problems with including windows.h.  Unknowns reasons
 * are usually bad, so someone should investigate this.
 */
#include "Xproto.h"
#include "Xutil.h"
#include "Xlocale.h"

/* Fixups to prevent collisions between Windows and X headers */
#define ATOM			DWORD

/* Windows headers */
#include <windows.h>


/* Clipboard module constants */
#define WIN_CLIPBOARD_WINDOW_CLASS		"xwinclip"
#define WIN_CLIPBOARD_WINDOW_TITLE		"xwinclip"
#define WIN_MSG_QUEUE_FNAME			"/dev/windows"
#define WIN_CONNECT_RETRIES			3
#define WIN_CONNECT_DELAY			4
#define WIN_JMP_OKAY				0
#define WIN_JMP_ERROR_IO			2

/*
 * Argument structure for Clipboard module main thread
 */

typedef struct _ClipboardProcArgRec {
  DWORD			dwScreen;
  pthread_mutex_t	*ppmServerStarted;
} ClipboardProcArgRec, *ClipboardProcArgPtr;


/*
 * References to external symbols
 */

extern char *display;
extern void ErrorF (const char* /*f*/, ...);


/*
 * winclipboardinit.c
 */

Bool
winInitClipboard (pthread_t *ptClipboardProc,
		  pthread_mutex_t *ppmServerStarted,
		  DWORD dwScreen);

HWND
winClipboardCreateMessagingWindow ();


/*
 * winclipboardtextconv.c
 */

void
winClipboardDOStoUNIX (char *pszData, int iLength);

void
winClipboardUNIXtoDOS (unsigned char **ppszData, int iLength);


/*
 * winclipboardthread.c
 */

void *
winClipboardProc (void *pArg);


/*
 * winclipboardunicode.c
 */

Bool
winClipboardDetectUnicodeSupport ();


/*
 * winclipboardwndproc.c
 */

BOOL
winClipboardFlushWindowsMessageQueue (HWND hwnd);

LRESULT CALLBACK
winClipboardWindowProc (HWND hwnd, UINT message, 
			WPARAM wParam, LPARAM lParam);


/*
 * winclipboardxevents.c
 */

Bool
winClipboardFlushXEvents (HWND hwnd,
			  Atom atomClipboard,
			  Atom atomLocalProperty,
			  Atom atomUTF8String,
			  Atom atomCompoundText,
			  Atom atomTargets,
			  Atom atomDeleteWindow,
			  int iWindow,
			  Display *pDisplay,
			  Bool fUnicodeSupport);
#endif
