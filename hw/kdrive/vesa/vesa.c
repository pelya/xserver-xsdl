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

#define DEFAULT_MODE 0x115

int vesa_video_mode = DEFAULT_MODE;
Bool vesa_force_mode = FALSE;
Bool vesa_swap_rgb = FALSE;

static Bool
vesaModeSupported(VbeInfoPtr vi, VbeModeInfoBlock *vmib, Bool complain)
{
    if((vmib->ModeAttributes & 0x10) == 0) {
        if(complain)
            ErrorF("Text mode specified.\n");
        return FALSE;
    }
    if(vmib->MemoryModel != 0x06 && vmib->MemoryModel != 0x04) {
        if(complain)
            ErrorF("Unsupported memory model 0x%X\n", vmib->MemoryModel);
        return FALSE;
    }
    if((vmib->ModeAttributes & 0x80) == 0) {
        if(complain)
            ErrorF("No linear framebuffer available in this mode\n");
        return FALSE;
    }
    if(!(vmib->ModeAttributes&1)) {
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
    unsigned i;

    vi = VbeSetup();
    if(!vi)
        goto fail;

    vib = VbeGetInfo(vi);
    if(!vib)
        goto fail;

    VbeReportInfo(vi, vib);
    i = MAKE_POINTER_1(vib->VideoModePtr);

    while(VbeMemoryW(vi, i) != 0xFFFF) {
        vmib = VbeGetModeInfo(vi, VbeMemoryW(vi, i));
        if(!vmib)
            goto fail;
        if(vesa_force_mode || vesaModeSupported(vi, vmib, FALSE))
           VbeReportModeInfo(vi, VbeMemoryW(vi, i), vmib);
        i+=2;
    }

    VbeCleanup(vi);
    return TRUE;

  fail:
    VbeCleanup(vi);
    return FALSE;
}

Bool
vesaInitialize (KdCardInfo *card, VesaPrivPtr priv)
{
    int code;
    
    priv->mode = vesa_video_mode;

    priv->vi = VbeSetup();
    if(!priv->vi)
        goto fail;

    priv->vib = VbeGetInfo(priv->vi);
    if(!priv->vib) 
        goto fail;

    priv->vmib = VbeGetModeInfo(priv->vi, priv->mode);
    if(!priv->vmib)
        goto fail;

    if(!vesa_force_mode && !vesaModeSupported(priv->vi, priv->vmib, TRUE))
       goto fail;

    code = VbeSetupStateBuffer(priv->vi);
    if(code < 0)
        goto fail;

    code = VbeSaveState(priv->vi);
    if(code<0)
        goto fail;

    priv->fb = VbeMapFramebuffer(priv->vi);
    if(!priv->vi)
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
    VesaPrivPtr priv;

    priv = xalloc(sizeof(VesaPrivRec));
    if(!priv)
        return FALSE;

    if (!vesaInitialize (card, priv))
    {
        xfree(priv);
	return FALSE;
    }
    
    return TRUE;
}

Bool
vesaScreenInit(KdScreenInfo *screen)
{
    VesaPrivPtr priv = screen->card->driver;
    Pixel allbits;
    int	depth;

    screen->width = priv->vmib->XResolution;
    screen->height = priv->vmib->YResolution;
    screen->fb[0].depth = priv->vmib->BitsPerPixel;
    screen->fb[0].bitsPerPixel = priv->vmib->BitsPerPixel;
    screen->fb[0].byteStride = priv->vmib->BytesPerScanLine;
    screen->fb[0].pixelStride = 
        (priv->vmib->BytesPerScanLine * 8) / priv->vmib->BitsPerPixel;

    if(priv->vmib->MemoryModel == 0x06) {
        /* TrueColor or DirectColor */
        screen->fb[0].visuals = (1 << TrueColor);
        screen->fb[0].redMask = 
            FbStipMask(priv->vmib->RedFieldPosition, priv->vmib->RedMaskSize);
        screen->fb[0].greenMask = 
            FbStipMask(priv->vmib->GreenFieldPosition, priv->vmib->GreenMaskSize);
        screen->fb[0].blueMask = 
            FbStipMask(priv->vmib->BlueFieldPosition, priv->vmib->BlueMaskSize);
        allbits = 
            screen->fb[0].redMask | 
            screen->fb[0].greenMask | 
            screen->fb[0].blueMask;
        depth = 32;
        while (depth && !(allbits & (1 << (depth - 1))))
            depth--;
        screen->fb[0].depth = depth;
    } else if (priv->vmib->MemoryModel == 0x04) {
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
    } else {
        ErrorF("Unsupported VESA MemoryModel 0x%02X\n",
               priv->vmib->MemoryModel);
        return FALSE;
    }

    screen->rate = 72;
    screen->fb[0].frameBuffer = (CARD8 *)(priv->fb);

    return TRUE;
}

Bool
vesaInitScreen(ScreenPtr pScreen)
{
    return TRUE;
}

void
vesaEnable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaPrivPtr priv = pScreenPriv->card->driver;
    int code;
    int palette_wait = 0, palette_hi = 0;

    code = VbeSetMode(priv->vi, priv->mode);
    if(code < 0)
        FatalError("Couldn't set mode\n");

    if(priv->vib->Capabilities[0] & 1)
        palette_hi = 1;
    if(priv->vib->Capabilities[0] & 4)
        palette_wait = 1;
    if(palette_hi || palette_wait)
        VbeSetPaletteOptions(priv->vi, palette_hi?8:6, palette_wait);

    return;
}

void
vesaDisable(ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    VesaPrivPtr priv = pScreenPriv->card->driver;

}

void
vesaPreserve(KdCardInfo *card)
{
    VesaPrivPtr priv = card->driver;
    int code;

    code = VbeSaveState(priv->vi);
    if(code < 0)
        FatalError("Couldn't save state\n");

    return;
}

void
vesaRestore(KdCardInfo *card)
{
    VesaPrivPtr priv = card->driver;
    VbeRestoreState(priv->vi);
    return;
}

void
vesaCardFini(KdCardInfo *card)
{
    VesaPrivPtr priv = card->driver;
    VbeUnmapFramebuffer(priv->vi);
    VbeCleanup(priv->vi);
    return;
}

void 
vesaScreenFini(KdScreenInfo *screen)
{
  return;
}

void
vesaPutColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    VesaPrivPtr priv = pScreenPriv->card->driver;
    int i, j, k;
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
        VbeSetPalette(priv->vi, pdefs[i].pixel, j - i, scratch);
        i = j;
    }
}

void
vesaGetColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    KdScreenPriv(pScreen);
    VesaPrivPtr priv = pScreenPriv->card->driver;
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
    }
    
    return 0;
}
