/*
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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "s3.h"

void
s3GetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
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

#ifndef S3_TRIO
#define Shift(v,d)  ((d) < 0 ? ((v) >> (-d)) : ((v) << (d)))

void
s3SetTrueChromaKey (ScreenPtr pScreen, int pfb, xColorItem *pdef)
{
    FbOverlayScrPrivPtr	pScrPriv = fbOverlayGetScrPriv(pScreen);
    KdScreenPriv(pScreen);
    s3ScreenInfo(pScreenPriv);
    int		fb, ma;
    CARD32	key;
    int		r, g, b;

    for (ma = 0; s3s->fbmap[ma] >= 0; ma++) 
    {
	fb = s3s->fbmap[ma];
	if (fb != pfb && pScreenPriv->screen->fb[fb].redMask)
	{
	    r = KdComputeCmapShift (pScreenPriv->screen->fb[fb].redMask);
	    g = KdComputeCmapShift (pScreenPriv->screen->fb[fb].greenMask);
	    b = KdComputeCmapShift (pScreenPriv->screen->fb[fb].blueMask);
	    key = ((Shift(pdef->red,r) & pScreenPriv->screen->fb[fb].redMask) |
		   (Shift(pdef->green,g) & pScreenPriv->screen->fb[fb].greenMask) |
		   (Shift(pdef->blue,b) & pScreenPriv->screen->fb[fb].blueMask));
	    if (pScrPriv->layer[fb].key != key)
	    {
		pScrPriv->layer[fb].key = key;
		(*pScrPriv->PaintKey) (&pScrPriv->layer[fb].u.run.pixmap->drawable,
				       &pScrPriv->layer[pfb].u.run.region,
				       pScrPriv->layer[fb].key, fb);
	    }
	}
    }
}
#endif

void
s3PutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    s3ScreenInfo(pScreenPriv);
    S3Vga	*s3vga = &s3c->s3vga;
    xColorItem	*chroma = 0;
    CARD32	key;

#if 0
    _s3WaitVRetrace (s3vga);
#else
    S3Ptr   s3 = s3c->s3;
    _s3WaitVRetraceFast(s3);
#endif
#ifndef S3_TRIO
    if (pScreenPriv->screen->fb[1].depth)
    {
	FbOverlayScrPrivPtr	pScrPriv = fbOverlayGetScrPriv(pScreen);
	key = pScrPriv->layer[fb].key;
    }
#endif
    else
	key = ~0;
    while (ndef--)
    {
	if (pdefs->pixel == key)
	    chroma = pdefs;
	s3SetImm (s3vga, s3_dac_write_index, pdefs->pixel);
	s3SetImm (s3vga, s3_dac_data, pdefs->red >> 8);
	s3SetImm (s3vga, s3_dac_data, pdefs->green >> 8);
	s3SetImm (s3vga, s3_dac_data, pdefs->blue >> 8);
	pdefs++;
    }
#ifndef S3_TRIO
    if (chroma && !pScreenPriv->closed)
	s3SetTrueChromaKey (pScreen, fb, chroma);
#endif
}

