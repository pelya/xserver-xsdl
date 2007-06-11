/*
 * Copyright © 2004, 2005 Red Hat, Inc.
 * Copyright © 2004 Nicholas Miell
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
 * Minor Improvements: Nicholas Miell (nmiell@gmail.com)
 * MMX code paths for fbcompose.c by Lars Knoll (lars@trolltech.com) 
 *
 * Based on work by Owen Taylor
 */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef USE_MMX

#if defined(__amd64__) || defined(__x86_64__)
#define USE_SSE
#endif

#include <mmintrin.h>
#ifdef USE_SSE
#include <xmmintrin.h> /* for _mm_shuffle_pi16 and _MM_SHUFFLE */
#endif

#ifdef RENDER

#include "fb.h"
#include "fbmmx.h"

#include "picturestr.h"
#include "mipict.h"
#include "fbpict.h"

#define noVERBOSE

#ifdef VERBOSE
#define CHECKPOINT() ErrorF ("at %s %d\n", __FUNCTION__, __LINE__)
#else
#define CHECKPOINT()
#endif


typedef unsigned long long ullong;

#ifdef __GNUC__
typedef ullong mmxdatafield;
#endif
#ifdef _MSC_VER
typedef unsigned __int64 ullong;
typedef __m64 mmxdatafield;
#endif

Bool
fbFillmmx (FbBits *bits,
	   FbStride stride,
	   int bpp,
	   int x,
	   int y,
	   int width,
	   int height,
	   FbBits xor)
{
    ullong	fill;
    __m64	vfill;
    CARD32	byte_width;
    CARD8	*byte_line;
#ifdef __GNUC__
    __m64	v1, v2, v3, v4, v5, v6, v7;
#endif
    
    if (bpp == 16 && (xor >> 16 != (xor & 0xffff)))
	return FALSE;
    
    if (bpp != 16 && bpp != 32)
	return FALSE;
    
    if (bpp == 16)
    {
	stride = stride * sizeof (FbBits) / 2;
	byte_line = (CARD8 *)(((CARD16 *)bits) + stride * y + x);
	byte_width = 2 * width;
	stride *= 2;
    }
    else
    {
	stride = stride * sizeof (FbBits) / 4;
	byte_line = (CARD8 *)(((CARD32 *)bits) + stride * y + x);
	byte_width = 4 * width;
	stride *= 4;
    }
    
    fill = ((ullong)xor << 32) | xor;
    vfill = (__m64)fill;
    
#ifdef __GNUC__
    __asm__ (
	"movq		%7,	%0\n"
	"movq		%7,	%1\n"
	"movq		%7,	%2\n"
	"movq		%7,	%3\n"
	"movq		%7,	%4\n"
	"movq		%7,	%5\n"
	"movq		%7,	%6\n"
	: "=y" (v1), "=y" (v2), "=y" (v3),
	  "=y" (v4), "=y" (v5), "=y" (v6), "=y" (v7)
	: "y" (vfill));
#endif
    
    while (height--)
    {
	int w;
	CARD8 *d = byte_line;
	byte_line += stride;
	w = byte_width;
	
	while (w >= 2 && ((unsigned long)d & 3))
	{
	    *(CARD16 *)d = xor;
	    w -= 2;
	    d += 2;
	}
	
	while (w >= 4 && ((unsigned long)d & 7))
	{
	    *(CARD32 *)d = xor;
	    
	    w -= 4;
	    d += 4;
	}

	while (w >= 64)
	{
#ifdef __GNUC__
	    __asm__ (
		"movq	%1,	  (%0)\n"
		"movq	%2,	 8(%0)\n"
		"movq	%3,	16(%0)\n"
		"movq	%4,	24(%0)\n"
		"movq	%5,	32(%0)\n"
		"movq	%6,	40(%0)\n"
		"movq	%7,	48(%0)\n"
		"movq	%8,	56(%0)\n"
		:
		: "r" (d),
		  "y" (vfill), "y" (v1), "y" (v2), "y" (v3),
		  "y" (v4), "y" (v5), "y" (v6), "y" (v7)
		: "memory");
#else
	    *(__m64*) (d +  0) = vfill;
	    *(__m64*) (d +  8) = vfill;
	    *(__m64*) (d + 16) = vfill;
	    *(__m64*) (d + 24) = vfill;
	    *(__m64*) (d + 32) = vfill;
	    *(__m64*) (d + 40) = vfill;
	    *(__m64*) (d + 48) = vfill;
	    *(__m64*) (d + 56) = vfill;
#endif    
	    w -= 64;
	    d += 64;
	}
	
	while (w >= 4)
	{
	    *(CARD32 *)d = xor;
	    
	    w -= 4;
	    d += 4;
	}
	if (w >= 2)
	{
	    *(CARD16 *)d = xor;
	    w -= 2;
	    d += 2;
	}
    }
    
    _mm_empty();
    return TRUE;
}

Bool
fbSolidFillmmx (DrawablePtr	pDraw,
		int		x,
		int		y,
		int		width,
		int		height,
		FbBits		xor)
{ 
    FbStride	stride;
    int		bpp;
    FbBits      *bits;
    int		xoff, yoff;
    
    CHECKPOINT();
    
    fbGetDrawable(pDraw, bits, stride, bpp, xoff, yoff);

    return fbFillmmx (bits, stride, bpp, x + xoff, y + yoff, width, height, xor);
}

Bool
fbBltmmx (FbBits *src_bits,
	  FbBits *dst_bits,
	  FbStride src_stride,
	  FbStride dst_stride,
	  int src_bpp,
	  int dst_bpp,
	  int src_x, int src_y,
	  int dst_x, int dst_y,
	  int width, int height)
{
    CARD8 *	src_bytes;
    CARD8 *	dst_bytes;
    int		byte_width;
    
    if (src_bpp != dst_bpp)
	return FALSE;
    
    if (src_bpp == 16)
    {
	src_stride = src_stride * sizeof (FbBits) / 2;
	dst_stride = dst_stride * sizeof (FbBits) / 2;
	src_bytes = (CARD8 *)(((CARD16 *)src_bits) + src_stride * (src_y) + (src_x));
	dst_bytes = (CARD8 *)(((CARD16 *)dst_bits) + dst_stride * (dst_y) + (dst_x));
	byte_width = 2 * width;
	src_stride *= 2;
	dst_stride *= 2;
    } else if (src_bpp == 32) {
	src_stride = src_stride * sizeof (FbBits) / 4;
	dst_stride = dst_stride * sizeof (FbBits) / 4;
	src_bytes = (CARD8 *)(((CARD32 *)src_bits) + src_stride * (src_y) + (src_x));
	dst_bytes = (CARD8 *)(((CARD32 *)dst_bits) + dst_stride * (dst_y) + (dst_x));
	byte_width = 4 * width;
	src_stride *= 4;
	dst_stride *= 4;
    } else {
	return FALSE;
    }

    while (height--)
    {
	int w;
	CARD8 *s = src_bytes;
	CARD8 *d = dst_bytes;
	src_bytes += src_stride;
	dst_bytes += dst_stride;
	w = byte_width;
	
	while (w >= 2 && ((unsigned long)d & 3))
	{
	    *(CARD16 *)d = *(CARD16 *)s;
	    w -= 2;
	    s += 2;
	    d += 2;
	}
	
	while (w >= 4 && ((unsigned long)d & 7))
	{
	    *(CARD32 *)d = *(CARD32 *)s;
	    
	    w -= 4;
	    s += 4;
	    d += 4;
	}
	
	while (w >= 64)
	{
#ifdef __GNUC__
	    __asm__ (
		"movq	  (%1),	  %%mm0\n"
		"movq	 8(%1),	  %%mm1\n"
		"movq	16(%1),	  %%mm2\n"
		"movq	24(%1),	  %%mm3\n"
		"movq	32(%1),	  %%mm4\n"
		"movq	40(%1),	  %%mm5\n"
		"movq	48(%1),	  %%mm6\n"
		"movq	56(%1),	  %%mm7\n"

		"movq	%%mm0,	  (%0)\n"
		"movq	%%mm1,	 8(%0)\n"
		"movq	%%mm2,	16(%0)\n"
		"movq	%%mm3,	24(%0)\n"
		"movq	%%mm4,	32(%0)\n"
		"movq	%%mm5,	40(%0)\n"
		"movq	%%mm6,	48(%0)\n"
		"movq	%%mm7,	56(%0)\n"
		:
		: "r" (d), "r" (s)
		: "memory",
		  "%mm0", "%mm1", "%mm2", "%mm3",
		  "%mm4", "%mm5", "%mm6", "%mm7");
#else
	    __m64 v0 = *(__m64 *)(s + 0);
	    __m64 v1 = *(__m64 *)(s + 8);
	    __m64 v2 = *(__m64 *)(s + 16);
	    __m64 v3 = *(__m64 *)(s + 24);
	    __m64 v4 = *(__m64 *)(s + 32);
	    __m64 v5 = *(__m64 *)(s + 40);
	    __m64 v6 = *(__m64 *)(s + 48);
	    __m64 v7 = *(__m64 *)(s + 56);
	    *(__m64 *)(d + 0)  = v0;
	    *(__m64 *)(d + 8)  = v1;
	    *(__m64 *)(d + 16) = v2;
	    *(__m64 *)(d + 24) = v3;
	    *(__m64 *)(d + 32) = v4;
	    *(__m64 *)(d + 40) = v5;
	    *(__m64 *)(d + 48) = v6;
	    *(__m64 *)(d + 56) = v7;
#endif	    
	    
	    w -= 64;
	    s += 64;
	    d += 64;
	}
	while (w >= 4)
	{
	    *(CARD32 *)d = *(CARD32 *)s;

	    w -= 4;
	    s += 4;
	    d += 4;
	}
	if (w >= 2)
	{
	    *(CARD16 *)d = *(CARD16 *)s;
	    w -= 2;
	    s += 2;
	    d += 2;
	}
    }
    
    _mm_empty();

    return TRUE;
}

Bool
fbCopyAreammx (DrawablePtr	pSrc,
	       DrawablePtr	pDst,
	       int		src_x,
	       int		src_y,
	       int		dst_x,
	       int		dst_y,
	       int		width,
	       int		height)
{
    FbBits *	src_bits;
    FbStride	src_stride;
    int		src_bpp;
    int		src_xoff;
    int		src_yoff;

    FbBits *	dst_bits;
    FbStride	dst_stride;
    int		dst_bpp;
    int		dst_xoff;
    int		dst_yoff;
    
    fbGetDrawable(pSrc, src_bits, src_stride, src_bpp, src_xoff, src_yoff);
    fbGetDrawable(pDst, dst_bits, dst_stride, dst_bpp, dst_xoff, dst_yoff);

    return fbBltmmx (src_bits, dst_bits, src_stride, dst_stride, src_bpp, dst_bpp,
		     src_x + src_xoff, src_y + src_yoff,
		     dst_x + dst_xoff, dst_y + dst_yoff,
		     width, height);
}

#endif /* RENDER */
#endif /* USE_MMX */
