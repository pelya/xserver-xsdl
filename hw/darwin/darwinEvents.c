/*
 * Darwin event queue and event handling
 */
/*
Copyright (c) 2002 Torrey T. Lyons. All Rights Reserved.

This file is based on mieq.c by Keith Packard,
which contains the following copyright:
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
 */

#define NEED_EVENTS
#include   "X.h"
#include   "Xmd.h"
#include   "Xproto.h"
#include   "misc.h"
#include   "windowstr.h"
#include   "pixmapstr.h"
#include   "inputstr.h"
#include   "mi.h"
#include   "scrnintstr.h"
#include   "mipointer.h"

#include "darwin.h"
#include "quartz/quartz.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <IOKit/hidsystem/IOLLEvent.h>

/* Fake button press/release for scroll wheel move. */
#define SCROLLWHEELUPFAKE	4
#define SCROLLWHEELDOWNFAKE	5

#define QUEUE_SIZE 256

typedef struct _Event {
    xEvent	event;
    ScreenPtr	pScreen;
} EventRec, *EventPtr;

typedef struct _EventQueue {
    HWEventQueueType	head, tail; /* long for SetInputCheck */
    CARD32	lastEventTime;      /* to avoid time running backwards */
    Bool	lastMotion;
    EventRec	events[QUEUE_SIZE]; /* static allocation for signals */
    DevicePtr	pKbd, pPtr;         /* device pointer, to get funcs */
    ScreenPtr	pEnqueueScreen;     /* screen events are being delivered to */
    ScreenPtr	pDequeueScreen;     /* screen events are being dispatched to */
} EventQueueRec, *EventQueuePtr;

static EventQueueRec darwinEventQueue;


/*
 * DarwinPressModifierMask
 *  Press or release the given modifier key, specified by its mask.
 */
static void DarwinPressModifierMask(
    xEvent *xe,      // must already have type, time and mouse location
    int mask)       // one of NX_*MASK constants
{
    int key = DarwinModifierNXMaskToNXKey(mask);

    if (key != -1) {
        int keycode = DarwinModifierNXKeyToNXKeycode(key, 0);
        if (keycode != 0) {
            xe->u.u.detail = keycode + MIN_KEYCODE;
            (*darwinEventQueue.pKbd->processInputProc)(xe,
                            (DeviceIntPtr)darwinEventQueue.pKbd, 1);
        }
    }
}


/*
 * DarwinUpdateModifiers
 *  Send events to update the modifier state.
 */
static void DarwinUpdateModifiers(
    xEvent *xe,         // event template with time and mouse position set
    int pressed,        // KeyPress or KeyRelease
    int flags )         // modifier flags that have changed
{
    xe->u.u.type = pressed;
    if (flags & NX_ALPHASHIFTMASK) {
        DarwinPressModifierMask(xe, NX_ALPHASHIFTMASK);
    }
    if (flags & NX_COMMANDMASK) {
        DarwinPressModifierMask(xe, NX_COMMANDMASK);
    }
    if (flags & NX_CONTROLMASK) {
        DarwinPressModifierMask(xe, NX_CONTROLMASK);
    }
    if (flags & NX_ALTERNATEMASK) {
        DarwinPressModifierMask(xe, NX_ALTERNATEMASK);
    }
    if (flags & NX_SHIFTMASK) {
        DarwinPressModifierMask(xe, NX_SHIFTMASK);
    }
    if (flags & NX_SECONDARYFNMASK) {
        DarwinPressModifierMask(xe, NX_SECONDARYFNMASK);
    }
}


/*
 * DarwinSimulateMouseClick
 *  Send a mouse click to X when multiple mouse buttons are simulated
 *  with modifier-clicks, such as command-click for button 2. The dix
 *  layer is told that the previously pressed modifier key(s) are
 *  released, the simulated click event is sent. After the mouse button
 *  is released, the modifier keys are reverted to their actual state,
 *  which may or may not be pressed at that point. This is usually
 *  closest to what the user wants. Ie. the user typically wants to
 *  simulate a button 2 press instead of Command-button 2.
 */
static void DarwinSimulateMouseClick(
    xEvent *xe,         // event template with time and
                        // mouse position filled in
    int whichButton,    // mouse button to be pressed
    int modifierMask)   // modifiers used for the fake click
{
    // first fool X into forgetting about the keys
    DarwinUpdateModifiers(xe, KeyRelease, modifierMask);

    // push the mouse button
    xe->u.u.type = ButtonPress;
    xe->u.u.detail = whichButton;
    (*darwinEventQueue.pPtr->processInputProc)
            (xe, (DeviceIntPtr)darwinEventQueue.pPtr, 1);
}


Bool
DarwinEQInit(
    DevicePtr pKbd,
    DevicePtr pPtr)
{
    darwinEventQueue.head = darwinEventQueue.tail = 0;
    darwinEventQueue.lastEventTime = GetTimeInMillis ();
    darwinEventQueue.pKbd = pKbd;
    darwinEventQueue.pPtr = pPtr;
    darwinEventQueue.pEnqueueScreen = screenInfo.screens[0];
    darwinEventQueue.pDequeueScreen = darwinEventQueue.pEnqueueScreen;
    SetInputCheck (&darwinEventQueue.head, &darwinEventQueue.tail);
    return TRUE;
}


/*
 * DarwinEQEnqueue
 *  Must be thread safe with ProcessInputEvents.
 *    DarwinEQEnqueue    - called from event gathering thread
 *    ProcessInputEvents - called from X server thread
 *  DarwinEQEnqueue should never be called from more than one thread.
 */
void
DarwinEQEnqueue(
    const xEvent *e)
{
    HWEventQueueType oldtail, newtail;

    oldtail = darwinEventQueue.tail;

    // mieqEnqueue() collapses successive motion events into one event.
    // This is difficult to do in a thread-safe way and rarely useful.

    newtail = oldtail + 1;
    if (newtail == QUEUE_SIZE)
        newtail = 0;
    /* Toss events which come in late */
    if (newtail == darwinEventQueue.head)
        return;

    darwinEventQueue.events[oldtail].event = *e;
    /*
     * Make sure that event times don't go backwards - this
     * is "unnecessary", but very useful
     */
    if (e->u.keyButtonPointer.time < darwinEventQueue.lastEventTime &&
	darwinEventQueue.lastEventTime - e->u.keyButtonPointer.time < 10000)
    {
	darwinEventQueue.events[oldtail].event.u.keyButtonPointer.time =
	    darwinEventQueue.lastEventTime;
    }
    darwinEventQueue.events[oldtail].pScreen = darwinEventQueue.pEnqueueScreen;

    // Update the tail after the event is prepared
    darwinEventQueue.tail = newtail;
}


/*
 * DarwinEQPointerPost
 *  Post a pointer event. Used by the mipointer.c routines.
 */
void
DarwinEQPointerPost(
    xEvent *e)
{
    (*darwinEventQueue.pPtr->processInputProc)
            (e, (DeviceIntPtr)darwinEventQueue.pPtr, 1);
}


void
DarwinEQSwitchScreen(
    ScreenPtr   pScreen,
    Bool        fromDIX)
{
    darwinEventQueue.pEnqueueScreen = pScreen;
    if (fromDIX)
	darwinEventQueue.pDequeueScreen = pScreen;
}


/*
 * ProcessInputEvents
 *  Read and process events from the event queue until it is empty.
 */
void ProcessInputEvents(void)
{
    EventRec	*e;
    int		x, y;
    xEvent	xe;
    static int  old_flags = 0;	// last known modifier state
    // button number and modifier mask of currently pressed fake button
    static int darwinFakeMouseButtonDown = 0;
    static int darwinFakeMouseButtonMask = 0;

    // Empty the signaling pipe
    x = sizeof(xe);
    while (x == sizeof(xe)) {
        x = read(darwinEventFD, &xe, sizeof(xe));
    }

    while (darwinEventQueue.head != darwinEventQueue.tail)
    {
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens (SCREEN_SAVER_OFF, ScreenSaverReset);

	e = &darwinEventQueue.events[darwinEventQueue.head];
        xe = e->event;

        // Shift from global screen coordinates to coordinates relative to
        // the origin of the current screen.
        xe.u.keyButtonPointer.rootX -= darwinMainScreenX +
                dixScreenOrigins[miPointerCurrentScreen()->myNum].x;
        xe.u.keyButtonPointer.rootY -= darwinMainScreenY +
                dixScreenOrigins[miPointerCurrentScreen()->myNum].y;

	/*
	 * Assumption - screen switching can only occur on motion events
	 */
	if (e->pScreen != darwinEventQueue.pDequeueScreen)
	{
	    darwinEventQueue.pDequeueScreen = e->pScreen;
	    x = xe.u.keyButtonPointer.rootX;
	    y = xe.u.keyButtonPointer.rootY;
	    if (darwinEventQueue.head == QUEUE_SIZE - 1)
	    	darwinEventQueue.head = 0;
	    else
	    	++darwinEventQueue.head;
	    NewCurrentScreen (darwinEventQueue.pDequeueScreen, x, y);
	}
	else
	{
	    if (darwinEventQueue.head == QUEUE_SIZE - 1)
	    	darwinEventQueue.head = 0;
	    else
	    	++darwinEventQueue.head;
	    switch (xe.u.u.type) 
	    {
	    case KeyPress:
	    case KeyRelease:
                xe.u.u.detail += MIN_KEYCODE;
	    	(*darwinEventQueue.pKbd->processInputProc)
				(&xe, (DeviceIntPtr)darwinEventQueue.pKbd, 1);
	    	break;

	    case ButtonPress:
                miPointerAbsoluteCursor(xe.u.keyButtonPointer.rootX,
                                        xe.u.keyButtonPointer.rootY,
                                        xe.u.keyButtonPointer.time);
                if (darwinFakeButtons && xe.u.u.detail == 1) {
                    // Mimic multi-button mouse with modifier-clicks
                    // If both sets of modifiers are pressed,
                    // button 2 is clicked.
                    if ((old_flags & darwinFakeMouse2Mask) ==
                        darwinFakeMouse2Mask)
                    {
                        DarwinSimulateMouseClick(&xe, 2, darwinFakeMouse2Mask);
                        darwinFakeMouseButtonDown = 2;
                        darwinFakeMouseButtonMask = darwinFakeMouse2Mask;
                        break;
                    }
                    else if ((old_flags & darwinFakeMouse3Mask) ==
                             darwinFakeMouse3Mask)
                    {
                        DarwinSimulateMouseClick(&xe, 3, darwinFakeMouse3Mask);
                        darwinFakeMouseButtonDown = 3;
                        darwinFakeMouseButtonMask = darwinFakeMouse3Mask;
                        break;
                    }
                }
                (*darwinEventQueue.pPtr->processInputProc)
                        (&xe, (DeviceIntPtr)darwinEventQueue.pPtr, 1);
                break;

            case ButtonRelease:
                miPointerAbsoluteCursor(xe.u.keyButtonPointer.rootX,
                                        xe.u.keyButtonPointer.rootY,
                                        xe.u.keyButtonPointer.time);
                if (darwinFakeButtons && xe.u.u.detail == 1 &&
                    darwinFakeMouseButtonDown)
                {
                    // If last mousedown was a fake click, don't check for
                    // mouse modifiers here. The user may have released the
                    // modifiers before the mouse button.
                    xe.u.u.detail = darwinFakeMouseButtonDown;
                    darwinFakeMouseButtonDown = 0;
                    (*darwinEventQueue.pPtr->processInputProc)
                            (&xe, (DeviceIntPtr)darwinEventQueue.pPtr, 1);

                    // Bring modifiers back up to date
                    DarwinUpdateModifiers(&xe, KeyPress,
                            darwinFakeMouseButtonMask & old_flags);
                    darwinFakeMouseButtonMask = 0;
                } else {
                    (*darwinEventQueue.pPtr->processInputProc)
                            (&xe, (DeviceIntPtr)darwinEventQueue.pPtr, 1);
                }
                break;

            case MotionNotify:
                miPointerAbsoluteCursor(xe.u.keyButtonPointer.rootX,
                                        xe.u.keyButtonPointer.rootY,
                                        xe.u.keyButtonPointer.time);
	    	break;

            case kXDarwinUpdateModifiers:
            {
                // Update modifier state.
                // Any amount of modifiers may have changed.
                int flags = xe.u.clientMessage.u.l.longs0;
                DarwinUpdateModifiers(&xe, KeyRelease,
                                      old_flags & ~flags);
                DarwinUpdateModifiers(&xe, KeyPress,
                                      ~old_flags & flags);
                old_flags = flags;
                break;
            }

            case kXDarwinUpdateButtons:
            {
                long hwDelta = xe.u.clientMessage.u.l.longs0;
                long hwButtons = xe.u.clientMessage.u.l.longs1;
                int i;

                for (i = 1; i < 5; i++) {
                    if (hwDelta & (1 << i)) {
                        // IOKit and X have different numbering for the
                        // middle and right mouse buttons.
                        if (i == 1) {
                            xe.u.u.detail = 3;
                        } else if (i == 2) {
                            xe.u.u.detail = 2;
                        } else {
                            xe.u.u.detail = i + 1;
                        }
                        if (hwButtons & (1 << i)) {
                            xe.u.u.type = ButtonPress;
                        } else {
                            xe.u.u.type = ButtonRelease;
                        }
                        (*darwinEventQueue.pPtr->processInputProc)
				(&xe, (DeviceIntPtr)darwinEventQueue.pPtr, 1);
                    }
                }
                break;
            }

            case kXDarwinScrollWheel:
            {
                short count = xe.u.clientMessage.u.s.shorts0;

                if (count > 0) {
                    xe.u.u.detail = SCROLLWHEELUPFAKE;
                } else {
                    xe.u.u.detail = SCROLLWHEELDOWNFAKE;
                    count = -count;
                }

                for (; count; --count) {
                    xe.u.u.type = ButtonPress;
                    (*darwinEventQueue.pPtr->processInputProc)
                            (&xe, (DeviceIntPtr)darwinEventQueue.pPtr, 1);
                    xe.u.u.type = ButtonRelease;
                    (*darwinEventQueue.pPtr->processInputProc)
                            (&xe, (DeviceIntPtr)darwinEventQueue.pPtr, 1);
                }
                break;
            }

            default:
                if (quartz) {
                    QuartzProcessEvent(&xe);
                } else {
                    ErrorF("Unknown X event caught: %d\n", xe.u.u.type);
                }
	    }
	}
    }

    miPointerUpdate();
}
