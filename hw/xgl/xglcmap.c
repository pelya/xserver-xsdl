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
#include "colormapst.h"
#include "micmap.h"
#include "fb.h"

static xglPixelFormatRec xglPixelFormats[] = {
    {
	8, 8,
	{
	    8,
	    0x000000ff,
	    0x00000000,
	    0x00000000,
	    0x00000000
	}
    }, {
	15, 5,
	{
	    16,
	    0x00000000,
	    0x00007c00,
	    0x000003e0,
	    0x0000001f
	}
    }, {
	16, 6,
	{
	    16,
	    0x00000000,
	    0x0000f800,
	    0x000007e0,
	    0x0000001f
	}
    }, {
	24, 8,
	{
	    32,
	    0x00000000,
	    0x00ff0000,
	    0x0000ff00,
	    0x000000ff
	}
    }, {
	32, 8,
	{
	    32,
	    0xff000000,
	    0x00ff0000,
	    0x0000ff00,
	    0x000000ff
	}
    }
};

#define NUM_XGL_PIXEL_FORMATS				     \
    (sizeof (xglPixelFormats) / sizeof (xglPixelFormats[0]))

xglVisualPtr xglVisuals  = NULL;
int	     nxglVisuals = 0;

xglVisualPtr xglPbufferVisuals  = NULL;
int	     nxglPbufferVisuals = 0;

static xglPixelFormatPtr
xglFindPixelFormat (glitz_drawable_format_t *format,
		    int		            visuals)
{
    glitz_color_format_t *color;
    int			 depth, i;

    color = &format->color;
    
    depth = color->red_size + color->green_size + color->blue_size;

    if (!visuals)
	depth += color->alpha_size;
    
    for (i = 0; i < NUM_XGL_PIXEL_FORMATS; i++)
    {
	if (xglPixelFormats[i].depth == depth)
	{
	    xglPixelFormatPtr pPixel;

	    pPixel = &xglPixelFormats[i];
	    if (Ones (pPixel->masks.red_mask)   == color->red_size &&
		Ones (pPixel->masks.green_mask) == color->green_size &&
		Ones (pPixel->masks.blue_mask)  == color->blue_size)
	    {
		
		if (visuals)
		    return pPixel;

		if (Ones (pPixel->masks.alpha_mask) == color->alpha_size)
		    return pPixel;
	    }
	}
    }
    
    return NULL;
}

void
xglSetVisualTypesAndMasks (ScreenInfo	           *pScreenInfo,
			   glitz_drawable_format_t *format,
			   unsigned long           visuals)
{
    xglPixelFormatPtr pPixelFormat;

    pPixelFormat = xglFindPixelFormat (format, visuals);
    if (pPixelFormat)
    {
	if (visuals && format->types.window)
	{
	    xglVisuals = xrealloc (xglVisuals,
				   (nxglVisuals + 1) * sizeof (xglVisualRec));
	    
	    if (xglVisuals)
	    {
		xglVisuals[nxglVisuals].format  = format;
		xglVisuals[nxglVisuals].pPixel  = pPixelFormat;
		xglVisuals[nxglVisuals].visuals = visuals;
		nxglVisuals++;
	    }
	}

	if (format->types.pbuffer)
	{
	    xglPbufferVisuals =
		xrealloc (xglPbufferVisuals,
			  (nxglPbufferVisuals + 1) * sizeof (xglVisualRec));
	    
	    if (xglPbufferVisuals)
	    {
		xglPbufferVisuals[nxglPbufferVisuals].format  = format;
		xglPbufferVisuals[nxglPbufferVisuals].pPixel  = NULL;
		xglPbufferVisuals[nxglPbufferVisuals].visuals = 0;
		nxglPbufferVisuals++;
	    }
	}
    }
}

void
xglInitVisuals (ScreenInfo *pScreenInfo)
{
    int i, j;

    for (i = 0; i < pScreenInfo->numPixmapFormats; i++)
    {
	unsigned long visuals;
	unsigned int  bitsPerRGB;
	Pixel	      rm, gm, bm;
	
	visuals = 0;
	bitsPerRGB = 0;
	rm = gm = bm = 0;

	for (j = 0; j < nxglVisuals; j++)
	{
	    if (pScreenInfo->formats[i].depth == xglVisuals[j].pPixel->depth)
	    {
		visuals    = xglVisuals[j].visuals;
		bitsPerRGB = xglVisuals[j].pPixel->bitsPerRGB;
	    
		rm = xglVisuals[j].pPixel->masks.red_mask;
		gm = xglVisuals[j].pPixel->masks.green_mask;
		bm = xglVisuals[j].pPixel->masks.blue_mask;

		fbSetVisualTypesAndMasks (pScreenInfo->formats[i].depth,
					  visuals, bitsPerRGB,
					  rm, gm, bm);
	    }
	}

	if (!visuals)
	{
	    for (j = 0; j < NUM_XGL_PIXEL_FORMATS; j++)
	    {
		if (pScreenInfo->formats[i].depth == xglPixelFormats[j].depth)
		{
		    bitsPerRGB = xglPixelFormats[j].bitsPerRGB;
	    
		    rm = xglPixelFormats[j].masks.red_mask;
		    gm = xglPixelFormats[j].masks.green_mask;
		    bm = xglPixelFormats[j].masks.blue_mask;
		    break;
		}
	    }
	    
	    fbSetVisualTypesAndMasks (pScreenInfo->formats[i].depth,
				      visuals, bitsPerRGB,
				      rm, gm, bm);
	}
    }
}

void
xglClearVisualTypes (void)
{
    nxglVisuals        = 0;
    nxglPbufferVisuals = 0;

    if (xglVisuals)
	xfree (xglVisuals);

    if (xglPbufferVisuals)
	xfree (xglPbufferVisuals);

    xglVisuals        = NULL;
    xglPbufferVisuals = NULL;

    miClearVisualTypes ();
}

void
xglInitPixmapFormats (ScreenPtr pScreen)
{
    glitz_format_t *format, **best;
    int		   i, j;
    
    XGL_SCREEN_PRIV (pScreen);

    for (i = 0; i < 33; i++)
    {
	pScreenPriv->pixmapFormats[i].pPixel = NULL;
	pScreenPriv->pixmapFormats[i].format = NULL;
	
	for (j = 0; j < NUM_XGL_PIXEL_FORMATS; j++)
	{
	    if (xglPixelFormats[j].depth == i)
	    {
		int rs, gs, bs, as, k;
		
		pScreenPriv->pixmapFormats[i].pPixel = &xglPixelFormats[j];
		pScreenPriv->pixmapFormats[i].format = NULL;
		best = &pScreenPriv->pixmapFormats[i].format;

		rs = Ones (xglPixelFormats[j].masks.red_mask);
		gs = Ones (xglPixelFormats[j].masks.green_mask);
		bs = Ones (xglPixelFormats[j].masks.blue_mask);
		as = Ones (xglPixelFormats[j].masks.alpha_mask);

		k = 0;
		do {
		    format = glitz_find_format (pScreenPriv->drawable,
						0, NULL, k++);
		    if (format && format->type == GLITZ_FORMAT_TYPE_COLOR)
		    {
			/* find best matching sufficient format */
			if (format->color.red_size   >= rs &&
			    format->color.green_size >= gs &&
			    format->color.blue_size  >= bs &&
			    format->color.alpha_size >= as)
			{
			    if (*best)
			    {
				if (((format->color.red_size   - rs) +
				     (format->color.green_size - gs) +
				     (format->color.blue_size  - bs)) <
				    (((*best)->color.red_size   - rs) +
				     ((*best)->color.green_size - gs) +
				     ((*best)->color.blue_size  - bs)))
				    *best = format;
			    } else
				*best = format;
			}
		    }
		} while (format);
	    }
	}
    }
}

#define PIXEL_TO_COLOR(p, mask)						    \
    (((uint32_t) ((((uint64_t) (((uint32_t) (p)) & (mask))) * 0xffffffff) / \
		  ((uint64_t) (mask)))))

#define PIXEL_TO_RGB_COLOR(p, mask)	  \
    ((mask)? PIXEL_TO_COLOR (p, mask): 0)

void
xglPixelToColor (xglPixelFormatPtr pFormat,
		 CARD32		   pixel,
		 glitz_color_t	   *color)
{
    color->red   = PIXEL_TO_RGB_COLOR (pixel, pFormat->masks.red_mask);
    color->green = PIXEL_TO_RGB_COLOR (pixel, pFormat->masks.green_mask);
    color->blue  = PIXEL_TO_RGB_COLOR (pixel, pFormat->masks.blue_mask);
    
    if (pFormat->masks.alpha_mask)
	color->alpha = PIXEL_TO_COLOR (pixel, pFormat->masks.alpha_mask);
    else
	color->alpha = 0xffff;
}
