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

/*
 * This offscreen memory manager is horrible and needs some serious work.
 *
 * It's a recursive memory manager. It's quite fast but wastes huge
 * amounts of memory. A simple scoring mechanism is used and pixmaps
 * that blit to screen get high scores which makes a compositing
 * manager run fast.
 *
 * NOTE: With GL_ARB_uber_buffer or GL_EXT_render_target we probably
 * wont need this offscreen management at all.
 */

static glitz_drawable_buffer_t _buffers[] = {
    GLITZ_DRAWABLE_BUFFER_BACK_COLOR,
    GLITZ_DRAWABLE_BUFFER_FRONT_COLOR
};

#define MAX_LEVEL 6

static Bool
xglOffscreenMoveIn (xglOffscreenAreaPtr pArea,
		    PixmapPtr		pPixmap)
{
    XGL_PIXMAP_PRIV (pPixmap);

    if (!xglSyncSurface (&pPixmap->drawable))
	FatalError (XGL_SW_FAILURE_STRING);

    pArea->pPixmapPriv = pPixmapPriv;
    pArea->state       = xglOffscreenAreaOccupied;
    
    pPixmapPriv->pArea  = pArea;
    pPixmapPriv->target = xglPixmapTargetIn;

    glitz_surface_attach (pPixmapPriv->surface,
			  pArea->pOffscreen->drawable,
			  pArea->pOffscreen->buffer,
			  pArea->x, pArea->y);

    XGL_INCREMENT_PIXMAP_SCORE (pPixmapPriv, 500);

    return TRUE;
}

static void
xglOffscreenMoveOut (xglOffscreenAreaPtr pArea)
{
    glitz_surface_detach (pArea->pPixmapPriv->surface);

    pArea->pPixmapPriv->pArea  = NULL;
    pArea->pPixmapPriv->target = xglPixmapTargetOut;
    pArea->pPixmapPriv	       = NULL;
    pArea->state	       = xglOffscreenAreaAvailable;
}

static xglOffscreenAreaPtr
xglCreateOffscreenArea (xglOffscreenPtr pOffscreen,
			int	  	level,
			int		x,
			int		y)
{
    xglOffscreenAreaPtr pArea;
    int			i;
    
    pArea = xalloc (sizeof (xglOffscreenAreaRec));
    if (!pArea)
	return NULL;

    pArea->level	= level;
    pArea->x		= x;
    pArea->y		= y;
    pArea->pOffscreen	= pOffscreen;
    pArea->pPixmapPriv	= NULL;
    pArea->state	= xglOffscreenAreaAvailable;
    
    for (i = 0; i < 4; i++)
	pArea->pArea[i] = NULL;
    
    return pArea;
}

static void
xglDestroyOffscreenArea (xglOffscreenAreaPtr pArea)
{   
    if (!pArea)
	return;

    if (pArea->pPixmapPriv)
    {
	xglOffscreenMoveOut (pArea);
    }
    else
    {
	int i;
	
	for (i = 0; i < 4; i++)
	    xglDestroyOffscreenArea (pArea->pArea[i]);
    }
    
    xfree (pArea);
}

static Bool
xglOffscreenInit (xglOffscreenPtr	  pOffscreen,
		  glitz_drawable_t	  *drawable,
		  glitz_drawable_buffer_t buffer,
		  unsigned int		  width,
		  unsigned int		  height)
{
    pOffscreen->pArea = xglCreateOffscreenArea (NULL, 0, 0, 0);
    if (!pOffscreen->pArea)
	return FALSE;

    glitz_drawable_reference (drawable);

    pOffscreen->drawable = drawable;
    pOffscreen->format   = glitz_drawable_get_format (drawable);
    pOffscreen->buffer   = buffer;
    pOffscreen->width    = width;
    pOffscreen->height   = height;
    
    return TRUE;
}

static void
xglOffscreenFini (xglOffscreenPtr pOffscreen)
{
    xglDestroyOffscreenArea (pOffscreen->pArea);
    glitz_drawable_destroy (pOffscreen->drawable);
}

static int
xglOffscreenAreaGetTopScore (xglOffscreenAreaPtr pArea)
{
    int topScore;
    
    if (pArea->pPixmapPriv)
    {
	topScore = pArea->pPixmapPriv->score;
	XGL_DECREMENT_PIXMAP_SCORE (pArea->pPixmapPriv, 5);
	
	return topScore;
    }
    else
    {
	int topScore, score, i;
	
	topScore = 0;
	for (i = 0; i < 4; i++)
	{
	    if (pArea->pArea[i])
	    {
		score = xglOffscreenAreaGetTopScore (pArea->pArea[i]);
		if (score > topScore)
		    topScore = score;
	    }
	}
	return topScore;
    }
}


static Bool
xglOffscreenFindArea (xglOffscreenAreaPtr pArea,
		      PixmapPtr		  pPixmap,
		      int		  level)
{
    if (pArea->level > level)
	return FALSE;
	
    switch (pArea->state) {
    case xglOffscreenAreaOccupied:
    {
	XGL_PIXMAP_PRIV (pPixmap);
	
	if (pPixmapPriv->score < pArea->pPixmapPriv->score)
	{
	    XGL_DECREMENT_PIXMAP_SCORE (pArea->pPixmapPriv, 10);
	    
	    return FALSE;
	}
	
	xglOffscreenMoveOut (pArea);
    }
    /* fall-through */
    case xglOffscreenAreaAvailable:
    {
	if (pArea->level == level || pArea->level == MAX_LEVEL)
	{
	    if (xglOffscreenMoveIn (pArea, pPixmap))
		return TRUE;
	}
	else
	{
	    int dx[4], dy[4], i;
	    
	    dx[0] = dx[2] = dy[0] = dy[1] = 0;
	    dx[1] = dx[3] = pArea->pOffscreen->width  >> (pArea->level + 1);
	    dy[2] = dy[3] = pArea->pOffscreen->height >> (pArea->level + 1);
	    
	    for (i = 0; i < 4; i++)
	    {
		pArea->pArea[i] =
		    xglCreateOffscreenArea (pArea->pOffscreen,
					    pArea->level + 1,
					    pArea->x + dx[i],
					    pArea->y + dy[i]);
	    }

	    pArea->state = xglOffscreenAreaDivided;
	    
	    if (xglOffscreenFindArea (pArea->pArea[0], pPixmap, level))
		return TRUE;
	}
    } break;
    case xglOffscreenAreaDivided:
    {
	int i;
	
	if (pArea->level == level)
	{
	    int topScore;

	    XGL_PIXMAP_PRIV (pPixmap);

	    topScore = xglOffscreenAreaGetTopScore (pArea);
	    
	    if (pPixmapPriv->score >= topScore)
	    {
		/*
		 * Kick out old pixmaps
		 */
		for (i = 0; i < 4; i++)
		{
		    xglDestroyOffscreenArea (pArea->pArea[i]);
		    pArea->pArea[i] = NULL;
		}

		if (xglOffscreenMoveIn (pArea, pPixmap))
		    return TRUE;
	    }
	}
	else
	{
	    for (i = 0; i < 4; i++)
	    {
		if (xglOffscreenFindArea (pArea->pArea[i], pPixmap, level))
		    return TRUE;
	    }
	}
    } break;
    }

    return FALSE;
}

Bool
xglInitOffscreen (ScreenPtr	   pScreen,
		  xglScreenInfoPtr pScreenInfo)
{
    xglOffscreenPtr	    pOffscreen;
    int			nOffscreen;
    glitz_drawable_format_t *format;

    XGL_SCREEN_PRIV (pScreen);

    pScreenPriv->pOffscreen = NULL;
    pScreenPriv->nOffscreen = 0;

    format = glitz_drawable_get_format (pScreenPriv->drawable);

    /*
     * Use back buffer as offscreen area.
     */
    if (format->doublebuffer)
    {
	pScreenPriv->pOffscreen =
	    xrealloc (pScreenPriv->pOffscreen,
		      sizeof (xglOffscreenRec) *
		      (pScreenPriv->nOffscreen + 1));
	if (pScreenPriv->pOffscreen)
	{
	    pOffscreen = &pScreenPriv->pOffscreen[pScreenPriv->nOffscreen];
	    
	    if (xglOffscreenInit (pOffscreen,
				  pScreenPriv->drawable,
				  GLITZ_DRAWABLE_BUFFER_BACK_COLOR,
				  pScreenInfo->width, pScreenInfo->height))
	    {
		pScreenPriv->nOffscreen++;
		ErrorF ("Initialized %dx%d back buffer offscreen area\n",
			pScreenInfo->width, pScreenInfo->height);
	    }
	}
    }

    if (nxglPbufferVisuals)
    {
	glitz_pbuffer_attributes_t attributes;
	unsigned long		   mask;
	glitz_drawable_t           *pbuffer;
	int			   i;

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
	    attributes.width  = 2048;
	    attributes.height = 2048;
	    
	    mask = GLITZ_PBUFFER_WIDTH_MASK | GLITZ_PBUFFER_HEIGHT_MASK;

	    pbuffer =
		glitz_create_pbuffer_drawable (pScreenPriv->drawable,
					       xglPbufferVisuals[i].format,
					       &attributes, mask);

	    if (pbuffer)
	    {
		unsigned long width, height;
		int	      j;
		
		width  = glitz_drawable_get_width (pbuffer);
		height = glitz_drawable_get_height (pbuffer);
		
		j = 0;

		/*
		 * No back buffer? only add front buffer.
		 */
		if (!xglPbufferVisuals[i].format->doublebuffer)
		    j++;
		
		while (j < 2)
		{
		    pScreenPriv->pOffscreen =
			xrealloc (pScreenPriv->pOffscreen,
				  sizeof (xglOffscreenRec) *
				  (pScreenPriv->nOffscreen + 1));
		    if (pScreenPriv->pOffscreen)
		    {
			pOffscreen =
			    &pScreenPriv->pOffscreen[pScreenPriv->nOffscreen];

			if (xglOffscreenInit (pOffscreen,
					      pbuffer, _buffers[j],
					      width, height))
			{
			    pScreenPriv->nOffscreen++;
			    ErrorF ("Initialized %dx%d pbuffer offscreen "
				    "area\n", width, height);
			}
		    }
		    j++;
		}
		glitz_drawable_destroy (pbuffer);
	    }
	}
    }

    pOffscreen = pScreenPriv->pOffscreen;
    nOffscreen = pScreenPriv->nOffscreen;

    /*
     * Update offscreen pointers in root offscreen areas
     */
    while (nOffscreen--)
    {
	pOffscreen->pArea->pOffscreen = pOffscreen;
	pOffscreen++;
    }
    
    return TRUE;
}

void
xglFiniOffscreen (ScreenPtr pScreen)
{
    XGL_SCREEN_PRIV (pScreen);
	
    while (pScreenPriv->nOffscreen--)
	xglOffscreenFini (&pScreenPriv->pOffscreen[pScreenPriv->nOffscreen]);
    
    if (pScreenPriv->pOffscreen)
	xfree (pScreenPriv->pOffscreen);
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
	int level;

	if (pOffscreen->format->color.red_size   >= pColor->red_size &&
	    pOffscreen->format->color.green_size >= pColor->green_size &&
	    pOffscreen->format->color.blue_size  >= pColor->blue_size &&
	    pOffscreen->format->color.alpha_size >= pColor->alpha_size)
	{

	    level = 0;
	    while ((pOffscreen->width  >> level) >= pPixmap->drawable.width &&
		   (pOffscreen->height >> level) >= pPixmap->drawable.height)
		level++;
	    
	    if (!level)
		continue;

	    if (xglOffscreenFindArea (pOffscreen->pArea, pPixmap, level - 1))
		return TRUE;
	}
	pOffscreen++;
    }

    return FALSE;
}

void
xglWithdrawOffscreenArea (xglOffscreenAreaPtr pArea)
{
    pArea->pPixmapPriv = NULL;
    pArea->state       = xglOffscreenAreaAvailable;
}
