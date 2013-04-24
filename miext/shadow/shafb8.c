/*
 *
 * Copyright © 2013 Geert Uytterhoeven
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Geert Uytterhoeven not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Geert Uytterhoeven makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * GEERT UYTTERHOEVEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL GEERT UYTTERHOEVEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Based on shpacked.c, which is Copyright © 2000 Keith Packard
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include    <X11/X.h>
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    <X11/fonts/font.h>
#include    "dixfontstr.h"
#include    <X11/fonts/fontstruct.h>
#include    "mi.h"
#include    "regionstr.h"
#include    "globals.h"
#include    "gcstruct.h"
#include    "shadow.h"
#include    "fb.h"
#include    "c2p_core.h"


    /*
     *  Perform a full C2P step on 32 8-bit pixels, stored in 8 32-bit words
     *  containing
     *    - 32 8-bit chunky pixels on input
     *    - permutated planar data (1 plane per 32-bit word) on output
     */

static void c2p_32x8(CARD32 d[8])
{
    transp8(d, 16, 4);
    transp8(d, 8, 2);
    transp8(d, 4, 1);
    transp8(d, 2, 4);
    transp8(d, 1, 2);
}


    /*
     *  Store a full block of permutated planar data after c2p conversion
     */

static inline void store_afb8(void *dst, unsigned int stride,
                              const CARD32 d[8])
{
    CARD8 *p = dst;

    *(CARD32 *)p = d[7]; p += stride;
    *(CARD32 *)p = d[5]; p += stride;
    *(CARD32 *)p = d[3]; p += stride;
    *(CARD32 *)p = d[1]; p += stride;
    *(CARD32 *)p = d[6]; p += stride;
    *(CARD32 *)p = d[4]; p += stride;
    *(CARD32 *)p = d[2]; p += stride;
    *(CARD32 *)p = d[0]; p += stride;
}


void
shadowUpdateAfb8(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    RegionPtr damage = shadowDamage(pBuf);
    PixmapPtr pShadow = pBuf->pPixmap;
    int nbox = RegionNumRects(damage);
    BoxPtr pbox = RegionRects(damage);
    FbBits *shaBase;
    CARD32 *shaLine, *sha;
    FbStride shaStride;
    int scrLine;
    _X_UNUSED int shaBpp, shaXoff, shaYoff;
    int x, y, w, h;
    int i, n;
    CARD32 *win;
    CARD32 off, winStride;
    union {
        CARD8 bytes[32];
        CARD32 words[8];
    } d;

    fbGetDrawable(&pShadow->drawable, shaBase, shaStride, shaBpp, shaXoff,
                  shaYoff);
    if (sizeof(FbBits) != sizeof(CARD32))
        shaStride = shaStride * sizeof(FbBits) / sizeof(CARD32);

    while (nbox--) {
        x = pbox->x1;
        y = pbox->y1;
        w = pbox->x2 - pbox->x1;
        h = pbox->y2 - pbox->y1;

        scrLine = x & -32;
        shaLine = (CARD32 *)shaBase + y * shaStride + scrLine / sizeof(CARD32);

        off = scrLine / 8;              /* byte offset in bitplane scanline */
        n = ((x & 31) + w + 31) / 32;   /* number of c2p units in scanline */

        while (h--) {
            sha = shaLine;
            win = (CARD32 *) (*pBuf->window) (pScreen,
                                              y,
                                              off,
                                              SHADOW_WINDOW_WRITE,
                                              &winStride,
                                              pBuf->closure);
            if (!win)
                return;
            for (i = 0; i < n; i++) {
                memcpy(d.bytes, sha, sizeof(d.bytes));
                c2p_32x8(d.words);
                store_afb8(win++, winStride, d.words);
                sha += sizeof(d.bytes) / sizeof(*sha);
            }
            shaLine += shaStride;
            y++;
        }
        pbox++;
    }
}
