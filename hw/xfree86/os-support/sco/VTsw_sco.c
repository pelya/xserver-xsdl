/* XFree86: $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 * Copyright 1993 by David McCullough <davidm@stallion.oz.au>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $Xorg: VTsw_sco.c,v 1.3 2000/08/17 19:51:28 cpqbld Exp $ */

#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/*
 * Handle the VT-switching interface for SCO
 */

/*
 * This function is the signal handler for the VT-switching signal.  It
 * is only referenced inside the OS-support layer.
 */
void xf86VTRequest(sig)
int sig;
{
	signal(sig, (void(*)())xf86VTRequest);
	xf86Info.vtRequestsPending = TRUE;
	return;
}

Bool xf86VTSwitchPending()
{
	return(xf86Info.vtRequestsPending ? TRUE : FALSE);
}

Bool xf86VTSwitchAway()
{
	xf86Info.vtRequestsPending = FALSE;
	if (ioctl(xf86Info.consoleFd, VT_RELDISP, 1) < 0)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}

Bool xf86VTSwitchTo()
{
	xf86Info.vtRequestsPending = FALSE;
	if (ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)
	{
		return(FALSE);
	}
	else
	{
		/*
		 * make sure the console driver thinks the console is in
		 * graphics mode.  Under mono we have to do the two as the
		 * console driver only allows valid modes for the current
		 * video card and Herc or vga are the only devices currently
		 * supported.
		 */
		if (ioctl(xf86Info.consoleFd, SW_VGA12, 0) < 0)
			if (ioctl(xf86Info.consoleFd, SW_HGC_P0, 0) < 0)
			{
				ErrorF("Failed to set graphics mode : %s\n",
				       strerror(errno));
			}

		return(TRUE);
	}
}
