/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sysv/xqueue.c,v 3.20 2001/03/06 18:20:31 dawes Exp $ */
/*
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
/* $XConsortium: xqueue.c /main/8 1996/10/19 18:08:11 kaleb $ */

#include "X.h"
#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86Xinput.h"
#include "xf86OSmouse.h"
#include "xqueue.h"

#ifdef XQUEUE

static xqEventQueue      *XqueQaddr;
static int xqueFd = -1;
#ifndef XQUEUE_ASYNC
static int xquePipe[2];
#endif

#ifdef XKB
#include "inputstr.h"
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XKBsrv.h>
extern Bool noXkbExtension;
#endif

#include "xf86Xinput.h"
#include "mipointer.h"

typedef struct {
	int		xquePending;
	int		xqueSema;
} XqInfoRec, *XqInfoPtr;

InputInfoPtr XqMouse = NULL;
InputInfoPtr XqKeyboard = NULL;

#ifndef XQUEUE_ASYNC
/*
 * xf86XqueSignal --
 *	Trap the signal from xqueue and let it be known that events are
 *	ready for collection
 */

static void
xf86XqueSignal(int signum)
{
  ((XqInfoPtr)(((MouseDevPtr)(XqMouse->private))->mousePriv))->xquePending = 1;
  /*
   * This is a hack, but it is the only reliable way I can find of letting
   * the main select() loop know that there is more input waiting.  Receiving
   * a signal will interrupt select(), but there is no way I can find of
   * dealing with events that come in between the end of processing the
   * last set and when select() gets called.
   *
   * Suggestions for better ways of dealing with this without going back to
   * asynchronous event processing are welcome.
   */
#ifdef DEBUG
  ErrorF("xf86XqueSignal\n");
#endif
  write(xquePipe[1], "X", 1);
  signal(SIGUSR2, xf86XqueSignal);
}
#endif
  

/*
 * xf86XqueKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 */

int
xf86XqueKbdProc(DeviceIntPtr pKeyboard, int what)
{
  KeySymsRec  keySyms;
  CARD8       modMap[MAP_LENGTH];

  switch (what) {
      
  case DEVICE_INIT:
    
    xf86KbdGetMapping(&keySyms, modMap);
    
    /*
     * Get also the initial led settings
     */
    ioctl(xf86Info.consoleFd, KDGETLED, &xf86Info.leds);

    /*
     * Perform final initialization of the system private keyboard
     * structure and fill in various slots in the device record
     * itself which couldn't be filled in before.
     */
    pKeyboard->public.on = FALSE;

#ifdef XKB
    if (noXkbExtension) {
#endif
    InitKeyboardDeviceStruct((DevicePtr)xf86Info.pKeyboard,
			     &keySyms,
			     modMap,
			     xf86KbdBell,
			     (KbdCtrlProcPtr)xf86KbdCtrl);
#ifdef XKB
    } else {
	XkbComponentNamesRec names;
	if (XkbInitialMap) {
	    if ((xf86Info.xkbkeymap = strchr(XkbInitialMap, '/')) != NULL)
		xf86Info.xkbkeymap++;
	    else
		xf86Info.xkbkeymap = XkbInitialMap;
	}
	if (xf86Info.xkbkeymap) {
	    names.keymap = xf86Info.xkbkeymap;
	    names.keycodes = NULL;
	    names.types = NULL;
	    names.compat = NULL;
	    names.symbols = NULL;
	    names.geometry = NULL;
	} else {
	    names.keymap = NULL;
	    names.keycodes = xf86Info.xkbkeycodes;
	    names.types = xf86Info.xkbtypes;
	    names.compat = xf86Info.xkbcompat;
	    names.symbols = xf86Info.xkbsymbols;
	    names.geometry = xf86Info.xkbgeometry;
	}
	if ((xf86Info.xkbkeymap || xf86Info.xkbcomponents_specified)
	   && (xf86Info.xkbmodel == NULL || xf86Info.xkblayout == NULL)) {
		xf86Info.xkbrules = NULL;
	}
	XkbSetRulesDflts(xf86Info.xkbrules, xf86Info.xkbmodel,
			 xf86Info.xkblayout, xf86Info.xkbvariant,
			 xf86Info.xkboptions);
	XkbInitKeyboardDeviceStruct(pKeyboard, 
				    &names,
				    &keySyms, 
				    modMap, 
				    xf86KbdBell,
				    (KbdCtrlProcPtr)xf86KbdCtrl);
    }
#endif

    xf86InitKBD(TRUE);
    break;
    
  case DEVICE_ON:
    pKeyboard->public.on = TRUE;
    xf86InitKBD(FALSE);
    break;
    
  case DEVICE_CLOSE:
  case DEVICE_OFF:
    pKeyboard->public.on = FALSE;
    break;
  }
  
  return (Success);
}


/*
 * xf86XqueEvents --
 *      Get some events from our queue. Nothing to do here ...
 */

void
xf86XqueEvents()
{
}


#ifdef XQUEUE_ASYNC
static void XqDoInput(int signum);
#endif

void
XqReadInput(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    XqInfoPtr pXq;
    xqEvent *XqueEvents;
    int XqueHead;
    char buf[100];
    signed char dx, dy;

    if (xqueFd < 0)
	return;

    pMse = pInfo->private;
    pXq = pMse->mousePriv;

    XqueEvents = XqueQaddr->xq_events;
    XqueHead = XqueQaddr->xq_head;

    while (XqueHead != XqueQaddr->xq_tail) {
	switch (XqueEvents[XqueHead].xq_type) {
	case XQ_BUTTON:
	    pMse->PostEvent(pInfo, ~(XqueEvents[XqueHead].xq_code) & 0x07,
			    0, 0, 0, 0);
#ifdef DEBUG
	    ErrorF("xqueue: buttons: %d\n", ~(XqueEvents[XqueHead].xq_code) & 0x07);
#endif
	    break;

	case XQ_MOTION:
	    dx = (signed char)XqueEvents[XqueHead].xq_x;
	    dy = (signed char)XqueEvents[XqueHead].xq_y;
	    pMse->PostEvent(pInfo, ~(XqueEvents[XqueHead].xq_code) & 0x07,
			    (int)dx, (int)dy, 0, 0);
#ifdef DEBUG
	    ErrorF("xqueue: Motion: (%d, %d) (buttons: %d)\n", dx, dy, ~(XqueEvents[XqueHead].xq_code) & 0x07);
#endif
	    break;

	case XQ_KEY:
	    /* XXX Need to deal with the keyboard part nicely. */
#ifdef DEBUG
	    ErrorF("xqueue: key: %d\n", XqueEvents[XqueHead].xq_code);
#endif
	    xf86PostKbdEvent(XqueEvents[XqueHead].xq_code);
	    break;
	default:
	    xf86Msg(X_WARNING, "Unknown Xque Event: 0x%02x\n",
		    XqueEvents[XqueHead].xq_type);
	}
      
	if ((++XqueHead) == XqueQaddr->xq_size) XqueHead = 0;
	xf86Info.inputPending = TRUE;
    }

    /* reenable the signal-processing */
#ifdef XQUEUE_ASYNC
    signal(SIGUSR2, XqDoInput);
#endif

#ifndef XQUEUE_ASYNC
    {
	int rval;

	while ((rval = read(xquePipe[0], buf, sizeof(buf))) > 0)
#ifdef DEBUG
	    ErrorF("Read %d bytes from xquePipe[0]\n", rval);
#else
	    ;
#endif
    }
#endif

#ifdef DEBUG
    ErrorF("Leaving XqReadInput()\n");
#endif
    pXq->xquePending = 0;
    XqueQaddr->xq_head = XqueQaddr->xq_tail;
    XqueQaddr->xq_sigenable = 1; /* UNLOCK */
}

#ifdef XQUEUE_ASYNC
static void
XqDoInput(int signum)
{
    if (XqMouse)
	XqReadInput(XqMouse);
}
#endif

static void
XqBlock(pointer blockData, OSTimePtr pTimeout, pointer pReadmask)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    XqInfoPtr pXq;
    /*
     * On MP SVR4 boxes, a race condition exists because the XQUEUE does
     * not have anyway to lock it for exclusive access. This results in one
     * processor putting something on the queue at the same time the other
     * processor is taking it something off. The count of items in the queue
     * can get off by 1. This just goes and checks to see if an extra event
     * was put in the queue a during this period. The signal for this event
     * was ignored while processing the previous event.
     */

    pInfo = blockData;
    pMse = pInfo->private;
    pXq = pMse-> mousePriv;
    if (!pXq->xquePending) {
#ifdef DEBUG
	ErrorF("XqBlock: calling XqReadInput()\n");
#endif
	XqReadInput((InputInfoPtr)blockData);
    } else {
#ifdef DEBUG
	ErrorF("XqBlock: not calling XqReadInput()\n");
#endif
	;
    }
    /*
     * Make sure that any events that come in here are passed on without.
     * waiting for the next wakeup.
     */
    if (xf86Info.inputPending) {
#ifdef DEBUG
	ErrorF("XqBlock: calling ProcessInputEvents()\n");
#endif
	ProcessInputEvents();
    } else {
#ifdef DEBUG
	ErrorF("XqBlock: not calling ProcessInputEvents()\n");
#endif
	;
    }
}

/*
 * XqEnable --
 *      Enable the handling of the Xque
 */

static int
XqEnable(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    XqInfoPtr pXq;
    static struct kd_quemode xqueMode;
    static Bool was_here = FALSE;

    pMse = pInfo->private;
    pXq = pMse->mousePriv;

    if (xqueFd < 0) {
	if ((xqueFd = open("/dev/mouse", O_RDONLY | O_NDELAY)) < 0) {
	    if (xf86GetAllowMouseOpenFail()) {
		xf86Msg(X_WARNING,
		    "%s: Cannot open /dev/mouse (%s) - Continuing...\n",
		    pInfo->name, strerror(errno));
		return Success;
	    } else {
		xf86Msg(X_ERROR, "%s: Cannot open /dev/mouse (%s)\n",
			pInfo->name, strerror(errno));
		return !Success;
	    }
	}
    }
#ifndef XQUEUE_ASYNC
    if (!was_here) {
	pipe(xquePipe);
	fcntl(xquePipe[0], F_SETFL, fcntl(xquePipe[0], F_GETFL, 0) | O_NDELAY);
	fcntl(xquePipe[1], F_SETFL, fcntl(xquePipe[1], F_GETFL, 0) | O_NDELAY);
	was_here = TRUE;
    }
#endif

    if (pXq->xqueSema++ == 0) {
#ifdef XQUEUE_ASYNC
	(void) signal(SIGUSR2, XqDoInput);
#else
	(void) signal(SIGUSR2, xf86XqueSignal);
#endif
	xqueMode.qsize = 64;    /* max events */
	xqueMode.signo = SIGUSR2;
	ioctl(xf86Info.consoleFd, KDQUEMODE, NULL);

	if (ioctl(xf86Info.consoleFd, KDQUEMODE, &xqueMode) < 0) {
	    xf86Msg(X_ERROR, "%s: Cannot set KDQUEMODE", pInfo->name);
	    return !Success;
	}
	XqueQaddr = (xqEventQueue *)xqueMode.qaddr;
	XqueQaddr->xq_sigenable = 1; /* UNLOCK */
    }

    return Success;
}



/*
 * xf86XqueDisable --
 *      disable the handling of the Xque
 */

static int
XqDisable(InputInfoPtr pInfo)
{
    MouseDevPtr pMse;
    XqInfoPtr pXq;

    pMse = pInfo->private;
    pXq = pMse->mousePriv;

    if (pXq->xqueSema-- == 1)
    {
	XqueQaddr->xq_sigenable = 0; /* LOCK */
      
	if (ioctl(xf86Info.consoleFd, KDQUEMODE, NULL) < 0) {
	    xf86Msg(X_ERROR, "%s: Cannot unset KDQUEMODE", pInfo->name);
	    return !Success;
	}
    }

    if (xqueFd >= 0) {
	close(xqueFd);
	xqueFd = -1;
    }

    return Success;
}

/*
 * XqMouseProc --
 *      Handle the initialization, etc. of a mouse
 */

static int
XqMouseProc(DeviceIntPtr pPointer, int what)
{
    InputInfoPtr pInfo;
    MouseDevPtr pMse;
    unchar        map[4];
    int ret;
 
    pInfo = pPointer->public.devicePrivate;
    pMse = pInfo->private;
    pMse->device = pPointer;

    switch (what) {
    case DEVICE_INIT: 
	pPointer->public.on = FALSE;

	map[1] = 1;
	map[2] = 2;
	map[3] = 3;

	InitPointerDeviceStruct((DevicePtr)pPointer, 
				map, 
				3, 
				miPointerGetMotionEvents, 
				pMse->Ctrl,
				miPointerGetMotionBufferSize());
	/* X valuator */
	xf86InitValuatorAxisStruct(pPointer, 0, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 0);
	/* Y valuator */
	xf86InitValuatorAxisStruct(pPointer, 1, 0, -1, 1, 0, 1);
	xf86InitValuatorDefaults(pPointer, 1);
	xf86MotionHistoryAllocate(pInfo);
	RegisterBlockAndWakeupHandlers(XqBlock, (WakeupHandlerProcPtr)NoopDDA,
					pInfo);
	break;
      
    case DEVICE_ON:
	pMse->lastButtons = 0;
	pMse->emulateState = 0;
	pPointer->public.on = TRUE;
	ret = XqEnable(pInfo);
#ifndef XQUEUE_ASYNC
	if (xquePipe[0] != -1) {
	    pInfo->fd = xquePipe[0];
	    AddEnabledDevice(xquePipe[0]);
	}
#endif
	return ret;
      
    case DEVICE_CLOSE:
    case DEVICE_OFF:
	pPointer->public.on = FALSE;
	ret = XqDisable(pInfo);
#ifndef XQUEUE_ASYNC
	if (xquePipe[0] != -1) {
	    RemoveEnabledDevice(xquePipe[0]);
	    pInfo->fd = -1;
	}
#endif
	return ret;
    }
    return Success;
}

Bool
XqueueMousePreInit(InputInfoPtr pInfo, const char *protocol, int flags)
{
    MouseDevPtr pMse;
    XqInfoPtr pXq;

    pMse = pInfo->private;
    pMse->protocol = protocol;
    xf86Msg(X_CONFIG, "%s: Protocol: %s\n", pInfo->name, protocol);
    pXq = pMse->mousePriv = xnfcalloc(sizeof(XqInfoRec), 1);

    /* Collect the options, and process the common options. */
    xf86CollectInputOptions(pInfo, NULL, NULL);
    xf86ProcessCommonOptions(pInfo, pInfo->options);

    /* Process common mouse options (like Emulate3Buttons, etc). */
    pMse->CommonOptions(pInfo);

    /* Setup the local procs. */
    pInfo->device_control = XqMouseProc;
#ifdef XQUEUE_ASYNC
    pInfo->read_input = NULL;
#else
    pInfo->read_input = XqReadInput;
#endif
    pInfo->fd = -1;

    XqMouse = pInfo;

    pInfo->flags |= XI86_CONFIGURED;
    return TRUE;
}

#endif /* XQUEUE */
