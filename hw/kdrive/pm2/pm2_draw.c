#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "kdrive.h"

#include "pm2.h"

static PM2CardInfo	*card;
static VOL8	*mmio;

static void pmWait (PM2CardInfo *card, int n);

static void
pmWait (PM2CardInfo *card, int n)
{
    if (card->in_fifo_space >= n)
	card->in_fifo_space -= n;
    else {
        int tmp;
        while((tmp = *(VOL32 *) (mmio + InFIFOSpace)) < n);
        /* Clamp value due to bugs in PM3 */
        if (tmp > card->fifo_size)
    	    tmp = card->fifo_size;
        card->in_fifo_space = tmp - n;
    }
}

static Bool
pmPrepareSolid (PixmapPtr   	pPixmap,
		int		alu,
		Pixel		pm,
		Pixel		fg)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    KdScreenPriv(pScreen);
    pmCardInfo(pScreenPriv);

    card = pm2c;
    if (~pm & FbFullMask(pPixmap->drawable.depth))
	return FALSE;

    pmWait(card, 6);
    *(VOL32 *) (mmio + FBHardwareWriteMask) = pm;
    if (alu == GXcopy) {
	*(VOL32 *) (mmio + ColorDDAMode) = UNIT_DISABLE;
  	*(VOL32 *) (mmio + FBReadMode) = card->pprod;
  	*(VOL32 *) (mmio + FBBlockColor) = fg;
    } else {
	*(VOL32 *) (mmio + ColorDDAMode) = UNIT_ENABLE;
	*(VOL32 *) (mmio + ConstantColor) = fg;
  	*(VOL32 *) (mmio + FBReadMode) = card->pprod|FBRM_DstEnable|FBRM_Packed;
    }
    *(VOL32 *) (mmio + LogicalOpMode) = alu<<1|UNIT_ENABLE;
    card->ROP = alu;

    return TRUE;
}

static void
pmSolid (int x1, int y1, int x2, int y2)
{
    int speed = 0;

    if (card->ROP == GXcopy) {
	pmWait(card, 3);
	*(VOL32 *) (mmio + RectangleOrigin) = GLINT_XY(x1, y1);
	*(VOL32 *) (mmio + RectangleSize) = GLINT_XY(x2-x1, y2-y1);
  	speed = FastFillEnable;
    } else {
	pmWait(card, 4);
	*(VOL32 *) (mmio + RectangleOrigin) = GLINT_XY(x1, y1);
	*(VOL32 *) (mmio + RectangleSize) = GLINT_XY((x2-x1)+7, y2-y1);
	*(VOL32 *) (mmio + PackedDataLimits) = x1<<16|(x1+(x2-x1));
  	speed = 0;
    }
    *(VOL32 *) (mmio + Render) = PrimitiveRectangle | XPositive | YPositive | speed;
}


static void
pmDoneSolid (void)
{
}

static Bool
pmPrepareCopy (PixmapPtr	pSrcPixmap,
	       PixmapPtr	pDstPixmap,
	       int		dx,
	       int		dy,
	       int		alu,
	       Pixel		pm)
{
    ScreenPtr pScreen = pDstPixmap->drawable.pScreen;
    KdScreenPriv(pScreen);
    pmCardInfo(pScreenPriv);

    ErrorF ("pmPrepareCopy\n");
    
    card = pm2c;
    if (~pm & FbFullMask(pDstPixmap->drawable.depth))
	return FALSE;

    pmWait(card, 5);
    *(VOL32 *) (mmio + FBHardwareWriteMask) = pm;
    *(VOL32 *) (mmio + ColorDDAMode) = UNIT_DISABLE;
    if ((alu == GXset) || (alu == GXclear)) {
  	*(VOL32 *) (mmio + FBReadMode) = card->pprod;
    } else {
    	if ((alu == GXcopy) || (alu == GXcopyInverted)) {
  	    *(VOL32 *) (mmio + FBReadMode) = card->pprod|FBRM_SrcEnable;
        } else {
  	    *(VOL32 *) (mmio + FBReadMode) = card->pprod|FBRM_SrcEnable|FBRM_DstEnable;
        }
    }
    *(VOL32 *) (mmio + LogicalOpMode) = alu<<1|UNIT_ENABLE;
    card->ROP = alu;

    return TRUE;
}

static void
pmCopy (int srcX,
        int srcY,
        int dstX,
        int dstY,
        int w,
        int h)
{
    ErrorF ("pmCopy %d %d %d %d %d %d\n", srcX, srcY, dstX, dstY, w, h);
    pmWait(card, 4);
    *(VOL32 *) (mmio + RectangleOrigin) = GLINT_XY(dstX, dstY);
    *(VOL32 *) (mmio + RectangleSize) = GLINT_XY(w, h);
    *(VOL32 *) (mmio + FBSourceDelta) = ((srcY-dstY)&0x0FFF)<<16 | (srcX-dstX)&0x0FFF;
    *(VOL32 *) (mmio + Render) = PrimitiveRectangle | XPositive | YPositive;
}

static void
pmDoneCopy (void)
{
}

KaaScreenInfoRec    pmKaa = {
    pmPrepareSolid,
    pmSolid,
    pmDoneSolid,

    pmPrepareCopy,
    pmCopy,
    pmDoneCopy,
};

Bool
pmDrawInit (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    pmCardInfo(pScreenPriv);
    Bool    ret = TRUE;

    card = pm2c;
    mmio = pm2c->reg_base;

    if (pScreenPriv->screen->fb[0].depth <= 16) {
	ErrorF ("depth(%d) <= 16 \n", pScreenPriv->screen->fb[0].depth);
	ret = FALSE;
    }    
    if (ret && !kaaDrawInit (pScreen, &pmKaa))
    {
	ErrorF ("kaaDrawInit failed\n");
	ret = FALSE;
    }

    return ret;
}

void
pmDrawEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    pmCardInfo(pScreenPriv);
    
    pm2c->in_fifo_space = 0;

    KdMarkSync (pScreen);
}

void
pmDrawDisable (ScreenPtr pScreen)
{
}

void
pmDrawFini (ScreenPtr pScreen)
{
}

void
pmDrawSync (ScreenPtr pScreen)
{
    if (card->clipping_on) {
	card->clipping_on = FALSE;
	pmWait(card, 1);
	*(VOL32 *) (mmio + ScissorMode) = 0;
    }

    while (*(VOL32 *) (mmio + DMACount) != 0);

    pmWait(card, 2);

    *(VOL32 *) (mmio + FilterMode) = 0x400;
    *(VOL32 *) (mmio + GlintSync) = 0;

    do {
   	while(*(VOL32 *) (mmio + OutFIFOWords) == 0);
    } while (*(VOL32 *) (mmio + OutputFIFO) != Sync_tag);
}
