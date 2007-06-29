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
#include "trident.h"
#include <sys/io.h>

#undef TRI_DEBUG

int	trident_clk = 0;
int	trident_mclk = 0;

#define CLOCK 14318	/* KHz */
#define CLK_N(a,b)	(a & 0xff)
#define CLK_M(a,b)	((b) & 0x3f)
#define CLK_K(a,b)	(((b) >> 6) & 3)
#define CLK_FREQ(a,b)	(((CLK_N(a,b) + 8) * CLOCK) / ((CLK_M(a,b)+2) << CLK_K(a,b)))

Bool
tridentCardInit (KdCardInfo *card)
{
    int			k;
    char		*pixels;
    TridentCardInfo	*tridentc;
    CARD8		r39;

    tridentc = (TridentCardInfo *) xalloc (sizeof (TridentCardInfo));
    if (!tridentc)
	return FALSE;
    
    iopl (3);
    tridentc->cop_base = (CARD8 *) KdMapDevice (TRIDENT_COP_BASE(card),
						TRIDENT_COP_SIZE(card));
    
    if (tridentc->cop_base)
    {
	KdSetMappedMode (TRIDENT_COP_BASE(card),
			 TRIDENT_COP_SIZE(card),
			 KD_MAPPED_MODE_REGISTERS);
    }
    tridentc->cop = (Cop *) (tridentc->cop_base + TRIDENT_COP_OFF(card));
    tridentc->mmio = FALSE;
    r39 = tridentReadIndex (tridentc, 0x3d4, 0x39);
    if (r39 & 1)
    {
	tridentc->mmio = TRUE;
	r39 = tridentReadIndex (tridentc, 0x3d4, 0x39);
	if ((r39 & 1) == 0)
	{
	    ErrorF ("Trident: inconsisent IO mapping values\n");
	    return FALSE;
	}
    }
    
#ifdef VESA
    if (!vesaInitialize (card, &tridentc->vesa))
#else
    if (!fbdevInitialize (card, &tridentc->fb))
#endif
    {
	xfree (tridentc);
	return FALSE;
    }
    
#ifdef USE_PCI
    tridentc->window = (CARD32 *) (tridentc->cop_base + 0x10000);
#else
    tridentc->window = 0;
#endif
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
#ifdef VESA
    if (!vesaScreenInitialize (screen, &tridents->vesa))
#else
    if (!fbdevScreenInitialize (screen, &tridents->fbdev))
#endif
    {
	xfree (tridents);
	return FALSE;
    }
    if (!tridentc->cop)
	screen->dumb = TRUE;
#ifdef VESA
    if (tridents->vesa.mapping != VESA_LINEAR)
	screen->dumb = TRUE;
    tridents->screen = tridents->vesa.fb;
    memory = tridents->vesa.fb_size;
#else
    tridents->screen = tridentc->fb.fb;
    memory = (2048 + 512) * 1024;
#endif
    screen_size = screen->fb[0].byteStride * screen->height;
    if (tridents->screen && memory >= screen_size + 2048)
    {
	memory -= 2048;
	tridents->cursor_base = tridents->screen + memory - 2048;
    }
    else
	tridents->cursor_base = 0;
    memory -= screen_size;
    if (memory > screen->fb[0].byteStride)
    {
	tridents->off_screen = tridents->screen + screen_size;
	tridents->off_screen_size = memory;
    }
    else
    {
	tridents->off_screen = 0;
	tridents->off_screen_size = 0;
    }
    screen->driver = tridents;
    return TRUE;
}

Bool
tridentInitScreen (ScreenPtr pScreen)
{
#ifdef VESA
    return vesaInitScreen (pScreen);
#else
    return fbdevInitScreen (pScreen);
#endif
}

Bool
tridentFinishInitScreen (ScreenPtr pScreen)
{
#ifdef VESA
    return vesaFinishInitScreen (pScreen);
#endif
}

CARD8
tridentReadIndex (TridentCardInfo *tridentc, CARD16 port, CARD8 index)
{
    CARD8   value;
    
    if (tridentc->mmio)
    {
	tridentc->cop_base[port] = index;
	value = tridentc->cop_base[port+1];
    }
    else
    {
	outb (index, port);
	value = inb (port+1);
    }
    return value;
}

void
tridentWriteIndex (TridentCardInfo *tridentc, CARD16 port, CARD8 index, CARD8 value)
{
    if (tridentc->mmio)
    {
	tridentc->cop_base[port] = index;
	tridentc->cop_base[port+1] = value;
    }
    else
    {
	outb (index, port);
	outb (value, port+1);
    }
}

CARD8
tridentReadReg (TridentCardInfo *tridentc, CARD16 port)
{
    CARD8   value;
    
    if (tridentc->mmio)
    {
	value = tridentc->cop_base[port];
    }
    else
    {
	value = inb (port);
    }
    return value;
}

void
tridentWriteReg (TridentCardInfo *tridentc, CARD16 port, CARD8 value)
{
    if (tridentc->mmio)
    {
	tridentc->cop_base[port] = value;
    }
    else
    {
	outb (value, port);
    }
}


void
tridentPause ()
{
    struct timeval  tv;

    tv.tv_sec = 0;
    tv.tv_usec = 50 * 1000;
    select (1, 0, 0, 0, &tv);
}

void
tridentPreserve (KdCardInfo *card)
{
    TridentCardInfo	*tridentc = card->driver;

#ifdef VESA
    vesaPreserve(card);
#else
    fbdevPreserve (card);
#endif
    tridentPause ();
    tridentc->save.reg_3c4_0e = tridentReadIndex (tridentc, 0x3c4, 0x0e);
    tridentc->save.reg_3d4_36 = tridentReadIndex (tridentc, 0x3d4, 0x36);
    tridentc->save.reg_3d4_39 = tridentReadIndex (tridentc, 0x3d4, 0x39);
    tridentc->save.reg_3d4_62 = tridentReadIndex (tridentc, 0x3d4, 0x62);
    tridentc->save.reg_3ce_21 = tridentReadIndex (tridentc, 0x3ce, 0x21);
    tridentc->save.reg_3c2 = tridentReadReg (tridentc, 0x3cc);
    tridentc->save.reg_3c4_16 = tridentReadIndex (tridentc, 0x3c4, 0x16);
    tridentc->save.reg_3c4_17 = tridentReadIndex (tridentc, 0x3c4, 0x17);
    tridentc->save.reg_3c4_18 = tridentReadIndex (tridentc, 0x3c4, 0x18);
    tridentc->save.reg_3c4_19 = tridentReadIndex (tridentc, 0x3c4, 0x19);
    ErrorF ("clk low 0x%x high 0x%x freq %d\n",
	    tridentc->save.reg_3c4_18,
	    tridentc->save.reg_3c4_19,
	    CLK_FREQ(tridentc->save.reg_3c4_18,
		     tridentc->save.reg_3c4_19));
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
tridentSetCLK(int clock, CARD8 *a, CARD8 *b)
{
    int powerup[4] = { 1,2,4,8 };
    int clock_diff = 750;
    int freq, ffreq;
    int m, n, k;
    int p, q, r, s; 
    int startn, endn;
    int endm, endk;

    p = q = r = s = 0;

    startn = 64;
    endn = 255;
    endm = 63;
    endk = 3;

    freq = clock;

    for (k=0;k<=endk;k++)
	for (n=startn;n<=endn;n++)
	    for (m=1;m<=endm;m++)
	    {
		ffreq = ( ( ((n + 8) * CLOCK) / ((m + 2) * powerup[k]) ));
		if ((ffreq > freq - clock_diff) && (ffreq < freq + clock_diff)) 
		{
		    clock_diff = (freq > ffreq) ? freq - ffreq : ffreq - freq;
		    p = n; q = m; r = k; s = ffreq;
		}
	    }

    ErrorF ("ffreq %d clock %d\n", s, clock);
    if (s == 0)
    {
	FatalError("Unable to set programmable clock.\n"
		   "Frequency %d is not a valid clock.\n"
		   "Please modify XF86Config for a new clock.\n",	
		   freq);
    }

    /* N is all 8bits */
    *a = p;
    /* M is first 6bits, with K last 2bits */
    *b = (q & 0x3F) | (r << 6);
}

void
tridentSetMCLK(int clock, CARD8 *a, CARD8 *b)
{
    int powerup[4] = { 1,2,4,8 };
    int clock_diff = 750;
    int freq, ffreq;
    int m,n,k;
    int p, q, r, s; 
    int startn, endn;
    int endm, endk;

    p = q = r = s = 0;

    startn = 64;
    endn = 255;
    endm = 63;
    endk = 3;

    freq = clock;

    for (k=0;k<=endk;k++)
	for (n=startn;n<=endn;n++)
	    for (m=1;m<=endm;m++) {
		ffreq = ((((n+8)*CLOCK)/((m+2)*powerup[k])));
		if ((ffreq > freq - clock_diff) && (ffreq < freq + clock_diff)) 
		{
		    clock_diff = (freq > ffreq) ? freq - ffreq : ffreq - freq;
		    p = n; q = m; r = k; s = ffreq;
		}
	    }

    if (s == 0)
    {
	FatalError("Unable to set memory clock.\n"
		   "Frequency %d is not a valid clock.\n"
		   "Please modify XF86Config for a new clock.\n",	
		   freq);
    }

    /* N is all 8bits */
    *a = p;
    /* M is first 6bits, with K last 2bits */
    *b = (q & 0x3F) | (r << 6);
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
	/* enable screen */
	tridentWriteIndex (tridentc, 0x3ce, 0x21, 0x80);
#ifdef USE_PCI
	/* enable burst r/w, enable memory mapped ports */
	tridentWriteIndex (tridentc, 0x3d4, 0x39, 7);
	tridentc->mmio = TRUE;
	/* reset GE, enable GE, set GE to pci 1 */
	tridentWriteIndex (tridentc, 0x3d4, 0x36, 0x90);
#else
	/* enable burst r/w, disable memory mapped ports */
	tridentWriteIndex (tridentc, 0x3d4, 0x39, 0x6);
	/* reset GE, enable GE, set GE to 0xbff00 */
	tridentWriteIndex (tridentc, 0x3d4, 0x36, 0x92);
#endif
	/* set clock */
	if (trident_clk)
	{
	    CARD8   a, b;

	    a = tridentReadIndex (tridentc, 0x3c4, 0x18);
	    b = tridentReadIndex (tridentc, 0x3c4, 0x19);
	    ErrorF ("old clock 0x%x 0x%x %d\n", 
		    a, b, CLK_FREQ(a,b));
	    tridentSetCLK (trident_clk, &a, &b);
	    ErrorF ("clk %d-> 0x%x 0x%x %d\n", trident_clk, a, b,
		    CLK_FREQ(a,b));
#if 1
	    tridentWriteIndex (tridentc, 0x3c4, 0x18, a);
	    tridentWriteIndex (tridentc, 0x3c4, 0x19, b);
#endif
	}
	if (trident_mclk)
	{
	    CARD8   a, b;

	    tridentSetMCLK (trident_mclk, &a, &b);
	    ErrorF ("mclk %d -> 0x%x 0x%x\n", trident_mclk, a, b);
#if 0
	    tridentWriteIndex (tridentc, 0x3c4, 0x16, a);
	    tridentWriteIndex (tridentc, 0x3c4, 0x17, b);
#endif
	}
	if (trident_clk || trident_mclk)
	{
	    CARD8   mode;

	    mode = tridentReadReg (tridentc, 0x3cc);
	    ErrorF ("old mode 0x%x\n", mode);
	    mode = (mode & 0xf3) | 0x08;
	    ErrorF ("new mode 0x%x\n", mode);
#if 1
	    tridentWriteReg (tridentc, 0x3c2, mode);
#endif
	}
#ifdef TRI_DEBUG
	fprintf (stderr, "0x36: 0x%02x\n",
		 tridentReadIndex (tridentc, 0x3d4, 0x36));
#endif
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
    tridentPause ();
#if 0
    tridentWriteIndex (tridentc, 0x3c4, 0x16, tridentc->save.reg_3c4_16);
    tridentWriteIndex (tridentc, 0x3c4, 0x17, tridentc->save.reg_3c4_17);
#endif
    tridentWriteIndex (tridentc, 0x3c4, 0x18, tridentc->save.reg_3c4_18);
    tridentWriteIndex (tridentc, 0x3c4, 0x19, tridentc->save.reg_3c4_19);
    tridentWriteReg (tridentc, 0x3c2, tridentc->save.reg_3c2);
    tridentPause ();
    tridentWriteIndex (tridentc, 0x3ce, 0x21, tridentc->save.reg_3ce_21);
    tridentPause ();
    tridentWriteIndex (tridentc, 0x3d4, 0x62, tridentc->save.reg_3d4_62);
    tridentWriteIndex (tridentc, 0x3d4, 0x39, tridentc->save.reg_3d4_39);
    tridentc->mmio = FALSE;
    tridentWriteIndex (tridentc, 0x3d4, 0x36, tridentc->save.reg_3d4_36);
    tridentWriteIndex (tridentc, 0x3c4, 0x0e, tridentc->save.reg_3c4_0e);
    tridentPause ();
}

Bool
tridentEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    TridentCardInfo	*tridentc = pScreenPriv->card->driver;

#ifdef VESA
    if (!vesaEnable (pScreen))
	return FALSE;
#else
    if (!fbdevEnable (pScreen))
	return FALSE;
#endif
    tridentSetMMIO (tridentc);
    return TRUE;
}

void
tridentDisable (ScreenPtr pScreen)
{
#ifdef VESA
    vesaDisable (pScreen);
#else
    fbdevDisable (pScreen);
#endif
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
    tridentPause ();
    return TRUE;
}

void
tridentRestore (KdCardInfo *card)
{
    TridentCardInfo	*tridentc = card->driver;

    tridentResetMMIO (tridentc);
#ifdef VESA
    vesaRestore (card);
#else
    fbdevRestore (card);
#endif
}

void
tridentScreenFini (KdScreenInfo *screen)
{
    TridentScreenInfo	*tridents = (TridentScreenInfo *) screen->driver;

#ifdef VESA
    vesaScreenFini (screen);
#endif
    xfree (tridents);
    screen->driver = 0;
}

void
tridentCardFini (KdCardInfo *card)
{
    TridentCardInfo	*tridentc = card->driver;

    if (tridentc->cop_base)
    {
	KdUnmapDevice ((void *) tridentc->cop_base, TRIDENT_COP_SIZE(card));
	KdResetMappedMode (TRIDENT_COP_BASE(card),
			   TRIDENT_COP_SIZE(card),
			   KD_MAPPED_MODE_REGISTERS);
    }
#ifdef VESA
    vesaCardFini (card);
#else
    fbdevCardFini (card);
#endif
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
    
#ifdef VESA
    vesaGetColors,    	    /* getColors */
    vesaPutColors,	    /* putColors */
#else
    fbdevGetColors,    	    /* getColors */
    fbdevPutColors,	    /* putColors */
#endif
    tridentFinishInitScreen /* finishInitScreen */
};
