/*
 * Xephyr - A kdrive X server thats runs in a host X window.
 *          Authored by Matthew Allum <mallum@openedhand.com>
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
 *  o Support multiple screens, shouldn't be hard just alot of rejigging.
 */

#ifdef HAVE_CONFIG_H
#include <kdrive-config.h>
#endif
#include "ephyr.h"

#include "inputstr.h"

extern int KdTsPhyScreen;
extern DeviceIntPtr pKdKeyboard;

static int mouseState = 0;

Bool   EphyrWantGrayScale = 0;

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
  int width = 640, height = 480; 

  if (hostx_want_screen_size(&width, &height) 
      || !screen->width || !screen->height)
    {
      screen->width = width;
      screen->height = height;
    }

  if (EphyrWantGrayScale)
    screen->fb[0].depth = 8;

  if (screen->fb[0].depth && screen->fb[0].depth != hostx_get_depth())
    {
      if (screen->fb[0].depth < hostx_get_depth()
	  && (screen->fb[0].depth == 24 || screen->fb[0].depth == 16
	      || screen->fb[0].depth == 8))
	{
	  hostx_set_server_depth(screen->fb[0].depth);
	}
      else 
	ErrorF("\nXephyr: requested screen depth not supported, setting to match hosts.\n");
    }
  
  screen->fb[0].depth = hostx_get_server_depth();
  screen->rate = 72;
  
  if (screen->fb[0].depth <= 8)
    {
      if (EphyrWantGrayScale)
	screen->fb[0].visuals = ((1 << StaticGray) | (1 << GrayScale));
      else
	screen->fb[0].visuals = ((1 << StaticGray) |
				 (1 << GrayScale) |
				 (1 << StaticColor) |
				 (1 << PseudoColor) |
				 (1 << TrueColor) |
				 (1 << DirectColor));
      
      screen->fb[0].redMask   = 0x00;
      screen->fb[0].greenMask = 0x00;
      screen->fb[0].blueMask  = 0x00;
      screen->fb[0].depth        = 8;
      screen->fb[0].bitsPerPixel = 8;
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
  int buffer_height;
  
  EPHYR_DBG(" screen->width: %d, screen->height: %d",
	    screen->width, screen->height);
  
  KdComputeMouseMatrix (&m, scrpriv->randr, screen->width, screen->height);
    
  KdSetMouseMatrix (&m);
  
  priv->bytes_per_line = ((screen->width * screen->fb[0].bitsPerPixel + 31) >> 5) << 2;
  
  /* point the framebuffer to the data in an XImage */
  /* If fakexa is enabled, allocate a larger buffer so that fakexa has space to
   * put offscreen pixmaps.
   */
  if (ephyrFuncs.initAccel == NULL)
    buffer_height = screen->height;
  else
    buffer_height = 3 * screen->height;
  
  priv->base = hostx_screen_init (screen->width, screen->height, buffer_height);

  screen->memory_base  = (CARD8 *) (priv->base);
  screen->memory_size  = priv->bytes_per_line * buffer_height;
  screen->off_screen_base = priv->bytes_per_line * screen->height;
  
  if ((scrpriv->randr & RR_Rotate_0) && !(scrpriv->randr & RR_Reflect_All))
    {
      scrpriv->shadow = FALSE;
      
      screen->fb[0].byteStride = priv->bytes_per_line;
      screen->fb[0].pixelStride = screen->width;
      screen->fb[0].frameBuffer = (CARD8 *) (priv->base);
    }
  else
    {
      /* Rotated/Reflected so we need to use shadow fb */
      scrpriv->shadow = TRUE;
      
      EPHYR_DBG("allocing shadow");
      
      KdShadowFbAlloc (screen, 0, 
		       scrpriv->randr & (RR_Rotate_90|RR_Rotate_270));
    }
  
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
  EphyrScrPriv  *scrpriv = screen->driver;
  
  if (scrpriv->shadow)
    KdShadowFbFree (screen, 0);
  
  /* Note, priv->base will get freed when XImage recreated */
  
  return TRUE;
}

void 
ephyrShadowUpdate (ScreenPtr pScreen, shadowBufPtr pBuf)
{
  KdScreenPriv(pScreen);
  KdScreenInfo *screen = pScreenPriv->screen;
  
  EPHYR_DBG("slow paint");
  
  /* FIXME: Slow Rotated/Reflected updates could be much
   * much faster efficiently updating via tranforming 
   * pBuf->pDamage  regions     
  */
  shadowUpdateRotatePacked(pScreen, pBuf);
  hostx_paint_rect(0,0,0,0, screen->width, screen->height);
}

static void
ephyrInternalDamageRedisplay (ScreenPtr pScreen)
{
  KdScreenPriv(pScreen);
  KdScreenInfo	*screen = pScreenPriv->screen;
  EphyrScrPriv	*scrpriv = screen->driver;
  RegionPtr	 pRegion;
  
  if (!scrpriv || !scrpriv->pDamage)
    return;
  
  pRegion = DamageRegion (scrpriv->pDamage);
  
  if (REGION_NOTEMPTY (pScreen, pRegion))
    {
      int           nbox;
      BoxPtr        pbox;
      
      nbox = REGION_NUM_RECTS (pRegion);
      pbox = REGION_RECTS (pRegion);
      
      while (nbox--)
	{
	  hostx_paint_rect(pbox->x1, pbox->y1,
			   pbox->x1, pbox->y1,
			   pbox->x2 - pbox->x1,
			   pbox->y2 - pbox->y1);
	  pbox++;
	}
      
      DamageEmpty (scrpriv->pDamage);
    }
}

static void
ephyrInternalDamageBlockHandler (pointer   data,
				 OSTimePtr pTimeout,
				 pointer   pRead)
{
  ScreenPtr pScreen = (ScreenPtr) data;
  
  ephyrInternalDamageRedisplay (pScreen);
}

static void
ephyrInternalDamageWakeupHandler (pointer data, int i, pointer LastSelectMask)
{
  /* FIXME: Not needed ? */
}

Bool
ephyrSetInternalDamage (ScreenPtr pScreen)
{
  KdScreenPriv(pScreen);
  KdScreenInfo	*screen = pScreenPriv->screen;
  EphyrScrPriv	*scrpriv = screen->driver;
  PixmapPtr      pPixmap = NULL;
  
  scrpriv->pDamage = DamageCreate ((DamageReportFunc) 0,
				   (DamageDestroyFunc) 0,
				   DamageReportNone,
				   TRUE,
				   pScreen,
				   pScreen);
  
  if (!RegisterBlockAndWakeupHandlers (ephyrInternalDamageBlockHandler,
				       ephyrInternalDamageWakeupHandler,
				       (pointer) pScreen))
    return FALSE;
  
  pPixmap = (*pScreen->GetScreenPixmap) (pScreen);
  
  DamageRegister (&pPixmap->drawable, scrpriv->pDamage);
      
  return TRUE;
}

void
ephyrUnsetInternalDamage (ScreenPtr pScreen)
{
  KdScreenPriv(pScreen);
  KdScreenInfo	*screen = pScreenPriv->screen;
  EphyrScrPriv	*scrpriv = screen->driver;
  PixmapPtr      pPixmap = NULL;
  
  pPixmap = (*pScreen->GetScreenPixmap) (pScreen);
  DamageUnregister (&pPixmap->drawable, scrpriv->pDamage);
  
  RemoveBlockAndWakeupHandlers (ephyrInternalDamageBlockHandler,
				ephyrInternalDamageWakeupHandler,
				(pointer) pScreen);
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
      { 1152, 864 },
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
  
  if (!hostx_want_preexisting_window()
      && !hostx_want_fullscreen()) /* only if no -parent switch */
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
  int		oldwidth, oldheight, oldmmwidth, oldmmheight;
  Bool          oldshadow;
  int		newwidth, newheight;
  
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
    
  oldwidth    = screen->width;
  oldheight   = screen->height;
  oldmmwidth  = pScreen->mmWidth;
  oldmmheight = pScreen->mmHeight;
  oldshadow   = scrpriv->shadow;
  
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
  
  /* FIXME below should go in own call */
  
  if (oldshadow)
    KdShadowUnset (screen->pScreen);
  else
    ephyrUnsetInternalDamage(screen->pScreen);
  
  if (scrpriv->shadow)
    {
      if (!KdShadowSet (screen->pScreen, 
			scrpriv->randr, 
			ephyrShadowUpdate, 
			ephyrWindowLinear))
	goto bail4;
    }
  else
    {
      /* Without shadow fb ( non rotated ) we need 
       * to use damage to efficiently update display
       * via signal regions what to copy from 'fb'.
       */
      if (!ephyrSetInternalDamage(screen->pScreen))
	goto bail4;
    }
  
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
  pScreen->CreateColormap = ephyrCreateColormap;
  return TRUE;
}

Bool
ephyrFinishInitScreen (ScreenPtr pScreen)
{
  /* FIXME: Calling this even if not using shadow.  
   * Seems harmless enough. But may be safer elsewhere.
   */
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
  KdScreenPriv(pScreen);
  KdScreenInfo	*screen    = pScreenPriv->screen;
  EphyrScrPriv	*scrpriv   = screen->driver;

  EPHYR_DBG("mark");

  if (scrpriv->shadow) 
    return KdShadowSet (pScreen, 
			scrpriv->randr, 
			ephyrShadowUpdate, 
			ephyrWindowLinear);
  else
    return ephyrSetInternalDamage(pScreen); 
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

/*  
 * Port of Mark McLoughlin's Xnest fix for focus in + modifier bug.
 * See https://bugs.freedesktop.org/show_bug.cgi?id=3030
 */
void
ephyrUpdateModifierState(unsigned int state)
{
  DeviceIntPtr pkeydev;
  KeyClassPtr  keyc;
  int          i;
  CARD8        mask;

  pkeydev = (DeviceIntPtr)LookupKeyboardDevice();

  if (!pkeydev)
    return;
  
  keyc = pkeydev->key;
  
  state = state & 0xff;
  
  if (keyc->state == state)
    return;
  
  for (i = 0, mask = 1; i < 8; i++, mask <<= 1) 
    {
      int key;
      
      /* Modifier is down, but shouldn't be   */
      if ((keyc->state & mask) && !(state & mask)) 
	{
	  int count = keyc->modifierKeyCount[i];
	  
	  for (key = 0; key < MAP_LENGTH; key++)
	    if (keyc->modifierMap[key] & mask) 
	      {
		int bit;
		BYTE *kptr;
		
		kptr = &keyc->down[key >> 3];
		bit = 1 << (key & 7);
		
		if (*kptr & bit)
		  KdEnqueueKeyboardEvent(key, TRUE); /* release */
		
		if (--count == 0)
		  break;
	      }
	}
       
      /* Modifier shoud be down, but isn't   */
      if (!(keyc->state & mask) && (state & mask))
	for (key = 0; key < MAP_LENGTH; key++)
	  if (keyc->modifierMap[key] & mask) 
	    {
	      KdEnqueueKeyboardEvent(key, FALSE); /* press */
	      break;
	    }
    }
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
	  ephyrUpdateModifierState(ev.key_state);
	  mouseState |= ev.data.mouse_down.button_num;
	  KdEnqueueMouseEvent(kdMouseInfo, mouseState|KD_MOUSE_DELTA, 0, 0);
	  break;

	case EPHYR_EV_MOUSE_RELEASE:
	  ephyrUpdateModifierState(ev.key_state);
	  mouseState &= ~ev.data.mouse_up.button_num;
	  KdEnqueueMouseEvent(kdMouseInfo, mouseState|KD_MOUSE_DELTA, 0, 0);
	  break;

	case EPHYR_EV_KEY_PRESS:
	  ephyrUpdateModifierState(ev.key_state);
	  KdEnqueueKeyboardEvent (ev.data.key_down.scancode, FALSE);
	  break;

	case EPHYR_EV_KEY_RELEASE:
	  ephyrUpdateModifierState(ev.key_state);
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
  /* XXX Not sure if this is right */
  
  EPHYR_DBG("mark");
  
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
  int min, max, p;

  /* XXX Not sure if this is right */

  min = 256;
  max = 0;
  
  while (n--)
    {
      p = pdefs->pixel;
      if (p < min)
	min = p;
      if (p > max)
	max = p;

      hostx_set_cmap_entry(p, 		
			   pdefs->red >> 8,
			   pdefs->green >> 8,
			   pdefs->blue >> 8);
      pdefs++;
    }
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
