/*
 * Copyright © 2001 Keith Packard
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
/* $XFree86: xc/programs/Xserver/hw/kdrive/mach64/mach64.c,v 1.5 2001/06/23 03:41:24 keithp Exp $ */

#include "mach64.h"
#include <sys/io.h>

Bool
mach64CardInit (KdCardInfo *card)
{
    Mach64CardInfo	*mach64c;

    mach64c = (Mach64CardInfo *) xalloc (sizeof (Mach64CardInfo));
    if (!mach64c)
	return FALSE;
    
    mach64c->reg_base = (CARD8 *) KdMapDevice (MACH64_REG_BASE(card),
					       MACH64_REG_SIZE(card));
    
    if (mach64c->reg_base)
    {
	KdSetMappedMode (MACH64_REG_BASE(card),
			 MACH64_REG_SIZE(card),
			 KD_MAPPED_MODE_REGISTERS);
    }
    mach64c->reg = (Reg *) (mach64c->reg_base + MACH64_REG_OFF(card));
    mach64c->media_reg = (MediaReg *) (mach64c->reg_base + MACH64_MEDIA_REG_OFF(card));
    mach64c->lcdEnabled = FALSE;
    
    if (!vesaInitialize (card, &mach64c->vesa))
    {
	xfree (mach64c);
	return FALSE;
    }

    card->driver = mach64c;
    
    return TRUE;
}

Bool
mach64ScreenInit (KdScreenInfo *screen)
{
    Mach64CardInfo	*mach64c = screen->card->driver;
    Mach64ScreenInfo	*mach64s;
    int			screen_size, memory;

    mach64s = (Mach64ScreenInfo *) xalloc (sizeof (Mach64ScreenInfo));
    if (!mach64s)
	return FALSE;
    memset (mach64s, '\0', sizeof (Mach64ScreenInfo));
    if (!vesaScreenInitialize (screen, &mach64s->vesa))
    {
	xfree (mach64s);
	return FALSE;
    }
    if (!mach64c->reg)
	screen->dumb = TRUE;
    if (mach64s->vesa.mapping != VESA_LINEAR)
	screen->dumb = TRUE;
    mach64s->screen = mach64s->vesa.fb;
    switch (screen->fb[0].depth) {
    case 8:
	mach64s->colorKey = 0xff;
	break;
    case 15:
    case 16:
	mach64s->colorKey = 0x001e;
	break;
    case 24:
	mach64s->colorKey = 0x0000fe;
	break;
    default:
	mach64s->colorKey = 1;
	break;
    }
    memory = mach64s->vesa.fb_size;
    screen_size = screen->fb[0].byteStride * screen->height;
    if (mach64s->screen && memory >= screen_size + 2048)
    {
	memory -= 2048;
	mach64s->cursor_base = mach64s->screen + memory - 2048;
    }
    else
	mach64s->cursor_base = 0;
    screen->softCursor = TRUE;	/* XXX for now */
    memory -= screen_size;
    if (memory > screen->fb[0].byteStride)
    {
	mach64s->off_screen = mach64s->screen + screen_size;
	mach64s->off_screen_size = memory;
    }
    else
    {
	mach64s->off_screen = 0;
	mach64s->off_screen_size = 0;
    }
    screen->driver = mach64s;
    return TRUE;
}

Bool
mach64InitScreen (ScreenPtr pScreen)
{
#ifdef XV
    KdScreenPriv(pScreen);
    Mach64CardInfo	*mach64c = pScreenPriv->screen->card->driver;
    if (mach64c->media_reg && mach64c->reg)
	mach64InitVideo(pScreen);
#endif
    return vesaInitScreen (pScreen);
}

#ifdef RANDR
mach64RandRSetConfig (ScreenPtr		pScreen,
		      Rotation		rotation,
		      RRScreenSizePtr	pSize,
		      RRVisualGroupPtr	pVisualGroup)
{
    KdScreenPriv(pScreen);
    
    KdCheckSync (pScreen);

    if (!vesaRandRSetConfig (pScreen, rotation, pSize, pVisualGroup))
	return FALSE;
    
    return TRUE;
}

void
mach64RandRInit (ScreenPtr pScreen)
{
    rrScrPriv(pScreen);

    pScrPriv->rrSetConfig = mach64RandRSetConfig;
}
#endif

Bool
mach64FinishInitScreen (ScreenPtr pScreen)
{
    Bool    ret;
    ret = vesaFinishInitScreen (pScreen);
#ifdef RANDR
    mach64RandRInit (pScreen);
#endif
    return ret;
}

CARD32
mach64ReadLCD (Reg *reg, int id)
{
    CARD32  LCD_INDEX;

    LCD_INDEX = reg->LCD_INDEX & ~(0x3f);
    reg->LCD_INDEX = (LCD_INDEX | id);
    return reg->LCD_DATA;
}

void
mach64WriteLCD (Reg *reg, int id, CARD32 data)
{
    CARD32  LCD_INDEX;

    LCD_INDEX = reg->LCD_INDEX & ~(0x3f);
    reg->LCD_INDEX = (LCD_INDEX | id);
    reg->LCD_DATA = data;
}

void
mach64Preserve (KdCardInfo *card)
{
    Mach64CardInfo	*mach64c = card->driver;
    Reg			*reg = mach64c->reg;

    vesaPreserve(card);
    if (reg)
    {
	mach64c->save.LCD_GEN_CTRL = mach64ReadLCD (reg, 1);
    }
}

void
mach64SetMMIO (Mach64CardInfo *mach64c)
{
    if (mach64c->reg->GUI_STAT == 0xffffffff)
	FatalError ("Mach64 REG not visible\n");
}

void
mach64ResetMMIO (Mach64CardInfo *mach64c)
{
}

Bool
mach64Enable (ScreenPtr pScreen)
{
    KdScreenPriv(pScreen);
    Mach64CardInfo	*mach64c = pScreenPriv->card->driver;

    if (!vesaEnable (pScreen))
	return FALSE;
    
    mach64SetMMIO (mach64c);
    mach64DPMS (pScreen, KD_DPMS_NORMAL);
#ifdef XV
    KdXVEnable (pScreen);
#endif
    return TRUE;
}

void
mach64Disable (ScreenPtr pScreen)
{
#ifdef XV
    KdXVDisable (pScreen);
#endif
    vesaDisable (pScreen);
}

const CARD8	mach64DPMSModes[4] = {
    0x80,	    /* KD_DPMS_NORMAL */
    0x8c,	    /* KD_DPMS_STANDBY */
    0x8c,	    /* KD_DPMS_STANDBY */
    0x8c,	    /* KD_DPMS_STANDBY */
/*    0xb0,	    /* KD_DPMS_SUSPEND */
/*    0xbc,	    /* KD_DPMS_POWERDOWN */
};

#define PWR_MGT_ON		    (1 << 0)
#define PWR_MGT_MODE		    (3 << 1)
#define  PWR_MGT_MODE_PIN	    (0 << 1)
#define  PWR_MGT_MODE_REG	    (1 << 1)
#define  PWR_MGT_MODE_TIMER	    (2 << 1)
#define  PWR_MGR_MODE_PCI	    (3 << 1)
#define AUTO_PWRUP_EN		    (1 << 3)
#define ACTIVITY_PIN_ON		    (1 << 4)
#define STANDBY_POL		    (1 << 5)
#define SUSPEND_POL		    (1 << 6)
#define SELF_REFRESH		    (1 << 7)
#define ACTIVITY_PIN_EN		    (1 << 8)
#define KEYBD_SNOOP		    (1 << 9)
#define DONT_USE_F32KHZ		    (1 << 10)
#define TRISTATE_MEM_EN		    (1 << 11)
#define LCDENG_TEST_MODE	    (0xf << 12)
#define STANDBY_COUNT		    (0xf << 16)
#define SUSPEND_COUNT		    (0xf << 20)
#define BIASON			    (1 << 24)
#define BLON			    (1 << 25)
#define DIGON			    (1 << 26)
#define PM_D3_RST_ENB		    (1 << 27)
#define STANDBY_NOW		    (1 << 28)
#define SUSPEND_NOW		    (1 << 29)
#define PWR_MGT_STATUS		    (3 << 30)
#define  PWR_MGT_STATUS_ON	    (0 << 30)
#define  PWR_MGT_STATUS_STANDBY	    (1 << 30)
#define  PWR_MGT_STATUS_SUSPEND	    (2 << 30)
#define  PWR_MGT_STATUS_TRANSITION  (3 << 30)

Bool
mach64DPMS (ScreenPtr pScreen, int mode)
{
    KdScreenPriv(pScreen);
    Mach64CardInfo	*mach64c = pScreenPriv->card->driver;
    int			hsync_off, vsync_off, blank;
    CARD32		CRTC_GEN_CNTL;
    CARD32		LCD_GEN_CTRL;
    Reg			*reg = mach64c->reg;

    if (!reg)
	return FALSE;
    
    CRTC_GEN_CNTL = reg->CRTC_GEN_CNTL;
    LCD_GEN_CTRL = mach64ReadLCD (reg, 1);

    switch (mode) {
    case KD_DPMS_NORMAL:
	hsync_off = 0;
	vsync_off = 0;
	blank = 0;
	break;
    case KD_DPMS_STANDBY:
	hsync_off = 1;
	vsync_off = 0;
	blank = 1;
	break;
    case KD_DPMS_SUSPEND:
	hsync_off = 0;
	vsync_off = 1;
	blank = 1;
	break;
    case KD_DPMS_POWERDOWN:
	hsync_off = 1;
	vsync_off = 1;
	blank = 1;
    }
    
    if (hsync_off)
	CRTC_GEN_CNTL |= (1 << 2);
    else
	CRTC_GEN_CNTL &= ~(1 << 2);
    if (vsync_off)
	CRTC_GEN_CNTL |= (1 << 3);
    else
	CRTC_GEN_CNTL &= ~(1 << 3);
    if (blank)
    {
	mach64c->lcdEnabled = (LCD_GEN_CTRL & (1 << 1)) != 0;
	LCD_GEN_CTRL &= ~(1 << 1);
	CRTC_GEN_CNTL |= (1 << 6);
	
    }
    else
    {
	if (!(LCD_GEN_CTRL & 3) || mach64c->lcdEnabled)
	    LCD_GEN_CTRL |= (1 << 1);
	CRTC_GEN_CNTL &= ~(1 << 6);
    }
    
    KdCheckSync (pScreen);

    mach64WriteLCD (reg, 1, LCD_GEN_CTRL);
    
    reg->CRTC_GEN_CNTL = CRTC_GEN_CNTL;
    return TRUE;
}

void
mach64Restore (KdCardInfo *card)
{
    Mach64CardInfo	*mach64c = card->driver;
    Reg			*reg = mach64c->reg;

    if (reg)
    {
	mach64WriteLCD (reg, 1, mach64c->save.LCD_GEN_CTRL);
    }
    mach64ResetMMIO (mach64c);
    vesaRestore (card);
}

void
mach64ScreenFini (KdScreenInfo *screen)
{
    Mach64ScreenInfo	*mach64s = (Mach64ScreenInfo *) screen->driver;

    vesaScreenFini (screen);
    xfree (mach64s);
    screen->driver = 0;
}

void
mach64CardFini (KdCardInfo *card)
{
    Mach64CardInfo	*mach64c = card->driver;

    if (mach64c->reg_base)
    {
	KdUnmapDevice ((void *) mach64c->reg_base, MACH64_REG_SIZE(card));
	KdResetMappedMode (MACH64_REG_BASE(card),
			   MACH64_REG_SIZE(card),
			   KD_MAPPED_MODE_REGISTERS);
    }
    vesaCardFini (card);
}

#define mach64CursorInit 0       /* initCursor */
#define mach64CursorEnable 0    /* enableCursor */
#define mach64CursorDisable 0   /* disableCursor */
#define mach64CursorFini 0       /* finiCursor */
#define mach64RecolorCursor 0   /* recolorCursor */

KdCardFuncs	mach64Funcs = {
    mach64CardInit,	    /* cardinit */
    mach64ScreenInit,	    /* scrinit */
    mach64InitScreen,	    /* initScreen */
    mach64Preserve,	    /* preserve */
    mach64Enable,	    /* enable */
    mach64DPMS,		    /* dpms */
    mach64Disable,	    /* disable */
    mach64Restore,	    /* restore */
    mach64ScreenFini,	    /* scrfini */
    mach64CardFini,	    /* cardfini */
    
    mach64CursorInit,	    /* initCursor */
    mach64CursorEnable,	    /* enableCursor */
    mach64CursorDisable,    /* disableCursor */
    mach64CursorFini,	    /* finiCursor */
    mach64RecolorCursor,    /* recolorCursor */
    
    mach64DrawInit,	    /* initAccel */
    mach64DrawEnable,	    /* enableAccel */
    mach64DrawSync,	    /* syncAccel */
    mach64DrawDisable,	    /* disableAccel */
    mach64DrawFini,	    /* finiAccel */
    
    vesaGetColors,    	    /* getColors */
    vesaPutColors,	    /* putColors */

    mach64FinishInitScreen, /* finishInitScreen */
};
