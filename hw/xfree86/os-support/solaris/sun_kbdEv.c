/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sunos/sun_kbdEv.c,v 1.4 2001/11/08 04:15:33 tsi Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by David Dawes <dawes@xfree86.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the names of Thomas Roell and David Dawes not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Thomas Roell and David Dawes make no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THOMAS ROELL AND DAVID DAWES DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL THOMAS ROELL OR DAVID DAWES BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* [JCH-96/01/21] Extended std reverse map to four buttons. */

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#ifdef XINPUT
#include "XI.h"
#include "XIproto.h"
#include "xf86Xinput.h"
#else
#include "inputstr.h"
#endif

#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

#include <sys/vuid_event.h>
#include "atKeynames.h"

#ifdef XKB
extern Bool noXkbExtension;
#endif

#define XE_POINTER  1
#define XE_KEYBOARD 2

#ifdef XTESTEXT1

#define	XTestSERVER_SIDE
#include "xtestext1.h"
extern short xtest_mousex;
extern short xtest_mousey;
extern int   on_steal_input;
extern Bool  XTestStealKeyData();
extern void  XTestStealMotionData();

#ifdef XINPUT
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  if (!on_steal_input ||  \
      XTestStealKeyData((ev)->u.u.detail, (ev)->u.u.type, dev_type, \
			xtest_mousex, xtest_mousey)) \
  xf86eqEnqueue((ev))
#else
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  if (!on_steal_input ||  \
      XTestStealKeyData((ev)->u.u.detail, (ev)->u.u.type, dev_type, \
			xtest_mousex, xtest_mousey)) \
  mieqEnqueue((ev))
#endif

#else /* ! XTESTEXT1 */

#ifdef XINPUT
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  xf86eqEnqueue((ev))
#else
#define ENQUEUE(ev, code, direction, dev_type) \
  (ev)->u.u.detail = (code); \
  (ev)->u.u.type   = (direction); \
  mieqEnqueue((ev))
#endif

#endif

static void startautorepeat(long keycode);
static CARD32 processautorepeat(OsTimerPtr timer, CARD32 now, pointer arg);

static OsTimerPtr sunTimer = NULL;

/* Map the Solaris keycodes to the "XFree86" keycodes. */
/*
 * This doesn't seem right.  It probably needs to be dependent on a keyboard
 * type.
 */
static unsigned char map[256] = {
#if defined(i368) || defined(__i386) || defined(__i386__)
	KEY_NOTUSED,		/*   0 */
	KEY_Tilde,		/*   1 */
	KEY_1,			/*   2 */
	KEY_2,			/*   3 */
	KEY_3,			/*   4 */
	KEY_4,			/*   5 */
	KEY_5,			/*   6 */
	KEY_6,			/*   7 */
	KEY_7,			/*   8 */
	KEY_8,			/*   9 */
	KEY_9,			/*  10 */
	KEY_0,			/*  11 */
	KEY_Minus,		/*  12 */
	KEY_Equal,		/*  13 */
	0x7D, /*KEY_P_YEN*/	/*  14 */
	KEY_BackSpace,		/*  15 */
	KEY_Tab,		/*  16 */
	KEY_Q,			/*  17 */
	KEY_W,			/*  18 */
	KEY_E,			/*  19 */
	KEY_R,			/*  20 */
	KEY_T,			/*  21 */
	KEY_Y,			/*  22 */
	KEY_U,			/*  23 */
	KEY_I,			/*  24 */
	KEY_O,			/*  25 */
	KEY_P,			/*  26 */
	KEY_LBrace,		/*  27 */
	KEY_RBrace,		/*  28 */
	KEY_BSlash,		/*  29 */
	KEY_CapsLock,		/*  30 */
	KEY_A,			/*  31 */
	KEY_S,			/*  32 */
	KEY_D,			/*  33 */
	KEY_F,			/*  34 */
	KEY_G,			/*  35 */
	KEY_H,			/*  36 */
	KEY_J,			/*  37 */
	KEY_K,			/*  38 */
	KEY_L,			/*  39 */
	KEY_SemiColon,		/*  40 */
	KEY_Quote,		/*  41 */
	KEY_UNKNOWN,		/*  42 */
	KEY_Enter,		/*  43 */
	KEY_ShiftL,		/*  44 */
	KEY_Less,		/*  45 */
	KEY_Z,			/*  46 */
	KEY_X,			/*  47 */
	KEY_C,			/*  48 */
	KEY_V,			/*  49 */
	KEY_B,			/*  50 */
	KEY_N,			/*  51 */
	KEY_M,			/*  52 */
	KEY_Comma,		/*  53 */
	KEY_Period,		/*  54 */
	KEY_Slash,		/*  55 */
	KEY_BSlash2,		/*  56 */
	KEY_ShiftR,		/*  57 */
	KEY_LCtrl,		/*  58 */
	KEY_LMeta,		/*  59 */
	KEY_Alt,		/*  60 */
	KEY_Space,		/*  61 */
	KEY_AltLang,		/*  62 */
	KEY_RMeta,		/*  63 */
	KEY_RCtrl,		/*  64 */
	KEY_Menu,		/*  65 */
	KEY_UNKNOWN,		/*  66 */
	KEY_UNKNOWN,		/*  67 */
	KEY_UNKNOWN,		/*  68 */
	KEY_UNKNOWN,		/*  69 */
	KEY_UNKNOWN,		/*  70 */
	KEY_UNKNOWN,		/*  71 */
	KEY_UNKNOWN,		/*  72 */
	KEY_UNKNOWN,		/*  73 */
	KEY_UNKNOWN,		/*  74 */
	KEY_Insert,		/*  75 */
	KEY_Delete,		/*  76 */
	KEY_UNKNOWN,		/*  77 */
	KEY_UNKNOWN,		/*  78 */
	KEY_Left,		/*  79 */
	KEY_Home,		/*  80 */
	KEY_End,		/*  81 */
	KEY_UNKNOWN,		/*  82 */
	KEY_Up,			/*  83 */
	KEY_Down,		/*  84 */
	KEY_PgUp,		/*  85 */
	KEY_PgDown,		/*  86 */
	KEY_UNKNOWN,		/*  87 */
	KEY_UNKNOWN,		/*  88 */
	KEY_Right,		/*  89 */
	KEY_NumLock,		/*  90 */
	KEY_KP_7,		/*  91 */
	KEY_KP_4,		/*  92 */
	KEY_KP_1,		/*  93 */
	KEY_UNKNOWN,		/*  94 */
	KEY_KP_Divide,		/*  95 */
	KEY_KP_8,		/*  96 */
	KEY_KP_5,		/*  97 */
	KEY_KP_2,		/*  98 */
	KEY_KP_0,		/*  99 */
	KEY_KP_Multiply,	/* 100 */
	KEY_KP_9,		/* 101 */
	KEY_KP_6,		/* 102 */
	KEY_KP_3,		/* 103 */
	KEY_KP_Decimal,		/* 104 */
	KEY_KP_Minus,		/* 105 */
	KEY_KP_Plus,		/* 106 */
	KEY_UNKNOWN,		/* 107 */
	KEY_KP_Enter,		/* 108 */
	KEY_UNKNOWN,		/* 109 */
	KEY_Escape,		/* 110 */
	KEY_UNKNOWN,		/* 111 */
	KEY_F1,			/* 112 */
	KEY_F2,			/* 113 */
	KEY_F3,			/* 114 */
	KEY_F4,			/* 115 */
	KEY_F5,			/* 116 */
	KEY_F6,			/* 117 */
	KEY_F7,			/* 118 */
	KEY_F8,			/* 119 */
	KEY_F9,			/* 120 */
	KEY_F10,		/* 121 */
	KEY_F11,		/* 122 */
	KEY_F12,		/* 123 */
	KEY_Print,		/* 124 */
	KEY_ScrollLock,		/* 125 */
	KEY_Pause,		/* 126 */
	KEY_UNKNOWN,		/* 127 */
	KEY_UNKNOWN,		/* 128 */
	KEY_UNKNOWN,		/* 129 */
	KEY_UNKNOWN,		/* 130 */
	KEY_NFER,		/* 131 */
	KEY_XFER,		/* 132 */
	KEY_HKTG,		/* 133 */
	KEY_UNKNOWN,		/* 134 */
#elif defined(sparc) || defined(__sparc__)
	KEY_UNKNOWN,		/* 0x00 */
	KEY_UNKNOWN,		/* 0x01 */
	KEY_UNKNOWN,		/* 0x02 */
	KEY_UNKNOWN,		/* 0x03 */
	KEY_UNKNOWN,		/* 0x04 */
	KEY_F1,			/* 0x05 */
	KEY_F2,			/* 0x06 */
	KEY_F10,		/* 0x07 */
	KEY_F3,			/* 0x08 */
	KEY_F11,		/* 0x09 */
	KEY_F4,			/* 0x0A */
	KEY_F12,		/* 0x0B */
	KEY_F5,			/* 0x0C */
	KEY_UNKNOWN,		/* 0x0D */
	KEY_F6,			/* 0x0E */
	KEY_UNKNOWN,		/* 0x0F */
	KEY_F7,			/* 0x10 */
	KEY_F8,			/* 0x11 */
	KEY_F9,			/* 0x12 */
	KEY_Alt,		/* 0x13 */
	KEY_Up,			/* 0x14 */
	KEY_Pause,		/* 0x15 */
	KEY_SysReqest,		/* 0x16 */
	KEY_ScrollLock,		/* 0x17 */
	KEY_Left,		/* 0x18 */
	KEY_UNKNOWN,		/* 0x19 */
	KEY_UNKNOWN,		/* 0x1A */
	KEY_Down,		/* 0x1B */
	KEY_Right,		/* 0x1C */
	KEY_Escape,		/* 0x1D */
	KEY_1,			/* 0x1E */
	KEY_2,			/* 0x1F */
	KEY_3,			/* 0x20 */
	KEY_4,			/* 0x21 */
	KEY_5,			/* 0x22 */
	KEY_6,			/* 0x23 */
	KEY_7,			/* 0x24 */
	KEY_8,			/* 0x25 */
	KEY_9,			/* 0x26 */
	KEY_0,			/* 0x27 */
	KEY_Minus,		/* 0x28 */
	KEY_Equal,		/* 0x29 */
	KEY_Tilde,		/* 0x2A */
	KEY_BackSpace,		/* 0x2B */
	KEY_Insert,		/* 0x2C */
	KEY_UNKNOWN,		/* 0x2D */
	KEY_KP_Divide,		/* 0x2E */
	KEY_KP_Multiply,	/* 0x2F */
	KEY_UNKNOWN,		/* 0x30 */
	KEY_UNKNOWN,		/* 0x31 */
	KEY_KP_Decimal,		/* 0x32 */
	KEY_UNKNOWN,		/* 0x33 */
	KEY_Home,		/* 0x34 */
	KEY_Tab,		/* 0x35 */
	KEY_Q,			/* 0x36 */
	KEY_W,			/* 0x37 */
	KEY_E,			/* 0x38 */
	KEY_R,			/* 0x39 */
	KEY_T,			/* 0x3A */
	KEY_Y,			/* 0x3B */
	KEY_U,			/* 0x3C */
	KEY_I,			/* 0x3D */
	KEY_O,			/* 0x3E */
	KEY_P,			/* 0x3F */
	KEY_LBrace,		/* 0x40 */
	KEY_RBrace,		/* 0x41 */
	KEY_Delete,		/* 0x42 */
	KEY_UNKNOWN,		/* 0x43 */
	KEY_KP_7,		/* 0x44 */
	KEY_KP_8,		/* 0x45 */
	KEY_KP_9,		/* 0x46 */
	KEY_KP_Minus,		/* 0x47 */
	KEY_UNKNOWN,		/* 0x48 */
	KEY_UNKNOWN,		/* 0x49 */
	KEY_End,		/* 0x4A */
	KEY_UNKNOWN,		/* 0x4B */
	KEY_LCtrl,		/* 0x4C */
	KEY_A,			/* 0x4D */
	KEY_S,			/* 0x4E */
	KEY_D,			/* 0x4F */
	KEY_F,			/* 0x50 */
	KEY_G,			/* 0x51 */
	KEY_H,			/* 0x52 */
	KEY_J,			/* 0x53 */
	KEY_K,			/* 0x54 */
	KEY_L,			/* 0x55 */
	KEY_SemiColon,		/* 0x56 */
	KEY_Quote,		/* 0x57 */
	KEY_BSlash,		/* 0x58 */
	KEY_Enter,		/* 0x59 */
	KEY_KP_Enter,		/* 0x5A */
	KEY_KP_4,		/* 0x5B */
	KEY_KP_5,		/* 0x5C */
	KEY_KP_6,		/* 0x5D */
	KEY_KP_0,		/* 0x5E */
	KEY_UNKNOWN,		/* 0x5F */
	KEY_PgUp,		/* 0x60 */
	KEY_UNKNOWN,		/* 0x61 */
	KEY_NumLock,		/* 0x62 */
	KEY_ShiftL,		/* 0x63 */
	KEY_Z,			/* 0x64 */
	KEY_X,			/* 0x65 */
	KEY_C,			/* 0x66 */
	KEY_V,			/* 0x67 */
	KEY_B,			/* 0x68 */
	KEY_N,			/* 0x69 */
	KEY_M,			/* 0x6A */
	KEY_Comma,		/* 0x6B */
	KEY_Period,		/* 0x6C */
	KEY_Slash,		/* 0x6D */
	KEY_ShiftR,		/* 0x6E */
	KEY_UNKNOWN,		/* 0x6F */
	KEY_KP_1,		/* 0x70 */
	KEY_KP_2,		/* 0x71 */
	KEY_KP_3,		/* 0x72 */
	KEY_UNKNOWN,		/* 0x73 */
	KEY_UNKNOWN,		/* 0x74 */
	KEY_UNKNOWN,		/* 0x75 */
	KEY_UNKNOWN,		/* 0x76 */
	KEY_CapsLock,		/* 0x77 */
	KEY_LMeta,		/* 0x78 */
	KEY_Space,		/* 0x79 */
	KEY_RMeta,		/* 0x7A */
	KEY_PgDown,		/* 0x7B */
	KEY_UNKNOWN,		/* 0x7C */
	KEY_KP_Plus,		/* 0x7D */
	KEY_UNKNOWN,		/* 0x7E */
	KEY_UNKNOWN,		/* 0x7F */
#endif
	/* The rest default to KEY_UNKNOWN */
};

/*
 * sunPostKbdEvent --
 *	Translate the raw hardware Firm_event into an XEvent, and tell DIX
 *	about it. KeyCode preprocessing and so on is done ...
 *
 * Most of the Solaris stuff has whacked Panix/PC98 support in the
 * interests of simplicity - DWH 8/30/99
 */

static void
sunPostKbdEvent(Firm_event *event)
{
    Bool        down;
    KeyClassRec *keyc = ((DeviceIntPtr)xf86Info.pKeyboard)->key;
    Bool        updateLeds = FALSE;
    xEvent      kevent;
    KeySym      *keysym;
    int         keycode;
    static int  lockkeys = 0;

    /* Give down a value */
    if (event->value == VKEY_DOWN)
	down = TRUE;
    else
	down = FALSE;

    /*
     * and now get some special keysequences
     */

    keycode = map[event->id];

    if ((ModifierDown(ControlMask | AltMask)) ||
 	(ModifierDown(ControlMask | AltLangMask)))
    {
	switch (keycode) {
	/*
	 * The idea here is to pass the scancode down to a list of registered
	 * routines.  There should be some standard conventions for processing
	 * certain keys.
	 */

	case KEY_BackSpace:
	    if (!xf86Info.dontZap) {
		DGAShutdown();
		GiveUp(0);
	    }
	    break;

	/* Check grabs */
	case KEY_KP_Divide:
	    if (!xf86Info.grabInfo.disabled &&
		xf86Info.grabInfo.allowDeactivate) {
		if (inputInfo.pointer && inputInfo.pointer->grab != NULL &&
		    inputInfo.pointer->DeactivateGrab)
			(*inputInfo.pointer->DeactivateGrab)(inputInfo.pointer);
		if (inputInfo.keyboard && inputInfo.keyboard->grab != NULL &&
		    inputInfo.keyboard->DeactivateGrab)
			(*inputInfo.keyboard->DeactivateGrab)(inputInfo.keyboard);
	    }
	    break;

	case KEY_KP_Multiply:
	    if (!xf86Info.grabInfo.disabled &&
		xf86Info.grabInfo.allowClosedown) {
		ClientPtr pointer, keyboard, server;

		pointer = keyboard = server = NULL;
		if (inputInfo.pointer && inputInfo.pointer->grab != NULL)
		    pointer =
			clients[CLIENT_ID(inputInfo.pointer->grab->resource)];

		if (inputInfo.keyboard && inputInfo.keyboard->grab != NULL) {
		    keyboard =
			clients[CLIENT_ID(inputInfo.keyboard->grab->resource)];
		    if (keyboard == pointer)
			keyboard = NULL;
		}

	    if ((xf86Info.grabInfo.server.grabstate == SERVER_GRABBED) &&
		(((server = xf86Info.grabInfo.server.client) == pointer) ||
		 (server == keyboard)))
		server = NULL;

	    if (pointer)
		CloseDownClient(pointer);
	    if (keyboard)
		CloseDownClient(keyboard);
	    if (server)
		CloseDownClient(server);
	    }
	    break;
	
	case KEY_KP_Minus:	/* Keypad - */
	    if (!xf86Info.dontZoom) {
		if (down)
		    xf86ZoomViewport(xf86Info.currentScreen, -1);
		return;
	    }
	    break;

	case KEY_KP_Plus:	/* Keypad + */
	    if (!xf86Info.dontZoom) {
		if (down)
		    xf86ZoomViewport(xf86Info.currentScreen,  1);
		return;
	    }
	    break;
	}
    }

    /*
     * Now map the scancodes to real X-keycodes ...
     */
    if (keycode == KEY_NOTUSED) {
	xf86MsgVerb(X_INFO, 0,
	    "raw code %d mapped to KEY_NOTUSED -- please report\n", event->id);
	return;
    }
    if (keycode == KEY_UNKNOWN) {
	xf86MsgVerb(X_INFO, 0,
	    "raw code %d mapped to KEY_UNKNOWN -- please report\n", event->id);
	return;
    }
    keycode += MIN_KEYCODE;
    keysym = keyc->curKeySyms.map +
	     (keyc->curKeySyms.mapWidth *
	      (keycode - keyc->curKeySyms.minKeyCode));

#ifdef XKB
    if (noXkbExtension)
#endif
    {
	/*
	 * Toggle lock keys.
	 */
#define CAPSFLAG 0x01
#define NUMFLAG 0x02
#define SCROLLFLAG 0x04
#define MODEFLAG 0x08

	if (down) {
	    /*
	     * Handle the KeyPresses of the lock keys.
	     */

	    switch (keysym[0]) {

	    case XK_Caps_Lock:
		if (lockkeys & CAPSFLAG) {
		    lockkeys &= ~CAPSFLAG;
		    return;
		}
		lockkeys |= CAPSFLAG;
		updateLeds = TRUE;
		xf86Info.capsLock = down;
		break;

	    case XK_Num_Lock:
		if (lockkeys & NUMFLAG) {
		    lockkeys &= ~NUMFLAG;
		    return;
		}
		lockkeys |= NUMFLAG;
		updateLeds = TRUE;
		xf86Info.numLock = down;
		break;

	    case XK_Scroll_Lock:
		if (lockkeys & SCROLLFLAG) {
		    lockkeys &= ~SCROLLFLAG;
		    return;
		}
		lockkeys |= SCROLLFLAG;
		updateLeds = TRUE;
		xf86Info.scrollLock = down;
		break;
	    }
	} else {
	    /*
	     * Handle the releases of the lock keys.
	     */

	    switch (keysym[0]) {

	    case XK_Caps_Lock:
		if (lockkeys & CAPSFLAG)
		    return;
		updateLeds = TRUE;
		xf86Info.capsLock = down;
		break;

	    case XK_Num_Lock:
		if (lockkeys & NUMFLAG)
		    return;
		updateLeds = TRUE;
		xf86Info.numLock = down;
		break;

	    case XK_Scroll_Lock:
		if (lockkeys & SCROLLFLAG)
		    return;
		updateLeds = TRUE;
		xf86Info.scrollLock = down;
		break;
	    }
	}

  	if (updateLeds)
	    xf86KbdLeds();

	/*
	 * If this keycode is not a modifier key, and its down initiate the
	 * autorepeate sequence.  (Only necessary if not using XKB).
	 *
	 * If its not down, then reset the timer.
	 */
	if (!keyc->modifierMap[keycode]) {
	    if (down) {
		startautorepeat(keycode);
	    } else {
		TimerFree(sunTimer);
		sunTimer = NULL;
	    }
  	}
    }

    xf86Info.lastEventTime =
	kevent.u.keyButtonPointer.time =
	    GetTimeInMillis();

    /*
     * And now send these prefixes ...
     * NOTE: There cannot be multiple Mode_Switch keys !!!!
     */

    ENQUEUE(&kevent, keycode, (down ? KeyPress : KeyRelease), XE_KEYBOARD);
}

/*
 * Lets try reading more than one keyboard event at a time in the hopes that
 * this will be slightly more efficient.  Or we could just try the MicroSoft
 * method, and forget about efficiency. :-)
 */
void
xf86KbdEvents()
{
    Firm_event event[64];
    int        nBytes, i;

    /* I certainly hope its not possible to read partial events */

    if ((nBytes = read(xf86Info.kbdFd, (char *)event, sizeof(event))) > 0)
    {
	for (i = 0; i < (nBytes / sizeof(Firm_event)); i++)
	    sunPostKbdEvent(&event[i]);
    }
}

/*
 * Autorepeat stuff
 */

void
startautorepeat(long keycode)
{
    sunTimer = TimerSet(sunTimer, 		/* Timer */
			0, 			/* Flags */
			xf86Info.kbdDelay,	/* millis */
			processautorepeat,	/* callback */
			(pointer) keycode);	/* arg for timer */
}

CARD32
processautorepeat(OsTimerPtr timer, CARD32 now, pointer arg)
{
    xEvent kevent;
    int    keycode;

    keycode = (long)arg;

    xf86Info.lastEventTime =
	kevent.u.keyButtonPointer.time =
	    GetTimeInMillis();

    /*
     * Repeat a key by faking a KeyRelease, and a KeyPress event in rapid
     * succession
     */

    ENQUEUE(&kevent, keycode,  KeyRelease, XE_KEYBOARD);
    ENQUEUE(&kevent, keycode,  KeyPress, XE_KEYBOARD);

    /* And return the appropriate value so we get rescheduled */
    return xf86Info.kbdRate;
}
