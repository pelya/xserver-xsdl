/*
 * Copyright © 2004 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $RCSId$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "ati.h"
#include "ati_reg.h"
#include "cursorstr.h"
#include "ati_draw.h"

static void
ATIMoveCursor(ScreenPtr pScreen, int x, int y)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;
	CARD16 xoff, yoff;
	CARD8 *mmio = atic->reg_base;

	if (!pCurPriv->has_cursor)
		return;

	if (!pScreenPriv->enabled)
		return;

	x -= pCurPriv->xhot;
	xoff = 0;
	if (x < 0) {
		xoff = -x;
		x = 0;
	}
	y -= pCurPriv->yhot;
	yoff = 0;
	if (y < 0) {
		yoff = -y;
		y = 0;
	}

	MMIO_OUT32(mmio, ATI_REG_CUR_HORZ_VERT_OFF, ATI_CUR_LOCK |
	    (xoff << 16) | yoff);
	MMIO_OUT32(mmio, ATI_REG_CUR_HORZ_VERT_POSN, ATI_CUR_LOCK |
	    (x << 16) | y);
	MMIO_OUT32(mmio, ATI_REG_CUR_OFFSET, (pCurPriv->offset + yoff * 16));
}

static void
ATIAllocCursorColors(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;
	CursorPtr pCursor = pCurPriv->pCursor;

	KdAllocateCursorPixels(pScreen, 0, pCursor, &pCurPriv->source,
	    &pCurPriv->mask);
	switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
	case 4:
		pCurPriv->source |= pCurPriv->source << 4;
		pCurPriv->mask |= pCurPriv->mask << 4;
		/* FALLTHROUGH */
	case 8:
		pCurPriv->source |= pCurPriv->source << 8;
		pCurPriv->mask |= pCurPriv->mask << 8;
		/* FALLTHROUGH */
	case 16:
		pCurPriv->source |= pCurPriv->source << 16;
		pCurPriv->mask |= pCurPriv->mask << 16;
	}
}

static void
ATISetCursorColors(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;
	CARD8 *mmio = atic->reg_base;

	MMIO_OUT32(mmio, ATI_REG_CUR_CLR0, pCurPriv->mask);
	MMIO_OUT32(mmio, ATI_REG_CUR_CLR1, pCurPriv->source);
}

void
ATIRecolorCursor(ScreenPtr pScreen, int ndef, xColorItem *pdef)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;
	CursorPtr pCursor = pCurPriv->pCursor;

	if (!pCurPriv->has_cursor || !pCursor)
		return;

	if (!pScreenPriv->enabled)
		return;

	if (pdef) { 
		while (ndef != 0) {
			if (pdef->pixel == pCurPriv->source || 
			    pdef->pixel == pCurPriv->mask)
				break;
			ndef--;
		}

		if (ndef == 0)
			return;
	}
	ATIAllocCursorColors(pScreen);
	ATISetCursorColors(pScreen);
}

#define InvertBits32(v) do { \
	v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555); \
	v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333); \
	v = ((v & 0x0f0f0f0f) << 4) | ((v >> 4) & 0x0f0f0f0f); \
} while (0)

static void
ATILoadCursor(ScreenPtr pScreen, int x, int y)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;
	CursorPtr pCursor = pCurPriv->pCursor;
	CursorBitsPtr bits = pCursor->bits;
	int h;
	CARD32 *ram, *msk, *mskLine, *src, *srcLine;
	int i;
	int lwsrc;
	CARD32 tmp;
	CARD8 *mmio = atic->reg_base;

	ATIAllocCursorColors(pScreen);

	pCurPriv->pCursor = pCursor;
	pCurPriv->xhot = pCursor->bits->xhot;
	pCurPriv->yhot = pCursor->bits->yhot;

	/* Stick new image into cursor memory */
	ram = (CARD32 *)(pScreenPriv->screen->memory_base +
	    pCurPriv->offset);
	mskLine = (CARD32 *)bits->mask;
	srcLine = (CARD32 *)bits->source;

	h = bits->height;
	if (h > ATI_CURSOR_HEIGHT)
		h = ATI_CURSOR_HEIGHT;

	lwsrc = BitmapBytePad(bits->width) / 4;		/* words per line */

	tmp = MMIO_IN32(mmio, ATI_REG_GEN_CNTL);
	MMIO_OUT32(mmio, ATI_REG_GEN_CNTL, tmp & ~ATI_CRTC_CUR_EN);

	for (i = 0; i < ATI_CURSOR_HEIGHT; i++) {
		CARD32 m1, m2, s1, s2;

		msk = mskLine;
		src = srcLine;
		mskLine += lwsrc;
		srcLine += lwsrc;

		if (i < h && 0 < lwsrc) {
			m1 = ~*msk++;
			s1 = *src++;
			InvertBits32(m1);
			InvertBits32(s1);
		} else {
			m1 = 0xffffffff;
			s1 = 0x0;
		}
		if (i < h && 1 < lwsrc) {
			m2 = ~*msk++;
			s2 = *src++;
			InvertBits32(m2);
			InvertBits32(s2);
		} else {
			m2 = 0xffffffff;
			s2 = 0x0;
		}

		*ram++ = m1;
		*ram++ = m2;
		*ram++ = s1;
		*ram++ = s2;
	}

	/* Not sure why this is necessary, but it prevents some cursor
	 * corruption.  Not even all of it.
	 */
	for (i = 0; i < ATI_CURSOR_HEIGHT; i++) {
		*ram++ = 0xffffffff;
		*ram++ = 0xffffffff;
		*ram++ = 0x0;
		*ram++ = 0x0;
	}

	/* Enable the cursor */
	tmp = MMIO_IN32(mmio, ATI_REG_GEN_CNTL);
	MMIO_OUT32(mmio, ATI_REG_GEN_CNTL, tmp | ATI_CRTC_CUR_EN);

	/* Set new color */
	ATISetCursorColors(pScreen);

	/* Move to new position */
	ATIMoveCursor(pScreen, x, y);
}

static void
ATIUnloadCursor(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	CARD8 *mmio = atic->reg_base;
	CARD32 tmp;

	tmp = MMIO_IN32(mmio, ATI_REG_GEN_CNTL) & ~ATI_CRTC_CUR_EN;
	MMIO_OUT32(mmio, ATI_REG_GEN_CNTL, tmp);
}

static Bool
ATIRealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;

	if (!pScreenPriv->enabled)
		return TRUE;

	/* miRecolorCursor does this */
	if (pCursor && pCurPriv->pCursor == pCursor)
	{
		int x, y;

		miPointerPosition(&x, &y);
		ATILoadCursor(pScreen, x, y);
	}

	return TRUE;
}

static Bool
ATIUnrealizeCursor(ScreenPtr pScreen, CursorPtr pCursor)
{
	return TRUE;
}

static void
ATISetCursor(ScreenPtr pScreen, CursorPtr pCursor, int x, int y)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;

	pCurPriv->pCursor = pCursor;

	if (!pScreenPriv->enabled)
		return;

	if (pCursor)
		ATILoadCursor(pScreen, x, y);
	else
		ATIUnloadCursor(pScreen);
}

miPointerSpriteFuncRec ATIPointerSpriteFuncs = {
	ATIRealizeCursor,
	ATIUnrealizeCursor,
	ATISetCursor,
	ATIMoveCursor,
};

static void
ATIQueryBestSize(int class, unsigned short *pwidth, unsigned short *pheight, 
    ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;

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
		fbQueryBestSize(class, pwidth, pheight, pScreen);
		break;
	}
}

void
ATICursorEnable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;

	if (!pCurPriv->has_cursor)
		return;

	if (pCurPriv->pCursor) {
		int x, y;

		miPointerPosition(&x, &y);
		ATILoadCursor(pScreen, x, y);
	}
	else
		ATIUnloadCursor(pScreen);
}

void
ATICursorDisable(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;

	if (!pScreenPriv->enabled || !pCurPriv->has_cursor)
		return;

	if (pCurPriv->pCursor)
		ATIUnloadCursor(pScreen);
}

Bool
ATICursorInit(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATICardInfo(pScreenPriv);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;

	pCurPriv->has_cursor = FALSE;

	if (pCurPriv->offset == 0)
		return FALSE;

	if (atic->reg_base == NULL)
		return FALSE;

	pCurPriv->width = ATI_CURSOR_WIDTH;
	pCurPriv->height= ATI_CURSOR_HEIGHT;
	pScreen->QueryBestSize = ATIQueryBestSize;
	miPointerInitialize(pScreen, &ATIPointerSpriteFuncs,
	    &kdPointerScreenFuncs, FALSE);
	pCurPriv->has_cursor = TRUE;
	pCurPriv->pCursor = NULL;
	return TRUE;
}

void  
ATICursorFini(ScreenPtr pScreen)
{
	KdScreenPriv(pScreen);
	ATIScreenInfo(pScreenPriv);
	ATICursor *pCurPriv = &atis->cursor;

	pCurPriv->has_cursor = FALSE;
	pCurPriv->pCursor = NULL;
}
