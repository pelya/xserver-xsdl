/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * David Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * David Reveman makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"
#include "fb.h"

#ifdef RENDER

#define XGL_PICTURE_FALLBACK_PROLOGUE(pPicture, func) \
    xglSyncDamageBoxBits (pPicture->pDrawable);	      \
    XGL_PICTURE_SCREEN_UNWRAP (func)

#define XGL_PICTURE_FALLBACK_EPILOGUE(pPicture, func, xglfunc) \
    XGL_PICTURE_SCREEN_WRAP (func, xglfunc);		       \
    xglAddCurrentSurfaceDamage (pPicture->pDrawable)

void
xglComposite (CARD8	 op,
	      PicturePtr pSrc,
	      PicturePtr pMask,
	      PicturePtr pDst,
	      INT16	 xSrc,
	      INT16	 ySrc,
	      INT16	 xMask,
	      INT16	 yMask,
	      INT16	 xDst,
	      INT16	 yDst,
	      CARD16	 width,
	      CARD16	 height)
{
    PictureScreenPtr pPictureScreen;
    ScreenPtr	     pScreen = pDst->pDrawable->pScreen;

    XGL_SCREEN_PRIV (pScreen);

    if (xglComp (op,
		 pSrc, pMask, pDst,
		 xSrc, ySrc,
		 xMask, yMask,
		 xDst + pDst->pDrawable->x, yDst + pDst->pDrawable->y,
		 width, height,
		 NULL, NULL))
    {
	xglAddCurrentBitDamage (pDst->pDrawable);
	return;
    }
    
    pPictureScreen = GetPictureScreen (pScreen);

    if (!xglSyncBits (pSrc->pDrawable, NullBox))
	FatalError (XGL_SW_FAILURE_STRING);
    
    if (pMask)
    {
	if (!xglSyncBits (pMask->pDrawable, NullBox))
	    FatalError (XGL_SW_FAILURE_STRING);
    }

    if (op == PictOpSrc)
    {
	XGL_DRAWABLE_PIXMAP (pDst->pDrawable);
		
	if (!xglMapPixmapBits (pPixmap))
	    FatalError (XGL_SW_FAILURE_STRING);
    } else
	xglSyncDamageBoxBits (pDst->pDrawable);
    
    XGL_PICTURE_SCREEN_UNWRAP (Composite);
    (*pPictureScreen->Composite) (op, pSrc, pMask, pDst,
				  xSrc, ySrc, xMask, yMask, xDst, yDst,
				  width, height);
    XGL_PICTURE_SCREEN_WRAP (Composite, xglComposite);

    if (op == PictOpSrc)
    {
	RegionRec region;

	xDst += pDst->pDrawable->x;
	yDst += pDst->pDrawable->y;

	xSrc += pSrc->pDrawable->x;
	ySrc += pSrc->pDrawable->y;

	if (pMask)
	{
	    xMask += pMask->pDrawable->x;
	    yMask += pMask->pDrawable->y;
	}
	
        if (!miComputeCompositeRegion (&region, pSrc, pMask, pDst,
				       xSrc, ySrc, xMask, yMask, xDst, yDst,
				       width, height))
	    return;

	xglAddSurfaceDamage (pDst->pDrawable, &region);
	REGION_UNINIT (pDst->pDrawable->pScreen, &region);
    } else
	xglAddCurrentSurfaceDamage (pDst->pDrawable);
}

void
xglAddTriangles (PicturePtr pDst,
		 INT16	    xOff,
		 INT16	    yOff,
		 int	    ntri,
		 xTriangle  *tris)
{
    PictureScreenPtr pPictureScreen; 
    ScreenPtr	     pScreen = pDst->pDrawable->pScreen;

    XGL_SCREEN_PRIV (pScreen);
    XGL_DRAWABLE_PIXMAP_PRIV (pDst->pDrawable);

    pPictureScreen = GetPictureScreen (pScreen);

    pPixmapPriv->damageBox.x1 = 0;
    pPixmapPriv->damageBox.y1 = 0;
    pPixmapPriv->damageBox.x2 = pDst->pDrawable->width;
    pPixmapPriv->damageBox.y2 = pDst->pDrawable->height;

    XGL_PICTURE_FALLBACK_PROLOGUE (pDst, AddTriangles);
    (*pPictureScreen->AddTriangles) (pDst, xOff, yOff, ntri, tris);
    XGL_PICTURE_FALLBACK_EPILOGUE (pDst, AddTriangles, xglAddTriangles);
}


void
xglChangePicture (PicturePtr pPicture,
		  Mask	     mask)
{
    PictureScreenPtr pPictureScreen; 
    ScreenPtr	     pScreen = pPicture->pDrawable->pScreen;

    XGL_SCREEN_PRIV (pScreen);
    XGL_DRAWABLE_PIXMAP_PRIV (pPicture->pDrawable);

    pPictureScreen = GetPictureScreen (pScreen);

    if (pPicture->stateChanges & CPRepeat)
	pPixmapPriv->pictureMask |= xglPCFillMask;

    if (pPicture->stateChanges & CPComponentAlpha)
	pPixmapPriv->pictureMask |= xglPCComponentAlphaMask;

    if (pPicture->stateChanges & CPDither)
	pPixmapPriv->pictureMask |= xglPCDitherMask;

    XGL_PICTURE_SCREEN_UNWRAP (ChangePicture);
    (*pPictureScreen->ChangePicture) (pPicture, mask);
    XGL_PICTURE_SCREEN_WRAP (ChangePicture, xglChangePicture);
}

int
xglChangePictureTransform (PicturePtr    pPicture,
			   PictTransform *transform)
{
    PictureScreenPtr pPictureScreen; 
    ScreenPtr	     pScreen = pPicture->pDrawable->pScreen;
    int		     ret;

    XGL_SCREEN_PRIV (pScreen);
    XGL_DRAWABLE_PIXMAP_PRIV (pPicture->pDrawable);

    pPictureScreen = GetPictureScreen (pScreen);
    
    pPixmapPriv->pictureMask |= xglPCTransformMask;

    XGL_PICTURE_SCREEN_UNWRAP (ChangePictureTransform);
    ret = (*pPictureScreen->ChangePictureTransform) (pPicture, transform);
    XGL_PICTURE_SCREEN_WRAP (ChangePictureTransform,
			     xglChangePictureTransform);
    
    return ret;
}

int
xglChangePictureFilter (PicturePtr pPicture,
			int	   filter,
			xFixed	   *params,
			int	   nparams)
{
    PictureScreenPtr pPictureScreen; 
    ScreenPtr	     pScreen = pPicture->pDrawable->pScreen;
    int		     ret;

    XGL_SCREEN_PRIV (pScreen);
    XGL_DRAWABLE_PIXMAP_PRIV (pPicture->pDrawable);

    pPictureScreen = GetPictureScreen (pScreen);
    
    pPixmapPriv->pictureMask |= xglPCFilterMask;

    XGL_PICTURE_SCREEN_UNWRAP (ChangePictureFilter);
    ret = (*pPictureScreen->ChangePictureFilter) (pPicture, filter,
						  params, nparams);
    XGL_PICTURE_SCREEN_WRAP (ChangePictureFilter, xglChangePictureFilter);

    return ret;
}

void
xglUpdatePicture (PicturePtr pPicture)
{
    glitz_surface_t *surface;
    
    XGL_DRAWABLE_PIXMAP_PRIV (pPicture->pDrawable);

    surface = pPixmapPriv->surface;

    if (pPixmapPriv->pictureMask & xglPCFillMask)
    {
	if (pPicture->repeat)
	    glitz_surface_set_fill (surface, GLITZ_FILL_REPEAT);
	else
	    glitz_surface_set_fill (surface, GLITZ_FILL_TRANSPARENT);
    }

    if (pPixmapPriv->pictureMask & xglPCFilterMask)
    {
	switch (pPicture->filter) {
	case PictFilterNearest:
	case PictFilterFast:
	    glitz_surface_set_filter (surface, GLITZ_FILTER_NEAREST, NULL, 0);
	    break;
	case PictFilterGood:
	case PictFilterBest:
	case PictFilterBilinear:
	    glitz_surface_set_filter (surface, GLITZ_FILTER_BILINEAR, NULL, 0);
	    break;
	case PictFilterConvolution:
	    glitz_surface_set_filter (surface, GLITZ_FILTER_CONVOLUTION,
				      (glitz_fixed16_16_t *)
				      pPicture->filter_params,
				      pPicture->filter_nparams);
	    break;
	}
    }

    if (pPixmapPriv->pictureMask & xglPCTransformMask)
    {
	glitz_surface_set_transform (surface, (glitz_transform_t *)
				     pPicture->transform);
    }

    if (pPixmapPriv->pictureMask & xglPCComponentAlphaMask)
    {
	if (pPicture->componentAlpha)
	    glitz_surface_set_component_alpha (surface, 1);
	else
	    glitz_surface_set_component_alpha (surface, 0);
    }

    if (pPixmapPriv->pictureMask & xglPCDitherMask)
    {
	if (pPicture->dither)
	    glitz_surface_set_dither (surface, 1);
	else
	    glitz_surface_set_dither (surface, 0);
    }

    pPixmapPriv->pictureMask &= ~XGL_PICTURE_CHANGES (~0);
}

static int
xglVisualDepth (ScreenPtr pScreen, VisualPtr pVisual)
{
    DepthPtr pDepth;
    int	     d, v;

    for (d = 0; d < pScreen->numDepths; d++)
    {
	pDepth = &pScreen->allowedDepths[d];
	for (v = 0; v < pDepth->numVids; v++)
	    if (pDepth->vids[v] == pVisual->vid)
		return pDepth->depth;
    }
    
    return 0;
}

typedef struct _xglformatInit {
    CARD32 format;
    CARD8  depth;
} xglFormatInitRec, *xglFormatInitPtr;

static int
xglAddFormat (xglFormatInitPtr formats,
	      int	       nformat,
	      CARD32	       format,
	      CARD8	       depth)
{
    int	n;

    for (n = 0; n < nformat; n++)
	if (formats[n].format == format && formats[n].depth == depth)
	    return nformat;
    
    formats[nformat].format = format;
    formats[nformat].depth = depth;
    
    return ++nformat;
}

#define Mask(n)	((n) == 32 ? 0xffffffff : ((1 << (n)) - 1))

Bool
xglPictureInit (ScreenPtr pScreen)
{
    int		     f, nformats = 0;
    PictFormatPtr    pFormats;
    xglFormatInitRec formats[64];
    CARD32	     format;
    CARD8	     depth;
    VisualPtr	     pVisual;
    int		     v;
    int		     bpp;
    int		     r, g, b;
    int		     d;
    DepthPtr	     pDepth;
    
    /* formats required by protocol */
    formats[nformats].format = PICT_a1;
    formats[nformats].depth = 1;
    nformats++;
    formats[nformats].format = PICT_a4;
    formats[nformats].depth = 4;
    nformats++;
    formats[nformats].format = PICT_a8;
    formats[nformats].depth = 8;
    nformats++;
    formats[nformats].format = PICT_a8r8g8b8;
    formats[nformats].depth = 32;
    nformats++;

    /* now look through the depths and visuals adding other formats */
    for (v = 0; v < pScreen->numVisuals; v++)
    {
	pVisual = &pScreen->visuals[v];
	depth = xglVisualDepth (pScreen, pVisual);
	if (!depth)
	    continue;
	
    	bpp = BitsPerPixel (depth);
	switch (pVisual->class) {
	case DirectColor:
	case TrueColor:
	    r = Ones (pVisual->redMask);
	    g = Ones (pVisual->greenMask);
	    b = Ones (pVisual->blueMask);
	    if (pVisual->offsetBlue == 0 &&
		pVisual->offsetGreen == b &&
		pVisual->offsetRed == b + g)
	    {
		format = PICT_FORMAT (bpp, PICT_TYPE_ARGB, 0, r, g, b);
		nformats = xglAddFormat (formats, nformats, format, depth);
	    }
	    break;
	case StaticColor:
	case PseudoColor:
	case StaticGray:
	case GrayScale:
	    break;
	}
    }

    /* walk supported depths and add missing Direct formats */
    for (d = 0; d < pScreen->numDepths; d++)
    {
	pDepth = &pScreen->allowedDepths[d];
	bpp = BitsPerPixel (pDepth->depth);
	format = 0;
	switch (bpp) {
	case 16:
	    if (pDepth->depth == 15)
		nformats = xglAddFormat (formats, nformats,
					 PICT_x1r5g5b5, pDepth->depth);
	    if (pDepth->depth == 16) 
		nformats = xglAddFormat (formats, nformats,
					 PICT_r5g6b5, pDepth->depth);
	    break;
	case 24:
	    if (pDepth->depth == 24)
		nformats = xglAddFormat (formats, nformats,
					 PICT_r8g8b8, pDepth->depth);
	    break;
	case 32:
	    if (pDepth->depth == 24)
		nformats = xglAddFormat (formats, nformats,
					 PICT_x8r8g8b8, pDepth->depth);
	    break;
	}
    }

    pFormats = (PictFormatPtr) xalloc (nformats * sizeof (PictFormatRec));
    if (!pFormats)
	return 0;
    
    memset (pFormats, '\0', nformats * sizeof (PictFormatRec));
    for (f = 0; f < nformats; f++)
    {
        pFormats[f].id = FakeClientID (0);
	pFormats[f].depth = formats[f].depth;
	format = formats[f].format;
	pFormats[f].format = format;
	pFormats[f].type = PictTypeDirect;
	switch (PICT_FORMAT_TYPE (format)) {
	case PICT_TYPE_ARGB:
	    pFormats[f].direct.alphaMask = Mask (PICT_FORMAT_A (format));
	    if (pFormats[f].direct.alphaMask)
		pFormats[f].direct.alpha = (PICT_FORMAT_R (format) +
					    PICT_FORMAT_G (format) +
					    PICT_FORMAT_B (format));
	    
	    pFormats[f].direct.redMask = Mask (PICT_FORMAT_R (format));
	    pFormats[f].direct.red = (PICT_FORMAT_G (format) + 
				      PICT_FORMAT_B (format));
	    
	    pFormats[f].direct.greenMask = Mask (PICT_FORMAT_G (format));
	    pFormats[f].direct.green = PICT_FORMAT_B (format);
	    
	    pFormats[f].direct.blueMask = Mask (PICT_FORMAT_B (format));
	    pFormats[f].direct.blue = 0;
	    break;
	case PICT_TYPE_A:
	    pFormats[f].direct.alpha = 0;
	    pFormats[f].direct.alphaMask = Mask (PICT_FORMAT_A (format));
	    break;
	case PICT_TYPE_COLOR:
	case PICT_TYPE_GRAY:
	    break;
	}
    }

    if (!fbPictureInit (pScreen, pFormats, nformats))
	return FALSE;

    PictureAddFilter (pScreen, FilterConvolution, miFilterValidateParams);
    
    return TRUE;
}

void
xglPictureClipExtents (PicturePtr pPicture,
		       BoxPtr     extents)
{
    if (pPicture->clientClipType != CT_NONE)
    {
	BoxPtr clip = REGION_EXTENTS (pPicture->pDrawable->pScreen,
				      (RegionPtr) pPicture->clientClip);

	if (extents->x1 < pPicture->clipOrigin.x + clip->x1)
	    extents->x1 = pPicture->clipOrigin.x + clip->x1;

	if (extents->y1 < pPicture->clipOrigin.y + clip->y1)
	    extents->y1 = pPicture->clipOrigin.y + clip->y1;

	if (extents->x2 > pPicture->clipOrigin.x + clip->x2)
	    extents->x2 = pPicture->clipOrigin.x + clip->x2;

	if (extents->y2 > pPicture->clipOrigin.y + clip->y2)
	    extents->y2 = pPicture->clipOrigin.y + clip->y2;
    }
    else
    {
	if (extents->x1 < 0)
	    extents->x1 = 0;

	if (extents->y1 < 0)
	    extents->y1 = 0;

	if (extents->x2 > pPicture->pDrawable->width)
	    extents->x2 = pPicture->pDrawable->width;

	if (extents->y2 > pPicture->pDrawable->height)
	    extents->y2 = pPicture->pDrawable->height;
    }
}

#endif
