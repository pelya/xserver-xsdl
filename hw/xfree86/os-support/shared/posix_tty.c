/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/posix_tty.c,v 3.8.2.1 1998/02/07 14:27:25 dawes Exp $ */
/*
 * Copyright 1993 by David Dawes <dawes@physics.su.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Dawes 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.
 * David Dawes makes no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * DAVID DAWES DISCLAIMS ALL WARRANTIES WITH REGARD TO 
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL DAVID DAWES BE LIABLE FOR 
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER 
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF 
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: posix_tty.c,v 1.3 2000/08/17 19:51:30 cpqbld Exp $ */

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"

#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86_Config.h"

static Bool not_a_tty = FALSE;

void xf86SetMouseSpeed(mouse, old, new, cflag)
MouseDevPtr mouse;
int old;
int new;
unsigned cflag;
{
	struct termios tty;
	char *c;

	if (not_a_tty)
		return;

	if (tcgetattr(mouse->mseFd, &tty) < 0)
	{
		not_a_tty = TRUE;
		ErrorF("Warning: %s unable to get status of mouse fd (%s)\n",
		       mouse->mseDevice, strerror(errno));
		return;
	}

	/* this will query the initial baudrate only once */
	if (mouse->oldBaudRate < 0) { 
	   switch (cfgetispeed(&tty)) 
	      {
	      case B9600: 
		 mouse->oldBaudRate = 9600;
		 break;
	      case B4800: 
		 mouse->oldBaudRate = 4800;
		 break;
	      case B2400: 
		 mouse->oldBaudRate = 2400;
		 break;
	      case B1200: 
	      default:
		 mouse->oldBaudRate = 1200;
		 break;
	      }
	}

	tty.c_iflag = IGNBRK | IGNPAR;
	tty.c_oflag = 0;
	tty.c_lflag = 0;
	tty.c_cflag = (tcflag_t)cflag;
	tty.c_cc[VTIME] = 0;
	tty.c_cc[VMIN] = 1;

	switch (old)
	{
	case 9600:
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
		break;
	case 4800:
		cfsetispeed(&tty, B4800);
		cfsetospeed(&tty, B4800);
		break;
	case 2400:
		cfsetispeed(&tty, B2400);
		cfsetospeed(&tty, B2400);
		break;
	case 1200:
	default:
		cfsetispeed(&tty, B1200);
		cfsetospeed(&tty, B1200);
	}

	if (tcsetattr(mouse->mseFd, TCSADRAIN, &tty) < 0)
	{
		if (xf86AllowMouseOpenFail) {
			ErrorF("Unable to set status of mouse fd (%s) - Continuing...\n",
			       strerror(errno));
			return;
		}
		xf86FatalError("Unable to set status of mouse fd (%s)\n",
			       strerror(errno));
	}

	switch (new)
	{
	case 9600:
		c = "*q";
		cfsetispeed(&tty, B9600);
		cfsetospeed(&tty, B9600);
		break;
	case 4800:
		c = "*p";
		cfsetispeed(&tty, B4800);
		cfsetospeed(&tty, B4800);
		break;
	case 2400:
		c = "*o";
		cfsetispeed(&tty, B2400);
		cfsetospeed(&tty, B2400);
		break;
	case 1200:
	default:
		c = "*n";
		cfsetispeed(&tty, B1200);
		cfsetospeed(&tty, B1200);
	}

	if (mouse->mseType == P_LOGIMAN || mouse->mseType == P_LOGI)
	{
		if (write(mouse->mseFd, c, 2) != 2)
		{
			if (xf86AllowMouseOpenFail) {
				ErrorF("Unable to write to mouse fd (%s) - Continuing...\n",
				       strerror(errno));
				return;
			}
			xf86FatalError("Unable to write to mouse fd (%s)\n",
				       strerror(errno));
		}
	}
	usleep(100000);

	if (tcsetattr(mouse->mseFd, TCSADRAIN, &tty) < 0)
	{
		if (xf86AllowMouseOpenFail) {
			ErrorF("Unable to set status of mouse fd (%s) - Continuing...\n",
			       strerror(errno));
			return;
		}
		xf86FatalError("Unable to set status of mouse fd (%s)\n",
			       strerror(errno));
	}
}

int
xf86FlushInput(fd)
int fd;
{
	return tcflush(fd, TCIFLUSH);
}

