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

#include "vesa.h"

int vesa_video_mode = 0;
Bool vesa_force_mode = FALSE;
Bool vesa_swap_rgb = FALSE;
Bool vesa_shadow = FALSE;
Bool vesa_linear_fb = TRUE;
Bool vesa_restore = FALSE;

static Bool
vesaModeSupported(VbeInfoPtr vi, VbeModeInfoBlock *vmib, Bool complain)
{
    if((vmib->ModeAttributes & 0x10) == 0) {
        if(complain)
            ErrorF("Text mode specified.\n");
        return FALSE;
    }
    if(vmib->MemoryModel != 0x06 && vmib->MemoryModel != 0x04 && vmib->MemoryModel != 0x03) {
        if(complain)
            ErrorF("Unsupported memory model 0x%X\n", vmib->MemoryModel);
        return FALSE;
    }
    if((vmib->ModeAttributes & 0x80) == 0) {
	if ((vmib->WinAAttributes & 0x5) != 0x5) {
	    if(complain)
		ErrorF("Neither linear nor windowed framebuffer available in this mode\n");
	    return FALSE;
	}
    }
    if(!(vmib->ModeAttributes & 1)) {
        if(complain)
            ErrorF("Mode not supported on this hardware\n");
        return FALSE;
    }
    return TRUE;
}

Bool
vesaListModes()
{
    int code;
    VbeInfoPtr vi = NULL;
    VbeInfoBlock *vib;
    VbeModeInfoBlock *vmib;
    unsigned p, num_modes, i;
    CARD16 *modes_list = NULL;

    vi = VbeSetup();
    if(!vi)
        goto fail;

    vib = VbeGetInfo(vi);
    if(!vib)
        goto fail;

    VbeReportInfo(vi, vib);
    /* The spec says you need to copy the list */
    p = MAKE_POINTER_1(vib->VideoModePtr);
    num_modes = 0;
    while(VbeMemoryW(vi, p) != 0xFFFF) {
        num_modes++;
        p+=2;
    }
    modes_list = ALLOCATE_LOCAL(num_modes * sizeof(CARD16));
    if(!modes_list)
        goto fail;
    p = MAKE_POINTER_1(vib->VideoModePtr);
    for(i=0; i<num_modes; i++) {
        modes_list[i] = VbeMemoryW(vi, p);
        p += 2;
    }
        
    for(i=0; i<num_modes; i++) {
        vmib = VbeGetModeInfo(vi, modes_list[i]);
        if(!vmib)
            goto fail;
        if(vesa_force_mode || vesaModeSupported(vi, vmib, FALSE))
           VbeReportModeInfo(vi, modes_list[i], vmib);
    }

    if(modes_list)
        DEALLOCATE_LOCAL(modes_list);
    VbeCleanup(vi);
    return TRUE;

  fail:
    if(modes_list)
        DEALLOCATE_LOCAL(modes_list);
    VbeCleanup(vi);
    return FALSE;
}

Bool
vesaGetModes (KdCardInfo *card, VesaCardPrivPtr priv)
{
    VesaModePtr		mode;
    int			nmode;
    unsigned int	i;
    VbeInfoPtr		vi = priv->vi;
    VbeInfoBlock	*vib = priv->vib;
    VbeModeInfoBlock	*vmib;

    /* The spec says you need to copy the list */
    i = MAKE_POINTER_1(vib->VideoModePtr);
    nmode = 0;
    while(VbeMemoryW(vi, i) != 0xFFFF) {
        nmode++;
        i+=2;
    }
    if (!nmode)
	return FALSE;
    priv->modes = xalloc (nmode * sizeof (VesaModeRec));
    if (!priv->modes)
	return FALSE;
    priv->nmode = nmode;
    i = MAKE_POINTER_1(vib->VideoModePtr);
    nmode = 0;
    while(nmode < priv->nmode) {
	priv->modes[nmode].mode = VbeMemoryW(vi, i);
        nmode++;
        i+=2;
    }
    i = MAKE_POINTER_1(vib->VideoModePtr);
    nmode = 0;
    while(nmode < priv->nmode) {
	vmib = VbeGetModeInfo(vi, priv->modes[nmode].mode);
	if(!vmib)
	    break;
	priv->modes[nmode].vmib = *vmib;
	i += 2;
	nmode++;
    }
    return TRUE;
}


Bool
vesaInitialize (KdCardInfo *card, VesaCardPrivPtr priv)
{
    int code;
    
    priv->vi = VbeSetup();
    if(!priv->vi)
        goto fail;

    priv->vib = VbeGetInfo(priv->vi);
    if(!priv->vib) 
        goto fail;

    code = VbeSetupStateBuffer(priv->vi);
    if(code < 0)
        goto fail;

    code = VbeSaveState(priv->vi);
    if(code<0)
        goto fail;

    if (!vesaGetModes (card, priv))
	goto fail;
    
    card->driver = priv;

    return TRUE;

  fail:
    if(priv->vi)
	VbeCleanup(priv->vi);
    return FALSE;
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
vesaDepth (VbeModeInfoBlock *m)
{
    if (m->MemoryModel == 0x06)
	return (m->RedMaskSize +
		m->GreenMaskSize +
		m->BlueMaskSize);
    else
	return m->BitsPerPixel;
}

Bool
vesaModeGood (KdScreenInfo	    *screen,
	      VbeModeInfoBlock	    *a)
{
    if (a->XResolution <= screen->width &&
	a->YResolution <= screen->height &&
	vesaDepth (a) >= screen->fb[0].depth)
    {
	return TRUE;
    }
}

#define vabs(a)	((a) >= 0 ? (a) : -(a))

int
vesaSizeError (KdScreenInfo          *screen,
	       VbeModeInfoBlock      *a)
{
    int     xdist, ydist;
    xdist = vabs (screen->width - a->XResolution);
    ydist = vabs (screen->height - a->YResolution);
    return xdist * xdist + ydist * ydist;
}

Bool
vesaModeBetter (KdScreenInfo	    *screen,
		VbeModeInfoBlock    *a,
		VbeModeInfoBlock    *b)
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
		(vesaModeSupported (priv->vi, &priv->modes[best].vmib, FALSE) ||
		 vesa_force_mode))
		return &priv->modes[best];
    }
    for (best = 0; best < priv->nmode; best++)
    {
	if (vesaModeSupported (priv->vi, &priv->modes[best].vmib, FALSE))
	    break;
    }
    if (best == priv->nmode)
	return 0;
    for (i = best + 1; i < priv->nmode; i++)
	if (vesaModeSupported (priv->vi, &priv->modes[i].vmib, FALSE) &&
	    vesaModeBetter (screen, &priv->modes[i].vmib, 
			    &priv->modes[best].vmib))
	    best = i;
    return &priv->modes[best];
}   
    
Bool
vesaScreenInitialize (KdScreenInfo *screen, VesaScreenPrivPtr pscr)
{
    VesaCardPrivPtr	priv = screen->card->driver;
    VbeModeInfoBlock	*vmib;
    Pixel allbits;
    int	depth;

    pscr->mode = vesaSelectMode (screen);
    if (!pscr->mode)
	return FALSE;

    pscr->shadow = vesa_shadow;
    if (vesa_linear_fb)
	pscr->mapping = VESA_LINEAR;
    else
	pscr->mapping = VESA_WINDOWED;
    
    vmib = &pscr->mode->vmib;

    screen->width = vmib->XResolution;
    screen->height = vmib->YResolution;
    screen->fb[0].depth = vesaDepth (vmib);
    screen->fb[0].bitsPerPixel = vmib->BitsPerPixel;
    screen->fb[0].byteStride = vmib->BytesPerScanLine;
    screen->fb[0].pixelStride = ((vmib->BytesPerScanLine * 8) / 
				 vmib->BitsPerPixel);

    switch (vmib->MemoryModel) {
    case 0x06:
        /* TrueColor or DirectColor */
        screen->fb[0].visuals = (1 << TrueColor);
        screen->fb[0].redMask = 
            FbStipMask(vmib->RedFieldPosition, vmib->RedMaskSize);
        screen->fb[0].greenMask = 
            FbStipMask(vmib->GreenFieldPosition, vmib->GreenMaskSize);
        screen->fb[0].blueMask = 
            FbStipMask(vmib->BlueFieldPosition, vmib->BlueMaskSize);
        allbits = 
            screen->fb[0].redMask | 
            screen->fb[0].greenMask | 
            screen->fb[0].blueMask;
        depth = 32;
        while (depth && !(allbits & (1 << (depth - 1))))
            depth--;
        screen->fb[0].depth = depth;
	break;
    case 0x04:
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
    case 0x03:
	/* 4 plane planar */
	screen->fb[0].visuals = (1 << StaticColor);
	screen->fb[0].blueMask  = 0x00;
	screen->fb[0].greenMask = 0x00;
	screen->fb[0].redMask   = 0x00;
	screen->fb[0].depth = 4;
	screen->fb[0].bitsPerPixel = 4;
	pscr->mapping = VESA_PLANAR;
	break;
    default:
        ErrorF("Unsupported VESA MemoryModel 0x%02X\n",
               vmib->MemoryModel);
        return FALSE;
    }

    if (pscr->mapping == VESA_LINEAR && !(vmib->ModeAttributes & 0x80))
	pscr->mapping = VESA_WINDOWED;
    
    switch (pscr->mapping) {
    case VESA_LINEAR:
	pscr->fb = VbeMapFramebuffer(priv->vi, vmib);
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

    if (pscr->shadow)
	return KdShadowScreenInit (screen);
    
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
    screen->driver = pscr;
    return TRUE;
}

void *
vesaWindowPlanar (ScreenPtr pScreen,
		  CARD32    row,
		  CARD32    offset,
		  int	    mode,
		  CARD32    *size)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    VbeModeInfoBlock	*vmib = &pscr->mode->vmib;
    static int		plane;
    int			winSize;
    void		*base;

    if (!pScreenPriv->enabled)
	return 0;
    plane = offset & 3;
    VbeSetWritePlaneMask (priv->vi, (1 << plane));
    offset = offset >> 2;
    base = VbeSetWindow (priv->vi,
			 vmib->BytesPerScanLine * row + offset,
			 mode,
			 &winSize);
    *size = (CARD32) winSize;
    return base;
}

void *
vesaWindowLinear (ScreenPtr pScreen,
		  CARD32    row,
		  CARD32    offset,
		  int	    mode,
		  CARD32    *size)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    VbeModeInfoBlock	*vmib = &pscr->mode->vmib;

    if (!pScreenPriv->enabled)
	return 0;
    *size = vmib->BytesPerScanLine;
    return (CARD8 *) pscr->fb + row * vmib->BytesPerScanLine + offset;
}

void *
vesaWindowWindowed (ScreenPtr	pScreen,
		    CARD32	row,
		    CARD32	offset,
		    int		mode,
		    CARD32	*size)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    VbeModeInfoBlock	*vmib = &pscr->mode->vmib;
    int			winSize;
    void		*base;

    if (!pScreenPriv->enabled)
	return 0;
    base = VbeSetWindow (priv->vi,
			 vmib->BytesPerScanLine * row + offset,
			 mode,
			 &winSize);
    *size = (CARD32) winSize;
    return base;
}

static CARD16	vga16Colors[16][3] = {
    { 0,   0,   0,   },       /*  0 */
    { 0x80,0,   0,   },       /*  1 */
    { 0,   0x80,0,   },       /*  2 */
    { 0x80,0x80,0,   },       /*  3 */
    { 0,   0,   0x80,},       /*  4 */
    { 0x80,0,   0x80,},       /*  5 */
    { 0,   0x80,0x80,},       /*  6 */
    { 0x80,0x80,0x80,},       /*  7 */
    { 0xC0,0xC0,0xC0,},       /*  8 */
    { 0xFF,0,   0   ,},       /*  9 */
    { 0,   0xFF,0   ,},       /* 10 */
    { 0xFF,0xFF,0   ,},       /* 11 */
    { 0   ,0,   0xFF,},       /* 12 */
    { 0xFF,0,   0xFF,},       /* 13 */
    { 0,   0xFF,0xFF,},       /* 14 */
    { 0xFF,0xFF,0xFF,},       /* 15 */
};

Bool
vesaCreateColormap16 (ColormapPtr pmap)
{
    int	    i;

    for (i = 0; i < 16; i++)
    {
	pmap->red[i].co.local.red = (vga16Colors[i][0]<<8)|vga16Colors[i][0];
	pmap->red[i].co.local.green = (vga16Colors[i][1]<<8)|vga16Colors[i][1];
	pmap->red[i].co.local.blue = (vga16Colors[i][2]<<8)|vga16Colors[i][2];
    }
    return TRUE;
}


Bool
vesaInitScreen(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    
    if (pscr->shadow)
    {
	switch (pscr->mapping) {
	case VESA_LINEAR:
	    return KdShadowInitScreen (pScreen, shadowUpdatePacked, vesaWindowLinear);
	case VESA_WINDOWED:
	    return KdShadowInitScreen (pScreen, shadowUpdatePacked, vesaWindowWindowed);
	case VESA_PLANAR:
	    pScreen->CreateColormap = vesaCreateColormap16;
	    return KdShadowInitScreen (pScreen, shadowUpdatePlanar4, vesaWindowPlanar);
	}
    }
    
    return TRUE;
}

Bool
vesaEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int code;
    int palette_wait = 0, palette_hi = 0;
    int i=0;
    int size;
    char *p;

    code = VbeSetMode(priv->vi, pscr->mode->mode, pscr->mapping == VESA_LINEAR);
    if(code < 0)
	return FALSE;

    if(priv->vib->Capabilities[0] & 1)
        palette_hi = 1;
    if(priv->vib->Capabilities[0] & 4)
        palette_wait = 1;
    if(palette_hi || palette_wait)
        VbeSetPaletteOptions(priv->vi, palette_hi?8:6, palette_wait);

    switch (pscr->mapping) {
    case VESA_LINEAR:
	memcpy (priv->text, pscr->fb, VESA_TEXT_SAVE);
	break;
    case VESA_WINDOWED:
        while(i < VESA_TEXT_SAVE) {
            p = VbeSetWindow(priv->vi, i, VBE_WINDOW_READ, &size);
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
	p = VbeSetWindow (priv->vi, 0, VBE_WINDOW_READ, &size);
	for (i = 0; i < 4; i++)
	{
	    VbeSetReadPlaneMap (priv->vi, i);
	    memcpy (((char *)priv->text) + i * (VESA_TEXT_SAVE/4), p,
		    (VESA_TEXT_SAVE/4));
	}
	break;
    }
    return TRUE;
}

void
vesaDisable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaCardPrivPtr priv = pScreenPriv->card->driver;
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    int i=0;
    int size;
    char *p;

    switch (pscr->mapping) {
    case VESA_LINEAR:
        memcpy(pscr->fb, priv->text, VESA_TEXT_SAVE);
	break;
    case VESA_WINDOWED:
        while(i < VESA_TEXT_SAVE) {
            p = VbeSetWindow(priv->vi, i, VBE_WINDOW_WRITE, &size);
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
	p = VbeSetWindow (priv->vi, 0, VBE_WINDOW_WRITE, &size);
	for (i = 0; i < 4; i++)
	{
	    VbeSetWritePlaneMask (priv->vi, 1 << i);
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

    code = VbeSaveState(priv->vi);
    if(code < 0)
        FatalError("Couldn't save state\n");

    return;
}

void
vesaRestore(KdCardInfo *card)
{
    VesaCardPrivPtr priv = card->driver;
    VbeRestoreState(priv->vi);
    return;
}

void
vesaCardFini(KdCardInfo *card)
{
    VesaCardPrivPtr priv = card->driver;
    if (vesa_restore)
	VbeSetTextMode(priv->vi,3);
    VbeCleanup(priv->vi);
    return;
}

void 
vesaScreenFini(KdScreenInfo *screen)
{
    VesaScreenPrivPtr	pscr = screen->driver;
    VesaCardPrivPtr	priv = screen->card->driver;
    
    if (pscr->fb)
	VbeUnmapFramebuffer(priv->vi, &pscr->mode->vmib, pscr->fb);
    return;
}

void
vesaPutColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    VesaScreenPrivPtr	pscr = pScreenPriv->screen->driver;
    VesaCardPrivPtr	priv = pScreenPriv->card->driver;
    int i, j, k, start;
    CARD8 scratch[4*256];
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

    i = 0;
    while(i < n) {
        j = i + 1;
        /* For some reason, doing more than one entry at a time breaks */
        while(j < n && pdefs[j].pixel == pdefs[j-1].pixel + 1 && j - i < 1)
            j++;
        for(k=0; k<(j - i); k++) {
            scratch[k+red] = pdefs[i+k].red >> 8;
            scratch[k+green] = pdefs[i+k].green >> 8;
            scratch[k+blue] = pdefs[i+k].blue >> 8;
            scratch[k+3] = 0;
        }
	start = pdefs[i].pixel;
	if (pscr->mapping == VESA_PLANAR)
	{
	    for (start = pdefs[i].pixel; start < 256; start += 16)
		VbeSetPalette(priv->vi, start, j - i, scratch);
	}
	else
	    VbeSetPalette(priv->vi, start, j - i, scratch);
        i = j;
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
        VbeGetPalette(priv->vi, pdefs[i].pixel, 1, scratch);
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
    } else if(!strcmp(argv[i], "-restore")) {
	vesa_restore = TRUE;
	return 1;
    }
    
    return 0;
}
