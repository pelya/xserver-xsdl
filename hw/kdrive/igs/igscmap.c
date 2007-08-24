/*
 * Copyright © 2000 Keith Packard
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
#include "igs.h"

#define IGS_DAC_SHIFT	8
void
igsGetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    igsCardInfo(pScreenPriv);
    IgsVga   *igsvga = &igsc->igsvga;

    while (ndef--)
    {
	igsSetImm (igsvga, igs_dac_read_index, pdefs->pixel);
	pdefs->red = igsGetImm (igsvga, igs_dac_data) << IGS_DAC_SHIFT;
	pdefs->green = igsGetImm (igsvga, igs_dac_data) << IGS_DAC_SHIFT;
	pdefs->blue = igsGetImm (igsvga, igs_dac_data) << IGS_DAC_SHIFT;
	pdefs++;
    }
}

void
igsPutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    igsCardInfo(pScreenPriv);
    IgsVga   *igsvga = &igsc->igsvga;

    while (ndef--)
    {
	igsSetImm (igsvga, igs_dac_write_index, pdefs->pixel);
	igsSetImm (igsvga, igs_dac_data, pdefs->red >> IGS_DAC_SHIFT);
	igsSetImm (igsvga, igs_dac_data, pdefs->green >> IGS_DAC_SHIFT);
	igsSetImm (igsvga, igs_dac_data, pdefs->blue >> IGS_DAC_SHIFT);
	pdefs++;
    }
}

