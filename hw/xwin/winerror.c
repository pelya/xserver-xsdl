/*
 *Copyright (C) 2001-2004 Harold L Hunt II All Rights Reserved.
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
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of Harold L Hunt II
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Harold L Hunt II.
 *
 * Authors:	Harold L Hunt II
 */

#include "win.h"

/* References to external symbols */
extern char *		g_pszCommandLine;
extern Bool		g_fSilentFatalError;


#ifdef DDXOSVERRORF
/* Prototype */
void
OsVendorVErrorF (const char *pszFormat, va_list va_args);

void
OsVendorVErrorF (const char *pszFormat, va_list va_args)
{
  static pthread_mutex_t	s_pmPrinting = PTHREAD_MUTEX_INITIALIZER;

  /* Lock the printing mutex */
  pthread_mutex_lock (&s_pmPrinting);

  /* Print the error message to a log file, could be stderr */
  LogVWrite (0, pszFormat, va_args);

  /* Unlock the printing mutex */
  pthread_mutex_unlock (&s_pmPrinting);
}
#endif


/*
 * os/util.c/FatalError () calls our vendor ErrorF, so the message
 * from a FatalError will be logged.  Thus, the message for the
 * fatal error is not passed to this function.
 *
 * Attempt to do last-ditch, safe, important cleanup here.
 */
#ifdef DDXOSFATALERROR
void
OsVendorFatalError (void)
{
  /* Don't give duplicate warning if UseMsg was called */
  if (g_fSilentFatalError)
    return;

  winMessageBoxF ("A fatal error has occurred and Cygwin/X will now exit.\n" \
		  "Please open /tmp/XWin.log for more information.\n",
		  MB_ICONERROR);
}
#endif


/*
 * winMessageBoxF - Print a formatted error message in a useful
 * message box.
 */

void
winMessageBoxF (const char *pszError, UINT uType, ...)
{
  int		i;
  char *	pszErrorF = NULL;
  char *	pszMsgBox = NULL;
  va_list	args;

  /* Get length of formatted error string */
  va_start (args, uType);
  i = sprintf (NULL, pszError, args);
  va_end (args);
  
  /* Allocate memory for formatted error string */
  pszErrorF = malloc (i);
  if (!pszErrorF)
    goto winMessageBoxF_Cleanup;

  /* Create the formatted error string */
  va_start (args, uType);
  sprintf (pszErrorF, pszError, args);
  va_end (args);

#define MESSAGEBOXF \
	"%s\n" \
	"Vendor: %s\n" \
	"Release: %s\n" \
	"Contact: %s\n" \
	"XWin was started with the following command-line:\n\n" \
	"%s\n"

  /* Get length of message box string */
  i = sprintf (NULL, MESSAGEBOXF,
	       pszErrorF,
	       VENDOR_STRING, VERSION_STRING, VENDOR_CONTACT,
	       g_pszCommandLine);

  /* Allocate memory for message box string */
  pszMsgBox = malloc (i);
  if (!pszMsgBox)
    goto winMessageBoxF_Cleanup;

  /* Format the message box string */
  sprintf (pszMsgBox, MESSAGEBOXF,
	   pszErrorF,
	   VENDOR_STRING, VERSION_STRING, VENDOR_CONTACT,
	   g_pszCommandLine);

  /* Display the message box string */
  MessageBox (NULL,
	      pszMsgBox,
	      "Cygwin/X",
	      MB_OK | uType);

 winMessageBoxF_Cleanup:
  if (pszErrorF)
    free (pszErrorF);
  if (pszMsgBox)
    free (pszMsgBox);
#undef MESSAGEBOXF
}
