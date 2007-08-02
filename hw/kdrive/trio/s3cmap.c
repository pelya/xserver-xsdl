/*
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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "s3.h"

void
s3GetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    S3Ptr   s3 = s3c->s3;
    VOL8   *dac_rd_ad = &s3->crt_vga_dac_rd_ad;
    VOL8   *dac_data = &s3->crt_vga_dac_data;

    LockS3 (s3c);
    while (ndef--)
    {
	*dac_rd_ad = pdefs->pixel;
	pdefs->red = *dac_data << 10;
	pdefs->green = *dac_data << 10;
	pdefs->blue = *dac_data << 10;
	pdefs++;
    }
    UnlockS3(s3c);
}

void
s3PutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    S3Ptr   s3 = s3c->s3;
    VOL8   *dac_wt_ad = &s3->crt_vga_dac_wt_ad;
    VOL8   *dac_data = &s3->crt_vga_dac_data;

    LockS3(s3c);
    _s3WaitVRetrace (s3);
    while (ndef--)
    {
	*dac_wt_ad = pdefs->pixel;
	*dac_data = pdefs->red >> 10;
	*dac_data = pdefs->green >> 10;
	*dac_data = pdefs->blue >> 10;
	pdefs++;
    }
    UnlockS3(s3c);
}

