/*
 * $XFree86$
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

#include "kdrive.h"

Bool
KdShadowScreenInit (KdScreenInfo *screen)
{
    void    *buf;

    buf = shadowAlloc (screen->width, screen->height, screen->fb[0].bitsPerPixel);
    if (!buf)
	return FALSE;
    screen->fb[0].frameBuffer = buf;
    screen->fb[0].byteStride = BitmapBytePad (screen->width * screen->fb[0].bitsPerPixel);
    screen->fb[0].pixelStride = screen->fb[0].byteStride * 8 / screen->fb[0].bitsPerPixel;
    screen->dumb = TRUE;
    return TRUE;
}

Bool
KdShadowInitScreen (ScreenPtr pScreen, ShadowUpdateProc update, ShadowWindowProc window)
{
    KdScreenPriv(pScreen);

    return shadowInit (pScreen, update, window);
}

void
KdShadowScreenFini (KdScreenInfo *screen)
{
    if (screen->fb[0].frameBuffer)
	xfree (screen->fb[0].frameBuffer);
}
