/*
 * Id: fbdev.c,v 1.1 1999/11/02 03:54:46 keithp Exp $
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
/* $XFree86: xc/programs/Xserver/hw/kdrive/fbdev/fbdev.c,v 1.7 2000/09/22 06:25:08 keithp Exp $ */

#include "fbdev.h"

/* this code was used to debug MSB 24bpp code on a 16bpp frame buffer */
#undef FAKE24_ON_16

Bool
fbdevInitialize (KdCardInfo *card, FbdevPriv *priv)
{
    int		    k;
    unsigned long   off;
    if ((priv->fd = open("/dev/fb0", O_RDWR)) < 0) {
	perror("Error opening /dev/fb0\n");
	return FALSE;
    }
    if ((k=ioctl(priv->fd, FBIOGET_FSCREENINFO, &priv->fix)) < 0) {
	perror("Error with /dev/fb ioctl FIOGET_FSCREENINFO");
	close (priv->fd);
	return FALSE;
    }
    if ((k=ioctl(priv->fd, FBIOGET_VSCREENINFO, &priv->var)) < 0) {
	perror("Error with /dev/fb ioctl FIOGET_VSCREENINFO");
	close (priv->fd);
	return FALSE;
    }

    priv->fb_base = (unsigned char *) mmap ((caddr_t) NULL,
					    priv->fix.smem_len,
					    PROT_READ|PROT_WRITE,
					    MAP_SHARED,
					    priv->fd, 0);
    
    if (priv->fb_base == (char *)-1) 
    {
        perror("ERROR: mmap framebuffer fails!");
	close (priv->fd);
	return FALSE;
    }
    off = (unsigned long) priv->fix.smem_start % (unsigned long) getpagesize();
    priv->fb = priv->fb_base + off;
    return TRUE;
}

Bool
fbdevCardInit (KdCardInfo *card)
{
    int		k;
    char	*pixels;
    FbdevPriv	*priv;

    priv = (FbdevPriv *) xalloc (sizeof (FbdevPriv));
    if (!priv)
	return FALSE;
    
    if (!fbdevInitialize (card, priv))
    {
	xfree (priv);
	return FALSE;
    }
    card->driver = priv;
    
    return TRUE;
}

Bool
fbdevScreenInitialize (KdScreenInfo *screen, FbdevScrPriv *scrpriv)
{
    FbdevPriv	*priv = screen->card->driver;
    Pixel	allbits;
    int		depth;
    Bool	rotate;
    Bool	shadow;
#ifdef FAKE24_ON_16
    Bool	fake24;
#endif

    depth = priv->var.bits_per_pixel;
    
    switch (priv->fix.visual) {
    case FB_VISUAL_PSEUDOCOLOR:
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
    case FB_VISUAL_TRUECOLOR:
    case FB_VISUAL_DIRECTCOLOR:
	screen->fb[0].visuals = (1 << TrueColor);
#define Mask(o,l)   (((1 << l) - 1) << o)
	screen->fb[0].redMask = Mask (priv->var.red.offset, priv->var.red.length);
	screen->fb[0].greenMask = Mask (priv->var.green.offset, priv->var.green.length);
	screen->fb[0].blueMask = Mask (priv->var.blue.offset, priv->var.blue.length);
	allbits = screen->fb[0].redMask | screen->fb[0].greenMask | screen->fb[0].blueMask;
	depth = 32;
	while (depth && !(allbits & (1 << (depth - 1))))
	    depth--;
	break;
    default:
	return FALSE;
	break;
    }
    screen->rate = 72;
    scrpriv->rotate = ((priv->var.xres < priv->var.yres) != 
		       (screen->width < screen->height));
#ifdef FAKE24_ON_16
    if (screen->fb[0].depth == 24 && screen->fb[0].bitsPerPixel == 24 &&
	priv->var.bits_per_pixel == 16)
    {
	fake24 = TRUE;
	scrpriv->shadow = TRUE;
	scrpriv->rotate = FALSE;
	screen->fb[0].redMask = 0xff0000;
	screen->fb[0].greenMask = 0x00ff00;
	screen->fb[0].blueMask = 0x0000ff;
	screen->width = priv->var.xres;
	screen->height = priv->var.yres;
	screen->softCursor = TRUE;
    }
    else
#endif
    {
	screen->fb[0].depth = depth;
	screen->fb[0].bitsPerPixel = priv->var.bits_per_pixel;
	if (!scrpriv->rotate)
	{
	    screen->width = priv->var.xres;
	    screen->height = priv->var.yres;
	    screen->fb[0].byteStride = priv->fix.line_length;
	    screen->fb[0].pixelStride = (priv->fix.line_length * 8 / 
					 priv->var.bits_per_pixel);
	    screen->fb[0].frameBuffer = (CARD8 *) (priv->fb);
	    return TRUE;
	}
	else
	{
	    screen->width = priv->var.yres;
	    screen->height = priv->var.xres;
	    screen->softCursor = TRUE;
	}
    }
    if (scrpriv->rotate)
	scrpriv->shadow = TRUE;
    if (scrpriv->shadow)
	return KdShadowScreenInit (screen);
    return TRUE;
}

Bool
fbdevScreenInit (KdScreenInfo *screen)
{
    FbdevScrPriv *scrpriv;

    scrpriv = xalloc (sizeof (FbdevScrPriv));
    if (!scrpriv)
	return FALSE;
    if (!fbdevScreenInitialize (screen, scrpriv))
    {
	xfree (scrpriv);
	return FALSE;
    }
    screen->driver = scrpriv;
    return TRUE;
}
    
void *
fbdevWindowLinear (ScreenPtr	pScreen,
		   CARD32	row,
		   CARD32	offset,
		   int		mode,
		   CARD32	*size)
{
    KdScreenPriv(pScreen);
    FbdevPriv	    *priv = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled)
	return 0;
    *size = priv->fix.line_length;
    return (CARD8 *) priv->fb + row * priv->fix.line_length + offset;
}

#ifdef FAKE24_ON_16
void
fbdevUpdateFake24 (ScreenPtr pScreen,
		   PixmapPtr pShadow,
		   RegionPtr damage)
{
    shadowScrPriv(pScreen);
    int		nbox = REGION_NUM_RECTS (damage);
    BoxPtr	pbox = REGION_RECTS (damage);
    FbBits	*shaBits;
    CARD8	*shaBase, *shaLine, *sha;
    CARD16	s;
    FbStride	shaStride;
    int		scrBase, scrLine, scr;
    int		shaBpp;
    int		x, y, w, h, width;
    int         i;
    CARD16	*winBase, *winLine, *win;
    CARD32      winSize;

    fbGetDrawable (&pShadow->drawable, shaBits, shaStride, shaBpp);
    shaStride = shaStride * sizeof (FbBits) / sizeof (CARD8);
    shaBase = (CARD8 *) shaBits;
    while (nbox--)
    {
	x = pbox->x1;
	y = pbox->y1;
	w = (pbox->x2 - pbox->x1);
	h = pbox->y2 - pbox->y1;

	shaLine = shaBase + y * shaStride + x * 3;
				   
	while (h--)
	{
	    winSize = 0;
	    scrBase = 0;
	    width = w;
	    scr = x;
	    sha = shaLine;
	    while (width) {
		/* how much remains in this window */
		i = scrBase + winSize - scr;
		if (i <= 0 || scr < scrBase)
		{
		    winBase = (CARD16 *) (*pScrPriv->window) (pScreen,
							      y,
							      scr * sizeof (CARD16),
							      SHADOW_WINDOW_WRITE,
							      &winSize);
		    if(!winBase)
			return;
		    scrBase = scr;
		    winSize /= sizeof (CARD16);
		    i = winSize;
		}
		win = winBase + (scr - scrBase);
		if (i > width)
		    i = width;
		width -= i;
		scr += i;
		while (i--)
		{
#if IMAGE_BYTE_ORDER == MSBFirst
		    *win++ = ((sha[2] >> 3) | 
			      ((sha[1] & 0xf8) << 2) |
			      ((sha[0] & 0xf8) << 7));
#else
		    *win++ = ((sha[0] >> 3) | 
			      ((sha[1] & 0xfc) << 3) |
			      ((sha[2] & 0xf8) << 8));
#endif
		    sha += 3;
		}
	    }
	    shaLine += shaStride;
	    y++;
	}
	pbox++;
    }
}
#endif /* FAKE24_ON_16 */

Bool
fbdevInitScreen (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    FbdevPriv		*priv = pScreenPriv->card->driver;
    FbdevScrPriv	*scrpriv = pScreenPriv->screen->driver;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;

    if (scrpriv->shadow)
    {
        window = fbdevWindowLinear;
#ifdef FAKE24_ON_16
	if (pScreenPriv->screen->fb[0].bitsPerPixel == 24 && priv->var.bits_per_pixel == 16)
	{
	    update = fbdevUpdateFake24;
	}
	else
#endif /* FAKE24_ON_16 */
	{
	    update = shadowUpdatePacked;
	    if (scrpriv->rotate)
	    {
		window = fbdevWindowLinear;
		switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
		case 8:
		    update = shadowUpdateRotate8; break;
		case 16:
		    update = shadowUpdateRotate16; break;
		case 32:
		    update = shadowUpdateRotate32; break;
		}
	    }
	}
	return KdShadowInitScreen (pScreen, update, window);
    }
    return TRUE;
}

void
fbdevPreserve (KdCardInfo *card)
{
}

Bool
fbdevEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    FbdevPriv	*priv = pScreenPriv->card->driver;
    FbdevScrPriv	*scrpriv = pScreenPriv->screen->driver;
    int			k;
    KdMouseMatrix	m;

    priv->var.activate = FB_ACTIVATE_NOW|FB_CHANGE_CMAP_VBL;
    
    /* display it on the LCD */
    k = ioctl (priv->fd, FBIOPUT_VSCREENINFO, &priv->var);
    if (k < 0)
    {
	perror ("FBIOPUT_VSCREENINFO");
	return FALSE;
    }
    if (scrpriv->rotate)
    {
	m.matrix[0][0] = 0; m.matrix[0][1] = 1; m.matrix[0][2] = 0;
	m.matrix[1][0] = -1; m.matrix[1][1] = 0; m.matrix[1][2] = pScreen->height - 1;
    }
    else
    {
	m.matrix[0][0] = 1; m.matrix[0][1] = 0; m.matrix[0][2] = 0;
	m.matrix[1][0] = 0; m.matrix[1][1] = 1; m.matrix[1][2] = 0;
    }
    if (priv->fix.visual == FB_VISUAL_DIRECTCOLOR)
    {
	struct fb_cmap	cmap;
	int		i;

	for (i = 0; 
	     i < (1 << priv->var.red.length) ||
	     i < (1 << priv->var.green.length) ||
	     i < (1 << priv->var.blue.length); i++)
	{
	    priv->red[i] = i * 65535 / ((1 << priv->var.red.length) - 1);
	    priv->green[i] = i * 65535 / ((1 << priv->var.green.length) - 1);
	    priv->blue[i] = i * 65535 / ((1 << priv->var.blue.length) - 1);
	}
	cmap.start = 0;
	cmap.len = i;
	cmap.red = &priv->red[0];
	cmap.green = &priv->green[0];
	cmap.blue = &priv->blue[0];
	cmap.transp = 0;
	ioctl (priv->fd, FBIOPUTCMAP, &cmap);
    }
    KdSetMouseMatrix (&m);
    return TRUE;
}

Bool
fbdevDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    FbdevPriv	*priv = pScreenPriv->card->driver;

#ifdef FBIOPUT_POWERMODE
    if (ioctl (priv->fd, FBIOPUT_POWERMODE, &mode) >= 0)
	return TRUE;
#endif
#ifdef FBIOBLANK
    if (ioctl (priv->fd, FBIOBLANK, mode ? mode + 1 : 0) >= 0)
	return TRUE;
#endif
    return FALSE;
}

void
fbdevDisable (ScreenPtr pScreen)
{
}

void
fbdevRestore (KdCardInfo *card)
{
}

void
fbdevScreenFini (KdScreenInfo *screen)
{
    FbdevScrPriv	*scrpriv = screen->driver;
    
    if (scrpriv->shadow)
	KdShadowScreenFini (screen);
}

void
fbdevCardFini (KdCardInfo *card)
{
    int	k;
    FbdevPriv	*priv = card->driver;
    
    munmap (priv->fb_base, priv->fix.smem_len);
    close (priv->fd);
    xfree (priv);
}

void
fbdevGetColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    FbdevPriv	    *priv = pScreenPriv->card->driver;
    struct fb_cmap  cmap;
    int		    p;
    int		    k;
    int		    min, max;

    min = 256;
    max = 0;
    for (k = 0; k < n; k++)
    {
	if (pdefs[k].pixel < min)
	    min = pdefs[k].pixel;
	if (pdefs[k].pixel > max)
	    max = pdefs[k].pixel;
    }
    cmap.start = min;
    cmap.len = max - min + 1;
    cmap.red = &priv->red[min];
    cmap.green = &priv->green[min];;
    cmap.blue = &priv->blue[min];
    cmap.transp = 0;
    k = ioctl (priv->fd, FBIOGETCMAP, &cmap);
    if (k < 0)
    {
	perror ("can't get colormap");
	return;
    }
    while (n--)
    {
	p = pdefs->pixel;
	pdefs->red = priv->red[p];
	pdefs->green = priv->green[p];
	pdefs->blue = priv->blue[p];
	pdefs++;
    }
}

void
fbdevPutColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    FbdevPriv	*priv = pScreenPriv->card->driver;
    struct fb_cmap  cmap;
    int		    p;
    int		    min, max;

    min = 256;
    max = 0;
    while (n--)
    {
	p = pdefs->pixel;
	priv->red[p] = pdefs->red;
	priv->green[p] = pdefs->green;
	priv->blue[p] = pdefs->blue;
	if (p < min)
	    min = p;
	if (p > max)
	    max = p;
	pdefs++;
    }
    cmap.start = min;
    cmap.len = max - min + 1;
    cmap.red = &priv->red[min];
    cmap.green = &priv->green[min];
    cmap.blue = &priv->blue[min];
    cmap.transp = 0;
    ioctl (priv->fd, FBIOPUTCMAP, &cmap);
}
