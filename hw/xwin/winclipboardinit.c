/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
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

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <unistd.h>
#include <pthread.h>

#include "os.h"
#include "winclipboard.h"

#define WIN_CLIPBOARD_RETRIES			40
#define WIN_CLIPBOARD_DELAY			1

/*
 * References to external symbols
 */

extern Bool g_fClipboard;
extern HWND g_hwndClipboard;
extern Bool g_fClipboardStarted;

/*
 * Local variables
 */

static pthread_t g_ptClipboardProc;

/*
 *
 */
static void *
winClipboardThreadProc(void *arg)
{
  int clipboardRestarts = 0;

  while (1)
    {
      ++clipboardRestarts;

      /* Flag that clipboard client has been launched */
      g_fClipboardStarted = TRUE;

      winClipboardProc(arg);

      /* Flag that clipboard client has stopped */
      g_fClipboardStarted = FALSE;

      /* checking if we need to restart */
      if (clipboardRestarts >= WIN_CLIPBOARD_RETRIES) {
        /* terminates clipboard thread but the main server still lives */
        ErrorF("winClipboardProc - the clipboard thread has restarted %d times and seems to be unstable, disabling clipboard integration\n", clipboardRestarts);
        g_fClipboard = FALSE;
        break;
      }

      sleep(WIN_CLIPBOARD_DELAY);
      ErrorF("winClipboardProc - trying to restart clipboard thread \n");
    }

  return NULL;
}

/*
 * Intialize the Clipboard module
 */

Bool
winInitClipboard(void)
{
    winDebug("winInitClipboard ()\n");

    /* Spawn a thread for the Clipboard module */
    if (pthread_create(&g_ptClipboardProc, NULL, winClipboardThreadProc, NULL)) {
        /* Bail if thread creation failed */
        ErrorF("winInitClipboard - pthread_create failed.\n");
        return FALSE;
    }

    return TRUE;
}

void
winClipboardShutdown(void)
{
  /* Close down clipboard resources */
  if (g_fClipboard && g_fClipboardStarted) {
    /* Synchronously destroy the clipboard window */
    if (g_hwndClipboard != NULL) {
      SendMessage(g_hwndClipboard, WM_DESTROY, 0, 0);
      /* NOTE: g_hwndClipboard is set to NULL in winclipboardthread.c */
    }
    else
      return;

    /* Wait for the clipboard thread to exit */
    pthread_join(g_ptClipboardProc, NULL);

    g_fClipboardStarted = FALSE;

    winDebug("winClipboardShutdown - Clipboard thread has exited.\n");
  }
}

void
winFixClipboardChain(void)
{
    if (g_fClipboard && g_hwndClipboard) {
        PostMessage(g_hwndClipboard, WM_WM_REINIT, 0, 0);
    }
}
