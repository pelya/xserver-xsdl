/*
 * $Id$
 *
 * Copyright © 2004 Damien Ciabrini
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Anders Carlsson not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Anders Carlsson makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * DAMIEN CIABRINI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ANDERS CARLSSON BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mga.h"
#include "g400_common.h"


static PixmapPtr currentSrc;
static PixmapPtr currentMask;

struct blendinfo {
	Bool dst_alpha;
	Bool src_alpha;
	CARD32 blend_cntl;
};

static struct blendinfo mgaBlendOP[] = {
	/* Clear */
	{0, 0, MGA_SRC_ZERO			| MGA_DST_ZERO},
	/* Src */
	{0, 0, MGA_SRC_ONE			| MGA_DST_ZERO},
	/* Dst */
	{0, 0, MGA_SRC_ZERO			| MGA_DST_ONE},
	/* Over */
	{0, 1, MGA_SRC_ONE			| MGA_DST_ONE_MINUS_SRC_ALPHA},
	/* OverReverse */
	{1, 0, MGA_SRC_ONE_MINUS_DST_ALPHA	| MGA_DST_ONE},
	/* In */
	{1, 0, MGA_SRC_DST_ALPHA		| MGA_DST_ZERO},
	/* InReverse */
	{0, 1, MGA_SRC_ZERO			| MGA_DST_SRC_ALPHA},
	/* Out */
	{1, 0, MGA_SRC_ONE_MINUS_DST_ALPHA	| MGA_DST_ZERO},
	/* OutReverse */
	{0, 1, MGA_SRC_ZERO			| MGA_DST_ONE_MINUS_SRC_ALPHA},
	/* Atop */
	{1, 1, MGA_SRC_DST_ALPHA		| MGA_DST_ONE_MINUS_SRC_ALPHA},
	/* AtopReverse */
	{1, 1, MGA_SRC_ONE_MINUS_DST_ALPHA	| MGA_DST_SRC_ALPHA},
	/* Xor */
	{1, 1, MGA_SRC_ONE_MINUS_DST_ALPHA	| MGA_DST_ONE_MINUS_SRC_ALPHA},
	/* Add */
	{0, 0, MGA_SRC_ONE			| MGA_DST_ONE},
};

struct formatinfo {
	int fmt;
	CARD32 card_fmt;
};

static struct formatinfo texformats[] = {
    {PICT_a8r8g8b8, MGA_TW32},
    {PICT_x8r8g8b8, MGA_TW32},
    {PICT_r5g6b5, MGA_TW16},
    {PICT_a1r5g5b5, MGA_TW15},
    {PICT_x1r5g5b5, MGA_TW15},
    {PICT_a4r4g4b4, MGA_TW12},
    {PICT_x4r4g4b4, MGA_TW12},
    {PICT_a8, MGA_TW8A},
};

static int MGA_LOG2( int val )
{
    int ret = 0;
    
    while (val >> ret)
	ret++;
    
    return ((1 << (ret-1)) == val) ? (ret-1) : ret;
}

static Bool
mgaCheckSourceTexture (int		tmu,
		       PicturePtr 	pPict)
{
    int w = pPict->pDrawable->width;
    int h = pPict->pDrawable->height;
    int i;
    CARD32 texctl = 0;

    if ((w > 2047) || (h > 2047))
	MGA_FALLBACK(("Picture too large for composition (%dx%d)\n", w, h));

    for (i = 0; i < sizeof(texformats) / sizeof(texformats[0]); i++) {
	if (texformats[i].fmt == pPict->format) {
	    texctl = texformats[i].card_fmt;
	    break;
	}
    }
    if (texctl == 0) {
	    MGA_FALLBACK(("Picture is in unsupported format 0x%x\n"
			  pPict->format));
    }

    return TRUE;
}

static Bool
PrepareSourceTexture (int		tmu,
		      PicturePtr 	pSrcPicture,
		      PixmapPtr 	pSrc)
{
    KdScreenPriv (pSrc->drawable.pScreen);  
    int mem_base=(int)pScreenPriv->screen->memory_base;
    int pitch = pSrc->devKind / (pSrc->drawable.bitsPerPixel >> 3); 
    int i;

    int w = pSrc->drawable.width;
    int h = pSrc->drawable.height;
    int w_log2 = MGA_LOG2(w);
    int h_log2 = MGA_LOG2(h);

    int texctl = MGA_PITCHLIN | ((pitch & (2048 - 1)) << 9) | 
	MGA_CLAMPUV | MGA_NOPERSPECTIVE;  
    int texctl2=MGA_G400_TC2_MAGIC;
 
    for (i = 0; i < sizeof(texformats) / sizeof(texformats[0]); i++) {
	if (texformats[i].fmt == pSrcPicture->format) {
	    texctl |= texformats[i].card_fmt;
	    break;
	}
    }
    if (PICT_FORMAT_A(pSrcPicture->format) != 0) {
	texctl |= MGA_TAKEY;
    } else {
	texctl |= MGA_TAMASK | MGA_TAKEY;
    }

    if (tmu == 1)
	texctl2 |= MGA_TC2_DUALTEX | MGA_TC2_SELECT_TMU1;
  
    mgaWaitAvail (6);
    MGA_OUT32 (mmio, MGA_REG_TEXCTL2, texctl2);
    MGA_OUT32 (mmio, MGA_REG_TEXCTL, texctl);  
    /* Source (texture) address + pitch */
    MGA_OUT32 (mmio, MGA_REG_TEXORG, ((int)pSrc->devPrivate.ptr - mem_base));
    MGA_OUT32 (mmio, MGA_REG_TEXWIDTH, (w-1)<<18 | ((8-w_log2)&63)<<9 | w_log2);
    MGA_OUT32 (mmio, MGA_REG_TEXHEIGHT, (h-1)<<18 | ((8-h_log2)&63)<<9 | h_log2);
    /* Disable filtering since we aren't doing stretch blit */
    MGA_OUT32 (mmio, MGA_REG_TEXFILTER, (0x10<<21) | MGA_MAG_NRST | MGA_MIN_NRST);
  
    if (tmu == 1) {
	mgaWaitAvail (1);
	MGA_OUT32 (mmio, MGA_REG_TEXCTL2, MGA_G400_TC2_MAGIC | MGA_TC2_DUALTEX);
    }
    
    return TRUE;
}

Bool
mgaCheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
    if (op >= sizeof(mgaBlendOP) / sizeof(mgaBlendOP[0]))
	MGA_FALLBACK(("unsupported op %x", op));
    if (!mgaCheckSourceTexture (0, pSrcPicture))
	return FALSE;
    if (pSrcPicture->transform != NULL)
	    MGA_FALLBACK(("Transformed src unsupported"));
    if (pMaskPicture != NULL) {
	if (PICT_FORMAT_A(pMaskPicture->format) == 0)
	    MGA_FALLBACK(("Mask without alpha unsupported"));
	if (!mgaCheckSourceTexture (1, pMaskPicture))
	    return FALSE;
	if (pMaskPicture->transform != NULL)
	    MGA_FALLBACK(("Transformed mask unsupported"));
    }

    if (pMaskPicture->componentAlpha)
	MGA_FALLBACK(("Component alpha unsupported"));

    switch (pDstPicture->format) {
    case PICT_a8:
	MGA_FALLBACK(("render to A8 unsupported"));
    }

    return TRUE;
}

#define C_ARG1_CUR		0x0	
#define C_ARG2_DIFFUSE		MGA_TDS_COLOR_ARG2_DIFFUSE
#define C_ARG2_PREV		MGA_TDS_COLOR_ARG2_PREVSTAGE
#define COLOR_MUL		MGA_TDS_COLOR_SEL_MUL
#define COLOR_ARG1		MGA_TDS_COLOR_SEL_ARG1
#define COLOR_ARG2		MGA_TDS_COLOR_SEL_ARG2
#define A_ARG1_CUR		0x0
#define A_ARG2_IGN		A_ARG2_DIFFUSE
#define A_ARG2_DIFFUSE		MGA_TDS_ALPHA_ARG2_DIFFUSE
#define A_ARG2_PREV		MGA_TDS_ALPHA_ARG2_PREVSTAGE
#define ALPHA_MUL		MGA_TDS_ALPHA_SEL_MUL
#define ALPHA_ARG1		MGA_TDS_ALPHA_SEL_ARG1
#define ALPHA_ARG2		MGA_TDS_ALPHA_SEL_ARG2

Bool
mgaPrepareComposite (int		op,
		     PicturePtr		pSrcPicture,
		     PicturePtr		pMaskPicture,
		     PicturePtr		pDstPicture,
		     PixmapPtr		pSrc,
		     PixmapPtr		pMask,
		     PixmapPtr		pDst)
{
    KdScreenPriv (pSrc->drawable.pScreen);  
    int mem_base=(int)pScreenPriv->screen->memory_base;
    int cmd, blendcntl;

    mgaWaitIdle();
    /* Init MGA (clipping) */
    mgaSetup (pSrc->drawable.pScreen, pDst->drawable.bitsPerPixel, 5);
    MGA_OUT32 (mmio, MGA_REG_TMR1, 0); 
    MGA_OUT32 (mmio, MGA_REG_TMR2, 0);
    MGA_OUT32 (mmio, MGA_REG_TMR3, 0);
    MGA_OUT32 (mmio, MGA_REG_TMR4, 0); 
    MGA_OUT32 (mmio, MGA_REG_TMR5, 0); 
    MGA_OUT32 (mmio, MGA_REG_TMR6, 0);
    MGA_OUT32 (mmio, MGA_REG_TMR7, 0);
    MGA_OUT32 (mmio, MGA_REG_TMR8, 0x10000); 

    /* Initialize colors to 0, used in the src = A8 case */
    mgaWaitAvail(9);
    MGA_OUT32 (mmio, MGA_REG_DR4, 0); 
    MGA_OUT32 (mmio, MGA_REG_DR6, 0);
    MGA_OUT32 (mmio, MGA_REG_DR7, 0);
    MGA_OUT32 (mmio, MGA_REG_DR8, 0); 
    MGA_OUT32 (mmio, MGA_REG_DR10, 0); 
    MGA_OUT32 (mmio, MGA_REG_DR11, 0);
    MGA_OUT32 (mmio, MGA_REG_DR12, 0);
    MGA_OUT32 (mmio, MGA_REG_DR14, 0); 
    MGA_OUT32 (mmio, MGA_REG_DR15, 0);

    /* Destination flags */
    mgaWaitAvail (2);
    MGA_OUT32 (mmio, MGA_REG_DSTORG, ((int)pDst->devPrivate.ptr - mem_base));
    MGA_OUT32 (mmio, MGA_REG_PITCH,
	       pDst->devKind / (pDst->drawable.bitsPerPixel >> 3));
    
    /* Source(s) flags */
    if (!PrepareSourceTexture (0, pSrcPicture, pSrc)) return FALSE;
    if (pMask != NULL)
	if (!PrepareSourceTexture (1, pMaskPicture, pMask)) return FALSE;

    /* MultiTexture modulation */
    mgaWaitAvail (2);
    if (pSrcPicture->format == PICT_a8) {
	/* C = 0	A = As */
	MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE0,
		   C_ARG2_DIFFUSE | COLOR_ARG2 |
		   A_ARG1_CUR | ALPHA_ARG1);
    } else {
	/* C = Cs	A = As */
	MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE0,
		   C_ARG1_CUR | COLOR_ARG1 |
		   A_ARG1_CUR | ALPHA_ARG1);
    }
    if (pMask != NULL) {
	if (PICT_FORMAT_A (pSrcPicture->format) == 0) {
	    MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE1, 
		       C_ARG1_CUR | C_ARG2_PREV | COLOR_MUL |
		       A_ARG1_CUR | A_ARG2_IGN | ALPHA_ARG1 |
		       MGA_TDS_COLOR_ARG1_REPLICATEALPHA);
	} else {
	    MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE1, 
		       C_ARG1_CUR | C_ARG2_PREV | COLOR_MUL |
		       A_ARG1_CUR | A_ARG2_PREV | ALPHA_MUL |
		       MGA_TDS_COLOR_ARG1_REPLICATEALPHA);
	}
    } else {
	MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE1, 0);    
    }
  
    cmd = MGA_OPCOD_TEXTURE_TRAP | MGA_ATYPE_RSTR | 0x000c0000 |
	MGA_DWGCTL_SHIFTZERO | MGA_DWGCTL_SGNZERO | MGA_DWGCTL_ARZERO |
	MGA_ATYPE_I;
    
    blendcntl = mgaBlendOP[op].blend_cntl;
    if (PICT_FORMAT_A(pDstPicture->format) == 0 && mgaBlendOP[op].dst_alpha) {
	if ((blendcntl & MGA_SRC_BLEND_MASK) == MGA_SRC_DST_ALPHA)
	    blendcntl = (blendcntl & ~MGA_SRC_BLEND_MASK) | MGA_SRC_ONE;
	else if ((blendcntl & MGA_SRC_BLEND_MASK) == MGA_SRC_ONE_MINUS_DST_ALPHA)
	    blendcntl = (blendcntl & ~MGA_SRC_BLEND_MASK) | MGA_SRC_ZERO;
    }

    mgaWaitAvail (2);
    MGA_OUT32 (mmio, MGA_REG_DWGCTL, cmd);
    MGA_OUT32 (mmio, MGA_REG_ALPHACTRL, MGA_ALPHACHANNEL | blendcntl);
    
    currentSrc=pSrc;
    currentMask=pMask;

    return TRUE;
}

void
mgaComposite (int	srcX,
	      int	srcY,
	      int	maskX,
	      int	maskY,
	      int	dstX,
	      int	dstY,
	      int	width,
	      int	height)
{  
    int src_w2 = MGA_LOG2 (currentSrc->drawable.width);
    int src_h2 = MGA_LOG2 (currentSrc->drawable.height);

    /* Source positions can be outside source textures' boundaries.
     * We clamp the values here to avoid rendering glitches.
     */
    srcX=srcX % currentSrc->drawable.width;
    srcY=srcY % currentSrc->drawable.height;
    maskX=maskX % currentMask->drawable.width;
    maskY=maskY % currentMask->drawable.height;
  
    mgaWaitAvail (4);
    /* Start position in the texture */
    MGA_OUT32 (mmio, MGA_REG_TMR6, srcX<<(20-src_w2));
    MGA_OUT32 (mmio, MGA_REG_TMR7, srcY<<(20-src_h2));
    /* Increments (1 since we aren't doing stretch blit) */
    MGA_OUT32 (mmio, MGA_REG_TMR0, 1<<(20-src_w2));
    MGA_OUT32 (mmio, MGA_REG_TMR3, 1<<(20-src_h2));

    if (currentMask != NULL) {
	int mask_w2 = MGA_LOG2 (currentMask->drawable.width);
	int mask_h2 = MGA_LOG2 (currentMask->drawable.height);

	mgaWaitAvail (6);
	MGA_OUT32  (mmio, MGA_REG_TEXCTL2,
		    MGA_G400_TC2_MAGIC | MGA_TC2_DUALTEX | MGA_TC2_SELECT_TMU1);

	MGA_OUT32 (mmio, MGA_REG_TMR6, maskX<<(20-mask_w2));
	MGA_OUT32 (mmio, MGA_REG_TMR7, maskY<<(20-mask_h2));
	MGA_OUT32 (mmio, MGA_REG_TMR0, 1<<(20-mask_w2));
	MGA_OUT32 (mmio, MGA_REG_TMR3, 1<<(20-mask_h2));

	MGA_OUT32 (mmio, MGA_REG_TEXCTL2, MGA_G400_TC2_MAGIC | MGA_TC2_DUALTEX);
    }  
  
    /* Destination Bounding Box
     * (Boundary Right | Boundary Left, Y dest | Height)
     */
    mgaWaitAvail (2);
    MGA_OUT32 (mmio, MGA_REG_FXBNDRY,
	       ((dstX + width) << 16) | (dstX & 0xffff));
    MGA_OUT32 (mmio, MGA_REG_YDSTLEN | MGA_REG_EXEC,
	       (dstY << 16) | (height & 0xffff));
}

void
mgaDoneComposite (void)
{

}
