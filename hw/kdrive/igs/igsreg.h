/*
 * $XFree86$
 *
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

#ifndef _IGSREG_H_
#define _IGSREG_H_

#include "vga.h"

#define IGS_SR	    0
#define IGS_NSR	    5
#define IGS_GR	    (IGS_SR+IGS_NSR)
#define IGS_NGR	    0xC0
#define IGS_GREX    (IGS_GR+IGS_NGR)
#define IGS_GREXBASE	0x3c
#define IGS_NGREX   1
#define IGS_AR	    (IGS_GREX+IGS_NGREX)
#define IGS_NAR	    0x15
#define IGS_CR	    (IGS_AR+IGS_NAR)
#define IGS_NCR	    0x48
#define IGS_DAC	    (IGS_CR+IGS_NCR)
#define IGS_NDAC    4
#define IGS_DACEX   (IGS_DAC+IGS_NDAC)
#define IGS_NDACEX  4
#define IGS_MISC_OUT	    (IGS_DACEX + IGS_NDACEX)
#define IGS_INPUT_STATUS_1  (IGS_MISC_OUT+1)
#define IGS_NREG	    (IGS_INPUT_STATUS_1+1)

#include "igsregs.t"

#define igsGet(sv,r)	    VgaGet(&(sv)->card, (r))
#define igsGetImm(sv,r)	    VgaGetImm(&(sv)->card, (r))
#define igsSet(sv,r,v)	    VgaSet(&(sv)->card, (r), (v))
#define igsSetImm(sv,r,v)    VgaSetImm(&(sv)->card, (r), (v))

typedef struct _igsVga {
    VgaCard	card;
    VgaValue	values[IGS_NREG];
} IgsVga;

void
igsRegInit (IgsVga *igsvga, VGAVOL8 *mmio);

void
igsSave (IgsVga *igsvga);

void
igsReset (IgsVga *igsvga);

#endif /* _IGSREG_H_ */
