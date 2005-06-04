/*
 * $Id$
 *
 * Copyright © 2004 Keith Packard
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define NEED_EVENTS
#include <errno.h>
#include <linux/input.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xpoll.h>
#define XK_PUBLISHING
#include <X11/keysym.h>
#include "inputstr.h"
#include "kkeymap.h"
#include "scrnintstr.h"
#include "xegl.h"

#define NUM_EVENTS  128
#define ABS_UNSET   -65535

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define ISBITSET(x,y) ((x)[LONG(y)] & BIT(y))
#define OFF(x)   ((x)%BITS_PER_LONG)
#define LONG(x)  ((x)/BITS_PER_LONG)
#define BIT(x)	 (1 << OFF(x))
#define SETBIT(x,y) ((x)[LONG(y)] |= BIT(y))
#define CLRBIT(x,y) ((x)[LONG(y)] &= ~BIT(y))
#define ASSIGNBIT(x,y,z)    ((x)[LONG(y)] = ((x)[LONG(y)] & ~BIT(y)) | (z << OFF(y)))

typedef struct _kevdevMouse {
    /* current device state */
    int			    rel[REL_MAX + 1];
    int			    abs[ABS_MAX + 1];
    int			    prevabs[ABS_MAX + 1];
    long		    key[NBITS(KEY_MAX + 1)];

    /* supported device info */
    long		    relbits[NBITS(REL_MAX + 1)];
    long		    absbits[NBITS(ABS_MAX + 1)];
    long		    keybits[NBITS(KEY_MAX + 1)];
    struct input_absinfo    absinfo[ABS_MAX + 1];
    int			    max_rel;
    int			    max_abs;
} Kevdev;

static int flags = 0;

static void
EvdevMotion (KdMouseInfo    *mi)
{
    Kevdev		*ke = mi->driver;
    int			i;

    for (i = 0; i <= ke->max_rel; i++)
	if (ke->rel[i])
	{
            KdEnqueueMouseEvent (mi, flags | KD_MOUSE_DELTA, ke->rel[0], ke->rel[1]);
	    int a;
//	    ErrorF ("rel");
	    for (a = 0; a <= ke->max_rel; a++)
	    {
//		if (ISBITSET (ke->relbits, a))
//		    ErrorF (" %d=%d", a, ke->rel[a]);
		ke->rel[a] = 0;
	    }
//	    ErrorF ("\n");
	    break;
	}
    for (i = 0; i < ke->max_abs; i++)
	if (ke->abs[i] != ke->prevabs[i])
	{
            KdEnqueueMouseEvent (mi, flags, ke->abs[0], ke->abs[1]);
	    int a;
//	    ErrorF ("abs");
	    for (a = 0; a <= ke->max_abs; a++)
	    {
//		if (ISBITSET (ke->absbits, a))
//		    ErrorF (" %d=%d", a, ke->abs[a]);
		ke->prevabs[a] = ke->abs[a];
	    }
//	    ErrorF ("\n");
	    break;
	}
}

static void
EvdevRead (int evdevPort, void *closure)
{
    KdMouseInfo		*mi = closure;
    Kevdev		*ke = mi->driver;
    int			i, n, f;
    struct input_event	events[NUM_EVENTS];

    n = read (evdevPort, &events, NUM_EVENTS * sizeof (struct input_event));
    if (n <= 0)
	return;
    n /= sizeof (struct input_event);
    for (i = 0; i < n; i++)
    {
	switch (events[i].type) {
	case EV_SYN:
	    break;
	case EV_KEY:
	    EvdevMotion (mi);
	    ASSIGNBIT(ke->key,events[i].code, events[i].value);
	    if (events[i].code < 0x100)
		ErrorF ("key %d %d\n", events[i].code, events[i].value);
	    else
		ErrorF ("key 0x%x %d\n", events[i].code, events[i].value);
            switch (events[i].code) {
            case BTN_LEFT:
                f = KD_BUTTON_1;
                break;
            case BTN_RIGHT:
                f = KD_BUTTON_2;
                break;
            case BTN_MIDDLE:
                f = KD_BUTTON_3;
                break;
            case BTN_FORWARD:
                f = KD_BUTTON_4;
                break;
            case BTN_BACK:
                f = KD_BUTTON_5;
                break;
            }
            flags |= f;
            KdEnqueueMouseEvent (mi, KD_MOUSE_DELTA | flags, 0, 0);
            ErrorF("Flags is %x\n", flags);
	    break;
	case EV_REL:
            ErrorF("EV_REL\n");
	    ke->rel[events[i].code] += events[i].value;
	    break;
	case EV_ABS:
            ErrorF("EV_ABS\n");
	    ke->abs[events[i].code] = events[i].value;
	    break;
	}
    }
    EvdevMotion (mi);
}

int EvdevInputType;

char *kdefaultEvdev[] =  {
//    "/dev/input/event0",
//    "/dev/input/event1",
//    "/dev/input/event2",
//    "/dev/input/event3",
//    "/dev/input/event4",
    "/dev/input/event5",
};

#define NUM_DEFAULT_EVDEV    (sizeof (kdefaultEvdev) / sizeof (kdefaultEvdev[0]))

static Bool
EvdevInit (void)
{
    int		i;
    int		fd;
    KdMouseInfo	*mi, *next;
    int		n = 0;
    char	*prot;
    char        name[100];

    if (!EvdevInputType)
	EvdevInputType = KdAllocInputType ();

    for (mi = kdMouseInfo; mi; mi = next)
    {
	next = mi->next;
	prot = mi->prot;
	if (mi->inputType)
	    continue;
	if (!mi->name)
	{
	    for (i = 0; i < NUM_DEFAULT_EVDEV; i++)
	    {
		fd = open (kdefaultEvdev[i], 2);
		if (fd >= 0)
		{
                    ioctl(fd, EVIOCGRAB, 1);

                    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
                    ErrorF("Name is %s\n", name);
                    ioctl(fd, EVIOCGPHYS(sizeof(name)), name);
                    ErrorF("Phys Loc is %s\n", name);
                    ioctl(fd, EVIOCGUNIQ(sizeof(name)), name);
                    ErrorF("Unique is %s\n", name);

		    mi->name = KdSaveString (kdefaultEvdev[i]);
		    break;
		}
	    }
	}
	else
	    fd = open (mi->name, 2);

	if (fd >= 0)
	{
	    unsigned long   ev[NBITS(EV_MAX)];
	    Kevdev	    *ke;

 	    if (ioctl (fd, EVIOCGBIT(0 /*EV*/, sizeof (ev)), ev) < 0)
	    {
		perror ("EVIOCGBIT 0");
		close (fd);
		continue;
	    }
	    ke = xalloc (sizeof (Kevdev));
	    if (!ke)
	    {
		close (fd);
		continue;
	    }
	    memset (ke, '\0', sizeof (Kevdev));
	    if (ISBITSET (ev, EV_KEY))
	    {
		if (ioctl (fd, EVIOCGBIT (EV_KEY, sizeof (ke->keybits)),
			   ke->keybits) < 0)
		{
		    perror ("EVIOCGBIT EV_KEY");
		    xfree (ke);
		    close (fd);
		    continue;
		}
	    }
	    if (ISBITSET (ev, EV_REL))
	    {
		if (ioctl (fd, EVIOCGBIT (EV_REL, sizeof (ke->relbits)),
			   ke->relbits) < 0)
		{
		    perror ("EVIOCGBIT EV_REL");
		    xfree (ke);
		    close (fd);
		    continue;
		}
		for (ke->max_rel = REL_MAX; ke->max_rel >= 0; ke->max_rel--)
		    if (ISBITSET(ke->relbits, ke->max_rel))
			break;
	    }
	    if (ISBITSET (ev, EV_ABS))
	    {
		int i;

		if (ioctl (fd, EVIOCGBIT (EV_ABS, sizeof (ke->absbits)),
			   ke->absbits) < 0)
		{
		    perror ("EVIOCGBIT EV_ABS");
		    xfree (ke);
		    close (fd);
		    continue;
		}
		for (ke->max_abs = ABS_MAX; ke->max_abs >= 0; ke->max_abs--)
		    if (ISBITSET(ke->absbits, ke->max_abs))
			break;
		for (i = 0; i <= ke->max_abs; i++)
		{
		    if (ISBITSET (ke->absbits, i))
			if (ioctl (fd, EVIOCGABS(i), &ke->absinfo[i]) < 0)
			{
			    perror ("EVIOCGABS");
			    break;
			}
		    ke->prevabs[i] = ABS_UNSET;
		}
		if (i <= ke->max_abs)
		{
		    xfree (ke);
		    close (fd);
		    continue;
		}
	    }
	    mi->driver = ke;
	    mi->inputType = EvdevInputType;
	    if (KdRegisterFd (EvdevInputType, fd, EvdevRead, (void *) mi))
		n++;
	}
    }
    return TRUE;
}

static void
EvdevFini (void)
{
    KdMouseInfo	*mi;

    KdUnregisterFds (EvdevInputType, TRUE);
    for (mi = kdMouseInfo; mi; mi = mi->next)
    {
	if (mi->inputType == EvdevInputType)
	{
	    xfree (mi->driver);
	    mi->driver = 0;
	    mi->inputType = 0;
	}
    }
}

KdMouseFuncs LinuxEvdevMouseFuncs = {
    EvdevInit,
    EvdevFini,
};


static void EvdevKbdLoad(void)
{
}

static const KeySym linux_to_x[128] = {
/*KEY_RESERVED*/ NoSymbol,
/*KEY_ESC*/ XK_Escape,
/*KEY_1*/ XK_1,
/*KEY_2*/ XK_2,
/*KEY_3*/ XK_3,
/*KEY_4*/ XK_4,
/*KEY_5*/ XK_5,
/*KEY_6*/ XK_6,
/*KEY_7*/ XK_7,
/*KEY_8*/ XK_8,
/*KEY_9*/ XK_9,
/*KEY_0*/ XK_0,
/*KEY_MINUS*/ XK_minus,
/*KEY_EQUAL*/ XK_equal,
/*KEY_BACKSPACE*/ XK_BackSpace,
/*KEY_TAB*/ XK_Tab,
/*KEY_Q*/ XK_Q,
/*KEY_W*/ XK_W,
/*KEY_E*/ XK_E,
/*KEY_R*/ XK_R,
/*KEY_T*/ XK_T,
/*KEY_Y*/ XK_Y,
/*KEY_U*/ XK_U,
/*KEY_I*/ XK_I,
/*KEY_O*/ XK_O,
/*KEY_P*/ XK_P,
/*KEY_LEFTBRACE*/ XK_braceleft,
/*KEY_RIGHTBRACE*/ XK_braceright,
/*KEY_ENTER*/ XK_Return,
/*KEY_LEFTCTRL*/ XK_Control_L,
/*KEY_A*/ XK_A,
/*KEY_S*/ XK_S,
/*KEY_D*/ XK_D,
/*KEY_F*/ XK_F,
/*KEY_G*/ XK_G,
/*KEY_H*/ XK_H,
/*KEY_J*/ XK_J,
/*KEY_K*/ XK_K,
/*KEY_L*/ XK_L,
/*KEY_SEMICOLON*/ XK_semicolon,
/*KEY_APOSTROPHE*/ XK_apostrophe,
/*KEY_GRAVE*/ XK_grave,
/*KEY_LEFTSHIFT*/ XK_Shift_L,
/*KEY_BACKSLASH*/ XK_backslash,
/*KEY_Z*/ XK_Z,
/*KEY_X*/ XK_X,
/*KEY_C*/ XK_C,
/*KEY_V*/ XK_V,
/*KEY_B*/ XK_B,
/*KEY_N*/ XK_N,
/*KEY_M*/ XK_M,
/*KEY_COMMA*/ XK_comma,
/*KEY_DOT*/ XK_period,
/*KEY_SLASH*/ XK_slash,
/*KEY_RIGHTSHIFT*/ XK_Shift_R,
/*KEY_KPASTERISK*/ XK_KP_Multiply,
/*KEY_LEFTALT*/ XK_Alt_L,
/*KEY_SPACE*/ XK_space,
/*KEY_CAPSLOCK*/ XK_Caps_Lock,
/*KEY_F1*/ XK_F1,
/*KEY_F2*/ XK_F2,
/*KEY_F3*/ XK_F3,
/*KEY_F4*/ XK_F4,
/*KEY_F5*/ XK_F5,
/*KEY_F6*/ XK_F6,
/*KEY_F7*/ XK_F7,
/*KEY_F8*/ XK_F8,
/*KEY_F9*/ XK_F9,
/*KEY_F10*/ XK_F10,
/*KEY_NUMLOCK*/ XK_Num_Lock,
/*KEY_SCROLLLOCK*/ XK_Scroll_Lock,
/*KEY_KP7*/ XK_KP_7,
/*KEY_KP8*/ XK_KP_8,
/*KEY_KP9*/ XK_KP_9,
/*KEY_KPMINUS*/ XK_KP_Subtract,
/*KEY_KP4*/ XK_KP_4,
/*KEY_KP5*/ XK_KP_5,
/*KEY_KP6*/ XK_KP_6,
/*KEY_KPPLUS*/ XK_KP_Add,
/*KEY_KP1*/ XK_KP_1,
/*KEY_KP2*/ XK_KP_2,
/*KEY_KP3*/ XK_KP_3,
/*KEY_KP0*/ XK_KP_0,
/*KEY_KPDOT*/ XK_KP_Decimal,
/*KEY_ZENKAKUHANKAKU*/ NoSymbol,
/*KEY_102ND*/ NoSymbol,
/*KEY_F11*/ XK_F11,
/*KEY_F12*/ XK_F12,
/*KEY_RO*/ NoSymbol,
/*KEY_KATAKANA*/ NoSymbol,
/*KEY_HIRAGANA*/ NoSymbol,
/*KEY_HENKAN*/ NoSymbol,
/*KEY_KATAKANAHIRAGANA*/ NoSymbol,
/*KEY_MUHENKAN*/ NoSymbol,
/*KEY_KPJPCOMMA*/ NoSymbol,
/*KEY_KPENTER*/ XK_KP_Enter,
/*KEY_RIGHTCTRL*/ XK_Control_R,
/*KEY_KPSLASH*/ XK_KP_Divide,
/*KEY_SYSRQ*/ NoSymbol,
/*KEY_RIGHTALT*/ XK_Alt_R,
/*KEY_LINEFEED*/ XK_Linefeed,
/*KEY_HOME*/ XK_Home,
/*KEY_UP*/ XK_Up,
/*KEY_PAGEUP*/ XK_Prior,
/*KEY_LEFT*/ XK_Left,
/*KEY_RIGHT*/ XK_Right,
/*KEY_END*/ XK_End,
/*KEY_DOWN*/ XK_Down,
/*KEY_PAGEDOWN*/ XK_Next,
/*KEY_INSERT*/ XK_Insert,
/*KEY_DELETE*/ XK_Delete,
/*KEY_MACRO*/ NoSymbol,
/*KEY_MUTE*/ NoSymbol,
/*KEY_VOLUMEDOWN*/ NoSymbol,
/*KEY_VOLUMEUP*/ NoSymbol,
/*KEY_POWER*/ NoSymbol,
/*KEY_KPEQUAL*/ NoSymbol,
/*KEY_KPPLUSMINUS*/ NoSymbol,
/*KEY_PAUSE*/ NoSymbol,
/*KEY_KPCOMMA*/ XK_KP_Separator,
/*KEY_HANGUEL*/ NoSymbol,
/*KEY_HANJA*/ NoSymbol,
/*KEY_YEN*/ NoSymbol,
/*KEY_LEFTMETA*/ NoSymbol,
/*KEY_RIGHTMETA*/ NoSymbol,
/*KEY_COMPOSE*/ NoSymbol,
};

static void
EvdevRead1 (int evdevPort, void *closure)
{
    int i, n, xk;
    struct input_event  events[NUM_EVENTS];

    n = read (evdevPort, &events, NUM_EVENTS * sizeof (struct input_event));
    if (n <= 0)
        return;
    n /= sizeof (struct input_event);
    for (i = 0; i < n; i++)
    {
        switch (events[i].type) {
        case EV_SYN:
            break;
        case EV_KEY:
            xk = NoSymbol;
            if (events[i].code < 128)
                xk = linux_to_x[events[i].code];
            if (events[i].code < 0x100)
                ErrorF ("key %d %d xk %d\n", events[i].code, events[i].value, xk);
            else
                ErrorF ("key 0x%x %d xk %d\n", events[i].code, events[i].value, xk);
            if (events[i].value == 2) {
                KdEnqueueKeyboardEvent (xk - KD_MIN_KEYCODE, 0);
                KdEnqueueKeyboardEvent (xk - KD_MIN_KEYCODE, 1);
            } else
                KdEnqueueKeyboardEvent (xk - KD_MIN_KEYCODE, !events[i].value);
            break;
        }
    }
}

char *kdefaultEvdev1[] =  {
//    "/dev/input/event0",
//    "/dev/input/event1",
//    "/dev/input/event2",
    "/dev/input/event3",
//    "/dev/input/event4",
//    "/dev/input/event5",
};

#define NUM_DEFAULT_EVDEV1    (sizeof (kdefaultEvdev1) / sizeof (kdefaultEvdev1[0]))

static Bool
EvdevKbdInit (void)
{
    int         i;
    int         fd;
    int         n = 0;
    char        name[100];

    kdMinScanCode = 0;
    kdMaxScanCode = 255;

    if (!EvdevInputType)
        EvdevInputType = KdAllocInputType ();

    for (i = 0; i < NUM_DEFAULT_EVDEV; i++)
    {
        fd = open (kdefaultEvdev1[i], 2);
        if (fd >= 0)
        {
            ioctl(fd, EVIOCGRAB, 1);

            ioctl(fd, EVIOCGNAME(sizeof(name)), name);
            ErrorF("Name is %s\n", name);
            ioctl(fd, EVIOCGPHYS(sizeof(name)), name);
            ErrorF("Phys Loc is %s\n", name);
            ioctl(fd, EVIOCGUNIQ(sizeof(name)), name);
            ErrorF("Unique is %s\n", name);

        }

        if (fd >= 0)
        {
            unsigned long   ev[NBITS(EV_MAX)];
            Kevdev          *ke;

            if (ioctl (fd, EVIOCGBIT(0 /*EV*/, sizeof (ev)), ev) < 0)
            {
                perror ("EVIOCGBIT 0");
                close (fd);
                continue;
            }
            ke = xalloc (sizeof (Kevdev));
            if (!ke)
            {
                close (fd);
                continue;
            }
            memset (ke, '\0', sizeof (Kevdev));
            if (ISBITSET (ev, EV_KEY))
            {
                if (ioctl (fd, EVIOCGBIT (EV_KEY, sizeof (ke->keybits)),
                           ke->keybits) < 0)
                {
                    perror ("EVIOCGBIT EV_KEY");
                    xfree (ke);
                    close (fd);
                    continue;
                }
            }
            if (ISBITSET (ev, EV_REL))
            {
                if (ioctl (fd, EVIOCGBIT (EV_REL, sizeof (ke->relbits)),
                           ke->relbits) < 0)
                {
                    perror ("EVIOCGBIT EV_REL");
                    xfree (ke);
                    close (fd);
                    continue;
                }
                for (ke->max_rel = REL_MAX; ke->max_rel >= 0; ke->max_rel--)
                    if (ISBITSET(ke->relbits, ke->max_rel))
                        break;
            }
            if (ISBITSET (ev, EV_ABS))
            {
                int i;

                if (ioctl (fd, EVIOCGBIT (EV_ABS, sizeof (ke->absbits)),
                           ke->absbits) < 0)
                {
                    perror ("EVIOCGBIT EV_ABS");
                    xfree (ke);
                    close (fd);
                    continue;
                }
                for (ke->max_abs = ABS_MAX; ke->max_abs >= 0; ke->max_abs--)
                    if (ISBITSET(ke->absbits, ke->max_abs))
                        break;
                for (i = 0; i <= ke->max_abs; i++)
                {
                    if (ISBITSET (ke->absbits, i))
                        if (ioctl (fd, EVIOCGABS(i), &ke->absinfo[i]) < 0)
                        {
                            perror ("EVIOCGABS");
                            break;
                        }
                    ke->prevabs[i] = ABS_UNSET;
                }
                if (i <= ke->max_abs)
                {
                    xfree (ke);
                    close (fd);
                    continue;
                }
            }
            if (KdRegisterFd (EvdevInputType, fd, EvdevRead1, NULL))
                n++;
        }
    }
    return TRUE;
}

static void
EvdevKbdFini (void)
{
}

static void
EvdevKbdLeds (int leds)
{
}


static void EvdevKbdBell(int volume, int pitch, int duration)
{
}


KdKeyboardFuncs LinuxEvdevKeyboardFuncs = {
    EvdevKbdLoad,
    EvdevKbdInit,
    EvdevKbdLeds,
    EvdevKbdBell,
    EvdevKbdFini,
    0,
};
