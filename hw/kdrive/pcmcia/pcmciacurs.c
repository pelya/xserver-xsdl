/*
 * Copyright 2001 by Alan Hourihane, Sychdyn, North Wales, UK.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "pcmcia.h"
#include "cursorstr.h"

#define SetupCursor(s)	    KdScreenPriv(s); \
			    pcmciaCardInfo(pScreenPriv); \
			    pcmciaScreenInfo(pScreenPriv); \
			    pcmciaCursor *pCurPriv = &pcmcias->cursor

static void
_pcmciaMoveCursor (ScreenPtr pScreen, int x, int y)
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
    
    /* This is the recommended order to move the cursor */
    if (pcmciac->HP) {
    	xlow = (CARD8) x;
    	xhigh = (CARD8) (x >> 8);
    	ylow = (CARD8) y;
    	yhigh = (CARD8) (y >> 8);
	pcmciaWriteIndex (pcmciac, 0x3d4, 0x40, xlow);
	pcmciaWriteIndex (pcmciac, 0x3d4, 0x41, xhigh);
	pcmciaWriteIndex (pcmciac, 0x3d4, 0x42, ylow);
	pcmciaWriteIndex (pcmciac, 0x3d4, 0x43, yhigh);
	pcmciaWriteIndex (pcmciac, 0x3d4, 0x46, xoff);
	pcmciaWriteIndex (pcmciac, 0x3d4, 0x47, yoff);
    } else {
	x >>= 3;
	y >>= 3;
    	xlow = (CARD8) x;
    	xhigh = (CARD8) (x >> 8);
    	ylow = (CARD8) y;
    	yhigh = (CARD8) (y >> 8);
	/* Don't be alarmed, yes the upper 3bits of the index are correct */
	pcmciaWriteIndex (pcmciac, 0x3c4, 0x10 | xhigh << 5, xlow);
	pcmciaWriteIndex (pcmciac, 0x3c4, 0x11 | yhigh << 5, ylow);
    }
}

static void
pcmciaMoveCursor (ScreenPtr pScreen, int x, int y)
{
    SetupCursor (pScreen);
    
    if (!pCurPriv->has_cursor)
	return;
    
    if (!pScreenPriv->enabled)
	return;
    
    _pcmciaMoveCursor (pScreen, x, y);
}

static void
pcmciaAllocCursorColors (ScreenPtr pScreen)
{
    SetupCursor (pScreen);
    CursorPtr	    pCursor = pCurPriv->pCursor;
    
    KdAllocateCursorPixels (pScreen, 0, pCursor, 
			    &pCurPriv->source, &pCurPriv->mask);
    switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
    case 4:
	pCurPriv->source |= pCurPriv->source << 4;
	pCurPriv->mask |= pCurPriv->mask << 4;
    case 8:
	pCurPriv->source |= pCurPriv->source << 8;
	pCurPriv->mask |= pCurPriv->mask << 8;
    case 16:
	pCurPriv->source |= pCurPriv->source << 16;
	pCurPriv->mask |= pCurPriv->mask << 16;
    }
}

static void
pcmciaSetCursorColors (ScreenPtr pScreen)
{
    SetupCursor (pScreen);
    CursorPtr	pCursor = pCurPriv->pCursor;
    CARD32	fg, bg;
    
    fg = pCurPriv->source;
    bg = pCurPriv->mask;
    
    if (pcmciac->HP) {
	/* 
	 * This trident chip uses the palette for it's cursor colors - ouch!
	 * We enforce it to always stay the black/white colors as we don't
	 * want it to muck with the overscan color. Tough. Use softCursor
	 * if you want to change cursor colors.
	 */
	pcmciaWriteReg (pcmciac, 0x3c8, 0xff); /* DAC 0 */
	pcmciaWriteReg (pcmciac, 0x3c9, 0x00); 
	pcmciaWriteReg (pcmciac, 0x3c9, 0x00);
	pcmciaWriteReg (pcmciac, 0x3c9, 0x00);
	pcmciaWriteReg (pcmciac, 0x3c8, 0x00); /* DAC 255 */
	pcmciaWriteReg (pcmciac, 0x3c9, 0x3f);
	pcmciaWriteReg (pcmciac, 0x3c9, 0x3f);
	pcmciaWriteReg (pcmciac, 0x3c9, 0x3f);
    } else {
        CARD8 temp;
	temp = pcmciaReadIndex(pcmciac, 0x3c4, 0x12);
    	pcmciaWriteIndex (pcmciac, 0x3c4, 0x12, (temp & 0xFE) | 0x02);

	pcmciaWriteReg (pcmciac, 0x3c8, 0x00); /* DAC 256 */
	pcmciaWriteReg (pcmciac, 0x3c9, fg); 
	pcmciaWriteReg (pcmciac, 0x3c9, fg >> 8);
	pcmciaWriteReg (pcmciac, 0x3c9, fg >> 16);
	pcmciaWriteReg (pcmciac, 0x3c8, 0x00); /* DAC 257 */
	pcmciaWriteReg (pcmciac, 0x3c9, bg);
	pcmciaWriteReg (pcmciac, 0x3c9, bg >> 8);
	pcmciaWriteReg (pcmciac, 0x3c9, bg >> 16);

    	pcmciaWriteIndex (pcmciac, 0x3c4, 0x12, temp);
    }
}
    
void
pcmciaRecolorCursor (ScreenPtr pScreen, int ndef, xColorItem *pdef)
{
    SetupCursor (pScreen);
    CursorPtr	    pCursor = pCurPriv->pCursor;
    xColorItem	    sourceColor, maskColor;

    if (!pCurPriv->has_cursor || !pCursor)
	return;
    
    if (!pScreenPriv->enabled)
	return;
    
    if (pdef)
    {
	while (ndef)
	{
	    if (pdef->pixel == pCurPriv->source || 
		pdef->pixel == pCurPriv->mask)
		break;
	    ndef--;
	}
	if (!ndef)
	    return;
    }
    pcmciaAllocCursorColors (pScreen);
    pcmciaSetCursorColors (pScreen);
}
    
#define InvertBits32(v) { \
    v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555); \
    v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333); \
    v = ((v & 0x0f0f0f0f) << 4) | ((v >> 4) & 0x0f0f0f0f); \
}

static void
pcmciaLoadCursor (ScreenPtr pScreen, int x, int y)
{
    SetupCursor(pScreen);
    CursorPtr	    pCursor = pCurPriv->pCursor;
    CursorBitsPtr   bits = pCursor->bits;
    int	w, h;
    CARD8	    *ram;
    CARD32	    *msk, *mskLine, *src, *srcLine;
    int		    i, j;
    int		    cursor_address;
    int		    lwsrc;
    unsigned char   ramdac_control_;
    CARD32	    offset;

    /*
     * Allocate new colors
     */
    pcmciaAllocCursorColors (pScreen);
    
    pCurPriv->pCursor = pCursor;
    pCurPriv->xhot = pCursor->bits->xhot;
    pCurPriv->yhot = pCursor->bits->yhot;
    
    /*
     * Stick new image into cursor memory
     */
    if (pcmciac->HP) {
    	ram = (CARD8 *) pcmcias->cursor_base;
    } else {
	/* The last bank */
    	ram = (CARD8 *) pcmciac->fb;
    	pcmciaWriteIndex (pcmciac, 0x3ce, 0x09, 0x7f);
    	pcmciaWriteIndex (pcmciac, 0x3ce, 0x0A, 0x7f);
    }
	
    mskLine = (CARD32 *) bits->mask;
    srcLine = (CARD32 *) bits->source;

    h = bits->height;
    if (h > PCMCIA_CURSOR_HEIGHT)
	h = PCMCIA_CURSOR_HEIGHT;

    lwsrc = BitmapBytePad(bits->width) / 4;

    for (i = 0; i < PCMCIA_CURSOR_HEIGHT; i++) {
	msk = mskLine;
	src = srcLine;
	mskLine += lwsrc;
	srcLine += lwsrc;
	for (j = 0; j < PCMCIA_CURSOR_WIDTH / 32; j++) {

	    CARD32  m, s;

	    if (i < h && j < lwsrc) 
	    {
		m = *msk++;
		s = *src++;
		InvertBits32(m);
		InvertBits32(s);
	    }
	    else
	    {
		m = 0;
		s = 0;
	    }

	    /* Do 8bit access */
	    *ram++ = (m & 0xff);
	    *ram++ = (m & 0xff00) >> 8;
	    *ram++ = (m & 0xff0000) >> 16;
	    *ram++ = (m & 0xff000000) >> 24;
	    *ram++ = (s & 0xff);
	    *ram++ = (s & 0xff00) >> 8;
	    *ram++ = (s & 0xff0000) >> 16;
	    *ram++ = (s & 0xff000000) >> 24;
	}
    }
    
    /* Set address for cursor bits */
    if (pcmciac->HP) {
    	offset = pcmcias->cursor_base - (CARD8 *) pcmcias->screen;
    	offset >>= 10;
    	pcmciaWriteIndex (pcmciac, 0x3d4, 0x44, (CARD8) (offset & 0xff));
    	pcmciaWriteIndex (pcmciac, 0x3d4, 0x45, (CARD8) (offset >> 8));
    } else {
    	pcmciaWriteIndex (pcmciac, 0x3c4, 0x13, 15); /* ?? */
    }
    
    /* Set new color */
    pcmciaSetCursorColors (pScreen);
     
    /* Enable the cursor */
    if (pcmciac->HP)
    	pcmciaWriteIndex (pcmciac, 0x3d4, 0x50, 0xc1);
    else
    	pcmciaWriteIndex (pcmciac, 0x3c4, 0x12, 0x05);
    
    /* Move to new position */
    pcmciaMoveCursor (pScreen, x, y);
}

static void
pcmciaUnloadCursor (ScreenPtr pScreen)
{
    SetupCursor (pScreen);
    
    /* Disable cursor */
    if (pcmciac->HP) 
    	pcmciaWriteIndex (pcmciac, 0x3d4, 0x50, 0);
    else
    	pcmciaWriteIndex (pcmciac, 0x3c4, 0x12, 0);
}

static Bool
pcmciaRealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
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
	    pcmciaLoadCursor (pScreen, x, y);
	}
    }
    return TRUE;
}

static Bool
pcmciaUnrealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

static void
pcmciaSetCursor (ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
    SetupCursor(pScreen);

    pCurPriv->pCursor = pCursor;
    
    if (!pScreenPriv->enabled)
	return;
    
    if (pCursor)
	pcmciaLoadCursor (pScreen, x, y);
    else
	pcmciaUnloadCursor (pScreen);
}

miPointerSpriteFuncRec pcmciaPointerSpriteFuncs = {
    pcmciaRealizeCursor,
    pcmciaUnrealizeCursor,
    pcmciaSetCursor,
    pcmciaMoveCursor,
};

static void
pcmciaQueryBestSize (int class, 
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
pcmciaCursorInit (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (!pcmcias->cursor_base)
    {
	pCurPriv->has_cursor = FALSE;
	return FALSE;
    }
    
    pCurPriv->width = PCMCIA_CURSOR_WIDTH;
    pCurPriv->height= PCMCIA_CURSOR_HEIGHT;
    pScreen->QueryBestSize = pcmciaQueryBestSize;
    miPointerInitialize (pScreen,
			 &pcmciaPointerSpriteFuncs,
			 &kdPointerScreenFuncs,
			 FALSE);
    pCurPriv->has_cursor = TRUE;
    pCurPriv->pCursor = NULL;
    return TRUE;
}

void
pcmciaCursorEnable (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (pCurPriv->has_cursor)
    {
	if (pCurPriv->pCursor)
	{
	    int	    x, y;
	    
	    miPointerPosition (&x, &y);
	    pcmciaLoadCursor (pScreen, x, y);
	}
	else
	    pcmciaUnloadCursor (pScreen);
    }
}

void
pcmciaCursorDisable (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (!pScreenPriv->enabled)
	return;
    
    if (pCurPriv->has_cursor)
    {
	if (pCurPriv->pCursor)
	{
	    pcmciaUnloadCursor (pScreen);
	}
    }
}

void
pcmciaCursorFini (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    pCurPriv->pCursor = NULL;
}
