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
/* $XFree86: xc/programs/Xserver/hw/xwin/winclipboardxevents.c,v 1.3 2003/10/02 13:30:10 eich Exp $ */

#include "winclipboard.h"


/*
 * Process any pending X events
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
			  Bool fUnicodeSupport)
{
  Atom			atomReturnType;
  int			iReturnFormat;
  unsigned long		ulReturnItems;
  XTextProperty		xtpText;
  XEvent		event;
  XSelectionEvent	eventSelection;
  unsigned long		ulReturnBytesLeft;
  unsigned char		*pszReturnData = NULL;
  char			*pszGlobalData = NULL;
  int			iReturn;
  HGLOBAL		hGlobal;
  Bool			fReturn = TRUE;
  XICCEncodingStyle	xiccesStyle;
  int			iUTF8;
  char			*pszUTF8 = NULL;
  char			*pszTextList[2];
  int			iCount;
  char			**ppszTextList = NULL;
  wchar_t		*pwszUnicodeStr = NULL;
  int			iUnicodeLen = 0;

  /* Process all pending events */
  while (XPending (pDisplay))
    {
      /* Get the next event - will not block because one is ready */
      XNextEvent (pDisplay, &event);

      /* Branch on the event type */
      switch (event.type)
	{
	case ClientMessage:
	  if (event.xclient.data.l[0] == atomDeleteWindow)
	    {
	      ErrorF ("\nwinClipboardFlushXEvents - Received "
		      "WM_DELETE_WINDOW\n\n");
	      fReturn = FALSE;
	    }
	  else
	    ErrorF ("\nwinClipboardFlushXEvents - Unknown ClientMessage\n\n");
	  break;

	case SelectionClear:
	  /* Request the lost selection contents */
	  iReturn = XConvertSelection (pDisplay,
				       event.xselectionclear.selection,
				       atomCompoundText,
				       atomLocalProperty,
				       iWindow,
				       CurrentTime);
	  if (iReturn == BadAtom || iReturn == BadWindow)
	    {
	      ErrorF ("winClipboardFlushXEvents - SelectionClear - "
		      "XConvertSelection () failed\n");
	      pthread_exit (NULL);
	    }
	  break;

	  
	  /*
	   * SelectionRequest
	   */

	case SelectionRequest:
#if 0
	  char			*pszAtomName = NULL

	  ErrorF ("SelectionRequest - target %d\n",
		  event.xselectionrequest.target);

	  pszAtomName = XGetAtomName (pDisplay,
				      event.xselectionrequest.target);
	  ErrorF ("SelectionRequest - Target atom name %s\n", pszAtomName);
	  XFree (pszAtomName);
	  pszAtomName = NULL;
#endif

	  /* Abort if invalid target type */
	  if (event.xselectionrequest.target != XA_STRING
	      && event.xselectionrequest.target != atomUTF8String
	      && event.xselectionrequest.target != atomCompoundText
	      && event.xselectionrequest.target != atomTargets)
	    {
	      /* Setup selection notify event */
	      eventSelection.type = SelectionNotify;
	      eventSelection.send_event = True;
	      eventSelection.display = pDisplay;
	      eventSelection.requestor = event.xselectionrequest.requestor;
	      eventSelection.selection = event.xselectionrequest.selection;
	      eventSelection.target = event.xselectionrequest.target;
	      eventSelection.property = None;
	      eventSelection.time = event.xselectionrequest.time;

	      /* Notify the requesting window that the operation is complete */
	      iReturn = XSendEvent (pDisplay,
				    eventSelection.requestor,
				    False,
				    0L,
				    (XEvent *) &eventSelection);
	      if (iReturn == BadValue || iReturn == BadWindow)
		{
		  ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
			  "XSendEvent () failed\n");
		  pthread_exit (NULL);
		}

	      break;
	    }

	  /* Handle targets type of request */
	  if (event.xselectionrequest.target == atomTargets)
	    {
	      Atom atomTargetArr[4] = {atomTargets,
				       atomCompoundText,
				       atomUTF8String,
				       XA_STRING};

	      /* Try to change the property */
	      iReturn = XChangeProperty (pDisplay,
					 event.xselectionrequest.requestor,
					 event.xselectionrequest.property,
					 event.xselectionrequest.target,
					 8,
					 PropModeReplace,
					 (char *) atomTargetArr,
					 sizeof (atomTargetArr));
	      if (iReturn == BadAlloc
		  || iReturn == BadAtom
		  || iReturn == BadMatch
		  || iReturn == BadValue
		  || iReturn == BadWindow)
		{
		  ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
			  "XChangeProperty failed: %d\n",
			  iReturn);
		}

	      /* Setup selection notify xevent */
	      eventSelection.type = SelectionNotify;
	      eventSelection.send_event = True;
	      eventSelection.display	 = pDisplay;
	      eventSelection.requestor = event.xselectionrequest.requestor;
	      eventSelection.selection = event.xselectionrequest.selection;
	      eventSelection.target	 = event.xselectionrequest.target;
	      eventSelection.property  = event.xselectionrequest.property;
	      eventSelection.time	 = event.xselectionrequest.time;

	      /*
	       * Notify the requesting window that
	       * the operation has completed
	       */
	      iReturn = XSendEvent (pDisplay,
				    eventSelection.requestor,
				    False,
				    0L,
				    (XEvent *) &eventSelection);
	      if (iReturn == BadValue || iReturn == BadWindow)
		{
		  ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
			  "XSendEvent () failed\n");
		}
	      break;
	    }

	  /* Access the clipboard */
	  if (!OpenClipboard (hwnd))
	    {
	      ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
		      "OpenClipboard () failed: %08x\n",
		      GetLastError ());
	      pthread_exit (NULL);
	    }

	  /* Setup the string style */
	  if (event.xselectionrequest.target == XA_STRING)
	    xiccesStyle = XStringStyle;
	  else if (event.xselectionrequest.target == atomUTF8String)
	    xiccesStyle = XUTF8StringStyle;
	  else if (event.xselectionrequest.target == atomCompoundText)
	    xiccesStyle = XCompoundTextStyle;
	  else
	    xiccesStyle = XStringStyle;

	  /*
	   * FIXME: Can't pass CF_UNICODETEXT on Windows 95/98/Me
	   */
	  
	  /* Get a pointer to the clipboard text */
	  if (fUnicodeSupport)
	    hGlobal = GetClipboardData (CF_UNICODETEXT);
	  else
	    hGlobal = GetClipboardData (CF_TEXT);
	  if (!hGlobal)
	    {
	      ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
		      "GetClipboardData () failed: %08x\n",
		      GetLastError ());
	      pthread_exit (NULL);
	    }
	  pszGlobalData = (char *) GlobalLock (hGlobal);

	  /* Convert the Unicode string to UTF8 (MBCS) */
	  if (fUnicodeSupport)
	    {
	      iUTF8 = WideCharToMultiByte (CP_UTF8,
					   0,
					   (LPCWSTR)pszGlobalData,
					   -1,
					   NULL,
					   0,
					   NULL,
					   NULL);
	      pszUTF8 = (char *) malloc (iUTF8); /* Don't need +1 */
	      WideCharToMultiByte (CP_UTF8,
				   0,
				   (LPCWSTR)pszGlobalData,
				   -1,
				   pszUTF8,
				   iUTF8,
				   NULL,
				   NULL);
	    }

	  /* Convert DOS string to UNIX string */
	  if (fUnicodeSupport)
	    {
	      winClipboardDOStoUNIX (pszUTF8, strlen (pszUTF8));

	      /* Setup our text list */
	      pszTextList[0] = pszUTF8;
	      pszTextList[1] = NULL;
	      
	      /* Initialize the text property */
	      xtpText.value = NULL;
	      
	      /* Create the text property from the text list */
	      iReturn = Xutf8TextListToTextProperty (pDisplay,
						     pszTextList,
						     1,
						     xiccesStyle,
						     &xtpText);
	      if (iReturn == XNoMemory || iReturn == XLocaleNotSupported)
		{
		  ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
			  "Xutf8TextListToTextProperty failed: %d\n",
			  iReturn);
		  exit(1);
		}
	      
	      /* Free the UTF8 string */
	      free (pszUTF8);
	    }
	  else
	    winClipboardDOStoUNIX (pszGlobalData, strlen (pszGlobalData));

	  /*
	   * FIXME: Pass pszGlobalData and strlen (pszGlobalData(
	   * on 1 byte, pass xtpText.value and xtpText.nitems
	   * on 2 byte.
	   */

	  /* Copy the clipboard text to the requesting window */
	  if (fUnicodeSupport)
	    iReturn = XChangeProperty (pDisplay,
				       event.xselectionrequest.requestor,
				       event.xselectionrequest.property,
				       event.xselectionrequest.target,
				       8,
				       PropModeReplace,
				       xtpText.value,
				       xtpText.nitems);
	  else
	    iReturn = XChangeProperty (pDisplay,
				       event.xselectionrequest.requestor,
				       event.xselectionrequest.property,
				       event.xselectionrequest.target,
				       8,
				       PropModeReplace,
				       pszGlobalData,
				       strlen (pszGlobalData));
	  if (iReturn == BadAlloc || iReturn == BadAtom
	      || iReturn == BadMatch || iReturn == BadValue
	      || iReturn == BadWindow)
	    {
	      ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
		      "XChangeProperty failed: %d\n",
		      iReturn);
	      pthread_exit (NULL);
	    }

	  /* Release the clipboard data */
	  GlobalUnlock (hGlobal);
	  pszGlobalData = NULL;
	  CloseClipboard ();

	  /* FIXME: Don't clean up on 1 byte. */
	  if (fUnicodeSupport)
	    {
	      XFree (xtpText.value);
	      xtpText.value = NULL;
	    }

	  /* Setup selection notify event */
	  eventSelection.type = SelectionNotify;
	  eventSelection.send_event = True;
	  eventSelection.display = pDisplay;
	  eventSelection.requestor = event.xselectionrequest.requestor;
	  eventSelection.selection = event.xselectionrequest.selection;
	  eventSelection.target = event.xselectionrequest.target;
	  eventSelection.property = event.xselectionrequest.property;
	  eventSelection.time = event.xselectionrequest.time;

	  /* Notify the requesting window that the operation has completed */
	  iReturn = XSendEvent (pDisplay,
				eventSelection.requestor,
				False,
				0L,
				(XEvent *) &eventSelection);
	  if (iReturn == BadValue || iReturn == BadWindow)
	    {
	      ErrorF ("winClipboardFlushXEvents - SelectionRequest - "
		      "XSendEvent () failed\n");
	      pthread_exit (NULL);
	    }
	  break;
	  

	  /*
	   * SelectionNotify
	   */ 

	case SelectionNotify:
#if 0
	  ErrorF ("SelectionNotify\n");
#endif
	  {
	    char		*pszAtomName;
	    
	    pszAtomName = XGetAtomName (pDisplay,
					event.xselection.selection);

	    ErrorF ("winClipboardFlushXEvents - SelectionNotify - ATOM: %s\n",
		    pszAtomName);
	    
	    XFree (pszAtomName);
	  }

#if 0
	  /*
	   * TEMP: Bail if selection is anything other than CLIPBOARD
	   */

	  if (event.xselection.selection != atomClipboard)
	    break;
#endif

	  /*
	   *
	   * What are we doing here?
	   *
	   */
	  if (fUnicodeSupport)
	    {
	      if (event.xselection.property == None)
		{
		  if(event.xselection.target == XA_STRING)
		    {
#if 0
		      ErrorF ("winClipboardFlushXEvents - SelectionNotify - "
			      "XA_STRING\n");
#endif
		      return fReturn;
		    }
		  else if (event.xselection.target == atomUTF8String)
		    {
		      ErrorF ("winClipboardFlushXEvents - SelectionNotify "
			      "UTF8\n");
		      iReturn = XConvertSelection (pDisplay,
						   event.xselection.selection,
						   XA_STRING,
						   atomLocalProperty,
						   iWindow,
						   CurrentTime);
		      if (iReturn == BadAtom || iReturn == BadWindow)
			{
			  ErrorF ("winClipboardFlushXEvents - SelectionNotify "
				  "- XConvertSelection () failed\n");
			  pthread_exit (NULL);
			}
		      return fReturn;
		    }
		  else if (event.xselection.target == atomCompoundText)
		    {
		      ErrorF ("winClipboardFlushXEvents - SelectionNotify "
			      "CompoundText\n");
		      iReturn = XConvertSelection (pDisplay,
						   event.xselection.selection,
						   atomUTF8String,
						   atomLocalProperty,
						   iWindow,
						   CurrentTime);
		      if (iReturn == BadAtom || iReturn == BadWindow)
			{
			  ErrorF ("winClipboardFlushXEvents - SelectionNotify "
				  "- XConvertSelection () failed\n");
			  pthread_exit (NULL);
			}
		      return fReturn;
		    }
		  else
		    {
		      ErrorF ("winClipboardFlushXEvents - Unknown format\n");
		      return fReturn;
		    }
		}
	    }

	  /* Retrieve the size of the stored data */
	  if (fUnicodeSupport)
	    iReturn = XGetWindowProperty (pDisplay,
					  iWindow,
					  atomLocalProperty,
					  0,
					  0, /* Don't get data, just size */
					  False,
					  AnyPropertyType,
					  &xtpText.encoding,
					  &xtpText.format,
					  &xtpText.nitems,
					  &ulReturnBytesLeft,
					  &xtpText.value);
	  else
	    iReturn = XGetWindowProperty (pDisplay,
					  iWindow,
					  atomLocalProperty,
					  0,
					  0, /* Don't get data, just size */
					  False,
					  AnyPropertyType,
					  &atomReturnType,
					  &iReturnFormat,
					  &ulReturnItems,
					  &ulReturnBytesLeft,
					  &pszReturnData);
	  if (iReturn != Success)
	    {
	      ErrorF ("winClipboardFlushXEvents - SelectionNotify - "
		      "XGetWindowProperty () failed\n");
	      pthread_exit (NULL);
	    }

#if 0
	  if (fUnicodeSupport)
	    ErrorF ("SelectionNotify - returned data %d left %d\n",
		    xtpText.nitems, ulReturnBytesLeft);
	  else
	    ErrorF ("SelectionNotify - returned data %d left %d\n",
		    ulReturnItems, ulReturnBytesLeft);
#endif

	  /* Request the selection data */
	  if (fUnicodeSupport)
	    iReturn = XGetWindowProperty (pDisplay,
					  iWindow,
					  atomLocalProperty,
					  0,
					  ulReturnBytesLeft,
					  False,
					  AnyPropertyType,
					  &xtpText.encoding,
					  &xtpText.format,
					  &xtpText.nitems,
					  &ulReturnBytesLeft,
					  &xtpText.value);
	  else
	    iReturn = XGetWindowProperty (pDisplay,
					  iWindow,
					  atomLocalProperty,
					  0,
					  ulReturnBytesLeft,
					  False,
					  AnyPropertyType,
					  &atomReturnType,
					  &iReturnFormat,
					  &ulReturnItems,
					  &ulReturnBytesLeft,
					  &pszReturnData);
	  if (iReturn != Success)
	    {
	      ErrorF ("winClipboardFlushXEvents - SelectionNotify - "
		      "XGetWindowProperty () failed\n");
	      pthread_exit (NULL);
	    }

	  if (fUnicodeSupport)
	    {
#if 0
	      char		*pszAtomName = NULL;

	      ErrorF ("SelectionNotify - returned data %d left %d\n",
		      prop.nitems, ulReturnBytesLeft);
	      
	      pszAtomName = XGetAtomName(pDisplay, prop.encoding);
	      ErrorF ("Notify atom name %s\n", pszAtomName);
	      XFree (pszAtomName);
	      pszAtomName = NULL;
#endif
	      
	      /* Convert the text property to a text list */
	      Xutf8TextPropertyToTextList (pDisplay,
					   &xtpText,
					   &ppszTextList,
					   &iCount);
	      if (iCount > 0)
		{
		  pszReturnData = malloc (strlen (ppszTextList[0]) + 1);
		  strcpy (pszReturnData, ppszTextList[0]);
		}
	      else
		{
		  pszReturnData = malloc (1);
		  pszReturnData[0] = 0;
		}
	      
	      /* Free the data returned from XGetWindowProperty */
	      XFreeStringList (ppszTextList);
	      XFree (xtpText.value);
	    }
	      
	  /* Convert the X clipboard string to DOS format */
	  winClipboardUNIXtoDOS (&pszReturnData, strlen (pszReturnData));

	  if (fUnicodeSupport)
	    {
	      /* Find out how much space needed to convert MBCS to Unicode */
	      iUnicodeLen = MultiByteToWideChar (CP_UTF8,
						 0,
						 pszReturnData,
						 -1,
						 NULL,
						 0);
	      
	      /* Allocate memory for the Unicode string */
	      pwszUnicodeStr
		= (wchar_t*) malloc (sizeof (wchar_t) * (iUnicodeLen + 1));
	      
	      /* Do the actual conversion */
	      MultiByteToWideChar (CP_UTF8,
				   0,
				   pszReturnData,
				   -1,
				   pwszUnicodeStr,
				   iUnicodeLen);
	    }

	  /* Access the Windows clipboard */
	  if (!OpenClipboard (hwnd))
	    {
	      ErrorF ("winClipboardFlushXEvents - OpenClipboard () failed: "
		      "%08x\n", GetLastError ());
	      pthread_exit (NULL);
	    }

	  /* Take ownership of the Window clipboard */
	  if (!EmptyClipboard ())
	    {
	      ErrorF ("winClipboardFlushXEvents - EmptyClipboard () failed: "
		      "%08x\n", GetLastError ());
	      pthread_exit (NULL);
	    }

	  /* Allocate global memory for the X clipboard data */
	  if (fUnicodeSupport)
	    hGlobal = GlobalAlloc (GMEM_MOVEABLE,
				   sizeof (wchar_t) * (iUnicodeLen + 1));
	  else
	    hGlobal = GlobalAlloc (GMEM_MOVEABLE, strlen (pszReturnData) + 1);

	  /* Obtain a pointer to the global memory */
	  pszGlobalData = GlobalLock (hGlobal);
	  if (pszGlobalData == NULL)
	    {
	      ErrorF ("winClipboardFlushXEvents - Could not lock global "
		      "memory for clipboard transfer\n");
	      pthread_exit (NULL);
	    }

	  /* Copy the returned string into the global memory */
	  if (fUnicodeSupport)
	    memcpy (pszGlobalData,
		    pwszUnicodeStr,
		    sizeof (wchar_t) * (iUnicodeLen + 1));
	  else
	    strcpy (pszGlobalData, pszReturnData);

	  /* Free the data returned from XGetWindowProperty */
	  if (fUnicodeSupport)
	    {
	      free (pwszUnicodeStr);
	      pwszUnicodeStr = NULL;
	    }
	  else
	    {
	      XFree (pszReturnData);
	      pszReturnData = NULL;
	    }

	  /* Release the pointer to the global memory */
	  GlobalUnlock (hGlobal);
	  pszGlobalData = NULL;

	  /* Push the selection data to the Windows clipboard */
	  if (fUnicodeSupport)
	    SetClipboardData (CF_UNICODETEXT, hGlobal);
	  else
	  SetClipboardData (CF_TEXT, hGlobal);	    

	  /*
	   * NOTE: Do not try to free pszGlobalData, it is owned by
	   * Windows after the call to SetClipboardData ().
	   */

	  /* Release the clipboard */
	  if (!CloseClipboard ())
	    {
	      ErrorF ("winClipboardFlushXEvents - CloseClipboard () failed: "
		      "%08x\n",
		      GetLastError ());
	      pthread_exit (NULL);
	    }

	  /* Reassert ownership of the selection */	  
	  iReturn = XSetSelectionOwner (pDisplay,
					event.xselection.selection,
					iWindow, CurrentTime);
	  if (iReturn == BadAtom || iReturn == BadWindow)
	    {
	      char		*pszAtomName = NULL;

	      pszAtomName = XGetAtomName (pDisplay,
					  event.xselection.selection);
	      ErrorF ("winClipboardFlushXEvents - SelectionNotify - "
		      "Could not reassert ownership of selection ATOM: %s\n",
		      pszAtomName);
	      XFree (pszAtomName);
	      pszAtomName = NULL;
	      pthread_exit (NULL);
	    }
	  else
	    {
#if 0
	      char		*pszAtomName = NULL;

	      pszAtomName = XGetAtomName (pDisplay,
					  event.xselection.selection);
	      ErrorF ("SelectionNotify - Reasserted ownership of ATOM: %s\n",
		      pszAtomName);
	      XFree (pszAtomName);
	      pszAtomName = NULL;
#endif
	    }
#if 0
	  /* Reassert ownership of the CLIPBOARD */	  
	  iReturn = XSetSelectionOwner (pDisplay,
					atomClipboard,
					iWindow, CurrentTime);
	  if (iReturn == BadAtom || iReturn == BadWindow)
	    {
	      ErrorF ("winClipboardFlushXEvents - Could not reassert "
		      "ownership of selection\n");
	      pthread_exit (NULL);
	    }
#endif
	  break;

#if 0
	case CreateNotify:
	  ErrorF ("FlushXEvents - CreateNotify parent: %ld\twindow: %ld\n",
		  event.xcreatewindow.parent, event.xcreatewindow.window);
	  break;

	case DestroyNotify:
	  ErrorF ("FlushXEvents - DestroyNotify window: %ld\tevent: %ld\n",
		  event.xdestroywindow.window, event.xdestroywindow.event);
	  break;
#endif

	default:
	  break;
	}
    }

  return fReturn;
}
