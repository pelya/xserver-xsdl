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
#include "cursorstr.h"

#define SetupCursor(s)	    KdScreenPriv(s); \
			    sisCardInfo(pScreenPriv); \
			    sisScreenInfo(pScreenPriv); \
			    SisPtr sis = sisc->sis; \
			    SisCursor *pCurPriv = &siss->cursor

static void
_sisMoveCursor (ScreenPtr pScreen, int x, int y)
{
    SetupCursor(pScreen);
    CARD8   xlow, xhigh, ylow, yhigh;
    CARD8   xoff, yoff;

    x -= pCurPriv->xhot;
    xoff = 0;
    if (x < 0)
    {
	xoff = -x;
	x = 0;
    }
    y -= pCurPriv->yhot;
    yoff = 0;
    if (y < 0)
    {
	yoff = -y;
	y = 0;
    }
    xlow = (CARD8) x;
    xhigh = (CARD8) (x >> 8);
    ylow = (CARD8) y;
    yhigh = (CARD8) (y >> 8);
    
    PutSrtc (sisc, 0x5, 0x86);
    PutSrtc (sisc, 0x1c, xoff & 0x3f);
    PutSrtc (sisc, 0x1f, yoff & 0x3f);
    
    PutSrtc (sisc, 0x1a, xlow);
    PutSrtc (sisc, 0x1b, xhigh & 0xf);
    
    PutSrtc (sisc, 0x1d, ylow);
    PutSrtc (sisc, 0x1e, yhigh & 0x7);
}

static void
sisMoveCursor (ScreenPtr pScreen, int x, int y)
{
    SetupCursor (pScreen);
    
    if (!pCurPriv->has_cursor)
	return;
    
    if (!pScreenPriv->enabled)
	return;
    
    LockSis(sisc);
    _sisMoveCursor (pScreen, x, y);
    UnlockSis(sisc);
}

static void
_sisSetCursorColors (ScreenPtr pScreen)
{
    SetupCursor (pScreen);
    CursorPtr	pCursor = pCurPriv->pCursor;
    /* set foreground */
    PutSrtc (sisc, 0x17, pCursor->foreRed >> 10);
    PutSrtc (sisc, 0x18, pCursor->foreGreen >> 10);
    PutSrtc (sisc, 0x19, pCursor->foreBlue >> 10);
    
    /* set background */
    PutSrtc (sisc, 0x14, pCursor->backRed >> 10);
    PutSrtc (sisc, 0x15, pCursor->backGreen >> 10);
    PutSrtc (sisc, 0x16, pCursor->backBlue >> 10);
}
    
static void
sisLoadCursor (ScreenPtr pScreen, int x, int y)
{
    SetupCursor(pScreen);
    CursorPtr	    pCursor = pCurPriv->pCursor;
    CursorBitsPtr   bits = pCursor->bits;
    int		    w, h;
    unsigned short  *ram, r;
    unsigned char   *msk, *mskLine, *src, *srcLine;
    unsigned short  m, s;
    
    int		    i, j;
    int		    cursor_address;
    int		    src_stride, src_width;

    CARD8	    sr6;

    /*
     * Lock Sis so the cursor doesn't move while we're setting it
     */
    LockSis(sisc);
    
    pCurPriv->pCursor = pCursor;
    pCurPriv->xhot = pCursor->bits->xhot;
    pCurPriv->yhot = pCursor->bits->yhot;
    
    /*
     * Stick new image into cursor memory
     */
    ram = (unsigned short *) siss->cursor_base;
    mskLine = (unsigned char *) bits->mask;
    srcLine = (unsigned char *) bits->source;

    h = bits->height;
    if (h > SIS_CURSOR_HEIGHT)
	h = SIS_CURSOR_HEIGHT;

    src_stride = BitmapBytePad(bits->width);		/* bytes per line */
    src_width = (bits->width + 7) >> 3;

    for (i = 0; i < SIS_CURSOR_HEIGHT; i++) {
	msk = mskLine;
	src = srcLine;
	mskLine += src_stride;
	srcLine += src_stride;
	for (j = 0; j < SIS_CURSOR_WIDTH / 8; j++) {

	    unsigned short  m, s;

	    if (i < h && j < src_width) 
	    {
		m = *msk++;
		s = *src++ & m;
		m = ~m;
		/* mask off right side */
		if (j == src_width - 1 && (bits->width & 7))
		{
		    m |= 0xff << (bits->width & 7);
		}
	    }
	    else
	    {
		m = 0xff;
		s = 0x00;
	    }

	    /*
	     * The SIS530 HW cursor format is: source(AND) bit, 
	     *   then a mask(XOR) bit, etc.
	     * byte swapping in sis530 is:
	     *   abcd ==> cdab
	     */

#define bit(a,n)    (((a) >> (n)) & 0x1)
	    
	    r = ((bit(m, 0) <<  7) | (bit(s, 0) <<  6) |
		 (bit(m, 1) <<  5) | (bit(s, 1) <<  4) |
		 (bit(m, 2) <<  3) | (bit(s, 2) <<  2) |
		 (bit(m, 3) <<  1) | (bit(s, 3) <<  0) |
		 (bit(m, 4) << 15) | (bit(s, 4) << 14) |
		 (bit(m, 5) << 13) | (bit(s, 5) << 12) |
		 (bit(m, 6) << 11) | (bit(s, 6) << 10) |
		 (bit(m, 7) <<  9) | (bit(s, 7) <<  8));

	    *ram++ = r;
	}
    }
    
    /* Set new color */
    _sisSetCursorColors (pScreen);
     
    /* Move to new position */
    _sisMoveCursor (pScreen, x, y);
    
    /* Enable cursor */
    sr6 = GetSrtc (sisc, 0x6);
    sr6 |= 0x40;
    PutSrtc (sisc, 0x6, sr6);

    UnlockSis(sisc);
}

static void
sisUnloadCursor (ScreenPtr pScreen)
{
    SetupCursor (pScreen);
    CARD8	sr6;

    LockSis (sisc);

    /* Disable cursor */
    sr6 = GetSrtc (sisc, 0x6);
    sr6 &= ~0x40;
    PutSrtc (sisc, 0x6, sr6);
    PutSrtc (sisc, 0x1b, 0x00);
    
    UnlockSis (sisc);
}

static Bool
sisRealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
{
    SetupCursor(pScreen);

    if (!pScreenPriv->enabled)
	return TRUE;
    
    /* miRecolorCursor does this */
    if (pCurPriv->pCursor == pCursor)
    {
	if (pCursor)
	{
	    int	    x, y;
	    
	    miPointerPosition (&x, &y);
	    sisLoadCursor (pScreen, x, y);
	}
    }
    return TRUE;
}

static Bool
sisUnrealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

static void
sisSetCursor (ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
    SetupCursor(pScreen);

    pCurPriv->pCursor = pCursor;
    
    if (!pScreenPriv->enabled)
	return;
    
    if (pCursor)
	sisLoadCursor (pScreen, x, y);
    else
	sisUnloadCursor (pScreen);
}

miPointerSpriteFuncRec sisPointerSpriteFuncs = {
    sisRealizeCursor,
    sisUnrealizeCursor,
    sisSetCursor,
    sisMoveCursor,
};

static void
sisQueryBestSize (int class, 
		 unsigned short *pwidth, unsigned short *pheight, 
		 ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    switch (class)
    {
    case CursorShape:
	if (*pwidth > pCurPriv->width)
	    *pwidth = pCurPriv->width;
	if (*pheight > pCurPriv->height)
	    *pheight = pCurPriv->height;
	if (*pwidth > pScreen->width)
	    *pwidth = pScreen->width;
	if (*pheight > pScreen->height)
	    *pheight = pScreen->height;
	break;
    default:
	fbQueryBestSize (class, pwidth, pheight, pScreen);
	break;
    }
}

Bool
sisCursorInit (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (!siss->cursor_base)
    {
	pCurPriv->has_cursor = FALSE;
	return FALSE;
    }
    
    pCurPriv->width = SIS_CURSOR_WIDTH;
    pCurPriv->height= SIS_CURSOR_HEIGHT;
    pScreen->QueryBestSize = sisQueryBestSize;
    miPointerInitialize (pScreen,
			 &sisPointerSpriteFuncs,
			 &kdPointerScreenFuncs,
			 FALSE);
    pCurPriv->has_cursor = TRUE;
    pCurPriv->pCursor = NULL;
    return TRUE;
}

void
sisCursorEnable (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (pCurPriv->has_cursor)
    {
	if (pCurPriv->pCursor)
	{
	    int	    x, y;
	    
	    miPointerPosition (&x, &y);
	    sisLoadCursor (pScreen, x, y);
	}
	else
	    sisUnloadCursor (pScreen);
    }
}

void
sisCursorDisable (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (!pScreenPriv->enabled)
	return;
    
    if (pCurPriv->has_cursor)
    {
	if (pCurPriv->pCursor)
	{
	    sisUnloadCursor (pScreen);
	}
    }
}

void
sisCursorFini (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    pCurPriv->pCursor = NULL;
}
