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

Bool
xglSolid (DrawablePtr	   pDrawable,
	  glitz_operator_t op,
	  glitz_color_t	   *color,
	  xglGeometryPtr   pGeometry,
	  BoxPtr	   pBox,
	  int		   nBox)
{
    glitz_surface_t *surface;
    int		    xOff, yOff;
    
    XGL_SCREEN_PRIV (pDrawable->pScreen);

    if (!xglPrepareTarget (pDrawable))
	return FALSE;
    
    XGL_GET_DRAWABLE (pDrawable, surface, xOff, yOff);

    glitz_set_rectangle (pScreenPriv->solid, color, 0, 0, 1, 1);

    GEOMETRY_TRANSLATE (pGeometry, xOff, yOff);
    
    if (!GEOMETRY_ENABLE_ALL_VERTICES (pGeometry, surface))
	return FALSE;
    
    while (nBox--)
    {
	glitz_composite (op,
			 pScreenPriv->solid, NULL, surface,
			 0, 0,
			 0, 0,
			 pBox->x1 + xOff,
			 pBox->y1 + yOff,
			 pBox->x2 - pBox->x1, pBox->y2 - pBox->y1);

	pBox++;
    }

    if (glitz_surface_get_status (surface))
	return FALSE;

    return TRUE;
}
