/*
 *
 * Copyright © 2004 Franco Catrin
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Franco Catrin not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Franco Catrin makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * FRANCO CATRIN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL FRANCO CATRIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "neomagic.h"

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

static inline void neoWaitIdle( NeoCardInfo *neoc)
{
	// if MMIO is not working it may halt the machine
    int i = 0;
    while ((neoc->mmio->bltStat & 1) && ++i<10000);
    if (i>=10000) DBGOUT("Wait Idle timeout");
}

static inline void neoWaitFifo(NeoCardInfo *neoc,
                                     int requested_fifo_space )
{
  neoWaitIdle( neoc );
}

NeoMMIO           *mmio;
NeoScreenInfo *screen;
NeoCardInfo *card;
CARD32	fgColor;

static Bool neoPrepareSolid(PixmapPtr    pPixmap,
		     int	    alu,
		     Pixel	    pm,
		     Pixel	    fg)
{
	FbBits  depthMask = FbFullMask(pPixmap->drawable.depth);
	
	if ((pm & depthMask) != depthMask)
	{
		return FALSE;
	}
	else
	{
		fgColor = fg;
		neoWaitIdle(card);
		/* set blt control */
		mmio->bltCntl = 
			NEO_BC0_SRC_IS_FG    |
			NEO_BC3_SRC_XY_ADDR |
			NEO_BC3_DST_XY_ADDR |
			NEO_BC3_SKIP_MAPPING |  0x0c0000;
		
		mmio->fgColor = fgColor;
		return TRUE;
	}
}

void
neoSolid (int x1, int y1, int x2, int y2)
{
	DBGOUT("Solid (%i, %i) - (%i, %i).  \n", x1, y1, x2, y2);		
	int x, y, w, h;
	x = x1;
	y = y1;
	w = x2-x1 + 1;
	h = y2-y1 + 1;
	if (x1>x2)
	{
		x = x2;
		w = -w;
	}
	if (y1>y2)
	{
		y = y2;
		h = -h;
	}
	
	neoWaitIdle(card);
	mmio->dstStart = (y <<16) | (x & 0xffff);

	mmio->xyExt    = (h << 16) | (w & 0xffff);
	DBGOUT("Solid (%i, %i) - (%i, %i).  Color %x\n", x, y, w, h, fgColor);		
	DBGOUT("Offset %lx. Extent %lx\n",mmio->dstStart, mmio->xyExt);		
}


void
neoDoneSolid(void)
{
}

Bool
neoPrepareCopy (PixmapPtr	pSrcPixpam,
		    PixmapPtr	pDstPixmap,
		    int		dx,
		    int		dy,
		    int		alu,
		    Pixel	pm)
{
	return TRUE;
}

void
neoCopy (int srcX,
	     int srcY,
	     int dstX,
	     int dstY,
	     int w,
	     int h)
{
}

void
neoDoneCopy (void)
{
}

KaaScreenInfoRec    neoKaa = {
    neoPrepareSolid,
    neoSolid,
    neoDoneSolid,

    neoPrepareCopy,
    neoCopy,
    neoDoneCopy
};

Bool
neoDrawInit (ScreenPtr pScreen)
{
    ENTER();
//    SetupNeo(pScreen);
//    PictureScreenPtr    ps = GetPictureScreen(pScreen);
    
    if (!kaaDrawInit (pScreen, &neoKaa))
	return FALSE;

//    if (ps && tridents->off_screen)
//	ps->Composite = tridentComposite;
    LEAVE();
    return TRUE;
}

void
neoDrawEnable (ScreenPtr pScreen)
{
    ENTER();
    SetupNeo(pScreen);
    screen = neos;
    card = neoc;
    mmio = neoc->mmio;
    DBGOUT("NEO AA MMIO=%lx\n", mmio);
    screen->depth = screen->vesa.mode.BitsPerPixel/8;
    screen->pitch = screen->vesa.mode.BytesPerScanLine;
    DBGOUT("NEO depth=%x, pitch=%x\n", screen->depth, screen->pitch);
    LEAVE();
}

void
neoDrawDisable (ScreenPtr pScreen)
{
    ENTER();
    LEAVE();
}

void
neoDrawFini (ScreenPtr pScreen)
{
    ENTER();
    LEAVE();
}

void
neoDrawSync (ScreenPtr pScreen)
{
    ENTER();
    SetupNeo(pScreen);
    
    neoWaitIdle(neoc);
    LEAVE();
}
