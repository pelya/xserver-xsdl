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

static VOL8 *mmio;
int fifo_size;

void
mgaWaitAvail (int n)
{
    int i;

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

  if (!mmio)
    return FALSE;

  mgaWaitAvail (wait + 6);
  MGA_OUT32 (mmio, MGA_REG_PITCH, mgas->pitch);
  MGA_OUT32 (mmio, MGA_REG_DSTORG, 0);
  MGA_OUT32 (mmio, MGA_REG_MACCESS, mgas->pw);
  MGA_OUT32 (mmio, MGA_REG_CXBNDRY, 0xffff0000);
  MGA_OUT32 (mmio, MGA_REG_YTOP, 0x00000000);
  MGA_OUT32 (mmio, MGA_REG_YBOT, 0x007fffff);
}

Bool
mgaPrepareSolid (DrawablePtr pDrawable, int alu, Pixel pm, Pixel fg)
{
    int cmd;

    cmd = MGA_OPCOD_TRAP | MGA_DWGCTL_SOLID | MGA_DWGCTL_ARZERO | MGA_DWGCTL_SGNZERO |
	MGA_DWGCTL_SHIFTZERO;
    /* XXX */
    cmd |= (12 << 16);

    mgaSetup (pDrawable->pScreen, 4);
    MGA_OUT32 (mmio, MGA_REG_DWGCTL, cmd);
    MGA_OUT32 (mmio, MGA_REG_FCOL, fg);
    MGA_OUT32 (mmio, MGA_REG_PLNWT, pm);
    return TRUE;
}

void
mgaSolid (int x1, int y1, int x2, int y2)
{
    mgaWaitAvail (2);
    MGA_OUT32 (mmio, MGA_REG_FXBNDRY, (x2 << 16) | x1 & 0xffff);
    MGA_OUT32 (mmio, MGA_REG_YDSTLEN | MGA_REG_EXEC, (y1 << 16) | (y2 - y1));
}

void
mgaDoneSolid (void)
{
}

Bool
mgaPrepareCopy (DrawablePtr pSrcDrawable, DrawablePtr pDstDrawable, int	dx, int dy, int alu, Pixel pm)
{
    return FALSE;
}

void
mgaCopy (int srcX, int srcY, int dstX, int dstY, int w, int h)
{
}

void
mgaDoneCopy (void)
{
}

KaaScreenPrivRec    mgaKaa = {
    mgaPrepareSolid,
    mgaSolid,
    mgaDoneSolid,

    mgaPrepareCopy,
    mgaCopy,
    mgaDoneCopy,
};

Bool
mgaDrawInit (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);

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
