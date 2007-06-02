/*
 *
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include "fb.h"

#ifdef RENDER

#include "picturestr.h"
#include "mipict.h"
#include "fbpict.h"
#include "fbmmx.h"

static pixman_image_t *
create_solid_fill_image (PicturePtr pict)
{
    PictSolidFill *solid = &pict->pSourcePict->solidFill;
    pixman_color_t color;
    CARD32 a, r, g, b;
    
    a = (solid->color & 0xff000000) >> 24;
    r = (solid->color & 0x00ff0000) >> 16;
    g = (solid->color & 0x0000ff00) >>  8;
    b = (solid->color & 0x000000ff) >>  0;
    
    color.alpha = (a << 8) | a;
    color.red =   (r << 8) | r;
    color.green = (g << 8) | g;
    color.blue =  (b << 8) | b;
    
    return pixman_image_create_solid_fill (&color);
}

static pixman_image_t *
create_linear_gradient_image (PictGradient *gradient)
{
    PictLinearGradient *linear = (PictLinearGradient *)gradient;
    pixman_point_fixed_t p1;
    pixman_point_fixed_t p2;
    
    p1.x = linear->p1.x;
    p1.y = linear->p1.y;
    p2.x = linear->p2.x;
    p2.y = linear->p2.y;
    
    return pixman_image_create_linear_gradient (
	&p1, &p2, (pixman_gradient_stop_t *)gradient->stops, gradient->nstops);
}

static pixman_image_t *
create_radial_gradient_image (PictGradient *gradient)
{
    PictRadialGradient *radial = (PictRadialGradient *)gradient;
    pixman_point_fixed_t c1;
    pixman_point_fixed_t c2;
    
    c1.x = radial->c1.x;
    c1.y = radial->c1.y;
    c2.x = radial->c2.x;
    c2.y = radial->c2.y;
    
    return pixman_image_create_radial_gradient (
	&c1, &c2, radial->c1.radius,
	radial->c2.radius,
	(pixman_gradient_stop_t *)gradient->stops, gradient->nstops);
}

static pixman_image_t *
create_conical_gradient_image (PictGradient *gradient)
{
    PictConicalGradient *conical = (PictConicalGradient *)gradient;
    pixman_point_fixed_t center;
    
    center.x = conical->center.x;
    center.y = conical->center.y;
    
    return pixman_image_create_conical_gradient (
	&center, conical->angle, (pixman_gradient_stop_t *)gradient->stops,
	gradient->nstops);
}

static pixman_image_t *
create_bits_picture (PicturePtr pict,
		     Bool       has_clip)
{
    FbBits *bits;
    FbStride stride;
    int bpp, xoff, yoff;
    pixman_image_t *image;
    
    fbGetDrawable (pict->pDrawable, bits, stride, bpp, xoff, yoff);
    
    bits += yoff * stride + xoff;
    
    image = pixman_image_create_bits (
	pict->format,
	pict->pDrawable->width, pict->pDrawable->height,
	(uint32_t *)bits, stride * sizeof (FbStride));
    
    
#ifdef FB_ACCESS_WRAPPER
#if FB_SHIFT==5
    
    pixman_image_set_accessors (image,
				(pixman_read_memory_func_t)wfbReadMemory,
				(pixman_write_memory_func_t)wfbWriteMemory);
    
#else
    
#error The pixman library only works when FbBits is 32 bits wide
    
#endif
#endif
    
    /* pCompositeClip is undefined for source pictures, so
     * only set the clip region for pictures with drawables
     */
    if (has_clip)
	pixman_image_set_clip_region (image, pict->pCompositeClip);
    
    /* Indexed table */
    if (pict->pFormat->index.devPrivate)
	pixman_image_set_indexed (image, pict->pFormat->index.devPrivate);
    
    fbFinishAccess (pict->pDrawable);

    return image;
}

#define mod(a,b) ((b) == 1 ? 0 : (a) >= 0 ? (a) % (b) : (b) - (-a) % (b))

void
fbWalkCompositeRegion (CARD8 op,
		       PicturePtr pSrc,
		       PicturePtr pMask,
		       PicturePtr pDst,
		       INT16 xSrc,
		       INT16 ySrc,
		       INT16 xMask,
		       INT16 yMask,
		       INT16 xDst,
		       INT16 yDst,
		       CARD16 width,
		       CARD16 height,
		       Bool srcRepeat,
		       Bool maskRepeat,
		       CompositeFunc compositeRect)
{
    RegionRec	    region;
    int		    n;
    BoxPtr	    pbox;
    int		    w, h, w_this, h_this;
    int		    x_msk, y_msk, x_src, y_src, x_dst, y_dst;
    
    xDst += pDst->pDrawable->x;
    yDst += pDst->pDrawable->y;
    if (pSrc->pDrawable)
    {
        xSrc += pSrc->pDrawable->x;
        ySrc += pSrc->pDrawable->y;
    }
    if (pMask && pMask->pDrawable)
    {
	xMask += pMask->pDrawable->x;
	yMask += pMask->pDrawable->y;
    }

    if (!miComputeCompositeRegion (&region, pSrc, pMask, pDst, xSrc, ySrc,
				   xMask, yMask, xDst, yDst, width, height))
        return;
    
    n = REGION_NUM_RECTS (&region);
    pbox = REGION_RECTS (&region);
    while (n--)
    {
	h = pbox->y2 - pbox->y1;
	y_src = pbox->y1 - yDst + ySrc;
	y_msk = pbox->y1 - yDst + yMask;
	y_dst = pbox->y1;
	while (h)
	{
	    h_this = h;
	    w = pbox->x2 - pbox->x1;
	    x_src = pbox->x1 - xDst + xSrc;
	    x_msk = pbox->x1 - xDst + xMask;
	    x_dst = pbox->x1;
	    if (maskRepeat)
	    {
		y_msk = mod (y_msk - pMask->pDrawable->y, pMask->pDrawable->height);
		if (h_this > pMask->pDrawable->height - y_msk)
		    h_this = pMask->pDrawable->height - y_msk;
		y_msk += pMask->pDrawable->y;
	    }
	    if (srcRepeat)
	    {
		y_src = mod (y_src - pSrc->pDrawable->y, pSrc->pDrawable->height);
		if (h_this > pSrc->pDrawable->height - y_src)
		    h_this = pSrc->pDrawable->height - y_src;
		y_src += pSrc->pDrawable->y;
	    }
	    while (w)
	    {
		w_this = w;
		if (maskRepeat)
		{
		    x_msk = mod (x_msk - pMask->pDrawable->x, pMask->pDrawable->width);
		    if (w_this > pMask->pDrawable->width - x_msk)
			w_this = pMask->pDrawable->width - x_msk;
		    x_msk += pMask->pDrawable->x;
		}
		if (srcRepeat)
		{
		    x_src = mod (x_src - pSrc->pDrawable->x, pSrc->pDrawable->width);
		    if (w_this > pSrc->pDrawable->width - x_src)
			w_this = pSrc->pDrawable->width - x_src;
		    x_src += pSrc->pDrawable->x;
		}
		(*compositeRect) (op, pSrc, pMask, pDst,
				  x_src, y_src, x_msk, y_msk, x_dst, y_dst,
				  w_this, h_this);
		w -= w_this;
		x_src += w_this;
		x_msk += w_this;
		x_dst += w_this;
	    }
	    h -= h_this;
	    y_src += h_this;
	    y_msk += h_this;
	    y_dst += h_this;
	}
	pbox++;
    }
    REGION_UNINIT (pDst->pDrawable->pScreen, &region);
}

void
fbComposite (CARD8      op,
	     PicturePtr pSrc,
	     PicturePtr pMask,
	     PicturePtr pDst,
	     INT16      xSrc,
	     INT16      ySrc,
	     INT16      xMask,
	     INT16      yMask,
	     INT16      xDst,
	     INT16      yDst,
	     CARD16     width,
	     CARD16     height)
{
    pixman_region16_t region;
    pixman_image_t *src, *mask, *dest;
    
    xDst += pDst->pDrawable->x;
    yDst += pDst->pDrawable->y;
    if (pSrc->pDrawable)
    {
        xSrc += pSrc->pDrawable->x;
        ySrc += pSrc->pDrawable->y;
    }
    if (pMask && pMask->pDrawable)
    {
	xMask += pMask->pDrawable->x;
	yMask += pMask->pDrawable->y;
    }

    if (!miComputeCompositeRegion (&region, pSrc, pMask, pDst, xSrc, ySrc,
				   xMask, yMask, xDst, yDst, width, height))
        return;

    src = image_from_pict (pSrc, TRUE);
    mask = image_from_pict (pMask, TRUE);
    dest = image_from_pict (pDst, TRUE);

    if (src && dest && !(pMask && !mask))
    {
	pixman_image_composite (op, src, mask, dest,
				xSrc, ySrc, xMask, yMask, xDst, yDst,
				width, height, &region);
    }
    
    pixman_region_fini (&region);
    
    if (src)
	pixman_image_unref (src);
    if (mask)
	pixman_image_unref (mask);
    if (dest)
	pixman_image_unref (dest);
}

void
fbCompositeGeneral (CARD8	op,
		    PicturePtr	pSrc,
		    PicturePtr	pMask,
		    PicturePtr	pDst,
		    INT16	xSrc,
		    INT16	ySrc,
		    INT16	xMask,
		    INT16	yMask,
		    INT16	xDst,
		    INT16	yDst,
		    CARD16	width,
		    CARD16	height)
{
    return fbComposite (op, pSrc, pMask, pDst,
			xSrc, ySrc, xMask, yMask, xDst, yDst,
			width, height);
}

#endif /* RENDER */

Bool
fbPictureInit (ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{

#ifdef RENDER

    PictureScreenPtr    ps;

    if (!miPictureInit (pScreen, formats, nformats))
	return FALSE;
    ps = GetPictureScreen(pScreen);
    ps->Composite = fbComposite;
    ps->Glyphs = miGlyphs;
    ps->CompositeRects = miCompositeRects;
    ps->RasterizeTrapezoid = fbRasterizeTrapezoid;
    ps->AddTraps = fbAddTraps;
    ps->AddTriangles = fbAddTriangles;

#endif /* RENDER */

    return TRUE;
}

static void
set_image_properties (pixman_image_t *image, PicturePtr pict)
{
    pixman_repeat_t repeat;
    pixman_filter_t filter;
    
    if (pict->transform)
    {
	pixman_image_set_transform (
	    image, (pixman_transform_t *)pict->transform);
    }
    
    switch (pict->repeatType)
    {
    default:
    case RepeatNone:
	repeat = PIXMAN_REPEAT_NONE;
	break;
	
    case RepeatPad:
	repeat = PIXMAN_REPEAT_PAD;
	break;
	
    case RepeatNormal:
	repeat = PIXMAN_REPEAT_NORMAL;
	break;
	
    case RepeatReflect:
	repeat = PIXMAN_REPEAT_REFLECT;
	break;
    }
    
    pixman_image_set_repeat (image, repeat);
    
    if (pict->alphaMap)
    {
	pixman_image_t *alpha_map = image_from_pict (pict->alphaMap, TRUE);
	
	pixman_image_set_alpha_map (
	    image, alpha_map, pict->alphaOrigin.x, pict->alphaOrigin.y);
	
	pixman_image_unref (alpha_map);
    }
    
    pixman_image_set_component_alpha (image, pict->componentAlpha);

    switch (pict->filter)
    {
    default:
    case PictFilterNearest:
    case PictFilterFast:
	filter = PIXMAN_FILTER_NEAREST;
	break;
	
    case PictFilterBilinear:
    case PictFilterGood:
	filter = PIXMAN_FILTER_BILINEAR;
	break;
	
    case PictFilterConvolution:
	filter = PIXMAN_FILTER_CONVOLUTION;
	break;
    }
    
    pixman_image_set_filter (image, filter, (pixman_fixed_t *)pict->filter_params, pict->filter_nparams);
}

pixman_image_t *
image_from_pict (PicturePtr pict,
		 Bool       has_clip)
{
    pixman_image_t *image = NULL;

    if (!pict)
	return NULL;

    if (pict->pDrawable)
    {
	image = create_bits_picture (pict, has_clip);
    }
    else if (pict->pSourcePict)
    {
	SourcePict *sp = pict->pSourcePict;
	
	if (sp->type == SourcePictTypeSolidFill)
	{
	    image = create_solid_fill_image (pict);
	}
	else
	{
	    PictGradient *gradient = &pict->pSourcePict->gradient;
	    
	    if (sp->type == SourcePictTypeLinear)
		image = create_linear_gradient_image (gradient);
	    else if (sp->type == SourcePictTypeRadial)
		image = create_radial_gradient_image (gradient);
	    else if (sp->type == SourcePictTypeConical)
		image = create_conical_gradient_image (gradient);
	}
    }
    
    if (image)
	set_image_properties (image, pict);
    
    return image;
}




#ifdef USE_MMX
/* The CPU detection code needs to be in a file not compiled with
 * "-mmmx -msse", as gcc would generate CMOV instructions otherwise
 * that would lead to SIGILL instructions on old CPUs that don't have
 * it.
 */
#if !defined(__amd64__) && !defined(__x86_64__)

#ifdef HAVE_GETISAX
#include <sys/auxv.h>
#endif

enum CPUFeatures {
    NoFeatures = 0,
    MMX = 0x1,
    MMX_Extensions = 0x2, 
    SSE = 0x6,
    SSE2 = 0x8,
    CMOV = 0x10
};

static unsigned int detectCPUFeatures(void) {
    unsigned int features = 0;
    unsigned int result = 0;

#ifdef HAVE_GETISAX
    if (getisax(&result, 1)) {
        if (result & AV_386_CMOV)
            features |= CMOV;
        if (result & AV_386_MMX)
            features |= MMX;
        if (result & AV_386_AMD_MMX)
            features |= MMX_Extensions;
        if (result & AV_386_SSE)
            features |= SSE;
        if (result & AV_386_SSE2)
            features |= SSE2;
    }
#else
    char vendor[13];
#ifdef _MSC_VER
    int vendor0 = 0, vendor1, vendor2;
#endif
    vendor[0] = 0;
    vendor[12] = 0;

#ifdef __GNUC__
    /* see p. 118 of amd64 instruction set manual Vol3 */
    /* We need to be careful about the handling of %ebx and
     * %esp here. We can't declare either one as clobbered
     * since they are special registers (%ebx is the "PIC
     * register" holding an offset to global data, %esp the
     * stack pointer), so we need to make sure they have their
     * original values when we access the output operands.
     */
    __asm__ ("pushf\n"
             "pop %%eax\n"
             "mov %%eax, %%ecx\n"
             "xor $0x00200000, %%eax\n"
             "push %%eax\n"
             "popf\n"
             "pushf\n"
             "pop %%eax\n"
             "mov $0x0, %%edx\n"
             "xor %%ecx, %%eax\n"
             "jz 1f\n"

             "mov $0x00000000, %%eax\n"
	     "push %%ebx\n"
             "cpuid\n"
             "mov %%ebx, %%eax\n"
	     "pop %%ebx\n"
	     "mov %%eax, %1\n"
             "mov %%edx, %2\n"
             "mov %%ecx, %3\n"
             "mov $0x00000001, %%eax\n"
	     "push %%ebx\n"
             "cpuid\n"
	     "pop %%ebx\n"
             "1:\n"
             "mov %%edx, %0\n"
             : "=r" (result), 
               "=m" (vendor[0]), 
               "=m" (vendor[4]), 
               "=m" (vendor[8])
             :
             : "%eax", "%ecx", "%edx"
        );

#elif defined (_MSC_VER)

    _asm {
      pushfd
      pop eax
      mov ecx, eax
      xor eax, 00200000h
      push eax
      popfd
      pushfd
      pop eax
      mov edx, 0
      xor eax, ecx
      jz nocpuid

      mov eax, 0
      push ebx
      cpuid
      mov eax, ebx
      pop ebx
      mov vendor0, eax
      mov vendor1, edx
      mov vendor2, ecx
      mov eax, 1
      push ebx
      cpuid
      pop ebx
    nocpuid:
      mov result, edx
    }
    memmove (vendor+0, &vendor0, 4);
    memmove (vendor+4, &vendor1, 4);
    memmove (vendor+8, &vendor2, 4);

#else
#   error unsupported compiler
#endif
    
    features = 0;
    if (result) {
        /* result now contains the standard feature bits */
        if (result & (1 << 15))
            features |= CMOV;
        if (result & (1 << 23))
            features |= MMX;
        if (result & (1 << 25))
            features |= SSE;
        if (result & (1 << 26))
            features |= SSE2;
        if ((features & MMX) && !(features & SSE) &&
            (strcmp(vendor, "AuthenticAMD") == 0 ||
             strcmp(vendor, "Geode by NSC") == 0)) {
            /* check for AMD MMX extensions */
#ifdef __GNUC__
            __asm__("push %%ebx\n"
                    "mov $0x80000000, %%eax\n"
                    "cpuid\n"
                    "xor %%edx, %%edx\n"
                    "cmp $0x1, %%eax\n"
                    "jge 2f\n"
                    "mov $0x80000001, %%eax\n"
                    "cpuid\n"
                    "2:\n"
                    "pop %%ebx\n"
                    "mov %%edx, %0\n"
                    : "=r" (result)
                    :
                    : "%eax", "%ecx", "%edx"
                );
#elif defined _MSC_VER
            _asm {
              push ebx
              mov eax, 80000000h
              cpuid
              xor edx, edx
              cmp eax, 1
              jge notamd
              mov eax, 80000001h
              cpuid
            notamd:
              pop ebx
              mov result, edx
            }
#endif
            if (result & (1<<22))
                features |= MMX_Extensions;
        }
    }
#endif /* HAVE_GETISAX */

    return features;
}

Bool
fbHaveMMX (void)
{
    static Bool initialized = FALSE;
    static Bool mmx_present;

    if (!initialized)
    {
        unsigned int features = detectCPUFeatures();
	mmx_present = (features & (MMX|MMX_Extensions)) == (MMX|MMX_Extensions);
        initialized = TRUE;
    }
    
    return mmx_present;
}
#endif /* __amd64__ */
#endif
