/* $XdotOrg$ */
/*
 * Copyright 2005 by Kean Johnston <jkj@sco.com>
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993-1999 by The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any purpose.
 * It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium$ */

#include "X.h"
#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86Xinput.h"
#include "xf86OSmouse.h"
#include "xf86OSKbd.h"
#include "usl_xqueue.h"

#ifdef XKB
#include "inputstr.h"
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBsrv.h>
extern Bool noXkbExtension;
#endif

#include "xf86Xinput.h"
#include "mipointer.h"

#if !defined(XQ_WHEEL)
# define XQ_WHEEL	4
#endif

/*
 * Implementation notes
 *
 * This code is based on a mixture of the original XFree86 sysv/xqueue.c
 * and information gathered from the SCO X server code (no actual code
 * was used, just the principles).
 *
 * The XFree86 XQUEUE code went to some considerable lengths to implement
 * what it calls "asynchronous XQUEUE". This involved creating a pipe,
 * and writing to that pipe each time an XQUEUE signal is received. The
 * one end of that pipe was then added to the list of selectable file
 * descriptors with AddEnabledDevice(). I completely fail to see the need
 * for this, and this code does not implement that mechanism. The server
 * will be interrupted anyway by the XQUEUE driver, so whether we pull the
 * events off the queue at the time we receive the signal or whether we
 * write to a pipe and then have the main select() loop stop and call us,
 * it makes no difference I can fathom.
 *
 * The code also differs from the original XFree86 code in that it maintains
 * local variables for the number of devices initialized. The original code
 * stored that information in the private data pointer of the mouse structure,
 * but this same code is used for both the keyboard and the mouse, so that
 * was changed.
 *
 * Part of the difficulty in dealing with XQUEUE is that it is a single
 * interface to two devices. The recent changes in XFree86/Xorg try to
 * treat the mouse and keyboard as discrete devices, and the code is
 * structured in such a way that they should be able to be independently
 * opened and closed. But we can't do that with XQUEUE, so we have to
 * centralize XQUEUE access here in this module.
 */

static xqEventQueue *xqQaddr = NULL;
static int xqSigEnable = 1;
static int xqEnableCount = 0;
static struct kd_quemode xqMode;

/*
 * These two pointers are set when the keyboard/mouse handler procs
 * are called to turn them on or off. This is so that we can call the
 * correct PostEvent for the device.
 */
static InputInfoPtr xqMouse = NULL;
static InputInfoPtr xqKeyboard = NULL;

static void XqSignalHandler (int signo);

/*
 * Private functions
 */
static void
XqReset (void)
{
  if (xqEnableCount > 0) {
    xqQaddr->xq_head = xqQaddr->xq_tail;
    xqQaddr->xq_sigenable = xqSigEnable;
  }
}

#ifdef NOTNEEDED
static void
XqLock (void)
{
  xqSigEnable = 0;
  if (xqEnableCount > 0) {
    xqQaddr->xq_sigenable = xqSigEnable;
  }
}

static void
XqUnlock (void)
{
  xqSigEnable = 1;
  if (xqEnableCount > 0) {
    xqQaddr->xq_sigenable = xqSigEnable;
  }
}
#endif /* NOTNEEDED */

/*
 * Since this code is shared between two devices, we need to keep track
 * of how many times we've been enabled or disabled. For example, if the
 * keyboard has been turned off, but the mouse hasn't, then we do not
 * want the whole queue off. Only when both devices are turned off do we
 * actually disable Xqueue mode. When either device is turned on, we
 * enable it.
 */
static int
XqEnable (InputInfoPtr pInfo)
{
  struct sigaction xqsig;
  static int msefd = -1;

  if (msefd == -1) {
    msefd = open ("/dev/mouse", O_RDONLY | O_NONBLOCK);
#if 0
    msefd = open ("/dev/mouse", O_RDONLY | O_NONBLOCK | O_NOCTTY);
    if (msefd < 0) {
      /*
       * Try giving it a controlling tty 
       */
      msefd = open (ttyname(xf86Info.consoleFd), O_RDWR | O_NONBLOCK);
      if (msefd >= 0)
	close (msefd);
      msefd = open ("/dev/mouse", O_RDONLY | O_NONBLOCK | O_NOCTTY);
      if (msefd < 0)
	sleep(2);
    }
#endif
  }

  if (msefd < 0) {
    if (xf86GetAllowMouseOpenFail()) {
      ErrorF("%s: cannot open /dev/mouse (%s)\n",
	ttyname(xf86Info.consoleFd), strerror(errno));
    } else {
      sleep(5);
      FatalError ("%s: cannot open /dev/mouse (%s)\n",
	ttyname(xf86Info.consoleFd), strerror(errno));
    }
  }
 
  if (xqEnableCount++ == 0) {
    xqMode.qaddr = 0;
    ioctl (xf86Info.consoleFd, KDQUEMODE, NULL);

    /*
     * Note: We need to make sure the signal is armed before we enable
     * XQUEUE mode, so that if we get events immediately after the ioctl
     * we dont have an unhandled signal coming to the Xserver.
     * Also note that we use sigaction, so that we do not have to re-arm
     * the signal every time it is delivered, which just slows things
     * down (setting a signal is a fairly expensive operation).
     */

    xqsig.sa_handler = XqSignalHandler;
    sigfillset (&xqsig.sa_mask);
    xqsig.sa_flags = 0;
    sigaction (SIGUSR2, &xqsig, NULL);

    /*
     * This is a fairly large queue size. Since we are reacting to events
     * asynchronously, its best for performance if we deal with as many
     * events as possible, and high resolution mice generate a lot of
     * events.
     */
    xqMode.qsize = 64;
    xqMode.signo = SIGUSR2;
    xqMode.qaddr = 0;
    if (ioctl (xf86Info.consoleFd, KDQUEMODE, &xqMode) < 0) {
      xf86Msg (X_ERROR, "%s: could not set XQUEUE mode (%s)", pInfo->name,
	strerror(errno));
      xqEnableCount--;

      xqsig.sa_handler = SIG_DFL;
      sigfillset (&xqsig.sa_mask);
      xqsig.sa_flags = 0;
      sigaction (SIGUSR2, &xqsig, NULL);

      return !Success;
    }

    /*
     * We're in business. The workstation is now in XQUEUE mode.
     */
    xqQaddr = (xqEventQueue *)xqMode.qaddr;
    xqQaddr->xq_sigenable = 0; /* LOCK */
    nap(500);
    XqReset();
  }
  return Success;
}

static int
XqDisable (InputInfoPtr pInfo)
{
  struct sigaction xqsig;

  if (xqEnableCount-- == 1) {
    xqQaddr->xq_sigenable = 0; /* LOCK */

    if (ioctl (xf86Info.consoleFd, KDQUEMODE, NULL) < 0) {
      xf86Msg (X_ERROR, "%s: could not unset XQUEUE mode (%s)", pInfo->name,
	strerror(errno));
      xqEnableCount++;
      return !Success;
    }

    xqsig.sa_handler = SIG_DFL;
    sigfillset (&xqsig.sa_mask);
    xqsig.sa_flags = 0;
    sigaction (SIGUSR2, &xqsig, NULL);
  }

  return Success;
}

/*
 * XQUEUE signal handler. This is what goes through the list of events
 * we've already received and dispatches them to either the keyboard or
 * mouse event poster.
 */
static void
XqSignalHandler (int signo)
{
  xqEvent	*xqEvents = xqQaddr->xq_events;
  int		xqHead = xqQaddr->xq_head;
  xEvent	xE;
  MouseDevPtr	pMse = NULL;
  KbdDevPtr	pKbd = NULL;
  signed char	dx, dy;

  if (xqMouse)
    pMse = (MouseDevPtr)xqMouse->private;
  if (xqKeyboard)
    pKbd = (KbdDevPtr)xqKeyboard->private;

  while (xqHead != xqQaddr->xq_tail) {

    switch (xqEvents[xqHead].xq_type) {
      case XQ_MOTION:
	dx = (signed char)xqEvents[xqHead].xq_x;
	dy = (signed char)xqEvents[xqHead].xq_y;
	if (pMse)
	  pMse->PostEvent(xqMouse, ~(xqEvents[xqHead].xq_code) & 0x07,
			  (int)dx, (int)dy, 0, 0);
	break;

      case XQ_BUTTON:
	if (pMse)
	  pMse->PostEvent(xqMouse, ~(xqEvents[xqHead].xq_code) & 0x07,
			  0, 0, 0, 0);
	break;

      case XQ_WHEEL:
	if (pMse) {
	  int wbut = pMse->lastButtons, dz;
	  if (xqEvents[xqHead].xq_code == 1)
	    dz = 1;
	  else
	    dz = -1;
	  pMse->PostEvent(xqMouse, wbut, 0, 0, dz, 0);
	}
	break;

      case XQ_KEY:
	if (pKbd)
	  pKbd->PostEvent(xqKeyboard, xqEvents[xqHead].xq_code & 0x7f,
	    xqEvents[xqHead].xq_code & 0x80 ? FALSE : TRUE);
	break;

      default:
	xf86Msg(X_WARNING, "XQUEUE: unknown event type %d\n",
	  xqEvents[xqHead].xq_type);
	break;
    }

    xqHead++;
    if (xqHead == xqQaddr->xq_size)
      xqHead = 0;
    xf86Info.inputPending = TRUE;
  }

  XqReset();
}

/*
 * Public functions
 */
int
XqMseOnOff (InputInfoPtr pInfo, int on)
{
  if (on) {
    if (xqMouse) {
      if (xqMouse != pInfo)
	xf86Msg(X_WARNING, "XqMseOnOff: mouse pointer structure changed!\n");
      xqMouse = pInfo;
    } else {
      xqMouse = pInfo;
      return XqEnable(pInfo);
    }
  } else {
    xqMouse = NULL;
    return XqDisable(pInfo);
  }
  return Success;
}

int
XqKbdOnOff (InputInfoPtr pInfo, int on)
{
  if (on) {
    if (xqKeyboard) {
      if (xqKeyboard != pInfo)
	xf86Msg(X_WARNING, "XqKbdOnOff: keyboard pointer structure changed!\n");
      xqKeyboard = pInfo;
    } else {
      xqKeyboard = pInfo;
      return XqEnable(pInfo);
    }
  } else {
    xqKeyboard = NULL;
    return XqDisable(pInfo);
  }
  return Success;
}

