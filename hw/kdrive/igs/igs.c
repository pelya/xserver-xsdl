/*
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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
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

    igsc->frameBuffer = (CARD8 *) KdMapDevice (card->attr.address[0] +
					       IGS_FB,
					       IGS_MEM);
    
    igsc->vga = (VOL8 *) KdMapDevice (card->attr.address[0] +
				      IGS_VGA,
				      64 * 1024);

    igsc->cop = (Cop5xxx *) KdMapDevice (card->attr.address[0] + 
					 IGS_COP_OFFSET,
					 sizeof (Cop5xxx));
    
    igsc->copData = (VOL32 *) KdMapDevice (card->attr.address[0] +
					   IGS_COP_DATA,
					   IGS_COP_DATA_LEN);
    
    igsRegInit (&igsc->igsvga, igsc->vga);

    card->driver = igsc;
    
    return TRUE;
}

Bool
igsModeSupported (KdScreenInfo		*screen,
		 const KdMonitorTiming	*t)
{
    /* make sure the clock isn't too fast */
    if (t->clock > IGS_MAX_CLOCK)
	return FALSE;
    /* width must be a multiple of 16 */
    if (t->horizontal & 0xf)
	return FALSE;
    return TRUE;
}

Bool
igsModeUsable (KdScreenInfo	*screen)
{
    KdCardInfo	    *card = screen->card;
    int		    screen_size;
    int		    pixel_width;
    int		    byte_width;
    int		    fb = 0;
    
    screen_size = 0;
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
    else if (screen->fb[fb].depth >= 12)
    {
	screen->fb[fb].depth = 12;
	screen->fb[fb].bitsPerPixel = 16;
    }
    else
    {
	screen->fb[fb].depth = 8;
	screen->fb[fb].bitsPerPixel = 8;
    }

    byte_width = screen->width * (screen->fb[fb].bitsPerPixel >> 3);
    pixel_width = screen->width;
    screen->fb[fb].pixelStride = pixel_width;
    screen->fb[fb].byteStride = byte_width;
    screen_size += byte_width * screen->height;

    return TRUE;
}

Bool
igsScreenInit (KdScreenInfo *screen)
{
    IgsCardInfo		*igsc = screen->card->driver;
    int			fb = 0;
    IgsScreenInfo	*igss;
    int			screen_size, memory;
    int			pattern_size;
    int			tile_size;
    int			stipple_size;
    int			poffset, boffset;
    const KdMonitorTiming *t;

    if (!screen->width || !screen->height)
    {
	screen->width = 800;
	screen->height = 600;
	screen->rate = 72;
    }
    if (!screen->fb[0].depth)
	screen->fb[0].depth = 8;
    
    t = KdFindMode (screen, igsModeSupported);

    screen->rate = t->rate;
    screen->width = t->horizontal;
    screen->height = t->vertical;

    if (!KdTuneMode (screen, igsModeUsable, igsModeSupported))
    {
	return FALSE;
    }
    
    igss = (IgsScreenInfo *) xalloc (sizeof (IgsScreenInfo));
    if (!igss)
	return FALSE;

    memset (igss, '\0', sizeof (IgsScreenInfo));

    screen_size = screen->fb[fb].byteStride * screen->height;
    memory = IGS_MEM;
    memory -= screen_size;
    if (memory >= 1024)
    {
	igss->cursor_offset = memory - 1024;
#if BITMAP_BIT_ORDER == MSBFirst
	igss->cursor_base = (CARD8 *) KdMapDevice (card->attr.address[0] +
						   igss->cursor_offset,
						   1024);
#else
	igss->cursor_base = igsc->frameBuffer + igss->cursor_offset;
#endif
	memory -= 1024;
    }
    else
	igss->cursor_base = 0;

    tile_size = IgsTileSize(screen->fb[fb].bitsPerPixel) * IGS_NUM_PATTERN;
    stipple_size = IgsStippleSize(screen->fb[fb].bitsPerPixel) * IGS_NUM_PATTERN;
    pattern_size = tile_size + stipple_size;
    if (memory >= pattern_size)
    {
	boffset = screen_size;
	poffset = boffset * 8 / screen->fb[fb].bitsPerPixel;
	igss->tile.offset = poffset;
	igss->tile.base = igsc->frameBuffer + boffset;
	
	boffset = screen_size + tile_size;
	poffset = boffset * 8 / screen->fb[fb].bitsPerPixel;
	igss->stipple.offset = poffset;
	igss->stipple.base = igsc->frameBuffer + boffset;
	
	memory -= pattern_size;
    }
    else
    {
	igss->tile.base = 0;
	igss->stipple.base = 0;
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
    screen->driver = igss;
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
    IgsCardInfo	*igsc = card->driver;
    IgsVga	*igsvga = &igsc->igsvga;
    
    igsSave (igsvga);
}

void
igsSetBlank (IgsVga    *igsvga, Bool blank)
{
    igsSetImm(igsvga, igs_screen_off, blank ? 1 : 0);
}

void
igsSetSync (IgsCardInfo *igsc, int hsync, int vsync)
{
    IgsVga   *igsvga = &igsc->igsvga;
    
    igsSet (igsvga, igs_mexhsyn, hsync);
    igsSet (igsvga, igs_mexvsyn, vsync);
    VgaFlush (&igsvga->card);
}


/*
 * Clock synthesis:
 *
 *  scale = p ? (2 * p) : 1
 *  f_out = f_ref * ((M + 1) / ((N + 1) * scale))
 *
 *  Constraints:
 *
 *  1.	115MHz <= f_ref * ((M + 1) / (N + 1)) <= 260 MHz
 *  2.	N >= 1
 *
 *  Vertical refresh rate = clock / ((hsize + hblank) * (vsize + vblank))
 *  Horizontal refresh rate = clock / (hsize + hblank)
 */
 
/* all in kHz */

void
igsGetClock (int target, int *Mp, int *Np, int *Pp, int maxM, int maxN, int maxP, int minVco)
{
    int	    M, N, P, bestM, bestN;
    int	    f_vco, f_out;
    int	    err, abserr, besterr;

    /*
     * Compute correct P value to keep VCO in range
     */
    for (P = 0; P <= maxP; P++)
    {
	f_vco = target * IGS_SCALE(P);
	if (f_vco >= minVco)
	    break;
    }

    /* M = f_out / f_ref * ((N + 1) * IGS_SCALE(P)); */
    besterr = target;
    for (N = 1; N <= maxN; N++)
    {
	M = ((target * (N + 1) * IGS_SCALE(P) + (IGS_CLOCK_REF/2)) + IGS_CLOCK_REF/2) / IGS_CLOCK_REF - 1;
	if (0 <= M && M <= maxM)
	{
	    f_out = IGS_CLOCK(M,N,P);
	    err = target - f_out;
	    if (err < 0)
		err = -err;
	    if (err < besterr)
	    {
		besterr = err;
		bestM = M;
		bestN = N;
	    }
	}
    }
    *Mp = bestM;
    *Np = bestN;
    *Pp = P;
}

Bool
igsEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdCardInfo	    *card = pScreenPriv->card;
    KdScreenInfo    *screen = pScreenPriv->screen;
    IgsCardInfo	    *igsc = card->driver;
    IgsVga	    *igsvga = &igsc->igsvga;
    const KdMonitorTiming *t;
    int	    hactive, hblank, hfp, hbp;
    int	    vactive, vblank, vfp, vbp;
    int	    hsize;
    int	    fb = 0;
    int	    m, n, r;
    int	    h_total;
    int	    h_display_end;
    int	    h_blank_start;
    int	    h_blank_end;
    int	    h_sync_start;
    int	    h_sync_end;
    int	    h_screen_off;
    int	    v_total;
    int	    v_retrace_start;
    int	    v_retrace_end;
    int	    v_display_end;
    int	    v_blank_start;
    int	    v_blank_end;
    int	    offset;
    int	    num_fetch;
    int	    m_m, m_n, m_r;
    
    
    igsSetBlank (igsvga, TRUE);
    
    t = KdFindMode (screen, igsModeSupported);
    
    igsGetClock (t->clock, &m, &n, &r, 2047, 255, 7, IGS_MIN_VCO);

    /*
     * Set the chip so that 0x400000 is a big-endian frame buffer
     * with the correct byte swapping enabled
     */
    igsSet (igsvga, igs_biga22force, 0);
    igsSet (igsvga, igs_biga22en, 1);
    igsSet (igsvga, igs_biga24en, 1);
    /*
     * Enable 8-bit DACs
     */
    igsSet (igsvga, igs_rampwdn, 0);
    igsSet (igsvga, igs_dac6_8, 1);
    igsSet (igsvga, igs_dacpwdn, 0);
    /*
     * Set overscan to black
     */
    igsSet (igsvga, igs_overscan_red, 0x00);
    igsSet (igsvga, igs_overscan_green, 0x00);
    igsSet (igsvga, igs_overscan_blue, 0x00);
    /*
     * Enable PCI retries
     */
    igsSet (igsvga, igs_iow_retry, 1);
    igsSet (igsvga, igs_mw_retry, 1);
    igsSet (igsvga, igs_mr_retry, 1);
    igsSet (igsvga, igs_pci_burst_write, 1);
    igsSet (igsvga, igs_pci_burst_read, 1);
    /*
     * Set FIFO
     */
    igsSet (igsvga, igs_memgopg, 1);
    igsSet (igsvga, igs_memr2wpg, 0);
    igsSet (igsvga, igs_crtff16, 0);
    igsSet (igsvga, igs_fifomust, 0xff);
    igsSet (igsvga, igs_fifogen, 0xff);
    /*
     * Enable CRT reg access
     */
    igsSetImm (igsvga, igs_ena_vr_access, 1);
    igsSetImm (igsvga, igs_crt_protect, 0);

    hfp = t->hfp;
    hbp = t->hbp;
    hblank = t->hblank;
    hactive = t->horizontal;
    offset = screen->fb[0].byteStride;
    
    vfp = t->vfp;
    vbp = t->vbp;
    vblank = t->vblank;
    vactive = t->vertical;

    /*
     * Compute character lengths for horizontal timing values
     */
    hactive = screen->width / 8;
    hblank /= 8;
    hfp /= 8;
    hbp /= 8;
    offset /= 8;
    
    switch (screen->fb[fb].bitsPerPixel) {
    case 8:
	igsSet (igsvga, igs_overscan_red, pScreen->blackPixel);
	igsSet (igsvga, igs_overscan_green, pScreen->blackPixel);
	igsSet (igsvga, igs_overscan_blue, pScreen->blackPixel);
	igsSet (igsvga, igs_bigswap, IGS_BIGSWAP_8);
	igsSet (igsvga, igs_mode_sel, IGS_MODE_8);
	igsSet (igsvga, igs_ramdacbypass, 0);
	break;
    case 16:
	igsSet (igsvga, igs_bigswap, IGS_BIGSWAP_16);
	igsSet (igsvga, igs_ramdacbypass, 1);
	switch (screen->fb[fb].depth) {
	case 12:
	    igsSet (igsvga, igs_mode_sel, IGS_MODE_4444);
	    break;
	case 15:
	    igsSet (igsvga, igs_mode_sel, IGS_MODE_5551);
	    break;
	case 16:
	    igsSet (igsvga, igs_mode_sel, IGS_MODE_565);
	    break;
	}
	break;
    case 24:
	igsSet (igsvga, igs_ramdacbypass, 1);
	igsSet (igsvga, igs_bigswap, IGS_BIGSWAP_8);
	igsSet (igsvga, igs_mode_sel, IGS_MODE_888);
	break;
    case 32:
	igsSet (igsvga, igs_ramdacbypass, 1);
	igsSet (igsvga, igs_bigswap, IGS_BIGSWAP_32);
	igsSet (igsvga, igs_mode_sel, IGS_MODE_8888);
	break;
    }

    /*
     * Compute horizontal register values from timings
     */
    h_total = hactive + hblank - 5;
    h_display_end = hactive - 1;
   
    h_sync_start = hactive + hfp;
    h_sync_end = hactive + hblank - hbp;
    /* 
     * pad the blank values narrow a bit and use the border_select to
     * eliminate the remaining border; don't know why, but it doesn't
     * work in the documented fashion
     */
    h_blank_start = hactive - 1;
    h_blank_end = hactive + hblank - 1 - 1;

    num_fetch = (t->horizontal * screen->fb[fb].bitsPerPixel / 64) + 1;
    
    v_total = vactive + vblank - 2;
    v_display_end = vactive - 1;
    
    v_blank_start = vactive - 1;
    v_blank_end = v_blank_start + vblank - 1;
    
    v_retrace_start = vactive + vfp;
    v_retrace_end = vactive + vblank - vbp;

#if 0
#define chk(a,b,c)  fprintf (stderr, "%20.20s: BIOS %6d X %6d\n", a, igsGet(igsvga, b), c);

    chk("h_total", igs_h_total, h_total);
    chk("h_display_end", igs_h_de_end, h_display_end);
    chk("h_sync_start", igs_h_rstart, h_sync_start);
    chk("h_sync_end", igs_h_rend, h_sync_end&0x1f);
    chk("h_blank_start", igs_h_bstart, h_blank_start);
    chk("h_blank_end", igs_h_bend, h_blank_end&0x3f);
    chk("offset", igs_offset, offset);
    chk("num_fetch", igs_num_fetch, num_fetch);
    
    chk("v_total", igs_v_total, v_total);
    chk("v_display_end", igs_v_de_end, v_display_end);
    chk("v_blank_start", igs_v_bstart, v_blank_start);
    chk("v_blank_end", igs_v_bend, v_blank_end&0xf);
    chk("v_retrace_start", igs_v_rstart, v_retrace_start);
    chk("v_retrace_end", igs_v_rend, v_retrace_end&0xf);
    chk("vclk_m", igs_vclk_m, m);
    chk("vclk_n", igs_vclk_n, n);
    chk("vclk_p", igs_vclk_p, r);

    fprintf (stderr, "%20.20s: BIOS %6d X %6d\n", "vclk",
	     IGS_CLOCK(igsGet(igsvga,igs_vclk_m),
		       igsGet(igsvga,igs_vclk_n),
		       igsGet(igsvga,igs_vclk_p)),
	     IGS_CLOCK(m,n,r));
#endif
    igsSet (igsvga, igs_h_total, h_total);
    igsSet (igsvga, igs_h_de_end, h_display_end);
    igsSet (igsvga, igs_h_rstart, h_sync_start);
    igsSet (igsvga, igs_h_rend, h_sync_end);
    igsSet (igsvga, igs_h_bstart, h_blank_start);
    igsSet (igsvga, igs_h_bend, h_blank_end);
    igsSet (igsvga, igs_offset, offset);
    igsSet (igsvga, igs_num_fetch, num_fetch);
    
    igsSet (igsvga, igs_v_total, v_total);
    igsSet (igsvga, igs_v_de_end, v_display_end);
    igsSet (igsvga, igs_v_bstart, v_blank_start);
    igsSet (igsvga, igs_v_bend, v_blank_end&0xf);
    igsSet (igsvga, igs_v_rstart, v_retrace_start);
    igsSet (igsvga, igs_v_rend, v_retrace_end&0xf);

    igsSet (igsvga, igs_vclk_m, m);
    igsSet (igsvga, igs_vclk_n, n);
    igsSet (igsvga, igs_vclk_p, r);
    igsSet (igsvga, igs_vfsel, IGS_CLOCK(m, n, 0) >= 180000);
    VgaFlush (&igsvga->card);

    igsSetImm (igsvga, igs_frqlat, 0);
    igsSetImm (igsvga, igs_frqlat, 1);
    igsSetImm (igsvga, igs_frqlat, 0);
    
    igsSetBlank (igsvga, FALSE);
#if 0
#define dbg(a,b) fprintf(stderr, "%20.20s = 0x%x\n", a, igsGet(igsvga,b))
    
#include "reg.dbg"

    {
	VGA16	reg;
	char	buf[128];

	for (reg = 0; reg < IGS_NREG; reg++)
	    fprintf(stderr, "%20.20s = 0x%02x\n", igsRegName(buf, reg),
		    VgaFetch (&igsvga->card, reg));
    }
#endif
    return TRUE;
}

Bool
igsDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    IgsCardInfo	*igsc = pScreenPriv->card->driver;
    IgsVga	*igsvga = &igsc->igsvga;
    
    switch (mode) {
    case KD_DPMS_NORMAL:
	igsSetSync (igsc, 0, 0);
	igsSetBlank (igsvga, FALSE);
	break;
    case KD_DPMS_STANDBY:
	igsSetBlank (igsvga, TRUE);
	igsSetSync (igsc, 1, 0);
	break;
    case KD_DPMS_SUSPEND:
	igsSetBlank (igsvga, TRUE);
	igsSetSync (igsc, 0, 1);
	break;
    case KD_DPMS_POWERDOWN:
	igsSetBlank (igsvga, TRUE);
	igsSetSync (igsc, 1, 1);
	break;
    }
    return TRUE;
}

void
igsDisable (ScreenPtr pScreen)
{
}

void
igsRestore (KdCardInfo *card)
{
    IgsCardInfo	*igsc = card->driver;
    IgsVga	*igsvga = &igsc->igsvga;
    
    igsReset (igsvga);
}
    
void
igsScreenFini (KdScreenInfo *screen)
{
    IgsScreenInfo   *igss = (IgsScreenInfo *) screen->driver;

#if BITMAP_BIT_ORDER == MSBFirst
    if (igss->cursor_base)
	KdUnmapDevice ((void *) igss->cursor_base, 1024);
#endif
    xfree (igss);
    screen->driver = 0;
}
    
void
igsCardFini (KdCardInfo *card)
{
    IgsCardInfo	*igsc = card->driver;

    if (igsc->copData)
	KdUnmapDevice ((void *) igsc->copData, IGS_COP_DATA_LEN);
    if (igsc->cop)
	KdUnmapDevice ((void *) igsc->cop, sizeof (Cop5xxx));
    if (igsc->vga)
	KdUnmapDevice ((void *) igsc->vga, 64 * 1024);
    if (igsc->frameBuffer)
	KdUnmapDevice (igsc->frameBuffer, IGS_MEM);
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
    
    igsCursorInit,    	    /* initCursor */
    igsCursorEnable,	    /* enableCursor */
    igsCursorDisable,	    /* disableCursor */
    igsCursorFini,    	    /* finiCursor */
    0,			    /* recolorCursor */
    
    igsDrawInit,    	    /* initAccel */
    igsDrawEnable,    	    /* enableAccel */
    igsDrawSync,	    /* drawSync */
    igsDrawDisable,    	    /* disableAccel */
    igsDrawFini,    	    /* finiAccel */
    
    igsGetColors,    	    /* getColors */
    igsPutColors, 	    /* putColors */
};
