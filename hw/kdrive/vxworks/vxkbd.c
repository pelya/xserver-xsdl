/*
 * Id: vxkbd.c,v 1.1 1999/11/24 08:35:24 keithp Exp $
 *
 * Copyright © 1999 Network Computing Devices, Inc.  All rights reserved.
 *
 * Author: Keith Packard
 */

#include "kdrive.h"
#include "kkeymap.h"
#include <X11/keysym.h>
#include <inputstr.h>

#define VXWORKS_WIDTH  2

KeySym VxWorksKeymap[] = {
/*7   f1	*/  XK_F1,	    NoSymbol,
/*8   escape	*/  XK_Escape,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*13  tab	*/  XK_Tab,	    NoSymbol,
/*14  `		*/  XK_grave,	    XK_asciitilde,
/*15  f2	*/  XK_F2,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*17  lctrl	*/  XK_Control_L,   NoSymbol,
/*18  lshift	*/  XK_Shift_L,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*20  lock	*/  XK_Caps_Lock,   NoSymbol,
/*21  q		*/  XK_Q,	    NoSymbol,
/*22  1		*/  XK_1,	    XK_exclam,
/*23  f3	*/  XK_F3,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*25  lalt	*/  XK_Meta_L,	    XK_Alt_L,
/*26  z		*/  XK_Z,	    NoSymbol,
/*27  s		*/  XK_S,	    NoSymbol,
/*28  a		*/  XK_A,	    NoSymbol,
/*29  w		*/  XK_W,	    NoSymbol,
/*30  2		*/  XK_2,	    XK_at,
/*31  f4	*/  XK_F4,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*33  c		*/  XK_C,	    NoSymbol,
/*34  x		*/  XK_X,	    NoSymbol,
/*35  d		*/  XK_D,	    NoSymbol,
/*36  e		*/  XK_E,	    NoSymbol,
/*37  4		*/  XK_4,	    XK_dollar,
/*38  3		*/  XK_3,	    XK_numbersign,
/*39  f5	*/  XK_F5,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*41  space	*/  XK_space,	    NoSymbol,
/*42  v		*/  XK_V,	    NoSymbol,
/*43  f		*/  XK_F,	    NoSymbol,
/*44  t		*/  XK_T,	    NoSymbol,
/*45  r		*/  XK_R,	    NoSymbol,
/*46  5		*/  XK_5,	    XK_percent,
/*47  f6	*/  XK_F6,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*49  n		*/  XK_N,	    NoSymbol,
/*50  b		*/  XK_B,	    NoSymbol,
/*51  h		*/  XK_H,	    NoSymbol,
/*52  g		*/  XK_G,	    NoSymbol,
/*53  y		*/  XK_Y,	    NoSymbol,
/*54  6		*/  XK_6,	    XK_asciicircum,
/*55  f7	*/  XK_F7,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*57  ralt	*/  XK_Meta_R,	    XK_Alt_R,
/*58  m		*/  XK_M,	    NoSymbol,
/*59  j		*/  XK_J,	    NoSymbol,
/*60  u		*/  XK_U,	    NoSymbol,
/*61  7		*/  XK_7,	    XK_ampersand,
/*62  8		*/  XK_8,	    XK_asterisk,
/*63  f8	*/  XK_F8,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*65  ,		*/  XK_comma,	    XK_less,
/*66  k		*/  XK_K,	    NoSymbol,
/*67  i		*/  XK_I,	    NoSymbol,
/*68  o		*/  XK_O,	    NoSymbol,
/*69  0		*/  XK_0,	    XK_parenright,
/*70  9		*/  XK_9,	    XK_parenleft,
/*71  f9	*/  XK_F9,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*73  .		*/  XK_period,	    XK_greater,
/*74  /		*/  XK_slash,	    XK_question,
/*75  l		*/  XK_L,	    NoSymbol,
/*76  ;		*/  XK_semicolon,   XK_colon,
/*77  p		*/  XK_P,	    NoSymbol,
/*78  -		*/  XK_minus,	    XK_underscore,
/*79  f10	*/  XK_F10,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*82  '		*/  XK_apostrophe,  XK_quotedbl,
		    NoSymbol,	    NoSymbol,
/*84  [		*/  XK_bracketleft, XK_braceleft,
/*85  =		*/  XK_equal,	    XK_plus,
/*86  f11	*/  XK_F11,	    NoSymbol,
/*87  sysrq	*/  XK_Sys_Req,	    XK_Print,
/*88  rctrl	*/  XK_Control_R,   NoSymbol,
/*89  rshift	*/  XK_Shift_R,	    NoSymbol,
/*90  enter	*/  XK_Return,	    NoSymbol,
/*91  ]		*/  XK_bracketright,	XK_braceright,
/*92  \		*/  XK_backslash,   XK_bar,
		    NoSymbol,	    NoSymbol,
/*94  f12	*/  XK_F12,	    NoSymbol,
/*95  scrolllock*/  XK_Scroll_Lock, NoSymbol,
/*96  down	*/  XK_Down,	    NoSymbol,
/*97  left	*/  XK_Left,	    NoSymbol,
/*98  pause	*/  XK_Break,	    XK_Pause,
/*99  up	*/  XK_Up,	    NoSymbol,
/*100 delete	*/  XK_Delete,	    NoSymbol,
/*101 end	*/  XK_End,	    NoSymbol,
/*102 bs	*/  XK_BackSpace,   NoSymbol,
/*103 insert	*/  XK_Insert,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*105 np 1	*/  XK_KP_End,	    XK_KP_1,
/*106 right	*/  XK_Right,	    NoSymbol,
/*107 np 4	*/  XK_KP_Left,	    XK_KP_4,
/*108 np 7	*/  XK_KP_Home,	    XK_KP_7,
/*109 pgdn	*/  XK_Page_Down,   NoSymbol,
/*110 home	*/  XK_Home,	    NoSymbol,
/*111 pgup	*/  XK_Page_Up,	    NoSymbol,
/*112 np 0	*/  XK_KP_Insert,   XK_KP_0,
/*113 np .	*/  XK_KP_Delete,   XK_KP_Decimal,
/*114 np 2	*/  XK_KP_Down,	    XK_KP_2,
/*115 np 5	*/  XK_KP_5,	    NoSymbol,
/*116 np 6	*/  XK_KP_Right,    XK_KP_6,
/*117 np 8	*/  XK_KP_Up,	    XK_KP_8,
/*118 numlock	*/  XK_Num_Lock,    NoSymbol,
/*119 np /	*/  XK_KP_Divide,   NoSymbol,
		    NoSymbol,	    NoSymbol,
/*121 np enter	*/  XK_KP_Enter,    NoSymbol,
/*122 np 3	*/  XK_KP_Page_Down,	XK_KP_3,
		    NoSymbol,	    NoSymbol,
/*124 np +	*/  XK_KP_Add,	    NoSymbol,
/*125 np 9	*/  XK_KP_Page_Up,  XK_KP_9,
/*126 np *	*/  XK_KP_Multiply, NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*132 np -	*/  XK_KP_Subtract, NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
		    NoSymbol,	    NoSymbol,
/*139 lwin	*/  XK_Super_L,	    NoSymbol,
/*140 rwin	*/  XK_Super_R,	    NoSymbol,
/*141 menu	*/  XK_Menu,	    NoSymbol,
};

void
VxWorksKeyboardLoad (void)
{
    KeySym  *k;

    kdMinScanCode = 7;
    kdKeymapWidth = VXWORKS_WIDTH;
    kdMaxScanCode = 141;
    memcpy (kdKeymap, VxWorksKeymap, sizeof (VxWorksKeymap));
}

static int  kbdFd = -1;

#include <errno.h>
#include <event.h>
#include <kbd_ioctl.h>

extern KeybdCtrl    defaultKeyboardControl;

static void
VxWorksSetAutorepeat (unsigned char *repeats, Bool on)
{
    int		    i;
    unsigned char   mask;
    int		    scan_code;
    int		    key_code;
    unsigned char   realkc;

    if (on)
    {
	realkc = 1;
	ioctl (kbdFd, KBD_ALL_REPEAT, &realkc);
	for (scan_code = 7; scan_code <= 141; scan_code++)
	{
	    key_code = scan_code + 1;
	    i = key_code >> 3;
	    mask = 1 << (key_code & 7);
	    if ((repeats[i] & mask) == 0)
	    {
		realkc = scan_code;
		ioctl (kbdFd, KBD_NO_REPEAT, &realkc);
	    }
	}
    }
    else
    {
	realkc = 0;
	ioctl (kbdFd, KBD_ALL_REPEAT, &realkc);
    }
}

int
VxWorksKeyboardInit (void)
{
    
    kbdFd = open ("/dev/kbd", O_RDONLY, 0);
    if (kbdFd < 0)
	ErrorF ("keyboard open failure %d\n", errno);
    VxWorksSetAutorepeat (defaultKeyboardControl.autoRepeats, TRUE);
    return -1;
}

void
VxWorksKeyboardFini (int fd)
{
    if (kbdFd >= 0)
    {
	close (kbdFd);
	kbdFd = -1;
    }
}

void
VxWorksKeyboardRead (int fd)
{
}

void
VxWorksKeyboardLeds (int leds)
{
    DeviceIntPtr	pKeyboard = (DeviceIntPtr) LookupKeyboardDevice ();
    KeybdCtrl		*ctrl = &pKeyboard->kbdfeed->ctrl;
    led_ioctl_info	led_info;
    int			i;

    VxWorksSetAutorepeat (ctrl->autoRepeats, ctrl->autoRepeat);
    for (i = 0; i < 3; i++)
    {
	led_info.bit_n = 1 << i;
	led_info.OFF_or_ON = (leds & (1 << i)) != 0;
	led_info.reversed = 0;
	ioctl (kbdFd, KBD_SET_LED, &led_info);
    }
}

void
VxWorksKeyboardBell (int volume, int frequency, int duration)
{
}

KdKeyboardFuncs	VxWorksKeyboardFuncs = {
    VxWorksKeyboardLoad,
    VxWorksKeyboardInit,
    VxWorksKeyboardRead,
    VxWorksKeyboardLeds,
    VxWorksKeyboardBell,
    VxWorksKeyboardFini,
    3,
};
