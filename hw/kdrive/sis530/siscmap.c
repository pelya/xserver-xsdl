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
#include "sis.h"

void
sisGetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    sisCardInfo(pScreenPriv);

    LockSis (sisc);
    while (ndef--)
    {
	_sisOutb (pdefs->pixel, sisc->io_base+SIS_DAC_INDEX_READ);
	pdefs->red = _sisInb (sisc->io_base+SIS_DAC_DATA) << 10;
	pdefs->green = _sisInb (sisc->io_base+SIS_DAC_DATA) << 10;
	pdefs->blue = _sisInb (sisc->io_base+SIS_DAC_DATA) << 10;
	pdefs++;
    }
    UnlockSis (sisc);
}

void
sisPutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    sisCardInfo(pScreenPriv);

    LockSis(sisc);
    _sisWaitVRetrace (sisc);
    while (ndef--)
    {
	_sisOutb (pdefs->pixel, sisc->io_base+SIS_DAC_INDEX_WRITE);
	_sisOutb (pdefs->red >> 10, sisc->io_base+SIS_DAC_DATA);
	_sisOutb (pdefs->green >> 10, sisc->io_base+SIS_DAC_DATA);
	_sisOutb (pdefs->blue >> 10, sisc->io_base+SIS_DAC_DATA);
	pdefs++;
    }
    UnlockSis(sisc);
}

