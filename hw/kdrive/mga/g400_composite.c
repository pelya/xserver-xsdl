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

static CARD32 mgaBlendOP[14] = {
    /* Clear */
    MGA_SRC_ZERO	        | MGA_DST_ZERO,
    /* Src */
    MGA_SRC_ONE			| MGA_DST_ZERO,
    /* Dst */
    MGA_SRC_ZERO	        | MGA_DST_ONE,
    /* Over */
    MGA_SRC_ALPHA               | MGA_DST_ONE_MINUS_SRC_ALPHA,
    /* OverReverse */
    MGA_SRC_ONE_MINUS_DST_ALPHA | MGA_DST_ONE,
    /* In */
    MGA_SRC_DST_ALPHA           | MGA_DST_ZERO,
    /* InReverse */
    MGA_SRC_ZERO	        | MGA_DST_SRC_ALPHA,
    /* Out */
    MGA_SRC_ONE_MINUS_DST_ALPHA | MGA_DST_ZERO,
    /* OutReverse */
    MGA_SRC_ZERO	        | MGA_DST_ONE_MINUS_SRC_ALPHA,
    /* Atop */
    MGA_SRC_DST_ALPHA	        | MGA_DST_ONE_MINUS_SRC_ALPHA,
    /* AtopReverse */
    MGA_SRC_ONE_MINUS_DST_ALPHA | MGA_DST_SRC_ALPHA,
    /* Xor */
    MGA_SRC_ONE_MINUS_DST_ALPHA | MGA_DST_ONE_MINUS_SRC_ALPHA,
    /* Add */
    MGA_SRC_ONE                 | MGA_DST_ONE,
    /* Saturate */
    MGA_SRC_SRC_ALPHA_SATURATE  | MGA_DST_ONE
};


static int MGA_LOG2( int val )
{
    int ret = 0;
    
    while (val >> ret)
	ret++;
    
    return ((1 << (ret-1)) == val) ? (ret-1) : ret;
}


Bool
mgaPrepareBlend (int		op,
		 PicturePtr	pSrcPicture,
		 PicturePtr	pDstPicture,
		 PixmapPtr	pSrc,
		 PixmapPtr	pDst)
{
    return mgaPrepareComposite (op, pSrcPicture, NULL, pDstPicture,
				pSrc, NULL, pDst);
}

void
mgaBlend (int	srcX,
	  int	srcY,
	  int	dstX,
	  int	dstY,
	  int	width,
	  int	height)
{
    mgaComposite (srcX, srcY, 0, 0, dstX, dstY, width, height);
}

void
mgaDoneBlend (void)
{
    mgaDoneComposite ();
}



static Bool
PrepareSourceTexture (int		tmu,
		      PicturePtr 	pSrcPicture,
		      PixmapPtr 	pSrc)
{
    KdScreenPriv (pSrc->drawable.pScreen);  
    int mem_base=(int)pScreenPriv->screen->memory_base;
    int pitch = pSrc->devKind / (pSrc->drawable.bitsPerPixel >> 3); 

    int w = pSrc->drawable.width;
    int h = pSrc->drawable.height;
    int w_log2 = MGA_LOG2(w);
    int h_log2 = MGA_LOG2(h);
    
    int texctl=MGA_PITCHEXT | ((pitch&0x7ff)<<9) | 
	MGA_TAKEY | MGA_CLAMPUV | MGA_NOPERSPECTIVE;  
    int texctl2=MGA_G400_TC2_MAGIC;

    if ((w > 2047) || (h > 2047))
	MGA_FALLBACK(("Picture too large for composition (%dx%d)\n", w, h));

    switch (pSrcPicture->format) {
    case PICT_a8r8g8b8:
    case PICT_x8r8g8b8:
    case PICT_a8b8g8r8:
    case PICT_x8b8g8r8:
	texctl |= MGA_TW32;
	break;
    case PICT_r5g6b5:
    case PICT_b5g6r5:
	texctl |= MGA_TW16;
	break;
    case PICT_a8:
	texctl |= MGA_TW8A;
	break;
    default:
	MGA_FALLBACK(("unsupported Picture format for composition (%x)\n",
		      pSrcPicture->format));
    }
    
    if (tmu == 1) texctl2 |= MGA_TC2_DUALTEX | MGA_TC2_SELECT_TMU1;
  
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
    int cmd;

    /* sometimes mgaPrepareComposite is given the same pointer for Src
     * and Mask. PSrcPicture is an unsupported format (x8b8g8r8) whereas
     * pMaskPicture is supported (a8r8g8b8). We Choose to render a
     * simple blend and using pMaskPicture as the Source.
     */
    if (pMask == pSrc) {
	pMask=NULL;
	pMaskPicture=NULL;
    }

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
    MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE0, MGA_TDS_COLOR_SEL_ARG1);
    if (pMask != NULL) {
	if (PICT_FORMAT_A (pSrcPicture->format) == 0) {
	    MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE1, 
		       MGA_TDS_COLOR_ARG2_PREVSTAGE | MGA_TDS_COLOR_SEL_ARG2 |
		       MGA_TDS_ALPHA_SEL_ARG1);
	} else {
	    MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE1, 
		       MGA_TDS_COLOR_ARG2_PREVSTAGE | MGA_TDS_COLOR_SEL_ARG2 |
		       MGA_TDS_ALPHA_ARG2_PREVSTAGE | MGA_TDS_ALPHA_SEL_MUL);
	}
    } else {
	MGA_OUT32 (mmio, MGA_REG_TDUALSTAGE1, 0);    
    }
  
    cmd = MGA_OPCOD_TEXTURE_TRAP | MGA_ATYPE_RSTR | 0x000c0000 |
	MGA_DWGCTL_SHIFTZERO | MGA_DWGCTL_SGNZERO | MGA_DWGCTL_ARZERO |
	MGA_ATYPE_I;
    
    mgaWaitAvail (2);
    MGA_OUT32 (mmio, MGA_REG_DWGCTL, cmd);
    MGA_OUT32 (mmio, MGA_REG_ALPHACTRL, MGA_ALPHACHANNEL | mgaBlendOP[op]);
    
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
