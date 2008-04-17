/*
Darwin event queue and event handling

Copyright 2007-2008 Apple Inc.
Copyright 2004 Kaleb S. KEITHLEY. All Rights Reserved.
Copyright (c) 2002-2004 Torrey T. Lyons. All Rights Reserved.

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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define NEED_EVENTS
#include   <X11/X.h>
#include   <X11/Xmd.h>
#include   <X11/Xproto.h>
#include   "misc.h"
#include   "windowstr.h"
#include   "pixmapstr.h"
#include   "inputstr.h"
#include   "mi.h"
#include   "scrnintstr.h"
#include   "mipointer.h"

#include "darwin.h"
#include "quartz.h"
#include "quartzKeyboard.h"
#include "darwinEvents.h"

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <IOKit/hidsystem/IOLLEvent.h>

/* Fake button press/release for scroll wheel move. */
#define SCROLLWHEELUPFAKE    4
#define SCROLLWHEELDOWNFAKE  5
#define SCROLLWHEELLEFTFAKE  6
#define SCROLLWHEELRIGHTFAKE 7

#define _APPLEWM_SERVER_
#include "applewmExt.h"
#include <X11/extensions/applewm.h>

/* FIXME: Abstract this away into xpr */
#include <Xplugin.h>
#include "rootlessWindow.h"
WindowPtr xprGetXWindow(xp_window_id wid);

int input_check_zero, input_check_flag;

static int old_flags = 0;  // last known modifier state

xEvent *darwinEvents = NULL;

/*
 * DarwinPressModifierMask
 *  Press or release the given modifier key, specified by its mask.
 */
static void DarwinPressModifierMask(
    int pressed,				    
    int mask)       // one of NX_*MASK constants
{
    int key = DarwinModifierNXMaskToNXKey(mask);

    if (key != -1) {
        int keycode = DarwinModifierNXKeyToNXKeycode(key, 0);
        if (keycode != 0)
	  DarwinSendKeyboardEvents(pressed, keycode);
    }
}

#ifdef NX_DEVICELCTLKEYMASK
#define CONTROL_MASK(flags) (flags & (NX_DEVICELCTLKEYMASK|NX_DEVICERCTLKEYMASK))
#else
#define CONTROL_MASK(flags) (NX_CONTROLMASK)
#endif /* NX_DEVICELCTLKEYMASK */

#ifdef NX_DEVICELSHIFTKEYMASK
#define SHIFT_MASK(flags) (flags & (NX_DEVICELSHIFTKEYMASK|NX_DEVICERSHIFTKEYMASK))
#else
#define SHIFT_MASK(flags) (NX_SHIFTMASK)
#endif /* NX_DEVICELSHIFTKEYMASK */

#ifdef NX_DEVICELCMDKEYMASK
#define COMMAND_MASK(flags) (flags & (NX_DEVICELCMDKEYMASK|NX_DEVICERCMDKEYMASK))
#else
#define COMMAND_MASK(flags) (NX_COMMANDMASK)
#endif /* NX_DEVICELCMDKEYMASK */

#ifdef NX_DEVICELALTKEYMASK
#define ALTERNATE_MASK(flags) (flags & (NX_DEVICELALTKEYMASK|NX_DEVICERALTKEYMASK))
#else
#define ALTERNATE_MASK(flags) (NX_ALTERNATEMASK)
#endif /* NX_DEVICELALTKEYMASK */

/*
 * DarwinUpdateModifiers
 *  Send events to update the modifier state.
 */
static void DarwinUpdateModifiers(
    int pressed,        // KeyPress or KeyRelease
    int flags )         // modifier flags that have changed
{
    if (flags & NX_ALPHASHIFTMASK) {
        DarwinPressModifierMask(pressed, NX_ALPHASHIFTMASK);
    }
    if (flags & NX_COMMANDMASK) {
        DarwinPressModifierMask(pressed, COMMAND_MASK(flags));
    }
    if (flags & NX_CONTROLMASK) {
        DarwinPressModifierMask(pressed, CONTROL_MASK(flags));
    }
    if (flags & NX_ALTERNATEMASK) {
        DarwinPressModifierMask(pressed, ALTERNATE_MASK(flags));
    }
    if (flags & NX_SHIFTMASK) {
        DarwinPressModifierMask(pressed, SHIFT_MASK(flags));
    }
    if (flags & NX_SECONDARYFNMASK) {
        DarwinPressModifierMask(pressed, NX_SECONDARYFNMASK);
    }
}

/*
 * DarwinReleaseModifiers
 * This hacky function releases all modifier keys.  It should be called when X11.app
 * is deactivated (kXquartzDeactivate) to prevent modifiers from getting stuck if they
 * are held down during a "context" switch -- otherwise, we would miss the KeyUp.
 */
static void DarwinReleaseModifiers(void) {
	DarwinUpdateModifiers(KeyRelease, COMMAND_MASK(-1) | CONTROL_MASK(-1) | ALTERNATE_MASK(-1) | SHIFT_MASK(-1));
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
    int pointer_x,
    int pointer_y,
    float pressure,
    float tilt_x,
    float tilt_y,
    int whichButton,    // mouse button to be pressed
    int modifierMask)   // modifiers used for the fake click
{
    // first fool X into forgetting about the keys
	// for some reason, it's not enough to tell X we released the Command key -- 
	// it has to be the *left* Command key.
	if (modifierMask & NX_COMMANDMASK) modifierMask |=NX_DEVICELCMDKEYMASK ;
    DarwinUpdateModifiers(KeyRelease, modifierMask);

    // push the mouse button
    DarwinSendPointerEvents(ButtonPress, whichButton, pointer_x, pointer_y, 
			    pressure, tilt_x, tilt_y);
    DarwinSendPointerEvents(ButtonRelease, whichButton, pointer_x, pointer_y, 
			    pressure, tilt_x, tilt_y);

    // restore old modifiers
    DarwinUpdateModifiers(KeyPress, modifierMask);
}

/* Generic handler for Xquartz-specifc events.  When possible, these should
   be moved into their own individual functions and set as handlers using
   mieqSetHandler. */

void DarwinEventHandler(int screenNum, xEventPtr xe, DeviceIntPtr dev, int nevents) {
  int i;

  DEBUG_LOG("DarwinEventHandler(%d, %p, %p, %d)\n", screenNum, xe, dev, nevents);
  for (i=0; i<nevents; i++) {
	switch(xe[i].u.u.type) {
		case kXquartzControllerNotify:
            DEBUG_LOG("kXquartzControllerNotify\n");
            AppleWMSendEvent(AppleWMControllerNotify,
                             AppleWMControllerNotifyMask,
                             xe[i].u.clientMessage.u.l.longs0,
                             xe[i].u.clientMessage.u.l.longs1);
            break;

        case kXquartzPasteboardNotify:
            DEBUG_LOG("kXquartzPasteboardNotify\n");
            AppleWMSendEvent(AppleWMPasteboardNotify,
                             AppleWMPasteboardNotifyMask,
                             xe[i].u.clientMessage.u.l.longs0,
                             xe[i].u.clientMessage.u.l.longs1);
            break;

        case kXquartzActivate:
            DEBUG_LOG("kXquartzActivate\n");
            QuartzShow(xe[i].u.keyButtonPointer.rootX,
                       xe[i].u.keyButtonPointer.rootY);
            AppleWMSendEvent(AppleWMActivationNotify,
                             AppleWMActivationNotifyMask,
                             AppleWMIsActive, 0);
            break;

        case kXquartzDeactivate:
            DEBUG_LOG("kXquartzDeactivate\n");
      		DarwinReleaseModifiers();
            AppleWMSendEvent(AppleWMActivationNotify,
                             AppleWMActivationNotifyMask,
                             AppleWMIsInactive, 0);
            QuartzHide();
            break;

        case kXquartzWindowState:
            DEBUG_LOG("kXquartzWindowState\n");
            RootlessNativeWindowStateChanged(xprGetXWindow(xe[i].u.clientMessage.u.l.longs0),
                                             xe[i].u.clientMessage.u.l.longs1);
            break;

        case kXquartzWindowMoved:
            DEBUG_LOG("kXquartzWindowMoved\n");
            RootlessNativeWindowMoved ((WindowPtr)xe[i].u.clientMessage.u.l.longs0);
            break;

        case kXquartzToggleFullscreen:
            DEBUG_LOG("kXquartzToggleFullscreen\n");
#ifdef DARWIN_DDX_MISSING
            if (quartzEnableRootless) QuartzSetFullscreen(!quartzHasRoot);
            else if (quartzHasRoot) QuartzHide();
            else QuartzShow();
#else
    //      ErrorF("kXquartzToggleFullscreen not implemented\n");               
#endif
            break;

        case kXquartzSetRootless:
            DEBUG_LOG("kXquartzSetRootless\n");
#ifdef DARWIN_DDX_MISSING
            QuartzSetRootless(xe[i].u.clientMessage.u.l.longs0);
            if (!quartzEnableRootless && !quartzHasRoot) QuartzHide();
#else
    //      ErrorF("kXquartzSetRootless not implemented\n");                    
#endif
            break;

        case kXquartzSetRootClip:
            QuartzSetRootClip((BOOL)xe[i].u.clientMessage.u.l.longs0);
		     break;

        case kXquartzQuit:
            GiveUp(0);
            break;

        case kXquartzBringAllToFront:
     	    DEBUG_LOG("kXquartzBringAllToFront\n");
            RootlessOrderAllWindows();
            break;

		case kXquartzSpaceChanged:
            DEBUG_LOG("kXquartzSpaceChanged\n");
            QuartzSpaceChanged(xe[i].u.clientMessage.u.l.longs0);

            break;
        default:
            ErrorF("Unknown application defined event type %d.\n", xe[i].u.u.type);
		}	
  }
}

Bool DarwinEQInit(DevicePtr pKbd, DevicePtr pPtr) { 
    if (!darwinEvents)
        darwinEvents = (xEvent *)xcalloc(sizeof(xEvent), GetMaximumEventsNum());
    if (!darwinEvents)
        FatalError("Couldn't allocate event buffer\n");

    mieqInit();
    mieqSetHandler(kXquartzReloadKeymap, DarwinKeyboardReloadHandler);
    mieqSetHandler(kXquartzActivate, DarwinEventHandler);
    mieqSetHandler(kXquartzDeactivate, DarwinEventHandler);
    mieqSetHandler(kXquartzSetRootClip, DarwinEventHandler);
    mieqSetHandler(kXquartzQuit, DarwinEventHandler);
    mieqSetHandler(kXquartzReadPasteboard, QuartzReadPasteboard);
	mieqSetHandler(kXquartzWritePasteboard, QuartzWritePasteboard);
    mieqSetHandler(kXquartzToggleFullscreen, DarwinEventHandler);
    mieqSetHandler(kXquartzSetRootless, DarwinEventHandler);
    mieqSetHandler(kXquartzSpaceChanged, DarwinEventHandler);
    mieqSetHandler(kXquartzControllerNotify, DarwinEventHandler);
    mieqSetHandler(kXquartzPasteboardNotify, DarwinEventHandler);
    mieqSetHandler(kXquartzDisplayChanged, QuartzDisplayChangedHandler);
    mieqSetHandler(kXquartzWindowState, DarwinEventHandler);
    mieqSetHandler(kXquartzWindowMoved, DarwinEventHandler);

    return TRUE;
}

/*
 * ProcessInputEvents
 *  Read and process events from the event queue until it is empty.
 */
void ProcessInputEvents(void) {
    xEvent  xe;
	int x = sizeof(xe);

    mieqProcessInputEvents();

    // Empty the signaling pipe
    while (x == sizeof(xe)) {
      x = read(darwinEventReadFD, &xe, sizeof(xe));
    }
}

/* Sends a null byte down darwinEventWriteFD, which will cause the
   Dispatch() event loop to check out event queue */
void DarwinPokeEQ(void) {
	char nullbyte=0;
	input_check_flag++;
	//  <daniels> oh, i ... er ... christ.
	write(darwinEventWriteFD, &nullbyte, 1);
}

void DarwinSendPointerEvents(int ev_type, int ev_button, int pointer_x, int pointer_y, 
			     float pressure, float tilt_x, float tilt_y) {
	static int darwinFakeMouseButtonDown = 0;
	static int darwinFakeMouseButtonMask = 0;
	int i, num_events;

	if(!darwinEvents) {
		ErrorF("DarwinSendPointerEvents called before darwinEvents was initialized\n");
		return;
	}
	/* I can't find a spec for this, but at least GTK expects that tablets are
     just like mice, except they have either one or three extra valuators, in this
     order:
     
     X coord, Y coord, pressure, X tilt, Y tilt
     Pressure and tilt should be represented natively as floats; unfortunately,
     we can't do that.  Again, GTK seems to record the min/max of each valuator,
     and then perform scaling back to float itself using that info. Soo.... */

	int valuators[5] = {pointer_x, pointer_y, 
		      pressure * INT32_MAX * 1.0f, 
		      tilt_x * INT32_MAX * 1.0f, 
		      tilt_y * INT32_MAX * 1.0f};

	if (ev_type == ButtonPress && darwinFakeButtons && ev_button == 1) {
		// Mimic multi-button mouse with modifier-clicks
		// If both sets of modifiers are pressed,
		// button 2 is clicked.
		if ((old_flags & darwinFakeMouse2Mask) == darwinFakeMouse2Mask) {
			DarwinSimulateMouseClick(pointer_x, pointer_y, pressure, 
			       tilt_x, tilt_y, 2, darwinFakeMouse2Mask);
			darwinFakeMouseButtonDown = 2;
			darwinFakeMouseButtonMask = darwinFakeMouse2Mask;
			return;
		} else if ((old_flags & darwinFakeMouse3Mask) == darwinFakeMouse3Mask) {
			DarwinSimulateMouseClick(pointer_x, pointer_y, pressure, 
			       tilt_x, tilt_y, 3, darwinFakeMouse3Mask);
			darwinFakeMouseButtonDown = 3;
			darwinFakeMouseButtonMask = darwinFakeMouse3Mask;
			return;
		}
	}

	if (ev_type == ButtonRelease && darwinFakeButtons && darwinFakeMouseButtonDown) {
		// If last mousedown was a fake click, don't check for
		// mouse modifiers here. The user may have released the
		// modifiers before the mouse button.
		ev_button = darwinFakeMouseButtonDown;
		darwinFakeMouseButtonDown = 0;
		// Bring modifiers back up to date
		DarwinUpdateModifiers(KeyPress, darwinFakeMouseButtonMask & old_flags);
		darwinFakeMouseButtonMask = 0;
		return;
	} 

	num_events = GetPointerEvents(darwinEvents, darwinPointer, ev_type, ev_button, 
				POINTER_ABSOLUTE, 0, 5, valuators);
      
	for(i=0; i<num_events; i++) mieqEnqueue (darwinPointer,&darwinEvents[i]);
	DarwinPokeEQ();
}

void DarwinSendKeyboardEvents(int ev_type, int keycode) {
	int i, num_events;
	if(!darwinEvents) {
		ErrorF("DarwinSendKeyboardEvents called before darwinEvents was initialized\n");
		return;
	}

	if (old_flags == 0 && darwinSyncKeymap && darwinKeymapFile == NULL) {
		/* See if keymap has changed. */

		static unsigned int last_seed;
		unsigned int this_seed;

		this_seed = QuartzSystemKeymapSeed();
		if (this_seed != last_seed) {
			last_seed = this_seed;
			DarwinSendDDXEvent(kXquartzReloadKeymap, 0);
		}
	}

	num_events = GetKeyboardEvents(darwinEvents, darwinKeyboard, ev_type, keycode + MIN_KEYCODE);
	for(i=0; i<num_events; i++) mieqEnqueue(darwinKeyboard,&darwinEvents[i]);
	DarwinPokeEQ();
}

void DarwinSendProximityEvents(int ev_type, int pointer_x, int pointer_y, 
			       float pressure, float tilt_x, float tilt_y) {
	int i, num_events;
	int valuators[5] = {pointer_x, pointer_y, 
		      pressure * INT32_MAX * 1.0f, 
		      tilt_x * INT32_MAX * 1.0f, 
		      tilt_y * INT32_MAX * 1.0f};

	if(!darwinEvents) {
		ErrorF("DarwinSendProximityvents called before darwinEvents was initialized\n");
		return;
	}

	num_events = GetProximityEvents(darwinEvents, darwinPointer, ev_type,
				0, 5, valuators);
      
	for(i=0; i<num_events; i++) mieqEnqueue (darwinPointer,&darwinEvents[i]);
	DarwinPokeEQ();
}


/* Send the appropriate number of button clicks to emulate scroll wheel */
void DarwinSendScrollEvents(float count_x, float count_y, 
							int pointer_x, int pointer_y, 
			    			float pressure, float tilt_x, float tilt_y) {
	if(!darwinEvents) {
		ErrorF("DarwinSendScrollEvents called before darwinEvents was initialized\n");
		return;
	}

	int sign_x = count_x > 0.0f ? SCROLLWHEELLEFTFAKE : SCROLLWHEELRIGHTFAKE;
	int sign_y = count_y > 0.0f ? SCROLLWHEELUPFAKE : SCROLLWHEELDOWNFAKE;
	count_x = fabs(count_x);
	count_y = fabs(count_y);
	
	while ((count_x > 0.0f) || (count_y > 0.0f)) {
		if (count_x > 0.0f) {
			DarwinSendPointerEvents(ButtonPress, sign_x, pointer_x, pointer_y, pressure, tilt_x, tilt_y);
			DarwinSendPointerEvents(ButtonRelease, sign_x, pointer_x, pointer_y, pressure, tilt_x, tilt_y);
			count_x = count_x - 1.0f;
		}
		if (count_y > 0.0f) {
			DarwinSendPointerEvents(ButtonPress, sign_y, pointer_x, pointer_y, pressure, tilt_x, tilt_y);
			DarwinSendPointerEvents(ButtonRelease, sign_y, pointer_x, pointer_y, pressure, tilt_x, tilt_y);
			count_y = count_y - 1.0f;
		}
	}
}

/* Send the appropriate KeyPress/KeyRelease events to GetKeyboardEvents to
   reflect changing modifier flags (alt, control, meta, etc) */
void DarwinUpdateModKeys(int flags) {
	DarwinUpdateModifiers(KeyRelease, old_flags & ~flags);
	DarwinUpdateModifiers(KeyPress, ~old_flags & flags);
	old_flags = flags;
}


/*
 * DarwinSendDDXEvent
 *  Send the X server thread a message by placing it on the event queue.
 */
void DarwinSendDDXEvent(int type, int argc, ...) {
    xEvent xe;
    INT32 *argv;
    int i, max_args;
    va_list args;

    memset(&xe, 0, sizeof(xe));
    xe.u.u.type = type;
    xe.u.clientMessage.u.l.type = type;

    argv = &xe.u.clientMessage.u.l.longs0;
    max_args = 4;

    if (argc > 0 && argc <= max_args) {
        va_start (args, argc);
        for (i = 0; i < argc; i++)
            argv[i] = (int) va_arg (args, int);
        va_end (args);
    }

    mieqEnqueue(darwinPointer, &xe);
}
