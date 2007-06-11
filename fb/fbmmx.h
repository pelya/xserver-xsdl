/*
 * Copyright © 2004 Red Hat, Inc.
 * Copyright © 2005 Trolltech AS
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Author:  Søren Sandmann (sandmann@redhat.com)
 *          Lars Knoll (lars@trolltech.com)
 * 
 * Based on work by Owen Taylor
 */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef USE_MMX

#if !defined(__amd64__) && !defined(__x86_64__)
Bool fbHaveMMX(void);
#else
#define fbHaveMMX() TRUE
#endif

#else
#define fbHaveMMX() FALSE
#endif

#ifdef USE_MMX

Bool fbBltmmx (FbBits *src_bits,
	       FbBits *dst_bits,
	       FbStride src_stride,
	       FbStride dst_stride,
	       int src_bpp,
	       int dst_bpp,
	       int src_x, int src_y,
	       int dst_x, int dst_y,
	       int width, int height);
Bool fbCopyAreammx (DrawablePtr	pSrc,
		    DrawablePtr	pDst,
		    int		src_x,
		    int		src_y,
		    int		dst_x,
		    int		dst_y,
		    int		width,
		    int		height);
Bool fbSolidFillmmx (DrawablePtr	pDraw,
		     int		x,
		     int		y,
		     int		width,
		     int		height,
		     FbBits		xor);

#endif /* USE_MMX */
