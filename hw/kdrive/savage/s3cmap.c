/*
 * $Id$
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
/* $XFree86: $ */

#include "s3.h"

void
s3GetColors (ScreenPtr pScreen, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    S3Ptr   s3 = s3c->s3;
    VOL8   *dac_rd_ad = &s3->crt_vga_dac_rd_ad;
    VOL8   *dac_data = &s3->crt_vga_dac_data;

    while (ndef--)
    {
	*dac_rd_ad = pdefs->pixel;
	pdefs->red = *dac_data << 8;
	pdefs->green = *dac_data << 8;
	pdefs->blue = *dac_data << 8;
	pdefs++;
    }
}

void
s3PutColors (ScreenPtr pScreen, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    S3Ptr   s3 = s3c->s3;
    VOL8   *dac_wt_ad = &s3->crt_vga_dac_wt_ad;
    VOL8   *dac_data = &s3->crt_vga_dac_data;

    _s3WaitVRetrace (s3);
    while (ndef--)
    {
	*dac_wt_ad = pdefs->pixel;
	*dac_data = pdefs->red >> 8;
	*dac_data = pdefs->green >> 8;
	*dac_data = pdefs->blue >> 8;
	pdefs++;
    }
}

