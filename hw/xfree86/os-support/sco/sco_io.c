/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sco/sco_io.c,v 3.3 1996/12/23 06:50:49 dawes Exp $ */
/*
 * Copyright 1993 by David McCullough <davidm@stallion.oz.au>
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of David McCullough and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  David McCullough
 * and David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * DAVID MCCULLOUGH AND DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL DAVID MCCULLOUGH OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: sco_io.c,v 1.3 2000/08/17 19:51:29 cpqbld Exp $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

void xf86SoundKbdBell(loudness, pitch, duration)
int loudness;
int pitch;
int duration;
{
	if (loudness && pitch)
	{
		ioctl(xf86Info.consoleFd, KIOCSOUND, 1193180 / pitch);
		usleep(duration * loudness * 20);
		ioctl(xf86Info.consoleFd, KIOCSOUND, 0);
	}
}

void xf86SetKbdLeds(leds)
int leds;
{
	/*
	 * sleep the first time through under SCO.  There appears to be a
	 * timing problem in the driver which causes the keyboard to be lost.
	 * This sleep stops it from occurring.  The sleep could proably be
	 * a lot shorter as even trace can fix the problem.  You may
	 * prefer a usleep(100).
	 */
	static int once = 1;

	if (once)
	{
		sleep(1);
		once = 0;
	}
	ioctl(xf86Info.consoleFd, KDSETLED, leds );
}

void xf86MouseInit(mouse)
MouseDevPtr mouse;
{
	if ((mouse->mseFd = open(mouse->mseDevice, O_RDWR | O_NDELAY)) < 0)
	{
		if (xf86AllowMouseOpenFail) {
			ErrorF("Cannot open mouse (%s) - Continuing...\n",
				strerror(errno));
			return;
		}
		FatalError("Cannot open mouse (%s)\n", strerror(errno));
	}
}

int xf86MouseOn(mouse)
MouseDevPtr mouse;
{
	xf86SetupMouse(mouse);

	/* Flush any pending input */
	ioctl(mouse->mseFd, TCFLSH, 0);

	return(mouse->mseFd);
}

int xf86MouseOff(mouse, doclose)
MouseDevPtr mouse;
Bool doclose;
{
	if (mouse->mseFd >= 0)
	{
		if (mouse->mseType == P_LOGI)
		{
			write(mouse->mseFd, "U", 1);
			xf86SetMouseSpeed(mouse, mouse->baudRate, 
					  mouse->oldBaudRate,
				  	  xf86MouseCflags[P_LOGI]);
		}
		if (doclose)
		{
			close(mouse->mseFd);
		}
	}
	return(mouse->mseFd);
}
