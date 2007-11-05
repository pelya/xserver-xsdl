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
#include "sisdraw.h"

#include	"Xmd.h"
#include	"gcstruct.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"mistruct.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"fb.h"
#include	"migc.h"
#include	"miline.h"

CARD8 sisPatRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0xa0,         /* src AND dst */
    /* GXandReverse */      0x50,         /* src AND NOT dst */
    /* GXcopy       */      0xf0,         /* src */
    /* GXandInverted*/      0x0a,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x5a,         /* src XOR dst */
    /* GXor         */      0xfa,         /* src OR dst */
    /* GXnor        */      0x05,         /* NOT src AND NOT dst */
    /* GXequiv      */      0xa5,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xf5,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x0f,         /* NOT src */
    /* GXorInverted */      0xaf,         /* NOT src OR dst */
    /* GXnand       */      0x5f,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

CARD8 sisBltRop[16] = {
    /* GXclear      */      0x00,         /* 0 */
    /* GXand        */      0x88,         /* src AND dst */
    /* GXandReverse */      0x44,         /* src AND NOT dst */
    /* GXcopy       */      0xcc,         /* src */
    /* GXandInverted*/      0x22,         /* NOT src AND dst */
    /* GXnoop       */      0xaa,         /* dst */
    /* GXxor        */      0x66,         /* src XOR dst */
    /* GXor         */      0xee,         /* src OR dst */
    /* GXnor        */      0x11,         /* NOT src AND NOT dst */
    /* GXequiv      */      0x99,         /* NOT src XOR dst */
    /* GXinvert     */      0x55,         /* NOT dst */
    /* GXorReverse  */      0xdd,         /* src OR NOT dst */
    /* GXcopyInverted*/     0x33,         /* NOT src */
    /* GXorInverted */      0xbb,         /* NOT src OR dst */
    /* GXnand       */      0x77,         /* NOT src OR NOT dst */
    /* GXset        */      0xff,         /* 1 */
};

/* Align blts to this boundary or risk trashing an in-progress expand */
#define SIS_MIN_PATTERN	8

/* Do plane bits in this increment to balance CPU and graphics engine */
#define SIS_PATTERN_INC	1024

typedef struct _SisExpand {
    SisCardInfo	    *sisc;
    SisScreenInfo   *siss;
    CARD32	    off;
    int		    last;
} SisExpand;

static void
sisExpandInit (ScreenPtr   pScreen,
	       SisExpand   *e)
{
    KdScreenPriv(pScreen);
    sisCardInfo(pScreenPriv);
    sisScreenInfo(pScreenPriv);

    e->sisc = sisc;
    e->siss = siss;
    e->off = siss->expand_off;
    e->last = 0;
}

static CARD32 *
sisExpandAlloc (SisExpand	*e,
		int		nb)
{
    SisCardInfo	    *sisc = e->sisc;
    SisScreenInfo   *siss = e->siss;
    SisPtr	    sis = sisc->sis;
    CARD32	    off;
    
    /* round up to alignment boundary */
    nb = (nb + SIS_MIN_PATTERN-1) & ~(SIS_MIN_PATTERN-1);
    
    off = e->off + e->last;
    if (off + nb > siss->expand_off + siss->expand_len)
    {
	_sisWaitIdleEmpty (sis);
	off = siss->expand_off;
    }
    e->off = off;
    e->last = nb;
    return (CARD32 *) (sisc->frameBuffer + off);
}

void
sisGlyphBltClipped (DrawablePtr	    pDrawable, 
		    GCPtr	    pGC, 
		    int		    x, 
		    int		    y, 
		    unsigned int    nglyph,
		    CharInfoPtr	    *ppciInit,
		    Bool	    imageBlt)
{
    SetupSis(pDrawable->pScreen);
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate(pGC);
    int		    height;
    int		    width;
    int		    xBack, yBack;
    int		    hBack, wBack;
    int		    nb, bwidth, nl;
    FontPtr	    pfont = pGC->font;
    CharInfoPtr	    pci;
    CARD8	    *bits8, b;
    CARD16	    *bits16;
    CARD32	    *bits32;
    BoxPtr	    extents;
    BoxRec	    bbox;
    unsigned char   alu;
    CARD32	    cmd;
    SisExpand	    expand;
    CARD32	    *dst, d;
    int		    nbytes;
    int		    shift;
    int		    x1, y1, x2, y2;
    RegionPtr	    pClip = fbGetCompositeClip(pGC);
    BoxPtr	    pBox;
    int		    nbox;
    int		    rect_in;
    int		    widthBlt;
    CharInfoPtr	    *ppci;

    x += pDrawable->x;
    y += pDrawable->y;

    if (imageBlt)
    {
	xBack = x;
	yBack = y - FONTASCENT(pGC->font);
	wBack = 0;
	hBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
	if (hBack)
	{
	    height = nglyph;
	    ppci = ppciInit;
	    while (height--)
		wBack += (*ppci++)->metrics.characterWidth;
	}
	if (wBack < 0)
	{
	    xBack = xBack + wBack;
	    wBack = -wBack;
	}
	if (hBack < 0)
	{
	    yBack = yBack + hBack;
	    hBack = -hBack;
	}
	alu = GXcopy;
    }
    else
    {
	wBack = 0;
	alu = pGC->alu;
    }
    
    if (wBack)
    {
	_sisSetSolidRect (sis, pGC->bgPixel, GXcopy, cmd);
	for (nbox = REGION_NUM_RECTS (pClip),
	     pBox = REGION_RECTS (pClip);
	     nbox--;
	     pBox++)
	{
	    x1 = xBack;
	    x2 = xBack + wBack;
	    y1 = yBack;
	    y2 = yBack + hBack;
	    if (x1 < pBox->x1) x1 = pBox->x1;
	    if (x2 > pBox->x2) x2 = pBox->x2;
	    if (y1 < pBox->y1) y1 = pBox->y1;
	    if (y2 > pBox->y2) y2 = pBox->y2;
	    if (x1 < x2 && y1 < y2)
	    {
		_sisRect (sis, x1, y1, x2 - x1, y2 - y1, cmd);
	    }
	}
    }
    
    sisExpandInit (pDrawable->pScreen, &expand);
    
    sis->u.general.src_fg = pGC->fgPixel;
    sis->u.general.src_pitch = 0;
    sis->u.general.src_x = 0;
    sis->u.general.src_y = 0;
    
    cmd = (SIS_CMD_ENH_COLOR_EXPAND | 
	   SIS_CMD_SRC_SCREEN |
	   SIS_CMD_PAT_FG | 
	   (sisBltRop[alu] << 8) | 
	   SIS_CMD_INC_X |
	   SIS_CMD_INC_Y |
	   SIS_CMD_RECT_CLIP_ENABLE |
	   SIS_CMD_TRANSPARENT);
	
    ppci = ppciInit;
    while (nglyph--)
    {
	pci = *ppci++;
	height = pci->metrics.ascent + pci->metrics.descent;
	width = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	widthBlt = (width + 31) & ~31;
	nb = (widthBlt >> 3) * height;
	if (nb)
	{
	    x1 = x + pci->metrics.leftSideBearing;
	    y1 = y - pci->metrics.ascent;
	    bbox.x1 = x1;
	    bbox.y1 = y1;
	    bbox.x2 = x1 + width;
	    bbox.y2 = y1 + height;
	    rect_in = RECT_IN_REGION(pGC->pScreen, pClip, &bbox);
	    if (rect_in != rgnOUT)
	    {
		dst = sisExpandAlloc (&expand, nb);

		sis->u.general.src_base = expand.off;
		sis->u.general.dst_x = x1;
		sis->u.general.dst_y = y1;
		sis->u.general.rect_width = widthBlt;
		sis->u.general.rect_height = height;
		nb >>= 2;
		bits32 = (CARD32 *) pci->bits;
		while (nb--)
		{
		    d = *bits32++;
		    SisInvertBits32 (d);
		    *dst++ = d;
		}
		if (rect_in == rgnPART)
		{
		    for (nbox = REGION_NUM_RECTS (pClip),
			 pBox = REGION_RECTS (pClip);
			 nbox--;
			 pBox++)
		    {
			_sisClip (sis, pBox->x1, pBox->y1, pBox->x2, pBox->y2);
			sis->u.general.command = cmd;
		    }
		}
		else
		{
		    _sisClip (sis, 0, 0, x1+width, pScreenPriv->screen->height);
		    sis->u.general.command = cmd;
		}
	    }
	}
	x += pci->metrics.characterWidth;
    }
    _sisClip (sis, 0, 0, 
	      pScreenPriv->screen->width, pScreenPriv->screen->height);
    KdMarkSync (pDrawable->pScreen);
}

Bool
sisTEGlyphBlt (DrawablePtr	pDrawable, 
	       GCPtr		pGC, 
	       int		xInit, 
	       int		yInit, 
	       unsigned int	nglyph,
	       CharInfoPtr	*ppci,
	       Bool		imageBlt)
{
    SetupSis(pDrawable->pScreen);
    int		    x, y;
    int		    widthGlyphs, widthGlyph;
    int		    widthBlt;
    FbBits	    depthMask;
    int		    glyphsPer;
    FontPtr	    pfont = pGC->font;
    unsigned long   *char1, *char2, *char3, *char4, *char5;
    CARD8	    alu;
    CARD32	    *dst, tmp;
    CARD8	    *dst8, *bits8;
    int		    nb;
    int		    bwidth;
    CARD32	    cmd;
    int		    h;
    BoxRec	    bbox;
    SisExpand	    expand;
    int		    lwTmp, lw;
    int		    extra, n;
    
    widthGlyph = FONTMAXBOUNDS(pfont,characterWidth);
    if (!widthGlyph)
	return TRUE;
    
    h = FONTASCENT(pfont) + FONTDESCENT(pfont);
    if (!h)
	return TRUE;
    
    x = xInit + FONTMAXBOUNDS(pfont,leftSideBearing) + pDrawable->x;
    y = yInit - FONTASCENT(pfont) + pDrawable->y;
    
    bbox.x1 = x;
    bbox.x2 = x + (widthGlyph * nglyph);
    bbox.y1 = y;
    bbox.y2 = y + h;

    switch (RECT_IN_REGION(pGC->pScreen, fbGetCompositeClip(pGC), &bbox))
    {
    case rgnPART:
	if (x < 0 || y < 0)
	    return FALSE;
	sisGlyphBltClipped (pDrawable, pGC, xInit, yInit, nglyph, ppci, imageBlt);
    case rgnOUT:
	return TRUE;
    }

    if (widthGlyph <= 6)
	glyphsPer = 5;
    else if (widthGlyph <= 8)
	glyphsPer = 4;
    else if (widthGlyph <= 10)
	glyphsPer = 3;
    else if (widthGlyph <= 16)
	glyphsPer = 2;
    else
	glyphsPer = 1;

    widthGlyphs = widthGlyph * glyphsPer;
    widthBlt = widthGlyphs;
    
    /* make sure scanlines are  32-bit aligned */
    if (widthGlyphs <= 24)
	widthBlt = 25;

    cmd = (SIS_CMD_ENH_COLOR_EXPAND | 
	   SIS_CMD_SRC_SCREEN |
	   SIS_CMD_PAT_FG | 
	   SIS_CMD_INC_X |
	   SIS_CMD_INC_Y);
    
    if (imageBlt)
    {
	sis->u.general.clip_right = bbox.x2;
	cmd |= ((sisBltRop[GXcopy] << 8) | 
		SIS_CMD_OPAQUE | 
		SIS_CMD_RECT_CLIP_ENABLE);
    }
    else
    {
	cmd |= ((sisBltRop[pGC->alu] << 8) | 
		SIS_CMD_TRANSPARENT |
		SIS_CMD_RECT_CLIP_DISABLE);
    }
    
    sisExpandInit (pDrawable->pScreen, &expand);
    
    sis->u.general.src_fg = pGC->fgPixel;
    sis->u.general.src_bg = pGC->bgPixel;
    
    bwidth = (widthBlt + 7) >> 3;
    
    nb = bwidth * h;
    
#define LoopIt(count, loadup, fetch) \
    while (nglyph >= count) \
    { \
	nglyph -= count; \
	dst = sisExpandAlloc (&expand, nb); \
	sis->u.general.src_base = expand.off; \
	sis->u.general.src_pitch = 0; \
	sis->u.general.src_x = 0; \
	sis->u.general.src_y = 0; \
	sis->u.general.dst_x = x; \
	sis->u.general.dst_y = y; \
	sis->u.general.rect_width = widthBlt; \
	sis->u.general.rect_height = h; \
	x += widthGlyphs; \
	loadup \
	lwTmp = h; \
	while (lwTmp--) { \
	    tmp = fetch; \
	    SisInvertBits32(tmp); \
	    *dst++ = tmp; \
	} \
	sis->u.general.command = cmd; \
    }

    switch (glyphsPer) {
    case 5:
	LoopIt(5,
	       char1 = (unsigned long *) (*ppci++)->bits;
	       char2 = (unsigned long *) (*ppci++)->bits;
	       char3 = (unsigned long *) (*ppci++)->bits;
	       char4 = (unsigned long *) (*ppci++)->bits;
	       char5 = (unsigned long *) (*ppci++)->bits;,
	       (*char1++ | ((*char2++ | ((*char3++ | ((*char4++ | (*char5++ 
								   << widthGlyph))
						      << widthGlyph))
					 << widthGlyph))
			    << widthGlyph)));
	break;
    case 4:
	LoopIt(4,
	       char1 = (unsigned long *) (*ppci++)->bits;
	       char2 = (unsigned long *) (*ppci++)->bits;
	       char3 = (unsigned long *) (*ppci++)->bits;
	       char4 = (unsigned long *) (*ppci++)->bits;,
	       (*char1++ | ((*char2++ | ((*char3++ | (*char4++
						      << widthGlyph))
					 << widthGlyph))
			    << widthGlyph)));
	break;
    case 3:
	LoopIt(3,
	       char1 = (unsigned long *) (*ppci++)->bits;
	       char2 = (unsigned long *) (*ppci++)->bits;
	       char3 = (unsigned long *) (*ppci++)->bits;,
	       (*char1++ | ((*char2++ | (*char3++ << widthGlyph)) << widthGlyph)));
	break;
    case 2:
	LoopIt(2,
	       char1 = (unsigned long *) (*ppci++)->bits;
	       char2 = (unsigned long *) (*ppci++)->bits;,
	       (*char1++ | (*char2++ << widthGlyph)));
	break;
    }
    
    widthBlt = (widthGlyph + 31) & ~31;
		
    bwidth = widthBlt >> 3;
    
    nb = bwidth * h;
    
    lw = (widthBlt >> 5) * h;
    
    while (nglyph--)
    {
	dst = (CARD32 *) sisExpandAlloc (&expand, nb);
	char1 = (CARD32 *) (*ppci++)->bits;
	sis->u.general.src_base = expand.off;
	sis->u.general.src_pitch = 0;
	sis->u.general.src_x = 0;
	sis->u.general.src_y = 0;
	sis->u.general.dst_x = x;
	sis->u.general.dst_y = y;
	sis->u.general.rect_width = widthBlt;
	sis->u.general.rect_height = h;
	lwTmp = lw;
	while (lwTmp--)
	{
	    tmp = *char1++;
	    SisInvertBits32 (tmp);
	    *dst++ = tmp;
	}
	sis->u.general.command = cmd;
	x += widthGlyph;
    }
    if (imageBlt)
	sis->u.general.clip_right = pScreenPriv->screen->width;
    KdMarkSync (pDrawable->pScreen);
    return TRUE;
}

Bool
sisGlyphBlt(DrawablePtr	    pDrawable, 
	    GCPtr	    pGC, 
	    int		    x, 
	    int		    y, 
	    unsigned int    nglyph,
	    CharInfoPtr	    *ppciInit,
	    Bool	    imageBlt)
{
    SetupSis(pDrawable->pScreen);
    int		    height;
    int		    width;
    int		    xBack, yBack;
    int		    hBack, wBack;
    int		    nb, bwidth, nl;
    FontPtr	    pfont = pGC->font;
    CharInfoPtr	    pci;
    CARD8	    *bits8, b;
    CARD16	    *bits16;
    CARD32	    *bits32;
    BoxPtr	    extents;
    BoxRec	    bbox;
    CharInfoPtr	    *ppci;
    unsigned char   alu;
    CARD32	    cmd;
    SisExpand	    expand;
    CARD32	    *dst, d;
    int		    nbytes;
    int		    shift;

    x += pDrawable->x;
    y += pDrawable->y;

    /* compute an approximate (but covering) bounding box */
    ppci = ppciInit;
    width = 0;
    height = nglyph;
    while (height--)
	width += (*ppci++)->metrics.characterWidth;
    if (width < 0)
    {
	bbox.x1 = x + width;
	bbox.x2 = x;
    }
    else
    {
	bbox.x1 = x;
	bbox.x2 = x + width;
    }
    width = FONTMINBOUNDS(pfont,leftSideBearing);
    if (width < 0)
	bbox.x1 += width;
    width = FONTMAXBOUNDS(pfont, rightSideBearing) - FONTMINBOUNDS(pfont, characterWidth);
    if (width > 0)
	bbox.x2 += width;
    bbox.y1 = y - FONTMAXBOUNDS(pfont,ascent);
    bbox.y2 = y + FONTMAXBOUNDS(pfont,descent);
    
    switch (RECT_IN_REGION(pGC->pScreen, fbGetCompositeClip(pGC), &bbox))
    {
    case rgnPART:
	if (bbox.x1 < 0 || bbox.y1 < 0)
	    return FALSE;
	sisGlyphBltClipped (pDrawable, pGC, 
			    x - pDrawable->x, y - pDrawable->y, 
			    nglyph, ppciInit, imageBlt);
    case rgnOUT:
	return TRUE;
    }
    
    if (imageBlt)
    {
	xBack = x;
	yBack = y - FONTASCENT(pGC->font);
	wBack = 0;
	hBack = FONTASCENT(pGC->font) + FONTDESCENT(pGC->font);
	if (hBack)
	{
	    height = nglyph;
	    ppci = ppciInit;
	    while (height--)
		wBack += (*ppci++)->metrics.characterWidth;
	}
	if (wBack < 0)
	{
	    xBack = xBack + wBack;
	    wBack = -wBack;
	}
	if (hBack < 0)
	{
	    yBack = yBack + hBack;
	    hBack = -hBack;
	}
	alu = GXcopy;
    }
    else
    {
	wBack = 0;
	alu = pGC->alu;
    }
    
    if (wBack)
    {
	_sisSetSolidRect (sis, pGC->bgPixel, GXcopy, cmd);
	_sisRect (sis, xBack, yBack, wBack, hBack, cmd);
    }
    
    sisExpandInit (pDrawable->pScreen, &expand);
    
    sis->u.general.src_fg = pGC->fgPixel;
    
    cmd = (SIS_CMD_ENH_COLOR_EXPAND | 
	   SIS_CMD_SRC_SCREEN |
	   SIS_CMD_PAT_FG | 
	   (sisBltRop[alu] << 8) | 
	   SIS_CMD_INC_X |
	   SIS_CMD_INC_Y |
	   SIS_CMD_RECT_CLIP_DISABLE |
	   SIS_CMD_TRANSPARENT);
	
    ppci = ppciInit;
    while (nglyph--)
    {
	pci = *ppci++;
	height = pci->metrics.ascent + pci->metrics.descent;
	width = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
	/*
	 * For glyphs wider than 16 pixels, expand the blt to the nearest multiple
	 * of 32; this allows the scanlines to be padded to a 32-bit boundary
	 * instead of requiring byte packing
	 */
	if (width > 16)
	    width = (width + 31) & ~31;
	bwidth = (width + 7) >> 3;
	nb = bwidth * height;
	if (nb)
	{
	    dst = sisExpandAlloc (&expand, nb);

	    sis->u.general.src_base = expand.off;
	    sis->u.general.src_pitch = 0;
	    sis->u.general.src_x = 0;
	    sis->u.general.src_y = 0;
	    sis->u.general.dst_x = x + pci->metrics.leftSideBearing;
	    sis->u.general.dst_y = y - pci->metrics.ascent;
	    sis->u.general.rect_width = width;
	    sis->u.general.rect_height = height;
	    switch (bwidth) {
	    case 1:
		bits8 = (CARD8 *) pci->bits;
		while (height >= 4)
		{
		    d = (bits8[0] | (bits8[4] << 8) | 
			 (bits8[8] << 16) | (bits8[12] << 24));
		    SisInvertBits32(d);
		    *dst++ = d;
		    bits8 += 16;
		    height -= 4;
		}
		if (height)
		{
		    switch (height) {
		    case 3:
			d = bits8[0] | (bits8[4] << 8) | (bits8[8] << 16);
			break;
		    case 2:
			d = bits8[0] | (bits8[4] << 8);
			break;
		    case 1:
			d = bits8[0];
			break;
		    }
		    SisInvertBits32(d);
		    *dst++ = d;
		}
		break;
	    case 2:
		bits16 = (CARD16 *) pci->bits;
		while (height >= 2)
		{
		    d = bits16[0] | (bits16[2] << 16);
		    SisInvertBits32(d);
		    *dst++ = d;
		    bits16 += 4;
		    height -= 2;
		}
		if (height)
		{
		    d = bits16[0];
		    SisInvertBits32(d);
		    *dst++ = d;
		}
		break;
	    default:
		nb >>= 2;
		bits32 = (CARD32 *) pci->bits;
		while (nb--)
		{
		    d = *bits32++;
		    SisInvertBits32 (d);
		    *dst++ = d;
		}
	    }
	    sis->u.general.command = cmd;
	}
	x += pci->metrics.characterWidth;
    }
    KdMarkSync (pDrawable->pScreen);
    return TRUE;
}
/*
 * Blt glyphs using Sis image transfer register, this does both
 * poly glyph blt and image glyph blt (when pglyphBase == 1)
 */

void
sisPolyGlyphBlt (DrawablePtr pDrawable, 
		 GCPtr pGC, 
		 int x, int y, 
		 unsigned int nglyph,
		 CharInfoPtr *ppci, 
		 pointer pglyphBase)
{
    FbBits	    depthMask;
    
    depthMask = FbFullMask (pDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask &&
	pGC->fillStyle == FillSolid)
    {
	if (TERMINALFONT(pGC->font))
	{
	    if (sisTEGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, FALSE))
		return;
	}
	else
	{
	    if (sisGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, FALSE))
		return;
	}
    }
    KdCheckPolyGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
}

void
sisImageGlyphBlt (DrawablePtr pDrawable, 
		  GCPtr pGC, 
		  int x, int y, 
		  unsigned int nglyph, 
		  CharInfoPtr *ppci, 
		  pointer pglyphBase)
{
    FbBits	    depthMask;
    
    depthMask = FbFullMask (pDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask)
    {
	if (TERMINALFONT(pGC->font))
	{
	    if (sisTEGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, TRUE))
		return;
	}
	else
	{
	    if (sisGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, TRUE))
		return;
	}
    }
    KdCheckImageGlyphBlt (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
}

#define sourceInvarient(alu)	(((alu) & 3) == (((alu) >> 2) & 3))

#define sisPatternDimOk(d)  ((d) <= 8 && (((d) & ((d) - 1)) == 0))

BOOL
sisFillOk (GCPtr pGC)
{
    FbBits  depthMask;

    depthMask = FbFullMask(pGC->depth);
    if ((pGC->planemask & depthMask) != depthMask)
	return FALSE;
    switch (pGC->fillStyle) {
    case FillSolid:
	return TRUE;
    case FillTiled:
	return (sisPatternDimOk (pGC->tile.pixmap->drawable.width) &&
		sisPatternDimOk (pGC->tile.pixmap->drawable.height));
    case FillStippled:
    case FillOpaqueStippled:
	return (sisPatternDimOk (pGC->stipple->drawable.width) &&
		sisPatternDimOk (pGC->stipple->drawable.height));
    }
}

CARD32
sisStipplePrepare (DrawablePtr pDrawable, GCPtr pGC)
{
    SetupSis(pGC->pScreen);
    PixmapPtr   pStip = pGC->stipple;
    int		stipHeight = pStip->drawable.height;
    int		xRot, yRot;
    int		rot, stipX, stipY;
    FbStip	*stip, *stipEnd, bits;
    FbStride    stipStride;
    int		stipBpp;
    int		stipXoff, stipYoff; /* XXX assumed to be zero */
    int		y;
    CARD32	cmd;
    
    xRot = pGC->patOrg.x + pDrawable->x;
    yRot = pGC->patOrg.y + pDrawable->y;
    modulus (- yRot, stipHeight, stipY);
    modulus (- xRot, FB_UNIT, stipX);
    rot = stipX;

    fbGetStipDrawable (&pStip->drawable, stip, stipStride, stipBpp, stipXoff, stipYoff);
    for (y = 0; y < 8; y++)
    {
	bits = stip[stipY<<1];
	FbRotLeft(bits, rot);
	SisInvertBits32(bits);
	sis->u.general.mask[y] = (CARD8) bits;
	stipY++;
	if (stipY == stipHeight)
	    stipY = 0;
    }
    sis->u.general.pattern_fg = pGC->fgPixel;
    
    cmd = (SIS_CMD_BITBLT |
	   SIS_CMD_SRC_SCREEN |
	   SIS_CMD_PAT_MONO |
	   (sisPatRop[pGC->alu] << 8) |
	   SIS_CMD_INC_X |
	   SIS_CMD_INC_Y |
	   SIS_CMD_RECT_CLIP_DISABLE |
	   SIS_CMD_RECT_CLIP_DONT_MERGE); 
    if (pGC->fillStyle == FillOpaqueStippled)
    {
	sis->u.general.pattern_bg = pGC->bgPixel;
	cmd |= SIS_CMD_OPAQUE;
    }
    else
	cmd |= SIS_CMD_TRANSPARENT;
    return cmd;
}

CARD32
sisTilePrepare (PixmapPtr pTile, int xRot, int yRot, CARD8 alu)
{
    SetupSis(pTile->drawable.pScreen);
    int		tileHeight = pTile->drawable.height;
    int		tileWidth = pTile->drawable.width;
    FbBits	*tile;
    FbStride	tileStride;
    int		tileBpp;
    int		tileXoff, tileYoff; /* XXX assumed to be zero */

    fbGetDrawable (&pTile->drawable, tile, tileStride, tileBpp, tileXoff, tileYoff);
    
    /*
     * Tile the pattern register
     */
    fbTile ((FbBits *) sis->u.general.pattern,
	    (8 * tileBpp) >> FB_SHIFT,
	    0,
	    
	    8 * tileBpp, 8,
	    
	    tile,
	    tileStride,
	    tileWidth * tileBpp,
	    tileHeight,
	    GXcopy, FB_ALLONES, tileBpp,
	    xRot * tileBpp,
	    yRot);
    
    return (SIS_CMD_BITBLT |
	    SIS_CMD_SRC_SCREEN |
	    SIS_CMD_PAT_PATTERN |
	    (sisPatRop[alu] << 8) |
	    SIS_CMD_INC_X |
	    SIS_CMD_INC_Y |
	    SIS_CMD_RECT_CLIP_DISABLE |
	    SIS_CMD_RECT_CLIP_DONT_MERGE);
}

void
sisFillBoxSolid (DrawablePtr pDrawable, int nBox, BoxPtr pBox, 
		unsigned long pixel, int alu)
{
    SetupSis(pDrawable->pScreen);
    CARD32	cmd;

    _sisSetSolidRect(sis,pixel,alu,cmd);
    
    while (nBox--) 
    {
	_sisRect(sis,pBox->x1,pBox->y1,pBox->x2-pBox->x1,pBox->y2-pBox->y1,cmd);
	pBox++;
    }
    KdMarkSync (pDrawable->pScreen);
}

void
sisFillBoxStipple (DrawablePtr pDrawable, GCPtr pGC,
		   int nBox, BoxPtr pBox)
{
    SetupSis(pDrawable->pScreen);
    CARD32	cmd;
    
    cmd = sisStipplePrepare (pDrawable, pGC);

    while (nBox--) 
    {
	_sisRect(sis,pBox->x1,pBox->y1,pBox->x2-pBox->x1,pBox->y2-pBox->y1,cmd);
	pBox++;
    }
    KdMarkSync (pDrawable->pScreen);
}

void
sisFillBoxTiled (DrawablePtr pDrawable,
		 int nBox, BoxPtr pBox,
		 PixmapPtr pTile, int xRot, int yRot, CARD8 alu)
{
    SetupSis (pDrawable->pScreen);
    CARD32	cmd;

    cmd = sisTilePrepare (pTile, xRot, yRot, alu);

    while (nBox--) 
    {
	_sisRect(sis,pBox->x1,pBox->y1,pBox->x2-pBox->x1,pBox->y2-pBox->y1,cmd);
	pBox++;
    }
    KdMarkSync (pDrawable->pScreen);
}

/*
  sisDoBitBlt
  =============
  Bit Blit for all window to window blits.
*/

void
sisCopyNtoN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure)
{
    SetupSis(pDstDrawable->pScreen);
    int	    srcX, srcY, dstX, dstY;
    int	    w, h;
    CARD32  flags;
    CARD32  cmd;
    CARD8   alu;
    
    if (pGC)
    {
	alu = pGC->alu;
	if (sourceInvarient (pGC->alu))
	{
	    sisFillBoxSolid (pDstDrawable, nbox, pbox, 0, pGC->alu);
	    return;
	}
    }
    else
	alu = GXcopy;
    
    _sisSetBlt(sis,alu,cmd);
    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	flags = 0;
	if (reverse)
	{
	    dstX = pbox->x2 - 1;
	}
	else
	{
	    dstX = pbox->x1;
	    flags |= SIS_CMD_INC_X;
	}
	srcX = dstX + dx;
	
	if (upsidedown)
	{
	    dstY = pbox->y2 - 1;
	}
	else
	{
	    dstY = pbox->y1;
	    flags |= SIS_CMD_INC_Y;
	}
	srcY = dstY + dy;
	
	_sisBlt (sis, srcX, srcY, dstX, dstY, w, h, cmd|flags);
	pbox++;
    }
    KdMarkSync (pDstDrawable->pScreen);
}

RegionPtr
sisCopyArea(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	   int srcx, int srcy, int width, int height, int dstx, int dsty)
{
    KdScreenPriv(pDstDrawable->pScreen);
    FbBits	    depthMask;
    
    depthMask = FbFullMask (pDstDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask &&
	pSrcDrawable->type == DRAWABLE_WINDOW &&
	pDstDrawable->type == DRAWABLE_WINDOW)
    {
	return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
			 srcx, srcy, width, height, 
			 dstx, dsty, sisCopyNtoN, 0, 0);
    }
    return KdCheckCopyArea (pSrcDrawable, pDstDrawable, pGC, 
		       srcx, srcy, width, height, dstx, dsty);
}

typedef struct _sis1toNargs {
    unsigned long	copyPlaneFG, copyPlaneBG;
} sis1toNargs;

void
_sisStipple (ScreenPtr	pScreen,
	     FbStip	*psrcBase,
	     FbStride	widthSrc,
	     CARD8	alu,
	     int	srcx,
	     int	srcy,
	     int	dstx,
	     int	dsty,
	     int	width,
	     int	height)
{
    SetupSis(pScreen);
    FbStip	*psrcLine, *psrc;
    FbStride	widthRest;
    FbStip	bits, tmp, lastTmp;
    int		leftShift, rightShift;
    int		nl, nlMiddle;
    int		r;
    SisExpand	expand;
    CARD32	*dst;
    int		hthis;
    int		hper;
    int		bwidth;
    CARD32	cmd;
    
    sisExpandInit (pScreen, &expand);
    
    /* Compute blt address and parameters */
    psrc = psrcBase + srcy * widthSrc + (srcx >> 5);
    nlMiddle = (width + 31) >> 5;
    leftShift = srcx & 0x1f;
    rightShift = 32 - leftShift;
    widthRest = widthSrc - nlMiddle;
    
    cmd = (SIS_CMD_ENH_COLOR_EXPAND |
	   SIS_CMD_SRC_SCREEN |
	   SIS_CMD_PAT_FG |
	   (sisBltRop[alu] << 8) |
	   SIS_CMD_INC_X |
	   SIS_CMD_INC_Y |
	   SIS_CMD_OPAQUE |
	   SIS_CMD_RECT_CLIP_ENABLE);
    
    if (leftShift != 0)
	widthRest--;
    
    sis->u.general.src_x = 0;
    sis->u.general.src_y = 0;
    sis->u.general.dst_x = dstx;
    sis->u.general.rect_width = (width + 31) & ~31;
    sis->u.general.clip_right = (dstx + width);
    
    bwidth = nlMiddle << 2;
    hper = SIS_PATTERN_INC / bwidth;
    if (hper == 0)
	hper = 1;
    
    while (height)
    {
	hthis = hper;
	if (hthis > height)
	    hthis = height;
	dst = sisExpandAlloc (&expand, bwidth * hthis);
	sis->u.general.src_base = expand.off;
	sis->u.general.dst_y = dsty;
	sis->u.general.rect_height = hthis;
	
	dsty += hthis;
	height -= hthis;
	
	if (leftShift == 0)
	{
	    while (hthis--)
	    {
		nl = nlMiddle;
		while (nl--)
		{
		    tmp = *psrc++;
		    SisInvertBits32(tmp);
		    *dst++ = tmp;
		}
		psrc += widthRest;
	    }
	}
	else
	{
	    while (hthis--)
	    {
		bits = *psrc++;
		nl = nlMiddle;
		while (nl--)
		{
		    tmp = FbStipLeft(bits, leftShift);
		    bits = *psrc++;
		    tmp |= FbStipRight(bits, rightShift);
		    SisInvertBits32(tmp);
		    *dst++ = tmp;
		}
		psrc += widthRest;
	    }
	}
	sis->u.general.command = cmd;
    }
    sis->u.general.clip_right = pScreenPriv->screen->width;
}
	    
void
sisCopy1toN (DrawablePtr	pSrcDrawable,
	    DrawablePtr	pDstDrawable,
	    GCPtr	pGC,
	    BoxPtr	pbox,
	    int		nbox,
	    int		dx,
	    int		dy,
	    Bool	reverse,
	    Bool	upsidedown,
	    Pixel	bitplane,
	    void	*closure)
{
    SetupSis(pDstDrawable->pScreen);
    
    sis1toNargs		*args = closure;
    int			dstx, dsty;
    FbStip		*psrcBase;
    FbStride		widthSrc;
    int			srcBpp;
    int			srcXoff, srcYoff;

    if (sourceInvarient (pGC->alu))
    {
	sisFillBoxSolid (pDstDrawable, nbox, pbox,
			 pGC->bgPixel, pGC->alu);
	return;
    }
    
    fbGetStipDrawable (pSrcDrawable, psrcBase, widthSrc, srcBpp, srcXoff, srcYoff);
    
    sis->u.general.src_fg = args->copyPlaneFG;
    sis->u.general.src_bg = args->copyPlaneBG;
    
    while (nbox--)
    {
	dstx = pbox->x1;
	dsty = pbox->y1;
	
	_sisStipple (pDstDrawable->pScreen,
		     psrcBase, widthSrc, 
		     pGC->alu,
		     dstx + dx - srcXoff, dsty + dy - srcYoff,
		     dstx, dsty, 
		     pbox->x2 - dstx, pbox->y2 - dsty);
	pbox++;
    }
    KdMarkSync (pDstDrawable->pScreen);
}

RegionPtr
sisCopyPlane(DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, GCPtr pGC,
	int srcx, int srcy, int width, int height, 
	int dstx, int dsty, unsigned long bitPlane)
{
    KdScreenPriv (pDstDrawable->pScreen);
    RegionPtr		ret;
    sis1toNargs		args;
    FbBits		depthMask;

    depthMask = FbFullMask (pDstDrawable->depth);
    if ((pGC->planemask & depthMask) == depthMask &&
	pDstDrawable->type == DRAWABLE_WINDOW &&
	pSrcDrawable->depth == 1)
    {
	args.copyPlaneFG = pGC->fgPixel;
	args.copyPlaneBG = pGC->bgPixel;
	return fbDoCopy (pSrcDrawable, pDstDrawable, pGC, 
			 srcx, srcy, width, height, 
			 dstx, dsty, sisCopy1toN, bitPlane, &args);
    }
    return KdCheckCopyPlane(pSrcDrawable, pDstDrawable, pGC, 
		       srcx, srcy, width, height, 
		       dstx, dsty, bitPlane);
}

void
sisFillSpans (DrawablePtr pDrawable, GCPtr pGC, int n, 
	     DDXPointPtr ppt, int *pwidth, int fSorted)
{
    SetupSis(pDrawable->pScreen);
    DDXPointPtr	    pptFree;
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate(pGC);
    int		    *pwidthFree;/* copies of the pointers to free */
    CARD32	    cmd;
    int		    nTmp;
    INT16	    x, y;
    int		    width;
    
    if (!sisFillOk (pGC))
    {
	KdCheckFillSpans (pDrawable, pGC, n, ppt, pwidth, fSorted);
	return;
    }
    nTmp = n * miFindMaxBand(fbGetCompositeClip(pGC));
    pwidthFree = (int *)xalloc(nTmp * sizeof(int));
    pptFree = (DDXPointRec *)xalloc(nTmp * sizeof(DDXPointRec));
    if(!pptFree || !pwidthFree)
    {
	if (pptFree) xfree(pptFree);
	if (pwidthFree) xfree(pwidthFree);
	return;
    }
    n = miClipSpans(fbGetCompositeClip(pGC),
		     ppt, pwidth, n,
		     pptFree, pwidthFree, fSorted);
    pwidth = pwidthFree;
    ppt = pptFree;
    switch (pGC->fillStyle) {
    case FillSolid:
	_sisSetSolidRect(sis,pGC->fgPixel,pGC->alu,cmd);
	break;
    case FillTiled:
	cmd = sisTilePrepare (pGC->tile.pixmap,
			      pGC->patOrg.x + pDrawable->x,
			      pGC->patOrg.y + pDrawable->y,
			      pGC->alu);
	break;
    default:
	cmd = sisStipplePrepare (pDrawable, pGC);
	break;
    }
    while (n--)
    {
	x = ppt->x;
	y = ppt->y;
	ppt++;
	width = *pwidth++;
	if (width)
	{
	    _sisRect(sis,x,y,width,1,cmd);
	}
    }
    KdMarkSync (pDrawable->pScreen);
    xfree(pptFree);
    xfree(pwidthFree);
}

#define NUM_STACK_RECTS	1024

void
sisPolyFillRect (DrawablePtr pDrawable, GCPtr pGC, 
		int nrectFill, xRectangle *prectInit)
{
    SetupSis(pDrawable->pScreen);
    xRectangle	    *prect;
    RegionPtr	    prgnClip;
    register BoxPtr pbox;
    register BoxPtr pboxClipped;
    BoxPtr	    pboxClippedBase;
    BoxPtr	    pextent;
    BoxRec	    stackRects[NUM_STACK_RECTS];
    FbGCPrivPtr	    fbPriv = fbGetGCPrivate (pGC);
    int		    numRects;
    int		    n;
    int		    xorg, yorg;
    int		    x, y;
    
    if (!sisFillOk (pGC))
    {
	KdCheckPolyFillRect (pDrawable, pGC, nrectFill, prectInit);
	return;
    }
    prgnClip = fbGetCompositeClip(pGC);
    xorg = pDrawable->x;
    yorg = pDrawable->y;
    
    if (xorg || yorg)
    {
	prect = prectInit;
	n = nrectFill;
	while(n--)
	{
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
	}
    }
    
    prect = prectInit;

    numRects = REGION_NUM_RECTS(prgnClip) * nrectFill;
    if (numRects > NUM_STACK_RECTS)
    {
	pboxClippedBase = (BoxPtr)xalloc(numRects * sizeof(BoxRec));
	if (!pboxClippedBase)
	    return;
    }
    else
	pboxClippedBase = stackRects;

    pboxClipped = pboxClippedBase;
	
    if (REGION_NUM_RECTS(prgnClip) == 1)
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_RECTS(prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    if ((pboxClipped->x1 = prect->x) < x1)
		pboxClipped->x1 = x1;
    
	    if ((pboxClipped->y1 = prect->y) < y1)
		pboxClipped->y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    pboxClipped->x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    pboxClipped->y2 = by2;

	    prect++;
	    if ((pboxClipped->x1 < pboxClipped->x2) &&
		(pboxClipped->y1 < pboxClipped->y2))
	    {
		pboxClipped++;
	    }
    	}
    }
    else
    {
	int x1, y1, x2, y2, bx2, by2;

	pextent = REGION_EXTENTS(pGC->pScreen, prgnClip);
	x1 = pextent->x1;
	y1 = pextent->y1;
	x2 = pextent->x2;
	y2 = pextent->y2;
    	while (nrectFill--)
    	{
	    BoxRec box;
    
	    if ((box.x1 = prect->x) < x1)
		box.x1 = x1;
    
	    if ((box.y1 = prect->y) < y1)
		box.y1 = y1;
    
	    bx2 = (int) prect->x + (int) prect->width;
	    if (bx2 > x2)
		bx2 = x2;
	    box.x2 = bx2;
    
	    by2 = (int) prect->y + (int) prect->height;
	    if (by2 > y2)
		by2 = y2;
	    box.y2 = by2;
    
	    prect++;
    
	    if ((box.x1 >= box.x2) || (box.y1 >= box.y2))
	    	continue;
    
	    n = REGION_NUM_RECTS (prgnClip);
	    pbox = REGION_RECTS(prgnClip);
    
	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
		pboxClipped->x1 = max(box.x1, pbox->x1);
		pboxClipped->y1 = max(box.y1, pbox->y1);
		pboxClipped->x2 = min(box.x2, pbox->x2);
		pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

		/* see if clipping left anything */
		if(pboxClipped->x1 < pboxClipped->x2 && 
		   pboxClipped->y1 < pboxClipped->y2)
		{
		    pboxClipped++;
		}
	    }
    	}
    }
    if (pboxClipped != pboxClippedBase)
    {
	switch (pGC->fillStyle) {
	case FillSolid:
	    sisFillBoxSolid(pDrawable,
			   pboxClipped-pboxClippedBase, pboxClippedBase,
			   pGC->fgPixel, pGC->alu);
	    break;
	case FillTiled:
	    sisFillBoxTiled(pDrawable,
			    pboxClipped-pboxClippedBase, pboxClippedBase,
			    pGC->tile.pixmap,
			    pGC->patOrg.x + pDrawable->x,
			    pGC->patOrg.y + pDrawable->y,
			    pGC->alu);
	    break;
	case FillStippled:
	case FillOpaqueStippled:
	    sisFillBoxStipple (pDrawable, pGC,
			       pboxClipped-pboxClippedBase, pboxClippedBase);
	    break;
	}
    }
    if (pboxClippedBase != stackRects)
    	xfree(pboxClippedBase);
}

static const GCOps sisOps = {
    sisFillSpans,
    KdCheckSetSpans,
    KdCheckPutImage,
    sisCopyArea,
    sisCopyPlane,
    KdCheckPolyPoint,
    KdCheckPolylines,
    KdCheckPolySegment,
    miPolyRectangle,
    KdCheckPolyArc,
    miFillPolygon,
    sisPolyFillRect,
    KdCheckPolyFillArc,
    miPolyText8,
    miPolyText16,
    miImageText8,
    miImageText16,
    sisImageGlyphBlt,
    sisPolyGlyphBlt,
    KdCheckPushPixels,
};

void
sisValidateGC (GCPtr pGC, Mask changes, DrawablePtr pDrawable)
{
    FbGCPrivPtr fbPriv = fbGetGCPrivate(pGC);
    
    fbValidateGC (pGC, changes, pDrawable);
    
    if (pDrawable->type == DRAWABLE_WINDOW)
	pGC->ops = (GCOps *) &sisOps;
    else
	pGC->ops = (GCOps *) &kdAsyncPixmapGCOps;
}

GCFuncs	sisGCFuncs = {
    sisValidateGC,
    miChangeGC,
    miCopyGC,
    miDestroyGC,
    miChangeClip,
    miDestroyClip,
    miCopyClip
};

int
sisCreateGC (GCPtr pGC)
{
    if (!fbCreateGC (pGC))
	return FALSE;

    if (pGC->depth != 1)
	pGC->funcs = &sisGCFuncs;
    
    pGC->ops = (GCOps *) &kdAsyncPixmapGCOps;
    
    return TRUE;
}

void
sisCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg, RegionPtr prgnSrc)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    KdScreenPriv(pScreen);
    RegionRec	rgnDst;
    int		dx, dy;
    WindowPtr	pwinRoot;

    pwinRoot = WindowTable[pWin->drawable.pScreen->myNum];

    dx = ptOldOrg.x - pWin->drawable.x;
    dy = ptOldOrg.y - pWin->drawable.y;
    REGION_TRANSLATE(pWin->drawable.pScreen, prgnSrc, -dx, -dy);

    REGION_INIT (pWin->drawable.pScreen, &rgnDst, NullBox, 0);
    
    REGION_INTERSECT(pWin->drawable.pScreen, &rgnDst, &pWin->borderClip, prgnSrc);

    fbCopyRegion ((DrawablePtr)pwinRoot, (DrawablePtr)pwinRoot,
		  0,
		  &rgnDst, dx, dy, sisCopyNtoN, 0, 0);
    
    REGION_UNINIT(pWin->drawable.pScreen, &rgnDst);
}

Bool
sisDrawInit (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    sisScreenInfo(pScreenPriv);
    
    /*
     * Hook up asynchronous drawing
     */
    KdScreenInitAsync (pScreen);
    /*
     * Replace various fb screen functions
     */
    pScreen->CreateGC = sisCreateGC;
    pScreen->CopyWindow = sisCopyWindow;

    return TRUE;
}

void
sisDrawEnable (ScreenPtr pScreen)
{
    SetupSis(pScreen);
    sisScreenInfo(pScreenPriv);
    CARD32  cmd;
    CARD32  base;
    CARD16  stride;
    CARD16  op;
    
    base = pScreenPriv->screen->fb[0].frameBuffer - sisc->frameBuffer;
    stride = pScreenPriv->screen->fb[0].byteStride;
#if 0    
    sis->u.general.dst_base = base;
    sis->u.general.dst_pitch = stride;
    sis->u.general.src_pitch = stride;
    sis->u.general._pad0 = stride;
    sis->u.general.dst_height = pScreenPriv->screen->height;
    _sisClip (sis, 0, 0, 
	      pScreenPriv->screen->width, pScreenPriv->screen->height);
    _sisSetSolidRect(sis, pScreen->blackPixel, GXcopy, cmd);
    _sisRect (sis, 0, 0, 
	      pScreenPriv->screen->width, pScreenPriv->screen->height,
	      cmd);
#endif
    base = (CARD32) (pScreenPriv->screen->fb[0].frameBuffer);
    fprintf (stderr, "src 0x%x\n", sis->u.accel.src_addr);
    sis->u.accel.src_addr = (base & 0x3fffff);
    fprintf (stderr, "src 0x%x\n", sis->u.accel.src_addr);
    sis->u.accel.dst_addr = (base & 0x3fffff);
    sis->u.accel.pitch = (stride << 16) | stride;
    sis->u.accel.dimension = ((pScreenPriv->screen->height-1) << 16 | 
			      (pScreenPriv->screen->width - 1));
    sis->u.accel.fg = (sisBltRop[GXcopy] << 24) | 0xf800;
    sis->u.accel.bg = (sisBltRop[GXcopy] << 24) | 0x00;
    
#define sisLEFT2RIGHT           0x10
#define sisRIGHT2LEFT           0x00
#define sisTOP2BOTTOM           0x20
#define sisBOTTOM2TOP           0x00

#define sisSRCSYSTEM            0x03
#define sisSRCVIDEO		0x02
#define sisSRCFG		0x01
#define sisSRCBG		0x00

#define sisCMDBLT		0x0000
#define sisCMDBLTMSK		0x0100
#define sisCMDCOLEXP		0x0200
#define sisCMDLINE		0x0300

#define sisCMDENHCOLEXP		0x2000

#define sisXINCREASE		0x10
#define sisYINCREASE		0x20
#define sisCLIPENABL		0x40
#define sisCLIPINTRN		0x80 
#define sisCLIPEXTRN		0x00


#define sisPATREG		0x08
#define sisPATFG		0x04
#define sisPATBG		0x00

#define sisLASTPIX		0x0800
#define sisXMAJOR		0x0400

    op = sisCMDBLT | sisLEFT2RIGHT | sisTOP2BOTTOM | sisSRCFG | sisPATFG;
    
    sis->u.accel.cmd = op;
    
    KdMarkSync (pScreen);
}

void
sisDrawSync (ScreenPtr pScreen)
{
    SetupSis(pScreen);
    
    _sisWaitIdleEmpty (sis);
}

void
sisDrawDisable (ScreenPtr pScreen)
{
}

void
sisDrawFini (ScreenPtr pScreen)
{
}
