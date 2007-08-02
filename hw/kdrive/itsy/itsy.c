/*
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

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "itsy.h"

/* struct with LCD characteristics defined in fb_brutus.h  */
static struct FbLcdParamsStruct fbLcdParams; 
static int fb_d;
static int fbn;
Bool
itsyCardInit (KdCardInfo *card)
{
    int	    k;
    char    *fb;
    char    *pixels;

    if ((fb_d = open("/dev/fbclone", O_RDWR)) < 0) {
	perror("Error opening /dev/fb\n");
	return FALSE;
    }
    if ((k=ioctl(fb_d, FB_LCD_PARAMS, &fbLcdParams)) != 0) {
	perror("Error with /dev/fb ioctl FB_LCD_PARAMS call");
	return FALSE;
    }

    fb = (char *) mmap ((caddr_t) NULL, fbLcdParams.frameBufferSize,
			PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fb_d, 0);

    fprintf (stderr, "fb mapped at 0x%x\n", fb);
    if (fb == (char *)-1) {
	perror("ERROR: mmap framebuffer fails!");
	return FALSE;
    }
    
    card->driver = fb;
    
    return TRUE;
}

Bool
itsyScreenInit (KdScreenInfo *screen)
{
    CARD8   *fb = screen->card->driver;

    screen->width = fbLcdParams.screenSizeH;
    screen->height = fbLcdParams.screenSizeV;
    screen->depth = fbLcdParams.bitsPerPixel;
    screen->bitsPerPixel = fbLcdParams.bitsPerPixel;
    screen->byteStride = fbLcdParams.frameBufferSizeH;
    screen->pixelStride = (fbLcdParams.frameBufferSizeH * 8 / 
			   fbLcdParams.bitsPerPixel);
    fprintf (stderr, "width %d height %d depth %d pstride %d bstride %d\n",
	     screen->width, screen->height, screen->depth, 
	     screen->pixelStride, screen->byteStride);
    screen->dumb = FALSE;
    screen->softCursor = TRUE;
    screen->blueMask = 0;
    screen->greenMask = 0;
    screen->redMask = 0;
    screen->visuals = 1 << StaticGray;
    screen->rate = 72;
    screen->frameBuffer = (CARD8 *) (fb + 
				     fbLcdParams.pixelDataOffset +
				     (fbLcdParams.reserveTopRows * 
				      screen->byteStride));
    fprintf (stderr, "Frame buffer 0x%x\n", screen->frameBuffer);
    return TRUE;
}

static unsigned short itsyIntensity[16] = {
    0xffff,
    0xffff,
    0xedb6,
    0xdb6d,
    0xc924,
    0xb6db,
    0xa492,
    0x9249,
    0x8000,
    0x6db6,
    0x5b6d,
    0x4924,
    0x36db,
    0x2492,
    0x1249,
    0x0000,
};

Bool
itsyCreateColormap (ColormapPtr pmap)
{
    int	    i;

    for (i = 0; i < 16; i++)
    {
	pmap->red[i].co.local.red = itsyIntensity[i];
	pmap->red[i].co.local.green = itsyIntensity[i];
	pmap->red[i].co.local.blue = itsyIntensity[i];
    }
    return TRUE;
}

Bool
itsyInitScreen (ScreenPtr pScreen)
{
    pScreen->CreateColormap = itsyCreateColormap;
    return TRUE;
}

void
itsyPreserve (KdCardInfo *card)
{
}

void
itsyEnable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);

    fprintf (stderr, "Enabling LCD display\n");
    /* display it on the LCD */
    ioctl(fb_d, FB_LCD_SHOW, 0);
}

Bool
itsyDPMS (ScreenPtr pScreen, int mode)
{
    if (mode)
	ioctl (fb_d, FB_LCD_OFF, 0);
    else
	ioctl (fb_d, FB_LCD_ON, 0);
    return TRUE;
}

void
itsyDisable (ScreenPtr pScreen)
{
/*    ioctl (fb_d, FB_LCD_SWITCH, 0); */
/*    fprintf (stderr, "Disabling LCD display\n");*/
}

void
itsyRestore (KdCardInfo *card)
{
}

void
itsyScreenFini (KdScreenInfo *screen)
{
}

void
itsyCardFini (KdCardInfo *card)
{
    int	k;
    
    fprintf (stderr, "Unmapping driver at 0x%x\n", card->driver);
    munmap (card->driver, fbLcdParams.frameBufferSize);
    fprintf (stderr, "Releasing fbn %d\n", fbn);
    /* release it */
    if (ioctl(fb_d, FB_LCD_FREE, fbn) != 0) {
	printf("FB_LCD_FREE of %d fails!\n", fbn);
    }
    close (fb_d);
    fprintf (stderr, "itsyFini done\n");
}

KdCardFuncs	itsyFuncs = {
    itsyCardInit,	    /* cardinit */
    itsyScreenInit,	    /* scrinit */
    itsyInitScreen,	    /* initScreen */
    itsyPreserve,	    /* preserve */
    itsyEnable,		    /* enable */
    itsyDPMS,		    /* dpms */
    itsyDisable,	    /* disable */
    itsyRestore,	    /* restore */
    itsyScreenFini,	    /* scrfini */
    itsyCardFini,	    /* cardfini */
    
    0,			    /* initCursor */
    0,			    /* enableCursor */
    0,			    /* disableCursor */
    0,			    /* finiCursor */
    0,			    /* recolorCursor */
    
    0,			    /* initAccel */
    0,			    /* enableAccel */
    0,			    /* disableAccel */
    0,			    /* finiAccel */
    
    0,			    /* getColors */
    0,			    /* putColors */
};

void
InitCard (void)
{
    KdCardAttr	attr;
    
    KdCardInfoAdd (&itsyFuncs, &attr, 0);
}

void
InitOutput (ScreenInfo *pScreenInfo, int argc, char **argv)
{
    KdInitOutput (pScreenInfo, argc, argv);
}

void
InitInput (int argc, char **argv)
{
    KdInitInput (&itsyTsMouseFuncs, &itsyKeyboardFuncs);
}

int	itsySessionFd = -1;

int
ItsyOsInit (void)
{
    pid_t		sid;
    int			i;
    itsy_session_info	info;

    if (itsySessionFd < 0)
    {
	itsySessionFd = open ("/dev/session", 0);
	ErrorF("itsySessionFD %d\n", itsySessionFd);
    }
    
    (void) setsid ();
    sid = getsid (0);
    ErrorF ("Session ID %d PID %d\n", sid, getpid ());
    info.sid = sid;
    strcpy (info.name, "X");
    if (itsySessionFd >= 0)
    {
	i = ioctl (itsySessionFd, SESSION_SET_INFO, &info);
	if (i < 0)
	    perror ("SESSION_SET_INFO");
    }
    return 1;
}

void
ItsyOsEnable (void)
{
    itsy_session_request    req;
    int			    i;
    
#define MANAGER_SID_TO_FOREGROUND	2
    
    req.operation = MANAGER_SID_TO_FOREGROUND;
    req.data = 0;
    if (itsySessionFd >= 0)
    {
	i = ioctl (itsySessionFd, SESSION_MANAGER_REQUEST, &req);
	if (i < 0)
	    perror ("SESSION_MANAGER_REQUEST");
    }
}

Bool
ItsyOsSpecialKey (KeySym sym)
{
    return FALSE;
}

void
ItsyOsDisable (void)
{
}

void
ItsyOsFini (void)
{
}

KdOsFuncs   ItsyOsFuncs = {
    ItsyOsInit,
    ItsyOsEnable,
    ItsyOsSpecialKey,
    ItsyOsDisable,
    ItsyOsFini,
    0
};
    
void
OsVendorInit (void)
{
    KdOsInit (&ItsyOsFuncs);
}

void
ddxUseMsg (void)
{
    KdUseMsg();
}

int
ddxProcessArgument (int argc, char **argv, int i)
{
    return KdProcessArgument (argc, argv, i);
}
