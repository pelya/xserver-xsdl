/*
 * Copyright 1993 by Thomas Mueller
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Mueller not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Mueller makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS MUELLER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS MUELLER BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/lynxos/lynx_io.c,v 3.10 2003/02/17 15:11:57 dawes Exp $ */

#include "X.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#if defined(KDMKTONE) || defined(KIOCSOUND)
/* Lynx 2.2.1 has sophisticated atc stuff.... */
void
xf86SoundKbdBell(int loudness, int pitch, int duration)
{
	if (loudness && pitch)
	{
#ifdef KDMKTONE
		/*
		 * If we have KDMKTONE use it to avoid putting the server
		 * to sleep
		 */
		ioctl(xf86Info.consoleFd, KDMKTONE,
		      (pitch & 0xffff) |
		      (((unsigned long)duration *
			loudness / 50) << 16));
#else
		ioctl(xf86Info.consoleFd, KIOCSOUND, pitch);
		usleep(xf86Info.bell_duration * loudness * 20);
		ioctl(xf86Info.consoleFd, KIOCSOUND, 0);
#endif
	}
}

#else

/* this is pulled from /sys/drivers/vt100/atbeep.c */

#define	SPEAKER_CONTROL	0x61
#define	TIMER_CONTROL	0x43
#define	TIMER_DATA	0x42
#define	TIMER_LOAD_CMD	0xb6

#define	TIMER_CONSTANT	1193280
#define	FREQ_LO(f)	((TIMER_CONSTANT / (f)) % 256)
#define	FREQ_HI(f)	((TIMER_CONSTANT / (f)) / 256)

void
xf86SoundKbdBell(int loudness, int pitch, int duration)
{
	int	flo = FREQ_LO(pitch);
	int	fhi = FREQ_HI(pitch);

	outb(TIMER_CONTROL, TIMER_LOAD_CMD);
	outb(TIMER_DATA, flo);
	outb(TIMER_DATA, fhi);

	/* speaker on */
	outb(SPEAKER_CONTROL, inb(SPEAKER_CONTROL) | 3);
	usleep(xf86Info.bell_duration * loudness * 20);
	/* speaker off */
	outb(SPEAKER_CONTROL, inb(SPEAKER_CONTROL) & ~3);
}
#endif

void
xf86SetKbdLeds(int leds)
{
#ifdef KBD_SET_LEDS
	ioctl(xf86Info.consoleFd, KBD_SET_LEDS, &leds);
#endif
}

int
xf86GetKbdLeds()
{
#ifdef KBD_SET_LEDS
	int leds;

	if (ioctl(xf86Info.consoleFd, KBD_SET_LEDS, &leds) < 0)
		return 0;

	return leds;
#endif
	return 0;
}

void
xf86SetKbdRepeat(char rad)
{
}

static struct termio kbdtty;

void
xf86KbdInit()
{
	ioctl(xf86Info.consoleFd, TCGETA, &kbdtty);
}

int
xf86KbdOn()
{
	struct termio nTty;

	/* set CAPS_LOCK to behave as CAPS_LOCK not as CTRL */
	write(xf86Info.consoleFd, "\033<", 2);

	/* enable scan mode */
	ioctl(xf86Info.consoleFd, TIO_ENSCANMODE, NULL);

	nTty = kbdtty;
	nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
	nTty.c_oflag = 0;
	nTty.c_cflag = CREAD | CS8;
	nTty.c_lflag = 0;
	nTty.c_cc[VTIME]=0;
	nTty.c_cc[VMIN]=1;
	ioctl(xf86Info.consoleFd, TCSETA, &nTty);

	return(xf86Info.consoleFd);
}

int
xf86KbdOff()
{
	/* disable scan mode */
	ioctl(xf86Info.consoleFd, TIO_DISSCANMODE, NULL);
	ioctl(xf86Info.consoleFd, TCSETA, &kbdtty);
	return(xf86Info.consoleFd);
}

#include "xf86OSKbd.h"

Bool
xf86OSKbdPreInit(InputInfoPtr pInfo)
{
    return FALSE;
}
