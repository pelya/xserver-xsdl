/*
 * $XFree86$
 *
 * Copyright © 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#include "igs.h"

Bool
igsCardInit (KdCardInfo *card)
{
    int		k;
    char	*pixels;
    IgsCardInfo	*igsc;

    igsc = (IgsCardInfo *) xalloc (sizeof (IgsCardInfo));
    if (!igsc)
	return FALSE;
    
    memset (igsc, '\0', sizeof (IgsCardInfo));

    igsc->frameBuffer = (CARD8 *) KdMapDevice (card->attr.address[0],
					       4096 * 1024);
    
    igsc->cop = (Cop5xxx *) KdMapDevice (card->attr.address[0] + 
					 IGS_COP_OFFSET,
					 sizeof (Cop5xxx));
    
    igsc->copData = (VOL32 *) KdMapDevice (card->attr.address[0] +
					   IGS_COP_DATA,
					   IGS_COP_DATA_LEN);
    
    card->driver = igsc;
    
    return TRUE;
}

Bool
igsScreenInit (KdScreenInfo *screen)
{
    IgsCardInfo	*igsc = screen->card->driver;
    int		fb = 0;
    
    if (screen->fb[fb].depth >= 24)
    {
	screen->fb[fb].depth = 24;
	if (screen->fb[fb].bitsPerPixel != 24)
	    screen->fb[fb].bitsPerPixel = 32;
    }
    else if (screen->fb[fb].depth >= 16)
    {
	screen->fb[fb].depth = 16;
	screen->fb[fb].bitsPerPixel = 16;
    }
    else if (screen->fb[fb].depth >= 15)
    {
	screen->fb[fb].depth = 15;
	screen->fb[fb].bitsPerPixel = 16;
    }
    else
    {
	screen->fb[fb].depth = 8;
	screen->fb[fb].bitsPerPixel = 8;
    }
    switch (screen->fb[fb].depth) {
    case 8:
	screen->fb[fb].visuals = ((1 << StaticGray) |
				  (1 << GrayScale) |
				  (1 << StaticColor) |
				  (1 << PseudoColor) |
				  (1 << TrueColor) |
				  (1 << DirectColor));
	screen->fb[fb].blueMask  = 0x00;
	screen->fb[fb].greenMask = 0x00;
	screen->fb[fb].redMask   = 0x00;
	break;
    case 15:
	screen->fb[fb].visuals = (1 << TrueColor);
	screen->fb[fb].blueMask  = 0x001f;
	screen->fb[fb].greenMask = 0x03e0;
	screen->fb[fb].redMask   = 0x7c00;
	break;
    case 16:
	screen->fb[fb].visuals = (1 << TrueColor);
	screen->fb[fb].blueMask  = 0x001f;
	screen->fb[fb].greenMask = 0x07e0;
	screen->fb[fb].redMask   = 0xf800;
	break;
    case 24:
	screen->fb[fb].visuals = (1 << TrueColor);
	screen->fb[fb].blueMask  = 0x0000ff;
	screen->fb[fb].greenMask = 0x00ff00;
	screen->fb[fb].redMask   = 0xff0000;
	break;
    }
    screen->fb[fb].pixelStride = screen->width;
    screen->fb[fb].byteStride = screen->width * (screen->fb[fb].bitsPerPixel >> 3);
    screen->fb[fb].frameBuffer = igsc->frameBuffer;
    if (!igsc->cop)
	screen->dumb = TRUE;
    return TRUE;
}

Bool
igsInitScreen(ScreenPtr pScreen)
{
    return TRUE;
}
    
void
igsPreserve (KdCardInfo *card)
{
}

void
igsEnable (ScreenPtr pScreen)
{
}

Bool
igsDPMS (ScreenPtr pScreen, int mode)
{
    return TRUE;
}

void
igsDisable (ScreenPtr pScreen)
{
}

void
igsRestore (KdCardInfo *card)
{
}
    
void
igsScreenFini (KdScreenInfo *screen)
{
}
    
void
igsCardFini (KdCardInfo *card)
{
    IgsCardInfo	*igsc = card->driver;

    if (igsc->copData)
	KdUnmapDevice ((void *) igsc->copData, IGS_COP_DATA_LEN);
    if (igsc->cop)
	KdUnmapDevice (igsc->cop, sizeof (Cop5xxx));
    if (igsc->frameBuffer)
	KdUnmapDevice (igsc->frameBuffer, 4096 * 1024);
    xfree (igsc);
    card->driver = 0;
}

KdCardFuncs	igsFuncs = {
    igsCardInit,	    /* cardinit */
    igsScreenInit,	    /* scrinit */
    igsInitScreen,
    igsPreserve,	    /* preserve */
    igsEnable,		    /* enable */
    igsDPMS,		    /* dpms */
    igsDisable,		    /* disable */
    igsRestore,		    /* restore */
    igsScreenFini,	    /* scrfini */
    igsCardFini,	    /* cardfini */
    
    0,			    /* initCursor */
    0,			    /* enableCursor */
    0,			    /* disableCursor */
    0,			    /* finiCursor */
    0,			    /* recolorCursor */
    
    igsDrawInit,    	    /* initAccel */
    igsDrawEnable,    	    /* enableAccel */
    igsDrawSync,	    /* drawSync */
    igsDrawDisable,    	    /* disableAccel */
    igsDrawFini,    	    /* finiAccel */
    
    0,			    /* getColors */
    0,			    /* putColors */
};
