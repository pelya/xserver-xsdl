/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *          Authored by Matthew Allum <mallum@o-hand.com>
 * 
 * Copyright © 2004 Nokia 
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Nokia not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. Nokia makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * NOKIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL NOKIA BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*  TODO:
 *
 *  POSSIBLES
 *    - much improve keyboard handling *kind of done*
 *    - '-fullscreen' switch ?
 *    - full keyboard grab option somehow ? - use for testing WM key shortcuts 
 *      with out host WM getting them instead.   
 *    - Make cursor 'accel' better. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "ephyr.h"

extern int KdTsPhyScreen;

static int mouseState = 0;

Bool
ephyrInitialize (KdCardInfo *card, EphyrPriv *priv)
{
  OsSignal(SIGUSR1, hostx_handle_signal);

  priv->base = 0;
  priv->bytes_per_line = 0;
  return TRUE;
}

Bool
ephyrCardInit (KdCardInfo *card)
{
    EphyrPriv	*priv;

    priv = (EphyrPriv *) xalloc (sizeof (EphyrPriv));
    if (!priv)
	return FALSE;
    
    if (!ephyrInitialize (card, priv))
    {
	xfree (priv);
	return FALSE;
    }
    card->driver = priv;
    
    return TRUE;
}

Bool
ephyrScreenInitialize (KdScreenInfo *screen, EphyrScrPriv *scrpriv)
{
  int   width = 640, height = 480; 

  if (hostx_want_screen_size(&width, &height) 
      || !screen->width || !screen->height)
    {
      screen->width = width;
      screen->height = height;
    }


  if (screen->fb[0].depth && screen->fb[0].depth != hostx_get_depth())
      ErrorF("\nXephyr screen depth must match hosts, ignoring.\n");

  screen->fb[0].depth = hostx_get_depth();
  screen->rate = 72;

  if (screen->fb[0].depth <= 8)
    {
      screen->fb[0].visuals = ((1 << StaticGray) |
			       (1 << GrayScale) |
			       (1 << StaticColor) |
			       (1 << PseudoColor) |
			       (1 << TrueColor) |
			       (1 << DirectColor));
    }
  else 
    {
      screen->fb[0].visuals = (1 << TrueColor);

      if (screen->fb[0].depth <= 15)
	{
	  screen->fb[0].depth = 15;
	  screen->fb[0].bitsPerPixel = 16;
	  
	  hostx_get_visual_masks (&screen->fb[0].redMask,
				  &screen->fb[0].greenMask,
				  &screen->fb[0].blueMask);
	  
	}
	else if (screen->fb[0].depth <= 16)
	{
	    screen->fb[0].depth = 16;
	    screen->fb[0].bitsPerPixel = 16;

	    hostx_get_visual_masks (&screen->fb[0].redMask,
				 &screen->fb[0].greenMask,
				 &screen->fb[0].blueMask);
	}
	else
	{
	    screen->fb[0].depth = 24;
	    screen->fb[0].bitsPerPixel = 32;

	    hostx_get_visual_masks (&screen->fb[0].redMask,
				    &screen->fb[0].greenMask,
				    &screen->fb[0].blueMask);
	}
    }

    scrpriv->randr = screen->randr;

    return ephyrMapFramebuffer (screen);
}

Bool
ephyrScreenInit (KdScreenInfo *screen)
{
    EphyrScrPriv *scrpriv;

    scrpriv = xalloc (sizeof (EphyrScrPriv));
    if (!scrpriv)
	return FALSE;
    memset (scrpriv, 0, sizeof (EphyrScrPriv));
    screen->driver = scrpriv;
    if (!ephyrScreenInitialize (screen, scrpriv))
    {
	screen->driver = 0;
	xfree (scrpriv);
	return FALSE;
    }
    return TRUE;
}
    
void*
ephyrWindowLinear (ScreenPtr	pScreen,
		   CARD32	row,
		   CARD32	offset,
		   int		mode,
		   CARD32	*size,
		   void		*closure)
{
    KdScreenPriv(pScreen);
    EphyrPriv	    *priv = pScreenPriv->card->driver;

    if (!pScreenPriv->enabled)
      {
	return 0;
      }

    *size = priv->bytes_per_line;
    return priv->base + row * priv->bytes_per_line + offset;
}

Bool
ephyrMapFramebuffer (KdScreenInfo *screen)
{
    EphyrScrPriv  *scrpriv = screen->driver;
    EphyrPriv	  *priv    = screen->card->driver;
    KdMouseMatrix m;

    EPHYR_DBG(" screen->width: %d, screen->height: %d",
	      screen->width, screen->height);

    /* Always use shadow so we get damage notifications */
    scrpriv->shadow = TRUE;
    
    KdComputeMouseMatrix (&m, scrpriv->randr, screen->width, screen->height);
    
    KdSetMouseMatrix (&m);

    priv->bytes_per_line = ((screen->width * screen->fb[0].bitsPerPixel + 31) >> 5) << 2;

    /* point the framebuffer to the data in an XImage */
    priv->base = hostx_screen_init (screen->width, screen->height);
    
    screen->memory_base  = (CARD8 *) (priv->base);
    screen->memory_size  = 0;
    screen->off_screen_base = 0;

    KdShadowFbAlloc (screen, 0, 
		     scrpriv->randr & (RR_Rotate_90|RR_Rotate_270));
    return TRUE;
}

void
ephyrSetScreenSizes (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    EphyrScrPriv	*scrpriv = screen->driver;

    if (scrpriv->randr & (RR_Rotate_0|RR_Rotate_180))
    {
	pScreen->width = screen->width;
	pScreen->height = screen->height;
	pScreen->mmWidth = screen->width_mm;
	pScreen->mmHeight = screen->height_mm;
    }
    else 
    {
	pScreen->width = screen->height;
	pScreen->height = screen->width;
	pScreen->mmWidth = screen->height_mm;
	pScreen->mmHeight = screen->width_mm;
    }
}

Bool
ephyrUnmapFramebuffer (KdScreenInfo *screen)
{
    KdShadowFbFree (screen, 0);

    /* Note, priv->base will get freed when XImage recreated */

    return TRUE;
}

void 
ephyrShadowUpdate (ScreenPtr pScreen, shadowBufPtr pBuf)
{
  KdScreenPriv(pScreen);
  KdScreenInfo *screen = pScreenPriv->screen;
  EphyrScrPriv *scrpriv = screen->driver;
  int           nbox;
  BoxPtr        pbox;

  RegionPtr damage;

  if (!(scrpriv->randr & RR_Rotate_0) || (scrpriv->randr & RR_Reflect_All))
    {
      /*  Rotated. 
       *  TODO: Fix this to use damage as well so much faster. 
       *        Sledgehammer approach atm. 
       *
       *        Catch reflects here too - though thats wrong ... 
       */
      EPHYR_DBG("slow paint");
      shadowUpdateRotatePacked(pScreen, pBuf);
      hostx_paint_rect(0,0,0,0, screen->width, screen->height);
      return;
    } 
  else shadowUpdatePacked(pScreen, pBuf);

  /* Figure out what rects have changed and update em. */

  if (!pBuf || !pBuf->pDamage)
    return;
  
  damage = DamageRegion (pBuf->pDamage);
  
  if (!REGION_NOTEMPTY (pScreen, damage))
    return;
  
  nbox = REGION_NUM_RECTS (damage);
  pbox = REGION_RECTS (damage);
  
  while (nbox--)
    {
      hostx_paint_rect(pbox->x1, pbox->y1,
		       pbox->x1, pbox->y1,
		       pbox->x2 - pbox->x1,
		       pbox->y2 - pbox->y1);
      pbox++;
    }
}

Bool
ephyrSetShadow (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen = pScreenPriv->screen;
    EphyrScrPriv	*scrpriv = screen->driver;
    ShadowUpdateProc	update;
    ShadowWindowProc	window;

    window = ephyrWindowLinear;
    update = ephyrShadowUpdate; 

    return KdShadowSet (pScreen, scrpriv->randr, update, window);
}

#ifdef RANDR
Bool
ephyrRandRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	    *screen = pScreenPriv->screen;
    EphyrScrPriv	    *scrpriv = screen->driver;
    RRScreenSizePtr	    pSize;
    Rotation		    randr;
    int			    n = 0;

    EPHYR_DBG("mark");

    struct { int width, height; } sizes[] = 
      {
	{ 1600, 1200 },
	{ 1400, 1050 },
	{ 1280, 960  },
	{ 1280, 1024 },
	{ 1152, 768 },
	{ 1024, 768 },
	{ 832, 624 },
	{ 800, 600 },
	{ 720, 400 },
	{ 480, 640 },
	{ 640, 480 },
	{ 640, 400 },
	{ 320, 240 },
	{ 240, 320 },
	{ 160, 160 }, 
	{ 0, 0 }
      };
    
    *rotations = RR_Rotate_All|RR_Reflect_All;
    
    if (!hostx_want_preexisting_window()) /* only if no -parent switch */
      {
	while (sizes[n].width != 0 && sizes[n].height != 0)
	  {
	    RRRegisterSize (pScreen,
			    sizes[n].width,
			    sizes[n].height, 
			    (sizes[n].width * screen->width_mm)/screen->width,
			    (sizes[n].height *screen->height_mm)/screen->height
 			    );
	    n++;
	  }
      }

    pSize = RRRegisterSize (pScreen,
			    screen->width,
			    screen->height, 
			    screen->width_mm,
			    screen->height_mm);
    
    randr = KdSubRotation (scrpriv->randr, screen->randr);
    
    RRSetCurrentConfig (pScreen, randr, 0, pSize);
    
    return TRUE;
}

Bool
ephyrRandRSetConfig (ScreenPtr		pScreen,
		     Rotation		randr,
		     int		rate,
		     RRScreenSizePtr	pSize)
{
    KdScreenPriv(pScreen);
    KdScreenInfo	*screen    = pScreenPriv->screen;
    EphyrScrPriv	*scrpriv   = screen->driver;
    Bool		wasEnabled = pScreenPriv->enabled;
    EphyrScrPriv	oldscr;
    int			oldwidth;
    int			oldheight;
    int			oldmmwidth;
    int			oldmmheight;
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

    if (wasEnabled)
	KdDisableScreen (pScreen);

    oldscr = *scrpriv;
    
    oldwidth = screen->width;
    oldheight = screen->height;
    oldmmwidth = pScreen->mmWidth;
    oldmmheight = pScreen->mmHeight;
    
    /*
     * Set new configuration
     */
    
    scrpriv->randr = KdAddRotation (screen->randr, randr);

    KdOffscreenSwapOut (screen->pScreen);

    ephyrUnmapFramebuffer (screen); 
    
    screen->width  = newwidth;
    screen->height = newheight;

    if (!ephyrMapFramebuffer (screen))
	goto bail4;

    KdShadowUnset (screen->pScreen);

    if (!ephyrSetShadow (screen->pScreen))
	goto bail4;

    ephyrSetScreenSizes (screen->pScreen);

    /*
     * Set frame buffer mapping
     */
    (*pScreen->ModifyPixmapHeader) (fbGetScreenPixmap (pScreen),
				    pScreen->width,
				    pScreen->height,
				    screen->fb[0].depth,
				    screen->fb[0].bitsPerPixel,
				    screen->fb[0].byteStride,
				    screen->fb[0].frameBuffer);
    
    /* set the subpixel order */

    KdSetSubpixelOrder (pScreen, scrpriv->randr);


    if (wasEnabled)
	KdEnableScreen (pScreen);

    return TRUE;

bail4:
    EPHYR_DBG("bailed");

    ephyrUnmapFramebuffer (screen);
    *scrpriv = oldscr;
    (void) ephyrMapFramebuffer (screen);

    pScreen->width = oldwidth;
    pScreen->height = oldheight;
    pScreen->mmWidth = oldmmwidth;
    pScreen->mmHeight = oldmmheight;

    if (wasEnabled)
	KdEnableScreen (pScreen);
    return FALSE;
}

Bool
ephyrRandRInit (ScreenPtr pScreen)
{
    rrScrPrivPtr    pScrPriv;
    
    if (!RRScreenInit (pScreen))
      {
	return FALSE;
      }

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = ephyrRandRGetInfo;
    pScrPriv->rrSetConfig = ephyrRandRSetConfig;
    return TRUE;
}
#endif

Bool
ephyrCreateColormap (ColormapPtr pmap)
{
    return fbInitializeColormap (pmap);
}

Bool
ephyrInitScreen (ScreenPtr pScreen)
{
#ifdef TOUCHSCREEN
    KdTsPhyScreen = pScreen->myNum;
#endif

    pScreen->CreateColormap = ephyrCreateColormap;
    return TRUE;
}

Bool
ephyrFinishInitScreen (ScreenPtr pScreen)
{
  if (!shadowSetup (pScreen))
    return FALSE;
  
#ifdef RANDR
  if (!ephyrRandRInit (pScreen))
    return FALSE;
#endif
    
  return TRUE;
}

Bool
ephyrCreateResources (ScreenPtr pScreen)
{
    return ephyrSetShadow (pScreen);
}

void
ephyrPreserve (KdCardInfo *card)
{
}

Bool
ephyrEnable (ScreenPtr pScreen)
{
    return TRUE;
}

Bool
ephyrDPMS (ScreenPtr pScreen, int mode)
{
    return TRUE;
}

void
ephyrDisable (ScreenPtr pScreen)
{
}

void
ephyrRestore (KdCardInfo *card)
{
}

void
ephyrScreenFini (KdScreenInfo *screen)
{
}

void
ephyrPoll(void)
{
  EphyrHostXEvent ev;

  while (hostx_get_event(&ev))
    {
      switch (ev.type)
	{
	case EPHYR_EV_MOUSE_MOTION:
	  KdEnqueueMouseEvent(kdMouseInfo, mouseState,  
			      ev.data.mouse_motion.x, 
			      ev.data.mouse_motion.y);
	  break;

	case EPHYR_EV_MOUSE_PRESS:

	  mouseState |= ev.data.mouse_down.button_num;
	  KdEnqueueMouseEvent(kdMouseInfo, mouseState|KD_MOUSE_DELTA, 0, 0);
	  break;

	case EPHYR_EV_MOUSE_RELEASE:

	  mouseState &= ~ev.data.mouse_up.button_num;
	  KdEnqueueMouseEvent(kdMouseInfo, mouseState|KD_MOUSE_DELTA, 0, 0);
	  break;

	case EPHYR_EV_KEY_PRESS:

	  KdEnqueueKeyboardEvent (ev.data.key_down.scancode, FALSE);
	  break;

	case EPHYR_EV_KEY_RELEASE:

	  KdEnqueueKeyboardEvent (ev.data.key_up.scancode, TRUE);
	  break;

	default:
	  break;
	}
    }
}

void
ephyrCardFini (KdCardInfo *card)
{
    EphyrPriv	*priv = card->driver;
    xfree (priv);
}

void
ephyrGetColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
    while (n--)
    {
	pdefs->red = 0;
	pdefs->green = 0;
	pdefs->blue = 0;
	pdefs++;
    }
}

void
ephyrPutColors (ScreenPtr pScreen, int fb, int n, xColorItem *pdefs)
{
}

/* Mouse calls */

static Bool
MouseInit (void)
{
    return TRUE;
}

static void
MouseFini (void)
{
  ;
}

KdMouseFuncs EphyrMouseFuncs = {
    MouseInit,
    MouseFini,
};

/* Keyboard */

static void
EphyrKeyboardLoad (void)
{
  EPHYR_DBG("mark");

  hostx_load_keymap();
}

static int
EphyrKeyboardInit (void)
{
  return 0;
}

static void
EphyrKeyboardFini (void)
{
}

static void
EphyrKeyboardLeds (int leds)
{
}

static void
EphyrKeyboardBell (int volume, int frequency, int duration)
{
}

KdKeyboardFuncs	EphyrKeyboardFuncs = {
    EphyrKeyboardLoad,
    EphyrKeyboardInit,
    EphyrKeyboardLeds,
    EphyrKeyboardBell,
    EphyrKeyboardFini,
    0,
};
