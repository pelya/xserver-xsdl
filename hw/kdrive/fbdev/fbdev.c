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
/* $XFree86: xc/programs/Xserver/hw/kdrive/fbdev/fbdev.c,v 1.25 2001/07/16 19:48:00 keithp Exp $ */

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

#define FBDEV_KLUDGE_FORMAT
#ifdef FBDEV_KLUDGE_FORMAT
static Pixel
fbdevMakeContig (Pixel orig, Pixel others)
{
    Pixel   low;

    low = lowbit (orig) >> 1;
    while (low && (others & low) == 0)
    {
	orig |= low;
	low >>= 1;
    }
    return orig;
}
#endif

Bool
fbdevScreenInitialize (KdScreenInfo *screen, FbdevScrPriv *scrpriv)
{
    FbdevPriv	*priv = screen->card->driver;
    Pixel	allbits;
    int		depth;
    Bool	shadow;
    Bool	gray;
#ifdef FAKE24_ON_16
    Bool	fake24;
#endif

    depth = priv->var.bits_per_pixel;
    gray = priv->var.grayscale;
    
    switch (priv->fix.visual) {
    case FB_VISUAL_PSEUDOCOLOR:
	if (gray)
	{
	    screen->fb[0].visuals = ((1 << StaticGray) |
				     (1 << GrayScale));
	}
	else
	{
	    screen->fb[0].visuals = ((1 << StaticGray) |
			       (1 << GrayScale) |
			       (1 << StaticColor) |
			       (1 << PseudoColor) |
			       (1 << TrueColor) |
			       (1 << DirectColor));
	}
	screen->fb[0].blueMask  = 0x00;
	screen->fb[0].greenMask = 0x00;
	screen->fb[0].redMask   = 0x00;
	break;
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
	if (gray)
	{
	    screen->fb[0].visuals = (1 << StaticGray);
	}
	else
	{
	    screen->fb[0].visuals = (1 << StaticColor);
	}
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
#ifdef FBDEV_KLUDGE_FORMAT
	/*
	 * This is a kludge so that Render will work -- fill in the gaps
	 * in the pixel
	 */
	screen->fb[0].redMask = fbdevMakeContig (screen->fb[0].redMask,
						 screen->fb[0].greenMask|
						 screen->fb[0].blueMask);

	screen->fb[0].greenMask = fbdevMakeContig (screen->fb[0].greenMask,
						   screen->fb[0].redMask|
						   screen->fb[0].blueMask);

	screen->fb[0].blueMask = fbdevMakeContig (screen->fb[0].blueMask,
						  screen->fb[0].redMask|
						  screen->fb[0].greenMask);

#endif
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
    scrpriv->rotation = screen->rotation;
    
#ifdef FAKE24_ON_16
    if (screen->fb[0].depth == 24 && screen->fb[0].bitsPerPixel == 24 &&
	priv->var.bits_per_pixel == 16)
    {
	fake24 = TRUE;
	scrpriv->shadow = TRUE;
	scrpriv->rotation = 0;
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
        screen->width = priv->var.xres;
        screen->height = priv->var.yres;
        screen->fb[0].byteStride = priv->fix.line_length;
        screen->fb[0].pixelStride = (priv->fix.line_length * 8 / 
    				 priv->var.bits_per_pixel);
        screen->fb[0].frameBuffer = (CARD8 *) (priv->fb);
    }
    return TRUE;
}

Bool
fbdevScreenInit (KdScreenInfo *screen)
{
    FbdevScrPriv *scrpriv;

    scrpriv = xalloc (sizeof (FbdevScrPriv));
    if (!scrpriv)
	return FALSE;
    memset (scrpriv, '\0', sizeof (FbdevScrPriv));
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
		   CARD32	*size,
		   void		*closure)
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

LayerPtr
fbdevLayerCreate (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    FbdevPriv		*priv = pScreenPriv->card->driver;
    FbdevScrPriv	*scrpriv = screen->driver;
    LayerPtr		pLayer;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;
    PixmapPtr		pPixmap;
    int			kind;
    KdMouseMatrix	m;

    switch (scrpriv->rotation) {
    case 0:
	pScreen->width = screen->width;
	pScreen->height = screen->height;
	pScreen->mmWidth = screen->width_mm;
	pScreen->mmHeight = screen->height_mm;
	scrpriv->shadow = FALSE;
	m.matrix[0][0] = 1; m.matrix[0][1] = 0; m.matrix[0][2] = 0;
	m.matrix[1][0] = 0; m.matrix[1][1] = 1; m.matrix[1][2] = 0;
	break;
    case 90:
	pScreen->width = screen->height;
	pScreen->height = screen->width;
	pScreen->mmWidth = screen->height_mm;
	pScreen->mmHeight = screen->width_mm;
	scrpriv->shadow = TRUE;
	m.matrix[0][0] = 0; m.matrix[0][1] = -1; m.matrix[0][2] = screen->height - 1;
	m.matrix[1][0] = 1; m.matrix[1][1] = 0; m.matrix[1][2] = 0;
	break;
    case 180:
	pScreen->width = screen->width;
	pScreen->height = screen->height;
	pScreen->mmWidth = screen->width_mm;
	pScreen->mmHeight = screen->height_mm;
	scrpriv->shadow = TRUE;
	m.matrix[0][0] = -1; m.matrix[0][1] = 0; m.matrix[0][2] = screen->width - 1;
	m.matrix[1][0] = 0; m.matrix[1][1] = -1; m.matrix[1][2] = screen->height - 1;
	break;
    case 270:
	pScreen->width = screen->height;
	pScreen->height = screen->width;
	pScreen->mmWidth = screen->height_mm;
	pScreen->mmHeight = screen->width_mm;
	scrpriv->shadow = TRUE;
	m.matrix[0][0] = 0; m.matrix[0][1] = 1; m.matrix[0][2] = 0;
	m.matrix[1][0] = -1; m.matrix[1][1] = 0; m.matrix[1][2] = screen->width - 1;
	break;
    }
    KdSetMouseMatrix (&m);
    
    if (scrpriv->shadow)
    {
        window = fbdevWindowLinear;
	update = 0;
#ifdef FAKE24_ON_16
	if (pScreenPriv->screen->fb[0].bitsPerPixel == 24 && priv->var.bits_per_pixel == 16)
	{
	    update = fbdevUpdateFake24;
	}
	else
#endif /* FAKE24_ON_16 */
	{
	    switch (scrpriv->rotation) {
	    case 0:
		update = shadowUpdatePacked;
		break;
	    case 90:
		switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
		case 8:
		    update = shadowUpdateRotate8_90; break;
		case 16:
		    update = shadowUpdateRotate16_90; break;
		case 32:
		    update = shadowUpdateRotate32_90; break;
		}
		break;
	    case 180:
		switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
		case 8:
		    update = shadowUpdateRotate8_180; break;
		case 16:
		    update = shadowUpdateRotate16_180; break;
		case 32:
		    update = shadowUpdateRotate32_180; break;
		}
		break;
	    case 270:
		switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
		case 8:
		    update = shadowUpdateRotate8_270; break;
		case 16:
		    update = shadowUpdateRotate16_270; break;
		case 32:
		    update = shadowUpdateRotate32_270; break;
		}
		break;
	    }
	}
	if (!update)
	    abort ();
	kind = LAYER_SHADOW;
	pPixmap = 0;
    }
    else
    {
	kind = LAYER_FB;
	pPixmap = LAYER_SCREEN_PIXMAP;
	update = 0;
	window = 0;
    }
    return LayerCreate (pScreen, kind, screen->fb[0].depth, 
			pPixmap, update, window, 0);
}


#ifdef RANDR
Bool
fbdevRandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
    KdScreenPriv(pScreen);
    FbdevPriv		    *priv = pScreenPriv->card->driver;
    KdScreenInfo	    *screen = pScreenPriv->screen;
    FbdevScrPriv	    *scrpriv = screen->driver;
    RRVisualGroupPtr	    pVisualGroup;
    RRGroupOfVisualGroupPtr pGroupOfVisualGroup;
    RRScreenSizePtr	    pSize;
    Rotation		    rotateKind;
    int			    rotation;
    int			    n;
    
    *rotations = RR_Rotate_0|RR_Rotate_90|RR_Rotate_180|RR_Rotate_270;
    
    for (n = 0; n < pScreen->numDepths; n++)
	if (pScreen->allowedDepths[n].numVids)
	    break;
    if (n == pScreen->numDepths)
	return FALSE;
    
    pVisualGroup = RRCreateVisualGroup (pScreen);
    if (!pVisualGroup)
	return FALSE;
    if (!RRAddDepthToVisualGroup (pScreen,
				pVisualGroup,
				&pScreen->allowedDepths[n]))
    {
	RRDestroyVisualGroup (pScreen, pVisualGroup);
	return FALSE;
    }

    pVisualGroup = RRRegisterVisualGroup (pScreen, pVisualGroup);
    if (!pVisualGroup)
	return FALSE;
    
    pGroupOfVisualGroup = RRCreateGroupOfVisualGroup (pScreen);

    if (!RRAddVisualGroupToGroupOfVisualGroup (pScreen,
					 pGroupOfVisualGroup,
					 pVisualGroup))
    {
	RRDestroyGroupOfVisualGroup (pScreen, pGroupOfVisualGroup);
	/* pVisualGroup left until screen closed */
	return FALSE;
    }

    pGroupOfVisualGroup = RRRegisterGroupOfVisualGroup (pScreen, pGroupOfVisualGroup);
    if (!pGroupOfVisualGroup)
	return FALSE;
    
    pSize = RRRegisterSize (pScreen,
			    screen->width,
			    screen->height,
			    screen->width_mm,
			    screen->height_mm,
			    pGroupOfVisualGroup);
    
    rotation = scrpriv->rotation - screen->rotation;
    if (rotation < 0)
	rotation += 360;

    switch (rotation)
    {
    case 0:
	rotateKind = RR_Rotate_0;
	break;
    case 90:
	rotateKind = RR_Rotate_90;
	break;
    case 180:
	rotateKind = RR_Rotate_180;
	break;
    case 270:
	rotateKind = RR_Rotate_270;
	break;
    }

    RRSetCurrentConfig (pScreen, rotateKind, pSize, pVisualGroup);
    
    return TRUE;
}

int
fbdevLayerAdd (WindowPtr pWin, pointer value)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLayer = (LayerPtr) value;

    if (!LayerWindowAdd (pScreen, pLayer, pWin))
	return WT_STOPWALKING;

    return WT_WALKCHILDREN;
}

int
fbdevLayerRemove (WindowPtr pWin, pointer value)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLayer = (LayerPtr) value;

    LayerWindowRemove (pScreen, pLayer, pWin);

    return WT_WALKCHILDREN;
}

fbdevRandRSetConfig (ScreenPtr		pScreen,
		     Rotation		rotateKind,
		     RRScreenSizePtr	pSize,
		     RRVisualGroupPtr	pVisualGroup)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    FbdevPriv		*priv = pScreenPriv->card->driver;
    FbdevScrPriv	*scrpriv = screen->driver;
    int			rotation;
    Bool		wasEnabled = pScreenPriv->enabled;

    /*
     * The only thing that can change is rotation
     */
    switch (rotateKind)
    {
    case RR_Rotate_0:
	rotation = screen->rotation;
	break;
    case RR_Rotate_90:
	rotation = screen->rotation + 90;
	break;
    case RR_Rotate_180:
	rotation = screen->rotation + 180;
	break;
    case RR_Rotate_270:
	rotation = screen->rotation + 270;
	break;
    }
    if (rotation >= 360)
	rotation -= 360;

    if (scrpriv->rotation != rotation)
    {
	LayerPtr	pNewLayer;
	int		kind;
	int		oldrotation = scrpriv->rotation;
	int		oldshadow = scrpriv->shadow;
	int		oldwidth = pScreen->width;
	int		oldheight = pScreen->height;
	PixmapPtr	pPixmap;

	if (wasEnabled)
	    KdDisableScreen (pScreen);
	
	scrpriv->rotation = rotation;
	pNewLayer = fbdevLayerCreate (pScreen);
	if (!pNewLayer)
	{
	    scrpriv->shadow = oldshadow;
	    scrpriv->rotation = oldrotation;
	}
	if (WalkTree (pScreen, fbdevLayerAdd, (pointer) pNewLayer) == WT_STOPWALKING)
	{
	    WalkTree (pScreen, fbdevLayerRemove, (pointer) pNewLayer);
	    LayerDestroy (pScreen, pNewLayer);
	    scrpriv->rotation = oldrotation;
	    scrpriv->shadow = oldshadow;
	    pScreen->width = oldwidth;
	    pScreen->height = oldheight;
	    if (wasEnabled)
		KdEnableScreen (pScreen);
	    return FALSE;
	}
        WalkTree (pScreen, fbdevLayerRemove, (pointer) scrpriv->pLayer);
	LayerDestroy (pScreen, scrpriv->pLayer);
	scrpriv->pLayer = pNewLayer;
	if (wasEnabled)
	    KdEnableScreen (pScreen);
    }
    return TRUE;
}

Bool
fbdevRandRInit (ScreenPtr pScreen)
{
    rrScrPrivPtr    pScrPriv;
    
    if (!RRScreenInit (pScreen))
	return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = fbdevRandRGetInfo;
    pScrPriv->rrSetConfig = fbdevRandRSetConfig;
    return TRUE;
}
#endif

Bool
fbdevCreateColormap (ColormapPtr pmap)
{
    ScreenPtr		pScreen = pmap->pScreen;
    KdScreenPriv(pScreen);
    FbdevPriv		*priv = pScreenPriv->card->driver;
    VisualPtr		pVisual;
    int			i;
    int			nent;
    xColorItem		*pdefs;
    
    switch (priv->fix.visual) {
    case FB_VISUAL_STATIC_PSEUDOCOLOR:
	pVisual = pmap->pVisual;
	nent = pVisual->ColormapEntries;
	pdefs = ALLOCATE_LOCAL (nent * sizeof (xColorItem));
	if (!pdefs)
	    return FALSE;
	for (i = 0; i < nent; i++)
	    pdefs[i].pixel = i;
	fbdevGetColors (pScreen, 0, nent, pdefs);
	for (i = 0; i < nent; i++)
	{
	    pmap->red[i].co.local.red = pdefs[i].red;
	    pmap->red[i].co.local.green = pdefs[i].green;
	    pmap->red[i].co.local.blue = pdefs[i].blue;
	}
	DEALLOCATE_LOCAL (pdefs);
	return TRUE;
    default:
	return fbInitializeColormap (pmap);
    }
}

#ifdef TOUCHSCREEN
int TsFbdev = -1;
#endif

Bool
fbdevInitScreen (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    FbdevPriv		*priv = pScreenPriv->card->driver;
    FbdevScrPriv	*scrpriv = pScreenPriv->screen->driver;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;

#ifdef TOUCHSCREEN
    TsFbdev = pScreen->myNum;
#endif

    pScreen->CreateColormap = fbdevCreateColormap;

    if (!LayerStartInit (pScreen))
	return FALSE;
    if (!LayerFinishInit (pScreen))
	return FALSE;
    scrpriv->pLayer = fbdevLayerCreate (pScreen);
    if (!scrpriv->pLayer)
	return FALSE;
#ifdef RANDR
    if (!fbdevRandRInit (pScreen))
	return FALSE;
#endif
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
    KdScreenInfo	*screen = pScreenPriv->screen;
    FbdevPriv		*priv = pScreenPriv->card->driver;
    FbdevScrPriv	*scrpriv = pScreenPriv->screen->driver;
    int			k;

    priv->var.activate = FB_ACTIVATE_NOW|FB_CHANGE_CMAP_VBL;
    
    /* display it on the LCD */
    k = ioctl (priv->fd, FBIOPUT_VSCREENINFO, &priv->var);
    if (k < 0)
    {
	perror ("FBIOPUT_VSCREENINFO");
	return FALSE;
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
    return TRUE;
}

Bool
fbdevDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    FbdevPriv	*priv = pScreenPriv->card->driver;
    static int oldmode = -1;

    if (mode == oldmode)
	return TRUE;
#ifdef FBIOPUT_POWERMODE
    if (ioctl (priv->fd, FBIOPUT_POWERMODE, &mode) >= 0)
    {
	oldmode = mode;
	return TRUE;
    }
#endif
#ifdef FBIOBLANK
    if (ioctl (priv->fd, FBIOBLANK, mode ? mode + 1 : 0) >= 0)
    {
	oldmode = mode;
	return TRUE;
    }
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


KdCardFuncs	fbdevFuncs = {
    fbdevCardInit,	    /* cardinit */
    fbdevScreenInit,	    /* scrinit */
    fbdevInitScreen,	    /* initScreen */
    fbdevPreserve,	    /* preserve */
    fbdevEnable,	    /* enable */
    fbdevDPMS,		    /* dpms */
    fbdevDisable,	    /* disable */
    fbdevRestore,	    /* restore */
    fbdevScreenFini,	    /* scrfini */
    fbdevCardFini,	    /* cardfini */
    
    0,			    /* initCursor */
    0,			    /* enableCursor */
    0,			    /* disableCursor */
    0,			    /* finiCursor */
    0,			    /* recolorCursor */
    
    0,			    /* initAccel */
    0,			    /* enableAccel */
    0,			    /* syncAccel */
    0,			    /* disableAccel */
    0,			    /* finiAccel */
    
    fbdevGetColors,    	    /* getColors */
    fbdevPutColors,	    /* putColors */
};
