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
/* $XFree86: xc/programs/Xserver/hw/kdrive/fbdev/fbdev.c,v 1.5 2000/09/03 05:11:17 keithp Exp $ */

#include "fbdev.h"

Bool
fbdevInitialize (KdCardInfo *card, FbdevPriv *priv)
{
    int	    k;
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

    priv->fb = (unsigned char *) mmap ((caddr_t) NULL,
				       priv->fix.smem_len,
				       PROT_READ|PROT_WRITE,
				       MAP_SHARED,
				       priv->fd, 0);
    
    if (priv->fb == (char *)-1) 
    {
        perror("ERROR: mmap framebuffer fails!");
	close (priv->fd);
	return FALSE;
    }
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
	screen->fb[0].visuals = (1 << TrueColor);
	screen->fb[0].redMask = FbStipMask (priv->var.red.offset, priv->var.red.length);
	screen->fb[0].greenMask = FbStipMask (priv->var.green.offset, priv->var.green.length);
	screen->fb[0].blueMask = FbStipMask (priv->var.blue.offset, priv->var.blue.length);
	allbits = screen->fb[0].redMask | screen->fb[0].greenMask | screen->fb[0].blueMask;
	depth = 32;
	while (depth && !(allbits & (1 << (depth - 1))))
	    depth--;
	screen->fb[0].depth = depth;
	break;
    default:
	return FALSE;
	break;
    }
    screen->rate = 72;
    scrpriv->rotate = ((priv->var.xres < priv->var.yres) != 
		       (screen->width < screen->height));
    screen->fb[0].depth = priv->var.bits_per_pixel;
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
	return KdShadowScreenInit (screen);
    }
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

Bool
fbdevInitScreen (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    FbdevPriv		*priv = pScreenPriv->card->driver;
    FbdevScrPriv	*scrpriv = pScreenPriv->screen->driver;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;

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
    KdSetMouseMatrix (&m);
    return TRUE;
}

Bool
fbdevDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    FbdevPriv	*priv = pScreenPriv->card->driver;

#ifdef FBIOPUT_POWERMODE
    if (!ioctl (priv->fd, FBIOPUT_POWERMODE, &mode))
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
}

void
fbdevCardFini (KdCardInfo *card)
{
    int	k;
    FbdevPriv	*priv = card->driver;
    
    munmap (priv->fb, priv->fix.smem_len);
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
