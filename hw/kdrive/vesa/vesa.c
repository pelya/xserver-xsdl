/* 
Copyright (c) 2000 by Juliusz Chroboczek
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions: 
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software. 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
/* $XFree86: xc/programs/Xserver/hw/kdrive/vesa/vesa.c,v 1.19 2002/09/29 23:39:47 keithp Exp $ */

#include "vesa.h"
#ifdef RANDR
#include <randrstr.h>
#endif

int vesa_video_mode = 0;
Bool vesa_force_mode = FALSE;
Bool vesa_swap_rgb = FALSE;
Bool vesa_shadow = FALSE;
Bool vesa_linear_fb = TRUE;
Bool vesa_restore = FALSE;
Bool vesa_verbose = FALSE;
Bool vesa_force_text = FALSE;

#define VesaPriv(scr)	((VesaScreenPrivPtr) (scr)->driver)

#define vesaWidth(scr,vmib) ((vmib)->XResolution)
#define vesaHeight(scr,vmib) ((vmib)->YResolution)

static Bool
vesaModeSupportable (VesaModePtr mode, Bool complain)
{
    if((mode->ModeAttributes & 0x10) == 0) {
	if(complain)
	    ErrorF("Text mode specified.\n");
	return FALSE;
    }
    if(mode->MemoryModel != 0x06 && mode->MemoryModel != 0x04 && mode->MemoryModel != 0x03) {
	if(complain)
	    ErrorF("Unsupported memory model 0x%X\n", mode->MemoryModel);
	return FALSE;
    }
    if((mode->ModeAttributes & 0x80) == 0) {
	if ((mode->ModeAttributes & 0x40) != 0) {
	    if(complain)
		ErrorF("Neither linear nor windowed framebuffer available in this mode\n");
	    return FALSE;
	}
    }
    if(!(mode->ModeAttributes & 1)) {
	if(complain)
	    ErrorF("Mode not supported on this hardware\n");
	return FALSE;
    }
    return TRUE;
}

static Bool
vesaModeSupported (VesaCardPrivPtr priv, VesaModePtr mode, Bool complain)
{
    if (!priv->vbeInfo && mode->vbe) {
	if (complain)
	    ErrorF("VBE bios mode not usable.\n");
	return FALSE;
    }
    return vesaModeSupportable (mode, complain);
}

void
vesaReportMode (VesaModePtr mode)
{
    int supported = (mode->ModeAttributes&MODE_SUPPORTED)?1:0;
    int colour = (mode->ModeAttributes&MODE_COLOUR)?1:0;
    int graphics = (mode->ModeAttributes&MODE_GRAPHICS)?1:0;
    int vga_compatible = !((mode->ModeAttributes&MODE_VGA)?1:0);
    int linear_fb = (mode->ModeAttributes&MODE_LINEAR)?1:0;

    ErrorF("0x%04X: %dx%dx%d%s",
           (unsigned)mode->mode, 
           (int)mode->XResolution, (int)mode->YResolution,
	   vesaDepth (mode),
           colour?"":" (monochrome)");
    switch(mode->MemoryModel) {
    case MEMORY_TEXT:
        ErrorF(" text mode");
        break;
    case MEMORY_CGA:
        ErrorF(" CGA graphics");
        break;
    case MEMORY_HERCULES:
        ErrorF(" Hercules graphics");
        break;
    case MEMORY_PLANAR:
        ErrorF(" Planar (%d planes)", mode->NumberOfPlanes);
        break;
    case MEMORY_PSEUDO:
        ErrorF(" PseudoColor");
        break;
    case MEMORY_NONCHAIN:
        ErrorF(" Non-chain 4, 256 colour");
        break;
    case MEMORY_DIRECT:
        if(mode->DirectColorModeInfo & MODE_DIRECT)
            ErrorF(" DirectColor");
        else
            ErrorF(" TrueColor");
        ErrorF(" [%d:%d:%d:%d]",
               mode->RedMaskSize, mode->GreenMaskSize, mode->BlueMaskSize,
               mode->RsvdMaskSize);
        if(mode->DirectColorModeInfo & 2)
            ErrorF(" (reserved bits are reserved)");
        break;
    case MEMORY_YUV:
	ErrorF("YUV");
        break;
    default:
        ErrorF("unknown MemoryModel 0x%X ", mode->MemoryModel);
    }
    if(!supported)
        ErrorF(" (unsupported)");
    else if(!linear_fb)
        ErrorF(" (no linear framebuffer)");
    ErrorF("\n");
}

VesaModePtr
vesaGetModes (Vm86InfoPtr vi, int *ret_nmode)
{
    VesaModePtr		modes;
    int			nmode, nmodeVbe, nmodeVga;
    int			code;
    
    code = VgaGetNmode (vi);
    if (code <= 0)
	nmodeVga = 0;
    else
	nmodeVga = code;
    
    code = VbeGetNmode (vi);
    if (code <= 0)
	nmodeVbe = 0;
    else
	nmodeVbe = code;

    nmode = nmodeVga + nmodeVbe;
    if (nmode <= 0)
	return 0;
    
    modes = xalloc (nmode * sizeof (VesaModeRec));
    
    memset (modes, '\0', nmode * sizeof (VesaModeRec));
    
    if (nmodeVga)
    {
	code = VgaGetModes (vi, modes, nmodeVga);
	if (code <= 0)
	    nmodeVga = 0;
	else
	    nmodeVga = code;
    }

    if (nmodeVbe)
    {
	code = VbeGetModes (vi, modes + nmodeVga, nmodeVbe);
	if (code <= 0)
	    nmodeVbe = 0;
	else
	    nmodeVbe = code;
    }
    
    nmode = nmodeVga + nmodeVbe;

    if (nmode == 0)
    {
	xfree (modes);
	modes = 0;
	return 0;
    }
    *ret_nmode = nmode;
    return modes;
}

Bool
vesaInitialize (KdCardInfo *card, VesaCardPrivPtr priv)
{
    int code;

    priv->vi = Vm86Setup();
    if(!priv->vi)
	goto fail;

    priv->modes = vesaGetModes (priv->vi, &priv->nmode);
	
    if (!priv->modes)
	goto fail;

    priv->vbeInfo = VbeInit (priv->vi);
    
    card->driver = priv;

    return TRUE;

fail:
    if(priv->vi)
	Vm86Cleanup(priv->vi);
    return FALSE;
}

void
vesaListModes (void)
{
    Vm86InfoPtr	vi;
    VesaModePtr	modes;
    int		nmode;
    int		n;

    vi = Vm86Setup ();
    if (!vi)
    {
	ErrorF ("Can't setup vm86\n");
    }
    else
    {
	modes = vesaGetModes (vi, &nmode);
	if (!modes)
	{
	    ErrorF ("No modes available\n");
	}
	else
	{
	    VbeReportInfo (vi);
	    for (n = 0; n < nmode; n++)
	    {
		if (vesa_force_mode || vesaModeSupportable (modes+n, 0))
		    vesaReportMode (modes+n);
	    }
	    xfree (modes);
	}
	Vm86Cleanup (vi);
    }
}

Bool
vesaCardInit(KdCardInfo *card)
{
    VesaCardPrivPtr priv;

    priv = xalloc(sizeof(VesaCardPrivRec));
    if(!priv)
        return FALSE;

    if (!vesaInitialize (card, priv))
    {
        xfree(priv);
	return FALSE;
    }
    
    return TRUE;
}

int
vesaDepth (VesaModePtr mode)
{
    if (mode->MemoryModel == MEMORY_DIRECT)
	return (mode->RedMaskSize +
		mode->GreenMaskSize +
		mode->BlueMaskSize);
    else
	return mode->BitsPerPixel;
}

Bool
vesaModeGood (KdScreenInfo  *screen,
	      VesaModePtr   a)
{
    if (vesaWidth(screen,a) <= screen->width &&
	vesaHeight(screen,a) <= screen->height &&
	vesaDepth (a) >= screen->fb[0].depth)
    {
	return TRUE;
    }
}

#define vabs(a)	((a) >= 0 ? (a) : -(a))

int
vesaSizeError (KdScreenInfo *screen,
	       VesaModePtr  a)
{
    int     xdist, ydist;
    xdist = vabs (screen->width - vesaWidth(screen,a));
    ydist = vabs (screen->height - vesaHeight(screen,a));
    return xdist * xdist + ydist * ydist;
}

Bool
vesaModeBetter (KdScreenInfo	*screen,
		VesaModePtr	a,
		VesaModePtr	b)
{
    int	    aerr, berr;

    if (vesaModeGood (screen, a))
    {
	if (!vesaModeGood (screen, b))
	    return TRUE;
    }
    else
    {
	if (vesaModeGood (screen, b))
	    return FALSE;
    }
    aerr = vesaSizeError (screen, a);
    berr = vesaSizeError (screen, b);
    if (aerr < berr)
	return TRUE;
    if (berr < aerr)
	return FALSE;
    if (vabs (screen->fb[0].depth - vesaDepth (a)) < 
	vabs (screen->fb[0].depth - vesaDepth (b)))
	return TRUE;
    return FALSE;
}

VesaModePtr
vesaSelectMode (KdScreenInfo *screen)
{
    VesaCardPrivPtr priv = screen->card->driver;
    int		    i, best;
    
    if (vesa_video_mode)
    {
	for (best = 0; best < priv->nmode; best++)
	    if (priv->modes[best].mode == vesa_video_mode &&
		(vesaModeSupported (priv, &priv->modes[best], FALSE) ||
		 vesa_force_mode))
		return &priv->modes[best];
    }
    for (best = 0; best < priv->nmode; best++)
    {
	if (vesaModeSupported (priv, &priv->modes[best], FALSE))
	    break;
    }
    if (best == priv->nmode)
	return 0;
    for (i = best + 1; i < priv->nmode; i++)
	if (vesaModeSupported (priv, &priv->modes[i], FALSE) &&
	    vesaModeBetter (screen, &priv->modes[i], 
			    &priv->modes[best]))
	    best = i;
    return &priv->modes[best];
}   
    
Bool
vesaScreenInitialize (KdScreenInfo *screen, VesaScreenPrivPtr pscr)
{
    VesaCardPrivPtr	priv = screen->card->driver;
    VesaModePtr		mode;
    Pixel		allbits;

    screen->driver = pscr;
    
    if (!screen->width || !screen->height)
    {
	screen->width = 640;
	screen->height = 480;
    }
    if (!screen->fb[0].depth)
	screen->fb[0].depth = 4;
    
    if (vesa_verbose)
	ErrorF ("Mode requested %dx%dx%d\n",
		screen->width, screen->height, screen->fb[0].depth);
    
    mode = vesaSelectMode (screen);
    
    if (!mode)
    {
	if (vesa_verbose)
	    ErrorF ("No selectable mode\n");
	return FALSE;
    }
    pscr->mode = *mode;

    if (vesa_verbose)
    {
	ErrorF ("\t");
	vesaReportMode (&pscr->mode);
    }
    
    pscr->randr = screen->randr;
    pscr->shadow = vesa_shadow;
    pscr->origDepth = screen->fb[0].depth;
    pscr->layerKind = LAYER_FB;

    /*
     * Compute visual support for the selected depth
     */
    
    switch (pscr->mode.MemoryModel) {
    case MEMORY_DIRECT:
        /* TrueColor or DirectColor */
        screen->fb[0].visuals = (1 << TrueColor);
        screen->fb[0].redMask = 
            FbStipMask(pscr->mode.RedFieldPosition, pscr->mode.RedMaskSize);
        screen->fb[0].greenMask = 
            FbStipMask(pscr->mode.GreenFieldPosition, pscr->mode.GreenMaskSize);
        screen->fb[0].blueMask = 
            FbStipMask(pscr->mode.BlueFieldPosition, pscr->mode.BlueMaskSize);
	break;
    case MEMORY_PSEUDO:
        /* PseudoColor */
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
    case MEMORY_PLANAR:
	/* 4 plane planar */
	if (pscr->mode.ModeAttributes & MODE_COLOUR)
	    screen->fb[0].visuals = (1 << StaticColor);
	else
	    screen->fb[0].visuals = (1 << StaticGray);
	screen->fb[0].blueMask  = 0x00;
	screen->fb[0].greenMask = 0x00;
	screen->fb[0].redMask   = 0x00;
	break;
    default:
        ErrorF("Unsupported VESA MemoryModel 0x%02X\n",
               pscr->mode.MemoryModel);
        return FALSE;
    }
    screen->rate = 72;

    return vesaMapFramebuffer (screen);
}

Bool
vesaScreenInit(KdScreenInfo *screen)
{
    VesaScreenPrivPtr	pscr;
    
    pscr = xcalloc (1, sizeof (VesaScreenPrivRec));
    if (!pscr)
	return FALSE;
    if (!vesaScreenInitialize (screen, pscr))
	return FALSE;
    return TRUE;
}

void *
vesaSetWindowPlanar(ScreenPtr pScreen,
		  CARD32    row,
		  CARD32    offset,
		  int	    mode,
		  CARD32    *size)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    static int		plane;
    int			winSize;
    void		*base;

    plane = offset & 3;
    VgaSetWritePlaneMask (priv->vi, (1 << plane));
    offset = offset >> 2;
    if (pscr->mode.vbe)
    {
	base = VbeSetWindow (priv->vi,
			     priv->vbeInfo,
			     pscr->mode.BytesPerScanLine * row + offset,
			     mode,
			     &winSize);
    }
    else
    {
	base = VgaSetWindow (priv->vi,
			     pscr->mode.mode,
			     pscr->mode.BytesPerScanLine * row + offset,
			     mode,
			     &winSize);
    }
    *size = (CARD32) winSize;
    return base;
}

void *
vesaSetWindowLinear (ScreenPtr	pScreen,
		     CARD32	row,
		     CARD32	offset,
		     int	mode,
		     CARD32	*size)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;

    *size = pscr->mode.BytesPerScanLine;
    return (CARD8 *) pscr->fb + row * pscr->mode.BytesPerScanLine + offset;
}

void *
vesaSetWindowWindowed (ScreenPtr    pScreen,
		       CARD32	    row,
		       CARD32	    offset,
		       int	    mode,
		       CARD32	    *size)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int			winSize;
    void		*base;

    if (pscr->mode.vbe)
    {
	base = VbeSetWindow (priv->vi,
			     priv->vbeInfo,
			     pscr->mode.BytesPerScanLine * row + offset,
			     mode,
			     &winSize);
    }
    else
    {
	base = VgaSetWindow (priv->vi,
			     pscr->mode.mode,
			     pscr->mode.BytesPerScanLine * row + offset,
			     mode,
			     &winSize);
    }
    *size = (CARD32) winSize;
    return base;
}

void *
vesaWindowPlanar (ScreenPtr pScreen,
		  CARD32    row,
		  CARD32    offset,
		  int	    mode,
		  CARD32    *size,
		  void	    *closure)
{
    KdScreenPriv(pScreen);

    if (!pScreenPriv->enabled)
	return 0;
    return vesaSetWindowPlanar (pScreen, row, offset, mode, size);
}

void *
vesaWindowLinear (ScreenPtr pScreen,
		  CARD32    row,
		  CARD32    offset,
		  int	    mode,
		  CARD32    *size,
		  void	    *closure)
{
    KdScreenPriv(pScreen);

    if (!pScreenPriv->enabled)
	return 0;
    return vesaSetWindowLinear (pScreen, row, offset, mode, size);
}

void *
vesaWindowWindowed (ScreenPtr	pScreen,
		    CARD32	row,
		    CARD32	offset,
		    int		mode,
		    CARD32	*size,
		    void	*closure)
{
    KdScreenPriv(pScreen);

    if (!pScreenPriv->enabled)
	return 0;
    return vesaSetWindowWindowed (pScreen, row, offset, mode, size);
}

#define vesaInvertBits32(v) { \
    v = ((v & 0x55555555) << 1) | ((v >> 1) & 0x55555555); \
    v = ((v & 0x33333333) << 2) | ((v >> 2) & 0x33333333); \
    v = ((v & 0x0f0f0f0f) << 4) | ((v >> 4) & 0x0f0f0f0f); \
}

void *
vesaWindowCga (ScreenPtr    pScreen,
	       CARD32	    row,
	       CARD32	    offset,
	       int	    mode,
	       CARD32	    *size,
	       void	    *closure)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int			line;
    
    if (!pScreenPriv->enabled)
	return 0;
    *size = pscr->mode.BytesPerScanLine;
    line = ((row & 1) << 13) + (row >> 1) * pscr->mode.BytesPerScanLine;
    return (CARD8 *) pscr->fb + line + offset;
}

void
vesaUpdateMono (ScreenPtr	pScreen,
		shadowBufPtr	pBuf)
{
    RegionPtr	damage = &pBuf->damage;
    PixmapPtr	pShadow = pBuf->pPixmap;
    shadowScrPriv(pScreen);
    int		nbox = REGION_NUM_RECTS (damage);
    BoxPtr	pbox = REGION_RECTS (damage);
    FbBits	*shaBase, *shaLine, *sha;
    FbBits	s;
    FbStride	shaStride;
    int		scrBase, scrLine, scr;
    int		shaBpp;
    int		shaXoff, shaYoff;   /* XXX assumed to be zero */
    int		x, y, w, h, width;
    int         i;
    FbBits	*winBase, *winLine, *win;
    CARD32      winSize;
    FbBits	bits;
    int		plane;

    fbGetDrawable (&pShadow->drawable, shaBase, shaStride, shaBpp, shaXoff, shaYoff);
    while (nbox--)
    {
	x = pbox->x1 * shaBpp;
	y = pbox->y1;
	w = (pbox->x2 - pbox->x1) * shaBpp;
	h = pbox->y2 - pbox->y1;

	scrLine = (x >> FB_SHIFT);
	shaLine = shaBase + y * shaStride + (x >> FB_SHIFT);
				   
	x &= FB_MASK;
	w = (w + x + FB_MASK) >> FB_SHIFT;
	
	while (h--)
	{
	    winSize = 0;
	    scrBase = 0;
	    width = w;
	    scr = scrLine;
	    sha = shaLine;
	    while (width) {
		/* how much remains in this window */
		i = scrBase + winSize - scr;
		if (i <= 0 || scr < scrBase)
		{
		    winBase = (FbBits *) (*pBuf->window) (pScreen,
							  y,
							  scr * sizeof (FbBits),
							  SHADOW_WINDOW_WRITE,
							  &winSize,
							  pBuf->closure);
		    if(!winBase)
			return;
		    scrBase = scr;
		    winSize /= sizeof (FbBits);
		    i = winSize;
		}
		win = winBase + (scr - scrBase);
		if (i > width)
		    i = width;
		width -= i;
		scr += i;
		while (i--)
		{
		    bits = *sha++;
		    vesaInvertBits32(bits);
		    *win++ = bits;
		}
	    }
	    shaLine += shaStride;
	    y++;
	}
	pbox++;
    }
}

static const CARD16	vga16Colors[16][3] = {
    { 0,   0,   0,   },       /*  0 */
    { 0,   0,   0xAA,},       /*  1 */
    { 0,   0xAA,0,   },       /*  2 */
    { 0,   0xAA,0xAA,},       /*  3 */
    { 0xAA,0,   0,   },       /*  4 */
    { 0xAA,0,   0xAA,},       /*  5 */
    { 0xAA,0x55,0,   },       /*  6 */
    { 0xAA,0xAA,0xAA,},       /*  7 */
    { 0x55,0x55,0x55,},       /*  8 */
    { 0x55,0x55,0xFF,},       /*  9 */
    { 0x55,0xFF,0x55,},       /* 10 */
    { 0x55,0xFF,0xFF,},       /* 11 */
    { 0xFF,0x55,0x55,},       /* 12 */
    { 0xFF,0x55,0xFF,},       /* 13 */
    { 0xFF,0xFF,0x55,},       /* 14 */
    { 0xFF,0xFF,0xFF,},       /* 15 */
};

Bool
vesaCreateColormap16 (ColormapPtr pmap)
{
    int	    i, j;

    if (pmap->pVisual->ColormapEntries == 16)
    for (i = 0; i < pmap->pVisual->ColormapEntries; i++)
    {
	j = i & 0xf;
	pmap->red[i].co.local.red = (vga16Colors[j][0]<<8)|vga16Colors[j][0];
	pmap->red[i].co.local.green = (vga16Colors[j][1]<<8)|vga16Colors[j][1];
	pmap->red[i].co.local.blue = (vga16Colors[j][2]<<8)|vga16Colors[j][2];
    }
    return TRUE;
}

void
vesaConfigureScreen (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;

    KdMouseMatrix	m;

    if (pscr->mapping == VESA_PLANAR || pscr->mapping == VESA_MONO)
    {
	pscr->shadow = TRUE;
	pscr->randr = RR_Rotate_0;
    } 
    else if (pscr->mapping == VESA_WINDOWED)
	pscr->shadow = TRUE;
    else if (pscr->randr != RR_Rotate_0)
	pscr->shadow = TRUE;
    else
	pscr->shadow = vesa_shadow;
    
    KdComputeMouseMatrix (&m, pscr->randr, 
			  pscr->mode.XResolution, pscr->mode.YResolution);
    
    if (m.matrix[0][0])
    {
	pScreen->width = pscr->mode.XResolution;
	pScreen->height = pscr->mode.YResolution;
	pScreen->mmWidth = screen->width_mm;
	pScreen->mmHeight = screen->height_mm;
    }
    else
    {
	pScreen->width = pscr->mode.YResolution;
	pScreen->height = pscr->mode.XResolution;
	pScreen->mmWidth = screen->height_mm;
	pScreen->mmHeight = screen->width_mm;
    }
    KdSetMouseMatrix (&m);
}

LayerPtr
vesaLayerCreate (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    LayerPtr		pLayer;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;
    PixmapPtr		pPixmap;
    int			kind;

    if (pscr->shadow)
    {
	if (pscr->randr != RR_Rotate_0)
	    update = shadowUpdateRotatePacked;
	else
	    update = shadowUpdatePacked;
	switch (pscr->mapping) {
	case VESA_LINEAR:
	    window = vesaWindowLinear;
	    break;
	case VESA_WINDOWED:
	    window = vesaWindowWindowed;
	    break;
	case VESA_PLANAR:
	    pScreen->CreateColormap = vesaCreateColormap16;
	    if (pScreenPriv->screen->fb[0].bitsPerPixel == 8)
		update = shadowUpdatePlanar4x8;
	    else
		update = shadowUpdatePlanar4;
	    window = vesaWindowPlanar;
	    break;
	case VESA_MONO:
	    update = vesaUpdateMono;
	    if (pscr->mode.mode < 8)
		window = vesaWindowCga;
	    else
		window = vesaWindowLinear;
	    break;
	}
	
	kind = LAYER_SHADOW;
	pPixmap = 0;
    }
    else
    {
	kind = pscr->layerKind;
	pPixmap = LAYER_SCREEN_PIXMAP;
	update = 0;
	window = 0;
    }

    if (vesa_verbose)
	ErrorF ("Mode selected %dx%dx%d\n",
		pScreen->width, pScreen->height, screen->fb[0].depth);

    return LayerCreate (pScreen, kind, screen->fb[0].depth, 
			pPixmap, update, window, pscr->randr, 0);
}

Bool
vesaMapFramebuffer (KdScreenInfo    *screen)
{
    VesaCardPrivPtr	priv = screen->card->driver;
    VesaScreenPrivPtr	pscr = screen->driver;
    int			depth, bpp, fbbpp;
    Pixel		allbits;

    if (vesa_linear_fb)
	pscr->mapping = VESA_LINEAR;
    else
	pscr->mapping = VESA_WINDOWED;
    
    depth = vesaDepth (&pscr->mode);
    bpp = pscr->mode.BitsPerPixel;
    
    if (bpp > 24)
	bpp = 32;
    else if (bpp > 16)
	bpp = 24;
    else if (bpp > 8)
	bpp = 16;
    else if (bpp > 4)
	bpp = 8;
    else if (bpp > 1)
	bpp = 4;
    else
	bpp = 1;
    fbbpp = bpp;
    
    switch (pscr->mode.MemoryModel) {
    case MEMORY_DIRECT:
        allbits = (screen->fb[0].redMask | 
		   screen->fb[0].greenMask | 
		   screen->fb[0].blueMask);
        depth = 32;
        while (depth && !(allbits & (1 << (depth - 1))))
            depth--;
	if (vesa_verbose)
	    ErrorF ("\tTrue Color red 0x%x green 0x%x blue 0x%x\n",
		    bpp, depth, 
		    screen->fb[0].redMask,
		    screen->fb[0].greenMask,
		    screen->fb[0].blueMask);
	break;
    case MEMORY_PSEUDO:
	if (vesa_verbose)
	    ErrorF ("\tPseudo Color bpp %d depth %d\n",
		    bpp, depth);
	break;
    case MEMORY_PLANAR:
	if (bpp == 4)
	{
	    bpp = screen->fb[0].bitsPerPixel;
	    if (bpp != 8)
		bpp = 4;
	    depth = bpp;
	}
	if (bpp == 1)
	{
	    pscr->mapping = VESA_MONO;
	    if (vesa_verbose)
		ErrorF ("\tMonochrome\n");
	}
	else
	{
	    pscr->mapping = VESA_PLANAR;
	    if (vesa_verbose)
		ErrorF ("\tStatic color bpp %d depth %d\n",
			bpp, depth);
	}
	pscr->randr = RR_Rotate_0;
	break;
    default:
	return 0;
    }

    switch (fbbpp) {
    case 8:
    case 16:
    case 32:
	break;
    default:
	pscr->randr = RR_Rotate_0;
	break;
    }
    
    screen->width = pscr->mode.XResolution;
    screen->height = pscr->mode.YResolution;
    screen->fb[0].depth = depth;
    screen->fb[0].bitsPerPixel = bpp;
    screen->fb[0].byteStride = pscr->mode.BytesPerScanLine;
    screen->fb[0].pixelStride = ((pscr->mode.BytesPerScanLine * 8) / fbbpp);

    if (pscr->mapping == VESA_LINEAR && !(pscr->mode.ModeAttributes & MODE_LINEAR))
	pscr->mapping = VESA_WINDOWED;
    
    screen->softCursor = TRUE;

    switch (pscr->mapping) {
    case VESA_MONO:
    case VESA_LINEAR:
	if (pscr->mode.vbe)
	    pscr->fb = VbeMapFramebuffer(priv->vi, priv->vbeInfo, 
					 pscr->mode.mode,
					 &pscr->fb_size,
					 &pscr->fb_phys);
	else
	    pscr->fb = VgaMapFramebuffer (priv->vi, 
					  pscr->mode.mode,
					  &pscr->fb_size,
					  &pscr->fb_phys);
	if (!pscr->fb)
	    return FALSE;
	break;
    case VESA_WINDOWED:
	pscr->fb = NULL;
	break;
    case VESA_PLANAR:
	pscr->fb = NULL;
	break;
    }
    screen->fb[0].frameBuffer = (CARD8 *)(pscr->fb);
    return TRUE;
}

void
vesaUnmapFramebuffer (KdScreenInfo  *screen)
{
    VesaCardPrivPtr	    priv = screen->card->driver;
    VesaScreenPrivPtr	    pscr = screen->driver;
    
    if (pscr->fb)
    {
	if (pscr->mode.vbe)
	    VbeUnmapFramebuffer(priv->vi, priv->vbeInfo, pscr->mode.mode, pscr->fb);
	else
	    VgaUnmapFramebuffer (priv->vi);
	pscr->fb = 0;
    }
}

#ifdef RANDR
Bool
vesaRandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
    KdScreenPriv(pScreen);
    VesaModePtr		    modes, mode;
    KdScreenInfo	    *screen = pScreenPriv->screen;
    VesaCardPrivPtr	    priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	    pscr = pScreenPriv->screen->driver;
    int			    nmode;
    int			    n;
    RRScreenSizePtr	    pSize;
    
    *rotations = (RR_Rotate_0|RR_Rotate_90|RR_Rotate_180|RR_Rotate_270|
		  RR_Reflect_X|RR_Reflect_Y);
    /*
     * Get mode information from BIOS -- every time in case
     * something changes, like an external monitor is plugged in
     */
    modes = vesaGetModes (priv->vi, &nmode);
    if (!modes)
	return FALSE;
    if (priv->modes)
	xfree (priv->modes);
    priv->modes = modes;
    priv->nmode = nmode;
    for (n = 0; n < nmode; n++)
    {
	mode = &priv->modes[n];
	if (vesaModeSupported (priv, mode, FALSE))
	{
	    /*
	     * XXX limit reported modes to those matching the current
	     * format
	     */
	    if (mode->NumberOfPlanes == pscr->mode.NumberOfPlanes &&
		mode->BitsPerPixel == pscr->mode.BitsPerPixel &&
		mode->MemoryModel == pscr->mode.MemoryModel &&
		mode->RedMaskSize == pscr->mode.RedMaskSize &&
		mode->RedFieldPosition == pscr->mode.RedFieldPosition &&
		mode->GreenMaskSize == pscr->mode.GreenMaskSize &&
		mode->GreenFieldPosition == pscr->mode.GreenFieldPosition &&
		mode->BlueMaskSize == pscr->mode.BlueMaskSize &&
		mode->BlueFieldPosition == pscr->mode.BlueFieldPosition)
	    {
		int width, height, width_mm, height_mm;
		if (screen->randr & (RR_Rotate_0|RR_Rotate_180))
		{
		    width = mode->XResolution;
		    height = mode->YResolution;
		    width_mm = screen->width_mm;
		    height_mm = screen->height_mm;
		}
		else
		{
		    width = mode->YResolution;
		    height = mode->XResolution;
		    width_mm = screen->height_mm;
		    height_mm = screen->width_mm;
		}
		pSize = RRRegisterSize (pScreen,
					width, height,
					width_mm, height_mm);
		if (mode->XResolution == screen->width &&
		    mode->YResolution == screen->height)
		{
		    int	randr = KdSubRotation (pscr->randr, screen->randr);
		    RRSetCurrentConfig (pScreen, randr, 0, pSize);
		}
	    }
	}
    }
    return TRUE;
}

int
vesaLayerAdd (WindowPtr pWin, pointer value)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLayer = (LayerPtr) value;

    if (!LayerWindowAdd (pScreen, pLayer, pWin))
	return WT_STOPWALKING;

    return WT_WALKCHILDREN;
}

int
vesaLayerRemove (WindowPtr pWin, pointer value)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    LayerPtr	    pLayer = (LayerPtr) value;

    LayerWindowRemove (pScreen, pLayer, pWin);

    return WT_WALKCHILDREN;
}

Bool
vesaRandRSetConfig (ScreenPtr		pScreen,
		    Rotation		randr,
		    int			rate,
		    RRScreenSizePtr	pSize)
{
    KdScreenPriv(pScreen);
    VesaModePtr		mode;
    KdScreenInfo	*screen = pScreenPriv->screen;
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int			n;
    Bool		wasEnabled = pScreenPriv->enabled;
    Bool		ret = FALSE;
    VesaScreenPrivRec	oldscr;
    int			oldwidth;
    int			oldheight;
    int			oldmmwidth;
    int			oldmmheight;
    LayerPtr		pNewLayer;
    int			newwidth, newheight;

    if (screen->randr & (RR_Rotate_0|RR_Rotate_180))
    {
	newwidth = pSize->width;
	newheight = pSize->height;
    }
    else
    {
	newwidth = pSize->height;
	newheight = pSize->width;
    }
    for (n = 0; n < priv->nmode; n++)
    {
	mode = &priv->modes[n];
	if (vesaModeSupported (priv, mode, FALSE))
	{
	    /* 
	     * XXX all we have to match is the size
	     */
	    if (mode->XResolution == newwidth &&
		mode->YResolution == newheight &&
		mode->NumberOfPlanes == pscr->mode.NumberOfPlanes &&
		mode->BitsPerPixel == pscr->mode.BitsPerPixel &&
		mode->RedMaskSize == pscr->mode.RedMaskSize &&
		mode->RedFieldPosition == pscr->mode.RedFieldPosition &&
		mode->GreenMaskSize == pscr->mode.GreenMaskSize &&
		mode->GreenFieldPosition == pscr->mode.GreenFieldPosition &&
		mode->BlueMaskSize == pscr->mode.BlueMaskSize &&
		mode->BlueFieldPosition == pscr->mode.BlueFieldPosition)
		break;
	}
    }
    if (n == priv->nmode)
	goto bail0;
    
    if (wasEnabled)
	KdDisableScreen (pScreen);

    if (mode->mode != pscr->mode.mode)
    {
	ret = vesaSetMode (pScreen, mode);
	if (!ret)
	    goto bail1;
    }

    oldscr = *pscr;
    
    oldwidth = screen->width;
    oldheight = screen->height;
    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;
    
    /*
     * Set new configuration
     */
    
    pscr->mode = *mode;
    pscr->randr = KdAddRotation (screen->randr, randr);
			       
    /*
     * Can't rotate some formats
     */
    switch (screen->fb[0].bitsPerPixel) {
    case 8:
    case 16:
    case 32:
	break;
    default:
	if (pscr->randr)
	    goto bail2;
	break;
    }

    vesaUnmapFramebuffer (screen);
    if (!vesaMapFramebuffer (screen))
	goto bail3;
    
#if 0
    /*
     * XXX can't switch depths yet
     */
    screen->fb[0].depth = depth;
    screen->fb[0].bitsPerPixel = bpp;
#endif
    screen->fb[0].byteStride = mode->BytesPerScanLine;
    screen->fb[0].pixelStride = ((mode->BytesPerScanLine * 8) / screen->fb[0].bitsPerPixel);
    
    /*
     * Compute screen geometry 
     */
    vesaConfigureScreen (pScreen);

    /*
     * Set frame buffer mapping
     */
    if (!pscr->shadow)
    {
	(*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap (pScreen),
					pScreen->width,
					pScreen->height,
					screen->fb[0].depth,
					screen->fb[0].bitsPerPixel,
					screen->fb[0].byteStride,
					screen->fb[0].frameBuffer);
    }
	
    /*
     * Create the layer
     */
    pNewLayer = vesaLayerCreate (pScreen);
    if (!pNewLayer)
	goto bail4;
    
    if (WalkTree (pScreen, vesaLayerAdd, (pointer) pNewLayer) == WT_STOPWALKING)
	goto bail5;
    
    WalkTree (pScreen, vesaLayerRemove, (pointer) pscr->pLayer);
    LayerDestroy (pScreen, pscr->pLayer);

    pscr->pLayer = pNewLayer;

    /* set the subpixel order */
    KdSetSubpixelOrder (pScreen, pscr->randr);

    if (wasEnabled)
	KdEnableScreen (pScreen);

    return TRUE;

bail5:
    WalkTree (pScreen, vesaLayerRemove, (pointer) pNewLayer);
    LayerDestroy (pScreen, pNewLayer);
bail4:
    vesaUnmapFramebuffer (screen);
    *pscr = oldscr;
    (void) vesaMapFramebuffer (screen);
    
bail3:
    pScreen->width = oldwidth;
    pScreen->height = oldheight;
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;
    
bail2:
    *pscr = oldscr;
    
    /*
     * Set frame buffer mapping
     */
    if (!pscr->shadow)
    {
	(*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap (pScreen),
					pScreen->width,
					pScreen->height,
					screen->fb[0].depth,
					screen->fb[0].bitsPerPixel,
					screen->fb[0].byteStride,
					screen->fb[0].frameBuffer);
    }

    (void) vesaSetMode (pScreen, &pscr->mode);

bail1:
    if (wasEnabled)
	KdEnableScreen (pScreen);
bail0:
    
    return FALSE;
}

Bool
vesaRandRInit (ScreenPtr pScreen)
{
    rrScrPrivPtr    pScrPriv;
    
    if (!RRScreenInit (pScreen))
	return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = vesaRandRGetInfo;
    pScrPriv->rrSetConfig = vesaRandRSetConfig;
    return TRUE;
}
#endif

Bool
vesaInitScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    
    if (!LayerStartInit (pScreen))
	return FALSE;
    
    return TRUE;
}

Bool
vesaFinishInitScreen (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    
    pscr->layerKind = LayerNewKind (pScreen);

    if (!LayerFinishInit (pScreen))
	return FALSE;

    vesaConfigureScreen (pScreen);

    pscr->pLayer = vesaLayerCreate (pScreen);
    if (!pscr->pLayer)
	return FALSE;
    
#ifdef RANDR
    if (!vesaRandRInit (pScreen))
	return FALSE;
#endif
    
    return TRUE;
}

Bool
vesaSetMode (ScreenPtr	    pScreen,
	     VesaModePtr    mode)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int			code;
    
    if (mode->vbe)
    {
	if (vesa_verbose)
	    ErrorF ("Enable VBE mode 0x%x\n", mode->mode);
	code = VbeSetMode(priv->vi, priv->vbeInfo, mode->mode,
			  pscr->mapping == VESA_LINEAR,
			  mode->MemoryModel == MEMORY_DIRECT);
    }
    else
    {
	if (vesa_verbose)
	    ErrorF ("Enable BIOS mode 0x%x\n", mode->mode);
	code = VgaSetMode (priv->vi, mode->mode);
    }
    
    if(code < 0)
	return FALSE;
    
    return TRUE;
}

Bool
vesaEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    KdScreenInfo	*screen = pScreenPriv->screen;
    int			code;
    int			i;
    CARD32		size;
    char		*p;

    if (!vesaSetMode (pScreen, &pscr->mode))
	return FALSE;
    
    switch (pscr->mapping) {
    case VESA_MONO:
	VgaSetWritePlaneMask (priv->vi, 0x1);
    case VESA_LINEAR:
	/*
	 * Remap the frame buffer if necessary
	 */
	if (!pscr->fb)
	{
	    if (pscr->mode.vbe)
		pscr->fb = VbeMapFramebuffer(priv->vi, priv->vbeInfo, 
					     pscr->mode.mode,
					     &pscr->fb_size,
					     &pscr->fb_phys);
	    else
		pscr->fb = VgaMapFramebuffer (priv->vi, 
					      pscr->mode.mode,
					      &pscr->fb_size,
					      &pscr->fb_phys);
	    if (!pscr->fb)
		return FALSE;
	    screen->fb[0].frameBuffer = (CARD8 *)(pscr->fb);
	    /*
	     * Set frame buffer mapping
	     */
	    if (!pscr->shadow)
	    {
		(*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap (pScreen),
						pScreen->width,
						pScreen->height,
						screen->fb[0].depth,
						screen->fb[0].bitsPerPixel,
						screen->fb[0].byteStride,
						screen->fb[0].frameBuffer);
	    }
	}
	memcpy (priv->text, pscr->fb, VESA_TEXT_SAVE);
	break;
    case VESA_WINDOWED:
	for (i = 0; i < VESA_TEXT_SAVE;) 
	{
	    p = vesaSetWindowWindowed (pScreen, 0, i, VBE_WINDOW_READ, &size);
            if(!p) {
                ErrorF("Couldn't set window for saving VGA font\n");
                break;
            }
            if(i + size > VESA_TEXT_SAVE)
                size = VESA_TEXT_SAVE - i;
            memcpy(((char*)priv->text) + i, p, size);
            i += size;
        }
	break;
    case VESA_PLANAR:
	for (i = 0; i < 4; i++)
	{
	    p = vesaSetWindowPlanar (pScreen, 0, i, VBE_WINDOW_READ, &size);
	    memcpy (((char *)priv->text) + i * (VESA_TEXT_SAVE/4), p,
		    (VESA_TEXT_SAVE/4));
	}
	break;
    }
    return TRUE;
}

#ifndef TOSHIBA_SMM

# ifdef linux
#  define TOSHIBA_SMM 1
# endif

# ifndef TOSHIBA_SMM
#  define TOSHIBA_SMM 0
# endif

#endif

#if TOSHIBA_SMM
/*
 * Toshiba laptops use a special interface to operate the backlight
 */
#include <sys/ioctl.h>
#define TOSH_PROC "/proc/toshiba"
#define TOSH_DEVICE "/dev/toshiba"
#define TOSH_SMM _IOWR('t', 0x90, 24)

typedef struct {
	unsigned int eax;
	unsigned int ebx __attribute__ ((packed));
	unsigned int ecx __attribute__ ((packed));
	unsigned int edx __attribute__ ((packed));
	unsigned int esi __attribute__ ((packed));
	unsigned int edi __attribute__ ((packed));
} SMMRegisters;

#define HCI_BACKLIGHT	0x0002
#define HCI_DISABLE	0x0000
#define HCI_ENABLE	0x0001
#define HCI_GET		0xfe00,
#define HCI_SET		0xff00

Bool
toshibaDPMS (ScreenPtr pScreen, int mode)
{
    SMMRegisters    regs;
    static int	    fd;

    if (!fd)
	fd = open (TOSH_DEVICE, 2);
    if (fd < 0)
	return FALSE;
    regs.eax = HCI_SET;
    regs.ebx = HCI_BACKLIGHT;
    regs.ecx = mode ? HCI_DISABLE : HCI_ENABLE;
    if (ioctl (fd, TOSH_SMM, &regs) < 0)
	return FALSE;
    return TRUE;
}
#endif /* TOSHIBA_SMM */

Bool
vesaDPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;

#if TOSHIBA_SMM
    if (toshibaDPMS (pScreen, mode))
	return TRUE;
#endif
    if (pscr->mode.vbe)
	return VbeDPMS (priv->vi, priv->vbeInfo, mode);
    return FALSE;
}

void
vesaDisable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = screen->driver;
    int			i=0;
    CARD32		size;
    char		*p;

    switch (pscr->mapping) {
    case VESA_LINEAR:
    case VESA_MONO:
        memcpy(pscr->fb, priv->text, VESA_TEXT_SAVE);
	break;
    case VESA_WINDOWED:
        while(i < VESA_TEXT_SAVE) {
	    p = vesaSetWindowWindowed (pScreen, 0, i, VBE_WINDOW_WRITE, &size);
            if(!p) {
                ErrorF("Couldn't set window for restoring VGA font\n");
                break;
            }
            if(i + size > VESA_TEXT_SAVE)
                size = VESA_TEXT_SAVE - i;
            memcpy(p, ((char*)priv->text) + i, size);
            i += size;
        }
	break;
    case VESA_PLANAR:
	for (i = 0; i < 4; i++)
	{
	    p = vesaSetWindowPlanar (pScreen, 0, i, VBE_WINDOW_WRITE, &size);
	    memcpy (p,
		    ((char *)priv->text) + i * (VESA_TEXT_SAVE/4),
		    (VESA_TEXT_SAVE/4));
	}
	break;
    }
    vesaUnmapFramebuffer (screen);
}

void
vesaPreserve(KdCardInfo *card)
{
    VesaCardPrivPtr priv = card->driver;
    int code;

    /* The framebuffer might not be valid at this point, so we cannot
       save the VGA fonts now; we do it in vesaEnable. */

    if (VbeGetMode (priv->vi, &priv->old_vbe_mode) < 0)
	priv->old_vbe_mode = -1;

    if (VgaGetMode (priv->vi, &priv->old_vga_mode) < 0)
	priv->old_vga_mode = -1;
    
    if (vesa_verbose)
	ErrorF ("Previous modes: VBE 0x%x BIOS 0x%x\n",
		priv->old_vbe_mode, priv->old_vga_mode);
}

void
vesaRestore(KdCardInfo *card)
{
    VesaCardPrivPtr priv = card->driver;
    int		    n;

    if (vesa_force_text)
    {
	if (vesa_verbose)
	    ErrorF ("Forcing switch back to mode 3 text\n");
	priv->old_vbe_mode = -1;
	priv->old_vga_mode = 3;
    }
    for (n = 0; n < priv->nmode; n++)
	if (priv->modes[n].vbe && priv->modes[n].mode == (priv->old_vbe_mode&0x3fff))
	    break;

    if (n < priv->nmode)
    {
	if (vesa_verbose)
	    ErrorF ("Restore VBE mode 0x%x\n", priv->old_vbe_mode);
	VbeSetMode (priv->vi, priv->vbeInfo, priv->old_vbe_mode, 0, 0);
    }
    else
    {
	if (vesa_verbose)
	    ErrorF ("Restore BIOS mode 0x%x\n", priv->old_vga_mode);
	VgaSetMode (priv->vi, priv->old_vga_mode);
    }
}

void
vesaCardFini(KdCardInfo *card)
{
    VesaCardPrivPtr priv = card->driver;
    
    if (priv->vbeInfo)
	VbeCleanup (priv->vi, priv->vbeInfo);
    if (priv->modes)
	xfree (priv->modes);
    Vm86Cleanup(priv->vi);
}

void 
vesaScreenFini(KdScreenInfo *screen)
{
    VesaScreenPrivPtr	pscr = screen->driver;
    VesaCardPrivPtr	priv = screen->card->driver;
    
    vesaUnmapFramebuffer (screen);
    screen->fb[0].depth = pscr->origDepth;
}

int 
vesaSetPalette(VesaCardPrivPtr priv, int first, int number, U8 *entries)
{
    if (priv->vga_palette)
	return VgaSetPalette (priv->vi, first, number, entries);
    else
	return VbeSetPalette (priv->vi, priv->vbeInfo, first, number, entries);
}
    

int 
vesaGetPalette(VesaCardPrivPtr priv, int first, int number, U8 *entries)
{
    int	code;
    
    if (priv->vga_palette)
	code = VgaGetPalette (priv->vi, first, number, entries);
    else
    {
	code = VbeGetPalette (priv->vi, priv->vbeInfo, first, number, entries);
	if (code < 0)
	{
	    priv->vga_palette = 1;
	    code = VgaGetPalette (priv->vi, first, number, entries);
	}
    }
    return code;
}
    
void
vesaPutColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    int	    p;
    CARD8 *scratch;
    int	red, green, blue;
    int min, max;

    if (vesa_swap_rgb)
    {
	red = 2;
	green = 1;
	blue = 0;
    }
    else
    {
	red = 0;
	green = 1;
	blue = 2;
    }

    min = 256;
    max = 0;
    while (n--)
    {
	p = pdefs->pixel;
	if (p < min)
	    min = p;
	if (p > max)
	    max = p;
	scratch = priv->cmap + (p * 4);
        scratch[red] = pdefs->red >> 8;
	scratch[green] = pdefs->green >> 8;
	scratch[blue] = pdefs->blue >> 8;
	scratch[3] = 0;
	pdefs++;
	if (pscr->mapping == VESA_PLANAR)
	{
	    /*
	     * VGA modes are strange; this code covers all
	     * possible modes by duplicating the color information
	     * however the attribute registers might be set
	     */
	    if (p < 16)
	    {
		vesaSetPalette (priv, p, 1, scratch);
		if (p >= 8)
		    vesaSetPalette (priv, p+0x30, 1, scratch);
		else if (p == 6)
		    vesaSetPalette (priv, 0x14, 1, scratch);
	    }
	}
    }
    if (pscr->mapping != VESA_PLANAR)
	vesaSetPalette (priv, min, max-min+1, priv->cmap + min * 4);
}

void
vesaGetColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    VesaCardPrivPtr priv = pScreenPriv->card->driver;
    int first, i, j, k;
    int	red, green, blue;
    int min, max;
    int p;
    CARD8 *scratch;

    if (vesa_swap_rgb)
    {
	red = 2;
	green = 1;
	blue = 0;
    }
    else
    {
	red = 0;
	green = 1;
	blue = 2;
    }
    
    min = 256;
    max = 0;
    for(i = 0; i < n; i++) 
    {
	p = pdefs[i].pixel;
	if (p < min)
	    min = p;
	if (p > max)
	    max = p;
	if (pscr->mapping == VESA_PLANAR)
	    vesaGetPalette (priv, p, 1, priv->cmap + p * 4);
    }
    if (pscr->mapping != VESA_PLANAR)
	vesaGetPalette (priv, min, max - min + 1, priv->cmap + min * 4);
    for (i = 0; i < n; i++)
    {
	p = pdefs[i].pixel;
	scratch = priv->cmap + p * 4;
        pdefs[i].red = scratch[red]<<8;
        pdefs[i].green = scratch[green]<<8;
        pdefs[i].blue = scratch[blue]<<8;
    }
}

int
vesaProcessArgument (int argc, char **argv, int i)
{
    if(!strcmp(argv[i], "-mode")) {
        if(i+1 < argc) {
            vesa_video_mode = strtol(argv[i+1], NULL, 0);
        } else
            UseMsg();
        return 2;
    } else if(!strcmp(argv[i], "-force")) {
        vesa_force_mode = TRUE;
        return 1;
    } else if(!strcmp(argv[i], "-listmodes")) {
        vesaListModes();
        exit(0);
    } else if(!strcmp(argv[i], "-swaprgb")) {
	vesa_swap_rgb = TRUE;
	return 1;
    } else if(!strcmp(argv[i], "-shadow")) {
	vesa_shadow = TRUE;
	return 1;
    } else if(!strcmp(argv[i], "-nolinear")) {
        vesa_linear_fb = FALSE;
        return 1;
    } else if(!strcmp(argv[i], "-verbose")) {
	vesa_verbose = TRUE;
	return 1;
    } else if(!strcmp(argv[i], "-force-text")) {
	vesa_force_text = TRUE;
	return 1;
    }
    
    return 0;
}
