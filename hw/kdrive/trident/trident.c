/*
 * $Id$
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
/* $XFree86: xc/programs/Xserver/hw/kdrive/trident/trident.c,v 1.1 1999/11/19 13:54:01 hohndel Exp $ */

#include "trident.h"
#define extern
#include <asm/io.h>
#undef extern

#undef TRI_DEBUG

Bool
tridentCardInit (KdCardInfo *card)
{
    int		k;
    char	*pixels;
    TridentCardInfo	*tridentc;

    tridentc = (TridentCardInfo *) xalloc (sizeof (TridentCardInfo));
    if (!tridentc)
	return FALSE;
    
    if (!fbdevInitialize (card, &tridentc->fb))
    {
	xfree (tridentc);
	return FALSE;
    }
    
    iopl (3);
    tridentc->cop_base = (CARD8 *) KdMapDevice (TRIDENT_COP_BASE,
					      TRIDENT_COP_SIZE);
    tridentc->cop = (Cop *) (tridentc->cop_base + TRIDENT_COP_OFF);
    card->driver = tridentc;
    
    return TRUE;
}

Bool
tridentScreenInit (KdScreenInfo *screen)
{
    TridentCardInfo	*tridentc = screen->card->driver;
    TridentScreenInfo	*tridents;
    int			screen_size, memory;

    tridents = (TridentScreenInfo *) xalloc (sizeof (TridentScreenInfo));
    if (!tridents)
	return FALSE;
    memset (tridents, '\0', sizeof (TridentScreenInfo));
    if (!fbdevScreenInit (screen))
    {
	xfree (tridents);
	return FALSE;
    }
    if (!tridentc->cop)
	screen->dumb = TRUE;
    screen_size = screen->byteStride * screen->height;
    memory = (2048 + 512) * 1024;
    if (memory >= screen_size + 2048)
    {
	tridents->cursor_base = tridentc->fb.fb + memory - 2048;
    }
    else
	tridents->cursor_base = 0;
    screen->driver = tridents;
    return TRUE;
}

Bool
tridentInitScreen (ScreenPtr pScreen)
{
    return fbdevInitScreen (pScreen);
}

CARD8
tridentReadIndex (TridentCardInfo *tridentc, CARD16 port, CARD8 index)
{
    CARD8   value;
    
    outb (index, port);
    value = inb (port+1);
    return value;
}

void
tridentWriteIndex (TridentCardInfo *tridentc, CARD16 port, CARD8 index, CARD8 value)
{
    outb (index, port);
    outb (value, port+1);
}

void
tridentPause ()
{
    struct timeval  tv;

    tv.tv_sec = 0;
    tv.tv_usec = 200 * 1000;
    select (1, 0, 0, 0, &tv);
}

void
tridentPreserve (KdCardInfo *card)
{
    TridentCardInfo	*tridentc = card->driver;

    fbdevPreserve (card);
    tridentc->save.reg_3c4_0e = tridentReadIndex (tridentc, 0x3c4, 0x0e);
    tridentc->save.reg_3d4_36 = tridentReadIndex (tridentc, 0x3d4, 0x36);
    tridentc->save.reg_3d4_39 = tridentReadIndex (tridentc, 0x3d4, 0x39);
    tridentc->save.reg_3d4_62 = tridentReadIndex (tridentc, 0x3d4, 0x62);
    tridentc->save.reg_3ce_21 = tridentReadIndex (tridentc, 0x3ce, 0x21);
#ifdef TRI_DEBUG
    fprintf (stderr, "3c4 0e: %02x\n", tridentc->save.reg_3c4_0e);
    fprintf (stderr, "3d4 36: %02x\n", tridentc->save.reg_3d4_36);
    fprintf (stderr, "3d4 39: %02x\n", tridentc->save.reg_3d4_39);
    fprintf (stderr, "3d4 62: %02x\n", tridentc->save.reg_3d4_62);
    fprintf (stderr, "3ce 21: %02x\n", tridentc->save.reg_3ce_21);
    fflush (stderr);
#endif
    tridentPause ();
}

void
tridentSetMMIO (TridentCardInfo *tridentc)
{
    int	tries;
    CARD8   v;

#ifdef TRI_DEBUG
    fprintf (stderr, "Set MMIO\n");
#endif
    /* enable config port writes */
    for (tries = 0; tries < 3; tries++)
    {
	/* enable direct read when GE busy, enable PCI retries */
	tridentWriteIndex (tridentc, 0x3d4, 0x62,
			   tridentc->save.reg_3d4_62 | 0x70);
	/* make sure the chip is in new mode */
	tridentReadIndex (tridentc, 0x3c4, 0xb);
	/* enable access to upper registers */
	tridentWriteIndex (tridentc, 0x3c4, 0xe, 
			   tridentc->save.reg_3c4_0e | 0x80);
	v = tridentReadIndex (tridentc, 0x3c4, 0xe);
	if (!(v & 0x80))
	{
	    fprintf (stderr, "Trident GE not enabled 0x%x\n", v);
	    continue;
	}
	/* enable burst r/w, disable memory mapped ports */
	tridentWriteIndex (tridentc, 0x3d4, 0x39, 0x6);
	/* reset GE, enable GE, set GE to 0xbff00 */
	tridentWriteIndex (tridentc, 0x3d4, 0x36, 0x92);
	if (tridentc->cop->status != 0xffffffff)
	    break;
    }
#ifdef TRI_DEBUG
    fprintf (stderr, "COP status 0x%x\n", tridentc->cop->status);
#endif
    if (tridentc->cop->status == 0xffffffff)
	FatalError ("Trident COP not visible\n");
}

void
tridentResetMMIO (TridentCardInfo *tridentc)
{
#ifdef TRI_DEBUG
    fprintf (stderr, "Reset MMIO\n");
#endif
    tridentWriteIndex (tridentc, 0x3ce, 0x21, tridentc->save.reg_3ce_21);
    tridentWriteIndex (tridentc, 0x3d4, 0x62, tridentc->save.reg_3d4_62);
    tridentWriteIndex (tridentc, 0x3d4, 0x39, tridentc->save.reg_3d4_39);
    tridentWriteIndex (tridentc, 0x3d4, 0x36, tridentc->save.reg_3d4_36);
    tridentWriteIndex (tridentc, 0x3c4, 0x0e, tridentc->save.reg_3c4_0e);
    tridentPause ();
}

void
tridentEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    TridentCardInfo	*tridentc = pScreenPriv->card->driver;

    fbdevEnable (pScreen);
    tridentSetMMIO (tridentc);
}

void
tridentDisable (ScreenPtr pScreen)
{
    fbdevDisable (pScreen);
}

const CARD8	tridentDPMSModes[4] = {
    0x80,	    /* KD_DPMS_NORMAL */
    0x8c,	    /* KD_DPMS_STANDBY */
    0x8c,	    /* KD_DPMS_STANDBY */
    0x8c,	    /* KD_DPMS_STANDBY */
/*    0xb0,	    /* KD_DPMS_SUSPEND */
/*    0xbc,	    /* KD_DPMS_POWERDOWN */
};

Bool
tridentDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    TridentCardInfo	*tridentc = pScreenPriv->card->driver;

    tridentWriteIndex (tridentc, 0x3ce, 0x21, tridentDPMSModes[mode]);
    return TRUE;
}

void
tridentRestore (KdCardInfo *card)
{
    TridentCardInfo	*tridentc = card->driver;

    tridentResetMMIO (tridentc);
    fbdevRestore (card);
}

void
tridentScreenFini (KdScreenInfo *screen)
{
    TridentScreenInfo	*tridents = (TridentScreenInfo *) screen->driver;

    xfree (tridents);
    screen->driver = 0;
}

void
tridentCardFini (KdCardInfo *card)
{
    TridentCardInfo	*tridentc = card->driver;

    if (tridentc->cop_base)
	KdUnmapDevice ((void *) tridentc->cop_base, TRIDENT_COP_SIZE);
    fbdevCardFini (card);
}

KdCardFuncs	tridentFuncs = {
    tridentCardInit,	    /* cardinit */
    tridentScreenInit,	    /* scrinit */
    tridentInitScreen,	    /* initScreen */
    tridentPreserve,	    /* preserve */
    tridentEnable,	    /* enable */
    tridentDPMS,	    /* dpms */
    tridentDisable,	    /* disable */
    tridentRestore,	    /* restore */
    tridentScreenFini,	    /* scrfini */
    tridentCardFini,	    /* cardfini */
    
    tridentCursorInit,	    /* initCursor */
    tridentCursorEnable,    /* enableCursor */
    tridentCursorDisable,   /* disableCursor */
    tridentCursorFini,	    /* finiCursor */
    tridentRecolorCursor,   /* recolorCursor */
    
    tridentDrawInit,        /* initAccel */
    tridentDrawEnable,      /* enableAccel */
    tridentDrawSync,	    /* syncAccel */
    tridentDrawDisable,     /* disableAccel */
    tridentDrawFini,        /* finiAccel */
    
    fbdevGetColors,    	    /* getColors */
    fbdevPutColors,	    /* putColors */
};
