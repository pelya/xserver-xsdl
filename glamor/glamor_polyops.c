/*
 * Copyright © 2009 Intel Corporation
 * Copyright © 1998 Keith Packard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glamor_priv.h"

static Bool
_glamor_poly_point(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		   DDXPointPtr ppt, Bool fallback)
{
	if (!fallback 
	    && glamor_ddx_fallback_check_gc(pGC)
	    && glamor_ddx_fallback_check_pixmap(pDrawable))
		goto fail;
	glamor_prepare_access_gc(pGC);
	glamor_prepare_access(pDrawable, GLAMOR_ACCESS_RW);
	fbPolyPoint(pDrawable, pGC, mode, npt, ppt);
	glamor_finish_access(pDrawable, GLAMOR_ACCESS_RW);
	glamor_finish_access_gc(pGC);
	return TRUE;

fail:
	return FALSE;
}

void
glamor_poly_point(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr ppt)
{
	_glamor_poly_point(pDrawable, pGC, mode, npt, ppt, TRUE);
}

Bool
glamor_poly_point_nf(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		     DDXPointPtr ppt)
{
	return _glamor_poly_point(pDrawable, pGC, mode, npt, ppt, FALSE);
}

static Bool
_glamor_poly_segment(DrawablePtr pDrawable, GCPtr pGC, int nseg,
		     xSegment *pSeg, Bool fallback)
{
	if (!fallback 
	    && glamor_ddx_fallback_check_gc(pGC)
	    && glamor_ddx_fallback_check_pixmap(pDrawable))
		goto fail;
	/* For lineWidth is not zero, fb calls to mi functions. */
	if (pGC->lineWidth == 0) {
		glamor_prepare_access_gc(pGC);
		glamor_prepare_access(pDrawable, GLAMOR_ACCESS_RW);
		fbPolySegment(pDrawable, pGC, nseg, pSeg);
		glamor_finish_access(pDrawable, GLAMOR_ACCESS_RW);
		glamor_finish_access_gc(pGC);
	} else
	fbPolySegment(pDrawable, pGC, nseg, pSeg);

	return TRUE;

fail:
	return FALSE;
}

void
glamor_poly_segment(DrawablePtr pDrawable, GCPtr pGC, int nseg,
		    xSegment *pSeg)
{
	_glamor_poly_segment(pDrawable, pGC, nseg, pSeg, TRUE);
}

Bool
glamor_poly_segment_nf(DrawablePtr pDrawable, GCPtr pGC, int nseg,
		     xSegment *pSeg)
{
	return _glamor_poly_segment(pDrawable, pGC, nseg, pSeg, FALSE);
}

static Bool
_glamor_poly_line(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr ppt, Bool fallback)
{
	if (!fallback 
	    && glamor_ddx_fallback_check_gc(pGC)
	    && glamor_ddx_fallback_check_pixmap(pDrawable))
		goto fail;
	/* For lineWidth is not zero, fb calls to mi functions. */
	if (pGC->lineWidth == 0) {
		glamor_prepare_access_gc(pGC);
		glamor_prepare_access(pDrawable, GLAMOR_ACCESS_RW);
		fbPolyLine(pDrawable, pGC, mode, npt, ppt);
		glamor_finish_access(pDrawable, GLAMOR_ACCESS_RW);
		glamor_finish_access_gc(pGC);
	} else
	fbPolyLine(pDrawable, pGC, mode, npt, ppt);

	return TRUE;

fail:
	return FALSE;
}

void
glamor_poly_line(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		 DDXPointPtr ppt)
{
	_glamor_poly_line(pDrawable, pGC, mode, npt, ppt, TRUE);
}

Bool
glamor_poly_line_nf(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		    DDXPointPtr ppt)
{
	return _glamor_poly_line(pDrawable, pGC, mode, npt, ppt, FALSE);
}

