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

struct xglDataTypeInfo {
    glitz_data_type_t type;
    int		      size;
} dataTypes[] = {
    { GLITZ_DATA_TYPE_SHORT, sizeof (glitz_short_t) },
    { GLITZ_DATA_TYPE_FLOAT, sizeof (glitz_float_t) }
};

glitz_buffer_hint_t usageTypes[] = {
    GLITZ_BUFFER_HINT_STREAM_DRAW,
    GLITZ_BUFFER_HINT_STATIC_DRAW,
    GLITZ_BUFFER_HINT_DYNAMIC_DRAW
};

void
xglGeometryResize (ScreenPtr	  pScreen,
		   xglGeometryPtr pGeometry,
		   int		  size)
{
    XGL_SCREEN_PRIV (pScreen);

    if (pGeometry->broken)
	return;
    
    if (size == pGeometry->size)
	return;

    if (pGeometry->usage == GEOMETRY_USAGE_USERMEM)
    {
	pGeometry->data =
	    xrealloc (pGeometry->data,
		      size * dataTypes[pGeometry->dataType].size);

	if (pGeometry->buffer)
	    glitz_buffer_destroy (pGeometry->buffer);
	
	pGeometry->buffer = NULL;
	
	if (pGeometry->data)
	{
	    pGeometry->buffer = glitz_buffer_create_for_data (pGeometry->data);
	    if (!pGeometry->buffer)
	    {
		pGeometry->broken = TRUE;
		return;
	    }
	}
	else if (size)
	{
	    pGeometry->broken = TRUE;
	    return;
	}
    }
    else
    {
	glitz_buffer_t *newBuffer;
	int	       dataTypeSize;

	dataTypeSize = dataTypes[pGeometry->dataType].size;

	if (size)
	{
	    newBuffer =
		glitz_geometry_buffer_create (pScreenPriv->drawable, NULL,
					      size * dataTypeSize,
					      usageTypes[pGeometry->usage]);
	    if (!newBuffer)
	    {
		pGeometry->broken = TRUE;
		return;
	    }
	} else
	    newBuffer = NULL;
	
	if (pGeometry->buffer && newBuffer)
	{
	    void *oldData, *newData;
	    
	    oldData = glitz_buffer_map (pGeometry->buffer,
					GLITZ_BUFFER_ACCESS_READ_ONLY);
	    newData = glitz_buffer_map (newBuffer,
					GLITZ_BUFFER_ACCESS_WRITE_ONLY);
	    
	    if (oldData && newData)
		memcpy (newData, oldData,
			MIN (size, pGeometry->size) * dataTypeSize);
	    
	    glitz_buffer_unmap (pGeometry->buffer);
	    glitz_buffer_unmap (newBuffer);
	    
	    glitz_buffer_destroy (pGeometry->buffer);
	}
	pGeometry->buffer = newBuffer;
    }
    
    pGeometry->size = size;
    
    if (pGeometry->endOffset > size)
	pGeometry->endOffset = size;
}

/*
 * Storage for 100 extra vertices are always allocated if
 * buffer size is to small. Geometry should be initialized
 * to desired size prior to calling this function when size
 * is known.
 */
#define RESIZE_GEOMETRY_FOR_VERTICES(pScreen, pGeometry, nvertices)	   \
    if (((pGeometry)->size - (pGeometry)->endOffset) < ((nvertices) << 1)) \
    {									   \
	xglGeometryResize (pScreen, pGeometry,				   \
			   (pGeometry)->endOffset +			   \
			   ((nvertices) << 1) + 200);			   \
	if ((pGeometry)->broken)					   \
	    return;							   \
    }

/*
 * Adds a number of rectangles as GL_QUAD primitives
 */
void
xglGeometryAddRect (ScreenPtr	   pScreen,
		    xglGeometryPtr pGeometry,
		    xRectangle	   *pRect,
		    int		   nRect)
{
    int  nvertices;
    void *ptr;

    if (pGeometry->broken)
	return;

    if (nRect < 1)
	return;

    nvertices = nRect << 2;

    RESIZE_GEOMETRY_FOR_VERTICES (pScreen, pGeometry, nvertices);

    ptr = glitz_buffer_map (pGeometry->buffer, GLITZ_BUFFER_ACCESS_WRITE_ONLY);
    if (!ptr)
    {
	pGeometry->broken = TRUE;
	return;
    }
    
    switch (pGeometry->dataType) {
    case GEOMETRY_DATA_TYPE_SHORT:
    {
	glitz_short_t *data = (glitz_short_t *) ptr;

	data += pGeometry->endOffset;

	while (nRect--)
	{
	    *data++ = pRect->x;
	    *data++ = pRect->y;
	    *data++ = pRect->x + pRect->width;
	    *data++ = pRect->y;
	    *data++ = pRect->x + pRect->width;
	    *data++ = pRect->y + pRect->height;
	    *data++ = pRect->x;
	    *data++ = pRect->y + pRect->height;

	    pRect++;
	}
    } break;
    case GEOMETRY_DATA_TYPE_FLOAT:
    {
	glitz_float_t *data = (glitz_float_t *) ptr;

	data += pGeometry->endOffset;

	while (nRect--)
	{
	    *data++ = (glitz_float_t) pRect->x;
	    *data++ = (glitz_float_t) pRect->y;
	    *data++ = (glitz_float_t) pRect->x + pRect->width;
	    *data++ = (glitz_float_t) pRect->y;
	    *data++ = (glitz_float_t) pRect->x + pRect->width;
	    *data++ = (glitz_float_t) pRect->y + pRect->height;
	    *data++ = (glitz_float_t) pRect->x;
	    *data++ = (glitz_float_t) pRect->y + pRect->height;
		
	    pRect++;
	}
    } break;
    }
	
    if (glitz_buffer_unmap (pGeometry->buffer))
    {
	pGeometry->broken = TRUE;
	return;
    }

    pGeometry->endOffset += (nvertices << 1);
}

/*
 * Adds a number of boxes as GL_QUAD primitives
 */
void
xglGeometryAddBox (ScreenPtr	  pScreen,
		   xglGeometryPtr pGeometry,
		   BoxPtr	  pBox,
		   int		  nBox)
{
    int	 nvertices;
    void *ptr;

    if (pGeometry->broken)
	return;

    if (nBox < 1)
	return;

    nvertices = nBox << 2;

    RESIZE_GEOMETRY_FOR_VERTICES (pScreen, pGeometry, nvertices);
	
    ptr = glitz_buffer_map (pGeometry->buffer, GLITZ_BUFFER_ACCESS_WRITE_ONLY);
    if (!ptr)
    {
	pGeometry->broken = TRUE;
	return;
    }

    switch (pGeometry->dataType) {
    case GEOMETRY_DATA_TYPE_SHORT:
    {
	glitz_short_t *data = (glitz_short_t *) ptr;

	data += pGeometry->endOffset;

	while (nBox--)
	{
	    *data++ = (glitz_short_t) pBox->x1;
	    *data++ = (glitz_short_t) pBox->y1;
	    *data++ = (glitz_short_t) pBox->x2;
	    *data++ = (glitz_short_t) pBox->y1;
	    *data++ = (glitz_short_t) pBox->x2;
	    *data++ = (glitz_short_t) pBox->y2;
	    *data++ = (glitz_short_t) pBox->x1;
	    *data++ = (glitz_short_t) pBox->y2;

	    pBox++;
	}
    } break;
    case GEOMETRY_DATA_TYPE_FLOAT:
    {
	glitz_float_t *data = (glitz_float_t *) ptr;
	
	data += pGeometry->endOffset;

	while (nBox--)
	{
	    *data++ = (glitz_float_t) pBox->x1;
	    *data++ = (glitz_float_t) pBox->y1;
	    *data++ = (glitz_float_t) pBox->x2;
	    *data++ = (glitz_float_t) pBox->y1;
	    *data++ = (glitz_float_t) pBox->x2;
	    *data++ = (glitz_float_t) pBox->y2;
	    *data++ = (glitz_float_t) pBox->x1;
	    *data++ = (glitz_float_t) pBox->y2;

	    pBox++;
	}
    } break;
    }

    if (glitz_buffer_unmap (pGeometry->buffer))
    {
	pGeometry->broken = TRUE;
	return;
    }

    pGeometry->endOffset += (nvertices << 1);
}

/*
 * Adds a number of spans as GL_LINE primitives
 *
 * An extra 1 is added to *pwidth as OpenGL line segments are half-opened.
 */
void
xglGeometryAddSpan (ScreenPtr	   pScreen,
		    xglGeometryPtr pGeometry,
		    DDXPointPtr	   ppt,
		    int		   *pwidth,
		    int		   n)
{
    int  nvertices;
    void *ptr;

    if (pGeometry->broken)
	return;

    if (n < 1)
	return;

    nvertices = n << 1;

    RESIZE_GEOMETRY_FOR_VERTICES (pScreen, pGeometry, nvertices);
	
    ptr = glitz_buffer_map (pGeometry->buffer, GLITZ_BUFFER_ACCESS_WRITE_ONLY);
    if (!ptr)
    {
	pGeometry->broken = TRUE;
	return;
    }
    
    switch (pGeometry->dataType) {
    case GEOMETRY_DATA_TYPE_SHORT:
    {
	glitz_short_t *data = (glitz_short_t *) ptr;

	data += pGeometry->endOffset;

	while (n--)
	{
	    *data++ = (glitz_short_t) ppt->x;
	    *data++ = (glitz_short_t) ppt->y;
	    *data++ = (glitz_short_t) (ppt->x + *pwidth + 1);
	    *data++ = (glitz_short_t) ppt->y;
	
	    ppt++;
	    pwidth++;
	}
    } break;
    case GEOMETRY_DATA_TYPE_FLOAT:
    {
	glitz_float_t *data = (glitz_float_t *) ptr;

	data += pGeometry->endOffset;

	while (n--)
	{
	    *data++ = (glitz_float_t) ppt->x;
	    *data++ = (glitz_float_t) ppt->y;
	    *data++ = (glitz_float_t) (ppt->x + *pwidth + 1);
	    *data++ = (glitz_float_t) ppt->y;
	
	    ppt++;
	    pwidth++;
	}
    } break;
    }
	
    if (glitz_buffer_unmap (pGeometry->buffer))
    {
	pGeometry->broken = TRUE;
	return;
    }

    pGeometry->endOffset += (nvertices << 1);
}

/*
 * This macro is needed for end pixels to be rasterized correctly using
 * OpenGL as OpenGL line segments are half-opened.
 */
#define ADJUST_END_POINT(start, end, isPoint) \
    (((end) > (start)) ? (end) + 1:	      \
     ((end) < (start)) ? (end) - 1:	      \
     (isPoint)	       ? (end) + 1:	      \
     (end))

/*
 * Adds a number of connected lines as GL_LINE_STRIP primitives
 */
void
xglGeometryAddLine (ScreenPtr	   pScreen,
		    xglGeometryPtr pGeometry,
		    int		   loop,
		    int		   mode,
		    int		   npt,
		    DDXPointPtr    ppt)
{
    DDXPointRec pt;
    int		nvertices;
    void	*ptr;

    if (pGeometry->broken)
	return;

    if (npt < 2)
	return;

    nvertices = npt;

    RESIZE_GEOMETRY_FOR_VERTICES (pScreen, pGeometry, nvertices);
	
    ptr = glitz_buffer_map (pGeometry->buffer, GLITZ_BUFFER_ACCESS_WRITE_ONLY);
    if (!ptr)
    {
	pGeometry->broken = TRUE;
	return;
    }

    pt.x = 0;
    pt.y = 0;
    
    switch (pGeometry->dataType) {
    case GEOMETRY_DATA_TYPE_SHORT:
    {
	glitz_short_t *data = (glitz_short_t *) ptr;

	data += pGeometry->endOffset;

	while (npt--)
	{
	    if (mode == CoordModePrevious)
	    {
		pt.x += ppt->x;
		pt.y += ppt->y;
	    }
	    else
	    {
		pt.x = ppt->x;
		pt.y = ppt->y;
	    }

	    *data++ = pt.x;
	    *data++ = pt.y;

	    if (npt || loop)
	    {
		*data++ = pt.x;
		*data++ = pt.y;
	    }
	    else
	    {
		ppt--;
		*data++ = ADJUST_END_POINT (ppt->x, pt.x, ppt->y == pt.y);
		*data++ = ADJUST_END_POINT (ppt->y, pt.y, 0);
	    }
	
	    ppt++;
	}
    } break;
    case GEOMETRY_DATA_TYPE_FLOAT:
    {
	glitz_float_t *data = (glitz_float_t *) ptr;

	data += pGeometry->endOffset;
	
	while (npt--)
	{
	    if (mode == CoordModePrevious)
	    {
		pt.x += ppt->x;
		pt.y += ppt->y;
	    }
	    else
	    {
		pt.x = ppt->x;
		pt.y = ppt->y;
	    }

	    if (npt || loop)
	    {
		*data++ = (glitz_float_t) pt.x;
		*data++ = (glitz_float_t) pt.y;
	    }
	    else
	    {
		ppt--;
		*data++ = (glitz_float_t)
		    ADJUST_END_POINT (ppt->x, pt.x, ppt->y == pt.y);
		*data++ = (glitz_float_t) ADJUST_END_POINT (ppt->y, pt.y, 0);
	    }
	    
	    ppt++;
	}
    } break;
    }
	
    if (glitz_buffer_unmap (pGeometry->buffer))
    {
	pGeometry->broken = TRUE;
	return;
    }

    pGeometry->endOffset += (nvertices << 1);
}

/*
 * Adds a number of line segments as GL_LINE primitives
 */
void
xglGeometryAddSegment (ScreenPtr      pScreen,
		       xglGeometryPtr pGeometry,
		       int	      nsegInit,
		       xSegment       *pSegInit)
{
    int  nvertices;
    void *ptr;

    if (pGeometry->broken)
	return;

    if (nsegInit < 1)
	return;

    nvertices = nsegInit << 1;

    RESIZE_GEOMETRY_FOR_VERTICES (pScreen, pGeometry, nvertices);
	
    ptr = glitz_buffer_map (pGeometry->buffer, GLITZ_BUFFER_ACCESS_WRITE_ONLY);
    if (!ptr)
    {
	pGeometry->broken = TRUE;
	return;
    }

    switch (pGeometry->dataType) {
    case GEOMETRY_DATA_TYPE_SHORT:
    {
	glitz_short_t *data = (glitz_short_t *) ptr;

	data += pGeometry->endOffset;

	while (nsegInit--)
	{
	    *data++ = pSegInit->x1;
	    *data++ = pSegInit->y1;
	    *data++ = ADJUST_END_POINT (pSegInit->x1, pSegInit->x2,
					pSegInit->y1 == pSegInit->y2);
	    *data++ = ADJUST_END_POINT (pSegInit->y1, pSegInit->y2, 0);
	
	    pSegInit++;
	}
    } break;
    case GEOMETRY_DATA_TYPE_FLOAT:
    {
	glitz_float_t *data = (glitz_float_t *) ptr;

	data += pGeometry->endOffset;

	while (nsegInit--)
	{
	    *data++ = (glitz_float_t) pSegInit->x1;
	    *data++ = (glitz_float_t) pSegInit->y1;
	    *data++ = (glitz_float_t)
		ADJUST_END_POINT (pSegInit->x1, pSegInit->x2,
				  pSegInit->y1 == pSegInit->y2);
	    *data++ = (glitz_float_t)
		ADJUST_END_POINT (pSegInit->y1, pSegInit->y2, 0);
	    
	    pSegInit++;
	}
    } break;
    }
	
    if (glitz_buffer_unmap (pGeometry->buffer))
    {
	pGeometry->broken = TRUE;
	return;
    }

    pGeometry->endOffset += (nvertices << 1);
}

Bool
xglSetGeometry (xglGeometryPtr 	pGeometry,
		glitz_surface_t *surface,
		int		first,
		int		count)
{
    glitz_geometry_format_t format;
    
    if (pGeometry->broken)
	return FALSE;
    
    format.first     = first;
    format.count     = count;
    format.primitive = pGeometry->primitive;
    format.type      = dataTypes[pGeometry->dataType].type;
    format.mode      = GLITZ_GEOMETRY_MODE_DIRECT;
    format.edge_hint = GLITZ_GEOMETRY_EDGE_HINT_SHARP;

    glitz_set_geometry (surface,
			pGeometry->xOff, pGeometry->yOff,
			&format,
			pGeometry->buffer);

    return TRUE;
}
