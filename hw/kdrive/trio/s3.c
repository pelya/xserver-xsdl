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
/* $XFree86: $ */

#include "s3.h"

#define REGISTERS_OFFSET    (0x1000000)
#define PACKED_OFFSET	    (0x8100)

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
		24,	144,	304,    /* horizontal	56.604 KHz */
		3,	29,	38,	/* vertical	70.227 Hz  */
	40, 2,	1,			/* pixel	75.170 MHz */
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
		16,	248,	416,	/* horizontal	 90.561 KHz */
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

CARD8
_s3ReadIndexRegister (volatile CARD8 *base, CARD8 index)
{
    CARD8   ret;
    *base = index;
    ret = *(base + 1);
    DRAW_DEBUG ((DEBUG_CRTC, " 0x%3x 0x%02x -> 0x%02x",
		((int) base) & 0xfff, index, ret));
    return ret;
}

void
_s3WriteIndexRegister (volatile CARD8 *base, CARD8 index, CARD8 value)
{
    DRAW_DEBUG ((DEBUG_CRTC, " 0x%3x 0x%02x <- 0x%02x",
		((int) base) & 0xfff, index, value));
    *base = index;
    *(base + 1) = value;
}

/*
 * Map the S3 card and detect its configuration.  Do not touch the card
 */

static void
_s3LoadCrtc (S3Ptr s3, S3Crtc *crtc)
{
    crtc->h_total_0_7			= GetCrtc (s3, 0x00);
    crtc->h_display_end_0_7		= GetCrtc (s3, 0x01);
    crtc->h_blank_start_0_7		= GetCrtc (s3, 0x02);
    crtc->_h_blank_end			= GetCrtc (s3, 0x03);
    crtc->h_sync_start_0_7		= GetCrtc (s3, 0x04);
    crtc->_h_sync_end			= GetCrtc (s3, 0x05);
    crtc->v_total_0_7			= GetCrtc (s3, 0x06);
    crtc->crtc_overflow			= GetCrtc (s3, 0x07);
    crtc->preset_row_scan		= GetCrtc (s3, 0x08);
    crtc->_max_scan_line		= GetCrtc (s3, 0x09);
    
    crtc->start_address_8_15		= GetCrtc (s3, 0x0c);
    crtc->start_address_0_7		= GetCrtc (s3, 0x0d);

    crtc->v_retrace_start_0_7		= GetCrtc (s3, 0x10);
    crtc->_v_retrace_end		= GetCrtc (s3, 0x11);
    crtc->v_display_end_0_7		= GetCrtc (s3, 0x12);
    crtc->screen_off_0_7	    	= GetCrtc (s3, 0x13);

    crtc->v_blank_start_0_7		= GetCrtc (s3, 0x15);
    crtc->v_blank_end_0_7		= GetCrtc (s3, 0x16);
    
    crtc->line_compare_0_7		= GetCrtc (s3, 0x18);
    
    crtc->memory_configuration		= GetCrtc (s3, 0x31);
    
    crtc->misc_1			= GetCrtc (s3, 0x3a);    
    crtc->h_start_fifo_fetch_0_7    	= GetCrtc (s3, 0x3b);
    
    crtc->mode_control			= GetCrtc (s3, 0x42);
    
    crtc->hardware_cursor_mode		= GetCrtc (s3, 0x45);
    crtc->cursor_address_8_15		= GetCrtc (s3, 0x4C);
    crtc->cursor_address_0_7		= GetCrtc (s3, 0x4D);

    crtc->extended_system_control_1	= GetCrtc (s3, 0x50);
    crtc->extended_system_control_2	= GetCrtc (s3, 0x51);

    crtc->extended_memory_control	= GetCrtc (s3, 0x53);

    crtc->extended_ramdac_control	= GetCrtc (s3, 0x55);
    
    crtc->extended_horizontal_overflow	= GetCrtc (s3, 0x5d);
    crtc->extended_vertical_overflow	= GetCrtc (s3, 0x5e);

    crtc->l_parm_0_7			= GetCrtc (s3, 0x62);
    
    crtc->extended_misc_control		= GetCrtc (s3, 0x65);
    
    crtc->extended_misc_control_2	= GetCrtc (s3, 0x67);
    
    crtc->configuration_3		= GetCrtc (s3, 0x68);
    
    crtc->extended_system_control_3	= GetCrtc (s3, 0x69);
    
    crtc->extended_bios_5		= GetCrtc (s3, 0x6d);
    
    crtc->extended_sequencer_b		= GetSrtc (s3, 0x0b);
    crtc->extended_sequencer_d		= GetSrtc (s3, 0x0d);
    crtc->dclk_value_low		= GetSrtc (s3, 0x12);
    crtc->dclk_value_high		= GetSrtc (s3, 0x13);
    crtc->control_2			= GetSrtc (s3, 0x15);
    crtc->ramdac_control		= GetSrtc (s3, 0x18);

    crtc->dclk_value_low_savage		= GetSrtc (s3, 0x36);
    crtc->dclk_pll_m0_savage_0_7	= GetSrtc (s3, 0x37);
    crtc->dclk_pll_m1_savage_0_7	= GetSrtc (s3, 0x38);
    crtc->extended_seq_39		= GetSrtc (s3, 0x39);
    
    fprintf (stderr, "SR1A 0x%x SR1B 0x%x\n",
	     GetSrtc (s3, 0x1a),
	     GetSrtc (s3, 0x1b));
    fprintf (stderr, "SR36 0x%x SR37 0x%x SR38 0x%x SR39 0x%x\n",
	     GetSrtc (s3, 0x36),
	     GetSrtc (s3, 0x37),
	     GetSrtc (s3, 0x38),
	     GetSrtc (s3, 0x39));
/* combine values */
    
    switch (crtc_ge_screen_width(crtc)) {
    case 0:
	if (crtc->enable_two_page)
	    crtc->ge_screen_pitch = 2048;
	else
	    crtc->ge_screen_pitch = 1024;
	break;
    case 1:
	crtc->ge_screen_pitch = 640;
	break;
    case 2:
	/* ignore magic 1600x1200x4 mode */
	crtc->ge_screen_pitch = 800;
	break;
    case 3:
	crtc->ge_screen_pitch = 1280;
	break;
    case 4:
	crtc->ge_screen_pitch = 1152;
	break;
    case 5:
	crtc->ge_screen_pitch = 0;  /* reserved */
	break;
    case 6:
	crtc->ge_screen_pitch = 1600;
	break;
    case 7:
	crtc->ge_screen_pitch = 0;  /* reserved */
	break;
    }
    switch (crtc->pixel_length) {
    case 0: 
	crtc->bits_per_pixel = 8;
	crtc->pixel_width = (crtc_h_display_end(crtc) + 1) * 8;
	break;
    case 1: 
	crtc->bits_per_pixel = 16;
	crtc->pixel_width = (crtc_h_display_end(crtc) + 1) * 4;
	break;
    case 3: 
	crtc->bits_per_pixel = 32;
	crtc->pixel_width = (crtc_h_display_end(crtc) + 1) * 8;
	break;
    }
    crtc->double_pixel_mode = 0;
    switch (crtc->color_mode) {
    case 0x0:
	crtc->depth = 8; break;
    case 0x1:
	crtc->depth = 8; crtc->double_pixel_mode = 1; break;
    case 0x3:
	crtc->depth = 15; break;
    case 0x5:
	crtc->depth = 16; break;
    case 0x7:
	crtc->depth = 24; break;    /* unused */
    case 0xd:
	crtc->depth = 24; break;
    }
}

static void
_s3SetBlank (S3Ptr s3, Bool blank)
{
    CARD8   clock_mode;
    
    DRAW_DEBUG ((DEBUG_S3INIT, "3c4 at 0x%x\n", &s3->crt_vga_3c4));
    clock_mode = _s3ReadIndexRegister (&s3->crt_vga_3c4, 0x01);
    if (blank)
	clock_mode |= 0x20;
    else
	clock_mode &= ~0x20;
    _s3WaitVRetrace (s3);
    _s3WriteIndexRegister (&s3->crt_vga_3c4, 0x01, clock_mode);
    DRAW_DEBUG ((DEBUG_S3INIT, "blank is set to 0x%x", clock_mode));
}

static void
_s3SetDepth (S3Ptr s3, S3Crtc *crtc)
{
    CARD8   save_3c2;
    _s3SetBlank (s3, TRUE);
    PutCrtc(s3, 0x38, 0x48);
    PutCrtc(s3, 0x39, 0xA0);
    PutCrtc(s3, 0x00, crtc->h_total_0_7);
    PutCrtc(s3, 0x01, crtc->h_display_end_0_7);
    PutCrtc(s3, 0x02, crtc->h_blank_start_0_7);
    PutCrtc(s3, 0x03, crtc->_h_blank_end);
    PutCrtc(s3, 0x04, crtc->h_sync_start_0_7);
    PutCrtc(s3, 0x05, crtc->_h_sync_end);
    PutCrtc(s3, 0x06, crtc->v_total_0_7);
    PutCrtc(s3, 0x07, crtc->crtc_overflow);
    PutCrtc(s3, 0x09, crtc->_max_scan_line);
    PutCrtc(s3, 0x0c, crtc->start_address_8_15);
    PutCrtc(s3, 0x0d, crtc->start_address_0_7);
    PutCrtc(s3, 0x10, crtc->v_retrace_start_0_7);
    PutCrtc(s3, 0x11, crtc->_v_retrace_end);
    PutCrtc(s3, 0x12, crtc->v_display_end_0_7);
    PutCrtc(s3, 0x13, crtc->screen_off_0_7);
    PutCrtc(s3, 0x15, crtc->v_blank_start_0_7);
    PutCrtc(s3, 0x16, crtc->v_blank_end_0_7);
    PutCrtc(s3, 0x18, crtc->line_compare_0_7);
    PutCrtc(s3, 0x31, crtc->memory_configuration);
    PutCrtc(s3, 0x3a, crtc->misc_1);
    PutCrtc(s3, 0x3b, crtc->h_start_fifo_fetch_0_7);
    PutCrtc(s3, 0x42, crtc->mode_control);
    PutCrtc(s3, 0x45, crtc->hardware_cursor_mode);
    PutCrtc(s3, 0x4c, crtc->cursor_address_8_15);
    PutCrtc(s3, 0x4d, crtc->cursor_address_0_7);
    PutCrtc(s3, 0x50, crtc->extended_system_control_1);
    PutCrtc(s3, 0x51, crtc->extended_system_control_2);
    PutCrtc(s3, 0x53, crtc->extended_memory_control);
    PutCrtc(s3, 0x55, crtc->extended_ramdac_control);
    PutCrtc(s3, 0x5d, crtc->extended_horizontal_overflow);
    PutCrtc(s3, 0x5e, crtc->extended_vertical_overflow);
    PutCrtc(s3, 0x62, crtc->l_parm_0_7);
    PutCrtc(s3, 0x65, crtc->extended_misc_control);
    PutCrtc(s3, 0x67, crtc->extended_misc_control_2);
    PutCrtc(s3, 0x68, crtc->configuration_3);
    PutCrtc(s3, 0x69, crtc->extended_system_control_3);
    PutCrtc(s3, 0x6d, crtc->extended_bios_5);
    PutCrtc(s3, 0x39, 0x00);
    PutCrtc(s3, 0x38, 0x00);
    PutSrtc(s3, 0x0b, crtc->extended_sequencer_b);
    PutSrtc(s3, 0x0d, crtc->extended_sequencer_d);
    /*
     * Move new dclk/mclk values into PLL
     */
    save_3c2 = s3->crt_vga_3cc;
    DRAW_DEBUG ((DEBUG_S3INIT, "save_3c2 0x%x", save_3c2));
    s3->crt_vga_3c2 = 0x0f;
    
    PutSrtc (s3, 0x36, crtc->dclk_value_low_savage);
    PutSrtc (s3, 0x37, crtc->dclk_pll_m0_savage_0_7);
    PutSrtc (s3, 0x38, crtc->dclk_pll_m1_savage_0_7);
    PutSrtc (s3, 0x39, crtc->extended_seq_39);
    PutSrtc(s3, 0x12, crtc->dclk_value_low);
    PutSrtc(s3, 0x13, crtc->dclk_value_high);
    
    DRAW_DEBUG ((DEBUG_S3INIT, "Set PLL load enable, frobbing clk_load..."));
    crtc->dfrq_en = 1;
    PutSrtc(s3, 0x15, crtc->control_2);
    PutSrtc(s3, 0x18, crtc->ramdac_control);
    
    DRAW_DEBUG ((DEBUG_S3INIT, "Clk load frobbed, restoring 3c2 to 0x%x", save_3c2));
    s3->crt_vga_3c2 = save_3c2;
    
    DRAW_DEBUG ((DEBUG_S3INIT, "Enabling display"));
    _s3SetBlank (s3, FALSE);
}

void
_s3RestoreCrtc (S3Ptr s3, S3Crtc *crtc)
{
    _s3SetDepth (s3, crtc);
}

s3Reset (S3CardInfo *s3c)
{
    S3Ptr   s3 = s3c->s3;
    S3Save  *save = &s3c->save;
    CARD8   *cursor_base;

    LockS3 (s3c);
    
    _s3UnlockExt (s3);
    
    _s3RestoreCrtc (s3, &save->crtc);
    
    /* set foreground */
    /* Reset cursor color stack pointers */
    (void) GetCrtc(s3, 0x45);
    PutCrtc(s3, 0x4a, save->cursor_fg);
    /* XXX for deeper screens? */
	
    /* set background */
    /* Reset cursor color stack pointers */
    (void) GetCrtc(s3, 0x45);
    PutCrtc(s3, 0x4b, save->cursor_bg);
    
    _s3LockExt (s3);
    
    /* graphics engine state */
    s3->alt_mix = save->alt_mix;
    s3->write_mask = save->write_mask;
    s3->fg = save->fg;
    s3->bg = save->bg;
    /* XXX should save and restore real values? */
    s3->scissors_tl = 0x00000000;
    s3->scissors_br = 0x0fff0fff;
    
    _s3WriteIndexRegister (&s3->crt_vga_3c4, 0x01, save->clock_mode);
    PutSrtc(s3, 0x08, save->locksrtc);
    PutCrtc(s3, 0x39, save->lock2);
    PutCrtc(s3, 0x38, save->lock1);
    
    UnlockS3 (s3c);
}

void
s3Save (S3CardInfo  *s3c)
{
    S3Ptr   s3 = s3c->s3;
    S3Save  *save = &s3c->save;
    S3Crtc  newCrtc;
    CARD8   t1, t2;
    CARD8   *cursor_base;

    LockS3 (s3c);
    
    save->alt_mix = s3->alt_mix;
    save->write_mask = s3->write_mask;
    save->fg = s3->fg;
    save->bg = s3->bg;
    
    save->lock1 = GetCrtc(s3, 0x38);
    save->lock2 = GetCrtc(s3, 0x39);
    save->locksrtc = GetSrtc (s3, 0x08);
    
    /* unlock srtc registers to read them */
    PutSrtc (s3, 0x08, 0x06);
    
    save->clock_mode = _s3ReadIndexRegister (&s3->crt_vga_3c4, 0x01);
    
    _s3UnlockExt (s3);
    save->cursor_fg = GetCrtc(s3, 0x4a);
    save->cursor_bg = GetCrtc(s3, 0x4b);
    
    _s3LoadCrtc (s3, &save->crtc);
    
    _s3LockExt (s3);
    
    UnlockS3 (s3c);
}
Bool
s3CardInit (KdCardInfo *card)
{
    S3CardInfo	*s3c;
    S3Ptr	s3;
    int		size;
    CARD8	*registers;
    CARD32	s3FrameBuffer;
    CARD32	s3Registers;
    CARD8	*temp_buffer;
    CARD32	max_memory;

    DRAW_DEBUG ((DEBUG_S3INIT, "s3CardInit"));
    s3c = (S3CardInfo *) xalloc (sizeof (S3CardInfo));
    if (!s3c)
    {
	DRAW_DEBUG ((DEBUG_FAILURE, "can't alloc s3 card info"));
	goto bail0;
    }
    
    memset (s3c, '\0', sizeof (S3CardInfo));
    
    card->driver = s3c;
    
    if (card->attr.naddr > 1)
    {
	s3FrameBuffer = card->attr.address[1];
	s3Registers = card->attr.address[0];
	max_memory = 32 * 1024 * 1024;
	s3c->savage = TRUE;
    }
    else
    {
	s3FrameBuffer = card->attr.address[0];
	s3Registers = s3FrameBuffer + REGISTERS_OFFSET;
	max_memory = 4 * 1024 * 1024;
	s3c->savage = FALSE;
    }
	
    fprintf (stderr, "S3 at 0x%x/0x%x\n", s3Registers, s3FrameBuffer);
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
    
#if 0
    s3->crt_vga_3c3 = 1;    /* wake up part from deep sleep */
    s3->crt_vga_3c2 = 0x01 | 0x02 | 0x0c;
    
    s3->crt_vga_3c4 = 0x58;
    s3->crt_vga_3c5 = 0x10 | 0x3;
#endif
    
    /*
     * Can't trust S3 register value for frame buffer amount, must compute
     */
    temp_buffer = KdMapDevice (s3FrameBuffer, max_memory);
    
    s3c->memory = KdFrameBufferSize (temp_buffer, max_memory);

    fprintf (stderr, "Frame buffer 0x%x\n", s3c->memory);
    
    DRAW_DEBUG ((DEBUG_S3INIT, "Detected frame buffer %d", s3c->memory));

    KdUnmapDevice (temp_buffer, 4096 * 1024);
    
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

    DRAW_DEBUG ((DEBUG_S3INIT, "s3ScreenInit"));
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
    
    DRAW_DEBUG ((DEBUG_S3INIT, "Requested parameters %dx%dx%d",
		 screen->width, screen->height, screen->rate));
    for (i = 0, t = s3Timings; i < NUM_S3_TIMINGS; i++, t++)
    {
	DRAW_DEBUG ((DEBUG_S3INIT, "Available parameters %dx%dx%d",
		     t->horizontal, t->vertical, t->rate));
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
#if 0
    s3GetClock (t, &m, &n, &r, 
		s3c->savage ? 511 : 127,
		s3c->savage ? 127 : 31,
		s3c->savage ? 4 : 3);
#else
    s3GetClock (t, &m, &n, &r, 127, 31, 3);
#endif
#if 1
    fprintf (stderr, "computed %d,%d,%d (%d) provided %d,%d,%d (%d)\n",
	     m, n, r, S3_CLOCK(m,n,r),
	     t->dac_m, t->dac_n, t->dac_r, 
	     S3_CLOCK(t->dac_m, t->dac_n, t->dac_r));
#endif
    /*
     * Can only operate in pixel-doubled mode at 8 bits per pixel
     */
    if (s3c->savage)
    {
	if (screen->depth > 16 && S3_CLOCK(m,n,r) > S3_MAX_CLOCK)
	    screen->depth = 16;
    }
    else
    {
	if (screen->depth > 8 && S3_CLOCK(m,n,r) > S3_MAX_CLOCK)
	    screen->depth = 8;
    }
    
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
    
    DRAW_DEBUG ((DEBUG_S3INIT, "depth %d bits %d", screen->depth, screen->bitsPerPixel));
    
    DRAW_DEBUG ((DEBUG_S3INIT, "Screen size %dx%d memory %d",
		screen->width, screen->height, s3c->memory));
    DRAW_DEBUG ((DEBUG_S3INIT, "frame buffer 0x%x cursor 0x%x offscreen 0x%x",
		s3c->frameBuffer, s3s->cursor_base, s3s->offscreen));
    DRAW_DEBUG ((DEBUG_S3INIT, "offscreen %dx%d+%d+%d",
		s3s->offscreen_width, s3s->offscreen_height, 
		s3s->offscreen_x, s3s->offscreen_y));
    
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
    
    s3Save (s3c);
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
    
    S3Crtc  crtcR, *crtc;
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

    int	    v_total;
    int	    v_retrace_start;
    int	    v_retrace_end;
    int	    v_display_end;
    int	    v_blank_start;
    int	    v_blank_end;

    int	    h_adjust;
    int	    h_sync_extend_;
    int	    h_blank_extend_;
    int	    i;
    CARD16  cursor_address;
    S3Timing    *t;
    int	    m, n, r;
    
    DRAW_DEBUG ((DEBUG_S3INIT, "s3Enable"));
    
    DRAW_DEBUG ((DEBUG_S3INIT, "requested bpp %d current %d",
		pScreenPriv->bitsPerPixel, s3c->save.crtc.bits_per_pixel));
    
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

    crtcR = s3c->save.crtc;
    crtc = &crtcR;
    
#if 0
    if (s3c->savage)
    {
	m = ((int) crtc->dclk_pll_m_savage_8 << 8) | crtc->dclk_pll_m0_savage_0_7;
	n = (crtc->dclk_pll_n_savage_6 << 6) | crtc->dclk_pll_n_savage_0_5;
	r = (crtc->dclk_pll_r_savage_2 << 2) | crtc->dclk_pll_r_savage_0_1;
	fprintf (stderr, "old clock %d, %d, %d\n", m, n, r);
	s3GetClock (t, &m, &n, &r, 127, 31, 3);
#if 0
	s3GetClock (t, &m, &n, &r, 511, 127, 4);
	crtc->dclk_pll_m0_savage_0_7 = m;
	crtc->dclk_pll_m1_savage_0_7 = m;
	crtc->dclk_pll_m_savage_8 = m >> 8;
	crtc->dclk_pll_n_savage_0_5 = n;
	crtc->dclk_pll_n_savage_6 = n >> 6;
	crtc->dclk_pll_r_savage_0_1 = r;
	crtc->dclk_pll_r_savage_2 = r >> 2;
	crtc->dclk_pll_select_savage = 1;
#endif
	fprintf (stderr, "new clock %d, %d, %d\n", m, n, r);
    }
    else
    {
	s3GetClock (t, &m, &n, &r, 127, 31, 3);
	crtc->dclk_pll_m_trio = m;
	crtc->dclk_pll_n_trio = n;
	crtc->dclk_pll_r_trio = r;
    }

    if (!s3c->savage)
    {
	crtc->alt_refresh_count = 0x02;
	crtc->enable_alt_refresh = 1;
    }
    else
    {
	crtc->alt_refresh_count = 1;
	crtc->enable_alt_refresh = 0;
    }
    crtc->enable_256_or_more = 1;

    DRAW_DEBUG ((DEBUG_S3INIT, "memory_bus_size %d\n", crtc->memory_bus_size));
    if (!s3c->savage)
	crtc->memory_bus_size = 1;
    
    if (!s3c->savage)
	crtc->dclk_over_2 = 0;
    crtc->dclk_invert = 0;
    crtc->enable_clock_double = 0;
    crtc->delay_blank = 0;
    if (!s3c->savage)
	crtc->extended_bios_5 = 0;
    /*
     * Compute character lengths for horizontal timing values
     */
    switch (screen->bitsPerPixel) {
    case 8:
	hactive = screen->width / 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;
	h_screen_off = hactive;
	crtc->pixel_length = 0;
        crtc->color_mode = 0;
	/*
	 * Set up for double-pixel mode, switch color modes,
	 * divide the dclk and delay h blank by 2 dclks
	 */
	if (S3_CLOCK(m, n, r) > S3_MAX_CLOCK)
	{
	    DRAW_DEBUG ((DEBUG_S3INIT, "S3 clock %g > 80MHz, using pixel double mode",
			 S3_CLOCK(crtc->dclk_pll_m, crtc->dclk_pll_n, 
				  crtc->dclk_pll_r)));
	    crtc->color_mode = 1;
	    crtc->dclk_over_2 = 1;
	    crtc->enable_clock_double = 1;
	    crtc->delay_blank = 2;
	    crtc->extended_bios_5 = 2;
	}
	h_adjust = 1;
	break;
    case 16:
	if (s3c->savage)
	{
	    hactive = screen->width / 4;
	    hblank /= 4;
	    hfp /= 4;
	    hbp /= 4;
	    h_screen_off = hactive * 2;
	    crtc->pixel_length = 1;
	    if (S3_CLOCK(m,n,r) > S3_MAX_CLOCK)
	    {
		if (crtc->depth == 15)
		    crtc->color_mode = 3;
		else
		    crtc->color_mode = 5;
		crtc->dclk_over_2 = 1;
		crtc->enable_clock_double = 1;
		crtc->delay_blank = 2;
	    }
	    else
	    {
		if (crtc->depth == 15)
		    crtc->color_mode = 2;
		else
		    crtc->color_mode = 4;
		crtc->dclk_over_2 = 0;
		crtc->enable_clock_double = 0;
	    }
	    h_adjust = 1;
	}
	else
	{
	    hactive = screen->width / 4;
	    hblank /= 4;
	    hfp /= 4;
	    hbp /= 4;
	    h_screen_off = hactive;
	    crtc->pixel_length = 1;
	    crtc->extended_bios_5 = 2;
	    if (crtc->depth == 15)
		crtc->color_mode = 3;
	    else
		crtc->color_mode = 5;
	    h_adjust = 2;
	}
	break;
    case 32:
	hactive = screen->width / 8;
	hblank /= 8;
	hfp /= 8;
	hbp /= 8;
	h_screen_off = hactive * 4;
	crtc->pixel_length = 3;
	crtc->color_mode = 0xd;
	h_adjust = 1;
	break;
    }
#endif

    /*
     * X server starts frame buffer at top of memory
     */
    DRAW_DEBUG ((DEBUG_S3INIT, "Old start address 0x%x\n",
		 crtc_start_address (crtc)));
    crtc_set_start_address (crtc, 0);
    
#if 0    
    /*
     * Compute horizontal register values from timings
     */
    h_total = hactive + hblank - 5;
    h_display_end = hactive - 1;
    h_blank_start = h_display_end;
    h_blank_end = h_blank_start + hblank - h_adjust;
    h_sync_start = hactive + hfp + h_adjust;
    h_sync_end = h_sync_start + hblank - hbp - hfp;
    h_start_fifo_fetch = h_total - 5;

    DRAW_DEBUG ((DEBUG_S3INIT, "blank_end 0x%x sync_end 0x%x sync_start 0x%x\n",
		 h_blank_end, h_sync_end, h_sync_start));

    if (h_blank_end - h_blank_start > 0x40)
	h_blank_extend_ = 1;
    else
	h_blank_extend_ = 0;
    
    if (h_sync_end - h_sync_start > 0x20)
	h_sync_extend_ = 1;
    else
	h_sync_extend_ = 0;
    
    DRAW_DEBUG ((DEBUG_S3INIT, "blank_end 0x%x sync_end 0x%x extend %d\n",
		 h_blank_end, h_sync_end, h_sync_extend_));

    crtc_set_h_total(crtc, h_total);
    crtc_set_h_display_end (crtc, h_display_end);
    crtc_set_h_blank_start (crtc, h_blank_start);
    crtc_set_h_blank_end (crtc, h_blank_end);
    crtc_set_h_sync_start (crtc, h_sync_start);
    crtc_set_h_sync_end (crtc, h_sync_end);
    crtc_set_screen_off (crtc, h_screen_off);
    crtc_set_h_start_fifo_fetch (crtc, h_start_fifo_fetch);
    crtc->h_sync_extend = h_sync_extend_;
    crtc->h_blank_extend = h_blank_extend_;


    v_total = vactive + vblank - 2;
    v_retrace_start = vactive + vfp - 1;
    v_retrace_end = v_retrace_start + vblank - vbp - 1;
    v_display_end = vactive - 1;
    v_blank_start = vactive - 1;
    v_blank_end = v_blank_start + vblank - 1;
    
    crtc_set_v_total(crtc, v_total);
    crtc_set_v_retrace_start (crtc, v_retrace_start);
    crtc->v_retrace_end_0_3 = v_retrace_end;
    crtc_set_v_display_end (crtc, v_display_end);
    crtc_set_v_blank_start (crtc, v_blank_start);
    crtc->v_blank_end_0_7 = v_blank_end;
#endif
    
    /*
     * Set cursor
     */
    if (!screen->softCursor)
    {
	cursor_address = (s3s->cursor_base - screen->frameBuffer) / 1024;

	crtc->cursor_address_0_7 = cursor_address;
	crtc->cursor_address_8_15 = cursor_address >> 8;
	crtc->hardware_cursor_ms_x11 = 0;
	crtc->hardware_cursor_enable = 1;
    }
    else
	crtc->hardware_cursor_enable = 0;
    
    /*
     * Set accelerator
     */
    switch (screen->width) {
    case 640:	crtc_set_ge_screen_width(crtc,1); break;
    case 800:	crtc_set_ge_screen_width(crtc,2); break;
    case 1024:	crtc_set_ge_screen_width(crtc,0); break;
    case 1152:	crtc_set_ge_screen_width(crtc,4); break;
    case 1280:	crtc_set_ge_screen_width(crtc,3); break;
    case 1600:	crtc_set_ge_screen_width(crtc,6); break;
    }
    
    /*
     * Set depth values
     */
    crtc->bits_per_pixel = screen->bitsPerPixel;
    crtc->depth = screen->depth;

    crtc->l_parm_0_7 = screen->width / 4;	/* Undocumented. */

    crtc->disable_v_retrace_int = 1;    /* don't let retrace interrupt */
    
    DRAW_DEBUG ((DEBUG_S3INIT, "new        h total %d display_end %d",
		 crtc_h_total(crtc),
		 crtc_h_display_end(crtc)));
    DRAW_DEBUG ((DEBUG_S3INIT, "           sync_start %d sync_end %d (%d)",
		 crtc_h_sync_start(crtc), 
		 crtc_h_sync_end(crtc), h_sync_end));

    DRAW_DEBUG ((DEBUG_S3INIT, "           blank_start %d blank_end %d",
		 crtc_h_blank_start(crtc),
		 crtc_h_blank_end(crtc)));

    DRAW_DEBUG ((DEBUG_S3INIT, "           screen_off %d start_fifo %d",
		 crtc_screen_off(crtc), crtc_h_start_fifo_fetch(crtc)));

    DRAW_DEBUG ((DEBUG_S3INIT, "           active %d blank %d fp %d bp %d",
		 hactive, hblank, hfp, hbp));

    DRAW_DEBUG ((DEBUG_S3INIT, "new        v total %d display_end %d",
		 crtc_v_total(crtc),
		 crtc_v_display_end(crtc)));
    DRAW_DEBUG ((DEBUG_S3INIT, "           retrace_start %d retrace_end %d (%d)",
		 crtc_v_retrace_start(crtc),
		 crtc->v_retrace_end,
		 v_retrace_end));
    DRAW_DEBUG ((DEBUG_S3INIT, "           blank_start %d blank_end %d",
		 crtc_v_blank_start(crtc),
		 crtc->v_blank_end_0_7));

    DRAW_DEBUG ((DEBUG_S3INIT, "           active %d blank %d fp %d bp %d",
		 vactive, vblank, vfp, vbp));

    /*
     * Set DPMS to normal
     */
    crtc->hsync_control = 0;
    crtc->vsync_control = 0;
    
    LockS3 (s3c);
    _s3SetDepth (s3c->s3, crtc);
    UnlockS3 (s3c);
}

void
s3Disable (ScreenPtr pScreen)
{
}

void
s3Restore (KdCardInfo *card)
{
    S3CardInfo	*s3c = card->driver;
    
    s3Reset (s3c);
}

void
_s3SetSync (S3CardInfo *s3c, int hsync, int vsync)
{
    /* this abuses the macros defined to access the crtc structure */
    union extended_sequencer_d_u    _extended_sequencer_d_u;
    S3Ptr   s3 = s3c->s3;
    
    extended_sequencer_d = s3c->save.crtc.extended_sequencer_d;
    hsync_control = hsync;
    vsync_control = vsync;
    PutSrtc (s3, 0x0d, extended_sequencer_d);
}

Bool
s3DPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    s3CardInfo(pScreenPriv);
    
    switch (mode) {
    case KD_DPMS_NORMAL:
	_s3SetSync (s3c, 0, 0);
	_s3SetBlank (s3c->s3, FALSE);
	break;
    case KD_DPMS_STANDBY:
	_s3SetBlank (s3c->s3, TRUE);
	_s3SetSync (s3c, 1, 0);
	break;
    case KD_DPMS_SUSPEND:
	_s3SetBlank (s3c->s3, TRUE);
	_s3SetSync (s3c, 0, 1);
	break;
    case KD_DPMS_POWERDOWN:
	_s3SetBlank (s3c->s3, TRUE);
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
/*    DeleteCriticalSection (&s3c->lock); */
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
