/* COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 2000, 2001 Nokia Home Communications

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

X Window System is a trademark of The Open Group */

/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/



/* $XFree86$ */

/* i810_cursor.c: KDrive hardware cursor routines for the i810 chipset */

/*
 * Authors:
 *   Keith Whitwell <keithw@precisioninsight.com>
 *   Pontus Lidman <pontus.lidman@nokia.com>
 *
 */

#include "kdrive.h"
#include "kxv.h"
#include "i810.h"
#include "cursorstr.h"

#define SetupCursor(s)	    KdScreenPriv(s); \
			    i810CardInfo(pScreenPriv); \
			    i810ScreenInfo(pScreenPriv); \
			    i810Cursor *pCurPriv = &i810s->cursor


static void
writeStandardMMIO(I810CardInfo *i810c, int addr, CARD8 val) {
  moutb(addr, val);
}

void
_i810MoveCursor(ScreenPtr pScreen, int x, int y) {
    SetupCursor(pScreen);
    int flag;

    if (I810_DEBUG & DEBUG_VERBOSE_CURSOR)
      ErrorF( "I810SetCursorPosition %d %d\n", x, y);

    x += i810c->CursorOffset;

    if (x >= 0) flag = CURSOR_X_POS;
    else {
        flag = CURSOR_X_NEG;
        x=-x;
    }

    OUTREG8( CURSOR_X_LO, x&0xFF);
    OUTREG8( CURSOR_X_HI, (((x >> 8) & 0x07) | flag));
    
    if (y >= 0) flag = CURSOR_Y_POS;
    else {
        flag = CURSOR_Y_NEG;
        y=-y;
    }
    OUTREG8( CURSOR_Y_LO, y&0xFF);
    OUTREG8( CURSOR_Y_HI, (((y >> 8) & 0x07) | flag));

    /* Enable cursor */
    OUTREG( CURSOR_BASEADDR, i810c->CursorPhysical);
    OUTREG8( CURSOR_CONTROL, CURSOR_ORIGIN_DISPLAY | CURSOR_MODE_64_3C);

}

static void i810LoadCursor(ScreenPtr pScreen, int x, int y);

static void
i810MoveCursor (ScreenPtr pScreen, int x, int y)
{
    SetupCursor (pScreen);
    
    if (!pCurPriv->has_cursor)
	return;
    
    if (!pScreenPriv->enabled)
	return;
    
    _i810MoveCursor (pScreen, x, y);

    i810LoadCursor(pScreen, x, y);
}

static void
_i810SetCursorColors(ScreenPtr pScreen) { /* int bg, int fg */

    SetupCursor(pScreen);
    int tmp;

    int bg = 0xffffff;
    int fg = 0x000000;

    tmp=INREG8(PIXPIPE_CONFIG_0);
    tmp |= EXTENDED_PALETTE;
    OUTREG8( PIXPIPE_CONFIG_0, tmp);

    writeStandardMMIO(i810c, DACMASK, 0xFF);
    writeStandardMMIO(i810c, DACWX, 0x04);

    writeStandardMMIO(i810c, DACDATA, (bg & 0x00FF0000) >> 16);
    writeStandardMMIO(i810c, DACDATA, (bg & 0x0000FF00) >> 8);
    writeStandardMMIO(i810c, DACDATA, (bg & 0x000000FF));

    writeStandardMMIO(i810c, DACDATA, (fg & 0x00FF0000) >> 16);
    writeStandardMMIO(i810c, DACDATA, (fg & 0x0000FF00) >> 8);
    writeStandardMMIO(i810c, DACDATA, (fg & 0x000000FF));

    tmp=INREG8( PIXPIPE_CONFIG_0 );
    tmp &= ~EXTENDED_PALETTE;
    OUTREG8( PIXPIPE_CONFIG_0, tmp );
}

static void i810LoadCursor(ScreenPtr pScreen, int x, int y) {

    SetupCursor(pScreen);

    int		    w, h;
    unsigned short  r;
    unsigned char   *msk, *mskLine, *src, *srcLine;
    
    int		    i, j;
    int		    src_stride, src_width;

    CursorPtr	    pCursor = pCurPriv->pCursor;
    CursorBitsPtr   bits = pCursor->bits;
    CARD8 tmp;
    unsigned char *ram, *ramLine;

    pCurPriv->pCursor = pCursor;
    pCurPriv->xhot = pCursor->bits->xhot;
    pCurPriv->yhot = pCursor->bits->yhot;

    ramLine = (unsigned char *)(i810c->FbBase + i810c->CursorStart);
    mskLine = (unsigned char *) bits->mask;
    srcLine = (unsigned char *) bits->source;

    h = bits->height;
    if (h > I810_CURSOR_HEIGHT)
	h = I810_CURSOR_HEIGHT;


    src_stride = BitmapBytePad(bits->width);		/* bytes per line */
    src_width = (bits->width + 7) >> 3;

    for (i = 0; i < I810_CURSOR_HEIGHT; i++) {
	msk = mskLine;
	src = srcLine;
        ram = ramLine;
	mskLine += src_stride;
	srcLine += src_stride;
        ramLine += I810_CURSOR_WIDTH / 4;

	for (j = 0; j < I810_CURSOR_WIDTH / 8; j++) {

	    unsigned short  m, s, b1, b2;

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

            /* The i810 stores the cursor in an interleaved bitmap format,
             in reverse byte order */
            /* Not tested with cursors bigger than 16x16 !!! */

            ram[8+(j ^ 1)] = s; /* b2 */
            ram[0+(j ^ 1)] = m; /* b1 */
	}
    }
    /* Set new color */
    _i810SetCursorColors (pScreen);
     
    /* Move to new position */
    _i810MoveCursor (pScreen, x, y);
    
    /* Enable cursor */
    OUTREG( CURSOR_BASEADDR, i810c->CursorPhysical);
    OUTREG8( CURSOR_CONTROL, CURSOR_ORIGIN_DISPLAY | CURSOR_MODE_64_3C);
    
    tmp = INREG8( PIXPIPE_CONFIG_0 );
    tmp |= HW_CURSOR_ENABLE;
    OUTREG8( PIXPIPE_CONFIG_0, tmp);
}

static void
i810UnloadCursor(ScreenPtr pScreen) {

    SetupCursor(pScreen);

    unsigned char tmp;
    
    tmp=INREG8( PIXPIPE_CONFIG_0 );
    tmp &= ~HW_CURSOR_ENABLE;
    OUTREG8( PIXPIPE_CONFIG_0, tmp);
}


static Bool
i810RealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
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
	    i810LoadCursor (pScreen, x, y);
	}
    }
    return TRUE;
}

static Bool
i810UnrealizeCursor (ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

static void
i810SetCursor (ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
    SetupCursor(pScreen);

    pCurPriv->pCursor = pCursor;
    
    if (!pScreenPriv->enabled)
	return;
    
    if (pCursor)
	i810LoadCursor (pScreen, x, y);
    else
	i810UnloadCursor (pScreen);
}

miPointerSpriteFuncRec i810PointerSpriteFuncs = {
    i810RealizeCursor,
    i810UnrealizeCursor,
    i810SetCursor,
    i810MoveCursor,
};

static void
i810QueryBestSize (int class, 
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
i810CursorInit(ScreenPtr pScreen)
{

    SetupCursor(pScreen);

    if (!i810c->CursorStart) {
	pCurPriv->has_cursor = FALSE;
	return FALSE;
    }

    pCurPriv->width = I810_CURSOR_WIDTH;
    pCurPriv->height= I810_CURSOR_HEIGHT;
    pScreen->QueryBestSize = i810QueryBestSize;
    miPointerInitialize (pScreen,
			 &i810PointerSpriteFuncs,
			 &kdPointerScreenFuncs,
			 FALSE);
    pCurPriv->has_cursor = TRUE;
    pCurPriv->pCursor = NULL;
    return TRUE;
}

void
i810CursorEnable (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (pCurPriv->has_cursor)
    {
	if (pCurPriv->pCursor)
	{
	    int	    x, y;
	    
	    miPointerPosition (&x, &y);
	    i810LoadCursor (pScreen, x, y);
	}
	else
	    i810UnloadCursor (pScreen);
    }
}

void
i810CursorDisable (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    if (!pScreenPriv->enabled)
	return;
    
    if (pCurPriv->has_cursor)
    {
	if (pCurPriv->pCursor)
	{
	    i810UnloadCursor (pScreen);
	}
    }
}

void
i810CursorFini (ScreenPtr pScreen)
{
    SetupCursor (pScreen);

    pCurPriv->pCursor = NULL;
}

