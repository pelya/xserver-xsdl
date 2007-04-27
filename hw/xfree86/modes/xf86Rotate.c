/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#else
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#endif

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "xf86.h"
#include "xf86DDC.h"
#include "fb.h"
#include "windowstr.h"
#include "xf86Crtc.h"
#include "xf86Modes.h"
#include "xf86RandR12.h"
#include "X11/extensions/render.h"
#define DPMS_SERVER
#include "X11/extensions/dpms.h"
#include "X11/Xatom.h"

/* borrowed from composite extension, move to Render and publish? */

static VisualPtr
compGetWindowVisual (WindowPtr pWin)
{
    ScreenPtr	    pScreen = pWin->drawable.pScreen;
    VisualID	    vid = wVisual (pWin);
    int		    i;

    for (i = 0; i < pScreen->numVisuals; i++)
	if (pScreen->visuals[i].vid == vid)
	    return &pScreen->visuals[i];
    return 0;
}

static PictFormatPtr
compWindowFormat (WindowPtr pWin)
{
    ScreenPtr	pScreen = pWin->drawable.pScreen;
    
    return PictureMatchVisual (pScreen, pWin->drawable.depth,
			       compGetWindowVisual (pWin));
}

static void
xf86TranslateBox (BoxPtr b, int dx, int dy)
{
    b->x1 += dx;
    b->y1 += dy;
    b->x2 += dx;
    b->y2 += dy;
}

static void
xf86TransformBox (BoxPtr dst, BoxPtr src, Rotation rotation,
		  int xoff, int yoff,
		  int dest_width, int dest_height)
{
    BoxRec  stmp = *src;
    
    xf86TranslateBox (&stmp, -xoff, -yoff);
    switch (rotation & 0xf) {
    default:
    case RR_Rotate_0:
	*dst = stmp;
	break;
    case RR_Rotate_90:
	dst->x1 = stmp.y1;
	dst->y1 = dest_height - stmp.x2;
	dst->x2 = stmp.y2;
	dst->y2 = dest_height - stmp.x1;
	break;
    case RR_Rotate_180:
	dst->x1 = dest_width - stmp.x2;
	dst->y1 = dest_height - stmp.y2;
	dst->x2 = dest_width - stmp.x1;
	dst->y2 = dest_height - stmp.y1;
	break;
    case RR_Rotate_270:
	dst->x1 = dest_width - stmp.y2;
	dst->y1 = stmp.x1;
	dst->y2 = stmp.x2;
	dst->x2 = dest_width - stmp.y1;
	break;
    }
    if (rotation & RR_Reflect_X) {
	int x1 = dst->x1;
	dst->x1 = dest_width - dst->x2;
	dst->x2 = dest_width - x1;
    }
    if (rotation & RR_Reflect_Y) {
	int y1 = dst->y1;
	dst->y1 = dest_height - dst->y2;
	dst->y2 = dest_height - y1;
    }
}

static void
xf86RotateCrtcRedisplay (xf86CrtcPtr crtc, RegionPtr region)
{
    ScrnInfoPtr		scrn = crtc->scrn;
    ScreenPtr		screen = scrn->pScreen;
    WindowPtr		root = WindowTable[screen->myNum];
    PixmapPtr		dst_pixmap = crtc->rotatedPixmap;
    PictFormatPtr	format = compWindowFormat (WindowTable[screen->myNum]);
    int			error;
    PicturePtr		src, dst;
    PictTransform	transform;
    int			n = REGION_NUM_RECTS(region);
    BoxPtr		b = REGION_RECTS(region);
    XID			include_inferiors = IncludeInferiors;
    
    src = CreatePicture (None,
			 &root->drawable,
			 format,
			 CPSubwindowMode,
			 &include_inferiors,
			 serverClient,
			 &error);
    if (!src)
	return;

    dst = CreatePicture (None,
			 &dst_pixmap->drawable,
			 format,
			 0L,
			 NULL,
			 serverClient,
			 &error);
    if (!dst)
	return;

    memset (&transform, '\0', sizeof (transform));
    transform.matrix[2][2] = IntToxFixed(1);
    transform.matrix[0][2] = IntToxFixed(crtc->x);
    transform.matrix[1][2] = IntToxFixed(crtc->y);
    switch (crtc->rotation & 0xf) {
    default:
    case RR_Rotate_0:
	transform.matrix[0][0] = IntToxFixed(1);
	transform.matrix[1][1] = IntToxFixed(1);
	break;
    case RR_Rotate_90:
	transform.matrix[0][1] = IntToxFixed(-1);
	transform.matrix[1][0] = IntToxFixed(1);
	transform.matrix[0][2] += IntToxFixed(crtc->mode.VDisplay);
	break;
    case RR_Rotate_180:
	transform.matrix[0][0] = IntToxFixed(-1);
	transform.matrix[1][1] = IntToxFixed(-1);
	transform.matrix[0][2] += IntToxFixed(crtc->mode.HDisplay);
	transform.matrix[1][2] += IntToxFixed(crtc->mode.VDisplay);
	break;
    case RR_Rotate_270:
	transform.matrix[0][1] = IntToxFixed(1);
	transform.matrix[1][0] = IntToxFixed(-1);
	transform.matrix[1][2] += IntToxFixed(crtc->mode.HDisplay);
	break;
    }

    /* handle reflection */
    if (crtc->rotation & RR_Reflect_X)
    {
	/* XXX figure this out */
    }
    if (crtc->rotation & RR_Reflect_Y)
    {
	/* XXX figure this out too */
    }

    error = SetPictureTransform (src, &transform);
    if (error)
	return;

    while (n--)
    {
	BoxRec	dst_box;

	xf86TransformBox (&dst_box, b, crtc->rotation,
			  crtc->x, crtc->y,
			  crtc->mode.HDisplay, crtc->mode.VDisplay);
	CompositePicture (PictOpSrc,
			  src, NULL, dst,
			  dst_box.x1, dst_box.y1, 0, 0, dst_box.x1, dst_box.y1,
			  dst_box.x2 - dst_box.x1,
			  dst_box.y2 - dst_box.y1);
	b++;
    }
    FreePicture (src, None);
    FreePicture (dst, None);
}

static void
xf86CrtcDamageShadow (xf86CrtcPtr crtc)
{
    ScrnInfoPtr	pScrn = crtc->scrn;
    BoxRec	damage_box;
    RegionRec   damage_region;
    ScreenPtr	pScreen = pScrn->pScreen;

    damage_box.x1 = crtc->x;
    damage_box.x2 = crtc->x + xf86ModeWidth (&crtc->mode, crtc->rotation);
    damage_box.y1 = crtc->y;
    damage_box.y2 = crtc->y + xf86ModeHeight (&crtc->mode, crtc->rotation);
    REGION_INIT (pScreen, &damage_region, &damage_box, 1);
    DamageDamageRegion (&(*pScreen->GetScreenPixmap)(pScreen)->drawable,
			&damage_region);
    REGION_UNINIT (pScreen, &damage_region);
}

static void
xf86RotatePrepare (ScreenPtr pScreen)
{
    ScrnInfoPtr		pScrn = xf86Screens[pScreen->myNum];
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int			c;

    for (c = 0; c < xf86_config->num_crtc; c++)
    {
	xf86CrtcPtr crtc = xf86_config->crtc[c];
	
	if (crtc->rotatedData && !crtc->rotatedPixmap)
	{
	    crtc->rotatedPixmap = crtc->funcs->shadow_create (crtc,
							     crtc->rotatedData,
							     crtc->mode.HDisplay,
							     crtc->mode.VDisplay);
	    if (!xf86_config->rotation_damage_registered)
	    {
		/* Hook damage to screen pixmap */
		DamageRegister (&(*pScreen->GetScreenPixmap)(pScreen)->drawable,
				xf86_config->rotation_damage);
		xf86_config->rotation_damage_registered = TRUE;
	    }
	    
	    xf86CrtcDamageShadow (crtc);
	}
    }
}

static void
xf86RotateRedisplay(ScreenPtr pScreen)
{
    ScrnInfoPtr		pScrn = xf86Screens[pScreen->myNum];
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    DamagePtr		damage = xf86_config->rotation_damage;
    RegionPtr		region;

    if (!damage)
	return;
    xf86RotatePrepare (pScreen);
    region = DamageRegion(damage);
    if (REGION_NOTEMPTY(pScreen, region)) 
    {
	int			c;
	SourceValidateProcPtr	SourceValidate;

	/*
	 * SourceValidate is used by the software cursor code
	 * to pull the cursor off of the screen when reading
	 * bits from the frame buffer. Bypassing this function
	 * leaves the software cursor in place
	 */
	SourceValidate = pScreen->SourceValidate;
	pScreen->SourceValidate = NULL;

	for (c = 0; c < xf86_config->num_crtc; c++)
	{
	    xf86CrtcPtr	    crtc = xf86_config->crtc[c];

	    if (crtc->rotation != RR_Rotate_0 && crtc->enabled)
	    {
		BoxRec	    box;
		RegionRec   crtc_damage;

		/* compute portion of damage that overlaps crtc */
		box.x1 = crtc->x;
		box.x2 = crtc->x + xf86ModeWidth (&crtc->mode, crtc->rotation);
		box.y1 = crtc->y;
		box.y2 = crtc->y + xf86ModeHeight (&crtc->mode, crtc->rotation);
		REGION_INIT(pScreen, &crtc_damage, &box, 1);
		REGION_INTERSECT (pScreen, &crtc_damage, &crtc_damage, region);
		
		/* update damaged region */
		if (REGION_NOTEMPTY(pScreen, &crtc_damage))
    		    xf86RotateCrtcRedisplay (crtc, &crtc_damage);
		
		REGION_UNINIT (pScreen, &crtc_damage);
	    }
	}
	pScreen->SourceValidate = SourceValidate;
	DamageEmpty(damage);
    }
}

static void
xf86RotateBlockHandler(pointer data, OSTimePtr pTimeout, pointer pRead)
{
    ScreenPtr pScreen = (ScreenPtr) data;

    xf86RotateRedisplay(pScreen);
}

static void
xf86RotateWakeupHandler(pointer data, int i, pointer LastSelectMask)
{
}

static void
xf86RotateDestroy (xf86CrtcPtr crtc)
{
    ScrnInfoPtr		pScrn = crtc->scrn;
    ScreenPtr		pScreen = pScrn->pScreen;
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int			c;
    
    /* Free memory from rotation */
    if (crtc->rotatedPixmap || crtc->rotatedData)
    {
	crtc->funcs->shadow_destroy (crtc, crtc->rotatedPixmap, crtc->rotatedData);
	crtc->rotatedPixmap = NULL;
	crtc->rotatedData = NULL;
    }

    for (c = 0; c < xf86_config->num_crtc; c++)
	if (xf86_config->crtc[c]->rotatedPixmap ||
	    xf86_config->crtc[c]->rotatedData)
	    return;

    /*
     * Clean up damage structures when no crtcs are rotated
     */
    if (xf86_config->rotation_damage)
    {
	/* Free damage structure */
	if (xf86_config->rotation_damage_registered)
	{
	    DamageUnregister (&(*pScreen->GetScreenPixmap)(pScreen)->drawable,
			      xf86_config->rotation_damage);
	    xf86_config->rotation_damage_registered = FALSE;
	}
	DamageDestroy (xf86_config->rotation_damage);
	xf86_config->rotation_damage = NULL;
	/* Free block/wakeup handler */
	RemoveBlockAndWakeupHandlers (xf86RotateBlockHandler,
				      xf86RotateWakeupHandler,
				      (pointer) pScreen);
    }
}

void
xf86RotateCloseScreen (ScreenPtr screen)
{
    ScrnInfoPtr		scrn = xf86Screens[screen->myNum];
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int			c;

    for (c = 0; c < xf86_config->num_crtc; c++)
	xf86RotateDestroy (xf86_config->crtc[c]);
}

Bool
xf86CrtcRotate (xf86CrtcPtr crtc, DisplayModePtr mode, Rotation rotation)
{
    ScrnInfoPtr		pScrn = crtc->scrn;
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    ScreenPtr		pScreen = pScrn->pScreen;
    
    if (rotation == RR_Rotate_0)
    {
	xf86RotateDestroy (crtc);
    }
    else
    {
	/* 
	 * these are the size of the shadow pixmap, which
	 * matches the mode, not the pre-rotated copy in the
	 * frame buffer
	 */
	int	    width = mode->HDisplay;
	int	    height = mode->VDisplay;
	void	    *shadowData = crtc->rotatedData;
	PixmapPtr   shadow = crtc->rotatedPixmap;
	int	    old_width = shadow ? shadow->drawable.width : 0;
	int	    old_height = shadow ? shadow->drawable.height : 0;
	
	/* Allocate memory for rotation */
	if (old_width != width || old_height != height)
	{
	    if (shadow || shadowData)
	    {
		crtc->funcs->shadow_destroy (crtc, shadow, shadowData);
		crtc->rotatedPixmap = NULL;
		crtc->rotatedData = NULL;
	    }
	    shadowData = crtc->funcs->shadow_allocate (crtc, width, height);
	    if (!shadowData)
		goto bail1;
	    crtc->rotatedData = shadowData;
	    /* shadow will be damaged in xf86RotatePrepare */
	}
	else
	{
	    /* mark shadowed area as damaged so it will be repainted */
	    xf86CrtcDamageShadow (crtc);
	}
	
	if (!xf86_config->rotation_damage)
	{
	    /* Create damage structure */
	    xf86_config->rotation_damage = DamageCreate (NULL, NULL,
						DamageReportNone,
						TRUE, pScreen, pScreen);
	    if (!xf86_config->rotation_damage)
		goto bail2;
	    
	    /* Assign block/wakeup handler */
	    if (!RegisterBlockAndWakeupHandlers (xf86RotateBlockHandler,
						 xf86RotateWakeupHandler,
						 (pointer) pScreen))
	    {
		goto bail3;
	    }
	}
	if (0)
	{
bail3:
	    DamageDestroy (xf86_config->rotation_damage);
	    xf86_config->rotation_damage = NULL;
	    
bail2:
	    if (shadow || shadowData)
	    {
		crtc->funcs->shadow_destroy (crtc, shadow, shadowData);
		crtc->rotatedPixmap = NULL;
		crtc->rotatedData = NULL;
	    }
bail1:
	    if (old_width && old_height)
		crtc->rotatedPixmap = crtc->funcs->shadow_create (crtc,
								  NULL,
								  old_width,
								  old_height);
	    return FALSE;
	}
    }
    
    /* All done */
    return TRUE;
}
