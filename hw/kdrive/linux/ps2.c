/*
 * Id: ps2.c,v 1.1 1999/11/02 03:54:46 keithp Exp $
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

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "kdrive.h"
#include "Xpoll.h"

int
Ps2ReadBytes (int fd, char *buf, int len, int min)
{
    int		    n, tot;
    fd_set	    set;
    struct timeval  tv;

    tot = 0;
    while (len)
    {
	n = read (fd, buf, len);
	if (n > 0)
	{
	    tot += n;
	    buf += n;
	    len -= n;
	}
	if (tot % min == 0)
	    break;
	FD_ZERO (&set);
	FD_SET (fd, &set);
	tv.tv_sec = 0;
	tv.tv_usec = 100 * 1000;
	n = select (fd + 1, &set, 0, 0, &tv);
	if (n <= 0)
	    break;
    }
    return tot;
}

void
Ps2Read (int ps2Port)
{
    unsigned char   buf[3 * 200];
    unsigned char   *b;
    int		    n;
    int		    dx, dy;
    unsigned long   flags;

    while ((n = Ps2ReadBytes (ps2Port, buf, sizeof (buf), 3)) > 0)
    {
	b = buf;
	while (n >= 3)
	{
	    flags = KD_MOUSE_DELTA;
	    if (b[0] & 4)
		flags |= KD_BUTTON_2;
	    if (b[0] & 2)
		flags |= KD_BUTTON_3;
	    if (b[0] & 1)
		flags |= KD_BUTTON_1;
	    
	    dx = b[1];
	    if (b[0] & 0x10)
		dx -= 256;
	    dy = b[2];
	    if (b[0] & 0x20)
		dy -= 256;
	    dy = -dy;
	    n -= 3;
	    b += 3;
	    KdEnqueueMouseEvent (flags, dx, dy);
	}
    }
}

char	*Ps2Names[] = {
    "/dev/psaux",
    "/dev/mouse",
};

#define NUM_PS2_NAMES	(sizeof (Ps2Names) / sizeof (Ps2Names[0]))

int
Ps2Init (void)
{
    int	    i;
    int	    ps2Port;

    for (i = 0; i < NUM_PS2_NAMES; i++)
    {
	ps2Port = open (Ps2Names[i], 0);
	if (ps2Port >= 0)
	    return ps2Port;
    }
}

void
Ps2Fini (int ps2Port)
{
    if (ps2Port >= 0)
	close (ps2Port);
}

KdMouseFuncs Ps2MouseFuncs = {
    Ps2Init,
    Ps2Read,
    Ps2Fini
};
