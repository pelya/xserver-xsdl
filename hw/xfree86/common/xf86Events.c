/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Events.c,v 3.42.2.4 1998/02/07 09:23:28 hohndel Exp $ */
/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: xf86Events.c,v 1.3 2000/08/17 19:50:29 cpqbld Exp $ */

/* [JCH-96/01/21] Extended std reverse map to four buttons. */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "Xpoll.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"
#include "atKeynames.h"


#ifdef XFreeXDGA
#include "XIproto.h"
#include "extnsionst.h"
#include "scrnintstr.h"
#include "servermd.h"

#include "exevents.h"

#define _XF86DGA_SERVER_
#include "extensions/xf86dgastr.h"
#endif

#ifdef XINPUT
#include "XI.h"
#include "XIproto.h"
#include "xf86Xinput.h"
#endif

#include "mipointer.h"
#include "opaque.h"
#ifdef DPMSExtension
#include "extensions/dpms.h"
#endif

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

#define MOVEPOINTER(dx, dy, time) \
  if (on_steal_input) \
    XTestStealMotionData(dx, dy, XE_POINTER, xtest_mousex, xtest_mousey); \
  miPointerDeltaCursor (dx, dy, time)

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
#define MOVEPOINTER(dx, dy, time) \
  miPointerDeltaCursor (dx, dy, time)

#endif

Bool xf86VTSema = TRUE;

#ifdef	XINPUT
extern	InputInfo 	inputInfo;
#endif	/* XINPUT */

/*
 * The first of many hack's to get VT switching to work under
 * Solaris 2.1 for x86. The basic problem is that Solaris is supposed
 * to be SVR4. It is for the most part, except where the video interface
 * is concerned.  These hacks work around those problems.
 * See the comments for Linux, and SCO. 
 *
 * This is a toggleling variable:
 *  FALSE = No VT switching keys have been pressed last time around
 *  TRUE  = Possible VT switch Pending
 * (DWH - 12/2/93)
 *
 * This has been generalised to work with Linux and *BSD+syscons (DHD)
 */

#ifdef USE_VT_SYSREQ
static Bool VTSysreqToggle = FALSE;
#endif /* !USE_VT_SYSREQ */
static Bool VTSwitchEnabled = TRUE;   /* Allows run-time disabling for *BSD */

extern fd_set EnabledDevices;

#if defined(CODRV_SUPPORT)
extern unsigned char xf86CodrvMap[];
#endif

#if defined(XQUEUE) && !defined(XQUEUE_ASYNC)
extern void xf86XqueRequest(
#if NeedFunctionPrototypes
	void
#endif
	);
#endif

#ifdef DPMSExtension
extern BOOL DPMSEnabled;
extern void DPMSSet(CARD16);
#endif

static void xf86VTSwitch(
#if NeedFunctionPrototypes
	void
#endif
	);
#ifdef XFreeXDGA
static void XF86DirectVideoMoveMouse(
#if NeedFunctionPrototypes
	int x,
	int y,
	CARD32 mtime
#endif
	);
static void XF86DirectVideoKeyEvent(
#if NeedFunctionPrototypes
	xEvent *xE,
	int keycode,
	int etype
#endif
	);
#endif
static CARD32 buttonTimer(
#if NeedFunctionPrototypes
	OsTimerPtr timer,
	CARD32 now,
	pointer arg
#endif
     	);

/*
 * Lets create a simple finite-state machine:
 *
 *   state[?][0]: action1
 *   state[?][1]: action2
 *   state[?][2]: next state
 *
 *   action > 0: ButtonPress
 *   action = 0: nothing
 *   action < 0: ButtonRelease
 *
 * Why this stuff ??? Normally you cannot press both mousebuttons together, so
 * the mouse reports both pressed at the same time ...
 */

static char stateTab[48][3] = {

/* nothing pressed */
  {  0,  0,  0 },	
  {  0,  0,  8 },	/* 1 right -> delayed right */
  {  0,  0,  0 },       /* 2 nothing */
  {  0,  0,  8 },	/* 3 right -> delayed right */
  {  0,  0, 16 },	/* 4 left -> delayed left */
  {  2,  0, 24 },       /* 5 left & right (middle press) -> middle pressed */
  {  0,  0, 16 },	/* 6 left -> delayed left */
  {  2,  0, 24 },       /* 7 left & right (middle press) -> middle pressed */

/* delayed right */
  {  1, -1,  0 },	/* 8 nothing (right event) -> init */
  {  1,  0, 32 },       /* 9 right (right press) -> right pressed */
  {  1, -1,  0 },	/* 10 nothing (right event) -> init */
  {  1,  0, 32 },       /* 11 right (right press) -> right pressed */
  {  1, -1, 16 },       /* 12 left (right event) -> delayed left */
  {  2,  0, 24 },       /* 13 left & right (middle press) -> middle pressed */
  {  1, -1, 16 },       /* 14 left (right event) -> delayed left */
  {  2,  0, 24 },       /* 15 left & right (middle press) -> middle pressed */

/* delayed left */
  {  3, -3,  0 },	/* 16 nothing (left event) -> init */
  {  3, -3,  8 },       /* 17 right (left event) -> delayed right */
  {  3, -3,  0 },	/* 18 nothing (left event) -> init */
  {  3, -3,  8 },       /* 19 right (left event) -> delayed right */
  {  3,  0, 40 },	/* 20 left (left press) -> pressed left */
  {  2,  0, 24 },	/* 21 left & right (middle press) -> pressed middle */
  {  3,  0, 40 },	/* 22 left (left press) -> pressed left */
  {  2,  0, 24 },	/* 23 left & right (middle press) -> pressed middle */

/* pressed middle */
  { -2,  0,  0 },	/* 24 nothing (middle release) -> init */
  { -2,  0,  0 },	/* 25 right (middle release) -> init */
  { -2,  0,  0 },	/* 26 nothing (middle release) -> init */
  { -2,  0,  0 },	/* 27 right (middle release) -> init */
  { -2,  0,  0 },	/* 28 left (middle release) -> init */
  {  0,  0, 24 },	/* 29 left & right -> pressed middle */
  { -2,  0,  0 },	/* 30 left (middle release) -> init */
  {  0,  0, 24 },	/* 31 left & right -> pressed middle */

/* pressed right */
  { -1,  0,  0 },	/* 32 nothing (right release) -> init */
  {  0,  0, 32 },	/* 33 right -> pressed right */
  { -1,  0,  0 },	/* 34 nothing (right release) -> init */
  {  0,  0, 32 },	/* 35 right -> pressed right */
  { -1,  0, 16 },	/* 36 left (right release) -> delayed left */
  { -1,  2, 24 },	/* 37 left & right (r rel, m prs) -> middle pressed */
  { -1,  0, 16 },	/* 38 left (right release) -> delayed left */
  { -1,  2, 24 },	/* 39 left & right (r rel, m prs) -> middle pressed */

/* pressed left */
  { -3,  0,  0 },	/* 40 nothing (left release) -> init */
  { -3,  0,  8 },	/* 41 right (left release) -> delayed right */
  { -3,  0,  0 },	/* 42 nothing (left release) -> init */
  { -3,  0,  8 },	/* 43 right (left release) -> delayed right */
  {  0,  0, 40 },	/* 44 left -> left pressed */
  { -3,  2, 24 },	/* 45 left & right (l rel, mprs) -> middle pressed */
  {  0,  0, 40 },	/* 46 left -> left pressed */
  { -3,  2, 24 },	/* 47 left & right (l rel, mprs) -> middle pressed */
};


/*
 * Table to allow quick reversal of natural button mapping to correct mapping
 */

/*
 * [JCH-96/01/21] The ALPS GlidePoint pad extends the MS protocol
 * with a fourth button activated by tapping the PAD.
 * The 2nd line corresponds to 4th button on; the drv sends
 * the buttons in the following map (MSBit described first) :
 * 0 | 4th | 1st | 2nd | 3rd
 * And we remap them (MSBit described first) :
 * 0 | 4th | 3rd | 2nd | 1st
 */
static char reverseMap[32] = { 0,  4,  2,  6,  1,  5,  3,  7,
			       8, 12, 10, 14,  9, 13, 11, 15,
			      16, 20, 18, 22, 17, 21, 19, 23,
			      24, 28, 26, 30, 25, 29, 27, 31};


static char hitachMap[16] = {  0,  2,  1,  3, 
			       8, 10,  9, 11,
			       4,  6,  5,  7,
			      12, 14, 13, 15 };

#define reverseBits(map, b)	(((b) & ~0x0f) | map[(b) & 0x0f])


/*
 * TimeSinceLastInputEvent --
 *      Function used for screensaver purposes by the os module. Retruns the
 *      time in milliseconds since there last was any input.
 */

int
TimeSinceLastInputEvent()
{
  if (xf86Info.lastEventTime == 0) {
    xf86Info.lastEventTime = GetTimeInMillis();
  }
  return GetTimeInMillis() - xf86Info.lastEventTime;
}



/*
 * SetTimeSinceLastInputEvent --
 *      Set the lastEventTime to now.
 */

void
SetTimeSinceLastInputEvent()
{
  xf86Info.lastEventTime = GetTimeInMillis();
}



/*
 * ProcessInputEvents --
 *      Retrieve all waiting input events and pass them to DIX in their
 *      correct chronological order. Only reads from the system pointer
 *      and keyboard.
 */

void
ProcessInputEvents ()
{
  int x, y;
#ifdef INHERIT_LOCK_STATE
  static int generation = 0;
#endif

#ifdef AMOEBA
#define MAXEVENTS	    32
#define BUTTON_PRESS	    0x1000
#define MAP_BUTTON(ev,but)  (((ev) == EV_ButtonPress) ? \
			     ((but) | BUTTON_PRESS) : ((but) & ~BUTTON_PRESS))
#define KEY_RELEASE	    0x80
#define MAP_KEY(ev, key)    (((ev) == EV_KeyReleaseEvent) ? \
			     ((key) | KEY_RELEASE) : ((key) & ~KEY_RELEASE))

    register IOPEvent  *e, *elast;
    IOPEvent		events[MAXEVENTS];
    int			dx, dy, nevents;
#endif

    /*
     * With INHERIT_LOCK_STATE defined, the initial state of CapsLock, NumLock
     * and ScrollLock will be set to match that of the VT the server is
     * running on.
     */
#ifdef INHERIT_LOCK_STATE
    if (generation != serverGeneration) {
      xEvent kevent;
      DevicePtr pKeyboard = xf86Info.pKeyboard;
      extern unsigned int xf86InitialCaps, xf86InitialNum, xf86InitialScroll;

      generation = serverGeneration;
      kevent.u.keyButtonPointer.time = GetTimeInMillis();
      kevent.u.keyButtonPointer.rootX = 0;
      kevent.u.keyButtonPointer.rootY = 0;
      kevent.u.u.type = KeyPress;


      if (xf86InitialCaps) {
        kevent.u.u.detail = xf86InitialCaps;
        (* pKeyboard->processInputProc)(&kevent, (DeviceIntPtr)pKeyboard, 1);
        xf86InitialCaps = 0;
      }
      if (xf86InitialNum) {
        kevent.u.u.detail = xf86InitialNum;
        (* pKeyboard->processInputProc)(&kevent, (DeviceIntPtr)pKeyboard, 1);
        xf86InitialNum = 0;
      }
      if (xf86InitialScroll) {
        kevent.u.u.detail = xf86InitialScroll;
        (* pKeyboard->processInputProc)(&kevent, (DeviceIntPtr)pKeyboard, 1);
        xf86InitialScroll = 0;
      }
    }
#endif

#ifdef AMOEBA
    /*
     * Get all events from the IOP server
     */
    while ((nevents = AmoebaGetEvents(events, MAXEVENTS)) > 0) {
      for (e = &events[0], elast = &events[nevents]; e < elast; e++) {
          xf86Info.lastEventTime = e->time;
          switch (e->type) {
          case EV_PointerDelta:
	      if (e->x != 0 || e->y != 0) {
                  xf86PostMseEvent(&xf86Info.pMouse, 0, e->x, e->y);
	      }
              break;
          case EV_ButtonPress:
          case EV_ButtonRelease:
              xf86PostMseEvent(&xf86Info.pMouse, MAP_BUTTON(e->type, e->keyorbut), 0, 0);
              break;
          case EV_KeyPressEvent:
          case EV_KeyReleaseEvent:
              xf86PostKbdEvent(MAP_KEY(e->type, e->keyorbut));
              break;
          default:
              /* this shouldn't happen */
              ErrorF("stray event %d (%d,%d) %x\n",
                      e->type, e->x, e->y, e->keyorbut);
              break;
          }
      }
    }
#endif

  xf86Info.inputPending = FALSE;

#ifdef XINPUT
  xf86eqProcessInputEvents();
#else
  mieqProcessInputEvents();
#endif
  miPointerUpdate();

  miPointerPosition(&x, &y);
  xf86SetViewport(xf86Info.currentScreen, x, y);
}



/*
 * xf86PostKbdEvent --
 *	Translate the raw hardware KbdEvent into an XEvent, and tell DIX
 *	about it. Scancode preprocessing and so on is done ...
 *
 *  OS/2 specific xf86PostKbdEvent(key) has been moved to os-support/os2/os2_kbd.c
 *  as some things differ, and I did not want to scatter this routine with
 *  ifdefs further (hv).
 */

#ifdef ASSUME_CUSTOM_KEYCODES
extern u_char SpecialServerMap[];
#endif /* ASSUME_CUSTOM_KEYCODES */

#if !defined(__EMX__)
void
xf86PostKbdEvent(key)
     unsigned key;
{
  int         scanCode = (key & 0x7f);
  int         specialkey;
  Bool        down = (key & 0x80 ? FALSE : TRUE);
  KeyClassRec *keyc = ((DeviceIntPtr)xf86Info.pKeyboard)->key;
  Bool        updateLeds = FALSE;
  Bool        UsePrefix = FALSE;
  Bool        Direction = FALSE;
  xEvent      kevent;
  KeySym      *keysym;
  int         keycode;
  static int  lockkeys = 0;
#if defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)
  static Bool first_time = TRUE;
#endif

#if defined(CODRV_SUPPORT)
  if (xf86Info.consType == CODRV011 || xf86Info.consType == CODRV01X)
    scanCode = xf86CodrvMap[scanCode];
#endif

#if defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)
  if (first_time)
  {
    first_time = FALSE;
    VTSwitchEnabled = (xf86Info.consType == SYSCONS)
	    || (xf86Info.consType == PCVT);
  }
#endif

#if defined (i386) && defined (SVR4) && !defined (PC98)
    /* 
     * PANIX returns DICOP standards based keycodes in using 106jp 
     * keyboard. We need to remap some keys. 
     */
#define KEY_P_UP	0x5A
#define KEY_P_PGUP	0x5B
#define KEY_P_LEFT	0x5C
#define KEY_P_BKSL	0x73
#define KEY_P_YEN	0x7D
#define KEY_P_NFER	0x7B
#define KEY_P_XFER	0x79

  if(xf86Info.panix106 == TRUE){
    switch (scanCode) {
    /* case 0x78:        scanCode = KEY_P_UP;     break;   not needed*/
    case 0x56:        scanCode = KEY_P_BKSL;   break;  /* Backslash */
    case 0x5A:        scanCode = KEY_P_NFER;   break;  /* No Kanji Transfer*/
    case 0x5B:        scanCode = KEY_P_XFER;   break;  /* Kanji Tranfer */
    case 0x5C:        scanCode = KEY_P_YEN;    break;  /* Yen curs pgup */
    case 0x6B:        scanCode = KEY_P_LEFT;   break;  /* Cur Left */
    case 0x6F:        scanCode = KEY_P_PGUP;   break;  /* Cur PageUp */
    case 0x72:        scanCode = KEY_AltLang;  break;  /* AltLang(right) */
    case 0x73:        scanCode = KEY_RCtrl;    break;  /* not needed */
    }
  }
#endif  /* i386 && SVR4 */

#ifndef ASSUME_CUSTOM_KEYCODES
  /*
   * First do some special scancode remapping ...
   */
  if (xf86Info.scanPrefix == 0) {

    switch (scanCode) {
      
#ifndef PC98
    case KEY_Prefix0:
    case KEY_Prefix1:
#if defined(PCCONS_SUPPORT) || defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)
      if (xf86Info.consType == PCCONS || xf86Info.consType == SYSCONS
	  || xf86Info.consType == PCVT) {
#endif
        xf86Info.scanPrefix = scanCode;  /* special prefixes */
        return;
#if defined(PCCONS_SUPPORT) || defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)
      }
      break;
#endif
#endif /* not PC98 */
    }
#ifndef PC98
    if (xf86Info.serverNumLock) {
     if ((!xf86Info.numLock && ModifierDown(ShiftMask)) ||
         (xf86Info.numLock && !ModifierDown(ShiftMask))) {
      /*
       * Hardwired numlock handling ... (Some applications break if they have
       * these keys double defined, like twm)
       */
      switch (scanCode) {
      case KEY_KP_7:        scanCode = KEY_SN_KP_7;   break;  /* curs 7 */
      case KEY_KP_8:        scanCode = KEY_SN_KP_8;   break;  /* curs 8 */
      case KEY_KP_9:        scanCode = KEY_SN_KP_9;   break;  /* curs 9 */
      case KEY_KP_4:        scanCode = KEY_SN_KP_4;   break;  /* curs 4 */
      case KEY_KP_5:        scanCode = KEY_SN_KP_5;   break;  /* curs 5 */
      case KEY_KP_6:        scanCode = KEY_SN_KP_6;   break;  /* curs 6 */
      case KEY_KP_1:        scanCode = KEY_SN_KP_1;   break;  /* curs 1 */
      case KEY_KP_2:        scanCode = KEY_SN_KP_2;   break;  /* curs 2 */
      case KEY_KP_3:        scanCode = KEY_SN_KP_3;   break;  /* curs 3 */
      case KEY_KP_0:        scanCode = KEY_SN_KP_0;   break;  /* curs 0 */
      case KEY_KP_Decimal:  scanCode = KEY_SN_KP_Dec; break;  /* curs decimal */
      }
     } else {
      switch (scanCode) {
      case KEY_KP_7:        scanCode = KEY_SN_KP_Home;  break;  /* curs home */
      case KEY_KP_8:        scanCode = KEY_SN_KP_Up  ;  break;  /* curs up */
      case KEY_KP_9:        scanCode = KEY_SN_KP_Prior; break;  /* curs pgup */
      case KEY_KP_4:        scanCode = KEY_SN_KP_Left;  break;  /* curs left */
      case KEY_KP_5:        scanCode = KEY_SN_KP_Begin; break;  /* curs begin */
      case KEY_KP_6:        scanCode = KEY_SN_KP_Right; break;  /* curs right */
      case KEY_KP_1:        scanCode = KEY_SN_KP_End;   break;  /* curs end */
      case KEY_KP_2:        scanCode = KEY_SN_KP_Down;  break;  /* curs down */
      case KEY_KP_3:        scanCode = KEY_SN_KP_Next;  break;  /* curs pgdn */
      case KEY_KP_0:        scanCode = KEY_SN_KP_Ins;   break;  /* curs ins */
      case KEY_KP_Decimal:  scanCode = KEY_SN_KP_Del;   break;  /* curs del */
      }
     }
    }
#endif /* not PC98 */
  }

#ifndef PC98
  else if (
#ifdef CSRG_BASED
           (xf86Info.consType == PCCONS || xf86Info.consType == SYSCONS
	    || xf86Info.consType == PCVT) &&
#endif
           (xf86Info.scanPrefix == KEY_Prefix0)) {
    xf86Info.scanPrefix = 0;
	  
    switch (scanCode) {
    case KEY_KP_7:        scanCode = KEY_Home;      break;  /* curs home */
    case KEY_KP_8:        scanCode = KEY_Up;        break;  /* curs up */
    case KEY_KP_9:        scanCode = KEY_PgUp;      break;  /* curs pgup */
    case KEY_KP_4:        scanCode = KEY_Left;      break;  /* curs left */
    case KEY_KP_5:        scanCode = KEY_Begin;     break;  /* curs begin */
    case KEY_KP_6:        scanCode = KEY_Right;     break;  /* curs right */
    case KEY_KP_1:        scanCode = KEY_End;       break;  /* curs end */
    case KEY_KP_2:        scanCode = KEY_Down;      break;  /* curs down */
    case KEY_KP_3:        scanCode = KEY_PgDown;    break;  /* curs pgdown */
    case KEY_KP_0:        scanCode = KEY_Insert;    break;  /* curs insert */
    case KEY_KP_Decimal:  scanCode = KEY_Delete;    break;  /* curs delete */
    case KEY_Enter:       scanCode = KEY_KP_Enter;  break;  /* keypad enter */
    case KEY_LCtrl:       scanCode = KEY_RCtrl;     break;  /* right ctrl */
    case KEY_KP_Multiply: scanCode = KEY_Print;     break;  /* print */
    case KEY_Slash:       scanCode = KEY_KP_Divide; break;  /* keyp divide */
    case KEY_Alt:         scanCode = KEY_AltLang;   break;  /* right alt */
    case KEY_ScrollLock:  scanCode = KEY_Break;     break;  /* curs break */
    case 0x5b:            scanCode = KEY_LMeta;     break;
    case 0x5c:            scanCode = KEY_RMeta;     break;
    case 0x5d:            scanCode = KEY_Menu;      break;
    case KEY_F3:          scanCode = KEY_F13;       break;
    case KEY_F4:          scanCode = KEY_F14;       break;
    case KEY_F5:          scanCode = KEY_F15;       break;
    case KEY_F6:          scanCode = KEY_F16;       break;
    case KEY_F7:          scanCode = KEY_F17;       break;
    case KEY_KP_Plus:     scanCode = KEY_KP_DEC;    break;
      /*
       * Ignore virtual shifts (E0 2A, E0 AA, E0 36, E0 B6)
       */
    default:
      return;                                  /* skip illegal */
    }
  }
  
  else if (xf86Info.scanPrefix == KEY_Prefix1)
    {
      xf86Info.scanPrefix = (scanCode == KEY_LCtrl) ? KEY_LCtrl : 0;
      return;
    }
  
  else if (xf86Info.scanPrefix == KEY_LCtrl)
    {
      xf86Info.scanPrefix = 0;
      if (scanCode != KEY_NumLock) return;
      scanCode = KEY_Pause;       /* pause */
    }
#endif /* not PC98 */  
#endif /* !ASSUME_CUSTOM_KEYCODES */

  /*
   * and now get some special keysequences
   */

#ifdef ASSUME_CUSTOM_KEYCODES
  specialkey = SpecialServerMap[scanCode];
#else /* ASSUME_CUSTOM_KEYCODES */
  specialkey = scanCode;
#endif /* ASSUME_CUSTOM_KEYCODES */

  if ((ModifierDown(ControlMask | AltMask)) ||
      (ModifierDown(ControlMask | AltLangMask)))
    {
      
      switch (specialkey) {
	
      case KEY_BackSpace:
	if (!xf86Info.dontZap) {
#ifdef XFreeXDGA
  if (((ScrnInfoPtr)(xf86Info.currentScreen->devPrivates[xf86ScreenIndex].ptr))->directMode&XF86DGADirectGraphics) 
	break;
#endif
	 GiveUp(0);
        }
	break;
	
	/*
	 * The idea here is to pass the scancode down to a list of
	 * registered routines. There should be some standard conventions
	 * for processing certain keys.
	 */
      case KEY_KP_Minus:   /* Keypad - */
	if (!xf86Info.dontZoom) {
	  if (down) xf86ZoomViewport(xf86Info.currentScreen, -1);
	  return;
	}
	break;
	
      case KEY_KP_Plus:   /* Keypad + */
	if (!xf86Info.dontZoom) {
	  if (down) xf86ZoomViewport(xf86Info.currentScreen,  1);
	  return;
	}
	break;

#if defined(linux) || (defined(CSRG_BASED) && (defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT))) || defined(SCO)
	/*
	 * Under Linux, the raw keycodes are consumed before the kernel
	 * does any processing on them, so we must emulate the vt switching
	 * we want ourselves.
	 */
      case KEY_F1:
      case KEY_F2:
      case KEY_F3:
      case KEY_F4:
      case KEY_F5:
      case KEY_F6:
      case KEY_F7:
      case KEY_F8:
      case KEY_F9:
      case KEY_F10:
        if (VTSwitchEnabled && !xf86Info.vtSysreq
#if (defined(CSRG_BASED) && (defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)))
	    && (xf86Info.consType == SYSCONS || xf86Info.consType == PCVT)
#endif
	    )
        {
	  if (down)
#ifdef SCO325
            ioctl(xf86Info.consoleFd, VT_ACTIVATE, specialkey - KEY_F1);
#else
            ioctl(xf86Info.consoleFd, VT_ACTIVATE, specialkey - KEY_F1 + 1);
#endif
          return;
        }
	break;
      case KEY_F11:
      case KEY_F12:
        if (VTSwitchEnabled && !xf86Info.vtSysreq
#if (defined(CSRG_BASED) && (defined(SYSCONS_SUPPORT) || defined(PCVT_SUPPORT)))
	    && (xf86Info.consType == SYSCONS || xf86Info.consType == PCVT)
#endif
	    )
        {
	  if (down)
#ifdef SCO325
            ioctl(xf86Info.consoleFd, VT_ACTIVATE, specialkey - KEY_F11 + 10);
#else
            ioctl(xf86Info.consoleFd, VT_ACTIVATE, specialkey - KEY_F11 + 11);
#endif
          return;
        }
	break;
#endif /* linux || BSD with VTs */

      /* just worth mentioning here: any 386bsd keyboard driver
       * (pccons.c or co_kbd.c) catches CTRL-ALT-DEL and CTRL-ALT-ESC
       * before any application (e.g. XF86) will see it
       * OBS: syscons does not, nor does pcvt !
       */
      } 
    }

    /*
     * Start of actual Solaris VT switching code.  
     * This should pretty much emulate standard SVR4 switching keys.
     * 
     * DWH 12/2/93
     */

#ifdef USE_VT_SYSREQ
    if (VTSwitchEnabled && xf86Info.vtSysreq)
    {
      switch (specialkey)
      {
      /*
       * syscons on *BSD doesn't have a VT #0  -- don't think Linux does
       * either
       */
#if defined (sun) && defined (i386) && defined (SVR4)
      case KEY_H: 
	if (VTSysreqToggle && down)
        {
          ioctl(xf86Info.consoleFd, VT_ACTIVATE, 0);
          VTSysreqToggle = 0;
          return; 
        }
	break;

      /*
       * Yah, I know the N, and P keys seem backwards, however that's
       * how they work under Solaris
       * XXXX N means go to next active VT not necessarily vtno+1 (or vtno-1)
       */

      case KEY_N:
	if (VTSysreqToggle && down)
	{
          if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno - 1 ) < 0)
            ErrorF("Failed to switch consoles (%s)\n", strerror(errno));
          VTSysreqToggle = FALSE;
          return;
        }
	break;

      case KEY_P:
	if (VTSysreqToggle && down)
	{
          if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno + 1 ) < 0)
            if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, 0) < 0)
              ErrorF("Failed to switch consoles (%s)\n", strerror(errno));
          VTSysreqToggle = FALSE;
          return;
        }
	break;
#endif

      case KEY_F1:
      case KEY_F2:
      case KEY_F3:
      case KEY_F4:
      case KEY_F5:
      case KEY_F6:
      case KEY_F7:
      case KEY_F8:
      case KEY_F9:
      case KEY_F10:
	if (VTSysreqToggle && down)
	{
          if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, specialkey-KEY_F1 + 1) < 0)
            ErrorF("Failed to switch consoles (%s)\n", strerror(errno));
          VTSysreqToggle = FALSE;
          return;
        }
	break;

      case KEY_F11:
      case KEY_F12:
	if (VTSysreqToggle && down)
	{
          if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, specialkey-KEY_F11 + 11) < 0)
            ErrorF("Failed to switch consoles (%s)\n", strerror(errno));
          VTSysreqToggle = FALSE;
          return;
        }
	break;

      /* Ignore these keys -- ie don't let them cancel an alt-sysreq */
      case KEY_Alt:
#ifndef PC98
      case KEY_AltLang:
#endif /* not PC98 */
	break;

#ifndef PC98
      case KEY_SysReqest:
        if (down && (ModifierDown(AltMask) || ModifierDown(AltLangMask)))
          VTSysreqToggle = TRUE;
	break;
#endif /* not PC98 */

      default:
        if (VTSysreqToggle)
	{
	  /*
	   * We only land here when Alt-SysReq is followed by a
	   * non-switching key.
	   */
          VTSysreqToggle = FALSE;

        }
      }
    }

#endif /* USE_VT_SYSREQ */

#ifdef SCO
    /*
     *	With the console in raw mode, SCO will not switch consoles,
     *	you get around this by activating the next console along, if
     *	this fails then go back to console 0, if there is only one
     *	then it doesn't matter, switching to yourself is a nop as far
     *	as the console driver is concerned.
     *	We could do something similar to linux here but SCO ODT uses
     *	Ctrl-PrintScrn, so why change?
     */
    if (specialkey == KEY_Print && ModifierDown(ControlMask)) {
      if (down)
        if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, xf86Info.vtno + 1) < 0)
          if (ioctl(xf86Info.consoleFd, VT_ACTIVATE, 0) < 0)
            ErrorF("Failed to switch consoles (%s)\n", strerror(errno));
      return;
    }
#endif /* SCO */

  /*
   * Now map the scancodes to real X-keycodes ...
   */
  keycode = scanCode + MIN_KEYCODE;
  keysym = (keyc->curKeySyms.map +
	    keyc->curKeySyms.mapWidth * 
	    (keycode - keyc->curKeySyms.minKeyCode));
#ifdef XKB
  if (noXkbExtension) {
#endif
  /*
   * Filter autorepeated caps/num/scroll lock keycodes.
   */
#define CAPSFLAG 0x01
#define NUMFLAG 0x02
#define SCROLLFLAG 0x04
#define MODEFLAG 0x08
  if( down ) {
    switch( keysym[0] ) {
        case XK_Caps_Lock :
          if (lockkeys & CAPSFLAG)
              return;
	  else
	      lockkeys |= CAPSFLAG;
          break;

        case XK_Num_Lock :
          if (lockkeys & NUMFLAG)
              return;
	  else
	      lockkeys |= NUMFLAG;
          break;

        case XK_Scroll_Lock :
          if (lockkeys & SCROLLFLAG)
              return;
	  else
	      lockkeys |= SCROLLFLAG;
          break;
    }
    if (keysym[1] == XF86XK_ModeLock)
    {
      if (lockkeys & MODEFLAG)
          return;
      else
          lockkeys |= MODEFLAG;
    }
      
  }
  else {
    switch( keysym[0] ) {
        case XK_Caps_Lock :
            lockkeys &= ~CAPSFLAG;
            break;

        case XK_Num_Lock :
            lockkeys &= ~NUMFLAG;
            break;

        case XK_Scroll_Lock :
            lockkeys &= ~SCROLLFLAG;
            break;
    }
    if (keysym[1] == XF86XK_ModeLock)
      lockkeys &= ~MODEFLAG;
  }

  /*
   * LockKey special handling:
   * ignore releases, toggle on & off on presses.
   * Don't deal with the Caps_Lock keysym directly, but check the lock modifier
   */
#ifndef PC98
  if (keyc->modifierMap[keycode] & LockMask ||
      keysym[0] == XK_Scroll_Lock ||
      keysym[1] == XF86XK_ModeLock ||
      keysym[0] == XK_Num_Lock)
    {
      Bool flag;

      if (!down) return;
      if (KeyPressed(keycode)) {
	down = !down;
	flag = FALSE;
      }
      else
	flag = TRUE;

      if (keyc->modifierMap[keycode] & LockMask)   xf86Info.capsLock   = flag;
      if (keysym[0] == XK_Num_Lock)    xf86Info.numLock    = flag;
      if (keysym[0] == XK_Scroll_Lock) xf86Info.scrollLock = flag;
      if (keysym[1] == XF86XK_ModeLock)   xf86Info.modeSwitchLock = flag;
      updateLeds = TRUE;
    }
#endif /* not PC98 */	

#ifndef ASSUME_CUSTOM_KEYCODES
  /*
   * normal, non-keypad keys
   */
  if (scanCode < KEY_KP_7 || scanCode > KEY_KP_Decimal) {
#if !defined(CSRG_BASED) && !defined(MACH386) && !defined(MINIX) && !defined(__OSF__)
    /*
     * magic ALT_L key on AT84 keyboards for multilingual support
     */
    if (xf86Info.kbdType == KB_84 &&
	ModifierDown(AltMask) &&
	keysym[2] != NoSymbol)
      {
	UsePrefix = TRUE;
	Direction = TRUE;
      }
#endif /* !CSRG_BASED && !MACH386 && !MINIX && !__OSF__ */
  }
#endif /* !ASSUME_CUSTOM_KEYCODES */
  if (updateLeds) xf86KbdLeds();
#ifdef XKB
  }
#endif

  /*
   * check for an autorepeat-event
   */
  if ((down && KeyPressed(keycode)) &&
      (xf86Info.autoRepeat != AutoRepeatModeOn || keyc->modifierMap[keycode]))
    return;

  xf86Info.lastEventTime = kevent.u.keyButtonPointer.time = GetTimeInMillis();
  /*
   * And now send these prefixes ...
   * NOTE: There cannot be multiple Mode_Switch keys !!!!
   */
  if (UsePrefix)
    {
      ENQUEUE(&kevent,
	      keyc->modifierKeyMap[keyc->maxKeysPerModifier*7],
	      (Direction ? KeyPress : KeyRelease),
	      XE_KEYBOARD);
      ENQUEUE(&kevent, keycode, (down ? KeyPress : KeyRelease), XE_KEYBOARD);
      ENQUEUE(&kevent,
	      keyc->modifierKeyMap[keyc->maxKeysPerModifier*7],
	      (Direction ? KeyRelease : KeyPress),
	      XE_KEYBOARD);
    }
  else 
    {
#ifdef XFreeXDGA
      if (((ScrnInfoPtr)(xf86Info.currentScreen->devPrivates[xf86ScreenIndex].ptr))->directMode&XF86DGADirectKeyb) {
	  XF86DirectVideoKeyEvent(&kevent, keycode, (down ? KeyPress : KeyRelease));
      } else
#endif
      {
         ENQUEUE(&kevent, keycode, (down ? KeyPress : KeyRelease), XE_KEYBOARD);

      }
    }
}
#endif /* !__EMX__ */


static CARD32
buttonTimer(timer, now, arg)
     OsTimerPtr timer;
     CARD32 now;
     pointer arg;
{
    MouseDevPtr	priv = MOUSE_DEV((DeviceIntPtr) arg);

    xf86PostMseEvent(((DeviceIntPtr) arg), priv->truebuttons, 0, 0);
    return(0);
}


/*      
 * xf86PostMseEvent --
 *	Translate the raw hardware MseEvent into an XEvent(s), and tell DIX
 *	about it. Perform a 3Button emulation if required.
 */

void
xf86PostMseEvent(device, buttons, dx, dy)
    DeviceIntPtr device;
    int buttons, dx, dy;
{
  static OsTimerPtr timer = NULL;
  MouseDevPtr private = MOUSE_DEV(device);
  int         id, change;
  int         truebuttons;
  xEvent      mevent[2];
#ifdef XINPUT
  deviceKeyButtonPointer	*xev = (deviceKeyButtonPointer *) mevent;
  deviceValuator		*xv = (deviceValuator *) (xev+1);
  int				is_pointer; /* the mouse is the pointer ? */
#endif

#ifdef AMOEBA
  int	      pressed;

  pressed = ((buttons & BUTTON_PRESS) != 0);
  buttons &= ~BUTTON_PRESS;
#endif

#ifdef XINPUT
  is_pointer = xf86IsCorePointer(device);

  if (!is_pointer) {
    xev->time = xf86Info.lastEventTime = GetTimeInMillis();
  }
  else
#endif
  xf86Info.lastEventTime = mevent->u.keyButtonPointer.time = GetTimeInMillis();

  truebuttons = buttons;
  if (private->mseType == P_MMHIT)
    buttons = reverseBits(hitachMap, buttons);
  else
    buttons = reverseBits(reverseMap, buttons);

  if (dx || dy) {
    
    /*
     * accelerate the baby now if sqrt(dx*dx + dy*dy) > threshold !
     * but do some simpler arithmetic here...
     */
    if ((abs(dx) + abs(dy)) >= private->threshold) {
      dx = (dx * private->num) / private->den;
      dy = (dy * private->num)/ private->den;
    }

#ifdef XINPUT
    if (is_pointer) {
#endif
#ifdef XFreeXDGA
      if (((ScrnInfoPtr)(xf86Info.currentScreen->devPrivates[xf86ScreenIndex].ptr))->directMode&XF86DGADirectMouse) {
	XF86DirectVideoMoveMouse(dx, dy, mevent->u.keyButtonPointer.time);
      } else
#endif
	{
	  MOVEPOINTER(dx, dy, mevent->u.keyButtonPointer.time);
	}
#ifdef XINPUT
    }
    else {
      xev->type = DeviceMotionNotify;
      xev->deviceid = device->id | MORE_EVENTS;
      xv->type = DeviceValuator;
      xv->deviceid = device->id;
      xv->num_valuators = 2;
      xv->first_valuator = 0;
      xv->device_state = 0;
      xv->valuator0 = dx;
      xv->valuator1 = dy;
      xf86eqEnqueue(mevent);
    }
#endif
  }

  if (private->emulate3Buttons)
    {

      /*
       * Hack to operate the middle button even with Emulate3Buttons set.
       * Modifying the state table to keep track of the middle button state
       * would nearly double its size, so I'll stick with this fix.  - TJW
       */
      if (private->mseType == P_MMHIT)
        change = buttons ^ reverseBits(hitachMap, private->lastButtons);
      else
        change = buttons ^ reverseBits(reverseMap, private->lastButtons);
      if (change & 02)
	{
#ifdef XINPUT
	    if (xf86CheckButton(2, (buttons & 02))) {
#endif
	  ENQUEUE(mevent,
		  2, (buttons & 02) ? ButtonPress : ButtonRelease,
		  XE_POINTER);
#ifdef XINPUT
	    }
#endif
	}
      
      /*
       * emulate the third button by the other two
       */
      if ((id = stateTab[buttons + private->emulateState][0]) != 0)
	{
#ifdef XINPUT
          if (is_pointer) {
	      if (xf86CheckButton(abs(id), (id >= 0))) {
#endif
            ENQUEUE(mevent,
                    abs(id), (id < 0 ? ButtonRelease : ButtonPress), 
                    XE_POINTER);
#ifdef XINPUT
	      }
          }
          else {
            xev->type = (id < 0 ? DeviceButtonRelease : DeviceButtonPress);
            xev->deviceid = device->id | MORE_EVENTS;
	    xev->detail = abs(id);
            xv->type = DeviceValuator;
            xv->deviceid = device->id;
            xv->num_valuators = 0;
            xv->device_state = 0;
            xf86eqEnqueue(mevent);
          }
#endif 
	}

      if ((id = stateTab[buttons + private->emulateState][1]) != 0)
	{
#ifdef XINPUT
	  if (is_pointer) {
	    if (xf86CheckButton(abs(id), (id >= 0))) {
#endif
            ENQUEUE(mevent,
                    abs(id), (id < 0 ? ButtonRelease : ButtonPress), 
                    XE_POINTER);
#ifdef XINPUT
	    }
          }
          else {
            xev->type = (id < 0 ? DeviceButtonRelease : DeviceButtonPress);
            xev->deviceid = device->id | MORE_EVENTS;
	    xev->detail = abs(id);
            xv->type = DeviceValuator;
            xv->deviceid = device->id;
            xv->num_valuators = 0;
            xv->device_state = 0;
            xf86eqEnqueue(mevent);
          }   
#endif
	}

      private->emulateState = stateTab[buttons + private->emulateState][2];
      if (stateTab[buttons + private->emulateState][0] ||
          stateTab[buttons + private->emulateState][1])
        {
	    private->truebuttons = truebuttons;
	    timer = TimerSet(timer, 0, private->emulate3Timeout, buttonTimer,
			     (pointer)device);
        }
      else
        {
          if (timer)
            {
              TimerFree(timer);
              timer = NULL;
            }
        }
    }
  else
    {
#ifdef AMOEBA
      if (truebuttons != 0) {
#ifdef XINPUT
          if (is_pointer) {
	    if (xf86CheckButton(truebuttons)) {
#endif
	    ENQUEUE(mevent,
		    truebuttons, (pressed ? ButtonPress : ButtonRelease),
		    XE_POINTER);
#ifdef XINPUT
	    }
	  }
	  else {
            xev->type = pressed ? DeviceButtonPress : DeviceButtonRelease;
            xev->deviceid = device->id | MORE_EVENTS;
	    xev->detail = truebuttons;
            xv->type = DeviceValuator;
            xv->deviceid = device->id;
            xv->num_valuators = 0;
            xv->device_state = 0;
            xf86eqEnqueue(mevent);
	  }
#endif
      }
#else
      /*
       * real three button event
       * Note that xf86Info.lastButtons has the hardware button mapping which
       * is the reverse of the button mapping reported to the server.
       */
      if (private->mseType == P_MMHIT)
        change = buttons ^ reverseBits(hitachMap, private->lastButtons);
      else
        change = buttons ^ reverseBits(reverseMap, private->lastButtons);
      while (change)
	{
	  id = ffs(change);
	  change &= ~(1 << (id-1));
#ifdef XINPUT
          if (is_pointer) {
	    if (xf86CheckButton(id, (buttons&(1<<(id-1))))) {
#endif
            ENQUEUE(mevent,
                    id, (buttons&(1<<(id-1)))? ButtonPress : ButtonRelease,
                    XE_POINTER);
#ifdef XINPUT
	    }
          }
          else {
            xev->type = (buttons&(1<<(id-1)))? DeviceButtonPress : DeviceButtonRelease;
            xev->deviceid = device->id | MORE_EVENTS;
	    xev->detail = id;
            xv->type = DeviceValuator;
            xv->deviceid = device->id;
            xv->num_valuators = 0;
            xv->device_state = 0;
            xf86eqEnqueue(mevent);
          }
#endif
	}
#endif
    }
    private->lastButtons = truebuttons;
}



/*
 * xf86Block --
 *      Os block handler.
 */

/* ARGSUSED */
void
xf86Block(blockData, pTimeout, pReadmask)
     pointer blockData;
     OSTimePtr pTimeout;
     pointer  pReadmask;
{
}


#ifndef AMOEBA

/*
 * xf86Wakeup --
 *      Os wakeup handler.
 */

/* ARGSUSED */
void
xf86Wakeup(blockData, err, pReadmask)
     pointer blockData;
     int err;
     pointer pReadmask;
{

#ifndef __EMX__
#ifdef	__OSF__
  fd_set kbdDevices;
  fd_set mseDevices;
#endif	/* __OSF__ */
  fd_set* LastSelectMask = (fd_set*)pReadmask;
  fd_set devicesWithInput;

  if ((int)err >= 0) {
    XFD_ANDSET(&devicesWithInput, LastSelectMask, &EnabledDevices);
#ifdef	__OSF__
   /*
     * Until the two devices are made nonblock on read, we have to do this.
     */

    MASKANDSETBITS(devicesWithInput, pReadmask, EnabledDevices);

    CLEARBITS(kbdDevices);
    BITSET(kbdDevices, xf86Info.consoleFd);
    MASKANDSETBITS(kbdDevices, kbdDevices, devicesWithInput);

    CLEARBITS(mseDevices);
    BITSET(mseDevices, xf86Info.mouseDev->mseFd);
    MASKANDSETBITS(mseDevices, mseDevices, devicesWithInput);

    if (ANYSET(kbdDevices) || xf86Info.kbdRate)
        (xf86Info.kbdEvents)(ANYSET(kbdDevices));
    if (ANYSET(mseDevices))
        (xf86Info.mouseDev->mseEvents)(1);

#else
    if (XFD_ANYSET(&devicesWithInput))
      {
	(xf86Info.kbdEvents)();
	(xf86Info.mouseDev->mseEvents)(xf86Info.mouseDev);
      }
#endif	/* __OSF__ */
  }
#else   /* __EMX__ */

	(xf86Info.kbdEvents)();  /* Under OS/2, always call */
	(xf86Info.mouseDev->mseEvents)(xf86Info.mouseDev);

#endif  /* __EMX__ */

#if defined(XQUEUE) && !defined(XQUEUE_ASYNC)
  /* This could be done more cleanly */
  if (xf86Info.mouseDev->xqueSema && xf86Info.mouseDev->xquePending)
    xf86XqueRequest();
#endif

  if (xf86VTSwitchPending()) xf86VTSwitch();

  if (xf86Info.inputPending) ProcessInputEvents();
}

#endif /* AMOEBA */


/*
 * xf86SigHandler --
 *    Catch unexpected signals and exit cleanly.
 */
void
xf86SigHandler(signo)
     int signo;
{
  signal(signo,SIG_IGN);
  xf86Info.caughtSignal = TRUE;
  FatalError("Caught signal %d.  Server aborting\n", signo);
}

/*
 * xf86VTSwitch --
 *      Handle requests for switching the vt.
 */
static void
xf86VTSwitch()
{
  int j;

#ifdef XFreeXDGA
  /*
   * Not ideal, but until someone adds DGA events to the DGA client we
   * should protect the machine
   */
  if (((ScrnInfoPtr)(xf86Info.currentScreen->devPrivates[xf86ScreenIndex].ptr))->directMode&XF86DGADirectGraphics) {
   xf86Info.vtRequestsPending = FALSE;
   return;
  }
#endif
  if (xf86VTSema) {
    for (j = 0; j < screenInfo.numScreens; j++)
      (XF86SCRNINFO(screenInfo.screens[j])->EnterLeaveVT)(LEAVE, j);

#ifndef __EMX__
    DisableDevice((DeviceIntPtr)xf86Info.pKeyboard);
    DisableDevice((DeviceIntPtr)xf86Info.pMouse);
#endif

    if (!xf86VTSwitchAway()) {
      /*
       * switch failed 
       */

      for (j = 0; j < screenInfo.numScreens; j++)
        (XF86SCRNINFO(screenInfo.screens[j])->EnterLeaveVT)(ENTER, j);
      SaveScreens(SCREEN_SAVER_FORCER,ScreenSaverReset);
#ifdef DPMSExtension
      if (DPMSEnabled)
        DPMSSet(DPMSModeOn);
#endif

#ifndef __EMX__
      EnableDevice((DeviceIntPtr)xf86Info.pKeyboard);
      EnableDevice((DeviceIntPtr)xf86Info.pMouse);
#endif

    } else {
      xf86VTSema = FALSE;
    }
  } else {
    if (!xf86VTSwitchTo()) return;
      
    xf86VTSema = TRUE;
    for (j = 0; j < screenInfo.numScreens; j++)
      (XF86SCRNINFO(screenInfo.screens[j])->EnterLeaveVT)(ENTER, j);
      
    /* Turn screen saver off when switching back */
    SaveScreens(SCREEN_SAVER_FORCER,ScreenSaverReset);
#ifdef DPMSExtension
    if (DPMSEnabled)
      DPMSSet(DPMSModeOn);
#endif

#ifndef __EMX__
    EnableDevice((DeviceIntPtr)xf86Info.pKeyboard);
    EnableDevice((DeviceIntPtr)xf86Info.pMouse);
#endif

  }
}

#ifdef XTESTEXT1

void
XTestGetPointerPos(fmousex, fmousey)
     short *fmousex;
     short *fmousey;
{
  int x,y;

  miPointerPosition(&x, &y);
  *fmousex = x;
  *fmousey = y;
}



void
XTestJumpPointer(jx, jy, dev_type)
     int jx;
     int jy;
     int dev_type;
{
  miPointerAbsoluteCursor(jx, jy, GetTimeInMillis() );
}



void
XTestGenerateEvent(dev_type, keycode, keystate, mousex, mousey)
     int dev_type;
     int keycode;
     int keystate;
     int mousex;
     int mousey;
{
  xEvent tevent;
  
  tevent.u.u.type = (dev_type == XE_POINTER) ?
    (keystate == XTestKEY_UP) ? ButtonRelease : ButtonPress :
      (keystate == XTestKEY_UP) ? KeyRelease : KeyPress;
  tevent.u.u.detail = keycode;
  tevent.u.keyButtonPointer.rootX = mousex;
  tevent.u.keyButtonPointer.rootY = mousey;
  tevent.u.keyButtonPointer.time = xf86Info.lastEventTime = GetTimeInMillis();
#ifdef XINPUT
  xf86eqEnqueue(&tevent);
#else
  mieqEnqueue(&tevent);
#endif
  xf86Info.inputPending = TRUE;               /* virtual event */
}

#endif /* XTESTEXT1 */


#ifdef XFreeXDGA
static void
XF86DirectVideoMoveMouse(x, y, mtime)
     int x;
     int y;
     CARD32 mtime;
{
  xEvent xE;

  xE.u.u.type = MotionNotify;
  xE.u.keyButtonPointer.time = xf86Info.lastEventTime = mtime;
  xf86Info.lastEventTime = mtime;


  xE.u.keyButtonPointer.eventY = x;
  xE.u.keyButtonPointer.eventY = y;
  xE.u.keyButtonPointer.rootX = x;
  xE.u.keyButtonPointer.rootY = y;

  if (((DeviceIntPtr)(xf86Info.pMouse))->grab)
     DeliverGrabbedEvent(&xE, (xf86Info.pMouse), FALSE, 1);
  else
     DeliverDeviceEvents(GetSpriteWindow(), &xE, NullGrab, NullWindow,
			   (xf86Info.pMouse), 1);
}

static void
XF86DirectVideoKeyEvent(xE, keycode, etype)
xEvent *xE;
int keycode;
int etype;
{
  DeviceIntPtr keybd = (DeviceIntPtr)xf86Info.pKeyboard;
  KeyClassPtr keyc = keybd->key;
  BYTE *kptr;

  kptr = &keyc->down[keycode >> 3];
  xE->u.u.type = etype;
  xE->u.u.detail = keycode;

  /* clear the keypress state */
  if (etype == KeyPress) {
    *kptr &= ~(1 << (keycode & 7));
  }
  keybd->public.processInputProc(xE, keybd, 1);
}
#endif
