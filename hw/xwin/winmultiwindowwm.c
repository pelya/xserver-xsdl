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
/* $XFree86: xc/programs/Xserver/hw/xwin/winmultiwindowwm.c,v 1.3 2003/10/02 13:30:11 eich Exp $ */

/* X headers */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <fcntl.h>
#include <setjmp.h>
#include <pthread.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

/* Fixups to prevent collisions between Windows and X headers */
#define ATOM DWORD

/* Windows headers */
#include <windows.h>

/* Local headers */
#include "winwindow.h"


/*
 * Constant defines
 */

#define WIN_CONNECT_RETRIES	5
#define WIN_CONNECT_DELAY	5
#define WIN_MSG_QUEUE_FNAME	"/dev/windows"
#define WIN_JMP_OKAY		0
#define WIN_JMP_ERROR_IO	2


/*
 * Local structures
 */

typedef struct _WMMsgNodeRec {
  winWMMessageRec	msg;
  struct _WMMsgNodeRec	*pNext;
} WMMsgNodeRec, *WMMsgNodePtr;

typedef struct _WMMsgQueueRec {
  struct _WMMsgNodeRec	*pHead;
  struct _WMMsgNodeRec	*pTail;
  pthread_mutex_t	pmMutex;
  pthread_cond_t	pcNotEmpty;
} WMMsgQueueRec, *WMMsgQueuePtr;

typedef struct _WMInfo {
  Display		*pDisplay;
  WMMsgQueueRec		wmMsgQueue;
  Atom			atmWmProtos;
  Atom			atmWmDelete;
  Atom			atmPrivMap;
} WMInfoRec, *WMInfoPtr;

typedef struct _WMProcArgRec {
  DWORD			dwScreen;
  WMInfoPtr		pWMInfo;
  pthread_mutex_t	*ppmServerStarted;
} WMProcArgRec, *WMProcArgPtr;

typedef struct _XMsgProcArgRec {
  Display      *pDisplay;
  DWORD        dwScreen;
  WMInfoPtr    pWMInfo;
  pthread_mutex_t	*ppmServerStarted;
} XMsgProcArgRec, *XMsgProcArgPtr;


/*
 * References to external symbols
 */

extern char *display;
extern void ErrorF (const char* /*f*/, ...);
extern Bool g_fCalledSetLocale;
extern Bool g_fCalledXInitThreads;


/*
 * Prototypes for local functions
 */

static void
PushMessage (WMMsgQueuePtr pQueue, WMMsgNodePtr pNode);

static WMMsgNodePtr
PopMessage (WMMsgQueuePtr pQueue, WMInfoPtr pWMInfo);

static Bool
InitQueue (WMMsgQueuePtr pQueue);

static void
GetWindowName (Display * pDpy, Window iWin, char **ppName);

static int
SendXMessage (Display *pDisplay, Window iWin, Atom atmType, long nData);

static void
UpdateName (WMInfoPtr pWMInfo, Window iWindow);

static void*
winMultiWindowWMProc (void* pArg);

static int
winMultiWindowWMErrorHandler (Display *pDisp, XErrorEvent *e);

static void *
winMultiWindowXMsgProc (void *pArg);

static void
winInitMultiWindowWM (WMInfoPtr pWMInfo, WMProcArgPtr pProcArg);

static int
winMutliWindowWMIOErrorHandler (Display *pDisplay);


/*
 * Local globals
 */

static int			g_nQueueSize;
static jmp_buf			g_jmpEntry;
static Bool                     g_shutdown = FALSE;



/*
 * PushMessage - Push a message onto the queue
 */

static void
PushMessage (WMMsgQueuePtr pQueue, WMMsgNodePtr pNode)
{

  /* Lock the queue mutex */
  pthread_mutex_lock (&pQueue->pmMutex);

  pNode->pNext = NULL;
  
  if (pQueue->pTail != NULL)
    {
      pQueue->pTail->pNext = pNode;
    }
  pQueue->pTail = pNode;
  
  if (pQueue->pHead == NULL)
    {
      pQueue->pHead = pNode;
    }


#if 0
  switch (pNode->msg.msg)
    {
    case WM_WM_MOVE:
      ErrorF ("\tWM_WM_MOVE\n");
      break;
    case WM_WM_RAISE:
      ErrorF ("\tWM_WM_RAISE\n");
      break;
    case WM_WM_LOWER:
      ErrorF ("\tWM_WM_RAISE\n");
      break;
    case WM_WM_MAP:
      ErrorF ("\tWM_WM_MAP\n");
      break;
    case WM_WM_UNMAP:
      ErrorF ("\tWM_WM_UNMAP\n");
      break;
    case WM_WM_KILL:
      ErrorF ("\tWM_WM_KILL\n");
      break;
    default:
      ErrorF ("Unknown Message.\n");
      break;
    }
#endif

  /* Increase the count of elements in the queue by one */
  ++g_nQueueSize;

  /* Release the queue mutex */
  pthread_mutex_unlock (&pQueue->pmMutex);

  /* Signal that the queue is not empty */
  pthread_cond_signal (&pQueue->pcNotEmpty);
}


#if CYGMULTIWINDOW_DEBUG
/*
 * QueueSize - Return the size of the queue
 */

static int
QueueSize (WMMsgQueuePtr pQueue)
{
  WMMsgNodePtr		pNode;
  int			nSize = 0;
  
  /* Loop through all elements in the queue */
  for (pNode = pQueue->pHead; pNode != NULL; pNode = pNode->pNext)
    ++nSize;

  return nSize;
}
#endif


/*
 * PopMessage - Pop a message from the queue
 */

static WMMsgNodePtr
PopMessage (WMMsgQueuePtr pQueue, WMInfoPtr pWMInfo)
{
  WMMsgNodePtr		pNode;

  /* Lock the queue mutex */
  pthread_mutex_lock (&pQueue->pmMutex);

  /* Wait for --- */
  while (pQueue->pHead == NULL)
    {
      pthread_cond_wait (&pQueue->pcNotEmpty, &pQueue->pmMutex);
    }
  
  pNode = pQueue->pHead;
  if (pQueue->pHead != NULL)
    {
      pQueue->pHead = pQueue->pHead->pNext;
    }

  if (pQueue->pTail == pNode)
    {
      pQueue->pTail = NULL;
    }

  /* Drop the number of elements in the queue by one */
  --g_nQueueSize;

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("Queue Size %d %d\n", g_nQueueSize, QueueSize(pQueue));
#endif
  
  /* Release the queue mutex */
  pthread_mutex_unlock (&pQueue->pmMutex);

  return pNode;
}


#if 0
/*
 * HaveMessage - 
 */

static Bool
HaveMessage (WMMsgQueuePtr pQueue, UINT msg, Window iWindow)
{
  WMMsgNodePtr pNode;
  
  for (pNode = pQueue->pHead; pNode != NULL; pNode = pNode->pNext)
    {
      if (pNode->msg.msg==msg && pNode->msg.iWindow==iWindow)
	return True;
    }
  
  return False;
}
#endif


/*
 * InitQueue - Initialize the Window Manager message queue
 */

static
Bool
InitQueue (WMMsgQueuePtr pQueue)
{
  /* Check if the pQueue pointer is NULL */
  if (pQueue == NULL)
    {
      ErrorF ("InitQueue - pQueue is NULL.  Exiting.\n");
      return FALSE;
    }

  /* Set the head and tail to NULL */
  pQueue->pHead = NULL;
  pQueue->pTail = NULL;

  /* There are no elements initially */
  g_nQueueSize = 0;

#if CYGMULTIWINDOW_DEBUG
  ErrorF ("InitQueue - Queue Size %d %d\n", g_nQueueSize, QueueSize(pQueue));
#endif

  ErrorF ("InitQueue - Calling pthread_mutex_init\n");

  /* Create synchronization objects */
  pthread_mutex_init (&pQueue->pmMutex, NULL);

  ErrorF ("InitQueue - pthread_mutex_init returned\n");
  ErrorF ("InitQueue - Calling pthread_cond_init\n");

  pthread_cond_init (&pQueue->pcNotEmpty, NULL);

  ErrorF ("InitQueue - pthread_cond_init returned\n");

  return TRUE;
}


/*
 * GetWindowName - Retrieve the title of an X Window
 */

static void
GetWindowName (Display *pDisplay, Window iWin, char **ppName)
{
  int			nResult, nNum;
  char			**ppList;
  XTextProperty		xtpName;
  
#if CYGMULTIWINDOW_DEBUG
  ErrorF ("GetWindowName\n");
#endif

  /* Intialize ppName to NULL */
  *ppName = NULL;

  /* Try to get --- */
  nResult = XGetWMName (pDisplay, iWin, &xtpName);
  if (!nResult || !xtpName.value || !xtpName.nitems)
    {
#if CYGMULTIWINDOW_DEBUG
      ErrorF ("GetWindowName - XGetWMName failed.  No name.\n");
#endif
      return;
    }
  
  /* */
  if (xtpName.encoding == XA_STRING)
    {
      /* */
      if (xtpName.value)
	{
	  *ppName = strdup ((char*)xtpName.value);
	  XFree (xtpName.value);
	}

#if CYGMULTIWINDOW_DEBUG
      ErrorF ("GetWindowName - XA_STRING %s\n", *ppName);
#endif
    }
  else
    {
      XmbTextPropertyToTextList (pDisplay, &xtpName, &ppList, &nNum);

      /* */
      if (nNum && ppList && *ppList)
	{
	  XFree (xtpName.value);
	  *ppName = strdup (*ppList);
	  XFreeStringList (ppList);
	}

#if CYGMULTIWINDOW_DEBUG
      ErrorF ("GetWindowName - %s %s\n",
	      XGetAtomName (pDisplay, xtpName.encoding), *ppName);
#endif
    }


#if CYGMULTIWINDOW_DEBUG
  ErrorF ("GetWindowName - Returning\n");
#endif
}


/*
 * Send a message to the X server from the WM thread
 */

static int
SendXMessage (Display *pDisplay, Window iWin, Atom atmType, long nData)
{
  XEvent		e;

  /* Prepare the X event structure */
  e.type = ClientMessage;
  e.xclient.window = iWin;
  e.xclient.message_type = atmType;
  e.xclient.format = 32;
  e.xclient.data.l[0] = nData;
  e.xclient.data.l[1] = CurrentTime;

  /* Send the event to X */
  return XSendEvent (pDisplay, iWin, False, NoEventMask, &e);
}


/*
 * Updates the name of a HWND according to its X WM_NAME property
 */

static void
UpdateName (WMInfoPtr pWMInfo, Window iWindow)
{
  char			*pszName;
  Atom			atmType;
  int			fmtRet;
  unsigned long		items, remain;
  HWND			*retHwnd, hWnd;
  XWindowAttributes	attr;

  hWnd = 0;

  /* See if we can get the cached HWND for this window... */
  if (XGetWindowProperty (pWMInfo->pDisplay,
			  iWindow,
			  pWMInfo->atmPrivMap,
			  0,
			  1,
			  False,
			  pWMInfo->atmPrivMap,
			  &atmType,
			  &fmtRet,
			  &items,
			  &remain,
			  (unsigned char **) &retHwnd) == Success)
    {
      if (retHwnd)
	{
	  hWnd = *retHwnd;
	  XFree (retHwnd);
	}
    }
  
  /* Some sanity checks */
  if (!hWnd) return;
  if (!IsWindow (hWnd)) return;
  
  /* Set the Windows window name */
  GetWindowName (pWMInfo->pDisplay, iWindow, &pszName);
  if (pszName)
    {
      /* Get the window attributes */
      XGetWindowAttributes (pWMInfo->pDisplay,
			    iWindow,
			    &attr);
      if (!attr.override_redirect)
	{
	  SetWindowText (hWnd, pszName);
	  winUpdateIcon (iWindow);
	}

      free (pszName);
    }
}


/*
 * winMultiWindowWMProc
 */

static void *
winMultiWindowWMProc (void *pArg)
{
  WMProcArgPtr		pProcArg = (WMProcArgPtr)pArg;
  WMInfoPtr		pWMInfo = pProcArg->pWMInfo;
  
  /* Initialize the Window Manager */
  winInitMultiWindowWM (pWMInfo, pProcArg);
  
#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winMultiWindowWMProc ()\n");
#endif

  /* Loop until we explicity break out */
  for (;;)
    {
      WMMsgNodePtr	pNode;

      /* Pop a message off of our queue */
      pNode = PopMessage (&pWMInfo->wmMsgQueue, pWMInfo);
      if (pNode == NULL)
	{
	  /* Bail if PopMessage returns without a message */
	  /* NOTE: Remember that PopMessage is a blocking function. */
	  ErrorF ("winMultiWindowWMProc - Queue is Empty?\n");
	  pthread_exit (NULL);
	}

#if CYGMULTIWINDOW_DEBUG
      ErrorF ("winMultiWindowWMProc - %d ms MSG: %d ID: %d\n",
	      GetTickCount (), (int)pNode->msg.msg, (int)pNode->msg.dwID);
#endif
      
      /* Branch on the message type */
      switch (pNode->msg.msg)
	{
#if 0
	case WM_WM_MOVE:
	  ErrorF ("\tWM_WM_MOVE\n");
	  break;

	case WM_WM_SIZE:
	  ErrorF ("\tWM_WM_SIZE\n");
	  break;
#endif

	case WM_WM_RAISE:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\tWM_WM_RAISE\n");
#endif

	  /* Raise the window */
	  XRaiseWindow (pWMInfo->pDisplay, pNode->msg.iWindow);
	  break;

	case WM_WM_LOWER:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\tWM_WM_LOWER\n");
#endif

	  /* Lower the window */
	  XLowerWindow (pWMInfo->pDisplay, pNode->msg.iWindow);
	  break;

	case WM_WM_MAP:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\tWM_WM_MAP\n");
#endif
	  /* Put a note as to the HWND associated with this Window */
	  XChangeProperty (pWMInfo->pDisplay,
			   pNode->msg.iWindow,
			   pWMInfo->atmPrivMap,
			   pWMInfo->atmPrivMap,
			   32,
			   PropModeReplace,
			   (unsigned char *) &(pNode->msg.hwndWindow),
			   1);
	  UpdateName (pWMInfo, pNode->msg.iWindow);
	  winUpdateIcon (pNode->msg.iWindow);
	  break;

	case WM_WM_UNMAP:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\tWM_WM_UNMAP\n");
#endif
	  
	  /* Unmap the window */
	  XUnmapWindow (pWMInfo->pDisplay, pNode->msg.iWindow);
	  break;

	case WM_WM_KILL:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\tWM_WM_KILL\n");
#endif
	  {
	    int				i, n, found = 0;
	    Atom			*protocols;
	    
	    /* --- */
	    if (XGetWMProtocols (pWMInfo->pDisplay,
				 pNode->msg.iWindow,
				 &protocols,
				 &n))
	      {
		for (i = 0; i < n; ++i)
		  if (protocols[i] == pWMInfo->atmWmDelete)
		    ++found;
		
		XFree (protocols);
	      }

	    /* --- */
	    if (found)
	      SendXMessage (pWMInfo->pDisplay,
			    pNode->msg.iWindow,
			    pWMInfo->atmWmProtos,
			    pWMInfo->atmWmDelete);
	    else
	      XKillClient (pWMInfo->pDisplay,
			   pNode->msg.iWindow);
	  }
	  break;

	case WM_WM_ACTIVATE:
#if CYGMULTIWINDOW_DEBUG
	  ErrorF ("\tWM_WM_ACTIVATE\n");
#endif
	  
	  /* Set the input focus */
	  XSetInputFocus (pWMInfo->pDisplay,
			  pNode->msg.iWindow,
			  RevertToPointerRoot,
			  CurrentTime);
	  break;

	case WM_WM_NAME_EVENT:
	  UpdateName (pWMInfo, pNode->msg.iWindow);
	  break;

	case WM_WM_HINTS_EVENT:
	  winUpdateIcon (pNode->msg.iWindow);
	  break;

	case WM_WM_CHANGE_STATE:
	  /* Minimize the window in Windows */
	  winMinimizeWindow (pNode->msg.iWindow);
	  break;

	default:
	  ErrorF ("winMultiWindowWMProc - Unknown Message.\n");
	  pthread_exit (NULL);
	  break;
	}

      /* Free the retrieved message */
      free (pNode);

      /* Flush any pending events on our display */
      XFlush (pWMInfo->pDisplay);
    }

  /* Free the condition variable */
  pthread_cond_destroy (&pWMInfo->wmMsgQueue.pcNotEmpty);
  
  /* Free the mutex variable */
  pthread_mutex_destroy (&pWMInfo->wmMsgQueue.pmMutex);
  
  /* Free the passed-in argument */
  free (pProcArg);
  
#if CYGMULTIWINDOW_DEBUG
  ErrorF("-winMultiWindowWMProc ()\n");
#endif
}


/*
 * winMultiWindowWMErrorHandler - Our application specific error handler
 */

static int
winMultiWindowWMErrorHandler (Display *pDisplay, XErrorEvent *pErr)
{
  char pszErrorMsg[100];

  if (pErr->request_code == X_ChangeWindowAttributes
      && pErr->error_code == BadAccess)
    {
      ErrorF ("winMultiWindowWMErrorHandler - ChangeWindowAttributes "
	      "BadAccess.\n");
#if 0
      pthread_exit (NULL);
#endif
      return 0;
    }
  
  XGetErrorText (pDisplay,
		 pErr->error_code,
		 pszErrorMsg,
		 sizeof (pszErrorMsg));
  ErrorF ("winMultiWindowWMErrorHandler - ERROR: %s\n", pszErrorMsg);

  if (pErr->error_code == BadWindow
      || pErr->error_code == BadMatch
      || pErr->error_code == BadDrawable)
    {
#if 0
      pthread_exit (NULL);
#endif
      return 0;
    }

  pthread_exit (NULL);
  return 0;
}


/*
 *
 */

static void *
winMultiWindowXMsgProc (void *pArg)
{
  winWMMessageRec       msg;
  XMsgProcArgPtr	pProcArg = (XMsgProcArgPtr) pArg;
  char			pszDisplay[512];
  int                   iRetries;
  XEvent		event;
  Atom                  atmWmName;
  Atom                  atmWmHints;
  Atom			atmWmChange;
  int			iReturn;
  XIconSize		*xis;

  ErrorF ("winMultiWindowXMsgProc - Hello\n");

  /* Check that argument pointer is not invalid */
  if (pProcArg == NULL)
    {
      ErrorF ("winMultiWindowXMsgProc - pProcArg is NULL, bailing.\n");
      pthread_exit (NULL);
    }

  ErrorF ("winMultiWindowXMsgProc - Calling pthread_mutex_lock ()\n");

  /* Grab the server started mutex - pause until we get it */
  iReturn = pthread_mutex_lock (pProcArg->ppmServerStarted);
  if (iReturn != 0)
    {
      ErrorF ("winMultiWindowXMsgProc - pthread_mutex_lock () failed: %d\n",
	      iReturn);
      pthread_exit (NULL);
    }

  ErrorF ("winMultiWindowXMsgProc - pthread_mutex_lock () returned.\n");

  /* Only call XInitThreads once for the whole process */
  if (!g_fCalledXInitThreads)
    {
      /* Allow multiple threads to access Xlib */
      if (XInitThreads () == 0)
	{
	  ErrorF ("winMultiWindowXMsgProc - XInitThreads () failed.\n");
	  pthread_exit (NULL);
	}
      
      /* Flag that XInitThreads has been called */
      g_fCalledXInitThreads = TRUE;

      ErrorF ("winMultiWindowXMsgProc - XInitThreads () returned.\n");
    }

  /* Release the server started mutex */
  pthread_mutex_unlock (pProcArg->ppmServerStarted);

  ErrorF ("winMultiWindowXMsgProc - pthread_mutex_unlock () returned.\n");

  /* Setup the display connection string x */
  snprintf (pszDisplay,
	    512, "127.0.0.1:%s.%d", display, (int)pProcArg->dwScreen);

  /* Print the display connection string */
  ErrorF ("winMultiWindowXMsgProc - DISPLAY=%s\n", pszDisplay);
  
  iRetries = 0;

  /* Open the X display */
  do
    {
      /* Try to open the display */
      pProcArg->pDisplay = XOpenDisplay (pszDisplay);
      if (pProcArg->pDisplay == NULL)
	{
	  ErrorF ("winMultiWindowXMsgProc - Could not open display, try: %d, "
		  "sleeping: %d\n\f",
		  iRetries + 1, WIN_CONNECT_DELAY);
	  ++iRetries;
	  sleep (WIN_CONNECT_DELAY);
	  continue;
	}
      else
	break;
    }
  while (pProcArg->pDisplay == NULL && iRetries < WIN_CONNECT_RETRIES);
  
  /* Make sure that the display opened */
  if (pProcArg->pDisplay == NULL)
    {
      ErrorF ("winMultiWindowXMsgProcwinInitMultiWindowWM - "
	      "Failed opening the display, giving up.\n\f");
      pthread_exit (NULL);
    }

  ErrorF ("winMultiWindowXMsgProc - XOpenDisplay () returned and "
	  "successfully opened the display.\n");
  
  /* Install our error handler */
  XSetErrorHandler (winMultiWindowWMErrorHandler);
  XSetIOErrorHandler (winMutliWindowWMIOErrorHandler);

  XSelectInput (pProcArg->pDisplay,
		RootWindow(pProcArg->pDisplay, pProcArg->dwScreen),
		SubstructureNotifyMask);
  
  /* Set up the supported icon sizes */
  xis = XAllocIconSize ();
  if (xis)
    {
      xis->min_width = xis->min_height = 16;
      xis->max_width = xis->max_height = 48;
      xis->width_inc = xis->height_inc = 16;
      XSetIconSizes (pProcArg->pDisplay,
		     RootWindow (pProcArg->pDisplay, pProcArg->dwScreen),
		     xis,
		     1);
      XFree (xis);
    }

  atmWmName   = XInternAtom (pProcArg->pDisplay,
			     "WM_NAME",
			     False);
  atmWmHints   = XInternAtom (pProcArg->pDisplay,
			      "WM_HINTS",
			      False);
  atmWmChange  = XInternAtom (pProcArg->pDisplay,
			      "WM_CHANGE_STATE",
			      False);

  /* Loop until we explicitly break out */
  while (1)
    {
      /* Fetch next event */
      XNextEvent (pProcArg->pDisplay, &event);

      /* Branch on event type */
      if (event.type == CreateNotify)
	{
	  XSelectInput (pProcArg->pDisplay,
			event.xcreatewindow.window,
			PropertyChangeMask);
	}
      else if (event.type == PropertyNotify
	       && event.xproperty.atom == atmWmName)
	{
	  memset (&msg, 0, sizeof (msg));

	  msg.msg = WM_WM_NAME_EVENT;
	  msg.iWindow = event.xproperty.window;

	  /* Other fields ignored */
	  winSendMessageToWM (pProcArg->pWMInfo, &msg);
	}
      else if (event.type == PropertyNotify
	       && event.xproperty.atom == atmWmHints)
	{
	  memset (&msg, 0, sizeof (msg));

	  msg.msg = WM_WM_HINTS_EVENT;
	  msg.iWindow = event.xproperty.window;

	  /* Other fields ignored */
	  winSendMessageToWM (pProcArg->pWMInfo, &msg);
	}
      else if (event.type == ClientMessage
	       && event.xclient.message_type == atmWmChange
	       && event.xclient.data.l[0] == IconicState)
	{
	  ErrorF ("winMultiWindowXMsgProc - WM_CHANGE_STATE - IconicState\n");

	  memset (&msg, 0, sizeof (msg));

	  msg.msg = WM_WM_CHANGE_STATE;
	  msg.iWindow = event.xclient.window;

	  winSendMessageToWM (pProcArg->pWMInfo, &msg);
	}
    }
}


/*
 * winInitWM - Entry point for the X server to spawn
 * the Window Manager thread.  Called from
 * winscrinit.c/winFinishScreenInitFB ().
 */

Bool
winInitWM (void **ppWMInfo,
	   pthread_t *ptWMProc,
	   pthread_t *ptXMsgProc,
	   pthread_mutex_t *ppmServerStarted,
	   int dwScreen)
{
  WMProcArgPtr		pArg = (WMProcArgPtr) malloc (sizeof(WMProcArgRec));
  WMInfoPtr		pWMInfo = (WMInfoPtr) malloc (sizeof(WMInfoRec));
  XMsgProcArgPtr	pXMsgArg = (XMsgProcArgPtr) malloc (sizeof(XMsgProcArgRec));

  /* Bail if the input parameters are bad */
  if (pArg == NULL || pWMInfo == NULL)
    {
      ErrorF ("winInitWM - malloc fail.\n");
      return FALSE;
    }
  
  /* Set a return pointer to the Window Manager info structure */
  *ppWMInfo = pWMInfo;

  /* Setup the argument structure for the thread function */
  pArg->dwScreen = dwScreen;
  pArg->pWMInfo = pWMInfo;
  pArg->ppmServerStarted = ppmServerStarted;
  
  /* Intialize the message queue */
  if (!InitQueue (&pWMInfo->wmMsgQueue))
    {
      ErrorF ("winInitWM - InitQueue () failed.\n");
      return FALSE;
    }
  
  /* Spawn a thread for the Window Manager */
  if (pthread_create (ptWMProc, NULL, winMultiWindowWMProc, pArg))
    {
      /* Bail if thread creation failed */
      ErrorF ("winInitWM - pthread_create failed for Window Manager.\n");
      return FALSE;
    }

  /* Spawn the XNextEvent thread, will send messages to WM */
  pXMsgArg->dwScreen = dwScreen;
  pXMsgArg->pWMInfo = pWMInfo;
  pXMsgArg->ppmServerStarted = ppmServerStarted;
  if (pthread_create (ptXMsgProc, NULL, winMultiWindowXMsgProc, pXMsgArg))
    {
      /* Bail if thread creation failed */
      ErrorF ("winInitWM - pthread_create failed on XMSG.\n");
      return FALSE;
    }

#if CYGDEBUG || YES
  ErrorF ("winInitWM - Returning.\n");
#endif

  return TRUE;
}


/*
 * winInitMultiWindowWM - 
 */

Bool
winClipboardDetectUnicodeSupport ();

static void
winInitMultiWindowWM (WMInfoPtr pWMInfo, WMProcArgPtr pProcArg)
{
  int                   iRetries = 0;
  char			pszDisplay[512];
  int			iReturn;
  Bool			fUnicodeSupport;

  ErrorF ("winInitMultiWindowWM - Hello\n");

  /* Check that argument pointer is not invalid */
  if (pProcArg == NULL)
    {
      ErrorF ("winInitMultiWindowWM - pProcArg is NULL, bailing.\n");
      pthread_exit (NULL);
    }

  ErrorF ("winInitMultiWindowWM - Calling pthread_mutex_lock ()\n");

  /* Grab our garbage mutex to satisfy pthread_cond_wait */
  iReturn = pthread_mutex_lock (pProcArg->ppmServerStarted);
  if (iReturn != 0)
    {
      ErrorF ("winInitMultiWindowWM - pthread_mutex_lock () failed: %d\n",
	      iReturn);
      pthread_exit (NULL);
    }

  ErrorF ("winInitMultiWindowWM - pthread_mutex_lock () returned.\n");

  /* Do we have Unicode support? */
  fUnicodeSupport = winClipboardDetectUnicodeSupport ();

  /* Set the current locale?  What does this do? */
  if (fUnicodeSupport && !g_fCalledSetLocale)
    {
      ErrorF ("winInitMultiWindowWM - Calling setlocale ()\n");
      if (!setlocale (LC_ALL, ""))
	{
	  ErrorF ("winInitMultiWindowWM - setlocale () error\n");
	  pthread_exit (NULL);
	}
      ErrorF ("winInitMultiWindowWM - setlocale () returned\n");
      
      /* See if X supports the current locale */
      if (XSupportsLocale () == False)
	{
	  ErrorF ("winInitMultiWindowWM - Locale not supported by X\n");
	  pthread_exit (NULL);
	}
    }

  /* Flag that we have called setlocale */
  g_fCalledSetLocale = TRUE;
  
  /* Only call XInitThreads once for the whole process */
  if (!g_fCalledXInitThreads)
    {
      /* Allow multiple threads to access Xlib */
      if (XInitThreads () == 0)
	{
	  ErrorF ("winInitMultiWindowWM - XInitThreads () failed.\n");
	  pthread_exit (NULL);
	}
      
      /* Flag that XInitThreads has been called */
      g_fCalledXInitThreads = TRUE;

      ErrorF ("winInitMultiWindowWM - XInitThreads () returned.\n");
    }

  /* Release the server started mutex */
  pthread_mutex_unlock (pProcArg->ppmServerStarted);

  ErrorF ("winInitMultiWindowWM - pthread_mutex_unlock () returned.\n");

  /* Set jump point for IO Error exits */
  iReturn = setjmp (g_jmpEntry);
  
  /* Check if we should continue operations */
  if (iReturn != WIN_JMP_ERROR_IO
      && iReturn != WIN_JMP_OKAY)
    {
      /* setjmp returned an unknown value, exit */
      ErrorF ("winInitMultiWindowWM - setjmp returned: %d exiting\n",
	      iReturn);
      pthread_exit (NULL);
    }
  else if (g_shutdown)
    {
      /* Shutting down, the X server severed out connection! */
      ErrorF ("winInitMultiWindowWM - Detected shutdown in progress\n");
      pthread_exit (NULL);
    }
  else if (iReturn == WIN_JMP_ERROR_IO)
    {
      ErrorF ("winInitMultiWindowWM - setjmp returned WIN_JMP_ERROR_IO\n");
    }

  /* Setup the display connection string x */
  snprintf (pszDisplay,
	    512,
	    "127.0.0.1:%s.%d",
	    display,
	    (int) pProcArg->dwScreen);

  /* Print the display connection string */
  ErrorF ("winInitMultiWindowWM - DISPLAY=%s\n", pszDisplay);
  
  /* Open the X display */
  do
    {
      /* Try to open the display */
      pWMInfo->pDisplay = XOpenDisplay (pszDisplay);
      if (pWMInfo->pDisplay == NULL)
	{
	  ErrorF ("winInitMultiWindowWM - Could not open display, try: %d, "
		  "sleeping: %d\n\f",
		  iRetries + 1, WIN_CONNECT_DELAY);
	  ++iRetries;
	  sleep (WIN_CONNECT_DELAY);
	  continue;
	}
      else
	break;
    }
  while (pWMInfo->pDisplay == NULL && iRetries < WIN_CONNECT_RETRIES);
  
  /* Make sure that the display opened */
  if (pWMInfo->pDisplay == NULL)
    {
      ErrorF ("winInitMultiWindowWM - Failed opening the display, "
	      "giving up.\n\f");
      pthread_exit (NULL);
    }

  ErrorF ("winInitMultiWindowWM - XOpenDisplay () returned and "
	  "successfully opened the display.\n");
  
  /* Install our error handler */
  XSetErrorHandler (winMultiWindowWMErrorHandler);
  XSetIOErrorHandler (winMutliWindowWMIOErrorHandler);

  /* Create some atoms */
  pWMInfo->atmWmProtos = XInternAtom (pWMInfo->pDisplay,
				      "WM_PROTOCOLS",
				      False);
  pWMInfo->atmWmDelete = XInternAtom (pWMInfo->pDisplay,
				      "WM_DELETE_WINDOW",
				      False);
  pWMInfo->atmPrivMap  = XInternAtom (pWMInfo->pDisplay,
				      WIN_HWND_CACHE,
				      False);
}


/*
 * winSendMessageToWM - Send a message from the X thread to the WM thread
 */

void
winSendMessageToWM (void *pWMInfo, winWMMessagePtr pMsg)
{
  WMMsgNodePtr pNode;
  
#if CYGMULTIWINDOW_DEBUG
  ErrorF ("winSendMessageToWM ()\n");
#endif
  
  pNode = (WMMsgNodePtr)malloc(sizeof(WMMsgNodeRec));
  if (pNode != NULL)
    {
      memcpy (&pNode->msg, pMsg, sizeof(winWMMessageRec));
      PushMessage (&((WMInfoPtr)pWMInfo)->wmMsgQueue, pNode);
    }
}


/*
 * winMutliWindowWMIOErrorHandler - Our application specific IO error handler
 */

static int
winMutliWindowWMIOErrorHandler (Display *pDisplay)
{
  printf ("\nwinMutliWindowWMIOErrorHandler!\n\n");

  /* Restart at the main entry point */
  longjmp (g_jmpEntry, WIN_JMP_ERROR_IO);
  
  return 0;
}


/*
 * Notify the MWM thread we're exiting and not to reconnect
 */

void
winDeinitMultiWindowWM ()
{
  ErrorF ("winDeinitMultiWindowWM - Noting shutdown in progress\n");
  g_shutdown = TRUE;
}
