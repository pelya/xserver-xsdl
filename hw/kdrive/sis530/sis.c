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
#include "sis.h"

#define MAX_FB_SIZE	(4096 * 1024)

#define MMIO_SIZE	(64 * 1024)

int sisMemoryTable[8] = {
    1, 2, 4, 0, 0, 2, 4, 8
};

Bool
sisCardInit (KdCardInfo *card)
{
    SisCardInfo	    *sisc;
    SisPtr	    sis;
    int		    size;
    CARD8	    *registers;
    CARD8	    *temp_buffer;
    CARD8	    save_sr5;

    sisc = (SisCardInfo *) xalloc (sizeof (SisCardInfo));
    if (!sisc)
	goto bail0;
    
    sisc->io_base = card->attr.io;
    /*
     * enable access to SiS ports (no MMIO available) 
     */
    iopl(3);
    save_sr5 = GetSrtc(sisc,0x5);
    if (save_sr5 != 0x21)
	save_sr5 = 0x86;
    PutSrtc(sisc,0x5,0x86);
#if 0
    {
	int	i;

	for (i = 0; i <= 0x3f; i++)
	    fprintf (stderr, "SR%02x = %02x\n", i, GetSrtc(sisc,i));
    }
#endif
    sisc->memory = sisMemoryTable[GetSrtc(sisc,0xc)&0x7] * 1024 * 1024;

    PutSrtc(sisc,0x5,save_sr5);
    
    if (!sisc->memory)
    {
	ErrorF ("Can't detect SiS530 frame buffer\n");
	goto bail1;
    }

    /*
     * Map frame buffer and MMIO registers
     */
    sisc->frameBuffer = KdMapDevice (card->attr.address[0], sisc->memory);
    if (!sisc->frameBuffer)
	goto bail1;
    
    sisc->registers = KdMapDevice (card->attr.address[1], MMIO_SIZE);
    if (!sisc->registers)
	goto bail2;
    
    /*
     * Offset from base of MMIO to registers
     */
    sisc->sis = (SisPtr) (sisc->registers + SIS_MMIO_OFFSET);
    sisc->cpu_bitblt = (VOL32 *) sisc->registers;

    card->driver = sisc;
    
    return TRUE;
bail2:
    KdUnmapDevice (sisc->frameBuffer, sisc->memory);
bail1:
    xfree (sisc);
bail0:
    return FALSE;
}

Bool
sisModeSupported (KdScreenInfo *screen, const KdMonitorTiming *t)
{
    if (t->horizontal != 1600 &&
	t->horizontal != 1280 &&
	t->horizontal != 1152 &&
	t->horizontal != 1024 &&
	t->horizontal != 800 &&
	t->horizontal != 640)
	return FALSE;
    return TRUE;
}

Bool
sisModeUsable (KdScreenInfo *screen)
{
    KdCardInfo	    *card = screen->card;
    SisCardInfo	    *sisc = (SisCardInfo *) card->driver;
    SisScreenInfo   *siss;
    int		    i;
    KdMonitorTiming *t;
    CARD32	    memory;
    int		    byte_width, pixel_width, screen_size;
    
    if (screen->fb[0].depth >= 24)
    {
	screen->fb[0].depth = 24;
	screen->fb[0].bitsPerPixel = 24;
	screen->dumb = TRUE;
    }
    else if (screen->fb[0].depth >= 16)
    {
	screen->fb[0].depth = 16;
	screen->fb[0].bitsPerPixel = 16;
    }
    else if (screen->fb[0].depth >= 15)
    {
	screen->fb[0].depth = 15;
	screen->fb[0].bitsPerPixel = 16;
    }
    else
    {
	screen->fb[0].depth = 8;
	screen->fb[0].bitsPerPixel = 8;
    }
    byte_width = screen->width * (screen->fb[0].bitsPerPixel >> 3);
    pixel_width = screen->width;
    screen->fb[0].pixelStride = pixel_width;
    screen->fb[0].byteStride = byte_width;

    screen_size = byte_width * screen->height;

    return screen_size <= sisc->memory;
}

Bool
sisScreenInit (KdScreenInfo *screen)
{
    KdCardInfo	    *card = screen->card;
    SisCardInfo	    *sisc = (SisCardInfo *) card->driver;
    SisScreenInfo   *siss;
    int		    i;
    const KdMonitorTiming *t;
    CARD32	    memory;
    int		    byte_width, pixel_width, screen_size;

    siss = (SisScreenInfo *) xalloc (sizeof (SisScreenInfo));
    if (!siss)
	return FALSE;

    memset (siss, '\0', sizeof (SisScreenInfo));

    if (!screen->width || !screen->height)
    {
	screen->width = 800;
	screen->height = 600;
	screen->rate = 72;
    }
    if (!screen->fb[0].depth)
	screen->fb[0].depth = 8;
    
    t = KdFindMode (screen, sisModeSupported);
    
    screen->rate = t->rate;
    screen->width = t->horizontal;
    screen->height = t->vertical;
    
    if (!KdTuneMode (screen, sisModeUsable, sisModeSupported))
    {
	xfree (sisc);
	return FALSE;
    }

    memory = sisc->memory - screen_size;
    
    screen->fb[0].frameBuffer = sisc->frameBuffer;
    
    /*
     * Cursor lives in the last 16k of memory
     */
    if (memory >= 16384 && !screen->softCursor)
    {
	siss->cursor_base = sisc->frameBuffer + (sisc->memory - 16384);
	siss->cursor_off = siss->cursor_base - sisc->frameBuffer;
	memory -= 16384;
    }
    else
    {
	screen->softCursor = TRUE;
	siss->cursor_base = 0;
	siss->cursor_off = 0;
    }

    if (memory > 8192)
    {
	siss->expand = screen->fb[0].frameBuffer + screen_size;
	siss->expand_off = siss->expand - sisc->frameBuffer;
	siss->expand_len = memory;
	memory = 0;
    }
    else
    {
	siss->expand = 0;
	siss->expand_len = 0;
    }
    
    switch (screen->fb[0].depth) {
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
    case 24:
	screen->fb[0].visuals = (1 << TrueColor);
	screen->fb[0].blueMask  = 0x0000ff;
	screen->fb[0].greenMask = 0x00ff00;
	screen->fb[0].redMask   = 0xff0000;
	break;
    }
    
    screen->driver = siss;
    
    return TRUE;
}

static void
_sisGetCrtc (SisCardInfo *sisc, SisCrtc *crtc)
{
    crtc->misc_output			= _sisInb(sisc->io_base+0x4c);
    crtc->h_total_0_7			= GetCrtc (sisc, 0x00);
    crtc->h_display_end_0_7		= GetCrtc (sisc, 0x01);
    crtc->h_blank_start_0_7		= GetCrtc (sisc, 0x02);
    crtc->_h_blank_end			= GetCrtc (sisc, 0x03);
    crtc->h_sync_start_0_7		= GetCrtc (sisc, 0x04);
    crtc->_h_sync_end			= GetCrtc (sisc, 0x05);
    crtc->v_total_0_7			= GetCrtc (sisc, 0x06);
    crtc->crtc_overflow			= GetCrtc (sisc, 0x07);
    crtc->preset_row_scan		= GetCrtc (sisc, 0x08);
    crtc->_max_scan_line		= GetCrtc (sisc, 0x09);
    crtc->cursor_start			= GetCrtc (sisc, 0x0a);
    crtc->cursor_end			= GetCrtc (sisc, 0x0a);
    crtc->start_address_8_15		= GetCrtc (sisc, 0x0c);
    crtc->start_address_0_7		= GetCrtc (sisc, 0x0d);
    crtc->text_cursor_15_8		= GetCrtc (sisc, 0x0e);
    crtc->text_cursor_7_0		= GetCrtc (sisc, 0x0f);
    crtc->v_retrace_start_0_7		= GetCrtc (sisc, 0x10);
    crtc->_v_retrace_end		= GetCrtc (sisc, 0x11);
    crtc->v_display_end_0_7		= GetCrtc (sisc, 0x12);
    crtc->screen_off_0_7	    	= GetCrtc (sisc, 0x13);
    crtc->_underline_location		= GetCrtc (sisc, 0x14);
    crtc->v_blank_start_0_7		= GetCrtc (sisc, 0x15);
    crtc->v_blank_end_0_7		= GetCrtc (sisc, 0x16);
    crtc->crtc_mode			= GetCrtc (sisc, 0x17);
    
    crtc->line_compare_0_7		= GetCrtc (sisc, 0x18);
    
    crtc->mode_control			= GetArtc (sisc, 0x10);
    crtc->screen_border_color		= GetArtc (sisc, 0x11);
    crtc->enable_color_plane		= GetArtc (sisc, 0x12);
    crtc->horizontal_pixel_pan		= GetArtc (sisc, 0x13);

    crtc->mode_register			= GetGrtc (sisc, 0x5);
    crtc->misc_register			= GetGrtc (sisc, 0x6);
    crtc->color_dont_care		= GetGrtc (sisc, 0x7);
    
    crtc->clock_mode			= GetSrtc (sisc, 0x1);
    crtc->color_plane_w_enable		= GetSrtc (sisc, 0x2);
    crtc->memory_mode			= GetSrtc (sisc, 0x4);
    
    crtc->graphics_mode			= GetSrtc (sisc, 0x6);
    crtc->misc_control_0    		= GetSrtc (sisc, 0x7);
    crtc->crt_cpu_threshold_control_0	= GetSrtc (sisc, 0x8);
    crtc->crt_cpu_threshold_control_1	= GetSrtc (sisc, 0x9);
    crtc->extended_crt_overflow		= GetSrtc (sisc, 0xa);
    crtc->misc_control_1    		= GetSrtc (sisc, 0xb);
    crtc->misc_control_2    		= GetSrtc (sisc, 0xc);
    
    crtc->ddc_and_power_control		= GetSrtc (sisc, 0x11);
    crtc->extended_horizontal_overflow	= GetSrtc (sisc, 0x12);
    crtc->extended_clock_generator    	= GetSrtc (sisc, 0x13);
    crtc->cursor_0_red			= GetSrtc (sisc, 0x14);
    crtc->cursor_0_green    		= GetSrtc (sisc, 0x15);
    crtc->cursor_0_blue			= GetSrtc (sisc, 0x16);
    crtc->cursor_1_red			= GetSrtc (sisc, 0x17);
    crtc->cursor_1_green    		= GetSrtc (sisc, 0x18);
    crtc->cursor_1_blue			= GetSrtc (sisc, 0x19);
    crtc->cursor_h_start_0_7		= GetSrtc (sisc, 0x1a);
    crtc->cursor_h_start_1    		= GetSrtc (sisc, 0x1b);
    crtc->cursor_h_preset_0_5    	= GetSrtc (sisc, 0x1c);
    crtc->cursor_v_start_0_7		= GetSrtc (sisc, 0x1d);
    crtc->cursor_v_start_1    		= GetSrtc (sisc, 0x1e);
    crtc->cursor_v_preset_0_5 		= GetSrtc (sisc, 0x1f);
    crtc->linear_base_19_26		= GetSrtc (sisc, 0x20);
    crtc->linear_base_1			= GetSrtc (sisc, 0x21);

    crtc->graphics_engine_0		= GetSrtc (sisc, 0x26);
    crtc->graphics_engine_1		= GetSrtc (sisc, 0x27);
    crtc->internal_mclk_0		= GetSrtc (sisc, 0x28);
    crtc->internal_mclk_1		= GetSrtc (sisc, 0x29);
    crtc->internal_vclk_0		= GetSrtc (sisc, 0x2A);
    crtc->internal_vclk_1		= GetSrtc (sisc, 0x2B);

    crtc->misc_control_7		= GetSrtc (sisc, 0x38);
    
    crtc->misc_control_11		= GetSrtc (sisc, 0x3E);
    crtc->misc_control_12		= GetSrtc (sisc, 0x3F);
}

static void
_sisSetBlank (SisCardInfo *sisc, Bool blank)
{
    CARD8   clock;
    
    clock = GetSrtc (sisc, 0x01);
    if (blank)
	clock |= 0x20;
    else
	clock &= ~0x20;
    PutSrtc (sisc, 0x01, clock);
}

static void
_sisSetCrtc (SisCardInfo *sisc, SisCrtc *crtc)
{
    _sisSetBlank (sisc, TRUE);
    PutCrtc (sisc, 0x00, crtc->h_total_0_7);
    PutCrtc (sisc, 0x01, crtc->h_display_end_0_7);
    PutCrtc (sisc, 0x02, crtc->h_blank_start_0_7);
    PutCrtc (sisc, 0x03, crtc->_h_blank_end);
    PutCrtc (sisc, 0x04, crtc->h_sync_start_0_7);
    PutCrtc (sisc, 0x05, crtc->_h_sync_end);
    PutCrtc (sisc, 0x06, crtc->v_total_0_7);
    PutCrtc (sisc, 0x07, crtc->crtc_overflow);
    PutCrtc (sisc, 0x08, crtc->preset_row_scan);
    PutCrtc (sisc, 0x09, crtc->_max_scan_line);
    PutCrtc (sisc, 0x0a, crtc->cursor_start);
    PutCrtc (sisc, 0x0b, crtc->cursor_end);
    PutCrtc (sisc, 0x0c, crtc->start_address_8_15);
    PutCrtc (sisc, 0x0d, crtc->start_address_0_7);
    PutCrtc (sisc, 0x0e, crtc->text_cursor_15_8);
    PutCrtc (sisc, 0x0f, crtc->text_cursor_7_0);
    PutCrtc (sisc, 0x10, crtc->v_retrace_start_0_7);
    PutCrtc (sisc, 0x11, crtc->_v_retrace_end);
    PutCrtc (sisc, 0x12, crtc->v_display_end_0_7);
    PutCrtc (sisc, 0x13, crtc->screen_off_0_7);
    PutCrtc (sisc, 0x14, crtc->_underline_location);
    PutCrtc (sisc, 0x15, crtc->v_blank_start_0_7);
    PutCrtc (sisc, 0x16, crtc->v_blank_end_0_7);
    PutCrtc (sisc, 0x17, crtc->crtc_mode);
    PutCrtc (sisc, 0x18, crtc->line_compare_0_7);
    
    PutArtc (sisc, 0x10, crtc->mode_control);
    PutArtc (sisc, 0x11, crtc->screen_border_color);
    PutArtc (sisc, 0x12, crtc->enable_color_plane);
    PutArtc (sisc, 0x13, crtc->horizontal_pixel_pan);

    PutGrtc (sisc, 0x5, crtc->mode_register);
    PutGrtc (sisc, 0x6, crtc->misc_register);
    PutGrtc (sisc, 0x7, crtc->color_dont_care);
    
    PutSrtc (sisc, 0x1, crtc->clock_mode | 0x20);
    PutSrtc (sisc, 0x2, crtc->color_plane_w_enable);
    PutSrtc (sisc, 0x4, crtc->memory_mode);
    
    PutSrtc (sisc, 0x6, crtc->graphics_mode);
    PutSrtc (sisc, 0x7, crtc->misc_control_0);
    PutSrtc (sisc, 0x8, crtc->crt_cpu_threshold_control_0);
    PutSrtc (sisc, 0x9, crtc->crt_cpu_threshold_control_1);
    PutSrtc (sisc, 0xa, crtc->extended_crt_overflow);
    PutSrtc (sisc, 0xb, crtc->misc_control_1);
    PutSrtc (sisc, 0xc, crtc->misc_control_2);
    
    PutSrtc (sisc, 0x11, crtc->ddc_and_power_control);
    PutSrtc (sisc, 0x12, crtc->extended_horizontal_overflow);
    PutSrtc (sisc, 0x13, crtc->extended_clock_generator);
    PutSrtc (sisc, 0x14, crtc->cursor_0_red);
    PutSrtc (sisc, 0x15, crtc->cursor_0_green);
    PutSrtc (sisc, 0x16, crtc->cursor_0_blue);
    PutSrtc (sisc, 0x17, crtc->cursor_1_red);
    PutSrtc (sisc, 0x18, crtc->cursor_1_green);
    PutSrtc (sisc, 0x19, crtc->cursor_1_blue);
    PutSrtc (sisc, 0x1a, crtc->cursor_h_start_0_7);
    PutSrtc (sisc, 0x1b, crtc->cursor_h_start_1);
    PutSrtc (sisc, 0x1c, crtc->cursor_h_preset_0_5);
    PutSrtc (sisc, 0x1d, crtc->cursor_v_start_0_7);
    PutSrtc (sisc, 0x1e, crtc->cursor_v_start_1);
    PutSrtc (sisc, 0x1f, crtc->cursor_v_preset_0_5);
    PutSrtc (sisc, 0x20, crtc->linear_base_19_26);
    PutSrtc (sisc, 0x21, crtc->linear_base_1);

    PutSrtc (sisc, 0x26, crtc->graphics_engine_0);
    PutSrtc (sisc, 0x27, crtc->graphics_engine_1);
    PutSrtc (sisc, 0x28, crtc->internal_mclk_0);
    PutSrtc (sisc, 0x29, crtc->internal_mclk_1);
    PutSrtc (sisc, 0x2A, crtc->internal_vclk_0);
    PutSrtc (sisc, 0x2B, crtc->internal_vclk_1);

    PutSrtc (sisc, 0x38, crtc->misc_control_7);
    
    PutSrtc (sisc, 0x3E, crtc->misc_control_11);
    PutSrtc (sisc, 0x3F, crtc->misc_control_12);
    
#if 0
    PutCrtc (sisc, 0x5b, 0x27);
    PutCrtc (sisc, 0x5c, 0xe1);
    PutCrtc (sisc, 0x5d, 0x00);

    PutSrtc (sisc, 0x5a, 0xe6);
    PutSrtc (sisc, 0x5d, 0xa1);
    PutSrtc (sisc, 0x9a, 0xe6);
    PutSrtc (sisc, 0x9d, 0xa1);
    PutSrtc (sisc, 0xda, 0xe6);
    PutSrtc (sisc, 0xdd, 0x6c);
#endif
    
    _sisOutb(crtc->misc_output, sisc->io_base+0x42);
    
    outw (0x3c4, 0x0100);
    outw (0x3c4, 0x0300);
    
    _sisSetBlank (sisc, FALSE);
}

CARD8
_sisReadIndexRegister (CARD32 base, CARD8 index)
{
    CARD8   ret;

    _sisOutb (index, base);
    ret = _sisInb (base+1);
    return ret;
}

void
_sisWriteIndexRegister (CARD32 base, CARD8 index, CARD8 value)
{
    _sisOutb (index, base);
    _sisOutb (value, base+1);
}

CARD8
_sisReadArtc (CARD32 base, CARD8 index)
{
    CARD8   ret;
    
    _sisInb (base+0x1a);
    _sisOutb (index,base);
    ret = _sisInb (base+1);
    _sisInb (base+0x1a);
    _sisOutb (0x20,base);
    return ret;
}

void
_sisWriteArtc (CARD32 base, CARD8 index, CARD8 value)
{
    _sisInb (base+0x1a);
    _sisOutb (index|0x20,base);
    _sisOutb (value,base);
    _sisInb (base+0x1a);
    _sisOutb (0x20,base);
}

void
sisPreserve (KdCardInfo *card)
{
    SisCardInfo	*sisc = card->driver;
    CARD8	*r = sisc->registers;
    int		a, i, l;
    CARD8	line[16];
    CARD8	prev[16];
    BOOL	gotone;
    
    sisc->save.sr5 = GetSrtc(sisc,0x5);
    if (sisc->save.sr5 != 0x21)
	sisc->save.sr5 = 0x86;
    /* unlock extension registers */
    PutSrtc(sisc,0x5,0x86);
    /* unlock CRTC registers */
    PutCrtc(sisc,0x11,GetCrtc(sisc,0x11)&~0x80);
    /* enable vga */
    _sisOutb(0x1,sisc->io_base+0x43);
    
    /* enable MMIO access to registers */
    sisc->save.srb = GetSrtc(sisc,0xb);
    PutSrtc(sisc, 0xb, sisc->save.srb | 0x60);
    _sisGetCrtc (sisc, &sisc->save.crtc);
    memcpy (sisc->save.text_save, sisc->frameBuffer, SIS_TEXT_SAVE);
}

Bool
sisEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo    *screen = pScreenPriv->screen;
    KdCardInfo	    *card = pScreenPriv->card;
    SisCardInfo	    *sisc = card->driver;
    SisScreenInfo   *siss = screen->driver;
    const KdMonitorTiming *t;
    SisCrtc	    crtc;
    unsigned long   pixel;
    
    int	    hactive;
    int	    hblank;
    int	    hfp;
    int	    hbp;

    int	    vactive;
    int	    vblank;
    int	    vfp;
    int	    vbp;

    int	    h_total;
    int	    h_display_end;
    int	    h_blank_start;
    int	    h_blank_end;
    int	    h_sync_start;
    int	    h_sync_end;
    int	    h_screen_off;

    int	    h_adjust;

    int	    v_total;
    int	    v_retrace_start;
    int	    v_retrace_end;
    int	    v_display_end;
    int	    v_blank_start;
    int	    v_blank_end;

    crtc = sisc->save.crtc;
    
    t = KdFindMode (screen, sisModeSupported);
    
    /* CR9 */
    crtc.max_scan_line = 0;
    
    /* CRA */
    crtc.cursor_start = 0;

    /* CRB */
    crtc.cursor_end = 0;
    
    /* CRE */
    crtc.text_cursor_15_8 = 0;

    /* CRF */
    crtc.text_cursor_7_0 = 0;
    
    /* CR11 */
    crtc.disable_v_retrace_int = 1;
    
    /* CR14 */
    crtc.underline_location = 0;
    crtc.count_by_four = 0;
    crtc.doubleword_mode = 1;
    
    /* 3CC/3C2 */
    crtc.io_address_select = 1;
    crtc.display_ram_enable = 1;
    crtc.clock_select = 3;
    
    /* SR1 */
    crtc.clock_mode = 0;
    crtc.dot_clock_8_9 = 1;

    /* SR2 */
    crtc.color_plane_w_enable = 0xf;

    /* SR4 */
    crtc.memory_mode = 0;
    crtc.chain_4_enable = 1;
    crtc.odd_even_disable = 1;
    crtc.extended_memory_sz = 1;

    /* SR6 */
    crtc.graphics_mode_linear = 1;
    crtc.enhanced_graphics_mode = 1;
    
    /* SR9 */
    crtc.crt_cpu_threshold_control_1 = 0;

    /* SRB */
#if 0
    crtc.cpu_bitblt_enable = 1;
#endif
    crtc.memory_mapped_mode = 3;
    
    /* SRC */
    crtc.graphics_mode_32bit_enable = 1;
    crtc.read_ahead_enable = 1;
    
    /* SR11 */
    crtc.acpi_enable = 0;
    crtc.kbd_cursor_activate = 0;
    crtc.video_memory_activate = 0;
    crtc.vga_standby = 0;
    crtc.vga_suspend = 0;
    
    crtc.cursor_0_red = 0x3f;
    crtc.cursor_0_green = 0x3f;
    crtc.cursor_0_blue = 0x3f;

    /* SR20 */
    crtc.linear_base_19_26 = (card->attr.address[0] & 0x07f80000) >> 19;

    /* SR21 */
    crtc.linear_base_27_31 = (card->attr.address[0] & 0xf8000000) >> 27;
    crtc.linear_aperture   = SIS_LINEAR_APERTURE_4M;
     
    /* SR27 */
    crtc.logical_screen_width = 3;
    crtc.graphics_prog_enable = 1;
    
    /* SR38 */
    crtc.extended_clock_select = 0;
    
    /* AR10 */
    crtc.mode_control = 0;
    crtc.graphics_mode_enable = 1;
    /* AR11 */
    crtc.screen_border_color = 0;
    /* AR12 */
    crtc.enable_color_plane = 0xf;
    /* AR13 */
    crtc.horizontal_pixel_pan = 0;
    
    /* GR5 */
    crtc.mode_register = 0;

    /* GR6 */
    crtc.graphics_enable = 1;
    crtc.chain_odd_even = 0;
    crtc.memory_address_select = 1;

    /* GR7 */
    crtc.color_dont_care = 0xf;
    if (siss->cursor_base)
    {
	crtc_set_cursor_start_addr (&crtc, siss->cursor_off);
	crtc.graphics_mode_hw_cursor = 0;
    }
    
    hactive = t->horizontal;
    hblank = t->hblank;
    hbp = t->hbp;
    hfp = t->hfp;
    
    vactive = t->vertical;
    vblank = t->vblank;
    vbp = t->vbp;
    vfp = t->vfp;
    
    pixel = (hactive + hblank) * (vactive + vblank) * t->rate;
    
    switch (screen->fb[0].bitsPerPixel) {
    case 8:
	hactive /= 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;

	crtc.color_mode_256 = 1;
	h_screen_off = hactive;
	h_adjust = 1;
	
	break;
    case 16:
	hactive /= 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;

	h_screen_off = hactive * 2;
	h_adjust = 1;
	
	crtc.color_mode_256 = 0;
	
	if (screen->fb[0].depth == 15)
	    crtc.graphics_mode_32k = 1;
	else
	    crtc.graphics_mode_64k = 1;
	break;
    case 24:
	hactive /= 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;
	
	h_screen_off = hactive * 3;
	h_adjust = 1;

	crtc.color_mode_256 = 0;
	
	/* SR6 */
	crtc.graphics_mode_true = 1;
	/* SR7 */
	crtc.direct_color_24bit = 0;
	/* SR9 */
	crtc.true_color_32bpp = 0;
	/* SRB */
	crtc.true_color_order = 1;
	break;
    case 32:
	hactive /= 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;
	
	h_screen_off = hactive * 4;
	h_adjust = 1;

	crtc.color_mode_256 = 0;
	
	/* SR6 */
	crtc.graphics_mode_true = 1;
	/* SR7 */
	crtc.direct_color_24bit = 0;
	/* SR9 */
	crtc.true_color_32bpp = 1;
	/* SRB */
	crtc.true_color_order = 1;
	break;
    }
	
    sisGetClock (pixel, &crtc);
    
    crtc.high_speed_dac_0 = crtc.high_speed_dac_1 = pixel > 135000000;
    
    sisEngThresh (&crtc, pixel, screen->fb[0].bitsPerPixel);
    
    /*
     * Compute horizontal register values from timings
     */
    h_total = hactive + hblank - 5;
    h_display_end = hactive - 1;
    h_blank_start = h_display_end;
    h_blank_end = h_blank_start + hblank;
    
    h_sync_start = hactive + hfp + h_adjust;
    h_sync_end = h_sync_start + hblank - hbp - hfp;

    crtc_set_h_total(&crtc, h_total);
    crtc_set_h_display_end (&crtc, h_display_end);
    crtc_set_h_blank_start (&crtc, h_blank_start);
    crtc_set_h_blank_end (&crtc, h_blank_end);
    crtc_set_h_sync_start (&crtc, h_sync_start);
    crtc_set_h_sync_end (&crtc, h_sync_end);
    crtc_set_screen_off (&crtc, h_screen_off);

    v_total = vactive + vblank - 2;
    v_retrace_start = vactive + vfp - 1;
    v_retrace_end = v_retrace_start + vblank - vbp - vfp;
    v_display_end = vactive - 1;
    v_blank_start = vactive - 1;
    v_blank_end = v_blank_start + vblank /* - 1 */;
    
    crtc_set_v_total(&crtc, v_total);
    crtc_set_v_retrace_start (&crtc, v_retrace_start);
    crtc.v_retrace_end_0_3 = v_retrace_end;
    crtc_set_v_display_end (&crtc, v_display_end);
    crtc_set_v_blank_start (&crtc, v_blank_start);
    crtc.v_blank_end_0_7 = v_blank_end;
    
#if 0
    crtc.h_blank_start_0_7 = 0x6a;
    crtc._h_blank_end = 0x9a;
    crtc.h_sync_start_0_7 = 0x6b;
    crtc._h_sync_end = 0x9a;
    
    crtc.v_retrace_start_0_7 = 0x7d;
    crtc._v_retrace_end = 0x23;
    crtc.v_blank_start_0_7 = 0x7d;
    crtc.v_blank_end_0_7 = 0x84;

    crtc.crt_cpu_threshold_control_0 = 0xdf;	/* SR8 */
    crtc.crt_cpu_threshold_control_1 = 0x00;	/* SR9 */
    crtc.extended_clock_generator = 0x40;	/* SR13 */

    crtc.cursor_h_start_0_7 = 0x83;
    crtc.cursor_v_start_0_7 = 0x6c;

    crtc.internal_vclk_0 = 0x68;
    crtc.internal_vclk_1 = 0xc4;
    crtc.misc_control_7 = 0x70;
#endif
    
    _sisSetCrtc (sisc, &crtc);
    return TRUE;
}

Bool
sisDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    sisCardInfo(pScreenPriv);
    union ddc_and_power_control_u   _ddc_and_power_control_u;

    ddc_and_power_control = sisc->save.crtc.ddc_and_power_control;

    kbd_cursor_activate = 0;
    video_memory_activate = 0;
    vga_standby = 0;
    vga_suspend = 0;
    acpi_enable = 0;
    switch (mode) {
    case KD_DPMS_NORMAL:
	break;
    case KD_DPMS_STANDBY:
	vga_standby = 1;
	break;
    case KD_DPMS_SUSPEND:
	vga_suspend = 1;
	break;
    case KD_DPMS_POWERDOWN:
	acpi_enable = 1;
	break;
    }
    PutSrtc (sisc, 0x11, ddc_and_power_control);
    return TRUE;
}

void
sisDisable (ScreenPtr pScreen)
{
}

void
sisRestore (KdCardInfo *card)
{
    SisCardInfo	*sisc = (SisCardInfo *) card->driver;
    
    memcpy (sisc->frameBuffer, sisc->save.text_save, SIS_TEXT_SAVE);
    _sisSetCrtc (sisc, &sisc->save.crtc);
    PutSrtc (sisc, 0xb, sisc->save.srb);
    PutSrtc (sisc, 0x5, sisc->save.sr5);
}

void
sisScreenFini (KdScreenInfo *screen)
{
    SisScreenInfo   *siss = (SisScreenInfo *) screen->driver;
    
    xfree (siss);
    screen->driver = 0;
}

void
sisCardFini (KdCardInfo *card)
{
    SisCardInfo	*sisc = (SisCardInfo *) card->driver;
    
    KdUnmapDevice (sisc->frameBuffer, sisc->memory);
    KdUnmapDevice (sisc->registers, sizeof (SisRec));
}

KdCardFuncs	sisFuncs = {
    sisCardInit,
    sisScreenInit,
    0,
    sisPreserve,
    sisEnable,
    sisDPMS,
    sisDisable,
    sisRestore,
    sisScreenFini,
    sisCardFini,
    sisCursorInit,
    sisCursorEnable,
    sisCursorDisable,
    sisCursorFini,
    0,
    sisDrawInit,
    sisDrawEnable,
    sisDrawSync,
    sisDrawDisable,
    sisDrawFini,
    sisGetColors,
    sisPutColors,
};
