/*
 * Id: s3cmap.c,v 1.2 1999/11/02 06:16:29 keithp Exp $
 *
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */
/* $XFree86: xc/programs/Xserver/hw/kdrive/savage/s3cmap.c,v 1.2 1999/12/30 03:03:11 robin Exp $ */

#include "s3.h"

void
s3GetColors (ScreenPtr pScreen, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    S3Vga   *s3vga = &s3c->s3vga;

    while (ndef--)
    {
	s3SetImm (s3vga, s3_dac_read_index, pdefs->pixel);
	pdefs->red = s3GetImm (s3vga, s3_dac_data) << 8;
	pdefs->green = s3GetImm (s3vga, s3_dac_data) << 8;
	pdefs->blue = s3GetImm (s3vga, s3_dac_data) << 8;
	pdefs++;
    }
}

void
s3PutColors (ScreenPtr pScreen, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    s3ScreenInfo(pScreenPriv);
    S3Vga   *s3vga = &s3c->s3vga;
    Bool    hit_border = FALSE;
    Bool    check_border = FALSE;

    _s3WaitVRetrace (s3vga);
    if (pScreenPriv->enabled && s3s->manage_border && !s3s->managing_border)
	check_border = TRUE;
    while (ndef--)
    {
	if (check_border && pdefs->pixel == s3s->border_pixel)
	{
	    if (pdefs->red || pdefs->green || pdefs->blue)
		hit_border = TRUE;
	}
	s3SetImm (s3vga, s3_dac_write_index, pdefs->pixel);
	s3SetImm (s3vga, s3_dac_data, pdefs->red >> 8);
	s3SetImm (s3vga, s3_dac_data, pdefs->green >> 8);
	s3SetImm (s3vga, s3_dac_data, pdefs->blue >> 8);
	pdefs++;
    }
    if (hit_border)
    {
	xColorItem  black;

	black.red = 0;
	black.green = 0;
	black.blue = 0;
	s3s->managing_border = TRUE;
	FakeAllocColor (pScreenPriv->pInstalledmap,
			&black);
	s3s->border_pixel = black.pixel;
	FakeFreeColor (pScreenPriv->pInstalledmap, s3s->border_pixel);
/*	s3SetImm (&s3c->s3vga, s3_border_color, (VGA8) s3s->border_pixel); */
    }
}

