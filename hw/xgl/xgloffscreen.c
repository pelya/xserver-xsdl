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

static glitz_drawable_buffer_t _buffers[] = {
    GLITZ_DRAWABLE_BUFFER_BACK_COLOR,
    GLITZ_DRAWABLE_BUFFER_FRONT_COLOR
};

#define MAX_OFFSCREEN_LEVEL 8

static Bool
xglOffscreenCreate (xglAreaPtr pArea)
{
    return TRUE;
}

static Bool
xglOffscreenMoveIn (xglAreaPtr pArea,
		    pointer    closure)
{
    xglOffscreenPtr pOffscreen = (xglOffscreenPtr) pArea->pRoot->closure;
    PixmapPtr	    pPixmap = (PixmapPtr) closure;
    
    XGL_PIXMAP_PRIV (pPixmap);

    if (!xglSyncSurface (&pPixmap->drawable))
	FatalError (XGL_SW_FAILURE_STRING);

    pPixmapPriv->pArea  = pArea;
    pPixmapPriv->target = xglPixmapTargetIn;

    glitz_surface_attach (pPixmapPriv->surface,
			  pOffscreen->drawable, pOffscreen->buffer,
			  pArea->x, pArea->y);

    XGL_INCREMENT_PIXMAP_SCORE (pPixmapPriv, 100);

    return TRUE;
}

static void
xglOffscreenMoveOut (xglAreaPtr pArea,
		     pointer    closure)
{
    PixmapPtr pPixmap = (PixmapPtr) closure;

    XGL_PIXMAP_PRIV (pPixmap);
	
    glitz_surface_detach (pPixmapPriv->surface);
    
    pPixmapPriv->pArea  = NULL;
    pPixmapPriv->target = xglPixmapTargetOut;
}

static int
xglOffscreenCompareScore (xglAreaPtr pArea,
			  pointer    closure1,
			  pointer    closure2)
{
    int s1, s2;
    
    XGL_PIXMAP_PRIV ((PixmapPtr) closure1);

    s1 = pPixmapPriv->score;
    s2 = XGL_GET_PIXMAP_PRIV ((PixmapPtr) closure2)->score;
    
    if (s1 > s2)
	XGL_DECREMENT_PIXMAP_SCORE (pPixmapPriv, 10);

    if (pPixmapPriv->lock)
	return 1;
    
    return s1 - s2;
}

static const xglAreaFuncsRec xglOffscreenAreaFuncs = {
    xglOffscreenCreate,
    xglOffscreenMoveIn,
    xglOffscreenMoveOut,
    xglOffscreenCompareScore
};

Bool
xglInitOffscreen (ScreenPtr	   pScreen,
		  xglScreenInfoPtr pScreenInfo)
{
    xglOffscreenPtr	    pOffscreen;
    glitz_drawable_format_t *format;

    XGL_SCREEN_PRIV (pScreen);

    pOffscreen = pScreenPriv->pOffscreen;

    pScreenPriv->nOffscreen = 0;

    format = glitz_drawable_get_format (pScreenPriv->drawable);

    /*
     * Use back buffer as offscreen area.
     */
    if (format->doublebuffer)
    {
	pOffscreen->drawable = pScreenPriv->drawable;
	pOffscreen->format   = format;
	pOffscreen->buffer   = GLITZ_DRAWABLE_BUFFER_BACK_COLOR;
	
	if (xglRootAreaInit (&pOffscreen->rootArea,
			     MAX_OFFSCREEN_LEVEL,
			     pScreenInfo->width,
			     pScreenInfo->height, 0,
			     (xglAreaFuncsPtr) &xglOffscreenAreaFuncs,
			     (pointer) pOffscreen))
	{
	    glitz_drawable_reference (pOffscreen->drawable);
	    
	    pScreenPriv->nOffscreen++;
	    pOffscreen++;
	    ErrorF ("Initialized %dx%d back buffer offscreen area\n",
		    pScreenInfo->width, pScreenInfo->height);
	}
    }

    if (nxglPbufferVisuals)
    {
	glitz_drawable_t *pbuffer;
	unsigned int	 width, height;
	int		 i;
	
	for (i = 0; i < nxglPbufferVisuals; i++)
	{
	    /*
	     * This can be a bit tricky. I've noticed that when some OpenGL
	     * drivers can't create an accelerated pbuffer of the size we're
	     * requesting they create a software one with the correct
	     * size, but that's not what we want. So if your OpenGL driver
	     * supports accelerated pbuffers but offscreen drawing is really
	     * slow, try decrementing these values.
	     */
	    width  = 2048;
	    height = 2048;

	    do {
		pbuffer =
		    glitz_create_pbuffer_drawable (pScreenPriv->drawable,
						   xglPbufferVisuals[i].format,
						   width, height);
		width  >>= 1;
		height >>= 1;
	    } while (!pbuffer && width);

	    if (pbuffer)
	    {
		int j = 0;
		
		width  = glitz_drawable_get_width (pbuffer);
		height = glitz_drawable_get_height (pbuffer);
		
		/*
		 * No back buffer? only add front buffer.
		 */
		if (!xglPbufferVisuals[i].format->doublebuffer)
		    j++;
		
		while (j < 2)
		{
		    pOffscreen->drawable = pbuffer;
		    pOffscreen->format   = xglPbufferVisuals[i].format;
		    pOffscreen->buffer   = _buffers[j];
		    
		    if (xglRootAreaInit (&pOffscreen->rootArea,
					 MAX_OFFSCREEN_LEVEL,
					 width, height, 0,
					 (xglAreaFuncsPtr)
					 &xglOffscreenAreaFuncs,
					 (pointer) pOffscreen))
		    {
			glitz_drawable_reference (pbuffer);
			
			pScreenPriv->nOffscreen++;
			pOffscreen++;
			ErrorF ("Initialized %dx%d pbuffer offscreen area\n",
				width, height);
		    }
		    j++;
		}
		glitz_drawable_destroy (pbuffer);
	    }
	}
    }
    
    return TRUE;
}

void
xglFiniOffscreen (ScreenPtr pScreen)
{
    int n;
    
    XGL_SCREEN_PRIV (pScreen);

    n = pScreenPriv->nOffscreen;
    while (n--)
    {
	xglRootAreaFini (&pScreenPriv->pOffscreen[n].rootArea);
	glitz_drawable_destroy (pScreenPriv->pOffscreen[n].drawable);
    }

    pScreenPriv->nOffscreen = 0;
}

Bool
xglFindOffscreenArea (ScreenPtr pScreen,
		      PixmapPtr	pPixmap)
{
    xglOffscreenPtr	 pOffscreen;
    int			 nOffscreen;
    glitz_color_format_t *pColor;
    
    XGL_SCREEN_PRIV (pScreen);
    XGL_PIXMAP_PRIV (pPixmap);

    if (pPixmapPriv->score < 0)
	return FALSE;

    pColor = &pPixmapPriv->format->color;

    pOffscreen = pScreenPriv->pOffscreen;
    nOffscreen = pScreenPriv->nOffscreen;

    while (nOffscreen--)
    {
	if (pOffscreen->format->color.red_size   >= pColor->red_size &&
	    pOffscreen->format->color.green_size >= pColor->green_size &&
	    pOffscreen->format->color.blue_size  >= pColor->blue_size &&
	    pOffscreen->format->color.alpha_size >= pColor->alpha_size)
	{
	    /* Find available area */
	    if (xglFindArea (pOffscreen->rootArea.pArea,
			     pPixmap->drawable.width,
			     pPixmap->drawable.height,
			     FALSE,
			     (pointer) pPixmap))
		return TRUE;

	    /* Kicking out area with lower score */
	    if (xglFindArea (pOffscreen->rootArea.pArea,
			     pPixmap->drawable.width,
			     pPixmap->drawable.height,
			     TRUE,
			     (pointer) pPixmap))
		return TRUE;
	}

	pOffscreen++;
    }
    
    return FALSE;
}

void
xglLeaveOffscreenArea (PixmapPtr pPixmap)
{
    XGL_PIXMAP_PRIV (pPixmap);
    
    if (pPixmapPriv->pArea)
	xglLeaveArea (pPixmapPriv->pArea);
    
    pPixmapPriv->pArea = NULL;
}
