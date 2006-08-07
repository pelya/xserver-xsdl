/*
 *
Copyright 1990, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * mieq.c
 *
 * Machine independent event queue
 *
 */

# define NEED_EVENTS
# include   <X11/X.h>
# include   <X11/Xmd.h>
# include   <X11/Xproto.h>
# include   "misc.h"
# include   "windowstr.h"
# include   "pixmapstr.h"
# include   "inputstr.h"
# include   "mi.h"
# include   "mipointer.h"
# include   "scrnintstr.h"
# include   <X11/extensions/XI.h>
# include   <X11/extensions/XIproto.h>
# include   "extinit.h"
# include   "exglobals.h"

#define QUEUE_SIZE  256

typedef struct _Event {
    xEvent          event[7];
    int             nevents;
    ScreenPtr	    pScreen;
    DeviceIntPtr    pDev;
} EventRec, *EventPtr;

typedef struct _EventQueue {
    HWEventQueueType head, tail;         /* long for SetInputCheck */
    CARD32           lastEventTime;      /* to avoid time running backwards */
    int              lastMotion;         /* device ID if last event motion? */
    EventRec         events[QUEUE_SIZE]; /* static allocation for signals */
    ScreenPtr        pEnqueueScreen;     /* screen events are being delivered to */
    ScreenPtr        pDequeueScreen;     /* screen events are being dispatched to */
} EventQueueRec, *EventQueuePtr;

static EventQueueRec miEventQueue;

Bool
mieqInit ()
{
    miEventQueue.head = miEventQueue.tail = 0;
    miEventQueue.lastEventTime = GetTimeInMillis ();
    miEventQueue.lastMotion = FALSE;
    miEventQueue.pEnqueueScreen = screenInfo.screens[0];
    miEventQueue.pDequeueScreen = miEventQueue.pEnqueueScreen;
    SetInputCheck (&miEventQueue.head, &miEventQueue.tail);
    return TRUE;
}

/*
 * Must be reentrant with ProcessInputEvents.  Assumption: mieqEnqueue
 * will never be interrupted.  If this is called from both signal
 * handlers and regular code, make sure the signal is suspended when
 * called from regular code.
 */

void
mieqEnqueue (xEvent *e)
{
    HWEventQueueType       oldtail = miEventQueue.tail, newtail;
    int                    isMotion = 0;
    DeviceIntPtr           pDev = NULL;
    deviceKeyButtonPointer *kbp = (deviceKeyButtonPointer *) e;
    deviceValuator         *v = (deviceValuator *) e;
    EventPtr               laste = &miEventQueue.events[oldtail - 1];
    deviceKeyButtonPointer *lastkbp = (deviceKeyButtonPointer *)
                                      &laste->event[0];

#ifdef DEBUG
    ErrorF("mieqEnqueue: slamming an event on to the queue from %d\n", kbp->deviceid & DEVICE_BITS);
    ErrorF("    type %d, detail %d\n", e->u.u.type, e->u.u.detail);
#endif

    if (e->u.u.type == MotionNotify) {
        miPointerAbsoluteCursor(e->u.keyButtonPointer.rootX,
                                e->u.keyButtonPointer.rootY,
                                e->u.keyButtonPointer.time);
        pDev = inputInfo.pointer;
        isMotion = inputInfo.pointer->id & DEVICE_BITS;
    }
    else if (e->u.u.type == KeyPress || e->u.u.type == KeyRelease) {
        pDev = inputInfo.keyboard;
    }
    else if (e->u.u.type == ButtonPress || e->u.u.type == ButtonRelease) {
        pDev = inputInfo.pointer;
    }
    else {
        pDev = LookupDeviceIntRec(kbp->deviceid & DEVICE_BITS);

        /* We silently steal valuator events: just tack them on to the last
         * motion event they need to be attached to.  Sigh. */
        if (e->u.u.type == DeviceValuator) {
            if (laste->nevents > 6) {
                ErrorF("mieqEnqueue: more than six valuator events; dropping.\n");
                return;
            }
            if (oldtail == miEventQueue.head || 
                !(lastkbp->type == DeviceMotionNotify ||
                  lastkbp->type == DeviceButtonPress ||
                  lastkbp->type == DeviceButtonRelease) ||
                ((lastkbp->deviceid & DEVICE_BITS) !=
                 (v->deviceid & DEVICE_BITS))) {
                ErrorF("mieqEnequeue: out-of-order valuator event; dropping.\n");
                return;
            }
            memcpy(&(laste->event[laste->nevents++]), e, sizeof(xEvent));
            return;
        }
        else if (e->u.u.type == DeviceMotionNotify) {
            isMotion = pDev->id & DEVICE_BITS;
        }
    }

    if (!pDev)
        FatalError("Couldn't find device for event!\n");

    if (isMotion && isMotion == miEventQueue.lastMotion &&
        oldtail != miEventQueue.head) {
	if (oldtail == 0)
	    oldtail = QUEUE_SIZE;
	oldtail = oldtail - 1;
    }
    else {
    	newtail = oldtail + 1;
    	if (newtail == QUEUE_SIZE)
	    newtail = 0;
    	/* Toss events which come in late */
    	if (newtail == miEventQueue.head) {
            ErrorF("tossed event which came in late\n");
	    return;
        }
	miEventQueue.tail = newtail;
    }

    memcpy(&(miEventQueue.events[oldtail].event[0]), e, sizeof(xEvent));
    miEventQueue.events[oldtail].nevents = 1;

    /*
     * Make sure that event times don't go backwards - this
     * is "unnecessary", but very useful
     */
    if (e->u.keyButtonPointer.time < miEventQueue.lastEventTime &&
	miEventQueue.lastEventTime - e->u.keyButtonPointer.time < 10000)
    {
#ifdef DEBUG
        ErrorF("mieq: rewinding event time from %d to %d\n",
               miEventQueue.lastEventTime,
               miEventQueue.events[oldtail].event[0].u.keyButtonPointer.time);
#endif
	miEventQueue.events[oldtail].event[0].u.keyButtonPointer.time =
	    miEventQueue.lastEventTime;
    }
    miEventQueue.lastEventTime =
	miEventQueue.events[oldtail].event[0].u.keyButtonPointer.time;
    miEventQueue.events[oldtail].pScreen = miEventQueue.pEnqueueScreen;
    miEventQueue.events[oldtail].pDev = pDev;

    miEventQueue.lastMotion = isMotion;
}

void
mieqSwitchScreen (ScreenPtr pScreen, Bool fromDIX)
{
    miEventQueue.pEnqueueScreen = pScreen;
    if (fromDIX)
	miEventQueue.pDequeueScreen = pScreen;
}

/*
 * Call this from ProcessInputEvents()
 */

void mieqProcessInputEvents ()
{
    EventRec	*e;
    int		x, y;

    while (miEventQueue.head != miEventQueue.tail)
    {
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens (SCREEN_SAVER_OFF, ScreenSaverReset);

	e = &miEventQueue.events[miEventQueue.head];
	/*
	 * Assumption - screen switching can only occur on motion events
	 */
	if (e->pScreen != miEventQueue.pDequeueScreen)
	{
	    miEventQueue.pDequeueScreen = e->pScreen;
	    x = e->event[0].u.keyButtonPointer.rootX;
	    y = e->event[0].u.keyButtonPointer.rootY;
	    if (miEventQueue.head == QUEUE_SIZE - 1)
	    	miEventQueue.head = 0;
	    else
	    	++miEventQueue.head;
	    NewCurrentScreen (miEventQueue.pDequeueScreen, x, y);
	}
	else
	{
	    if (miEventQueue.head == QUEUE_SIZE - 1)
	    	miEventQueue.head = 0;
	    else
	    	++miEventQueue.head;
            (*e->pDev->public.processInputProc)(e->event, e->pDev, e->nevents);
	}
    }
}
