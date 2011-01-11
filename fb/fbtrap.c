/*
 * Copyright © 2004 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "fb.h"

#include "picturestr.h"
#include "mipict.h"
#include "fbpict.h"

void
fbAddTraps (PicturePtr	pPicture,
	    INT16	x_off,
	    INT16	y_off,
	    int		ntrap,
	    xTrap	*traps)
{
    int image_xoff, image_yoff;
    pixman_image_t *image = image_from_pict (pPicture, FALSE, &image_xoff, &image_yoff);

    if (!image)
	return;
    
    pixman_add_traps (image, x_off, y_off, ntrap, (pixman_trap_t *)traps);

    free_pixman_pict (pPicture, image);
}

void
fbRasterizeTrapezoid (PicturePtr    pPicture,
		      xTrapezoid  *trap,
		      int	    x_off,
		      int	    y_off)
{
    int	mask_xoff, mask_yoff;
    pixman_image_t *image = image_from_pict (pPicture, FALSE, &mask_xoff, &mask_yoff);

    if (!image)
	return;

    pixman_rasterize_trapezoid (image, (pixman_trapezoid_t *)trap, x_off, y_off);

    free_pixman_pict (pPicture, image);
}

static int
_GreaterY (xPointFixed *a, xPointFixed *b)
{
    if (a->y == b->y)
	return a->x > b->x;
    return a->y > b->y;
}

/*
 * Note that the definition of this function is a bit odd because
 * of the X coordinate space (y increasing downwards).
 */
static int
_Clockwise (xPointFixed *ref, xPointFixed *a, xPointFixed *b)
{
    xPointFixed	ad, bd;

    ad.x = a->x - ref->x;
    ad.y = a->y - ref->y;
    bd.x = b->x - ref->x;
    bd.y = b->y - ref->y;

    return ((xFixed_32_32) bd.y * ad.x - (xFixed_32_32) ad.y * bd.x) < 0;
}

/* FIXME -- this could be made more efficient */
void
fbAddTriangles (PicturePtr  pPicture,
		INT16	    x_off,
		INT16	    y_off,
		int	    ntri,
		xTriangle *tris)
{
    xPointFixed	  *top, *left, *right, *tmp;
    xTrapezoid	    trap;

    for (; ntri; ntri--, tris++)
    {
	top = &tris->p1;
	left = &tris->p2;
	right = &tris->p3;
	if (_GreaterY (top, left)) {
	    tmp = left; left = top; top = tmp;
	}
	if (_GreaterY (top, right)) {
	    tmp = right; right = top; top = tmp;
	}
	if (_Clockwise (top, right, left)) {
	    tmp = right; right = left; left = tmp;
	}
	
	/*
	 * Two cases:
	 *
	 *		+		+
	 *	       / \             / \
	 *	      /   \           /   \
	 *	     /     +         +     \
	 *          /    --           --    \
	 *         /   --               --   \
	 *        / ---                   --- \
	 *	 +--                         --+
	 */
	
	trap.top = top->y;
	trap.left.p1 = *top;
	trap.left.p2 = *left;
	trap.right.p1 = *top;
	trap.right.p2 = *right;
	if (right->y < left->y)
	    trap.bottom = right->y;
	else
	    trap.bottom = left->y;
	fbRasterizeTrapezoid (pPicture, &trap, x_off, y_off);
	if (right->y < left->y)
	{
	    trap.top = right->y;
	    trap.bottom = left->y;
	    trap.right.p1 = *right;
	    trap.right.p2 = *left;
	}
	else
	{
	    trap.top = left->y;
	    trap.bottom = right->y;
	    trap.left.p1 = *left;
	    trap.left.p2 = *right;
	}
	fbRasterizeTrapezoid (pPicture, &trap, x_off, y_off);
    }
}

typedef void (* CompositeShapesFunc) (pixman_op_t op,
				      pixman_image_t *src,
				      pixman_image_t *dst,
				      pixman_format_code_t mask_format,
				      int x_src, int y_src,
				      int x_dst, int y_dst,
				      int n_shapes, const uint8_t *shapes);

static void
fbShapes (CompositeShapesFunc	composite,
	  pixman_op_t		op,
	  PicturePtr		pSrc,
	  PicturePtr		pDst,
	  PictFormatPtr		maskFormat,
	  int16_t		xSrc,
	  int16_t		ySrc,
	  int16_t		xDst,
	  int16_t		yDst,
	  int			nshapes,
	  int			shape_size,
	  const uint8_t *	shapes)
{
    pixman_image_t *src, *dst;
    int src_xoff, src_yoff;
    int dst_xoff, dst_yoff;

    src = image_from_pict (pSrc, FALSE, &src_xoff, &src_yoff);
    dst = image_from_pict (pDst, TRUE, &dst_xoff, &dst_yoff);

    if (src && dst)
    {
	pixman_format_code_t format;

	if (!maskFormat)
	{
	    int i;

	    if (pDst->polyEdge == PolyEdgeSharp)
		format = PIXMAN_a1;
	    else
		format = PIXMAN_a8;

	    for (i = 0; i < nshapes; ++i)
	    {
		composite (op, src, dst, format,
			   xSrc + src_xoff,
			   ySrc + src_yoff,
			   xDst + dst_xoff,
			   yDst + dst_yoff,
			   1, shapes + i * shape_size);
	    }
	}
	else
	{
	    switch (PICT_FORMAT_A (maskFormat->format))
	    {
	    case 1:
		format = PIXMAN_a1;
		break;

	    case 4:
		format = PIXMAN_a4;
		break;

	    default:
	    case 8:
		format = PIXMAN_a8;
		break;
	    }
	    
	    composite (op, src, dst, format,
		       xSrc + src_xoff,
		       ySrc + src_yoff,
		       xDst + dst_xoff,
		       yDst + dst_yoff,
		       nshapes, shapes);
	}
    }

    free_pixman_pict (pSrc, src);
    free_pixman_pict (pDst, dst);
}

void
fbTrapezoids (CARD8	    op,
	      PicturePtr    pSrc,
	      PicturePtr    pDst,
	      PictFormatPtr maskFormat,
	      INT16	    xSrc,
	      INT16	    ySrc,
	      int	    ntrap,
	      xTrapezoid    *traps)
{
    int xDst, yDst;

    xDst = traps[0].left.p1.x >> 16;
    yDst = traps[0].left.p1.y >> 16;
    
    fbShapes ((CompositeShapesFunc)pixman_composite_trapezoids,
	      op, pSrc, pDst, maskFormat,
	      xSrc, ySrc, xDst, yDst,
	      ntrap, sizeof (xTrapezoid), (const uint8_t *)traps);
}

void
fbTriangles (CARD8	    op,
	     PicturePtr    pSrc,
	     PicturePtr    pDst,
	     PictFormatPtr maskFormat,
	     INT16	    xSrc,
	     INT16	    ySrc,
	     int	    ntris,
	     xTriangle    *tris)
{ 
    int xDst, yDst;

    xDst = tris[0].p1.x >> 16;
    yDst = tris[0].p1.y >> 16;
    
    fbShapes ((CompositeShapesFunc)pixman_composite_triangles,
	      op, pSrc, pDst, maskFormat,
	      xSrc, ySrc, xDst, yDst,
	      ntris, sizeof (xTriangle), (const uint8_t *)tris);
}
