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
/* $XFree86: xc/programs/Xserver/hw/kdrive/vesa/vesa.c,v 1.8 2000/10/20 00:19:50 keithp Exp $ */

#include "vesa.h"

int vesa_video_mode = 0;
Bool vesa_force_mode = FALSE;
Bool vesa_swap_rgb = FALSE;
Bool vesa_shadow = FALSE;
Bool vesa_linear_fb = TRUE;
Bool vesa_restore = FALSE;
Bool vesa_rotate = FALSE;
Bool vesa_verbose = FALSE;

#define VesaPriv(scr)	((VesaScreenPrivPtr) (scr)->driver)

#define ScreenRotated(scr)  (VesaPriv(scr)->rotate)
#define vesaWidth(scr,vmib) (ScreenRotated(scr) ? vmib->YResolution : vmib->XResolution)
#define vesaHeight(scr,vmib) (ScreenRotated(scr) ? vmib->XResolution : vmib->YResolution)

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
    int			depth;
    int			bpp, fbbpp;

    screen->driver = pscr;
    pscr->rotate = FALSE;
    if (screen->width < screen->height)
	pscr->rotate = TRUE;
    
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
    
    pscr->mode = vesaSelectMode (screen);
    
    if (!pscr->mode)
    {
	if (vesa_verbose)
	    ErrorF ("No selectable mode\n");
	return FALSE;
    }

    if (vesa_verbose)
    {
	ErrorF ("\t");
	vesaReportMode (pscr->mode);
    }
    
    pscr->shadow = vesa_shadow;
    pscr->origDepth = screen->fb[0].depth;
    if (vesa_linear_fb)
	pscr->mapping = VESA_LINEAR;
    else
	pscr->mapping = VESA_WINDOWED;
    
    mode = pscr->mode;

    depth = vesaDepth (mode);
    bpp = mode->BitsPerPixel;
    
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
    
    switch (mode->MemoryModel) {
    case MEMORY_DIRECT:
        /* TrueColor or DirectColor */
        screen->fb[0].visuals = (1 << TrueColor);
        screen->fb[0].redMask = 
            FbStipMask(mode->RedFieldPosition, mode->RedMaskSize);
        screen->fb[0].greenMask = 
            FbStipMask(mode->GreenFieldPosition, mode->GreenMaskSize);
        screen->fb[0].blueMask = 
            FbStipMask(mode->BlueFieldPosition, mode->BlueMaskSize);
        allbits = 
            screen->fb[0].redMask | 
            screen->fb[0].greenMask | 
            screen->fb[0].blueMask;
        depth = 32;
        while (depth && !(allbits & (1 << (depth - 1))))
            depth--;
	if (vesa_verbose)
	    ErrorF ("\tTrue Color bpp %d depth %d red 0x%x green 0x%x blue 0x%x\n",
		    bpp, depth, 
		    screen->fb[0].redMask,
		    screen->fb[0].greenMask,
		    screen->fb[0].blueMask);
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
	if (vesa_verbose)
	    ErrorF ("\tPseudo Color bpp %d depth %d\n",
		    bpp, depth);
	break;
    case MEMORY_PLANAR:
	/* 4 plane planar */
	if (mode->ModeAttributes & MODE_COLOUR)
	    screen->fb[0].visuals = (1 << StaticColor);
	else
	    screen->fb[0].visuals = (1 << StaticGray);
	screen->fb[0].blueMask  = 0x00;
	screen->fb[0].greenMask = 0x00;
	screen->fb[0].redMask   = 0x00;
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
	pscr->rotate = FALSE;
	break;
    default:
        ErrorF("Unsupported VESA MemoryModel 0x%02X\n",
               mode->MemoryModel);
        return FALSE;
    }

    screen->width = vesaWidth(screen, mode);
    screen->height = vesaHeight(screen, mode);
    screen->fb[0].depth = depth;
    screen->fb[0].bitsPerPixel = bpp;
    screen->fb[0].byteStride = mode->BytesPerScanLine;
    screen->fb[0].pixelStride = ((mode->BytesPerScanLine * 8) / fbbpp);

    if (pscr->mapping == VESA_LINEAR && !(mode->ModeAttributes & MODE_LINEAR))
	pscr->mapping = VESA_WINDOWED;
    
    if (pscr->rotate)
	pscr->shadow = TRUE;
    
    switch (pscr->mapping) {
    case VESA_MONO:
	pscr->shadow = TRUE;
	/* fall through */
    case VESA_LINEAR:
	if (mode->vbe)
	    pscr->fb = VbeMapFramebuffer(priv->vi, priv->vbeInfo, 
					 pscr->mode->mode,
					 &pscr->fb_size);
	else
	    pscr->fb = VgaMapFramebuffer (priv->vi, 
					  pscr->mode->mode,
					  &pscr->fb_size);
	break;
    case VESA_WINDOWED:
	pscr->fb = NULL;
	pscr->shadow = TRUE;
	break;
    case VESA_PLANAR:
	pscr->fb = NULL;
	pscr->shadow = TRUE;
	break;
    }

    screen->rate = 72;
    screen->fb[0].frameBuffer = (CARD8 *)(pscr->fb);

    if (pscr->rotate)
	screen->softCursor = TRUE;
    
    if (pscr->shadow)
	return KdShadowScreenInit (screen);
    
    if (vesa_verbose)
	ErrorF ("Mode selected %dx%dx%d\n",
		screen->width, screen->height, screen->fb[0].depth);
    
    return TRUE;
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
    if (pscr->mode->vbe)
    {
	base = VbeSetWindow (priv->vi,
			     priv->vbeInfo,
			     pscr->mode->BytesPerScanLine * row + offset,
			     mode,
			     &winSize);
    }
    else
    {
	base = VgaSetWindow (priv->vi,
			     pscr->mode->mode,
			     pscr->mode->BytesPerScanLine * row + offset,
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

    *size = pscr->mode->BytesPerScanLine;
    return (CARD8 *) pscr->fb + row * pscr->mode->BytesPerScanLine + offset;
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

    if (pscr->mode->vbe)
    {
	base = VbeSetWindow (priv->vi,
			     priv->vbeInfo,
			     pscr->mode->BytesPerScanLine * row + offset,
			     mode,
			     &winSize);
    }
    else
    {
	base = VgaSetWindow (priv->vi,
			     pscr->mode->mode,
			     pscr->mode->BytesPerScanLine * row + offset,
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
		  CARD32    *size)
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
		  CARD32    *size)
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
		    CARD32	*size)
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
	       CARD32	    *size)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int			line;
    
    if (!pScreenPriv->enabled)
	return 0;
    *size = pscr->mode->BytesPerScanLine;
    line = ((row & 1) << 13) + (row >> 1) * pscr->mode->BytesPerScanLine;
    return (CARD8 *) pscr->fb + line + offset;
}

void
vesaUpdateMono (ScreenPtr pScreen,
		PixmapPtr pShadow,
		RegionPtr damage)
{
    shadowScrPriv(pScreen);
    int		nbox = REGION_NUM_RECTS (damage);
    BoxPtr	pbox = REGION_RECTS (damage);
    FbBits	*shaBase, *shaLine, *sha;
    FbBits	s;
    FbStride	shaStride;
    int		scrBase, scrLine, scr;
    int		shaBpp;
    int		x, y, w, h, width;
    int         i;
    FbBits	*winBase, *winLine, *win;
    CARD32      winSize;
    FbBits	bits;
    int		plane;

    fbGetDrawable (&pShadow->drawable, shaBase, shaStride, shaBpp);
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
		    winBase = (FbBits *) (*pScrPriv->window) (pScreen,
							      y,
							      scr * sizeof (FbBits),
							      SHADOW_WINDOW_WRITE,
							      &winSize);
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

    for (i = 0; i < pmap->pVisual->ColormapEntries; i++)
    {
	j = i & 0xf;
	pmap->red[i].co.local.red = (vga16Colors[j][0]<<8)|vga16Colors[j][0];
	pmap->red[i].co.local.green = (vga16Colors[j][1]<<8)|vga16Colors[j][1];
	pmap->red[i].co.local.blue = (vga16Colors[j][2]<<8)|vga16Colors[j][2];
    }
    return TRUE;
}


Bool
vesaInitScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;
    
    if (pscr->shadow)
    {
	switch (pscr->mapping) {
	case VESA_LINEAR:
	    update = shadowUpdatePacked;
	    window = vesaWindowLinear;
	    break;
	case VESA_WINDOWED:
	    update = shadowUpdatePacked;
	    window = vesaWindowWindowed;
	    break;
	case VESA_PLANAR:
	    pScreen->CreateColormap = vesaCreateColormap16;
	    if (pScreenPriv->screen->fb[0].bitsPerPixel == 8)
		update = shadowUpdatePlanar4x8;
	    else
		update = shadowUpdatePlanar4;
	    window = vesaWindowPlanar;
	    pscr->rotate = FALSE;
	    break;
	case VESA_MONO:
	    update = vesaUpdateMono;
	    if (pscr->mode->mode < 8)
		window = vesaWindowCga;
	    else
		window = vesaWindowLinear;
	    pscr->rotate = FALSE;
	    break;
	}
	if (pscr->rotate)
	{
	    switch (pScreenPriv->screen->fb[0].bitsPerPixel) {
	    case 8:
		update = shadowUpdateRotate8; break;
	    case 16:
		update = shadowUpdateRotate16; break;
	    case 32:
		update = shadowUpdateRotate32; break;
	    }
	}
	
        return KdShadowInitScreen (pScreen, update, window);
    }
    
    return TRUE;
}

Bool
vesaEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int			code;
    int			i;
    CARD32		size;
    char		*p;
    KdMouseMatrix	m;

    if (pscr->mode->vbe)
    {
	if (vesa_verbose)
	    ErrorF ("Enable VBE mode 0x%x\n", pscr->mode->mode);
	code = VbeSetMode(priv->vi, priv->vbeInfo, pscr->mode->mode,
			  pscr->mapping == VESA_LINEAR);
    }
    else
    {
	if (vesa_verbose)
	    ErrorF ("Enable BIOS mode 0x%x\n", pscr->mode->mode);
	code = VgaSetMode (priv->vi, pscr->mode->mode);
    }
    
    if(code < 0)
	return FALSE;
    
    switch (pscr->mapping) {
    case VESA_MONO:
	VgaSetWritePlaneMask (priv->vi, 0x1);
    case VESA_LINEAR:
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
    if (pscr->rotate)
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

void
vesaDisable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
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

    for (n = 0; n < priv->nmode; n++)
	if (priv->modes[n].vbe && priv->modes[n].mode == (priv->old_vbe_mode&0x3fff))
	    break;

    if (n < priv->nmode)
    {
	if (vesa_verbose)
	    ErrorF ("Restore VBE mode 0x%x\n", priv->old_vbe_mode);
	VbeSetMode (priv->vi, priv->vbeInfo, priv->old_vbe_mode, 0);
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
    Vm86Cleanup(priv->vi);
}

void 
vesaScreenFini(KdScreenInfo *screen)
{
    VesaScreenPrivPtr	pscr = screen->driver;
    VesaCardPrivPtr	priv = screen->card->driver;
    
    if (pscr->fb)
    {
	if (pscr->mode->vbe)
	    VbeUnmapFramebuffer(priv->vi, priv->vbeInfo, pscr->mode->mode, pscr->fb);
	else
	    VgaUnmapFramebuffer (priv->vi);
    }
    
    if (pscr->shadow)
	KdShadowScreenFini (screen);
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
    CARD8 scratch[4];
    int	red, green, blue;

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

    while (n--)
    {
        scratch[red] = pdefs->red >> 8;
	scratch[green] = pdefs->green >> 8;
	scratch[blue] = pdefs->blue >> 8;
	scratch[3] = 0;
	p = pdefs->pixel;
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
	else
	    vesaSetPalette(priv, p, 1, scratch);
    }
}

void
vesaGetColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr priv = pScreenPriv->card->driver;
    int first, i, j, k;
    CARD8 scratch[4];
    int	red, green, blue;

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
    
    for(i = 0; i<n; i++) {
        vesaGetPalette(priv, pdefs[i].pixel, 1, scratch);
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
    }
    
    return 0;
}
