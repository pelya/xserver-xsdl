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
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winwindow.h,v 1.1 2003/02/12 15:01:38 alanh Exp $ */


#ifndef _WINWINDOW_H_
#define _WINWINDOW_H_

#ifndef NO
#define NO			0
#endif
#ifndef YES
#define YES			1
#endif

/* Constant strings */
#define WINDOW_CLASS		"cygwin/xfree86 rl"
#define WINDOW_TITLE		"Cygwin/XFree86 rl"
#define WIN_SCR_PROP		"cyg_screen_prop_rl"
#define WINDOW_CLASS_X		"cygwin/xfree86 X rl"
#define WINDOW_TITLE_X		"Cygwin/XFree86 X rl"
#define WIN_WINDOW_PROP		"cyg_window_prop_rl"
#define WIN_MSG_QUEUE_FNAME	"/dev/windows"
#define WIN_LOG_FNAME		"/tmp/XWinrl.log"
#define WIN_WID_PROP		"cyg_wid_prop_rl"
#define WIN_NEEDMANAGE_PROP	"cyg_override_redirect_prop_rl"
#define CYGMULTIWINDOW_DEBUG    NO

typedef struct _winPrivScreenRec *winPrivScreenPtr;

/*
 * Window privates
 */

typedef struct
{
  DWORD			dwDummy;
  HRGN			hRgn;
  HWND			hWnd;
  winPrivScreenPtr	pScreenPriv;
  int			iX;
  int			iY;
  int			iWidth;
  int			iHeight;
  Bool			fXKilled;
} winPrivWinRec, *winPrivWinPtr;

typedef struct _winWMMessageRec{
  DWORD			dwID;
  DWORD			msg;
  int			iWindow;
  HWND			hwndWindow;
  int			iX, iY;
  int			iWidth, iHeight;
} winWMMessageRec, *winWMMessagePtr;

/*
 * winrootlesswm.c
 */
#define		WM_WM_MOVE	(WM_USER + 1)
#define		WM_WM_SIZE	(WM_USER + 2)
#define		WM_WM_RAISE	(WM_USER + 3)
#define		WM_WM_LOWER	(WM_USER + 4)
#define		WM_WM_MAP	(WM_USER + 5)
#define		WM_WM_UNMAP	(WM_USER + 6)
#define		WM_WM_KILL	(WM_USER + 7)
#define		WM_WM_ACTIVATE	(WM_USER + 8)

#define		WMMSG_MSG	10


/*
 * winmultiwindowwm.c
 */

void
winSendMessageToWM (void *pWMInfo, winWMMessagePtr msg);

Bool
winInitWM (void **ppWMInfo,
	   pthread_t *ptWMProc,
#if 0
	   pthread_cond_t *ppcServerStarted,
#endif
	   pthread_mutex_t *ppmServerStarted,
#if 0
	   Bool *pfServerStarted,
#endif
	   int dwScreen);

#endif
