/*
 * $Id$
 *
 * Copyright © 2003 Anders Carlsson
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
 * ANDERS CARLSSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
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

CARD32 mgaRop[16] = {
    /* GXclear        */  MGA_ATYPE_RPL  | 0x00000000,	/* 0 */
    /* GXand          */  MGA_ATYPE_RSTR | 0x00080000,	/* src AND dst */
    /* GXandReverse   */  MGA_ATYPE_RSTR | 0x00040000,	/* src AND NOT dst */
    /* GXcopy         */  MGA_ATYPE_RSTR | 0x000c0000,	/* src */
    /* GXandInverted  */  MGA_ATYPE_RSTR | 0x00020000,	/* NOT src AND dst */
    /* GXnoop         */  MGA_ATYPE_RSTR | 0x000a0000,	/* dst */
    /* GXxor          */  MGA_ATYPE_RSTR | 0x00060000,	/* src XOR dst */
    /* GXor           */  MGA_ATYPE_RSTR | 0x000e0000,	/* src OR dst */
    /* GXnor          */  MGA_ATYPE_RSTR | 0x00010000,	/* NOT src AND NOT dst */
    /* GXequiv        */  MGA_ATYPE_RSTR | 0x00090000,	/* NOT src XOR dst */
    /* GXinvert       */  MGA_ATYPE_RSTR | 0x00050000,	/* NOT dst */
    /* GXorReverse    */  MGA_ATYPE_RSTR | 0x000d0000,	/* src OR NOT dst */
    /* GXcopyInverted */  MGA_ATYPE_RPL  | 0x00030000,	/* NOT src */
    /* GXorInverted   */  MGA_ATYPE_RSTR | 0x000b0000,	/* NOT src OR dst */
    /* GXnand         */  MGA_ATYPE_RSTR | 0x00070000,	/* NOT src OR NOT dst */
    /* GXset          */  MGA_ATYPE_RPL  | 0x000f0000	/* 1 */
};

static VOL8 *mmio;
int fifo_size;
int pitch, src_pitch;
int dir;

void
mgaWaitAvail (int n)
{
    if (fifo_size < n) {
      while ((fifo_size = MGA_IN32 (mmio, MGA_REG_FIFOSTATUS) & 0xff) < n)
	;
    }
    
    fifo_size -= n;
}

void
mgaWaitIdle (void)
{
    while (MGA_IN32 (mmio, MGA_REG_STATUS) & 0x10000);
}

static Bool
mgaSetup (ScreenPtr pScreen, int wait)
{
  KdScreenPriv (pScreen);
  mgaScreenInfo (pScreenPriv);
  mgaCardInfo (pScreenPriv);

  fifo_size = 0;
  mmio = mgac->reg_base;
  pitch = mgas->pitch;
  
  if (!mmio)
    return FALSE;

  mgaWaitAvail (wait + 4);
  MGA_OUT32 (mmio, MGA_REG_MACCESS, mgas->pw);
  MGA_OUT32 (mmio, MGA_REG_CXBNDRY, 0xffff0000);
  MGA_OUT32 (mmio, MGA_REG_YTOP, 0x00000000);
  MGA_OUT32 (mmio, MGA_REG_YBOT, 0x007fffff);

  return TRUE;
}

Bool
mgaPrepareSolid (PixmapPtr pPixmap, int alu, Pixel pm, Pixel fg)
{
    KdScreenPriv(pPixmap->drawable.pScreen);
    mgaScreenInfo (pScreenPriv);
    int cmd;
    int dst_org;

    cmd = MGA_OPCOD_TRAP | MGA_DWGCTL_SOLID | MGA_DWGCTL_ARZERO | MGA_DWGCTL_SGNZERO |
	MGA_DWGCTL_SHIFTZERO | mgaRop[alu];

    dst_org = (int)pPixmap->devPrivate.ptr - (int)pScreenPriv->screen->memory_base;
    
    mgaSetup (pPixmap->drawable.pScreen, 5);

    MGA_OUT32 (mmio, MGA_REG_DSTORG, dst_org);
    MGA_OUT32 (mmio, MGA_REG_PITCH, pPixmap->devKind / (pPixmap->drawable.bitsPerPixel >> 3));
    MGA_OUT32 (mmio, MGA_REG_DWGCTL, cmd);
    MGA_OUT32 (mmio, MGA_REG_FCOL, fg);
    MGA_OUT32 (mmio, MGA_REG_PLNWT, pm);
    
    return TRUE;
}

void
mgaSolid (int x1, int y1, int x2, int y2)
{
    mgaWaitAvail (2);

    MGA_OUT32 (mmio, MGA_REG_FXBNDRY, (x2 << 16) | (x1 & 0xffff));
    MGA_OUT32 (mmio, MGA_REG_YDSTLEN | MGA_REG_EXEC, (y1 << 16) | (y2 - y1));
}

void
mgaDoneSolid (void)
{
}

#define BLIT_LEFT	1
#define BLIT_UP		4

Bool
mgaPrepareCopy (PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap,
		int dx, int dy, int alu, Pixel pm)
{
    KdScreenPriv(pSrcPixmap->drawable.pScreen);
    mgaScreenInfo (pScreenPriv);

    int cmd;

    cmd = MGA_OPCOD_BITBLT | MGA_DWGCTL_BFCOL | MGA_DWGCTL_SHIFTZERO | mgaRop[alu];

    dir = 0;

    if (dy < 0)
	dir |= BLIT_UP;
    if (dx < 0)
	dir |= BLIT_LEFT;

    mgaSetup (pSrcPixmap->drawable.pScreen, 7);

    MGA_OUT32 (mmio, MGA_REG_SRCORG, ((int)pSrcPixmap->devPrivate.ptr - (int)pScreenPriv->screen->memory_base));
    MGA_OUT32 (mmio, MGA_REG_DSTORG, ((int)pDstPixmap->devPrivate.ptr - (int)pScreenPriv->screen->memory_base));
    MGA_OUT32 (mmio, MGA_REG_PITCH, pDstPixmap->devKind / (pDstPixmap->drawable.bitsPerPixel >> 3));
    src_pitch = pSrcPixmap->devKind / (pSrcPixmap->drawable.bitsPerPixel >> 3);
	
    MGA_OUT32 (mmio, MGA_REG_DWGCTL, cmd);
    MGA_OUT32 (mmio, MGA_REG_SGN, dir);
    MGA_OUT32 (mmio, MGA_REG_PLNWT, pm);
    MGA_OUT32 (mmio, MGA_REG_AR5, src_pitch * (dy < 0 ? -1 : 1) );

    return TRUE;
}

void
mgaCopy (int srcX, int srcY, int dstX, int dstY, int w, int h)
{
    int start, end;
    
    if (dir & BLIT_UP)
    {
	srcY += h - 1;
	dstY += h - 1;
    }

    w--;
    start = end = srcY * src_pitch + srcX;

    if (dir & BLIT_LEFT)
	start += w;
    else
	end += w;

    mgaWaitAvail (4);
    MGA_OUT32 (mmio, MGA_REG_AR0, end);
    MGA_OUT32 (mmio, MGA_REG_AR3, start);
    MGA_OUT32 (mmio, MGA_REG_FXBNDRY, ((dstX + w) << 16) | (dstX & 0xffff));
    MGA_OUT32 (mmio, MGA_REG_YDSTLEN | MGA_REG_EXEC, (dstY << 16) | h);
}

void
mgaDoneCopy (void)
{
}

KaaScreenInfoRec mgaKaa = {
    mgaPrepareSolid,
    mgaSolid,
    mgaDoneSolid,

    mgaPrepareCopy,
    mgaCopy,
    mgaDoneCopy,

    192, /* Offscreen byte alignment */
    64, /* Offset pitch */
    KAA_OFFSCREEN_PIXMAPS, /* Flags */
};

Bool
mgaDrawInit (ScreenPtr pScreen)
{
    if (!kaaDrawInit (pScreen, &mgaKaa))
	return FALSE;

    return TRUE;
}

void
mgaDrawEnable (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);
    mgaScreenInfo (pScreenPriv);

    mgas->pitch = pScreenPriv->screen->width;

    switch (pScreenPriv->screen->fb[0].depth) {
    case 8:
      mgas->pw = MGA_PW8;
      break;
    case 16:
      mgas->pw = MGA_PW16;
      break;
    case 24:
      mgas->pw = MGA_PW24;
      break;
    case 32:
      mgas->pw = MGA_PW32;
      break;
    default:
      FatalError ("unsupported pixel format");
    }

    KdMarkSync (pScreen);
}

void
mgaDrawDisable (ScreenPtr pScreen)
{
}

void
mgaDrawFini (ScreenPtr pScreen)
{
}

void
mgaDrawSync (ScreenPtr pScreen)
{
    KdScreenPriv (pScreen);
    mgaCardInfo (pScreenPriv);

    mmio = mgac->reg_base;

    mgaWaitIdle ();
}
