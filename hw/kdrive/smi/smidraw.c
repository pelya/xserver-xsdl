/*
 * Id: smidraw.c,v 1.1 1999/11/02 03:54:47 keithp Exp $
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
/* $XFree86: xc/programs/Xserver/hw/kdrive/smi/smidraw.c,v 1.7 2001/07/24 19:06:03 keithp Exp $ */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "smi.h"
#include "smidraw.h"

#include	<X11/Xmd.h>
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
#include	"picturestr.h"

CARD8 smiBltRop[16] = {
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

CARD8 smiSolidRop[16] = {
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


#define GET_STATUS(smic) smiGetIndex (smic, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x16)

#define ENGINE_IDLE_EMPTY(smic) ((GET_STATUS(smic) & 0x18) == 0x10)
#define FIFO_EMPTY(smic)	((GET_STATUS(smic) & 0x10) == 0x10)

#define MAX_FIFO    16

void
smiWaitAvail(SmiCardInfo *smic, int n)
{
    if (smic->avail < n)
    {
	while (!FIFO_EMPTY (smic))
	    ;
	smic->avail = MAX_FIFO;
    }
    smic->avail -= n;
}

void
smiWaitIdle (SmiCardInfo *smic)
{
    while (!ENGINE_IDLE_EMPTY (smic))
	;
    smic->avail = MAX_FIFO;
}

static SmiCardInfo	*smic;
static SmiScreenInfo	*smis;
static DPR		*dpr;
static CARD32		accel_cmd;

static Bool
smiSetup (ScreenPtr pScreen, int wait)
{
    KdScreenPriv(pScreen);

    smis = getSmiScreenInfo (pScreenPriv);
    smic = getSmiCardInfo(pScreenPriv);
    dpr = smic->dpr;
    
    if (!dpr)
	return FALSE;
    
    /* enable DPR/VPR registers */
    smiSetIndex (smic, VGA_SEQ_INDEX, VGA_SEQ_DATA, 0x21, 
		 smis->dpr_vpr_enable);
    
    smiWaitAvail (smic, wait + 9);
    dpr->src_stride = (smis->stride << 16) | smis->stride;
    dpr->data_format = smis->data_format;
    dpr->mask1 = 0xffffffff;
    dpr->mask2 = 0xffffffff;
    dpr->dst_stride = (smis->stride << 16) | smis->stride;
    dpr->unknown_40 = 0x0;
    dpr->unknown_44 = 0x0;
    dpr->scissors_ul = 0x0;
    dpr->scissors_lr = SMI_XY(4095,4095);

    return TRUE;
}

Bool
smiPrepareSolid (DrawablePtr    pDrawable,
		 int		alu,
		 Pixel		pm,
		 Pixel		fg)
{
    KdScreenPriv(pDrawable->pScreen);

    if (~pm & FbFullMask(pDrawable->depth))
	return FALSE;
    
    if (!smiSetup (pDrawable->pScreen, 3))
	return FALSE;
    
    accel_cmd = smiSolidRop[alu] | SMI_BITBLT | SMI_START_ENGINE;
    dpr->fg = fg;
    dpr->mask3 = 0xffffffff;
    dpr->mask4 = 0xffffffff;
    return TRUE;
}

void
smiSolid (int x1, int y1, int x2, int y2)
{
    smiWaitAvail(smic,3);
    dpr->dst_xy = SMI_XY(x1,y1);
    dpr->dst_wh = SMI_XY(x2-x1,y2-y1);
    dpr->accel_cmd = accel_cmd; 
}

void
smiDoneSolid (void)
{
}

static int copyDx;
static int copyDy;

Bool
smiPrepareCopy (DrawablePtr	pSrcDrawable,
		   DrawablePtr	pDstDrawable,
		   int		dx,
		   int		dy,
		   int		alu,
		   Pixel	pm)
{
    KdScreenPriv(pSrcDrawable->pScreen);

    if (~pm & FbFullMask(pSrcDrawable->depth))
	return FALSE;
    
    if (!smiSetup (pSrcDrawable->pScreen, 0))
	return FALSE;
    
    accel_cmd = smiBltRop[alu] | SMI_BITBLT | SMI_START_ENGINE;
    
    copyDx = dx;
    copyDy = dy;
    if (dy < 0 || (dy == 0 && dx < 0))
	accel_cmd |= SMI_RIGHT_TO_LEFT;
    return TRUE;
}

void
smiCopy (int srcX,
	    int srcY,
	    int dstX,
	    int dstY,
	    int w,
	    int h)
{
    if (accel_cmd & SMI_RIGHT_TO_LEFT)
    {
	srcX += w - 1;
	dstX += w - 1;
	srcY += h - 1;
	dstY += h - 1;
    }
    smiWaitAvail (smic, 4);
    dpr->src_xy = SMI_XY (srcX, srcY);
    dpr->dst_xy = SMI_XY (dstX, dstY);
    dpr->dst_wh = SMI_XY (w, h);
    dpr->accel_cmd = accel_cmd;
}

void
smiDoneCopy (void)
{
}

KaaScreenPrivRec    smiKaa = {
    smiPrepareSolid,
    smiSolid,
    smiDoneSolid,

    smiPrepareCopy,
    smiCopy,
    smiDoneCopy,
};

Bool
smiDrawInit (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    smiCardInfo (pScreenPriv);
    smiScreenInfo (pScreenPriv);
    
    ENTER ();
    if (pScreenPriv->screen->fb[0].depth == 4)
    {
	LEAVE ();
	return FALSE;
    }
    
    if (!smic->dpr)
    {
	LEAVE ();
	return FALSE;
    }

    if (!kaaDrawInit (pScreen, &smiKaa))
    {
	LEAVE ();
	return FALSE;
    }

    LEAVE ();
    return TRUE;
}

void
smiDrawEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    int i;
    static const int xyAddress[] = { 320, 400, 512, 640, 800, 1024, 1280, 1600, 2048 };
    
    ENTER ();
    smis = getSmiScreenInfo (pScreenPriv);
    smic = getSmiCardInfo(pScreenPriv);
    dpr = smic->dpr;
    
    iopl(3);
    
    smis->stride = pScreenPriv->screen->fb[0].byteStride;
    smis->dpr_vpr_enable = smiGetIndex (smic, VGA_SEQ_INDEX, 
					VGA_SEQ_DATA, 0x21) & ~0x03;
    
    switch (pScreenPriv->screen->fb[0].depth) {
    case 8:
	smis->data_format = 0x00000000;
	break;
    case 15:
    case 16:
	smis->data_format = 0x00100000;
	smis->stride >>= 1;
	break;
    case 24:
	smis->data_format = 0x00300000;
	break;
    case 32:
	smis->data_format = 0x00200000;
	smis->stride >>= 2;
	break;
    }
    for (i = 0; i < sizeof(xyAddress) / sizeof(xyAddress[0]); i++)
    {
	if (xyAddress[i] == pScreenPriv->screen->fb[0].pixelStride)
	{
	    smis->data_format |= i << 16;
	    break;
	}
    }
    
    KdMarkSync (pScreen);
    LEAVE ();
}

void
smiDrawDisable (ScreenPtr pScreen)
{
    ENTER ();
    smic = 0;
    smis = 0;
    dpr = 0;
    accel_cmd = 0;
    iopl (0);
    LEAVE ();
}

void
smiDrawFini (ScreenPtr pScreen)
{
    ENTER ();
    LEAVE ();
}

void
smiDrawSync (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    smic = getSmiCardInfo(pScreenPriv);
    
    smiWaitIdle (smic);
}
