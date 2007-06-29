/*
 * Copyright 2001 by Alan Hourihane, Sychdyn, North Wales, UK.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@fairlite.demon.co.uk>
 *
 * A driver for the following PCMCIA cards...
 * 		Hewlett Packards HP VGA Out (Model F1252A)
 *		Colorgraphics Voyager VGA
 *
 * Tested running under a Compaq IPAQ Pocket PC running Linux
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "pcmcia.h"
#define extern
#include <asm/io.h>
#undef extern

#define CLOCK 14318	/* KHz */
#define CLK_N(a,b)	(a & 0xff)
#define CLK_M(a,b)	((b) & 0x3f)
#define CLK_K(a,b)	(((b) >> 6) & 3)
#define CLK_FREQ(a,b)	(((CLK_N(a,b) + 8) * CLOCK) / ((CLK_M(a,b)+2) << CLK_K(a,b)))

extern void
tridentUpdatePacked (ScreenPtr pScreen,
		    shadowBufPtr pBuf);
extern void
cirrusUpdatePacked (ScreenPtr pScreen,
		    shadowBufPtr pBuf);

static Bool
tridentSetCLK(int clock, CARD8 *a, CARD8 *b);

static Bool
CirrusFindClock(int freq, int *num_out, int *den_out);
    
Bool
pcmciaCardInit (KdCardInfo *card)
{
    pcmciaCardInfo	*pcmciac;
    CARD8		r9;

    pcmciac = (pcmciaCardInfo *) xalloc (sizeof (pcmciaCardInfo));
    if (!pcmciac)
	return FALSE;
    
    pcmciac->cop_base = (CARD8 *) KdMapDevice (PCMCIA_COP_BASE(card),
					       PCMCIA_COP_SIZE(card));
    
    r9 = pcmciaReadIndex (pcmciac, 0x3c4, 0x09);
    /* 
     * Crude detection....
     * The trident chip has a read only register at 0x09, which returns 0x4.
     * If it's not that, we assume the cirrus chip.
     * BREAKAGE.! If we have an anonymous PCMCIA card inserted, we could 
     * potentially smash something here. FIXME !
     */
    if (r9 == 0x04) {
    	ErrorF("PCMCIA: Found HP VGA card\n");
	pcmciac->HP = TRUE;	/* Select HP VGA Out Card */
    } else {
    	ErrorF("PCMCIA: Found Voyager VGA card\n");
	pcmciac->HP = FALSE;	/* Select Voyager VGA Card */
    }

    if (pcmciac->HP) {
    	/* needed by the accelerator - later */
    	pcmciac->cop = (Cop *) (pcmciac->cop_base + TRIDENT_COP_OFF(card));
    }

    /*
     * Map frame buffer 
     */
    if (pcmciac->HP)
    	pcmciac->fb = KdMapDevice (0x2ce00000, 0x80000);
    else
    	pcmciac->fb = KdMapDevice (0x2c0a0000, 0x10000); /*64K bank switched*/

    if (!pcmciac->fb)
	return FALSE;

    pcmciac->window = 0;

    card->driver = pcmciac;

    return TRUE;
}

Bool
pcmciaModeSupported (KdScreenInfo		*screen,
		     const KdMonitorTiming	*t)
{
    KdCardInfo		*card = screen->card;
    pcmciaCardInfo	*pcmciac = (pcmciaCardInfo *) card->driver;

    if (pcmciac->HP)
    {
	CARD8	a, b;
	if (!tridentSetCLK (t->clock, &a, &b))
	    return FALSE;
    }
    else
    {
	int a, b;
	if (!CirrusFindClock (t->clock, &a, &b))
	    return FALSE;
    }
    
    /* width must be a multiple of 16 */
    if (t->horizontal & 0xf)
	return FALSE;
    return TRUE;
}

Bool
pcmciaModeUsable (KdScreenInfo	*screen)
{
    KdCardInfo		*card = screen->card;
    pcmciaCardInfo	*pcmciac = (pcmciaCardInfo *) card->driver;
    int			screen_size;
    int			pixel_width;
    int			byte_width;
    int			fb;
    
    if (screen->fb[0].depth == 8) 
    	screen->fb[0].bitsPerPixel = 8;
    else if (screen->fb[0].depth == 15 || screen->fb[0].depth == 16)
    	screen->fb[0].bitsPerPixel = 16;
    else
	return FALSE;

    screen_size = 0;
    screen->fb[0].pixelStride = screen->width;
    screen->fb[0].byteStride = screen->width * (screen->fb[0].bitsPerPixel >>3);
    screen->fb[0].frameBuffer = pcmciac->fb;
    screen_size = screen->fb[0].byteStride * screen->height;
    
    return screen_size <= pcmciac->memory;
}

Bool
pcmciaScreenInit (KdScreenInfo *screen)
{
    pcmciaCardInfo	*pcmciac = screen->card->driver;
    pcmciaScreenInfo	*pcmcias;
    int			screen_size, memory;
    int			i;
    const KdMonitorTiming   *t;

    pcmcias = (pcmciaScreenInfo *) xalloc (sizeof (pcmciaScreenInfo));
    if (!pcmcias)
	return FALSE;
    memset (pcmcias, '\0', sizeof (pcmciaScreenInfo));

    /* if (!pcmciac->cop) */
	screen->dumb = TRUE;

    if (screen->fb[0].depth < 8) 
	screen->fb[0].depth = 8;
    
    /* default to 16bpp */
    if (!screen->fb[0].depth)
	screen->fb[0].depth = 16;

    /* default to 60Hz refresh */
    if (!screen->width || !screen->height)
    {
	screen->width = 640;
	screen->height = 400;
	screen->rate = 60;
    }

    pcmciac->memory = 512 * 1024;
    if (pcmciac->HP && !screen->softCursor && screen->fb[0].depth == 8) 
    {
	/* ack, bail on the HW cursor for everything -- no ARGB falback */
	pcmcias->cursor_base = 0;
#if 0
	/* Let's do hw cursor for the HP card, only in 8bit mode though */
    	pcmcias->cursor_base = pcmcias->screen + pcmciac->memory - 4096; 
    	pcmciac->memory -= 4096;
#endif
    }

    pcmcias->screen = pcmciac->fb;
    screen->driver = pcmcias;

    t = KdFindMode (screen, pcmciaModeSupported);
    
    screen->rate = t->rate;
    screen->width = t->horizontal;
    screen->height = t->vertical;

    pcmcias->randr = screen->randr;

    if (!KdTuneMode (screen, pcmciaModeUsable, pcmciaModeSupported))
    {
	xfree (pcmcias);
	return FALSE;
    }

    switch (screen->fb[0].depth) {
    case 4:
	screen->fb[0].visuals = ((1 << StaticGray) |
			   (1 << GrayScale) |
			   (1 << StaticColor));
	screen->fb[0].blueMask  = 0x00;
	screen->fb[0].greenMask = 0x00;
	screen->fb[0].redMask   = 0x00;
	break;
    case 8:
	screen->fb[0].visuals = ((1 << StaticGray) |
			   (1 << GrayScale) |
			   (1 << StaticColor) |
			   (1 << PseudoColor) |
			   (1 << TrueColor) |
			   (1 << DirectColor));
	screen->fb[0].blueMask  = 0x00;
	screen->fb[0].greenMask = 0x00;
	screen->fb[0].redMask   = 0x00;
	break;
    case 15:
	screen->fb[0].visuals = (1 << TrueColor);
	screen->fb[0].blueMask  = 0x001f;
	screen->fb[0].greenMask = 0x03e0;
	screen->fb[0].redMask   = 0x7c00;
	break;
    case 16:
	screen->fb[0].visuals = (1 << TrueColor);
	screen->fb[0].blueMask  = 0x001f;
	screen->fb[0].greenMask = 0x07e0;
	screen->fb[0].redMask   = 0xf800;
	break;
    }

    return TRUE;
}

void *
tridentWindowLinear (ScreenPtr	pScreen,
		     CARD32	row,
		     CARD32	offset,
		     int	mode,
		     CARD32	*size,
		     void 	*closure)
{
    KdScreenPriv(pScreen);
    pcmciaCardInfo	*pcmciac = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled)
	return 0;

    *size = pScreenPriv->screen->fb[0].byteStride;
    return (CARD8 *) pcmciac->fb + row * pScreenPriv->screen->fb[0].byteStride + offset;
}

void *
cirrusWindowWindowed (ScreenPtr	pScreen,
		     CARD32	row,
		     CARD32	offset,
		     int	mode,
		     CARD32	*size,
		     void 	*closure)
{
    KdScreenPriv(pScreen);
    pcmciaCardInfo	*pcmciac = pScreenPriv->card->driver;
    int bank, boffset;

    if (!pScreenPriv->enabled)
	return 0;

    bank = (row * pScreenPriv->screen->fb[0].byteStride) / 0x1000;
    pcmciaWriteIndex(pcmciac, 0x3ce, 0x0B, 0x0c);
    pcmciaWriteIndex(pcmciac, 0x3ce, 0x09, bank);
    pcmciaWriteIndex(pcmciac, 0x3ce, 0x0A, bank);
    *size = pScreenPriv->screen->fb[0].byteStride;
    return (CARD8 *) pcmciac->fb + (row * pScreenPriv->screen->fb[0].byteStride) - (bank * 0x1000) + offset;
}

LayerPtr
pcmciaLayerCreate (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    pcmciaCardInfo	*pcmciac = pScreenPriv->card->driver;
    pcmciaScreenInfo	*pcmcias = (pcmciaScreenInfo *) pScreenPriv->screen->driver;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;
    PixmapPtr		pPixmap;
    int			kind;

    if (pcmciac->HP) {
    	window = tridentWindowLinear;
	if (pcmcias->randr == RR_Rotate_0)
	    update = tridentUpdatePacked;
	else
	    update = pcmciaUpdateRotatePacked;
    } else {
    	window = cirrusWindowWindowed;
	if (pcmcias->randr == RR_Rotate_0)
	    update = cirrusUpdatePacked;
	else
	    update = pcmciaUpdateRotatePacked;
    }

    if (!update)
	abort ();

    kind = LAYER_SHADOW;
    pPixmap = 0;

    return LayerCreate (pScreen, kind, screen->fb[0].depth, 
			pPixmap, update, window, pcmcias->randr, 0);
}

void
pcmciaConfigureScreen (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    FbdevPriv		*priv = pScreenPriv->card->driver;
    pcmciaScreenInfo	*pcmcias = (pcmciaScreenInfo *) screen->driver;
    KdMouseMatrix	m;

    KdComputeMouseMatrix (&m, pcmcias->randr, 
			  screen->width, screen->height);
    
    if (m.matrix[0][0])
    {
	pScreen->width = screen->width;
	pScreen->height = screen->height;
	pScreen->mmWidth = screen->width_mm;
	pScreen->mmHeight = screen->height_mm;
    }
    else
    {
	pScreen->width = screen->height;
	pScreen->height = screen->width;
	pScreen->mmWidth = screen->height_mm;
	pScreen->mmHeight = screen->width_mm;
    }
    KdSetMouseMatrix (&m);
}

#ifdef RANDR

Bool
pcmciaRandRSupported (ScreenPtr		    pScreen,
		      const KdMonitorTiming *t)
{
    KdScreenPriv(pScreen);
    pcmciaCardInfo	    *pcmciac = pScreenPriv->card->driver;
    KdScreenInfo	    *screen = pScreenPriv->screen;
    int			    screen_size;
    int			    byteStride;
    
    /* Make sure the clock is supported */
    if (!pcmciaModeSupported (screen, t))
	return FALSE;
    /* Check for sufficient memory */
    byteStride = screen->width * (screen->fb[0].bitsPerPixel >>3);
    screen_size = byteStride * screen->height;

    return screen_size <= pcmciac->memory;
}

Bool
pcmciaRandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
    KdScreenPriv(pScreen);
    pcmciaScreenInfo	*pcmcias = (pcmciaScreenInfo *) pScreenPriv->screen->driver;
    
    *rotations = (RR_Rotate_0|RR_Rotate_90|RR_Rotate_180|RR_Rotate_270|
		  RR_Reflect_X|RR_Reflect_Y);
    
    return KdRandRGetInfo (pScreen, pcmcias->randr, pcmciaRandRSupported);
}
 
int
pcmciaLayerAdd (WindowPtr pWin, pointer value)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLayer = (LayerPtr) value;

    if (!LayerWindowAdd (pScreen, pLayer, pWin))
	return WT_STOPWALKING;

    return WT_WALKCHILDREN;
}

int
pcmciaLayerRemove (WindowPtr pWin, pointer value)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLayer = (LayerPtr) value;

    LayerWindowRemove (pScreen, pLayer, pWin);

    return WT_WALKCHILDREN;
}

pcmciaRandRSetConfig (ScreenPtr		pScreen,
		      Rotation		randr,
		      int		rate,
		      RRScreenSizePtr	pSize)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	    *screen = pScreenPriv->screen;
    FbdevPriv		    *priv = pScreenPriv->card->driver;
    pcmciaScreenInfo	    *pcmcias = (pcmciaScreenInfo *) pScreenPriv->screen->driver;
    Bool		    wasEnabled = pScreenPriv->enabled;
    int			    newwidth, newheight;
    LayerPtr		    pNewLayer;
    int			    kind;
    int			    oldrandr = pcmcias->randr;
    PixmapPtr		    pPixmap;
    const KdMonitorTiming   *t;
    
    randr = KdAddRotation (screen->randr, randr);
    
    t = KdRandRGetTiming (pScreen, pcmciaRandRSupported, rate, pSize);
    
    if (wasEnabled)
        KdDisableScreen (pScreen);
	
    screen->rate = t->rate;
    screen->width = t->horizontal;
    screen->height = t->vertical;

    pcmcias->randr = randr;
    pcmciaConfigureScreen (pScreen);

    pNewLayer = pcmciaLayerCreate (pScreen);
    
    if (!pNewLayer)
    {
	pcmcias->randr = oldrandr;
	pcmciaConfigureScreen (pScreen);
	if (wasEnabled)
	    KdEnableScreen (pScreen);
	return FALSE;
    }
	
    if (WalkTree (pScreen, pcmciaLayerAdd, (pointer) pNewLayer) == WT_STOPWALKING)
    {
	WalkTree (pScreen, pcmciaLayerRemove, (pointer) pNewLayer);
	LayerDestroy (pScreen, pNewLayer);
	pcmcias->randr = oldrandr;
	pcmciaConfigureScreen (pScreen);
	if (wasEnabled)
	    KdEnableScreen (pScreen);
	return FALSE;
    }
    WalkTree (pScreen, pcmciaLayerRemove, (pointer) pcmcias->pLayer);
    LayerDestroy (pScreen, pcmcias->pLayer);
    pcmcias->pLayer = pNewLayer;
    if (wasEnabled)
	KdEnableScreen (pScreen);
    return TRUE;
}

Bool
pcmciaRandRInit (ScreenPtr pScreen)
{
    rrScrPrivPtr    pScrPriv;
    
    if (!RRScreenInit (pScreen))
	return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = pcmciaRandRGetInfo;
    pScrPriv->rrSetConfig = pcmciaRandRSetConfig;
    return TRUE;
}
#endif

Bool
pcmciaInitScreen (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    FbdevPriv		*priv = pScreenPriv->card->driver;
    pcmciaScreenInfo	*pcmcias = (pcmciaScreenInfo *) pScreenPriv->screen->driver;

    if (!LayerStartInit (pScreen))
	return FALSE;
    if (!LayerFinishInit (pScreen))
	return FALSE;

    pcmciaConfigureScreen (pScreen);

    pcmcias->pLayer = pcmciaLayerCreate (pScreen);
    if (!pcmcias->pLayer)
	return FALSE;
#ifdef RANDR
    if (!pcmciaRandRInit (pScreen))
	return FALSE;
#endif
    return TRUE;
}

CARD8
pcmciaReadIndex (pcmciaCardInfo *pcmciac, CARD16 port, CARD8 index)
{
    CARD8   value;
    
    pcmciac->cop_base[port] = index;
    value = pcmciac->cop_base[port+1];
    return value;
}

void
pcmciaWriteIndex (pcmciaCardInfo *pcmciac, CARD16 port, CARD8 index, CARD8 value)
{
    pcmciac->cop_base[port] = index;
    pcmciac->cop_base[port+1] = value;
}

CARD8
pcmciaReadReg (pcmciaCardInfo *pcmciac, CARD16 port)
{
    CARD8   value;

    value = pcmciac->cop_base[port];

    return value;
}

void
pcmciaWriteReg (pcmciaCardInfo *pcmciac, CARD16 port, CARD8 value)
{
    pcmciac->cop_base[port] = value;
}


void
pcmciaPause ()
{
    struct timeval  tv;

    tv.tv_sec = 0;
    tv.tv_usec = 50 * 1000;
    select (1, 0, 0, 0, &tv);
}

void
pcmciaPreserve (KdCardInfo *card)
{
}

/* CLOCK_FACTOR is double the osc freq in kHz (osc = 14.31818 MHz) */
#define CLOCK_FACTOR 28636

/* stability constraints for internal VCO -- MAX_VCO also determines the maximum Video pixel clock */
#define MIN_VCO CLOCK_FACTOR
#define MAX_VCO 111000

/* clock in kHz is (numer * CLOCK_FACTOR / (denom & 0x3E)) >> (denom & 1) */
#define VCOVAL(n, d) \
     ((((n) & 0x7F) * CLOCK_FACTOR / ((d) & 0x3E)) )

#define CLOCKVAL(n, d) \
     (VCOVAL(n, d) >> ((d) & 1))

static Bool
CirrusFindClock(int freq, int *num_out, int *den_out)
{
    int n;
    int num = 0, den = 0;
    int mindiff;

    /*
     * If max_clock is greater than the MAX_VCO default, ignore
     * MAX_VCO. On the other hand, if MAX_VCO is higher than max_clock,
     * make use of the higher MAX_VCO value.
     */

    mindiff = freq; 
    for (n = 0x10; n < 0x7f; n++) {
	int d;
	for (d = 0x14; d < 0x3f; d++) {
	    int c, diff;
	    /* Avoid combinations that can be unstable. */
	    if ((VCOVAL(n, d) < MIN_VCO) || (VCOVAL(n, d) > MAX_VCO))
		continue;
	    c = CLOCKVAL(n, d);
	    diff = abs(c - freq);
	    if (diff < mindiff) {
		mindiff = diff;
		num = n;
		den = d;
	    }
	}
    }
    if (n == 0x80)
	return FALSE;

    *num_out = num;
    *den_out = den;

    return TRUE;
}


static Bool
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

    startn = 0;
    endn = 121;
    endm = 31;
    endk = 1;

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

#if 0
    ErrorF ("ffreq %d clock %d\n", s, clock);
#endif
    if (s == 0)
	return FALSE;

    /* N is first 7bits, first M bit is 8th bit */
    *a = ((1 & q) << 7) | p;
    /* first 4bits are rest of M, 1bit for K value */
    *b = (((q & 0xFE) >> 1) | (r << 4));
    return TRUE;
}

Bool
pcmciaEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    pcmciaCardInfo	*pcmciac = pScreenPriv->card->driver;
    pcmciaScreenInfo	*pcmcias = (pcmciaScreenInfo *) screen->driver;
    int i,j;
    unsigned char Sequencer[6];
    unsigned char CRTC[31];
    unsigned char Graphics[9];
    unsigned char Attribute[21];
    unsigned char MiscOutReg;
    const KdMonitorTiming	*t;
    int	    hactive, hblank, hfp, hbp;
    int	    vactive, vblank, vfp, vbp;
    
    int	    h_active;
    int	    h_total;
    int	    h_display_end;
    int	    h_sync_start;
    int	    h_sync_end;
    int	    h_skew = 0;

    int	    v_active;
    int	    v_total;
    int	    v_sync_start;
    int	    v_sync_end;
    int	    v_skew = 0;

    t = KdFindMode (screen, pcmciaModeSupported);
    
    hactive = t->horizontal;
    hfp = t->hfp;
    hbp = t->hbp;
    hblank = t->hblank;
    
    h_active = hactive;
    h_sync_start = hactive + hfp;
    h_sync_end = hactive + hblank - hbp;
    h_total = hactive + hblank;

    vactive = t->vertical;
    vfp = t->vfp;
    vbp = t->vbp;
    vblank = t->vblank;
    
    v_active = vactive;
    v_sync_start = vactive + vfp;
    v_sync_end = vactive + vblank - vbp;
    v_total = vactive + vblank;

    /*
     * compute correct Hsync & Vsync polarity 
     */

    MiscOutReg = 0x23;
    if (t->hpol == KdSyncNegative)
	MiscOutReg |= 0x40;
    if (t->vpol == KdSyncNegative)
        MiscOutReg |= 0x80;
    
    /*
     * Time Sequencer
     */
    if (pScreenPriv->screen->fb[0].depth == 4)
        Sequencer[0] = 0x02;
    else
        Sequencer[0] = 0x00;
    Sequencer[1] = 0x01;
    Sequencer[2] = 0x0F;
    Sequencer[3] = 0x00;                             /* Font select */
    if (pScreenPriv->screen->fb[0].depth < 8)
        Sequencer[4] = 0x06;                             /* Misc */
    else
        Sequencer[4] = 0x0E;                             /* Misc */
    Sequencer[5] = 0x00;

    /*
     * CRTC Controller
     */
    CRTC[0]  = ((h_total) >> 3) - 5;
    CRTC[1]  = (hactive >> 3) - 1;
    CRTC[2]  = ((min(h_sync_start,h_active)) >> 3) - 1;
    CRTC[3]  = ((((min(h_sync_end,h_total)) >> 3) - 1) & 0x1F) | 0x80;
    i = (((h_skew << 2) + 0x10) & ~0x1F);
    if (i < 0x80)
	CRTC[3] |= i;
    CRTC[4]  = (h_sync_start >> 3);
    CRTC[5]  = (((((min(h_sync_end,h_total)) >> 3) - 1) & 0x20) << 2)
	| (((h_sync_end >> 3)) & 0x1F);
    
    CRTC[6]  = (v_total - 2) & 0xFF;
    CRTC[7]  = (((v_total - 2) & 0x100) >> 8)
	| (((v_active - 1) & 0x100) >> 7)
	| ((v_sync_start & 0x100) >> 6)
	| ((((min(v_sync_start,v_active)) - 1) & 0x100) >> 5)
	| 0x10
	| (((v_total - 2) & 0x200)   >> 4)
	| (((v_active - 1) & 0x200) >> 3)
	| ((v_sync_start & 0x200) >> 2);
    CRTC[8]  = 0x00;
    CRTC[9]  = ((((min(v_sync_start,v_active))-1) & 0x200) >> 4) | 0x40;
    CRTC[10] = 0x00;
    CRTC[11] = 0x00;
    CRTC[12] = 0x00;
    CRTC[13] = 0x00;
    CRTC[14] = 0x00;
    CRTC[15] = 0x00;
    CRTC[16] = v_sync_start & 0xFF;
    CRTC[17] = (v_sync_end & 0x0F) | 0x20;
    CRTC[18] = (v_active - 1) & 0xFF;
    if (pScreenPriv->screen->fb[0].depth == 4)
        CRTC[19] = pScreenPriv->screen->fb[0].pixelStride >> 4;
    else
    if (pScreenPriv->screen->fb[0].depth == 8)
        CRTC[19] = pScreenPriv->screen->fb[0].pixelStride >> 3;
    else
    if (pScreenPriv->screen->fb[0].depth == 16 ||
        pScreenPriv->screen->fb[0].depth == 15)
        CRTC[19] = pScreenPriv->screen->fb[0].pixelStride >> 2;
    CRTC[20] = 0x00;
    CRTC[21] = ((min(v_sync_end,v_active)) - 1) & 0xFF; 
    CRTC[22] = ((min(v_sync_end,v_active)) - 1) & 0xFF;
    if (pScreenPriv->screen->fb[0].depth < 8)
	CRTC[23] = 0xE3;
    else
	CRTC[23] = 0xC3;
    CRTC[24] = 0xFF;
    CRTC[25] = 0x00;
    CRTC[26] = 0x00;
#if 0
    if (!pcmciac->HP)
    	if (mode.Flags & V_INTERLACE) CRTC[26] |= 0x01;
#endif
    if (pcmciac->HP)
    	CRTC[27] = 0x00;
    else
    	CRTC[27] = 0x22;
    CRTC[28] = 0x00;
    CRTC[29] = 0x00;
    CRTC[30] = 0x80;
#if 0
    if (pcmciac->HP)
    	if (mode.Flags & V_INTERLACE) CRTC[30] |= 0x04;
#endif

{
    int nExtBits = 0;
    CARD32 ExtBits;
    CARD32 ExtBitMask = ((1 << nExtBits) - 1) << 6;

    CRTC[3]  = (CRTC[3] & ~0x1F) 
                     | ((((min(h_sync_end,h_total)) >> 3) - 1) & 0x1F);
    CRTC[5]  = (CRTC[5] & ~0x80) 
                     | (((((min(h_sync_end,h_total)) >> 3) - 1) & 0x20) << 2);
    ExtBits        = (((min(h_sync_end,h_total)) >> 3) - 1) & ExtBitMask;

    /* First the horizontal case */
    if ((((min(h_sync_end,h_total)) >> 3) == (h_total >> 3)))
    {
	int i = (CRTC[3] & 0x1F) 
	    | ((CRTC[5] & 0x80) >> 2)
	    | ExtBits;
	if ((i-- > ((((min(h_sync_start,h_active)) >> 3) - 1) 
		       & (0x3F | ExtBitMask)))
	    && ((min(h_sync_end,h_total)) == h_total))
	    i = 0;
	CRTC[3] = (CRTC[3] & ~0x1F) | (i & 0x1F);
	CRTC[5] = (CRTC[5] & ~0x80) | ((i << 2) & 0x80);
	ExtBits = i & ExtBitMask;
    }
}
{
    CARD32 ExtBits;
    CARD32 ExtBitMask = 0;
    /* If width is not known nBits should be 0. In this 
     * case BitMask is set to 0 so we can check for it. */
    CARD32 BitMask = 0;
    int VBlankStart = ((min(v_sync_start,v_active)) - 1) & 0xFF; 
    CRTC[22] = ((min(v_sync_end,v_total)) - 1) & 0xFF;
    ExtBits        = ((min(v_sync_end,v_total)) - 1) & ExtBitMask;

    if ((min(v_sync_end,v_total)) == v_total)
      /* Null top overscan */
    {
	int i = CRTC[22] | ExtBits;
	if (((BitMask && ((i & BitMask) > (VBlankStart & BitMask)))
	     || ((i > VBlankStart)  &&  		/* 8-bit case */
	    ((i & 0x7F) > (VBlankStart & 0x7F)))) &&	/* 7-bit case */
	    !(CRTC[9] & 0x9F))			/* 1 scanline/row */
	    i = 0;
	else
	    i = (i - 1);
	CRTC[22] = i & 0xFF;
	ExtBits = i & 0xFF00;
    }
}

    /*
     * Graphics Display Controller
     */
    Graphics[0] = 0x00;
    Graphics[1] = 0x00;
    Graphics[2] = 0x00;
    Graphics[3] = 0x00;
    Graphics[4] = 0x00;
    if (pScreenPriv->screen->fb[0].depth == 4)
        Graphics[5] = 0x02;
    else
        Graphics[5] = 0x40;
    Graphics[6] = 0x05;   /* only map 64k VGA memory !!!! */
    Graphics[7] = 0x0F;
    Graphics[8] = 0xFF;
  
    Attribute[0]  = 0x00; /* standard colormap translation */
    Attribute[1]  = 0x01;
    Attribute[2]  = 0x02;
    Attribute[3]  = 0x03;
    Attribute[4]  = 0x04;
    Attribute[5]  = 0x05;
    Attribute[6]  = 0x06;
    Attribute[7]  = 0x07;
    Attribute[8]  = 0x08;
    Attribute[9]  = 0x09;
    Attribute[10] = 0x0A;
    Attribute[11] = 0x0B;
    Attribute[12] = 0x0C;
    Attribute[13] = 0x0D;
    Attribute[14] = 0x0E;
    Attribute[15] = 0x0F;
    if (pScreenPriv->screen->fb[0].depth == 4)
        Attribute[16] = 0x81;
    else
        Attribute[16] = 0x41;
    if (pScreenPriv->screen->fb[0].bitsPerPixel == 16)
    	Attribute[17] = 0x00;
    else
    	Attribute[17] = 0xFF;
    Attribute[18] = 0x0F;
    Attribute[19] = 0x00;
    Attribute[20] = 0x00;

    /* Wake up the card */
    if (pcmciac->HP) {
	pcmciaWriteReg(pcmciac, 0x3c3, 0x1);
	pcmciaWriteReg(pcmciac, 0x46e8, 0x10);
    } else {
	pcmciaWriteReg(pcmciac, 0x105, 0x1);
	pcmciaWriteReg(pcmciac, 0x46e8, 0x1f);
	pcmciaWriteReg(pcmciac, 0x102, 0x1);
	pcmciaWriteReg(pcmciac, 0x46e8, 0xf);
	pcmciaWriteReg(pcmciac, 0x3c3, 0x1);
    }

    if (pcmciac->HP) {
    	/* unlock */
	pcmciaWriteIndex(pcmciac, 0x3c4, 0x11, 0x92);
	j = pcmciaReadIndex(pcmciac, 0x3c4, 0xb);
	pcmciaWriteIndex(pcmciac, 0x3c4, 0xe, 0xc2);

	/* switch on dac */
	pcmciaWriteIndex(pcmciac, 0x3d4, 0x29, 0x24);
	/* switch on the accelerator */
	pcmciaWriteIndex(pcmciac, 0x3d4, 0x36, 0x80);

	/* bump up memory clk */
	pcmciaWriteReg(pcmciac, 0x43c6, 0x65);
	pcmciaWriteReg(pcmciac, 0x43c7, 0x00);
    } else {
    	/* unlock */
	pcmciaWriteIndex(pcmciac, 0x3c4, 0x06, 0x12);
    	pcmciaWriteReg(pcmciac, 0x3c2, MiscOutReg);
    }

    /* synchronous reset */
    pcmciaWriteIndex(pcmciac, 0x3c4, 0, 0);

    pcmciaWriteReg(pcmciac, 0x3da, 0x10);

    for (i=0;i<6;i++)
	pcmciaWriteIndex(pcmciac, 0x3c4, i, Sequencer[i]);

    if (pcmciac->HP) { 
    	/* Stick chip into color mode */
	pcmciaWriteIndex(pcmciac, 0x3ce, 0x2f, 0x06);
    	/* Switch on Linear addressing */
	pcmciaWriteIndex(pcmciac, 0x3d4, 0x21, 0x2e);
    } else {
    	/* Stick chip into 8bit access mode - ugh! */
	pcmciaWriteIndex(pcmciac, 0x3c4, 0x0F, 0x20); /* 0x26 ? */
	/* reset mclk */
	pcmciaWriteIndex(pcmciac, 0x3c4, 0x1F, 0);
    }

    pcmciaWriteIndex(pcmciac, 0x3c4, 0, 0x3);

    for (i=0;i<31;i++)
	pcmciaWriteIndex(pcmciac, 0x3d4, i, CRTC[i]);

    for (i=0;i<9;i++)
	pcmciaWriteIndex(pcmciac, 0x3ce, i, Graphics[i]);

    j = pcmciaReadReg(pcmciac, 0x3da);

    for (i=0;i<21;i++) {
	pcmciaWriteReg(pcmciac, 0x3c0, i);
	pcmciaWriteReg(pcmciac, 0x3c0, Attribute[i]);
    }

    j = pcmciaReadReg(pcmciac, 0x3da);
    pcmciaWriteReg(pcmciac, 0x3c0, 0x20);

    j = pcmciaReadReg(pcmciac, 0x3c8);
    j = pcmciaReadReg(pcmciac, 0x3c6);
    j = pcmciaReadReg(pcmciac, 0x3c6);
    j = pcmciaReadReg(pcmciac, 0x3c6);
    j = pcmciaReadReg(pcmciac, 0x3c6);
    switch (pScreenPriv->screen->fb[0].depth) {
	/* This is here for completeness, when/if we ever do 4bpp */
	case 4:
		pcmciaWriteReg(pcmciac, 0x3c6, 0x0);
		if (pcmciac->HP) {
		    pcmciaWriteIndex(pcmciac, 0x3ce, 0x0f, 0x90);
		    pcmciaWriteIndex(pcmciac, 0x3d4, 0x38, 0x00);
		} else
		    pcmciaWriteIndex(pcmciac, 0x3c4, 0x07, 0x00);
		break;
	case 8:
		pcmciaWriteReg(pcmciac, 0x3c6, 0x0);
		if (pcmciac->HP) {
		    pcmciaWriteIndex(pcmciac, 0x3ce, 0x0f, 0x92);
		    pcmciaWriteIndex(pcmciac, 0x3d4, 0x38, 0x00);
		} else
		    pcmciaWriteIndex(pcmciac, 0x3c4, 0x07, 0x01);
		break;
	case 15:
		if (pcmciac->HP) {
		    pcmciaWriteReg(pcmciac, 0x3c6, 0x10);
		    pcmciaWriteIndex(pcmciac, 0x3ce, 0x0f, 0x9a);
		    pcmciaWriteIndex(pcmciac, 0x3d4, 0x38, 0x04);
		} else {
		    pcmciaWriteReg(pcmciac, 0x3c6, 0xC0);
		    pcmciaWriteIndex(pcmciac, 0x3c4, 0x07, 0x03);
		}
		break;
	case 16:
		if (pcmciac->HP) {
		    pcmciaWriteReg(pcmciac, 0x3c6, 0x30);
		    pcmciaWriteIndex(pcmciac, 0x3ce, 0x0f, 0x9a);
		    pcmciaWriteIndex(pcmciac, 0x3d4, 0x38, 0x04);
		} else {
		    pcmciaWriteReg(pcmciac, 0x3c6, 0xC1);
		    pcmciaWriteIndex(pcmciac, 0x3c4, 0x07, 0x03);
		}
		break;
    }
    j = pcmciaReadReg(pcmciac, 0x3c8);

    pcmciaWriteReg(pcmciac, 0x3c6, 0xff);

    for (i=0;i<256;i++)  {
	pcmciaWriteReg(pcmciac, 0x3c8, i);
	pcmciaWriteReg(pcmciac, 0x3c9, i);
	pcmciaWriteReg(pcmciac, 0x3c9, i);
	pcmciaWriteReg(pcmciac, 0x3c9, i);
    }

    /* Set the Clock */
    if (pcmciac->HP) {
	CARD8 a,b;
	int clock = t->clock;
    	if (pScreenPriv->screen->fb[0].bitsPerPixel == 16)
		clock *= 2;
	tridentSetCLK(clock, &a, &b);
	pcmciaWriteReg(pcmciac, 0x43c8, a);
	pcmciaWriteReg(pcmciac, 0x43c9, b);
    } else {
	int num, den;
	unsigned char tmp;
	int clock = t->clock;
    	if (pScreenPriv->screen->fb[0].bitsPerPixel == 16)
		clock *= 2;

	CirrusFindClock(clock, &num, &den);

	tmp = pcmciaReadIndex(pcmciac, 0x3c4, 0x0d);
	pcmciaWriteIndex(pcmciac, 0x3c4, 0x0d, (tmp & 0x80) | num);
	tmp = pcmciaReadIndex(pcmciac, 0x3c4, 0x1d);
	pcmciaWriteIndex(pcmciac, 0x3c4, 0x1d, (tmp & 0xc0) | den);
    }
    pcmciaWriteReg(pcmciac, 0x3c2, MiscOutReg | 0x08);

#if 1
    for (i=1;i<0x3f;i++)
	ErrorF("0x3c4:%02x: 0x%x\n",i,pcmciaReadIndex(pcmciac, 0x3c4, i));

    ErrorF("\n");

    for (i=0;i<0x3f;i++)
	ErrorF("0x3ce:%02x: 0x%x\n",i,pcmciaReadIndex(pcmciac, 0x3ce, i));

    ErrorF("\n");

    for (i=0;i<0x3f;i++)
	ErrorF("0x3d4:%02x: 0x%x\n",i,pcmciaReadIndex(pcmciac, 0x3d4, i));
#endif

    return TRUE;
}

void
pcmciaDisable (ScreenPtr pScreen)
{
}

const CARD8	tridentDPMSModes[4] = {
    0x00,	    /* KD_DPMS_NORMAL */
    0x01,	    /* KD_DPMS_STANDBY */
    0x02,	    /* KD_DPMS_SUSPEND */
    0x03,	    /* KD_DPMS_POWERDOWN */
};

Bool
pcmciaDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    pcmciaCardInfo	*pcmciac = pScreenPriv->card->driver;

    if (pcmciac->HP) {
	pcmciaWriteIndex (pcmciac, 0x3ce, 0x23, tridentDPMSModes[mode]);
    	pcmciaPause ();
    } else {
	/* Voyager */
    } 

    return TRUE;
}

void
pcmciaRestore (KdCardInfo *card)
{
}

void
pcmciaScreenFini (KdScreenInfo *screen)
{
    pcmciaScreenInfo	*pcmcias = (pcmciaScreenInfo *) screen->driver;

    xfree (pcmcias);
    screen->driver = 0;
}

void
pcmciaCardFini (KdCardInfo *card)
{
    pcmciaCardInfo	*pcmciac = card->driver;

    if (pcmciac->cop_base)
	KdUnmapDevice ((void *) pcmciac->cop_base, PCMCIA_COP_SIZE(card));
}

void
pcmciaGetColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    pcmciaCardInfo	*pcmciac = pScreenPriv->card->driver;

    while (ndef--)
    {
        pcmciaWriteReg (pcmciac, 0x3C7, pdefs->pixel);
	pdefs->red = pcmciaReadReg (pcmciac, 0x3C9) << 10;
	pdefs->green = pcmciaReadReg (pcmciac, 0x3C9) << 10;
	pdefs->blue = pcmciaReadReg (pcmciac, 0x3C9) << 10;
	pdefs++;
    }
}

void
pcmciaPutColors (ScreenPtr pScreen, int fb, int ndef, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    pcmciaCardInfo	*pcmciac = pScreenPriv->card->driver;

    while (ndef--)
    {
        pcmciaWriteReg (pcmciac, 0x3C8, pdefs->pixel);
	pcmciaWriteReg (pcmciac, 0x3C9, pdefs->red >> 10);
	pcmciaWriteReg (pcmciac, 0x3C9, pdefs->green >> 10);
	pcmciaWriteReg (pcmciac, 0x3C9, pdefs->blue >> 10);
	pdefs++;
    }
}


KdCardFuncs	pcmciaFuncs = {
    pcmciaCardInit,	    /* cardinit */
    pcmciaScreenInit,	    /* scrinit */
    pcmciaInitScreen,	    /* initScreen */
    pcmciaPreserve,	    /* preserve */
    pcmciaEnable,	    /* enable */
    pcmciaDPMS,	    /* dpms */
    pcmciaDisable,	    /* disable */
    pcmciaRestore,	    /* restore */
    pcmciaScreenFini,	    /* scrfini */
    pcmciaCardFini,	    /* cardfini */
    
    pcmciaCursorInit,	    /* initCursor */
    pcmciaCursorEnable,    /* enableCursor */
    pcmciaCursorDisable,   /* disableCursor */
    pcmciaCursorFini,	    /* finiCursor */
    pcmciaRecolorCursor,   /* recolorCursor */
    
#if 0 /* not yet */
    pcmciaDrawInit,        /* initAccel */
    pcmciaDrawEnable,      /* enableAccel */
    pcmciaDrawSync,	    /* syncAccel */
    pcmciaDrawDisable,     /* disableAccel */
    pcmciaDrawFini,        /* finiAccel */
#else 
    0,
    0,
    0,
    0,
    0,
#endif
    
    pcmciaGetColors,  	    /* getColors */
    pcmciaPutColors,	    /* putColors */
};
