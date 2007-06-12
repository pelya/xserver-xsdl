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

#endif /* RENDER */
#endif /* USE_MMX */
