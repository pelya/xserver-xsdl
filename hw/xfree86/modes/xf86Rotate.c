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

#define F(x)	IntToxFixed(x)

#define toF(x)	((float) (x) / 65536.0f)

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

    error = SetPictureTransform (src, &crtc->crtc_to_framebuffer);
    if (error)
	return;
    if (crtc->transform_in_use && crtc->filter)
	SetPicturePictFilter (src, crtc->filter,
			      crtc->params, crtc->nparams);

    while (n--)
    {
	BoxRec	dst_box;

	dst_box = *b;
	dst_box.x1 -= crtc->filter_width >> 1;
	dst_box.x2 += crtc->filter_width >> 1;
	dst_box.y1 -= crtc->filter_height >> 1;
	dst_box.y2 += crtc->filter_height >> 1;
	PictureTransformBounds (&dst_box, &crtc->framebuffer_to_crtc);
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

    damage_box.x1 = 0;
    damage_box.x2 = crtc->mode.HDisplay;
    damage_box.y1 = 0;
    damage_box.y2 = crtc->mode.VDisplay;
    PictureTransformBounds (&damage_box, &crtc->crtc_to_framebuffer);
    REGION_INIT (pScreen, &damage_region, &damage_box, 1);
    DamageRegionAppend(&(*pScreen->GetScreenPixmap)(pScreen)->drawable,
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

static Bool
xf86RotateRedisplay(ScreenPtr pScreen)
{
    ScrnInfoPtr		pScrn = xf86Screens[pScreen->myNum];
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    DamagePtr		damage = xf86_config->rotation_damage;
    RegionPtr		region;

    if (!damage)
	return FALSE;
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

	    if (crtc->transform_in_use && crtc->enabled)
	    {
		RegionRec   crtc_damage;

		/* compute portion of damage that overlaps crtc */
		REGION_INIT(pScreen, &crtc_damage, &crtc->bounds, 1);
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
    return TRUE;
}

static void
xf86RotateBlockHandler(int screenNum, pointer blockData,
		       pointer pTimeout, pointer pReadmask)
{
    ScreenPtr		pScreen = screenInfo.screens[screenNum];
    ScrnInfoPtr		pScrn = xf86Screens[screenNum];
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);

    pScreen->BlockHandler = xf86_config->BlockHandler;
    (*pScreen->BlockHandler) (screenNum, blockData, pTimeout, pReadmask);
    if (xf86RotateRedisplay(pScreen))
    {
	/* Re-wrap if rotation is still happening */
	xf86_config->BlockHandler = pScreen->BlockHandler;
	pScreen->BlockHandler = xf86RotateBlockHandler;
    }
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
	if (xf86_config->crtc[c]->transform_in_use)
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
    }
}

_X_EXPORT void
xf86RotateFreeShadow(ScrnInfoPtr pScrn)
{
    xf86CrtcConfigPtr config = XF86_CRTC_CONFIG_PTR(pScrn);
    int c;

   for (c = 0; c < config->num_crtc; c++) {
       xf86CrtcPtr crtc = config->crtc[c];

       if (crtc->rotatedPixmap || crtc->rotatedData) {
	   crtc->funcs->shadow_destroy(crtc, crtc->rotatedPixmap,
				crtc->rotatedData);
	   crtc->rotatedPixmap = NULL;
	   crtc->rotatedData = NULL;
       }
   }
}

_X_EXPORT void
xf86RotateCloseScreen (ScreenPtr screen)
{
    ScrnInfoPtr		scrn = xf86Screens[screen->myNum];
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int			c;

    for (c = 0; c < xf86_config->num_crtc; c++)
	xf86RotateDestroy (xf86_config->crtc[c]);
}

_X_EXPORT Bool
xf86CrtcRotate (xf86CrtcPtr crtc, DisplayModePtr mode, Rotation rotation)
{
    ScrnInfoPtr		pScrn = crtc->scrn;
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    /* if this is called during ScreenInit() we don't have pScrn->pScreen yet */
    ScreenPtr		pScreen = screenInfo.screens[pScrn->scrnIndex];
    PictTransform	crtc_to_fb, fb_to_crtc;
    xFixed		*new_params = NULL;
    int			new_nparams = 0;
    PictFilterPtr	new_filter = NULL;
    int			new_width = 0;
    int			new_height = 0;
    
    PictureTransformInitIdentity (&crtc_to_fb);
    PictureTransformInitIdentity (&fb_to_crtc);
    if (rotation != RR_Rotate_0)
    {
	xFixed	rot_cos, rot_sin, rot_dx, rot_dy;
	xFixed	scale_x, scale_y, scale_dx, scale_dy;
	int	mode_w = crtc->mode.HDisplay;
	int	mode_h = crtc->mode.VDisplay;
	
	/* rotation */
	switch (rotation & 0xf) {
	default:
	case RR_Rotate_0:
	    rot_cos = F ( 1);	    rot_sin = F ( 0);
	    rot_dx  = F ( 0);	    rot_dy  = F ( 0);
	    break;
	case RR_Rotate_90:
	    rot_cos = F ( 0);	    rot_sin = F ( 1);
	    rot_dx =  F ( mode_h);  rot_dy  = F (0);
	    break;
	case RR_Rotate_180:
	    rot_cos = F (-1);	    rot_sin = F ( 0);
	    rot_dx  = F (mode_w);   rot_dy  = F ( mode_h);
	    break;
	case RR_Rotate_270:
	    rot_cos = F ( 0);	    rot_sin = F (-1);
	    rot_dx  = F ( 0);	    rot_dy  = F ( mode_w);
	    break;
	}
	
	PictureTransformRotate (&crtc_to_fb, &fb_to_crtc, rot_cos, rot_sin);
	PictureTransformTranslate (&crtc_to_fb, &fb_to_crtc, rot_dx, rot_dy);

	/* reflection */
	scale_x = F (1);
	scale_dx = 0;
	scale_y = F (1);
	scale_dy = 0;
	if (rotation & RR_Reflect_X)
	{
	    scale_x = F(-1);
	    if (rotation & (RR_Rotate_0|RR_Rotate_180))
		scale_dx = F(mode_w);
	    else
		scale_dx = F(mode_h);
	}
	if (rotation & RR_Reflect_Y)
	{
	    scale_y = F(-1);
	    if (rotation & (RR_Rotate_0|RR_Rotate_180))
		scale_dy = F(mode_h);
	    else
		scale_dy = F(mode_w);
	}
	
	PictureTransformScale (&crtc_to_fb, &fb_to_crtc, scale_x, scale_y);
	PictureTransformTranslate (&crtc_to_fb, &fb_to_crtc, scale_dx, scale_dy);
    }
    
#ifdef RANDR_12_INTERFACE
    if (crtc->randr_crtc)
    {

	RRTransformPtr	transform = RRCrtcGetTransform (crtc->randr_crtc);
	if (transform)
	{
	    if (transform->nparams) {
		new_params = xalloc (transform->nparams * sizeof (xFixed));
		if (new_params) {
		    memcpy (new_params, transform->params,
			    transform->nparams * sizeof (xFixed));
		    new_nparams = transform->nparams;
		    new_filter = transform->filter;
		}
	    } else
		new_filter = transform->filter;
	    if (new_filter)
	    {
		new_width = new_filter->width;
		new_height = new_filter->height;
	    }
	    PictureTransformMultiply (&crtc_to_fb, &transform->transform, &crtc_to_fb);
	    PictureTransformMultiply (&fb_to_crtc, &fb_to_crtc, &transform->inverse);
	}
    }
#endif
    /*
     * If the untranslated transformation is the identity,
     * disable the shadow buffer
     */
    if (PictureTransformIsIdentity (&crtc_to_fb))
    {
	PictureTransformInitTranslate (&crtc_to_fb,
				       F (-crtc->x), F (-crtc->y));
	PictureTransformInitTranslate (&fb_to_crtc,
				       F ( crtc->x), F ( crtc->y));
	xf86RotateDestroy (crtc);
	crtc->transform_in_use = FALSE;
	if (new_params)
	    xfree (new_params);
	new_params = NULL;
	new_nparams = 0;
	new_filter = NULL;
	new_width = 0;
	new_height = 0;
    }
    else
    {
	int width, height, old_width, old_height;
	void *shadowData;
	PixmapPtr shadow;

	PictureTransformTranslate (&crtc_to_fb, &fb_to_crtc, F(crtc->x), F(crtc->y));

	/* 
	 * these are the size of the shadow pixmap, which
	 * matches the mode, not the pre-rotated copy in the
	 * frame buffer
	 */
	width = mode->HDisplay;
	height = mode->VDisplay;
	shadowData = crtc->rotatedData;
	shadow = crtc->rotatedPixmap;
	old_width = shadow ? shadow->drawable.width : 0;
	old_height = shadow ? shadow->drawable.height : 0;
	
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
	    
	    /* Wrap block handler */
	    xf86_config->BlockHandler = pScreen->BlockHandler;
	    pScreen->BlockHandler = xf86RotateBlockHandler;
	}
	if (0)
	{
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
	    if (new_params)
		xfree (new_params);
	    return FALSE;
	}
	crtc->transform_in_use = TRUE;
    }
    crtc->crtc_to_framebuffer = crtc_to_fb;
    crtc->framebuffer_to_crtc = fb_to_crtc;
    if (crtc->params)
	xfree (crtc->params);
    crtc->params = new_params;
    crtc->nparams = new_nparams;
    crtc->filter = new_filter;
    crtc->filter_width = new_width;
    crtc->filter_height = new_height;
    crtc->bounds.x1 = 0;
    crtc->bounds.x2 = crtc->mode.HDisplay;
    crtc->bounds.y1 = 0;
    crtc->bounds.y2 = crtc->mode.VDisplay;
    PictureTransformBounds (&crtc->bounds, &crtc_to_fb);
    
    /* All done */
    return TRUE;
}
