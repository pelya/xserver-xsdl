/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/ioperm_noop.c,v 3.1 1996/12/23 06:50:59 dawes Exp $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
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
/* $Xorg: ioperm_noop.c,v 1.3 2000/08/17 19:51:30 cpqbld Exp $ */

/*
 * Amoeba, Minix and 386BSD don't bother with I/O permissions, 
 * or the permissions are implicit with opening/enabling the console.
 */
void xf86ClearIOPortList(ScreenNum)
int ScreenNum;
{
	return;
}

/* ARGSUSED */
void xf86AddIOPorts(ScreenNum, NumPorts, Ports)
int ScreenNum;
int NumPorts;
unsigned *Ports;
{
	return;
}

void xf86EnableIOPorts(ScreenNum)
int ScreenNum;
{
	return;
}

void xf86DisableIOPorts(ScreenNum)
int ScreenNum;
{
	return;
}

void xf86DisableIOPrivs()
{
	return;
}
