/*
 * Id: kinput.c,v 1.1 1999/11/02 03:54:46 keithp Exp $
 *
 * Copyright © 1999 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $XFree86: xc/programs/Xserver/hw/kdrive/kinput.c,v 1.12 2001/01/23 06:25:05 keithp Exp $ */

#include "kdrive.h"
#include "inputstr.h"

#define XK_PUBLISHING
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include "kkeymap.h"
#include <signal.h>
#include <stdio.h>

static DeviceIntPtr	pKdKeyboard, pKdPointer;

static KdMouseFuncs	*kdMouseFuncs;
static KdKeyboardFuncs	*kdKeyboardFuncs;
static int		kdMouseFd = -1;
static int		kdKeyboardFd = -1;
static unsigned long	kdEmulationTimeout;
static Bool		kdTimeoutPending;
static int		kdBellPitch;
static int		kdBellDuration;
static int		kdLeds;
static Bool		kdInputEnabled;
static Bool		kdOffScreen;
static unsigned long	kdOffScreenTime;
static KdMouseMatrix	kdMouseMatrix = {
   { { 1, 0, 0 },
     { 0, 1, 0 } }
};

#ifdef TOUCHSCREEN
static KdTsFuncs	*kdTsFuncs;
static int		kdTsFd = -1;
#endif

int		kdMinScanCode;
int		kdMaxScanCode;
int		kdMinKeyCode;
int		kdMaxKeyCode;
int		kdKeymapWidth = KD_MAX_WIDTH;
KeySym		kdKeymap[KD_MAX_LENGTH * KD_MAX_WIDTH];
CARD8		kdModMap[MAP_LENGTH];
KeySymsRec	kdKeySyms;


void
KdResetInputMachine (void);
    
#define KD_MOUSEBUTTON_COUNT	3

#define KD_KEY_COUNT		248

CARD8		kdKeyState[KD_KEY_COUNT/8];

#define IsKeyDown(key) ((kdKeyState[(key) >> 3] >> ((key) & 7)) & 1)

void
KdSigio (int sig)
{
#ifdef TOUCHSCREEN    
    if (kdTsFd >= 0)
 	(*kdTsFuncs->Read) (kdTsFd);
#endif
    if (kdMouseFd >= 0)
	(*kdMouseFuncs->Read) (kdMouseFd);
    if (kdKeyboardFd >= 0)
	(*kdKeyboardFuncs->Read) (kdKeyboardFd);
}

void
KdBlockSigio (void)
{
    sigset_t	set;
    
    sigemptyset (&set);
    sigaddset (&set, SIGIO);
    sigprocmask (SIG_BLOCK, &set, 0);
}

void
KdUnblockSigio (void)
{
    sigset_t	set;
    
    sigemptyset (&set);
    sigaddset (&set, SIGIO);
    sigprocmask (SIG_UNBLOCK, &set, 0);
}

#define VERIFY_SIGIO
#ifdef VERIFY_SIGIO

void
KdAssertSigioBlocked (char *where)
{
    sigset_t	set, old;

    sigemptyset (&set);
    sigprocmask (SIG_BLOCK, &set, &old);
    if (!sigismember (&old, SIGIO))
	ErrorF ("SIGIO not blocked at %s\n", where);
}

#else

#define KdVerifySigioBlocked(s)

#endif

static int  kdnFds;

#ifdef FNONBLOCK
#define NOBLOCK FNONBLOCK
#else
#define NOBLOCK FNDELAY
#endif

void
KdAddFd (int fd)
{
    int	flags;
    struct sigaction	act;
    sigset_t		set;
    
    kdnFds++;
    fcntl (fd, F_SETOWN, getpid());
    flags = fcntl (fd, F_GETFL);
    flags |= FASYNC|NOBLOCK;
    fcntl (fd, F_SETFL, flags);
    AddEnabledDevice (fd);
    memset (&act, '\0', sizeof act);
    act.sa_handler = KdSigio;
    sigemptyset (&act.sa_mask);
    sigaddset (&act.sa_mask, SIGIO);
    sigaddset (&act.sa_mask, SIGALRM);
    sigaddset (&act.sa_mask, SIGVTALRM);
    sigaction (SIGIO, &act, 0);
    sigemptyset (&set);
    sigprocmask (SIG_SETMASK, &set, 0);
}

void
KdRemoveFd (int fd)
{
    struct sigaction	act;
    int			flags;
    
    kdnFds--;
    RemoveEnabledDevice (fd);
    flags = fcntl (fd, F_GETFL);
    flags &= ~(FASYNC|NOBLOCK);
    fcntl (fd, F_SETFL, flags);
    if (kdnFds == 0)
    {
	memset (&act, '\0', sizeof act);
	act.sa_handler = SIG_IGN;
	sigemptyset (&act.sa_mask);
	sigaction (SIGIO, &act, 0);
    }
}

void
KdDisableInput (void)
{
#ifdef TOUCHSCREEN
    if (kdTsFd >= 0)
    	KdRemoveFd (kdTsFd);
#endif
    if (kdMouseFd >= 0)
	KdRemoveFd (kdMouseFd);
    if (kdKeyboardFd >= 0)
	KdRemoveFd (kdKeyboardFd);
    kdInputEnabled = FALSE;
}

void
KdEnableInput (void)
{
    xEvent	xE;
    
    kdInputEnabled = TRUE;
#ifdef TOUCHSCREEN
    if (kdTsFd >= 0)
    	KdAddFd (kdTsFd);
#endif
    if (kdMouseFd >= 0)
	KdAddFd (kdMouseFd);
    if (kdKeyboardFd >= 0)
	KdAddFd (kdKeyboardFd);
    /* reset screen saver */
    xE.u.keyButtonPointer.time = GetTimeInMillis ();
    NoticeEventTime (&xE);
}

static int
KdMouseProc(DeviceIntPtr pDevice, int onoff)
{
    BYTE	map[4];
    DevicePtr	pDev = (DevicePtr)pDevice;
    int		i;
    
    if (!pDev)
	return BadImplementation;
    
    switch (onoff)
    {
    case DEVICE_INIT:
	for (i = 1; i <= KD_MOUSEBUTTON_COUNT; i++)
	    map[i] = i;
	InitPointerDeviceStruct(pDev, map, KD_MOUSEBUTTON_COUNT,
	    miPointerGetMotionEvents,
	    (PtrCtrlProcPtr)NoopDDA,
	    miPointerGetMotionBufferSize());
	break;
	
    case DEVICE_ON:
	pDev->on = TRUE;
	pKdPointer = pDevice;
	if (kdMouseFuncs)
	{
	    kdMouseFd = (*kdMouseFuncs->Init) ();
	    if (kdMouseFd >= 0 && kdInputEnabled)
		KdAddFd (kdMouseFd);
	}
#ifdef TOUCHSCREEN
	if (kdTsFuncs)
	{
	    kdTsFd = (*kdTsFuncs->Init) ();
	    if (kdTsFd >= 0 && kdInputEnabled)
		KdAddFd (kdTsFd);
	}
#endif
	break;
    case DEVICE_OFF:
    case DEVICE_CLOSE:
	if (pDev->on)
	{
	    pDev->on = FALSE;
	    pKdPointer = 0;
	    if (kdMouseFd >= 0)
	    {
		if (kdInputEnabled)
		    KdRemoveFd (kdMouseFd);
		(*kdMouseFuncs->Fini) (kdMouseFd);
		kdMouseFd = -1;
	    }
#ifdef TOUCHSCREEN
	    if (kdTsFd >= 0)
	    {
		if (kdInputEnabled)
		    KdRemoveFd (kdTsFd);
		(*kdTsFuncs->Fini) (kdTsFd);
		kdTsFd = -1;
	    }
#endif
	}
	break;
    }
    return Success;
}

Bool
LegalModifier(unsigned int key, DevicePtr pDev)
{
    return TRUE;
}

static void
KdBell (int volume, DeviceIntPtr pDev, pointer ctrl, int something)
{
    if (kdInputEnabled)
	(*kdKeyboardFuncs->Bell) (volume, kdBellPitch, kdBellDuration);
}


static void
KdSetLeds (void)
{
    if (kdInputEnabled)
	(*kdKeyboardFuncs->Leds) (kdLeds);
}

void
KdSetLed (int led, Bool on)
{
    NoteLedState (pKdKeyboard, led, on);
    kdLeds = pKdKeyboard->kbdfeed->ctrl.leds;
    KdSetLeds ();
}

void
KdSetMouseMatrix (KdMouseMatrix *matrix)
{
    kdMouseMatrix = *matrix;
}

static void
KdKbdCtrl (DeviceIntPtr pDevice, KeybdCtrl *ctrl)
{
    kdLeds = ctrl->leds;
    kdBellPitch = ctrl->bell_pitch;
    kdBellDuration = ctrl->bell_duration;
    KdSetLeds ();
}

static int
KdKeybdProc(DeviceIntPtr pDevice, int onoff)
{
    Bool        ret;
    DevicePtr   pDev = (DevicePtr)pDevice;

    if (!pDev)
	return BadImplementation;

    switch (onoff)
    {
    case DEVICE_INIT:
	if (pDev != LookupKeyboardDevice())
	{
	    return !Success;
	}
	ret = InitKeyboardDeviceStruct(pDev,
				       &kdKeySyms,
				       kdModMap,
				       KdBell, KdKbdCtrl);
	if (!ret)
	    return BadImplementation;
	break;
    case DEVICE_ON:
	pDev->on = TRUE;
	pKdKeyboard = pDevice;
	if (kdKeyboardFuncs)
	{
	    kdKeyboardFd = (*kdKeyboardFuncs->Init) ();
	    if (kdKeyboardFd >= 0 && kdInputEnabled)
		KdAddFd (kdKeyboardFd);
	}
	break;
    case DEVICE_OFF:
    case DEVICE_CLOSE:
	pKdKeyboard = 0;
	if (pDev->on)
	{
	    pDev->on = FALSE;
	    if (kdKeyboardFd >= 0)
	    {
		if (kdInputEnabled)
		    KdRemoveFd (kdKeyboardFd);
		(*kdKeyboardFuncs->Fini) (kdKeyboardFd);
		kdKeyboardFd = -1;
	    }
	}
	break;
    }
    return Success;
}

extern KeybdCtrl defaultKeyboardControl;

static void
KdInitAutoRepeats (void)
{
    int		    key_code;
    unsigned char   mask;
    int		    i;
    unsigned char   *repeats;

    repeats = defaultKeyboardControl.autoRepeats;
    memset (repeats, '\0', 32);
    for (key_code = KD_MIN_KEYCODE; key_code <= KD_MAX_KEYCODE; key_code++)
    {
	if (!kdModMap[key_code])
	{
	    i = key_code >> 3;
	    mask = 1 << (key_code & 7);
	    repeats[i] |= mask;
	}
    }
}

const KdKeySymModsRec kdKeySymMods[] = {
  {  XK_Control_L,	ControlMask },
  {  XK_Control_R, ControlMask },
  {  XK_Shift_L,	ShiftMask },
  {  XK_Shift_R,	ShiftMask },
  {  XK_Caps_Lock,	LockMask },
  {  XK_Shift_Lock, LockMask },
  {  XK_Alt_L,	Mod1Mask },
  {  XK_Alt_R,	Mod1Mask },
  {  XK_Meta_L,	Mod1Mask },
  {  XK_Meta_R,	Mod1Mask },
  {  XK_Num_Lock,	Mod2Mask },
  {  XK_Super_L,	Mod3Mask },
  {  XK_Super_R,	Mod3Mask },
  {  XK_Hyper_L,	Mod3Mask },
  {  XK_Hyper_R,	Mod3Mask },
  {  XK_Mode_switch, Mod4Mask },
#ifdef TOUCHSCREEN
  /* PDA specific hacks */
  {  XF86XK_Start, ControlMask },
  {  XK_Menu, ShiftMask },
  {  XK_telephone, Mod1Mask },
  {  XF86XK_AudioRecord, Mod2Mask },
  {  XF86XK_Calendar, Mod3Mask }
#endif
};

#define NUM_SYM_MODS (sizeof(kdKeySymMods) / sizeof(kdKeySymMods[0]))

static void
KdInitModMap (void)
{
    int	    key_code;
    int	    row;
    int	    width;
    KeySym  *syms;
    int	    i;

    width = kdKeySyms.mapWidth;
    for (key_code = kdMinKeyCode; key_code <= kdMaxKeyCode; key_code++)
    {
	kdModMap[key_code] = 0;
	syms = kdKeymap + (key_code - kdMinKeyCode) * width;
	for (row = 0; row < width; row++, syms++)
	{
	    for (i = 0; i < NUM_SYM_MODS; i++) 
	    {
		if (*syms == kdKeySymMods[i].modsym) 
		    kdModMap[key_code] |= kdKeySymMods[i].modbit;
	    }
	}
    }
}

void
KdInitInput(KdMouseFuncs    *pMouseFuncs,
	    KdKeyboardFuncs *pKeyboardFuncs)
{
    DeviceIntPtr	pKeyboard, pPointer;
    
    kdMouseFuncs = pMouseFuncs;
    kdKeyboardFuncs = pKeyboardFuncs;
    memset (kdKeyState, '\0', sizeof (kdKeyState));
    if (kdKeyboardFuncs)
	(*kdKeyboardFuncs->Load) ();
    kdMinKeyCode = kdMinScanCode + KD_KEY_OFFSET;
    kdMaxKeyCode = kdMaxScanCode + KD_KEY_OFFSET;
    kdKeySyms.map = kdKeymap;
    kdKeySyms.minKeyCode = kdMinKeyCode;
    kdKeySyms.maxKeyCode = kdMaxKeyCode;
    kdKeySyms.mapWidth = kdKeymapWidth;
    kdLeds = 0;
    kdBellPitch = 1000;
    kdBellDuration = 200;
    kdInputEnabled = TRUE;
    KdInitModMap ();
    KdInitAutoRepeats ();
    KdResetInputMachine ();
    pPointer  = AddInputDevice(KdMouseProc, TRUE);
    pKeyboard = AddInputDevice(KdKeybdProc, TRUE);
    RegisterPointerDevice(pPointer);
    RegisterKeyboardDevice(pKeyboard);
    miRegisterPointerDevice(screenInfo.screens[0], pPointer);
    mieqInit(&pKeyboard->public, &pPointer->public);
#ifdef XINPUT
    {
	static long zero1, zero2;

	SetExtInputCheck (&zero1, &zero2);
    }
#endif
}

#ifdef TOUCHSCREEN
void
KdInitTouchScreen(KdTsFuncs *pTsFuncs)
{
    kdTsFuncs = pTsFuncs;
}
#endif

/*
 * Middle button emulation state machine
 *
 *  Possible transitions:
 *	Button 1 press	    v1
 *	Button 1 release    ^1
 *	Button 2 press	    v2
 *	Button 2 release    ^2
 *	Button 3 press	    v3
 *	Button 3 release    ^3
 *	Mouse motion	    <>
 *	Keyboard event	    k
 *	timeout		    ...
 *	outside box	    <->
 *
 *  States:
 *	start
 *	button_1_pend
 *	button_1_down
 *	button_2_down
 *	button_3_pend
 *	button_3_down
 *	synthetic_2_down_13
 *	synthetic_2_down_3
 *	synthetic_2_down_1
 *
 *  Transition diagram
 *
 *  start
 *	v1  -> (hold) (settimeout) button_1_pend
 *	^1  -> (deliver) start
 *	v2  -> (deliver) button_2_down
 *	^2  -> (deliever) start
 *	v3  -> (hold) (settimeout) button_3_pend
 *	^3  -> (deliver) start
 *	<>  -> (deliver) start
 *	k   -> (deliver) start
 *
 *  button_1_pend	(button 1 is down, timeout pending)
 *	^1  -> (release) (deliver) start
 *	v2  -> (release) (deliver) button_1_down
 *	^2  -> (release) (deliver) button_1_down
 *	v3  -> (cleartimeout) (generate v2) synthetic_2_down_13
 *	^3  -> (release) (deliver) button_1_down
 *	<-> -> (release) (deliver) button_1_down
 *	<>  -> (deliver) button_1_pend
 *	k   -> (release) (deliver) button_1_down
 *	... -> (release) button_1_down
 *
 *  button_1_down	(button 1 is down)
 *	^1  -> (deliver) start
 *	v2  -> (deliver) button_1_down
 *	^2  -> (deliver) button_1_down
 *	v3  -> (deliver) button_1_down
 *	^3  -> (deliver) button_1_down
 *	<>  -> (deliver) button_1_down
 *	k   -> (deliver) button_1_down
 *
 *  button_2_down	(button 2 is down)
 *	v1  -> (deliver) button_2_down
 *	^1  -> (deliver) button_2_down
 *	^2  -> (deliver) start
 *	v3  -> (deliver) button_2_down
 *	^3  -> (deliver) button_2_down
 *	<>  -> (deliver) button_2_down
 *	k   -> (deliver) button_2_down
 *
 *  button_3_pend	(button 3 is down, timeout pending)
 *	v1  -> (generate v2) synthetic_2_down
 *	^1  -> (release) (deliver) button_3_down
 *	v2  -> (release) (deliver) button_3_down
 *	^2  -> (release) (deliver) button_3_down
 *	^3  -> (release) (deliver) start
 *	<-> -> (release) (deliver) button_3_down
 *	<>  -> (deliver) button_3_pend
 *	k   -> (release) (deliver) button_3_down
 *	... -> (release) button_3_down
 *
 *  button_3_down	(button 3 is down)
 *	v1  -> (deliver) button_3_down
 *	^1  -> (deliver) button_3_down
 *	v2  -> (deliver) button_3_down
 *	^2  -> (deliver) button_3_down
 *	^3  -> (deliver) start
 *	<>  -> (deliver) button_3_down
 *	k   -> (deliver) button_3_down
 *
 *  synthetic_2_down_13	(button 1 and 3 are down)
 *	^1  -> (generate ^2) synthetic_2_down_3
 *	v2  -> synthetic_2_down_13
 *	^2  -> synthetic_2_down_13
 *	^3  -> (generate ^2) synthetic_2_down_1
 *	<>  -> (deliver) synthetic_2_down_13
 *	k   -> (deliver) synthetic_2_down_13
 *
 *  synthetic_2_down_3 (button 3 is down)
 *	v1  -> (deliver) synthetic_2_down_3
 *	^1  -> (deliver) synthetic_2_down_3
 *	v2  -> synthetic_2_down_3
 *	^2  -> synthetic_2_down_3
 *	^3  -> start
 *	<>  -> (deliver) synthetic_2_down_3
 *	k   -> (deliver) synthetic_2_down_3
 *
 *  synthetic_2_down_1 (button 1 is down)
 *	^1  -> start
 *	v2  -> synthetic_2_down_1
 *	^2  -> synthetic_2_down_1
 *	v3  -> (deliver) synthetic_2_down_1
 *	^3  -> (deliver) synthetic_2_down_1
 *	<>  -> (deliver) synthetic_2_down_1
 *	k   -> (deliver) synthetic_2_down_1
 */
 
typedef enum _inputState {
    start,
    button_1_pend,
    button_1_down,
    button_2_down,
    button_3_pend,
    button_3_down,
    synth_2_down_13,
    synth_2_down_3,
    synth_2_down_1,
    num_input_states
} KdInputState;

typedef enum _inputClass {
    down_1, up_1,
    down_2, up_2,
    down_3, up_3,
    motion, outside_box,
    keyboard, timeout,
    num_input_class
} KdInputClass;

typedef enum _inputAction {
    noop,
    hold,
    setto,
    deliver,
    release,
    clearto,
    gen_down_2,
    gen_up_2
} KdInputAction;

#define MAX_ACTIONS 2

typedef struct _inputTransition {
    KdInputAction  actions[MAX_ACTIONS];
    KdInputState   nextState;
} KdInputTransition;

KdInputTransition  kdInputMachine[num_input_states][num_input_class] = {
    /* start */
    {
	{ { hold, setto },	    button_1_pend },	/* v1 */
	{ { deliver, noop },	    start },		/* ^1 */
	{ { deliver, noop },	    button_2_down },	/* v2 */
	{ { deliver, noop },	    start },		/* ^2 */
	{ { hold, setto },	    button_3_pend },	/* v3 */
	{ { deliver, noop },	    start },		/* ^3 */
	{ { deliver, noop },	    start },		/* <> */
	{ { deliver, noop },	    start },		/* <-> */
	{ { deliver, noop },	    start },		/* k */
	{ { noop, noop },	    start },		/* ... */
    },
    /* button_1_pend */
    {
	{ { noop, noop },	    button_1_pend },	/* v1 */
	{ { release, deliver },	    start },		/* ^1 */
	{ { release, deliver },	    button_1_down },	/* v2 */
	{ { release, deliver },	    button_1_down },	/* ^2 */
	{ { clearto, gen_down_2 },  synth_2_down_13 },	/* v3 */
	{ { release, deliver },	    button_1_down },	/* ^3 */
	{ { deliver, noop },	    button_1_pend },	/* <> */
	{ { release, deliver },	    button_1_down },	/* <-> */
	{ { release, deliver },	    button_1_down },	/* k */
	{ { release, noop },	    button_1_down },	/* ... */
    },
    /* button_1_down */
    {
	{ { noop, noop },	    button_1_down },	/* v1 */
	{ { deliver, noop },	    start },		/* ^1 */
	{ { deliver, noop },	    button_1_down },	/* v2 */
	{ { deliver, noop },	    button_1_down },	/* ^2 */
	{ { deliver, noop },	    button_1_down },	/* v3 */
	{ { deliver, noop },	    button_1_down },	/* ^3 */
	{ { deliver, noop },	    button_1_down },	/* <> */
	{ { deliver, noop },	    button_1_down },	/* <-> */
	{ { deliver, noop },	    button_1_down },	/* k */
	{ { noop, noop },	    button_1_down },	/* ... */
    },
    /* button_2_down */
    {
	{ { deliver, noop },	    button_2_down },	/* v1 */
	{ { deliver, noop },	    button_2_down },	/* ^1 */
	{ { noop, noop },	    button_2_down },	/* v2 */
	{ { deliver, noop },	    start },		/* ^2 */
	{ { deliver, noop },	    button_2_down },	/* v3 */
	{ { deliver, noop },	    button_2_down },	/* ^3 */
	{ { deliver, noop },	    button_2_down },	/* <> */
	{ { deliver, noop },	    button_2_down },	/* <-> */
	{ { deliver, noop },	    button_2_down },	/* k */
	{ { noop, noop },	    button_2_down },	/* ... */
    },
    /* button_3_pend */
    {
	{ { clearto, gen_down_2 },  synth_2_down_13 },	/* v1 */
	{ { release, deliver },	    button_3_down },	/* ^1 */
	{ { release, deliver },	    button_3_down },	/* v2 */
	{ { release, deliver },	    button_3_down },	/* ^2 */
	{ { release, deliver },	    button_3_down },	/* v3 */
	{ { release, deliver },	    start },		/* ^3 */
	{ { deliver, noop },	    button_3_pend },	/* <> */
	{ { release, deliver },	    button_3_down },	/* <-> */
	{ { release, deliver },	    button_3_down },	/* k */
	{ { release, noop },	    button_3_down },	/* ... */
    },
    /* button_3_down */
    {
	{ { deliver, noop },	    button_3_down },	/* v1 */
	{ { deliver, noop },	    button_3_down },	/* ^1 */
	{ { deliver, noop },	    button_3_down },	/* v2 */
	{ { deliver, noop },	    button_3_down },	/* ^2 */
	{ { noop, noop },	    button_3_down },	/* v3 */
	{ { deliver, noop },	    start },		/* ^3 */
	{ { deliver, noop },	    button_3_down },	/* <> */
	{ { deliver, noop },	    button_3_down },	/* <-> */
	{ { deliver, noop },	    button_3_down },	/* k */
	{ { noop, noop },	    button_3_down },	/* ... */
    },
    /* synthetic_2_down_13 */
    {
	{ { noop, noop },	    synth_2_down_13 },	/* v1 */
	{ { gen_up_2, noop },	    synth_2_down_3 },	/* ^1 */
	{ { noop, noop },	    synth_2_down_13 },	/* v2 */
	{ { noop, noop },	    synth_2_down_13 },	/* ^2 */
	{ { noop, noop },	    synth_2_down_13 },	/* v3 */
	{ { gen_up_2, noop },	    synth_2_down_1 },	/* ^3 */
	{ { deliver, noop },	    synth_2_down_13 },	/* <> */
	{ { deliver, noop },	    synth_2_down_13 },	/* <-> */
	{ { deliver, noop },	    synth_2_down_13 },	/* k */
	{ { noop, noop },	    synth_2_down_13 },	/* ... */
    },
    /* synthetic_2_down_3 */
    {
	{ { deliver, noop },	    synth_2_down_3 },	/* v1 */
	{ { deliver, noop },	    synth_2_down_3 },	/* ^1 */
	{ { deliver, noop },	    synth_2_down_3 },	/* v2 */
	{ { deliver, noop },	    synth_2_down_3 },	/* ^2 */
	{ { noop, noop },	    synth_2_down_3 },	/* v3 */
	{ { noop, noop },	    start },		/* ^3 */
	{ { deliver, noop },	    synth_2_down_3 },	/* <> */
	{ { deliver, noop },	    synth_2_down_3 },	/* <-> */
	{ { deliver, noop },	    synth_2_down_3 },	/* k */
	{ { noop, noop },	    synth_2_down_3 },	/* ... */
    },
    /* synthetic_2_down_1 */
    {
	{ { noop, noop },	    synth_2_down_1 },	/* v1 */
	{ { noop, noop },	    start },		/* ^1 */
	{ { deliver, noop },	    synth_2_down_1 },	/* v2 */
	{ { deliver, noop },	    synth_2_down_1 },	/* ^2 */
	{ { deliver, noop },	    synth_2_down_1 },	/* v3 */
	{ { deliver, noop },	    synth_2_down_1 },	/* ^3 */
	{ { deliver, noop },	    synth_2_down_1 },	/* <> */
	{ { deliver, noop },	    synth_2_down_1 },	/* <-> */
	{ { deliver, noop },	    synth_2_down_1 },	/* k */
	{ { noop, noop },	    synth_2_down_1 },	/* ... */
    },
};

Bool		kdEventHeld;
xEvent		kdHeldEvent;
int		kdEmulationDx, kdEmulationDy;

#define EMULATION_WINDOW    10
#define EMULATION_TIMEOUT   100

#define EventX(e)   ((e)->u.keyButtonPointer.rootX)
#define EventY(e)   ((e)->u.keyButtonPointer.rootY)

int
KdInsideEmulationWindow (xEvent *ev)
{
    if (ev->u.keyButtonPointer.pad1)
    {
	kdEmulationDx += EventX(ev);
	kdEmulationDy += EventY(ev);
    }
    else
    {
	kdEmulationDx = EventX(&kdHeldEvent) - EventX(ev);
	kdEmulationDy = EventY(&kdHeldEvent) - EventY(ev);
    }
    return (abs (kdEmulationDx) < EMULATION_WINDOW &&
	    abs (kdEmulationDy) < EMULATION_WINDOW);
}
				     
KdInputClass
KdClassifyInput (xEvent *ev)
{
    switch (ev->u.u.type) {
    case ButtonPress:
	switch (ev->u.u.detail) {
	case 1: return down_1;
	case 2: return down_2;
	case 3: return down_3;
	}
	break;
    case ButtonRelease:
	switch (ev->u.u.detail) {
	case 1: return up_1;
	case 2: return up_2;
	case 3: return up_3;
	}
	break;
    case MotionNotify:
	if (kdEventHeld && !KdInsideEmulationWindow(ev))
	    return outside_box;
	else
	    return motion;
    default:
	return keyboard;
    }
    return keyboard;
}

#ifndef NDEBUG
char	*kdStateNames[] = {
    "start",
    "button_1_pend",
    "button_1_down",
    "button_2_down",
    "button_3_pend",
    "button_3_down",
    "synth_2_down_13",
    "synth_2_down_3",
    "synthetic_2_down_1",
    "num_input_states"
};

char	*kdClassNames[] = {
    "down_1", "up_1",
    "down_2", "up_2",
    "down_3", "up_3",
    "motion", "ouside_box",
    "keyboard", "timeout",
    "num_input_class"
};

char *kdActionNames[] = {
    "noop",
    "hold",
    "setto",
    "deliver",
    "release",
    "clearto",
    "gen_down_2",
    "gen_up_2",
};
#endif

static void
KdQueueEvent (xEvent *ev)
{
    KdAssertSigioBlocked ("KdQueueEvent");
    if (ev->u.u.type == MotionNotify)
    {
	if (ev->u.keyButtonPointer.pad1)
	{
	    ev->u.keyButtonPointer.pad1 = 0;
	    miPointerDeltaCursor (ev->u.keyButtonPointer.rootX, 
				  ev->u.keyButtonPointer.rootY, 
				  ev->u.keyButtonPointer.time);
	}
	else
	{
	    miPointerAbsoluteCursor(ev->u.keyButtonPointer.rootX, 
				    ev->u.keyButtonPointer.rootY, 
				    ev->u.keyButtonPointer.time);
	}
    }
    else
    {
	mieqEnqueue (ev);
    }
}

KdInputState	kdInputState;

static void
KdRunInputMachine (KdInputClass c, xEvent *ev)
{
    KdInputTransition	*t;
    int			a;

    t = &kdInputMachine[kdInputState][c];
    for (a = 0; a < MAX_ACTIONS; a++)
    {
	switch (t->actions[a]) {
	case noop:
	    break;
	case hold:
	    kdEventHeld = TRUE;
	    kdEmulationDx = 0;
	    kdEmulationDy = 0;
	    kdHeldEvent = *ev;
	    break;
	case setto:
	    kdEmulationTimeout = GetTimeInMillis () + EMULATION_TIMEOUT;
	    kdTimeoutPending = TRUE;
	    break;
	case deliver:
	    KdQueueEvent (ev);
	    break;
	case release:
	    kdEventHeld = FALSE;
	    kdTimeoutPending = FALSE;
	    KdQueueEvent (&kdHeldEvent);
	    break;
	case clearto:
	    kdTimeoutPending = FALSE;
	    break;
	case gen_down_2:
	    ev->u.u.detail = 2;
	    kdEventHeld = FALSE;
	    KdQueueEvent (ev);
	    break;
	case gen_up_2:
	    ev->u.u.detail = 2;
	    KdQueueEvent (ev);
	    break;
	}
    }
    kdInputState = t->nextState;
}

void
KdResetInputMachine (void)
{
    kdInputState = start;
    kdEventHeld = FALSE;
}

void
KdHandleEvent (xEvent *ev)
{
    if (kdEmulateMiddleButton)
	KdRunInputMachine (KdClassifyInput (ev), ev);
    else
	KdQueueEvent (ev);
}

void
KdReceiveTimeout (void)
{
    KdRunInputMachine (timeout, 0);
}

#define KILL_SEQUENCE ((1L << KK_CONTROL)|(1L << KK_ALT)|(1L << KK_F8)|(1L << KK_F10))
#define SPECIAL_SEQUENCE ((1L << KK_CONTROL) | (1L << KK_ALT))
#define SETKILLKEY(b) (KdSpecialKeys |= (1L << (b)))
#define CLEARKILLKEY(b) (KdSpecialKeys &= ~(1L << (b)))
#define KEYMAP	    (pKdKeyboard->key->curKeySyms)
#define KEYCOL1(k) (KEYMAP.map[((k)-kdMinKeyCode)*KEYMAP.mapWidth])

CARD32	KdSpecialKeys = 0;

extern char dispatchException;

/*
 * kdCheckTermination
 *
 * This function checks for the key sequence that terminates the server.  When
 * detected, it sets the dispatchException flag and returns.  The key sequence
 * is:
 *	Control-Alt
 * It's assumed that the server will be waken up by the caller when this
 * function returns.
 */

extern int nClients;

void
KdCheckSpecialKeys(xEvent *xE)
{
    KeySym	sym;
    
    if (!pKdKeyboard) return;

    /*
     * Ignore key releases
     */
    
    if (xE->u.u.type == KeyRelease) return;
    
    /*
     * Check for control/alt pressed
     */
    if ((pKdKeyboard->key->state & (ControlMask|Mod1Mask)) !=
	(ControlMask|Mod1Mask))
	return;
    
    sym = KEYCOL1(xE->u.u.detail);
    
    /*
     * Let OS function see keysym first
     */
    
    if (kdOsFuncs->SpecialKey)
	if ((*kdOsFuncs->SpecialKey) (sym))
	    return;

    /*
     * Now check for backspace or delete; these signal the
     * X server to terminate
     */
    switch (sym) {
    case XK_BackSpace:
    case XK_Delete:
    case XK_KP_Delete:
	/* 
	 * Set the dispatch exception flag so the server will terminate the
	 * next time through the dispatch loop.
	 */
	dispatchException |= DE_TERMINATE;
	break;
    }
}

/*
 * kdEnqueueKeyboardEvent
 *
 * This function converts hardware keyboard event information into an X event
 * and enqueues it using MI.  It wakes up the server before returning so that
 * the event will be processed normally.
 *
 */

void
KdHandleKeyboardEvent (xEvent *ev)
{
    int	    key = ev->u.u.detail;
    int	    byte;
    CARD8   bit;
    
    byte = key >> 3;
    bit = 1 << (key & 7);
    switch (ev->u.u.type) {
    case KeyPress:
	kdKeyState[byte] |= bit;
	break;
    case KeyRelease:
	kdKeyState[byte] &= ~bit;
	break;
    }
    KdHandleEvent (ev);
}

void
KdReleaseAllKeys (void)
{
    xEvent  xE;
    int	    key;

    KdBlockSigio ();
    for (key = 0; key < KD_KEY_COUNT; key++)
	if (IsKeyDown(key))
	{
	    xE.u.keyButtonPointer.time = GetTimeInMillis();
	    xE.u.u.type = KeyRelease;
	    xE.u.u.detail = key;
	    KdHandleKeyboardEvent (&xE);
	}
    KdUnblockSigio ();
}

void
KdCheckLock (void)
{
    KeyClassPtr	    keyc = pKdKeyboard->key;
    Bool	    isSet, shouldBeSet;

    if (kdKeyboardFuncs->LockLed)
    {
	isSet = (kdLeds & (1 << (kdKeyboardFuncs->LockLed-1))) != 0;
	shouldBeSet = (keyc->state & LockMask) != 0;
	if (isSet != shouldBeSet)
	{
	    KdSetLed (kdKeyboardFuncs->LockLed, shouldBeSet);
	}
    }
}

void
KdEnqueueKeyboardEvent(unsigned char	scan_code,
		       unsigned char	is_up)
{
    unsigned char   key_code;
    xEvent	    xE;
    int		    e;
    KeyClassPtr	    keyc;

    if (!pKdKeyboard)
	return;
    keyc = pKdKeyboard->key;

    xE.u.keyButtonPointer.time = GetTimeInMillis();

    if (kdMinScanCode <= scan_code && scan_code <= kdMaxScanCode)
    {
	key_code = scan_code + KD_MIN_KEYCODE - kdMinScanCode;
	
	/*
	 * Set up this event -- the type may be modified below
	 */
	if (is_up)
	    xE.u.u.type = KeyRelease;
	else
	    xE.u.u.type = KeyPress;
	xE.u.u.detail = key_code;
	
	switch (KEYCOL1(key_code)) 
	{
	case XK_Num_Lock:
	case XK_Scroll_Lock:
	case XK_Shift_Lock:
	case XK_Caps_Lock:
	    if (xE.u.u.type == KeyRelease)
		return;
	    if (IsKeyDown (key_code))
		xE.u.u.type = KeyRelease;
	    else
		xE.u.u.type = KeyPress;
	}
	
	/*
	 * Check pressed keys which are already down
	 */
	if (IsKeyDown (key_code) && xE.u.u.type == KeyPress)
	{
	    KeybdCtrl	*ctrl = &pKdKeyboard->kbdfeed->ctrl;
	    
	    /*
	     * Check auto repeat
	     */
	    if (!ctrl->autoRepeat || keyc->modifierMap[key_code] ||
		!(ctrl->autoRepeats[key_code >> 3] & (1 << (key_code & 7))))
	    {
		return;
	    }
	    /*
	     * X delivers press/release even for autorepeat
	     */
	    xE.u.u.type = KeyRelease;
	    KdHandleKeyboardEvent (&xE);
	    xE.u.u.type = KeyPress;
	}
	/*
	 * Check released keys which are already up
	 */
	else if (!IsKeyDown (key_code) && xE.u.u.type == KeyRelease)
	{
	    return;
	}
	KdCheckSpecialKeys (&xE);
	KdHandleKeyboardEvent (&xE);
    }
}

#define SetButton(b,v, s) \
{\
    xE.u.u.detail = b; \
    xE.u.u.type = v; \
    KdHandleEvent (&xE); \
}

#define Press(b)         SetButton(b+1,ButtonPress,"Down")
#define Release(b)       SetButton(b+1,ButtonRelease,"Up")

static unsigned char	kdButtonState = 0;

/*
 * kdEnqueueMouseEvent
 *
 * This function converts hardware mouse event information into X event
 * information.  A mouse movement event is passed off to MI to generate
 * a MotionNotify event, if appropriate.  Button events are created and
 * passed off to MI for enqueueing.
 */

static int
KdMouseAccelerate (DeviceIntPtr	device, int delta)
{
    PtrCtrl *pCtrl = &device->ptrfeed->ctrl;

    if (abs(delta) > pCtrl->threshold)
	delta = (delta * pCtrl->num) / pCtrl->den;
    return delta;
}

void
KdEnqueueMouseEvent(unsigned long flags, int rx, int ry)
{
    CARD32  ms;
    xEvent  xE;
    unsigned char	buttons;
    int	    x, y;
    int	    (*matrix)[3] = kdMouseMatrix.matrix;

    if (!pKdPointer)
	return;
    
    ms = GetTimeInMillis();
    
    if (flags & KD_MOUSE_DELTA)
    {
	x = matrix[0][0] * rx + matrix[0][1] * ry;
	y = matrix[1][0] * rx + matrix[1][1] * ry;
	x = KdMouseAccelerate (pKdPointer, x);
	y = KdMouseAccelerate (pKdPointer, y);
	xE.u.keyButtonPointer.pad1 = 1;
    }
    else
    {
	x = matrix[0][0] * rx + matrix[0][1] * ry + matrix[0][2];
	y = matrix[1][0] * rx + matrix[1][1] * ry + matrix[1][2];
	xE.u.keyButtonPointer.pad1 = 0;
    }
    xE.u.keyButtonPointer.time = ms;
    xE.u.keyButtonPointer.rootX = x;
    xE.u.keyButtonPointer.rootY = y;

    xE.u.u.type = MotionNotify;
    xE.u.u.detail = 0;
    KdHandleEvent (&xE);

    buttons = flags;

    if ((kdButtonState & KD_BUTTON_1) ^ (buttons & KD_BUTTON_1))
    {
	if (buttons & KD_BUTTON_1)
	{
	    Press(0);
	}
	else
	{
	    Release(0);
	}
    }
    if ((kdButtonState & KD_BUTTON_2) ^ (buttons & KD_BUTTON_2)) 
    {
	if (buttons & KD_BUTTON_2)
	{
	    Press(1);
	}
	else
	{
	    Release(1);
	}
    }
    if ((kdButtonState & KD_BUTTON_3) ^ (buttons & KD_BUTTON_3))
    {
	if (buttons & KD_BUTTON_3)
	{
	    Press(2);
	}
	else
	{
	    Release(2);
	}
    }
    kdButtonState = buttons;
}

void
KdEnqueueMotionEvent (int x, int y)
{
    xEvent  xE;
    CARD32  ms;
    
    ms = GetTimeInMillis();
    
    xE.u.u.type = MotionNotify;
    xE.u.keyButtonPointer.time = ms;
    xE.u.keyButtonPointer.rootX = x;
    xE.u.keyButtonPointer.rootY = y;

    KdHandleEvent (&xE);
}

void
KdBlockHandler (int		screen,
		pointer		blockData,
		pointer		timeout,
		pointer		readmask)
{
    struct timeval	**pTimeout = timeout;
    
    if (kdTimeoutPending)
    {
	static struct timeval	tv;
	int			ms;

	ms = kdEmulationTimeout - GetTimeInMillis ();
	if (ms < 0)
	    ms = 0;
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000;
	if (*pTimeout)
	{
	    if ((*pTimeout)->tv_sec > tv.tv_sec ||
		((*pTimeout)->tv_sec == tv.tv_sec && 
		 (*pTimeout)->tv_usec > tv.tv_usec))
	    {
		*pTimeout = &tv;
	    }
	}
	else
	    *pTimeout = &tv;
    }
}

void
KdWakeupHandler (int		screen, 
		 pointer    	data,
		 unsigned long	lresult,
		 pointer	readmask)
{
    int	    result = (int) lresult;
    fd_set  *pReadmask = (fd_set *) readmask;
    
    if (kdInputEnabled && result > 0)
    {
	if (kdMouseFd >= 0 && FD_ISSET (kdMouseFd, pReadmask))
	{
	    KdBlockSigio ();
	    (*kdMouseFuncs->Read) (kdMouseFd);
	    KdUnblockSigio ();
	}
#ifdef TOUCHSCREEN
	if (kdTsFd >= 0 && FD_ISSET (kdTsFd, pReadmask))
	{
	    KdBlockSigio ();
	    (*kdTsFuncs->Read) (kdTsFd);
	    KdUnblockSigio ();
	}
#endif
	if (kdKeyboardFd >= 0 && FD_ISSET (kdKeyboardFd, pReadmask))
	{
	    KdBlockSigio ();
	    (*kdKeyboardFuncs->Read) (kdKeyboardFd);
	    KdUnblockSigio ();
	}
    }
    if (kdTimeoutPending)
    {
	if ((long) (GetTimeInMillis () - kdEmulationTimeout) >= 0)
	{
	    kdTimeoutPending = FALSE;
	    KdBlockSigio ();
	    KdReceiveTimeout ();
	    KdUnblockSigio ();
	}
    }
    if (kdSwitchPending)
	KdProcessSwitch ();
}

static Bool
KdCursorOffScreen(ScreenPtr *ppScreen, int *x, int *y)
{
    ScreenPtr	pScreen  = *ppScreen;
    int		n;
    CARD32	ms;
    
    if (kdDisableZaphod || screenInfo.numScreens <= 1)
	return FALSE;
    if (*x < 0 || *y < 0)
    {
        ms = GetTimeInMillis ();
	if (kdOffScreen && (int) (ms - kdOffScreenTime) < 1000)
	    return FALSE;
	kdOffScreen = TRUE;
	kdOffScreenTime = ms;
	
	n = pScreen->myNum - 1;
	if (n < 0)
	    n = screenInfo.numScreens - 1;
	pScreen = screenInfo.screens[n];
	if (*x < 0)
	    *x += pScreen->width;
	if (*y < 0)
	    *y += pScreen->height;
	*ppScreen = pScreen;
	return TRUE;
    }
    else if (*x >= pScreen->width || *y >= pScreen->height)
    {
        ms = GetTimeInMillis ();
	if (kdOffScreen && (int) (ms - kdOffScreenTime) < 1000)
	    return FALSE;
	kdOffScreen = TRUE;
	kdOffScreenTime = ms;
	
	n = pScreen->myNum + 1;
	if (n >= screenInfo.numScreens)
	    n = 0;
	if (*x >= pScreen->width)
	    *x -= pScreen->width;
	if (*y >= pScreen->height)
	    *y -= pScreen->height;
	pScreen = screenInfo.screens[n];
	*ppScreen = pScreen;
	return TRUE;
    }
    return FALSE;
}

static void
KdCrossScreen(ScreenPtr pScreen, Bool entering)
{
    if (entering)
	KdEnableScreen (pScreen);
    else
	KdDisableScreen (pScreen);
}

static void
KdWarpCursor (ScreenPtr pScreen, int x, int y)
{
    KdBlockSigio ();
    miPointerWarpCursor (pScreen, x, y);
    KdUnblockSigio ();
}

miPointerScreenFuncRec kdPointerScreenFuncs = 
{
    KdCursorOffScreen,
    KdCrossScreen,
    KdWarpCursor
};

void
ProcessInputEvents ()
{
    (void)mieqProcessInputEvents();
    miPointerUpdate();
    if (kdSwitchPending)
	KdProcessSwitch ();
    KdCheckLock ();
}
