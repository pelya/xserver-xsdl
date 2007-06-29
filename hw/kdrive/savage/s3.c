/*
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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "s3.h"

#define REGISTERS_OFFSET    (0x1000000)
#define PACKED_OFFSET	    (0x8100)
#define IOMAP_OFFSET	    (0x8000)

#define S3_MIN_CLOCK	    250000

static void
_s3SetBlank (S3Ptr s3, S3Vga *s3vga, Bool blank)
{
    CARD8   clock_mode;
    
    s3SetImm(s3vga, s3_screen_off, blank ? 1 : 0);
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
    VGA32	save_misc_output;

    s3c = (S3CardInfo *) xalloc (sizeof (S3CardInfo));
    if (!s3c)
    {
	goto bail0;
    }
    
    memset (s3c, '\0', sizeof (S3CardInfo));
    
    card->driver = s3c;
    
#ifdef VXWORKS
    s3c->bios_initialized = 0;
#else
    s3c->bios_initialized = 1;
#endif
    
    if (card->attr.naddr > 1 && card->attr.address[1])
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
    
    if (!s3c->bios_initialized)
    {
	volatile CARD32	*wakeup;

	wakeup = (volatile CARD32 *) (registers + 0x8510);
	ErrorF ("Wakeup S3 chip at 0x%x\n", wakeup);
	ErrorF ("Wakeup was 0x%x\n", *wakeup);
	/* wakeup the chip */
	*(volatile CARD32 *) (registers + 0x8510) = 1;
	ErrorF ("Wakeup is 0x%x\n", *wakeup);
    }
    s3Set (s3vga, s3_io_addr_select, 1);
    s3Set (s3vga, s3_enable_ram, 1);
    VgaFlush (&s3vga->card);
    
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
s3ModeSupported (KdScreenInfo		*screen,
		 const KdMonitorTiming	*t)
{
    if (screen->fb[1].depth)
    {
	/*
	 * Must have at least one true color stream
	 */
	if (screen->fb[0].depth <= 8 &&
	    screen->fb[1].depth <= 8)
	    return FALSE;
    }
    /* make sure the clock isn't too fast */
    if (t->clock > S3_MAX_CLOCK * 2)
	return FALSE;
    /* width must be a multiple of 16 */
    if (t->horizontal & 0xf)
	return FALSE;
    return TRUE;
}

Bool
s3ModeUsable (KdScreenInfo	*screen)
{
    KdCardInfo	    *card = screen->card;
    S3CardInfo	    *s3c = (S3CardInfo *) card->driver;
    int		    screen_size;
    int		    pixel_width;
    int		    byte_width;
    int		    fb;
    
    screen_size = 0;
    for (fb = 0; fb < KD_MAX_FB && screen->fb[fb].depth; fb++)
    {
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
    
        /*
         * SGRAM requires stride % 64 == 0
         */
        screen->fb[fb].pixelStride = (screen->width + 63) & ~63;
        screen->fb[fb].byteStride = screen->fb[fb].pixelStride * (screen->fb[fb].bitsPerPixel >> 3);
        screen_size += screen->fb[fb].byteStride * screen->height;
    }

    return screen_size <= s3c->memory;
}

Bool
s3ScreenInit (KdScreenInfo *screen)
{
    KdCardInfo	    *card = screen->card;
    S3CardInfo	    *s3c = (S3CardInfo *) card->driver;
    S3ScreenInfo    *s3s;
    int		    memory;
    int		    requested_memory;
    int		    v_total, h_total;
    int		    m, n, r;
    int		    i;
    const KdMonitorTiming *t;
    int		    screen_size;
    int		    fb;
    int		    ma;

    s3s = (S3ScreenInfo *) xalloc (sizeof (S3ScreenInfo));
    if (!s3s)
	return FALSE;

    memset (s3s, '\0', sizeof (S3ScreenInfo));

#ifdef PHOENIX
    screen->width = 1152;
    screen->height = 900;
    screen->rate = 85;
    screen->depth = 32;
#endif
    if (!screen->width || !screen->height)
    {
	screen->width = 800;
	screen->height = 600;
	screen->rate = 72;
    }
    if (!screen->fb[0].depth)
	screen->fb[0].depth = 8;
    
    t = KdFindMode (screen, s3ModeSupported);
    screen->rate = t->rate;
    screen->width = t->horizontal;
    screen->height = t->vertical;
    s3GetClock (t->clock, &m, &n, &r, 511, 127, 4, 250000);
#ifdef DEBUG
    fprintf (stderr, "computed %d,%d,%d (%d)\n",
	     m, n, r, S3_CLOCK(m,n,r));
#endif
#if 0
    /*
     * Can only operate in pixel-doubled mode at 8 or 16 bits per pixel
     */
    if (screen->depth > 16 && S3_CLOCK(m,n,r) > S3_MAX_CLOCK)
	screen->depth = 16;
#endif
    
    if (!KdTuneMode (screen, s3ModeUsable, s3ModeSupported))
    {
	xfree (s3s);
	return FALSE;
    }
    
    s3s->fbmap[2] = -1;
    if (screen->fb[1].depth)
    {
	if (screen->fb[0].bitsPerPixel >= 16)
	{
	    s3s->fbmap[0] = 1;
	    s3s->fbmap[1] = 0;
	}
	else
	{
	    s3s->fbmap[0] = 0;
	    s3s->fbmap[1] = 1;
	}
    }
    else
    {
	s3s->fbmap[0] = 0;
	s3s->fbmap[1] = -1;
    }
    
    screen_size = 0;
    for (fb = 0; fb < KD_MAX_FB && screen->fb[fb].depth; fb++)
	screen_size += screen->fb[fb].byteStride * screen->height;
    
    memory = s3c->memory - screen_size;
    
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

    screen_size = 0;
    for (ma = 0; s3s->fbmap[ma] >= 0; ma++)
    {
	fb = s3s->fbmap[ma];
	screen->fb[fb].frameBuffer = s3c->frameBuffer + screen_size;
	screen_size += screen->fb[fb].byteStride * screen->height;
	
	REGION_INIT(pScreen, (&s3s->region[fb]), NullBox, 0);
	if (screen->fb[fb].bitsPerPixel == 8)
	    s3s->fb[ma].chroma_key = 0xff;
	else
	    s3s->fb[ma].chroma_key = 0;
	
	/*
	 * Use remaining memory for off-screen storage, but only use
	 * one piece (either right or bottom).
	 */
	if (memory >= screen->fb[fb].byteStride * S3_TILE_SIZE)
	{
	    s3s->fb[ma].offscreen = screen->fb[fb].frameBuffer;
	    s3s->fb[ma].offscreen_x = 0;
	    s3s->fb[ma].offscreen_y = screen->height;
	    s3s->fb[ma].offscreen_width = screen->fb[fb].pixelStride;
	    s3s->fb[ma].offscreen_height = S3_TILE_SIZE;
	    memory -= s3s->fb[ma].offscreen_height * screen->fb[fb].byteStride;
	    screen_size += s3s->fb[ma].offscreen_height * screen->fb[fb].byteStride;
	}
	else
	    s3s->fb[ma].offscreen = 0;
    
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
    }
    
    screen->driver = s3s;

    return TRUE;
}

typedef struct _biosInit {
    VGA16   reg;
    VGA8    value;
} s3BiosInit;

s3BiosInit s3BiosReg[] = {
    S3_SR +0x15, 0x23,
    S3_MISC_OUT, 0x2f,
    0xffff, 1,
    S3_SR +0x15, 0x03,

    S3_SR + 0x0, 0x03,
    S3_SR + 0x1, 0x00,
    S3_SR + 0x2, 0x03,
    S3_SR + 0x3, 0x00,
    S3_SR + 0x4, 0x02,
    S3_SR + 0x5, 0x05,
    S3_SR + 0x6, 0x06,
    S3_SR + 0x7, 0x07,
/*    S3_SR + 0x8, 0x06, */
    S3_SR + 0x9, 0x00,
    S3_SR + 0xa, 0x0a,
    S3_SR + 0xb, 0x00,
    S3_SR + 0xc, 0x0c,
    S3_SR + 0xd, 0x00,
    S3_SR + 0xe, 0x0e,
    S3_SR + 0xf, 0x0f,

/*    S3_SR +0x10, 0x00, */
/*     S3_SR +0x11, 0x0c, */
    S3_SR +0x12, 0x01,
    S3_SR +0x13, 0x52,
    S3_SR +0x14, 0x00,
    
/*    S3_SR +0x15, 0x03, */
    
    S3_SR +0x16, 0xc5,
    S3_SR +0x17, 0xfc,
    S3_SR +0x18, 0x40,
    S3_SR +0x19, 0x00,
    S3_SR +0x1a, 0x01,
    S3_SR +0x1b, 0x02,
    S3_SR +0x1c, 0x5d,
    S3_SR +0x1d, 0x00,
    S3_SR +0x1e, 0x00,
    S3_SR +0x1f, 0x00,
    S3_SR +0x20, 0x20,
    S3_SR +0x21, 0x21,
    S3_SR +0x22, 0x22,
    S3_SR +0x23, 0x23,
    S3_SR +0x24, 0x24,
    S3_SR +0x25, 0x25,
    S3_SR +0x26, 0x26,
    S3_SR +0x27, 0x04,
    S3_SR +0x28, 0xff,
    S3_SR +0x29, 0x00,
    S3_SR +0x2a, 0x2a,
    S3_SR +0x2b, 0x2b,
    S3_SR +0x2c, 0x2c,
    S3_SR +0x2d, 0x2d,
    S3_SR +0x2e, 0x2e,
    S3_SR +0x2f, 0x2f,
    S3_SR +0x30, 0x00,
    S3_SR +0x31, 0x06,
    S3_SR +0x32, 0x41,
    S3_SR +0x33, 0x67,
    S3_SR +0x34, 0x00,
    S3_SR +0x35, 0x00,
    S3_SR +0x36, 0x01,
    S3_SR +0x37, 0x52,
    S3_SR +0x38, 0x5d,
    S3_SR +0x39, 0x05,
    S3_SR +0x3a, 0x3a,
    S3_SR +0x3b, 0x3b,
    S3_SR +0x3c, 0x3c,
    S3_SR +0x3d, 0x00,
    S3_SR +0x3e, 0x3e,
    S3_SR +0x3f, 0x00,
    S3_SR +0x40, 0x40,
    S3_SR +0x41, 0x41,
    S3_SR +0x42, 0x42,
    S3_SR +0x43, 0x43,
    S3_SR +0x44, 0x44,
    S3_SR +0x45, 0x45,
    S3_SR +0x46, 0x46,
    S3_SR +0x47, 0x47,
    S3_SR +0x48, 0x48,
    S3_SR +0x49, 0x49,
    S3_SR +0x4a, 0x4a,
    S3_SR +0x4b, 0x4b,
    S3_SR +0x4c, 0x4c,
    S3_SR +0x4d, 0x4d,
    S3_SR +0x4e, 0x4e,
    S3_SR +0x4f, 0x4f,
    S3_SR +0x50, 0x00,
    S3_SR +0x51, 0x00,
    S3_SR +0x52, 0x00,
    S3_SR +0x53, 0x00,
    S3_SR +0x54, 0x00,
    S3_SR +0x55, 0x00,
    S3_SR +0x56, 0x00,
    S3_SR +0x57, 0x00,
    S3_SR +0x58, 0x00,
    S3_SR +0x59, 0x70,
    S3_SR +0x5a, 0x38,
    S3_SR +0x5b, 0x08,
    S3_SR +0x5c, 0x77,
    S3_SR +0x5d, 0x77,
    S3_SR +0x5e, 0x00,
    S3_SR +0x5f, 0x00,
    S3_SR +0x60, 0xff,
    S3_SR +0x61, 0xbf,
    S3_SR +0x62, 0xff,
    S3_SR +0x63, 0xff,
    S3_SR +0x64, 0xf7,
    S3_SR +0x65, 0xff,
    S3_SR +0x66, 0xff,
    S3_SR +0x67, 0xff,
    S3_SR +0x68, 0xff,
    S3_SR +0x69, 0xff,
    S3_SR +0x6a, 0xff,
    S3_SR +0x6b, 0xff,
    S3_SR +0x6c, 0xff,
    S3_SR +0x6d, 0xff,
    S3_SR +0x6e, 0x9b,
    S3_SR +0x6f, 0xbf,

    S3_AR + 0x00, 0x00,
    S3_AR + 0x01, 0x01,
    S3_AR + 0x02, 0x02,
    S3_AR + 0x03, 0x03,
    S3_AR + 0x04, 0x04,
    S3_AR + 0x05, 0x05,
    S3_AR + 0x06, 0x06,
    S3_AR + 0x07, 0x07,
    S3_AR + 0x08, 0x08,
    S3_AR + 0x09, 0x09,
    S3_AR + 0x0a, 0x0a,
    S3_AR + 0x0b, 0x0b,
    S3_AR + 0x0c, 0x0c,
    S3_AR + 0x0d, 0x0d,
    S3_AR + 0x0e, 0x0e,
    S3_AR + 0x0f, 0x0f,
    S3_AR + 0x10, 0x05,
    S3_AR + 0x11, 0x00,
    S3_AR + 0x12, 0x0f,
    S3_AR + 0x13, 0x08,
    S3_AR + 0x14, 0x00,
    
    S3_GR + 0x00, 0x00,
    S3_GR + 0x01, 0x00,
    S3_GR + 0x02, 0x00,
    S3_GR + 0x03, 0x00,
    S3_GR + 0x04, 0x00,
    S3_GR + 0x05, 0x10,
    S3_GR + 0x06, 0x0e,
    S3_GR + 0x07, 0x00,

    S3_CR + 0x00, 0x5f,
    S3_CR + 0x01, 0x4f,
    S3_CR + 0x02, 0x50,
    S3_CR + 0x03, 0x82,
    S3_CR + 0x04, 0x55,
    S3_CR + 0x05, 0x81,
    S3_CR + 0x06, 0xbf,
    S3_CR + 0x07, 0x1f,
    S3_CR + 0x08, 0x00,
    S3_CR + 0x09, 0x4f,
    S3_CR + 0x0a, 0x0d,
    S3_CR + 0x0b, 0x0e,
    S3_CR + 0x0c, 0x00,
    S3_CR + 0x0d, 0x00,
    S3_CR + 0x0e, 0x3f,
    S3_CR + 0x0f, 0xff,
    S3_CR + 0x10, 0x9c,
    S3_CR + 0x11, 0x0e,
    S3_CR + 0x12, 0x8f,
    S3_CR + 0x13, 0x28,
    S3_CR + 0x14, 0x1f,
    S3_CR + 0x15, 0x96,
    S3_CR + 0x16, 0xb9,
    S3_CR + 0x17, 0xa3,
    S3_CR + 0x18, 0xff,
    S3_CR + 0x19, 0xdf,
    S3_CR + 0x1a, 0xdf,
    S3_CR + 0x1b, 0xdf,
    S3_CR + 0x1c, 0xdf,
    S3_CR + 0x1d, 0xdf,
    S3_CR + 0x1e, 0xdf,
    S3_CR + 0x1f, 0xdf,
    S3_CR + 0x20, 0xdf,
    S3_CR + 0x21, 0x00,
/*    S3_CR + 0x22, 0x07, */
    S3_CR + 0x23, 0x00,
    S3_CR + 0x24, 0xdf,
    S3_CR + 0x25, 0xdf,
    S3_CR + 0x26, 0x00,
    S3_CR + 0x27, 0xdf,
    S3_CR + 0x28, 0xdf,
    S3_CR + 0x29, 0xdf,
    S3_CR + 0x2a, 0xdf,
    S3_CR + 0x2b, 0xdf,
    S3_CR + 0x2c, 0xdf,
    S3_CR + 0x2d, 0x8a,
    S3_CR + 0x2e, 0x22,
    S3_CR + 0x2f, 0x02,
    S3_CR + 0x30, 0xe1,
    S3_CR + 0x31, 0x05,
    S3_CR + 0x32, 0x40,
    S3_CR + 0x33, 0x08,
    S3_CR + 0x34, 0x00,
    S3_CR + 0x35, 0x00,
    S3_CR + 0x36, 0xbf,
    S3_CR + 0x37, 0x9b,
/*    S3_CR + 0x38, 0x7b, */
/*    S3_CR + 0x39, 0xb8, */
    S3_CR + 0x3a, 0x45,
    S3_CR + 0x3b, 0x5a,
    S3_CR + 0x3c, 0x10,
    S3_CR + 0x3d, 0x00,
    S3_CR + 0x3e, 0xfd,
    S3_CR + 0x3f, 0x00,
    S3_CR + 0x40, 0x00,
    S3_CR + 0x41, 0x92,
    S3_CR + 0x42, 0xc0,
    S3_CR + 0x43, 0x68,
    S3_CR + 0x44, 0xff,
    S3_CR + 0x45, 0xe8,
    S3_CR + 0x46, 0xff,
    S3_CR + 0x47, 0xff,
    S3_CR + 0x48, 0xf8,
    S3_CR + 0x49, 0xff,
    S3_CR + 0x4a, 0xfe,
    S3_CR + 0x4b, 0xff,
    S3_CR + 0x4c, 0xff,
    S3_CR + 0x4d, 0xff,
    S3_CR + 0x4e, 0xff,
    S3_CR + 0x4f, 0xff,
    S3_CR + 0x50, 0x00,
    S3_CR + 0x51, 0x00,
    S3_CR + 0x52, 0x00,
    S3_CR + 0x53, 0x00,
    S3_CR + 0x54, 0x00,
    S3_CR + 0x55, 0x00,
    S3_CR + 0x56, 0x00,
    S3_CR + 0x57, 0x00,
#if 0
    S3_CR + 0x58, 0x00,
    S3_CR + 0x59, 0xf0,
#endif
    S3_CR + 0x5a, 0x00,
    S3_CR + 0x5b, 0x00,
#if 0
    S3_CR + 0x5c, 0x00,
#endif
    S3_CR + 0x5d, 0x00,
    S3_CR + 0x5e, 0x00,
    S3_CR + 0x5f, 0x00,
    S3_CR + 0x60, 0x09,
    S3_CR + 0x61, 0x9d,
    S3_CR + 0x62, 0xff,
    S3_CR + 0x63, 0x00,
    S3_CR + 0x64, 0xfd,
    S3_CR + 0x65, 0x04,
    S3_CR + 0x66, 0x88,
    S3_CR + 0x67, 0x00,
    S3_CR + 0x68, 0x7f,
    S3_CR + 0x69, 0x00,
    S3_CR + 0x6a, 0x00,
    S3_CR + 0x6b, 0x00,
    S3_CR + 0x6c, 0x00,
    S3_CR + 0x6d, 0x11,
    S3_CR + 0x6e, 0xff,
    S3_CR + 0x6f, 0xfe,

    S3_CR + 0x70, 0x30,
    S3_CR + 0x71, 0xc0,
    S3_CR + 0x72, 0x07,
    S3_CR + 0x73, 0x1f,
    S3_CR + 0x74, 0x1f,
    S3_CR + 0x75, 0x1f,
    S3_CR + 0x76, 0x0f,
    S3_CR + 0x77, 0x1f,
    S3_CR + 0x78, 0x01,
    S3_CR + 0x79, 0x01,
    S3_CR + 0x7a, 0x1f,
    S3_CR + 0x7b, 0x1f,
    S3_CR + 0x7c, 0x17,
    S3_CR + 0x7d, 0x17,
    S3_CR + 0x7e, 0x17,
    S3_CR + 0x7f, 0xfd,
    S3_CR + 0x80, 0x00,
    S3_CR + 0x81, 0x92,
    S3_CR + 0x82, 0x10,
    S3_CR + 0x83, 0x07,
    S3_CR + 0x84, 0x42,
    S3_CR + 0x85, 0x00,
    S3_CR + 0x86, 0x00,
    S3_CR + 0x87, 0x00,
    S3_CR + 0x88, 0x10,
    S3_CR + 0x89, 0xfd,
    S3_CR + 0x8a, 0xfd,
    S3_CR + 0x8b, 0xfd,
    S3_CR + 0x8c, 0xfd,
    S3_CR + 0x8d, 0xfd,
    S3_CR + 0x8e, 0xfd,
    S3_CR + 0x8f, 0xfd,
    S3_CR + 0x90, 0x00,
    S3_CR + 0x91, 0x4f,
    S3_CR + 0x92, 0x10,
    S3_CR + 0x93, 0x00,
    S3_CR + 0x94, 0xfd,
    S3_CR + 0x95, 0xfd,
    S3_CR + 0x96, 0xfd,
    S3_CR + 0x97, 0xfd,
    S3_CR + 0x98, 0xfd,
    S3_CR + 0x99, 0xff,
    S3_CR + 0x9a, 0xfd,
    S3_CR + 0x9b, 0xff,
    S3_CR + 0x9c, 0xfd,
    S3_CR + 0x9d, 0xfd,
    S3_CR + 0x9e, 0xfd,
    S3_CR + 0x9f, 0xff,
    S3_CR + 0xa0, 0x0f,
#if 0
    S3_CR + 0xa1, 0x00,
    S3_CR + 0xa2, 0x00,
    S3_CR + 0xa3, 0x00,
    S3_CR + 0xa4, 0x55,
#endif
    S3_CR + 0xa5, 0x09,
    S3_CR + 0xa6, 0x20,
#if 0
    S3_CR + 0xa7, 0x00,
    S3_CR + 0xa8, 0x00,
    S3_CR + 0xa9, 0x00,
    S3_CR + 0xaa, 0x00,
    S3_CR + 0xab, 0x00,
    S3_CR + 0xac, 0x00,
    S3_CR + 0xad, 0x00,
    S3_CR + 0xae, 0x00,
    S3_CR + 0xaf, 0x00,
    S3_CR + 0xb0, 0xff,
#endif
    S3_CR + 0xb1, 0x0e,
#if 0
    S3_CR + 0xb2, 0x55,
    S3_CR + 0xb3, 0x00,
    S3_CR + 0xb4, 0x55,
    S3_CR + 0xb5, 0x00,
    S3_CR + 0xb6, 0x00,
#endif
    S3_CR + 0xb7, 0x84,
#if 0
    S3_CR + 0xb8, 0xff,
    S3_CR + 0xb9, 0xff,
    S3_CR + 0xba, 0xff,
    S3_CR + 0xbb, 0xff,
    S3_CR + 0xbc, 0xff,
    S3_CR + 0xbd, 0xff,
    S3_CR + 0xbe, 0xff,
    S3_CR + 0xbf, 0xff,
#endif

    S3_SR +0x15, 0x23,
    0xffff, 1,
    S3_SR +0x15, 0x03,
    0xffff, 1,
};

#define	S3_NUM_BIOS_REG	(sizeof (s3BiosReg) / sizeof (s3BiosReg[0]))

typedef struct _bios32Init {
    VGA16   offset;
    VGA32   value;
} s3Bios32Init;

s3Bios32Init s3Bios32Reg[] = {
    0x8168, 0x00000000,
    0x816c, 0x00000001,
    0x8170, 0x00000000,
    0x8174, 0x00000000,
    0x8178, 0x00000000,
    0x817c, 0x00000000,
#if 0
    0x8180, 0x00140000,
    0x8184, 0x00000000,
    0x8188, 0x00000000,
    0x8190, 0x00000000,
    0x8194, 0x00000000,
    0x8198, 0x00000000,
    0x819c, 0x00000000,
    0x81a0, 0x00000000,
#endif
    0x81c0, 0x00000000,
    0x81c4, 0x01fbffff,
    0x81c8, 0x00f7ffbf,
    0x81cc, 0x00f7ff00,
    0x81d0, 0x11ffff7f,
    0x81d4, 0x7fffffdf,
    0x81d8, 0xfdfff9ff,
    0x81e0, 0xfd000000,
    0x81e4, 0x00000000,
    0x81e8, 0x00000000,
    0x81ec, 0x00010000,
    0x81f0, 0x07ff057f,
    0x81f4, 0x07ff07ff,
    0x81f8, 0x00000000,
    0x81fc, 0x00000000,
    0x8200, 0x00000000,
    0x8204, 0x00000000,
    0x8208, 0x33000000,
    0x820c, 0x7f000000,
    0x8210, 0x80000000,
    0x8214, 0x00000000,
    0x8218, 0xffffffff,
    0x8300, 0xff007fef,
    0x8304, 0xfffdf7bf,
    0x8308, 0xfdfffbff,
};

#define S3_NUM_BIOS32_REG   (sizeof (s3Bios32Reg) / sizeof (s3Bios32Reg[0]))

/*
 * Initialize the card precisely as the bios does
 */
s3DoBiosInit (KdCardInfo *card)
{
    S3CardInfo	*s3c = card->driver;
    CARD32	*regs = (CARD32 *) s3c->registers;
    S3Vga	*s3vga = &s3c->s3vga;
    int		r;

    for (r = 0; r < S3_NUM_BIOS_REG; r++)
    {
	if (s3BiosReg[r].reg == 0xffff)
	    sleep (s3BiosReg[r].value);
	else
	    VgaStore (&s3vga->card, s3BiosReg[r].reg, s3BiosReg[r].value);
    }
    VgaStore (&s3vga->card, S3_SR+0x10, 0x22);
    VgaStore (&s3vga->card, S3_SR+0x11, 0x44);
    VgaStore (&s3vga->card, S3_SR+0x15, 0x01);
    sleep (1);
    VgaStore (&s3vga->card, S3_SR+0x15, 0x03);
    VgaStore (&s3vga->card, S3_CR+0x6f, 0xff);
    VgaStore (&s3vga->card, S3_CR+0x3f, 0x3f);
    sleep (1);
    VgaStore (&s3vga->card, S3_CR+0x3f, 0x00);
    VgaStore (&s3vga->card, S3_CR+0x6f, 0xfe);
    VgaInvalidate (&s3vga->card);
    for (r = 0; r < S3_NUM_BIOS32_REG; r++)
	regs[s3Bios32Reg[r].offset/4] = s3Bios32Reg[r].value;
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
    CARD8   streams_mode;

    s3Save (s3vga);
    if (!s3c->bios_initialized)
	s3DoBiosInit (card);

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
    /*
     * Preserve streams processor state
     */
    streams_mode = s3Get (s3vga, s3_streams_mode);
    s3SetImm (s3vga, s3_streams_mode, 3);
    save->global_bitmap_1 = s3->global_bitmap_1;
    save->global_bitmap_2 = s3->global_bitmap_2;
    save->adv_func_cntl = s3->adv_func_cntl;
    save->primary_bitmap_1 = s3->primary_bitmap_1;
    save->primary_bitmap_2 = s3->primary_bitmap_2;
    save->secondary_bitmap_1 = s3->secondary_bitmap_1;
    save->secondary_bitmap_2 = s3->secondary_bitmap_2;
    save->primary_stream_control = s3->primary_stream_control;
    save->blend_control = s3->blend_control;
    save->primary_stream_addr_0 = s3->primary_stream_addr_0;
    save->primary_stream_addr_1 = s3->primary_stream_addr_1;
    save->primary_stream_stride = s3->primary_stream_stride;
    save->primary_stream_xy = s3->primary_stream_xy;
    save->primary_stream_size = s3->primary_stream_size;
    save->primary_stream_mem = s3->primary_stream_mem;
    save->secondary_stream_xy = s3->secondary_stream_xy;
    save->secondary_stream_size = s3->secondary_stream_size;
    save->streams_fifo = s3->streams_fifo;
    s3SetImm (s3vga, s3_streams_mode, streams_mode);
    _s3SetBlank (s3, s3vga, FALSE);
}

/*
 * Enable the card for rendering.  Manipulate the initial settings
 * of the card here.
 */
int  s3CpuTimeout, s3AccelTimeout;

void
s3SetGlobalBitmap (ScreenPtr pScreen, int ma)
{
    KdScreenPriv(pScreen);
    s3ScreenInfo (pScreenPriv);
    
    if (s3s->current_ma != ma)
    {
	s3CardInfo (pScreenPriv);
	S3Vga   *s3vga = &s3c->s3vga;
	S3Ptr   s3 = s3c->s3;
	CARD32  gb1, gb2;
	int	    depth;
	int	    length;
	KdCheckSync (pScreen);
	switch (s3s->fb[ma].accel_bpp) {
	case 8:
	case 24:
	    length = 0;
	    break;
	case 16:
	    length = 1;
	    break;
	case 32:
	    length = 3;
	    break;
	}
	s3SetImm (s3vga, s3_pixel_length, length);
	gb1 = s3s->fb[ma].bitmap_offset;
	gb2 = ((1 << 0) |
	       (0 << 2) |
	       (1 << 3) |
	       ((s3s->fb[ma].accel_stride >> 4) << 4) |
	       (s3s->fb[ma].accel_bpp << 16) |
	       (0 << 24) |
	       (1 << 28));
	s3->global_bitmap_1 = gb1;
	s3->global_bitmap_2 = gb2;
	s3->global_bitmap_2 = gb2;
	s3s->current_ma = ma;
    }
}

Bool
s3Enable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdCardInfo	    *card = pScreenPriv->card;
    KdScreenInfo    *screen = pScreenPriv->screen;
    s3CardInfo (pScreenPriv);
    s3ScreenInfo (pScreenPriv);
    
    S3Vga   *s3vga = &s3c->s3vga;
    S3Ptr   s3 = s3c->s3;
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

    int	    primary_stream_l1[KD_MAX_FB];

    int	    v_total;
    int	    v_retrace_start;
    int	    v_retrace_end;
    int	    v_display_end;
    int	    v_blank_start;
    int	    v_blank_end;
    int	    v_blank_start_adjust = 0;
    int	    v_blank_end_adjust = 0;

    int	    h_blank_start_adjust = 0;
    int	    h_blank_end_adjust = 0;
    int	    h_sync_start_adjust = 0;
    int	    h_sync_end_adjust = 0;
    int	    h_start_fifo_fetch_adjust = 0;
    int	    h_sync_extend;
    int	    h_blank_extend;
    int	    i;
    CARD16  cursor_address;
    const KdMonitorTiming *t;
    int	    m, n, r;
    Bool    clock_double;
    int	    cpu_timeout;
    int	    accel_timeout;
    int	    bytes_per_ms;
    CARD32  control[2];
    int	    fb;
    int	    ma;
    
    s3s->primary_depth = screen->fb[s3s->fbmap[0]].depth;
    
    s3s->use_streams = TRUE;
    
    t = KdFindMode (screen, s3ModeSupported);
    
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
#define DEBUG_CLOCK
#ifdef DEBUG_CLOCK
    fprintf (stderr, "old clock %d, %d, %d (%d)\n", m, n, r, S3_CLOCK(m,n,r));
#endif
    clock_double = FALSE;
    s3GetClock (t->clock, &m, &n, &r, 511, 127, 4, 250000);
    if (S3_CLOCK(m,n,r) > S3_MAX_CLOCK && !s3s->use_streams)
	clock_double = TRUE;
    s3Set (s3vga, s3_clock_select, 3);
    s3Set (s3vga, s3_dclk_m, m);
    s3Set (s3vga, s3_dclk_n, n);
    s3Set (s3vga, s3_dclk_r, r);
#ifdef DEBUG_CLOCK
    fprintf (stderr, "new clock %d, %d, %d (%d)\n", m, n, r, S3_CLOCK(m,n,r));
#endif

    if (s3s->use_streams)
    {
	s3Set (s3vga, s3_streams_mode, 3);
	s3Set (s3vga, s3_enable_l1_parameter, 1);
    }
    else
    {
	s3Set (s3vga, s3_streams_mode, 0);
	s3Set (s3vga, s3_enable_l1_parameter, 0);
    }
    s3Set (s3vga, s3_flat_panel_output_control_1, 0);
    s3Set (s3vga, s3_flat_panel_output_control_2, 0);
    s3Set (s3vga, s3_select_graphics_mode, 1);
    s3Set (s3vga, s3_enable_blinking, 0);
    s3Set (s3vga, s3_enable_vga_16bit, 0);
    s3Set (s3vga, s3_enhanced_memory_mapping, 1);
    s3Set (s3vga, s3_enable_sff, 1);
    s3Set (s3vga, s3_enable_2d_access, 1);
    s3Set (s3vga, s3_2bk_cga, 1);
    s3Set (s3vga, s3_4bk_hga, 1);
    s3Set (s3vga, s3_v_total_double, 0);
    s3Set (s3vga, s3_address_16k_wrap, 1);
    s3Set (s3vga, s3_word_mode, 0);
    s3Set (s3vga, s3_byte_mode, 1);
    s3Set (s3vga, s3_hardware_reset, 1);
    s3Set (s3vga, s3_max_scan_line, 0);
    s3Set (s3vga, s3_linear_window_size, 3);
    s3Set (s3vga, s3_enable_linear, 1);
    s3Set (s3vga, s3_enable_2d_3d, 1);
    s3Set (s3vga, s3_refresh_control, 1);
    s3Set (s3vga, s3_disable_pci_read_bursts, 0);
    s3Set (s3vga, s3_pci_disconnect_enable, 1);
    s3Set (s3vga, s3_primary_load_control, 0);
    s3Set (s3vga, s3_secondary_load_control, 0);
    s3Set (s3vga, s3_pci_retry_enable, 1);
    s3Set (s3vga, s3_enable_256, 1);
    s3Set (s3vga, s3_border_select, 1);	/* eliminate white border */
    s3SetImm (s3vga, s3_lock_palette, 0);	/* unlock palette/border regs */
    s3Set (s3vga, s3_disable_v_retrace_int, 1);
    if (t->hpol == KdSyncPositive)
	s3Set (s3vga, s3_horz_sync_neg, 0);
    else
	s3Set (s3vga, s3_horz_sync_neg, 1);
    if (t->vpol == KdSyncPositive)
	s3Set (s3vga, s3_vert_sync_neg, 0);
    else
	s3Set (s3vga, s3_vert_sync_neg, 1);
    
    s3Set (s3vga, s3_dot_clock_8, 1);
    s3Set (s3vga, s3_enable_write_plane, 0xf);
    s3Set (s3vga, s3_extended_memory_access, 1);
    s3Set (s3vga, s3_sequential_addressing_mode, 1);
    s3Set (s3vga, s3_select_chain_4_mode, 1);
    s3Set (s3vga, s3_linear_addressing_control, 1);

    s3Set (s3vga, s3_enable_gamma_correction, 0);

    s3Set (s3vga, s3_enable_8_bit_luts, 1);
    
    s3Set (s3vga, s3_dclk_invert, 0);
    s3Set (s3vga, s3_enable_clock_double, 0);
    s3Set (s3vga, s3_dclk_over_2, 0);

    s3Set (s3vga, s3_delay_h_enable, 0);
    s3Set (s3vga, s3_sdclk_skew, 0);
    
    s3Set (s3vga, s3_dac_mask, 0xff);
    
#if 0
#ifdef DEBUG_CLOCK
    m = s3Get (s3vga, s3_mclk_m);
    n = s3Get (s3vga, s3_mclk_n);
    r = s3Get (s3vga, s3_mclk_r);
    fprintf (stderr, "old mclk %d, %d, %d (%d)\n", m, n, r, S3_CLOCK(m,n,r));
#endif
    
    s3GetClock (125282, &m, &n, &r, 127, 31, 3, 250000);

#ifdef DEBUG_CLOCK
    fprintf (stderr, "new mclk %d, %d, %d (%d)\n", m, n, r,S3_CLOCK(m,n,r));
#endif
    
    s3Set (s3vga, s3_mclk_m, m);
    s3Set (s3vga, s3_mclk_n, n);
    s3Set (s3vga, s3_mclk_r, r);
    
#ifdef DEBUG_CLOCK
    m = s3Get (s3vga, s3_eclk_m);
    n = s3Get (s3vga, s3_eclk_n);
    r = s3Get (s3vga, s3_eclk_r);
    fprintf (stderr, "old eclk %d, %d, %d (%d)\n", m, n, r, S3_CLOCK(m,n,r));
#endif
    
#define S3_ECLK	125282
    
    s3GetClock (S3_ECLK, &m, &n, &r, 127, 31, 3, 250000);

#ifdef DEBUG_CLOCK
    fprintf (stderr, "new eclk %d, %d, %d (%d)\n", m, n, r,S3_CLOCK(m,n,r));
#endif
    
    s3Set (s3vga, s3_eclk_m, m);
    s3Set (s3vga, s3_eclk_n, n);
    s3Set (s3vga, s3_eclk_r, r);
#endif
    
    /*
     * Compute character lengths for horizontal timing values
     */
    hactive = screen->width / 8;
    hblank /= 8;
    hfp /= 8;
    hbp /= 8;
    /*
     * Set pixel size, choose clock doubling mode
     */

    bytes_per_ms = 0;
    
    for (ma = 0; s3s->fbmap[ma] >= 0; ma++)
    {
	fb = s3s->fbmap[ma];
	s3s->fb[ma].accel_bpp = screen->fb[fb].bitsPerPixel;
	s3s->fb[ma].accel_stride = screen->fb[fb].pixelStride;
	s3s->fb[ma].bitmap_offset = screen->fb[fb].frameBuffer - s3c->frameBuffer;
	switch (s3s->fb[ma].accel_bpp) {
	case 8:
	    h_screen_off = hactive;
	    s3Set (s3vga, s3_pixel_length, 0);
	    s3Set (s3vga, s3_color_mode, 0);
	    control[ma] = 0;	
	    /*
	     * Set up for double-pixel mode, switch color modes,
	     * divide the dclk and delay h blank by 2 dclks
	     */
	    if (clock_double)
	    {
		s3Set (s3vga, s3_color_mode, 1);
		s3Set (s3vga, s3_dclk_over_2, 1);
		s3Set (s3vga, s3_enable_clock_double, 1);
		s3Set (s3vga, s3_h_skew, 1);
		h_blank_start_adjust = -3;
		h_blank_end_adjust = -4;
		s3Set (s3vga, s3_border_select, 0);
	    }
	    break;
	case 16:
	    h_screen_off = hactive * 2;
	    s3Set (s3vga, s3_pixel_length, 1);
	    if (screen->fb[fb].depth == 15)
		control[ma] = 3 << 24;
	    else
		control[ma] = 5 << 24;
	    if (clock_double)
	    {
		if (screen->fb[fb].depth == 15)
		    s3Set (s3vga, s3_color_mode, 3);
		else
		    s3Set (s3vga, s3_color_mode, 5);
		s3Set (s3vga, s3_dclk_over_2, 1);
		s3Set (s3vga, s3_enable_clock_double, 1);
		s3Set (s3vga, s3_border_select, 0);
		h_blank_start_adjust = 4;
		h_blank_end_adjust = -4;
	    }
	    else
	    {
		if (screen->fb[fb].depth == 15)
		    s3Set (s3vga, s3_color_mode, 2);
		else
		    s3Set (s3vga, s3_color_mode, 4);
		s3Set (s3vga, s3_dclk_over_2, 0);
		s3Set (s3vga, s3_enable_clock_double, 0);
		s3Set (s3vga, s3_delay_blank, 0);
	    }
	    break;
	case 24:
	    control[ma] = 6 << 24;
	    h_screen_off = hactive * 3;
	    s3s->fb[ma].accel_bpp = 8;
	    s3s->fb[ma].accel_stride = screen->fb[fb].pixelStride * 3;
	    break;
	case 32:
	    control[ma] = 7 << 24;
	    h_screen_off = hactive * 4;
	    s3Set (s3vga, s3_pixel_length, 3);
	    s3Set (s3vga, s3_color_mode, 0xd);
	    break;
	}
	bytes_per_ms += t->clock * (screen->fb[fb].bitsPerPixel / 8);
	primary_stream_l1[ma] = (screen->width * screen->fb[fb].bitsPerPixel / (8 * 8)) - 1;
    }

    /*
     * X server starts frame buffer at top of memory
     */
    s3Set (s3vga, s3_start_address, 0);
    
    /*
     * Set various registers to avoid snow on the screen
     */
    
    fprintf (stderr, "bytes_per_ms %d\n", bytes_per_ms);
    fprintf (stderr, "primary 0x%x master 0x%x command 0x%x lpb 0x%x cpu 0x%x 2d 0x%x\n",
	     s3Get (s3vga, s3_primary_stream_timeout),
	     s3Get (s3vga, s3_master_control_unit_timeout),
	     s3Get (s3vga, s3_command_buffer_timeout),
	     s3Get (s3vga, s3_lpb_timeout),
	     s3Get (s3vga, s3_cpu_timeout),
	     s3Get (s3vga, s3_2d_graphics_engine_timeout));

    /*
     *	Test:
     *	    accel	x11perf -line500
     *	    cpu		x11perf -circle500
     *
     *				    cpu	    accel
     *	1600x1200x32x85 (918000)    1	    1	    not enough
     *  1600x1200x32x75 (810000)    3	    2
     *	1600x1200x32x70 (756000)    4	    3
     *	1600x1200x32x60 (648000)    6	    5
     *
     *	1280x1024x32x85 (630000)    6	    4
     *	1280x1024x32x75 (540000)    a	    6
     *	1280x1024x32x60 (432000)    1f	    a
     *
     *	1152x900x32x85	(490000)    a	    6
     *	1152x900x32x75	(433000)    1f	    8
     *	1152x900x32x70	(401000)    1f	    a
     *	1152x900x32x66	(380000)    1f	    a
     *
     *	1024x768x32x85	(378000)    1f	    a
     *	1024x768x32x75	(315000)    1f	    b
     *	1024x768x32x70	(300000)    1f	    b
     *	1024x768x32x60	(260000)    1f	    12
     *
     *	800x600x32x85	(225000)    1f	    1a
     *	800x600x32x72	(200000)    1f	    1d
     *	800x600x32x75	(198000)    1f	    1d
     *
     *	1600x1200x16x85 (459000)    1f	    8
     *	1600x1200x16x75	(405000)    1f	    a
     *	1600x1200x16x70	(378000)    1f	    b
     *	1600x1200x16x60	(324000)    1f	    f
     *
     *  1280x1024x16x85 (315000)    1f	    12
     *	1280x1024x16x75 (270000)    1f	    16
     *	1280x1024x16x60 (216000)    1f	    1d
     *
     *	1600x1200x8x85	(229000)    1f	    1f
     *
     */
    
    if (s3CpuTimeout)
    {
	if (s3CpuTimeout < 0)
	    cpu_timeout = 0;
	else
	    cpu_timeout = s3CpuTimeout;
	if (s3AccelTimeout < 0)
	    accel_timeout = 0;
	else if (s3AccelTimeout)
	    accel_timeout = s3AccelTimeout;
	else
	    accel_timeout = s3CpuTimeout;
    }
    else if (bytes_per_ms >= 900000)
    {
	cpu_timeout = 0x01;
	accel_timeout = 0x01;
    }
    else if (bytes_per_ms >= 800000)
    {
	cpu_timeout = 0x03;
	accel_timeout = 0x02;
    }
    else if (bytes_per_ms >= 700000)
    {
	cpu_timeout = 0x04;
	accel_timeout = 0x03;
    }
    else if (bytes_per_ms >= 600000)
    {
	cpu_timeout = 0x06;
	accel_timeout = 0x04;
    }
    else if (bytes_per_ms >= 475000)
    {
	cpu_timeout = 0x0a;
	accel_timeout = 0x06;
    }
    else if (bytes_per_ms >= 425000)
    {
	cpu_timeout = 0x1f;
	accel_timeout = 0x8;
    }
    else if (bytes_per_ms >= 300000)
    {
	cpu_timeout = 0x1f;
	accel_timeout = 0x0a;
    }
    else if (bytes_per_ms >= 250000)
    {
	cpu_timeout = 0x1f;
	accel_timeout = 0x12;
    }
    else if (bytes_per_ms >= 200000)
    {
	cpu_timeout = 0x1f;
	accel_timeout = 0x1a;
    }
    else
    {
	cpu_timeout = 0x1f;
	accel_timeout = 0x1f;
    }
	
    fprintf (stderr, "cpu 0x%x accel 0x%x\n", cpu_timeout, accel_timeout);
    
    s3Set (s3vga, s3_primary_stream_timeout, 0xc0);
    s3Set (s3vga, s3_master_control_unit_timeout, 0xf);
    s3Set (s3vga, s3_command_buffer_timeout, 0x1f);
    s3Set (s3vga, s3_lpb_timeout, 0xf);
    s3Set (s3vga, s3_2d_graphics_engine_timeout, accel_timeout);
    s3Set (s3vga, s3_cpu_timeout, cpu_timeout);
    
    s3Set (s3vga, s3_fifo_fetch_timing, 1);
    s3Set (s3vga, s3_fifo_drain_delay, 2);

    /*
     * Compute horizontal register values from timings
     */
    h_total = hactive + hblank - 5;
    h_display_end = hactive - 1;
    
    h_sync_start = hactive + hfp + h_sync_start_adjust;
    h_sync_end = hactive + hblank - hbp + h_sync_end_adjust;
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

    h_start_fifo_fetch += h_start_fifo_fetch_adjust;
    if (h_blank_end - h_blank_start >= 0x40)
	h_blank_extend = 1;
    else
	h_blank_extend = 0;
    
    if (h_sync_end - h_sync_start >= 0x20)
	h_sync_extend = 1;
    else
	h_sync_extend = 0;
    
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
    
    s3Set (s3vga, s3_dac_power_saving_disable, 0);
    s3Set (s3vga, s3_dac_power_up_time, hactive + hblank);
    
    s3Set (s3vga, s3_primary_stream_l1, primary_stream_l1[0]);

    s3Set (s3vga, s3_streams_fifo_delay, 0);
    
    v_total = vactive + vblank - 2;
    v_display_end = vactive - 1;
    
    v_blank_start = vactive - 1 + v_blank_start_adjust;
    v_blank_end = v_blank_start + vblank - 1 + v_blank_end_adjust;
    
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
	cursor_address = (s3s->cursor_base - s3c->frameBuffer) / 1024;

	s3Set (s3vga, s3_cursor_address, cursor_address);
	s3Set (s3vga, s3_cursor_ms_x11, 0);
	s3Set (s3vga, s3_cursor_enable, 1);
    }
    else
	s3Set (s3vga, s3_cursor_enable, 0);
    
#define MAKE_GBF(bds,be,stride,bpp,tile) (\
						  ((bds) << 0) | \
						  ((be) << 3) | \
						  ((stride) << 4) | \
						  ((bpp) << 16) | \
						  ((tile) << 24))
    /*
     * Set accelerator
     */
    switch (screen->width) {
#if 0
    case 640: s3Set (s3vga, s3_ge_screen_width, 1); break;
    case 800: s3Set (s3vga, s3_ge_screen_width, 2); break;
    case 1024:        s3Set (s3vga, s3_ge_screen_width, 0); break;
    case 1152:        s3Set (s3vga, s3_ge_screen_width, 4); break;
    case 1280:        s3Set (s3vga, s3_ge_screen_width, 3); break;
    case 1600:        s3Set (s3vga, s3_ge_screen_width, 6); break;
#endif
    default:
	s3Set (s3vga, s3_ge_screen_width, 7);   /* use global bitmap descriptor */
    }
    
#if 0
    crtc->l_parm_0_7 = screen->width / 4;	/* Undocumented. */
#endif

    /*
     * Set DPMS to normal
     */
    s3Set (s3vga, s3_hsync_control, 0);
    s3Set (s3vga, s3_vsync_control, 0);
    
    _s3SetBlank (s3, s3vga, TRUE);
    if (s3s->use_streams)
	s3Set (s3vga, s3_primary_stream_definition, 1);
    else
	s3Set (s3vga, s3_primary_stream_definition, 0);

    VgaFlush(&s3vga->card);
    VgaSetImm (&s3vga->card, s3_clock_load_imm, 1);
    VgaSetImm(&s3vga->card, s3_clock_load_imm, 0);


    if (s3s->use_streams)
    {
	fb = s3s->fbmap[0];
	s3->primary_stream_control = control[0];
	s3->primary_stream_addr_0 =
	s3->primary_stream_addr_1 = s3s->fb[0].bitmap_offset;
	s3->primary_stream_stride = screen->fb[fb].byteStride;
	s3->primary_stream_xy = (1 << 16) | 1;
	s3->primary_stream_size = ((screen->fb[fb].pixelStride - 1) << 16) | screen->height;
	s3->primary_stream_mem = (screen->fb[fb].byteStride * screen->height) / 8 - 1;
	if (s3s->fbmap[1] >= 0)
	{
	    fb = s3s->fbmap[1];
	    s3->blend_control = 5 << 24;
	    if (s3s->fb[0].accel_bpp == 8)
		s3->chroma_key_control = 0x33000000 | s3s->fb[0].chroma_key;
	    else
		s3->chroma_key_control = 0x13010101;
	    s3->secondary_stream_control = control[1] | screen->width;
	    s3->secondary_stream_h_scale = (1 << 15);
	    s3->color_adjustment = 0;
	    s3->secondary_stream_vscale = (1 << 15);
	    s3->secondary_stream_vinit = 0;
	    s3->secondary_stream_mbuf = 0;
	    s3->secondary_stream_addr_0 =
	    s3->secondary_stream_addr_1 = s3s->fb[1].bitmap_offset;
	    s3->secondary_stream_stride = screen->fb[fb].byteStride;
	    s3->secondary_stream_scount = screen->height;
	    s3->secondary_stream_xy = (1 << 16) | 1;
	    s3->secondary_stream_size = ((screen->fb[fb].pixelStride - 1) << 16) | screen->height;
	    s3->secondary_stream_mem = (1 << 22) | ((screen->fb[fb].byteStride * screen->height) / 8 - 1);
	}
	else
	{
	    s3->blend_control = 1 << 24;
	    s3->secondary_stream_xy = 0x07ff07ff;
	    s3->secondary_stream_size = 0x00010001;
	}
	s3->streams_fifo = (0x20 << 11) | (0x20 << 5) | 0x2;
    }
    s3->mult_misc_read_sel = (((1 << 9) |
			       (1 << 11) |
			       (0xe << 12)) |
			      (((0xe << 0) |
				(0xf << 12)) << 16));
    
    s3->cmd_overflow_buf_ptr = (1 << 3);
    s3->bci_power_management = (1 << 9);
    s3->adv_func_cntl = (3 << 8) | (1 << 4) | (1 << 2) | 1;
    s3->primary_bitmap_1 = 0;
    s3->primary_bitmap_2 = 0;
    s3->secondary_bitmap_1 = 0;
    s3->secondary_bitmap_2 = 0;
    s3s->current_ma = -1;
    _s3SetBlank (s3, s3vga, FALSE);
#if 0
    {
	VGA16	r;
	static CARD32	streams[][2] = {
	    /* PCI registers */
	    0x8000, 0x8024,
	    0x802c, 0x8034,
	    0x803c, 0x8040,
#if 0
	    0x8080, 0x808c,	/* AGP */
#endif
	    0x80dc, 0x80e0,
	    
	    /* 2D registers */
	    0x8168, 0x8188,
	    0x8190, 0x81a0,
	    0x81c0, 0x81d8,
	    0x81e0, 0x8218,
	    0x8300, 0x8308,
	    0x8504, 0x8510,

	    /* LPB/VIP registers */
	    0xff00, 0xff18,
	    0xff20, 0xff38,
	    0xff40, 0xff40,
	    0xff70, 0xff78,
	    0xff8c, 0xffa0,

#if 0
	    /* 3D registers */
	    0x48508, 0x48508,
	    0x48528, 0x48528,
	    0x48548, 0x48548,
	    0x48584, 0x485f0,
#endif

	    /* motion compensation registers */
	    0x48900, 0x48924,
#if 0
	    0x48928, 0x48928,
#endif

	    /* Mastered data transfer registers */
	    0x48a00, 0x48a1c,

	    /* configuation/status registers */
	    0x48c00, 0x48c18,
	    0x48c20, 0x48c24,
	    0x48c40, 0x48c50,
	    0x48c60, 0x48c64,

	    0, 0,
	};
#ifdef PHOENIX
#undef stderr
#define stderr stdout
#endif
	CARD32	    *regs = (CARD32 *) s3c->registers;
	int	    i;
	CARD32	    reg;


	for (r = S3_SR + 0; r < S3_SR + S3_NSR; r++)
	    fprintf (stderr, "SR%02x = %02x\n", r-S3_SR, VgaFetch (&s3vga->card, r));
	for (r = S3_GR + 0; r < S3_GR + S3_NGR; r++)
	    fprintf (stderr, "GR%02x = %02x\n", r-S3_GR, VgaFetch (&s3vga->card, r));
	for (r = S3_AR + 0; r < S3_AR + S3_NAR; r++)
	    fprintf (stderr, "AR%02x = %02x\n", r-S3_AR, VgaFetch (&s3vga->card, r));
	for (r = S3_CR + 0; r < S3_CR + S3_NCR; r++)
	    fprintf (stderr, "CR%02x = %02x\n", r-S3_CR, VgaFetch (&s3vga->card, r));
	for (r = S3_DAC + 0; r < S3_DAC + S3_NDAC; r++)
	    fprintf (stderr, "DAC%02x = %02x\n", r-S3_DAC, VgaFetch (&s3vga->card, r));
	fprintf (stderr, "MISC_OUT = %02x\n", VgaFetch (&s3vga->card, S3_MISC_OUT));
	fprintf (stderr, "INPUT_STATUS = %02x\n", VgaFetch (&s3vga->card, S3_INPUT_STATUS_1));


	for (i = 0; streams[i][0]; i++)
	{
	    for (reg = streams[i][0]; reg <= streams[i][1]; reg += 4)
		fprintf (stderr, "0x%4x: 0x%08x\n", reg, regs[reg/4]);
	}
    }
#endif
    return TRUE;
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
    CARD8	streams_mode;

    _s3SetBlank (s3, s3vga, TRUE);
    /* streams processor state */
    streams_mode = s3Get (s3vga, s3_streams_mode);
    s3SetImm (s3vga, s3_streams_mode, 3);
    s3->global_bitmap_1 = save->global_bitmap_1;
    s3->global_bitmap_2 = save->global_bitmap_2;
    s3->adv_func_cntl = save->adv_func_cntl;
    s3->primary_bitmap_1 = save->primary_bitmap_1;
    s3->primary_bitmap_2 = save->primary_bitmap_2;
    s3->secondary_bitmap_1 = save->secondary_bitmap_1;
    s3->secondary_bitmap_2 = save->secondary_bitmap_2;
    s3->primary_stream_control = save->primary_stream_control;
    s3->blend_control = save->blend_control;
    s3->primary_stream_addr_0 = save->primary_stream_addr_0;
    s3->primary_stream_addr_0 = save->primary_stream_addr_0;
    s3->primary_stream_stride = save->primary_stream_stride;
    s3->primary_stream_xy = save->primary_stream_xy;
    s3->primary_stream_size = save->primary_stream_size;
    s3->primary_stream_mem = save->primary_stream_mem;
    s3->secondary_stream_xy = save->secondary_stream_xy;
    s3->secondary_stream_size = save->secondary_stream_size;
    s3->streams_fifo = save->streams_fifo;
    s3SetImm (s3vga, s3_streams_mode, streams_mode);
    /* graphics engine state */
    s3->alt_mix = save->alt_mix;
    s3->write_mask = save->write_mask;
    s3->fg = save->fg;
    s3->bg = save->bg;
    /* XXX should save and restore real values? */
    s3->scissors_tl = 0x00000000;
    s3->scissors_br = 0x0fff0fff;
    
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

Bool
s3InitScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdCardInfo	    *card = pScreenPriv->card;
    KdScreenInfo    *screen = pScreenPriv->screen;
    s3CardInfo (pScreenPriv);
    s3ScreenInfo (pScreenPriv);
    int		ma, fb;

    if (screen->fb[1].depth)
    {
	FbOverlayScrPrivPtr pScrPriv = fbOverlayGetScrPriv(pScreen);

	for (ma = 0; s3s->fbmap[ma] >= 0; ma++)
	{
	    fb = s3s->fbmap[ma];
	    pScrPriv->layer[fb].key = s3s->fb[ma].chroma_key;
	}
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
    s3InitScreen,
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
    s3DrawSync,
    s3DrawDisable,
    s3DrawFini,
    s3GetColors,
    s3PutColors,
};
