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
/* $XFree86: xc/programs/Xserver/hw/xwin/winclipboardthread.c,v 1.3 2003/10/02 13:30:10 eich Exp $ */

#include "winclipboard.h"

/*
 * References to external symbols
 */

extern Bool g_fCalledSetLocale;


/*
 * Global variables
 */

static jmp_buf			g_jmpEntry;
static Bool                     g_shutdown = FALSE;


/*
 * Local function prototypes
 */

static int
winClipboardErrorHandler (Display *pDisplay, XErrorEvent *pErr);

static int
winClipboardIOErrorHandler (Display *pDisplay);


/*
 * Main thread function
 */

void *
winClipboardProc (void *pArg)
{
  Atom			atomClipboard, atomClipboardManager;
  Atom			atomLocalProperty, atomCompoundText;
  Atom			atomUTF8String, atomTargets;
  int			iReturn;
  HWND			hwnd = NULL;
  int			iConnectionNumber;
  int			fdMessageQueue;
  fd_set		fdsRead;
  int			iMaxDescriptor;
  Display		*pDisplay;
  Window		iWindow;
  Atom			atomDeleteWindow;
  Bool			fReturn;
  int			iRetries;
  Bool			fUnicodeSupport;
  char			szDisplay[512];
  ClipboardProcArgPtr	pProcArg = (ClipboardProcArgPtr) pArg;

  ErrorF ("winClipboardProc - Hello\n");

  /* Check that argument pointer is not invalid */
  if (pArg == NULL)
    {
      ErrorF ("winClipboardProc - pArg is NULL, bailing.\n");
      pthread_exit (NULL);
    }

  ErrorF ("winClipboardProc - Calling pthread_mutex_lock ()\n");

  /* Grab the server started mutex - pause until we get it */
  iReturn = pthread_mutex_lock (pProcArg->ppmServerStarted);
  if (iReturn != 0)
    {
      ErrorF ("winClipboardProc - pthread_mutex_lock () failed: %d\n",
	      iReturn);
      pthread_exit (NULL);
    }

  ErrorF ("winClipboardProc - pthread_mutex_lock () returned.\n");

  /* Do we have Unicode support? */
  fUnicodeSupport = winClipboardDetectUnicodeSupport ();

  /* Set the current locale?  What does this do? */
  if (fUnicodeSupport && !g_fCalledSetLocale)
    {
      ErrorF ("winClipboardProc - Calling setlocale ()\n");
      if (!setlocale (LC_ALL, ""))
	{
	  ErrorF ("winClipboardProc - setlocale () error\n");
	  pthread_exit (NULL);
	}
      ErrorF ("winClipboardProc - setlocale () returned\n");

      /* See if X supports the current locale */
      if (XSupportsLocale () == False)
	{
	  ErrorF ("winClipboardProc - Locale not supported by X\n");
	  pthread_exit (NULL);
	}
    }

  /* Flag that we have called setlocale */
  g_fCalledSetLocale = TRUE;

  /* Allow multiple threads to access Xlib */
  if (XInitThreads () == 0)
    {
      ErrorF ("winClipboardProc - XInitThreads failed.\n");
      pthread_exit (NULL);
    }

  ErrorF ("winClipboardProc - XInitThreads () returned.\n");

  /* Release the server started mutex */
  pthread_mutex_unlock (pProcArg->ppmServerStarted);

  ErrorF ("winClipboardProc - pthread_mutex_unlock () returned.\n");

  /* Set jump point for Error exits */
  iReturn = setjmp (g_jmpEntry);
  
  /* Check if we should continue operations */
  if (iReturn != WIN_JMP_ERROR_IO
      && iReturn != WIN_JMP_OKAY)
    {
      /* setjmp returned an unknown value, exit */
      ErrorF ("winClipboardProc - setjmp returned: %d exiting\n",
	      iReturn);
      pthread_exit (NULL);
    }
  else if (g_shutdown) 
    {
      /* Shutting down, the X server severed out connection! */
      ErrorF ("winClipboardProc - Detected shutdown in progress\n");
      pthread_exit (NULL);
    }
  else if (iReturn == WIN_JMP_ERROR_IO)
    {
      ErrorF ("winClipboardProc - setjmp returned and hwnd: %08x\n", hwnd);
    }

  /* Initialize retry count */
  iRetries = 0;

  /* Setup the display connection string x */
  snprintf (szDisplay,
	    512,
	    "127.0.0.1:%s.%d",
	    display,
	    (int) pProcArg->dwScreen);

  /* Print the display connection string */
  ErrorF ("winClipboardProc - DISPLAY=%s\n", szDisplay);

  /* Open the X display */
  do
    {
      pDisplay = XOpenDisplay (szDisplay);
      if (pDisplay == NULL)
	{
	  ErrorF ("winClipboardProc - Could not open display, "
		  "try: %d, sleeping: %d\n",
		  iRetries + 1, WIN_CONNECT_DELAY);
	  ++iRetries;
	  sleep (WIN_CONNECT_DELAY);
	  continue;
	}
      else
	break;
    }
  while (pDisplay == NULL && iRetries < WIN_CONNECT_RETRIES);

  /* Make sure that the display opened */
  if (pDisplay == NULL)
    {
      ErrorF ("winClipboardProc - Failed opening the display, giving up\n");
      pthread_exit (NULL);
    }

  ErrorF ("winClipboardProc - XOpenDisplay () returned and "
	  "successfully opened the display.\n");

  /* Create Windows messaging window */
  hwnd = winClipboardCreateMessagingWindow ();

  /* Get our connection number */
  iConnectionNumber = ConnectionNumber (pDisplay);

  /* Open a file descriptor for the windows message queue */
  fdMessageQueue = open (WIN_MSG_QUEUE_FNAME, O_RDONLY);
  if (fdMessageQueue == -1)
    {
      ErrorF ("winClipboardProc - Failed opening %s\n", WIN_MSG_QUEUE_FNAME);
      pthread_exit (NULL);
    }

  /* Find max of our file descriptors */
  iMaxDescriptor = max (fdMessageQueue, iConnectionNumber) + 1;

  /* Select event types to watch */
  if (XSelectInput (pDisplay,
		    DefaultRootWindow (pDisplay),
		    SubstructureNotifyMask |
		    StructureNotifyMask |
		    PropertyChangeMask) == BadWindow)
    ErrorF ("winClipboardProc - XSelectInput generated BadWindow "
	    "on RootWindow\n\n");

  /* Create a messaging window */
  iWindow = XCreateSimpleWindow (pDisplay,
				 DefaultRootWindow (pDisplay),
				 1, 1,
				 500, 500,
				 0,
				 BlackPixel (pDisplay, 0),
				 BlackPixel (pDisplay, 0));
  if (iWindow == 0)
    {
      ErrorF ("winClipboardProc - Could not create a window\n");
      pthread_exit (NULL);
    }

  /* This looks like our only hope for getting a message before shutdown */
  /* Register for WM_DELETE_WINDOW message from window manager */
  atomDeleteWindow = XInternAtom (pDisplay, "WM_DELETE_WINDOW", False);
  XSetWMProtocols (pDisplay, iWindow, &atomDeleteWindow, 1);

  /* Set error handler */
  XSetErrorHandler (winClipboardErrorHandler);
  XSetIOErrorHandler (winClipboardIOErrorHandler);

  /* Create an atom for CLIPBOARD_MANAGER */
  atomClipboardManager = XInternAtom (pDisplay, "CLIPBOARD_MANAGER", False);
  if (atomClipboardManager == None)
    {
      ErrorF ("winClipboardProc - Could not create CLIPBOARD_MANAGER atom\n");
      pthread_exit (NULL);
    }

  /* Assert ownership of CLIPBOARD_MANAGER */
  iReturn = XSetSelectionOwner (pDisplay, atomClipboardManager,
				iWindow, CurrentTime);
  if (iReturn == BadAtom || iReturn == BadWindow)
    {
      ErrorF ("winClipboardProc - Could not set CLIPBOARD_MANAGER owner\n");
      pthread_exit (NULL);
    }

  /* Create an atom for CLIPBOARD */
  atomClipboard = XInternAtom (pDisplay, "CLIPBOARD", False);
  if (atomClipboard == None)
    {
      ErrorF ("winClipboardProc - Could not create CLIPBOARD atom\n");
      pthread_exit (NULL);
    }

  /* Assert ownership of CLIPBOARD */
  iReturn = XSetSelectionOwner (pDisplay, atomClipboard,
				iWindow, CurrentTime);
  if (iReturn == BadAtom || iReturn == BadWindow)
    {
      ErrorF ("winClipboardProc - Could not set CLIPBOARD owner\n");
      pthread_exit (NULL);
    }

  /* Assert ownership of PRIMARY */
  iReturn = XSetSelectionOwner (pDisplay, XA_PRIMARY,
				iWindow, CurrentTime);
  if (iReturn == BadAtom || iReturn == BadWindow)
    {
      ErrorF ("winClipboardProc - Could not set PRIMARY owner\n");
      pthread_exit (NULL);
    }

  /* Local property to hold pasted data */
  atomLocalProperty = XInternAtom (pDisplay, "CYGX_CUT_BUFFER", False);
  if (atomLocalProperty == None)
    {
      ErrorF ("winClipboardProc - Could not create CYGX_CUT_BUFFER atom\n");
      pthread_exit (NULL);
    }

  /* Create an atom for UTF8_STRING */
  atomUTF8String = XInternAtom (pDisplay, "UTF8_STRING", False);
  if (atomUTF8String == None)
    {
      ErrorF ("winClipboardProc - Could not create UTF8_STRING atom\n");
      pthread_exit (NULL);
    }

  /* Create an atom for COMPOUND_TEXT */
  atomCompoundText = XInternAtom (pDisplay, "COMPOUND_TEXT", False);
  if (atomCompoundText == None)
    {
      ErrorF ("winClipboardProc - Could not create COMPOUND_TEXT atom\n");
      pthread_exit (NULL);
    }

  /* Create an atom for TARGETS */
  atomTargets = XInternAtom (pDisplay, "TARGETS", False);
  if (atomTargets == None)
    {
      ErrorF ("winClipboardProc - Could not create TARGETS atom\n");
      pthread_exit (NULL);
    }

  /* Pre-flush X events */
  /* 
   * NOTE: Apparently you'll freeze if you don't do this,
   *	   because there may be events in local data structures
   *	   already.
   */
  winClipboardFlushXEvents (hwnd,
			    atomClipboard,
			    atomLocalProperty,
			    atomUTF8String,
			    atomCompoundText,
			    atomTargets,
			    atomDeleteWindow,
			    iWindow,
			    pDisplay,
			    fUnicodeSupport);

  /* Pre-flush Windows messages */
  if (!winClipboardFlushWindowsMessageQueue (hwnd))
    return 0;

  /* Loop for X events */
  while (1)
    {
      /* Setup the file descriptor set */
      /*
       * NOTE: You have to do this before every call to select
       *       because select modifies the mask to indicate
       *       which descriptors are ready.
       */
      FD_ZERO (&fdsRead);
      FD_SET (fdMessageQueue, &fdsRead);
      FD_SET (iConnectionNumber, &fdsRead);

      /* Wait for a Windows event or an X event */
      iReturn = select (iMaxDescriptor,	/* Highest fds number */
			&fdsRead,	/* Read mask */
			NULL,		/* No write mask */
			NULL,		/* No exception mask */
			NULL);		/* No timeout */
      if (iReturn <= 0)
	{
	  ErrorF ("winClipboardProc - Call to select () failed: %d.  "
		  "Bailing.\n", iReturn);
	  break;
	}
      
      /* Branch on which descriptor became active */
      if (FD_ISSET (iConnectionNumber, &fdsRead))
	{
	  /* X event ready */
#if 0
	  ErrorF ("winClipboardProc - X event ready\n");
#endif

	  /* Process X events */
	  /* Exit when we see that server is shutting down */
	  fReturn = winClipboardFlushXEvents (hwnd,
					      atomClipboard,
					      atomLocalProperty,
					      atomUTF8String,
					      atomCompoundText,
					      atomTargets,
					      atomDeleteWindow,
					      iWindow,
					      pDisplay,
					      fUnicodeSupport);
	  if (!fReturn)
	    {
	      ErrorF ("winClipboardProc - Caught WM_DELETE_WINDOW - "
		      "shutting down\n");
	      break;
	    }
	}

      /* Check for Windows event ready */
      if (FD_ISSET (fdMessageQueue, &fdsRead))
	{
	  /* Windows event ready */
#if 0
	  ErrorF ("winClipboardProc - Windows event ready\n");
#endif
	  
	  /* Process Windows messages */
	  if (!winClipboardFlushWindowsMessageQueue (hwnd))
	    break;
	}
    }

  return 0;
}


/*
 * winClipboardErrorHandler - Our application specific error handler
 */

static int
winClipboardErrorHandler (Display *pDisplay, XErrorEvent *pErr)
{
  char pszErrorMsg[100];
  
  XGetErrorText (pDisplay,
		 pErr->error_code,
		 pszErrorMsg,
		 sizeof (pszErrorMsg));
  ErrorF ("winClipboardErrorHandler - ERROR: \n\t%s\n", pszErrorMsg);

  if (pErr->error_code == BadWindow
      || pErr->error_code == BadMatch
      || pErr->error_code == BadDrawable)
    {
#if 0
      pthread_exit (NULL);
#endif
    }

#if 0
  pthread_exit (NULL);
#endif

  return 0;
}


/*
 * winClipboardIOErrorHandler - Our application specific IO error handler
 */

static int
winClipboardIOErrorHandler (Display *pDisplay)
{
  printf ("\nwinClipboardIOErrorHandler!\n\n");

  /* Restart at the main entry point */
  longjmp (g_jmpEntry, WIN_JMP_ERROR_IO);
  
  return 0;
}


/*
 * Notify the clipboard thread we're exiting and not to reconnect
 */

void
winDeinitClipboard ()
{
  ErrorF ("winDeinitClipboard - Noting shutdown in progress\n");
  g_shutdown = TRUE;
}
