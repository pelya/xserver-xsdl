/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsd_io.c,v 3.12 1996/12/23 06:49:37 dawes Exp $ */
/*
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Rich Murphey and David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Rich Murphey and
 * David Dawes make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * RICH MURPHEY AND DAVID DAWES DISCLAIM ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL RICH MURPHEY OR DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: bsd_io.c,v 1.3 2000/08/17 19:51:21 cpqbld Exp $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "compiler.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"

void xf86SoundKbdBell(loudness, pitch, duration)
int loudness;
int pitch;
int duration;
{
    	if (loudness && pitch)
	{
#ifdef CODRV_SUPPORT
		struct kbd_sound s;
#endif
#ifdef PCCONS_SUPPORT
		int data[2];
#endif

	    	switch (xf86Info.consType) {

#ifdef PCCONS_SUPPORT
	    	case PCCONS:
		    	data[0] = pitch;
		    	data[1] = (duration * loudness) / 50;
		    	ioctl(xf86Info.consoleFd, CONSOLE_X_BELL, data);
			break;
#endif
#ifdef CODRV_SUPPORT
	    	case CODRV011:
	    	case CODRV01X:
		    	s.pitch = pitch;
		    	s.duration = (duration * loudness) / 50;
		    	ioctl(xf86Info.consoleFd, KBDSETBELL, &s);
			break;
#endif
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	    	case SYSCONS:
		case PCVT:
			ioctl(xf86Info.consoleFd, KDMKTONE,
			      ((1193190 / pitch) & 0xffff) |
			      (((unsigned long)duration*loudness/50)<<16));
			break;
#endif
	    	}
	}
}

void xf86SetKbdLeds(leds)
int leds;
{
	switch (xf86Info.consType) {

	case PCCONS:
		break;
#ifdef CODRV_SUPPORT
	case CODRV011:
	case CODRV01X:
	        leds = (leds&0x01)<<2 | leds&0x02 | (leds&0x04)>>2;
		ioctl(xf86Info.consoleFd, KBDSLEDS, &leds);
		break;
#endif
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
		ioctl(xf86Info.consoleFd, KDSETLED, leds);
		break;
#endif
	}
}

int xf86GetKbdLeds()
{
	int leds = 0;

	switch (xf86Info.consType) {

	case PCCONS:
		break;
#ifdef CODRV_SUPPORT
	case CODRV011:
	case CODRV01X:
		ioctl(xf86Info.consoleFd, KBDGLEDS, &leds);
	        leds = (leds&0x01)<<2 | leds&0x02 | (leds&0x04)>>2;
		break;
#endif
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
		ioctl(xf86Info.consoleFd, KDGETLED, &leds);
		break;
#endif
	}
	return(leds);
}

#if NeedFunctionPrototypes
void xf86SetKbdRepeat(char rad)
#else
void xf86SetKbdRepeat(rad)
char rad;
#endif
{
	switch (xf86Info.consType) {

	case PCCONS:
		break;
#ifdef CODRV_SUPPORT
	case CODRV011:
	case CODRV01X:
		ioctl(xf86Info.consoleFd, KBDSTPMAT, &rad);
		break;
#endif
#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
		ioctl(xf86Info.consoleFd, KDSETRAD, rad);
		break;
#endif
	}
}

static struct termio kbdtty;

void xf86KbdInit()
{
	switch (xf86Info.consType) {

	case CODRV011:
	case CODRV01X:
		break;
#if defined(PCCONS_SUPPORT) || defined(SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case PCCONS:
	case SYSCONS:
	case PCVT:
		tcgetattr(xf86Info.consoleFd, &kbdtty);
		break;
#endif
	}
}

int xf86KbdOn()
{
	struct termios nTty;

	switch (xf86Info.consType) {

	case CODRV011:
	case CODRV01X:
		break;

#if defined(SYSCONS_SUPPORT) || defined(PCCONS_SUPPORT) || defined(PCVT_SUPPORT)
	case SYSCONS:
	case PCCONS:
	case PCVT:
		nTty = kbdtty;
		nTty.c_iflag = IGNPAR | IGNBRK;
		nTty.c_oflag = 0;
		nTty.c_cflag = CREAD | CS8;
		nTty.c_lflag = 0;
		nTty.c_cc[VTIME] = 0;
		nTty.c_cc[VMIN] = 1;
		cfsetispeed(&nTty, 9600);
		cfsetospeed(&nTty, 9600);
		tcsetattr(xf86Info.consoleFd, TCSANOW, &nTty);

#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
		ioctl(xf86Info.consoleFd, KDSKBMODE, K_RAW);
#endif
		break;
#endif
	}
	return(xf86Info.consoleFd);
}

int xf86KbdOff()
{
	switch (xf86Info.consType) {

	case CODRV011:
	case CODRV01X:
		break;

#if defined (SYSCONS_SUPPORT) || defined (PCVT_SUPPORT)
	case SYSCONS:
	case PCVT:
		ioctl(xf86Info.consoleFd, KDSKBMODE, K_XLATE);
		/* FALL THROUGH */
#endif
#if defined(SYSCONS_SUPPORT) || defined(PCCONS_SUPPORT) || defined(PCVT_SUPPORT)
	case PCCONS:
		tcsetattr(xf86Info.consoleFd, TCSANOW, &kbdtty);
		break;
#endif
	}
	return(xf86Info.consoleFd);
}

void xf86MouseInit(mouse)
MouseDevPtr mouse;
{
	return;
}

int xf86MouseOn(mouse)
MouseDevPtr mouse;
{
	if ((mouse->mseFd = open(mouse->mseDevice, O_RDWR | O_NDELAY)) < 0)
	{
		if (xf86AllowMouseOpenFail) {
			ErrorF("Cannot open mouse (%s) - Continuing...\n",
				strerror(errno));
			return(-2);
		}
		FatalError("Cannot open mouse (%s)\n", strerror(errno));
	}

	xf86SetupMouse(mouse);

	/* Flush any pending input */
	tcflush(mouse->mseFd, TCIFLUSH);

	return(mouse->mseFd);
}
