/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the names of
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
 * Author: David Reveman <davidr@freedesktop.org>
 */

#include "xgl.h"

#ifdef RENDER

#define XGL_PICTURE_FALLBACK_PROLOGUE(pPicture, func) \
    xglSyncDamageBoxBits (pPicture->pDrawable);	      \
    XGL_PICTURE_SCREEN_UNWRAP (func)

#define XGL_PICTURE_FALLBACK_EPILOGUE(pPicture, func, xglfunc) \
    XGL_PICTURE_SCREEN_WRAP (func, xglfunc);		       \
    xglAddSurfaceDamage (pPicture->pDrawable)

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
		 xDst, yDst,
		 width, height))
	return;
    
    pPictureScreen = GetPictureScreen (pScreen);

    if (!xglSyncBits (pSrc->pDrawable, NullBox))
	FatalError (XGL_SW_FAILURE_STRING);
    
    if (pMask)
    {
	if (!xglSyncBits (pMask->pDrawable, NullBox))
	    FatalError (XGL_SW_FAILURE_STRING);
    }

    XGL_PICTURE_FALLBACK_PROLOGUE (pDst, Composite);
    (*pPictureScreen->Composite) (op, pSrc, pMask, pDst,
				  xSrc, ySrc, xMask, yMask, xDst, yDst,
				  width, height);
    XGL_PICTURE_FALLBACK_EPILOGUE (pDst, Composite, xglComposite);
}

void
xglGlyphs (CARD8	 op,
	   PicturePtr	 pSrc,
	   PicturePtr	 pDst,
	   PictFormatPtr maskFormat,
	   INT16	 xSrc,
	   INT16	 ySrc,
	   int		 nlist,
	   GlyphListPtr	 list,
	   GlyphPtr	 *glyphs)
{
    miGlyphs (op, pSrc, pDst, maskFormat, xSrc, ySrc, nlist, list, glyphs);
}

void
xglRasterizeTrapezoid (PicturePtr pDst,
		       xTrapezoid *trap,
		       int	  xOff,
		       int	  yOff)
{
    PictureScreenPtr pPictureScreen; 
    ScreenPtr	     pScreen = pDst->pDrawable->pScreen;

    XGL_SCREEN_PRIV (pScreen);

    pPictureScreen = GetPictureScreen (pScreen);

    XGL_PICTURE_FALLBACK_PROLOGUE (pDst, RasterizeTrapezoid);
    (*pPictureScreen->RasterizeTrapezoid) (pDst, trap, xOff, yOff);
    XGL_PICTURE_FALLBACK_EPILOGUE (pDst, RasterizeTrapezoid,
				   xglRasterizeTrapezoid);
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
	    pPixmapPriv->pictureMask &= ~xglPFFilterMask;
	    break;
	case PictFilterGood:
	case PictFilterBest:
	case PictFilterBilinear:
	    glitz_surface_set_filter (surface, GLITZ_FILTER_BILINEAR, NULL, 0);
	    pPixmapPriv->pictureMask &= ~xglPFFilterMask;
	    break;
	default:
	    pPixmapPriv->pictureMask |= xglPFFilterMask;
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

    pPixmapPriv->pictureMask &= ~XGL_PICTURE_CHANGES (~0);
}

#endif
