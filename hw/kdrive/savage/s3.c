/*
 * $Id$
 *
 * Copyright 1999 SuSE, Inc.
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
/* $XFree86: $ */

#include "s3.h"

#define REGISTERS_OFFSET    (0x1000000)
#define PACKED_OFFSET	    (0x8100)
#define IOMAP_OFFSET	    (0x8000)

/*
 * Clock synthesis:
 *
 *  f_out = f_ref * ((M + 2) / ((N + 2) * (1 << R)))
 *
 *  Constraints:
 *
 *  1.	135MHz <= f_ref * ((M + 2) / (N + 2)) <= 270 MHz
 *  2.	N >= 1
 *
 *  Vertical refresh rate = clock / ((hsize + hblank) * (vsize + vblank))
 *  Horizontal refresh rate = clock / (hsize + hblank)
 */
 
#define DEFAULT_S3_TIMING   1
    
S3Timing    s3Timings[] = {
	     /* FP	BP	BLANK   */
    /*  M   N   R blank bios5 */
    { 640, 480, 60,
		16,	48,	160,    /* horizontal	31.321 KHz */
		10,	33,	45,	/* vertical	59.568 Hz  */
	26, 0,	3,			/* pixel	25.057 MHz */
    },
    
    { 800, 600, 85,
		32,	152,	248,    /* horizontal	53.673 KHz */
		1,	27,	31,	/* vertical	85.060 Hz  */
	108, 5,	2,			/* pixel	56.249 MHz */
    },
    { 800, 600, 75,
		16,	160,	256,    /* horizontal	46.891 KHz */
		1,	21,	25,	/* vertical	75.025 Hz  */
	81, 4,	2,			/* pixel	49.516 MHz */
    },
    { 800, 600, 72,
		56,	64,	240,    /* horizontal	48.186 KHz */
		37,	23,	66,	/* vertical	72.351 Hz  */
	26, 0,	2,			/* pixel	50.113 MHz */
    },
    { 800, 600, 60,
		48,	80,	256,
		1,	23,	28,
	0, 0, 0,
    },
    { 1024, 768, 85,
		48,	208,	352,    /* horizontal	68.676 KHz */
		1,	36,	40,	/* vertical	84.996 Hz  */
	64, 3,	1,			/* pixel	94.499 MHz */
    },
    { 1024, 768, 75,
		16,	176,	288,    /* horizontal	60.022 KHz */
		1,	28,	32,	/* vertical	75.028 Hz  */
	20, 0,	1,			/* pixel	78.749 MHz */
    },
    { 1024, 768, 70,
		32,	136,	304,    /* horizontal	56.604 KHz */
		2,	30,	38,	/* vertical	70.227 Hz  */
	124, 1,	3,			/* pixel	75.170 MHz */
    },
    { 1024, 768, 66,
		24,	144,	304,	/* horizontal	53.234 KHz */
		3,	29,	38,	/* vertical	66.047 Hz  */
	77, 6,	1,			/* pixel	70.695 MHz */
    },
    
    { 1152, 900, 85,
		48,	208,	384,	/* horizontal	 79.900 KHz */
		1,	32,	38,	/* vertical	 85.181 Hz  */
	118, 5, 1,			/* pixel	122.726 MHz */
    },
    { 1152, 900, 75,
		32,	208,	384,	/* horizontal	 70.495 Khz */
		1,	32,	38,	/* vertical	 75.154  Hz */
	119, 6,	1,			/* pixel	108.280 MHz */
    },
    { 1152, 900, 70,
		32,	208,	384,    /* horizontal	 65.251 KHz */
		2,	32,	38,	/* vertical	 69.564 Hz  */
	 12, 0, 0,			/* pixel	100.226 MHz */
    },
    { 1152, 900, 66,
		32,	208,	384,    /* horizontal	61.817 KHz */
		1,	32,	38,	/* vertical	65.903 Hz  */
	124, 17, 0,			/* pixel	94.951 MHz */
    },
    { 1280, 1024, 85,
		32,	248,	416,	/* horizontal	 90.561 KHz */
		1,	40,	45,	/* vertical	 84.717 Hz  */
	116, 9,	0,			/* pixel	153.593 MHz */
    },
    { 1280, 1024, 75,
		16,	248,	408,    /* horizontal	 80.255 KHz */
		1,	38,	42,	/* vertical	 75.285 Hz  */
	111, 4,	1,			/* pixel	134.828 MHz */
    },
    { 1280, 1024, 70,
		32,	248,	400,    /* horizontal	 74.573 KHz */
		0,	36,	39,	/* vertical	 70.153 Hz  */
	68, 2,	1,			/* pixel        125.283 MHz */
    },
    { 1280, 1024, 66,
		32,	248,	400,    /* horizontal	 70.007 KHz */
		0,	36,	39,	/* vertical	 65.858 Hz  */
	113, 5,	1,			/* pixel        117.612 MHz */
    },
    { 1280, 1024, 60,
		56,	240,	408,    /* horizontal	 70.007 KHz */
		1,	38,	42,	/* vertical	 65.858 Hz  */
	113, 5,	1,			/* pixel        117.612 MHz */
    },
    { 1600, 1200, 85,
		64,	304,	560,    /* horizontal	106.059 KHz */
		1,	46,	50,	/* vertical	 84.847 Hz  */
	126, 6,	0,			/* pixel	229.088 MHz */
    },
    { 1600, 1200, 75,
		64,	304,	560,    /* horizontal	 93.748 KHz */
		1,	46,	50,	/* vertical	 74.999 Hz  */
	97, 5,	0,			/* pixel	202.497 MHz */
    },
    { 1600, 1200, 70,
		56,	304,	588,	/* horizontal	 87.524 KHz */
		1,	46,	50,	/* vertical	 70.019 Hz  */
	105, 6,  0,			/* pixel	191.503 MHz */
    },
    { 1600, 1200, 65,
		56,	308,	524,    /* horizontal	 80.050 KHz */
		1,	38,	42,	/* vertical	 64.453 Hz  */
	93, 6,	0,			/* pixel	170.026 MHz */
    },
};

#define NUM_S3_TIMINGS	(sizeof (s3Timings) / sizeof (s3Timings[0]))

static void
_s3SetBlank (S3Ptr s3, S3Vga *s3vga, Bool blank)
{
    CARD8   clock_mode;
    
    s3SetImm(s3vga, s3_screen_off, blank ? 1 : 0);
}

static void
_s3SetDepth (S3Ptr s3, S3Vga *s3vga)
{
    CARD8   save_3c2;
    _s3SetBlank (s3, s3vga, TRUE);
    VgaFlush(&s3vga->card);
    VgaSetImm (&s3vga->card, s3_clock_load_imm, 1);
    VgaSetImm(&s3vga->card, s3_clock_load_imm, 0);
    _s3SetBlank (s3, s3vga, FALSE);
}

Bool
s3CardInit (KdCardInfo *card)
{
    S3CardInfo	*s3c;
    S3Ptr	s3;
    S3Vga	*s3vga;
    int		size;
    CARD8	*registers;
    CARD32	s3FrameBuffer;
    CARD32	s3Registers;
    CARD8	*temp_buffer;
    CARD32	max_memory;
    VGA32	save_linear_window_size;
    VGA32	save_enable_linear;
    VGA32	save_register_lock_2;

    s3c = (S3CardInfo *) xalloc (sizeof (S3CardInfo));
    if (!s3c)
    {
	goto bail0;
    }
    
    memset (s3c, '\0', sizeof (S3CardInfo));
    
    card->driver = s3c;
    
    if (card->attr.naddr > 1)
    {
	s3FrameBuffer = card->attr.address[1];
	s3Registers = card->attr.address[0];
	max_memory = 32 * 1024 * 1024;
    }
    else
    {
	s3FrameBuffer = card->attr.address[0];
	s3Registers = s3FrameBuffer + REGISTERS_OFFSET;
	max_memory = 16 * 1024 * 1024;
    }
	
#ifdef DEBUG
    fprintf (stderr, "S3 at 0x%x/0x%x\n", s3Registers, s3FrameBuffer);
#endif
    registers = KdMapDevice (s3Registers,
			     sizeof (S3) + PACKED_OFFSET);
    if (!registers)
    {
	ErrorF ("Can't map s3 device\n");
	goto bail2;
    }
    s3 = (S3Ptr) (registers + PACKED_OFFSET);
    s3c->registers = registers;
    s3c->s3 = s3;
    
    s3vga = &s3c->s3vga;
    s3RegInit (s3vga, (VGAVOL8 *) (registers + IOMAP_OFFSET));
    
    save_register_lock_2 = s3Get (s3vga, s3_register_lock_2);
    s3SetImm (s3vga, s3_register_lock_2, 0xa0);
    save_linear_window_size = s3Get (s3vga, s3_linear_window_size);
    save_enable_linear = s3Get (s3vga, s3_enable_linear);
    s3Set (s3vga, s3_linear_window_size, 3);
    s3Set (s3vga, s3_enable_linear, 1);
    VgaFlush (&s3vga->card);
    VgaFinish (&s3vga->card);
    
    /*
     * Can't trust S3 register value for frame buffer amount, must compute
     */
    temp_buffer = KdMapDevice (s3FrameBuffer, max_memory);
    
    s3c->memory = KdFrameBufferSize (temp_buffer, max_memory);

    s3Set (s3vga, s3_linear_window_size, save_linear_window_size);
    s3Set (s3vga, s3_enable_linear, save_enable_linear);
    VgaFlush (&s3vga->card);
    s3SetImm (s3vga, s3_register_lock_2, save_register_lock_2);
    VgaFinish (&s3vga->card);
#ifdef DEBUG
    fprintf (stderr, "Frame buffer 0x%x\n", s3c->memory);
#endif
    KdUnmapDevice (temp_buffer, max_memory);
    
    if (!s3c->memory)
    {
	ErrorF ("Can't detect s3 frame buffer at 0x%x\n", s3FrameBuffer);
	goto bail3;
    }
    
    s3c->frameBuffer = KdMapDevice (s3FrameBuffer, s3c->memory);
    if (!s3c->frameBuffer)
    {
	ErrorF ("Can't map s3 frame buffer\n");
	goto bail3;
    }

    card->driver = s3c;
    
    return TRUE;
bail3:
    KdUnmapDevice ((void *) s3, sizeof (S3));
bail2:
bail1:
    xfree (s3c);
bail0:
    return FALSE;
}

Bool
s3ScreenInit (KdScreenInfo *screen)
{
    KdCardInfo	    *card = screen->card;
    S3CardInfo	    *s3c = (S3CardInfo *) card->driver;
    S3ScreenInfo    *s3s;
    int		    screen_size;
    int		    memory;
    int		    requested_memory;
    int		    v_total, h_total;
    int		    byte_width;
    int		    pixel_width;
    int		    m, n, r;
    int		    i;
    S3Timing	    *t;

    s3s = (S3ScreenInfo *) xalloc (sizeof (S3ScreenInfo));
    if (!s3s)
	return FALSE;

    memset (s3s, '\0', sizeof (S3ScreenInfo));

    if (!screen->width || !screen->height)
    {
	screen->width = 800;
	screen->height = 600;
	screen->rate = 72;
    }
    if (!screen->depth)
	screen->depth = 8;
    
    for (i = 0, t = s3Timings; i < NUM_S3_TIMINGS; i++, t++)
    {
	if (t->horizontal >= screen->width &&
	    t->vertical >= screen->height &&
	    (!screen->rate || t->rate <= screen->rate))
	    break;
    }
    if (i == NUM_S3_TIMINGS)
	t = &s3Timings[DEFAULT_S3_TIMING];
    screen->rate = t->rate;
    screen->width = t->horizontal;
    screen->height = t->vertical;
    s3GetClock (S3ModeClock(t), &m, &n, &r, 511, 127, 4);
#ifdef DEBUG
    fprintf (stderr, "computed %d,%d,%d (%d) provided %d,%d,%d (%d)\n",
	     m, n, r, S3_CLOCK(m,n,r),
	     t->dac_m, t->dac_n, t->dac_r, 
	     S3_CLOCK(t->dac_m, t->dac_n, t->dac_r));
#endif
    /*
     * Can only operate in pixel-doubled mode at 8 bits per pixel
     */
    if (screen->depth > 16 && S3_CLOCK(m,n,r) > S3_MAX_CLOCK)
	screen->depth = 16;
    
    for (;;)
    {
	if (screen->depth >= 24)
	{
	    screen->depth = 24;
	    screen->bitsPerPixel = 32;
	}
	else if (screen->depth >= 16)
	{
	    screen->depth = 16;
	    screen->bitsPerPixel = 16;
	}
	else if (screen->depth >= 15)
	{
	    screen->depth = 15;
	    screen->bitsPerPixel = 16;
	}
	else
	{
	    screen->depth = 8;
	    screen->bitsPerPixel = 8;
	}

	/* Normalize width to supported values */
	
	if (screen->width >= 1600)
	    screen->width = 1600;
	else if (screen->width >= 1280)
	    screen->width = 1280;
	else if (screen->width >= 1152)
	    screen->width = 1152;
	else if (screen->width >= 1024)
	    screen->width = 1024;
	else if (screen->width >= 800)
	    screen->width = 800;
	else
	    screen->width = 640;
	
	byte_width = screen->width * (screen->bitsPerPixel >> 3);
	pixel_width = screen->width;
	screen->pixelStride = pixel_width;
	screen->byteStride = byte_width;

	screen_size = byte_width * screen->height;
	
	if (screen_size <= s3c->memory)
	    break;

	/*
	 * Fix requested depth and geometry until it works
	 */
	if (screen->depth > 16)
	    screen->depth = 16;
	else if (screen->depth > 8)
	    screen->depth = 8;
	else if (screen->width > 1152)
	{
	    screen->width = 1152;
	    screen->height = 900;
	}
	else if (screen->width > 1024)
	{
	    screen->width = 1024;
	    screen->height = 768;
	}
	else if (screen->width > 800)
	{
	    screen->width = 800;
	    screen->height = 600;
	}
	else if (screen->width > 640)
	{
	    screen->width = 640;
	    screen->height = 480;
	}
	else
	{
	    xfree (s3s);
	    return FALSE;
	}
    }
    
    memory = s3c->memory - screen_size;
    
    /*
     * Stick frame buffer at start of memory
     */
    screen->frameBuffer = s3c->frameBuffer;
    
    /*
     * Stick cursor at end of memory
     */
    if (memory >= 2048)
    {
	s3s->cursor_base = s3c->frameBuffer + (s3c->memory - 2048);
	memory -= 2048;
    }
    else
	s3s->cursor_base = 0;

    /*
     * Use remaining memory for off-screen storage, but only use
     * one piece (either right or bottom).
     */
    if (memory >= byte_width * S3_TILE_SIZE)
    {
	s3s->offscreen = s3c->frameBuffer + screen_size;
	s3s->offscreen_x = 0;
	s3s->offscreen_y = screen_size / byte_width;
	s3s->offscreen_width = pixel_width;
	s3s->offscreen_height = memory / byte_width;
	memory -= s3s->offscreen_height * byte_width;
    }
    else if (pixel_width - screen->width >= S3_TILE_SIZE)
    {
	s3s->offscreen = s3c->frameBuffer + screen->width;
	s3s->offscreen_x = screen->width;
	s3s->offscreen_y = 0;
	s3s->offscreen_width = pixel_width - screen->width;
	s3s->offscreen_height = screen->height;
    }
    else
	s3s->offscreen = 0;
    
    switch (screen->depth) {
    case 8:
	screen->visuals = ((1 << StaticGray) |
			   (1 << GrayScale) |
			   (1 << StaticColor) |
			   (1 << PseudoColor) |
			   (1 << TrueColor) |
			   (1 << DirectColor));
	screen->blueMask  = 0x00;
	screen->greenMask = 0x00;
	screen->redMask   = 0x00;
	break;
    case 15:
	screen->visuals = (1 << TrueColor);
	screen->blueMask  = 0x001f;
	screen->greenMask = 0x03e0;
	screen->redMask   = 0x7c00;
	break;
    case 16:
	screen->visuals = (1 << TrueColor);
	screen->blueMask  = 0x001f;
	screen->greenMask = 0x07e0;
	screen->redMask   = 0xf800;
	break;
    case 24:
	screen->visuals = (1 << TrueColor);
	screen->blueMask  = 0x0000ff;
	screen->greenMask = 0x00ff00;
	screen->redMask   = 0xff0000;
	break;
    }
    
    screen->driver = s3s;

    return TRUE;
}

void
s3Preserve (KdCardInfo *card)
{
    S3CardInfo	*s3c = card->driver;
    S3Ptr   s3 = s3c->s3;
    S3Vga   *s3vga = &s3c->s3vga;
    S3Save  *save = &s3c->save;
    CARD8   t1, t2;
    CARD8   *cursor_base;

    s3Save (s3vga);
    _s3SetBlank (s3, s3vga, TRUE);
    /*
     * Preserve the first part of the frame buffer which holds
     * the text mode fonts and data
     */
    s3Set (s3vga, s3_linear_window_size, 3);
    s3Set (s3vga, s3_enable_linear, 1);
    VgaFlush (&s3vga->card);
    memcpy (save->text_save, s3c->frameBuffer, S3_TEXT_SAVE);
    /*
     * Preserve graphics engine state
     */
    save->alt_mix = s3->alt_mix;
    save->write_mask = s3->write_mask;
    save->fg = s3->fg;
    save->bg = s3->bg;
    _s3SetBlank (s3, s3vga, FALSE);
}

/*
 * Enable the card for rendering.  Manipulate the initial settings
 * of the card here.
 */
void
s3Enable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdCardInfo	    *card = pScreenPriv->card;
    KdScreenInfo    *screen = pScreenPriv->screen;
    s3CardInfo (pScreenPriv);
    s3ScreenInfo (pScreenPriv);
    
    S3Vga   *s3vga = &s3c->s3vga;
    int	    hactive, hblank, hfp, hbp;
    int	    vactive, vblank, vfp, vbp;
    int	    hsize;

    int	    h_total;
    int	    h_display_end;
    int	    h_blank_start;
    int	    h_blank_end;
    int	    h_sync_start;
    int	    h_sync_end;
    int	    h_screen_off;
    int	    h_start_fifo_fetch;

    int	    primary_stream_l1;

    int	    v_total;
    int	    v_retrace_start;
    int	    v_retrace_end;
    int	    v_display_end;
    int	    v_blank_start;
    int	    v_blank_end;

    int	    h_blank_start_adjust;
    int	    h_blank_end_adjust;
    int	    h_sync_extend;
    int	    h_blank_extend;
    int	    i;
    CARD16  cursor_address;
    S3Timing    *t;
    int	    m, n, r;
    Bool    clock_double;
    
    for (i = 0; i < NUM_S3_TIMINGS; i++)
    {
	t = &s3Timings[i];

	if (t->horizontal == screen->width &&
	    t->vertical == screen->height &&
	    t->rate <= screen->rate)
	    break;
    }
    if (i == NUM_S3_TIMINGS)
	t = &s3Timings[DEFAULT_S3_TIMING];
    
    hfp = t->hfp;
    hbp = t->hbp;
    hblank = t->hblank;
    hactive = t->horizontal;

    vfp = t->vfp;
    vbp = t->vbp;
    vblank = t->vblank;
    vactive = t->vertical;

    m = s3Get (s3vga, s3_dclk_m);
    n = s3Get (s3vga, s3_dclk_n);
    r = s3Get (s3vga, s3_dclk_r);
#ifdef DEBUG
    fprintf (stderr, "old clock %d, %d, %d\n", m, n, r);
#endif
    clock_double = FALSE;
    s3GetClock (S3ModeClock(t), &m, &n, &r, 511, 127, 4);
    if (S3_CLOCK(m,n,r) > S3_MAX_CLOCK)
	clock_double = TRUE;
    s3Set (s3vga, s3_clock_select, 3);
    s3Set (s3vga, s3_dclk_m, m);
    s3Set (s3vga, s3_dclk_n, n);
    s3Set (s3vga, s3_dclk_r, r);
#ifdef DEBUG
    fprintf (stderr, "new clock %d, %d, %d\n", m, n, r);
#endif

    s3Set (s3vga, s3_select_graphics_mode, 1);
    s3Set (s3vga, s3_enable_blinking, 0);
    s3Set (s3vga, s3_enable_vga_16bit, 0);
    s3Set (s3vga, s3_enhanced_memory_mapping, 1);
    s3Set (s3vga, s3_enable_sff, 1);
    s3Set (s3vga, s3_enable_2d_access, 1);
    s3Set (s3vga, s3_byte_mode, 1);
    s3Set (s3vga, s3_max_scan_line, 0);
    s3Set (s3vga, s3_linear_window_size, 3);
    s3Set (s3vga, s3_enable_linear, 1);
    s3Set (s3vga, s3_enable_2d_3d, 1);
    s3Set (s3vga, s3_refresh_control, 1);
    s3Set (s3vga, s3_disable_pci_read_bursts, 0);
    s3Set (s3vga, s3_pci_retry_enable, 1);
    s3Set (s3vga, s3_enable_256, 1);
#if 1
    s3Set (s3vga, s3_border_select, 1);	/* eliminate white border */
#else
    s3Set (s3vga, s3_border_select, 0);	/* eliminate white border */
#endif
    s3Set (s3vga, s3_disable_v_retrace_int, 1);
    s3Set (s3vga, s3_horz_sync_neg, 0);
    s3Set (s3vga, s3_vert_sync_neg, 0);
    
    s3Set (s3vga, s3_dot_clock_8, 1);
    s3Set (s3vga, s3_enable_write_plane, 0xf);
    s3Set (s3vga, s3_extended_memory_access, 1);
    s3Set (s3vga, s3_sequential_addressing_mode, 1);
    s3Set (s3vga, s3_select_chain_4_mode, 1);
#if 1
    s3Set (s3vga, s3_linear_addressing_control, 1);
#else
    s3Set (s3vga, s3_linear_addressing_control, 0);
#endif
    s3Set (s3vga, s3_enable_8_bit_luts, 1);
    
    s3Set (s3vga, s3_dclk_invert, 0);
    s3Set (s3vga, s3_enable_clock_double, 0);

    s3Set (s3vga, s3_cpu_timeout, 0x1f);
    s3Set (s3vga, s3_fifo_fetch_timing, 1);
    s3Set (s3vga, s3_fifo_drain_delay, 7);


    s3Set (s3vga, s3_delay_h_enable, 0);
    s3Set (s3vga, s3_sdclk_skew, 0);
    
    /*
     * Compute character lengths for horizontal timing values
     */
    h_blank_start_adjust = 0;
    h_blank_end_adjust = 0;
    switch (screen->bitsPerPixel) {
    case 8:
	hactive = screen->width / 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;
	h_screen_off = hactive;
	s3Set (s3vga, s3_2d_graphics_engine_timeout, 0x1f);
	s3Set (s3vga, s3_pixel_length, 0);
	s3Set (s3vga, s3_color_mode, 0);
	/*
	 * Set up for double-pixel mode, switch color modes,
	 * divide the dclk and delay h blank by 2 dclks
	 */
	if (clock_double)
	{
	    s3Set (s3vga, s3_color_mode, 1);
	    s3Set (s3vga, s3_dclk_over_2, 1);
	    s3Set (s3vga, s3_enable_clock_double, 1);
	    s3Set (s3vga, s3_border_select, 0);
#if 0
	    s3Set (s3vga, s3_delay_blank, 2);
	    s3Set (s3vga, s3_delay_h_enable, 2);
	    crtc->extended_bios_5 = 2;
#endif
	    h_blank_start_adjust = -1;
	    h_blank_end_adjust = 0;
	}
	break;
    case 16:
	hactive = screen->width / 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;
	h_screen_off = hactive * 2;
	s3Set (s3vga, s3_pixel_length, 1);
	s3Set (s3vga, s3_2d_graphics_engine_timeout, 0x14);
	if (clock_double)
	{
	    if (screen->depth == 15)
		s3Set (s3vga, s3_color_mode, 3);
	    else
		s3Set (s3vga, s3_color_mode, 5);
	    s3Set (s3vga, s3_dclk_over_2, 1);
	    s3Set (s3vga, s3_enable_clock_double, 1);
	    s3Set (s3vga, s3_border_select, 0);
	}
	else
	{
	    if (screen->depth == 15)
		s3Set (s3vga, s3_color_mode, 2);
	    else
		s3Set (s3vga, s3_color_mode, 4);
	    s3Set (s3vga, s3_dclk_over_2, 0);
	    s3Set (s3vga, s3_enable_clock_double, 0);
	    s3Set (s3vga, s3_delay_blank, 0);
	}
	break;
    case 32:
	hactive = screen->width / 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;
	h_screen_off = hactive * 4;
	s3Set (s3vga, s3_pixel_length, 3);
	s3Set (s3vga, s3_color_mode, 0xd);
	s3Set (s3vga, s3_2d_graphics_engine_timeout, 0x07);
	break;
    }

    /*
     * X server starts frame buffer at top of memory
     */
    s3Set (s3vga, s3_start_address, 0);
    
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
    h_blank_start = hactive + 1 + h_blank_start_adjust;
    h_blank_end = hactive + hblank - 2 + h_blank_end_adjust;
    /*
     * The manual says h_total - 5, but the
     * bios does differently...
     */
    if (screen->width >= 1600)
	h_start_fifo_fetch = h_total - 24;
    else if (screen->width >= 1280)
	h_start_fifo_fetch = h_total - 19;
    else if (screen->width >= 1024)
	h_start_fifo_fetch = h_total - 14;
    else if (screen->width >= 800)
	h_start_fifo_fetch = h_total - 10;
    else
	h_start_fifo_fetch = h_total - 5;

    if (h_blank_end - h_blank_start >= 0x40)
	h_blank_extend = 1;
    else
	h_blank_extend = 0;
    
    if (h_sync_end - h_sync_start >= 0x20)
	h_sync_extend = 1;
    else
	h_sync_extend = 0;
    
    primary_stream_l1 = (screen->width * screen->bitsPerPixel / (8 * 8)) - 1;
    
#ifdef DEBUG
    fprintf (stderr, "h_total %d h_display_end %d\n",
	     h_total, h_display_end);
    fprintf (stderr, "h_sync_start %d h_sync_end %d h_sync_extend %d\n",
	     h_sync_start, h_sync_end, h_sync_extend);
    fprintf (stderr, "h_blank_start %d h_blank_end %d h_blank_extend %d\n",
	     h_blank_start, h_blank_end, h_blank_extend);
#endif
    
    s3Set (s3vga, s3_h_total, h_total);
    s3Set (s3vga, s3_h_display_end, h_display_end);
    s3Set (s3vga, s3_h_blank_start, h_blank_start);
    s3Set (s3vga, s3_h_blank_end, h_blank_end);
    s3Set (s3vga, s3_h_sync_start, h_sync_start);
    s3Set (s3vga, s3_h_sync_end, h_sync_end);
    s3Set (s3vga, s3_screen_offset, h_screen_off);
    s3Set (s3vga, s3_h_start_fifo_fetch, h_start_fifo_fetch);
    s3Set (s3vga, s3_h_sync_extend, h_sync_extend);
    s3Set (s3vga, s3_h_blank_extend, h_blank_extend);
    
    s3Set (s3vga, s3_primary_stream_l1, primary_stream_l1);

    v_total = vactive + vblank - 2;
    v_display_end = vactive - 1;
    
#if 0
    v_blank_start = vactive - 1;
    v_blank_end = v_blank_start + vblank - 1;
#else
    v_blank_start = vactive - 1;
    v_blank_end = v_blank_start + vblank - 1;
#endif
    
    v_retrace_start = vactive + vfp;
    v_retrace_end = vactive + vblank - vbp;
    
    s3Set (s3vga, s3_v_total, v_total);
    s3Set (s3vga, s3_v_retrace_start, v_retrace_start);
    s3Set (s3vga, s3_v_retrace_end, v_retrace_end);
    s3Set (s3vga, s3_v_display_end, v_display_end);
    s3Set (s3vga, s3_v_blank_start, v_blank_start);
    s3Set (s3vga, s3_v_blank_end, v_blank_end);
    
    if (vactive >= 1024)
	s3Set (s3vga, s3_line_compare, 0x7ff);
    else
	s3Set (s3vga, s3_line_compare, 0x3ff);
    
    /*
     * Set cursor
     */
    if (!screen->softCursor)
    {
	cursor_address = (s3s->cursor_base - screen->frameBuffer) / 1024;

	s3Set (s3vga, s3_cursor_address, cursor_address);
	s3Set (s3vga, s3_cursor_ms_x11, 0);
	s3Set (s3vga, s3_cursor_enable, 1);
    }
    else
	s3Set (s3vga, s3_cursor_enable, 0);
    
    /*
     * Set accelerator
     */
    switch (screen->width) {
    case 640:	s3Set (s3vga, s3_ge_screen_width, 1); break;
    case 800:	s3Set (s3vga, s3_ge_screen_width, 2); break;
    case 1024:	s3Set (s3vga, s3_ge_screen_width, 0); break;
    case 1152:	s3Set (s3vga, s3_ge_screen_width, 4); break;
    case 1280:	s3Set (s3vga, s3_ge_screen_width, 3); break;
    case 1600:	s3Set (s3vga, s3_ge_screen_width, 6); break;
    }
    
#if 0
    crtc->l_parm_0_7 = screen->width / 4;	/* Undocumented. */
#endif

    /*
     * Set DPMS to normal
     */
    s3Set (s3vga, s3_hsync_control, 0);
    s3Set (s3vga, s3_vsync_control, 0);
    
    _s3SetDepth (s3c->s3, s3vga);
}

void
s3Disable (ScreenPtr pScreen)
{
}

void
s3Restore (KdCardInfo *card)
{
    S3CardInfo	*s3c = card->driver;
    S3Ptr	s3 = s3c->s3;
    S3Vga	*s3vga = &s3c->s3vga;
    S3Save	*save = &s3c->save;
    CARD8	*cursor_base;

    /* graphics engine state */
    s3->alt_mix = save->alt_mix;
    s3->write_mask = save->write_mask;
    s3->fg = save->fg;
    s3->bg = save->bg;
    /* XXX should save and restore real values? */
    s3->scissors_tl = 0x00000000;
    s3->scissors_br = 0x0fff0fff;
    
    _s3SetBlank (s3, s3vga, TRUE);
    VgaRestore (&s3vga->card);
    s3Set (s3vga, s3_linear_window_size, 3);
    s3Set (s3vga, s3_enable_linear, 1);
    VgaFlush (&s3vga->card);
    memcpy (s3c->frameBuffer, save->text_save, S3_TEXT_SAVE);
    s3Reset (s3vga);
    _s3SetBlank (s3, s3vga, FALSE);
}

void
_s3SetSync (S3CardInfo *s3c, int hsync, int vsync)
{
    /* this abuses the macros defined to access the crtc structure */
    S3Ptr   s3 = s3c->s3;
    S3Vga   *s3vga = &s3c->s3vga;
    
    s3Set (s3vga, s3_hsync_control, hsync);
    s3Set (s3vga, s3_vsync_control, vsync);
    VgaFlush (&s3vga->card);
}

Bool
s3DPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    S3Vga   *s3vga = &s3c->s3vga;
    
    switch (mode) {
    case KD_DPMS_NORMAL:
	_s3SetSync (s3c, 0, 0);
	_s3SetBlank (s3c->s3, s3vga, FALSE);
	break;
    case KD_DPMS_STANDBY:
	_s3SetBlank (s3c->s3, s3vga, TRUE);
	_s3SetSync (s3c, 1, 0);
	break;
    case KD_DPMS_SUSPEND:
	_s3SetBlank (s3c->s3, s3vga, TRUE);
	_s3SetSync (s3c, 0, 1);
	break;
    case KD_DPMS_POWERDOWN:
	_s3SetBlank (s3c->s3, s3vga, TRUE);
	_s3SetSync (s3c, 1, 1);
	break;
    }
    return TRUE;
}

void
s3ScreenFini (KdScreenInfo *screen)
{
    S3ScreenInfo    *s3s = (S3ScreenInfo *) screen->driver;
    
    xfree (s3s);
    screen->driver = 0;
}

void
s3CardFini (KdCardInfo *card)
{
    S3CardInfo	*s3c = (S3CardInfo *) card->driver;
    
    KdUnmapDevice (s3c->frameBuffer, s3c->memory);
    KdUnmapDevice (s3c->registers, sizeof (S3) + PACKED_OFFSET);
    xfree (s3c);
    card->driver = 0;
}

KdCardFuncs	s3Funcs = {
    s3CardInit,
    s3ScreenInit,
    s3Preserve,
    s3Enable,
    s3DPMS,
    s3Disable,
    s3Restore,
    s3ScreenFini,
    s3CardFini,
    s3CursorInit,
    s3CursorEnable,
    s3CursorDisable,
    s3CursorFini,
    s3RecolorCursor,
    s3DrawInit,
    s3DrawEnable,
    s3DrawDisable,
    s3DrawFini,
    s3GetColors,
    s3PutColors,
};
