/*
 * Copyright © 2004 Red Hat, Inc.
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
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Søren Sandmann (sandmann@redhat.com)
 * 
 * Based on work by Owen Taylor
 */

#include "fb.h"

#ifdef USE_GCC34_MMX

#ifdef RENDER

#include "picturestr.h"
#include "mipict.h"
#include "fbpict.h"

typedef int Vector1x64 __attribute__ ((mode(DI)));
typedef int Vector2x32 __attribute__ ((mode(V2SI)));
typedef int Vector4x16 __attribute__ ((mode(V4HI)));
typedef int Vector8x8  __attribute__ ((mode(V8QI)));

typedef unsigned long long ullong;

#define noVERBOSE

#ifdef VERBOSE
#define CHECKPOINT() ErrorF ("at %s %d\n", __FUNCTION__, __LINE__)
#else
#define CHECKPOINT()
#endif

typedef struct
{
    ullong mmx_zero;
    ullong mmx_4x00ff;
    ullong mmx_4x0080;
    ullong mmx_565_rgb;
    ullong mmx_565_unpack_multiplier;
    ullong mmx_565_r;
    ullong mmx_565_g;
    ullong mmx_565_b;
    ullong mmx_mask_0;
    ullong mmx_mask_1;
    ullong mmx_mask_2;
    ullong mmx_mask_3;
    ullong mmx_full_alpha;
    ullong mmx_ffff0000ffff0000;
    ullong mmx_0000ffff00000000;
    ullong mmx_000000000000ffff;
} MMXData;

static const MMXData c =
{
    .mmx_zero =				0x0000000000000000ULL,
    .mmx_4x00ff =			0x00ff00ff00ff00ffULL,
    .mmx_4x0080 =			0x0080008000800080ULL,
    .mmx_565_rgb =			0x000001f0003f001fULL,
    .mmx_565_r =			0x000000f800000000ULL,
    .mmx_565_g =			0x0000000000fc0000ULL,
    .mmx_565_b =			0x00000000000000f8ULL,
    .mmx_mask_0 =			0xffffffffffff0000ULL,
    .mmx_mask_1 =			0xffffffff0000ffffULL,
    .mmx_mask_2 =			0xffff0000ffffffffULL,
    .mmx_mask_3 =			0x0000ffffffffffffULL,
    .mmx_full_alpha =			0x00ff000000000000ULL,
    .mmx_565_unpack_multiplier =	0x0000008404100840ULL,
    .mmx_ffff0000ffff0000 =		0xffff0000ffff0000ULL,
    .mmx_0000ffff00000000 =		0x0000ffff00000000ULL,
    .mmx_000000000000ffff =		0x000000000000ffffULL,
};

static __inline__ Vector1x64
shift (Vector1x64 v, int s)
{
    if (s > 0)
	return __builtin_ia32_psllq (v, s);
    else if (s < 0)
	return __builtin_ia32_psrlq (v, -s);
    else
	return v;
}

static __inline__ Vector4x16
negate (Vector4x16 mask)
{
    return (Vector4x16)__builtin_ia32_pxor (
	(Vector1x64)mask,
	(Vector1x64)c.mmx_4x00ff);
}

static __inline__ Vector4x16
pix_multiply (Vector4x16 a, Vector4x16 b)
{
    Vector4x16 res;
    
    res = __builtin_ia32_pmullw (a, b);
    res = __builtin_ia32_paddw (res, (Vector4x16)c.mmx_4x0080);
    res = __builtin_ia32_psrlw (res, 8);
    
    return res;
}

#if 0
#define HAVE_PSHUFW
#endif

#ifdef HAVE_PSHUFW

static __inline__ Vector4x16
expand_alpha (Vector4x16 pixel)
{
    Vector4x16 result;
    __asm__ ("pshufw $0xFF, %1, %0\n\t" : "=y" (result) : "y" (pixel));
    return result;
}

static __inline__ Vector4x16
expand_alpha_rev (Vector4x16 pixel)
{
    Vector4x16 result;
    __asm__ ("pshufw $0x00, %1, %0\n\t" : "=y" (result) : "y" (pixel));
    return result;
}    

static __inline__ Vector4x16
invert_colors (Vector4x16 pixel)
{
    Vector4x16 result;

    /* 0xC6 = 11000110 */
    /*         3 0 1 2 */
    
    __asm__ ("pshufw $0xC6, %1, %0\n\t" : "=y" (result) : "y" (pixel));

    return result;
}

#else

static __inline__ Vector4x16
expand_alpha (Vector4x16 pixel)
{
    Vector1x64 t1, t2;

    t1 = shift ((Vector1x64)pixel, -48);
    t2 = shift (t1, 16);
    t1 = __builtin_ia32_por (t1, t2);
    t2 = shift (t1, 32);
    t1 = __builtin_ia32_por (t1, t2);

    return (Vector4x16)t1;
}

static __inline__ Vector4x16
expand_alpha_rev (Vector4x16 pixel)
{
    Vector1x64 t1, t2;

    t1 = shift ((Vector1x64)pixel,  48);
    t1 = shift (t1, -48);
    t2 = shift (t1, 16);
    t1 = __builtin_ia32_por (t1, t2);
    t2 = shift (t1, 32);
    t1 = __builtin_ia32_por (t1, t2);

    return (Vector4x16)t1;
}

static __inline__ Vector4x16
invert_colors (Vector4x16 pixel)
{
    Vector1x64 x, y, z;

    x = y = z = (Vector1x64)pixel;

    x = __builtin_ia32_pand (x, (Vector1x64)c.mmx_ffff0000ffff0000);
    y = __builtin_ia32_pand (y, (Vector1x64)c.mmx_000000000000ffff);
    z = __builtin_ia32_pand (z, (Vector1x64)c.mmx_0000ffff00000000);

    y = shift (y, 32);
    z = shift (z, -32);

    x = __builtin_ia32_por (x, y);
    x = __builtin_ia32_por (x, z);

    return (Vector4x16)x;
}

#endif

/* Notes about writing mmx code
 *
 * give memory operands as the second operand. If you give it as the
 * first, gcc will first load it into a register, then use that register
 *
 *   ie. use
 *
 *         __builtin_pmullw (x, mmx_constant[8]);
 *
 *   not
 *
 *         __builtin_pmullw (mmx_constant[8], x);
 *
 * Also try to minimize dependencies. Ie. when you need a value, try to calculate
 * it from a value that was calculated as early as possible.
 */

static __inline__ Vector4x16
over (Vector4x16 src, Vector4x16 srca, Vector4x16 dest)
{
    return (Vector4x16)__builtin_ia32_paddusb ((Vector8x8)src, (Vector8x8)pix_multiply(dest, negate(srca)));
}

static __inline__ Vector4x16
over_rev_non_pre (Vector4x16 src, Vector4x16 dest)
{
    Vector4x16 srca = expand_alpha (src);
    Vector4x16 srcfaaa = (Vector4x16)__builtin_ia32_por((Vector1x64)srca, (Vector1x64)c.mmx_full_alpha);

    return over(pix_multiply(invert_colors(src), srcfaaa), srca, dest);
}

static __inline__ Vector4x16
in (Vector4x16 src,
    Vector4x16 mask)
{
    return pix_multiply (src, mask);
}

static __inline__ Vector4x16
in_over (Vector4x16 src,
	 Vector4x16 srca,
	 Vector4x16 mask,
	 Vector4x16 dest)
{
    return over(in(src, mask), pix_multiply(srca, mask), dest);
}

static __inline__ Vector8x8
cvt32to64 (CARD32 v)
{
    ullong r = v;
    return (Vector8x8)r;
}

static __inline__ Vector4x16
load8888 (CARD32 v)
{
    return (Vector4x16)__builtin_ia32_punpcklbw (cvt32to64 (v),
						 (Vector8x8)c.mmx_zero);
}

static __inline__ Vector8x8
pack8888 (Vector4x16 lo, Vector4x16 hi)
{
    Vector8x8 r;
    r = __builtin_ia32_packuswb ((Vector4x16)lo, (Vector4x16)hi);
    return r;
}

/* Expand 16 bits positioned at @pos (0-3) of a mmx register into 00RR00GG00BB
   
--- Expanding 565 in the low word ---

m = (m << (32 - 3)) | (m << (16 - 5)) | m;
m = m & (01f0003f001f);
m = m * (008404100840);
m = m >> 8;

Note the trick here - the top word is shifted by another nibble to avoid
it bumping into the middle word
*/
static __inline__ Vector4x16
expand565 (Vector4x16 pixel, int pos)
{
    Vector1x64 p = (Vector1x64)pixel;
    
    /* move pixel to low 16 bit and zero the rest */
    p = shift (shift (p, (3 - pos) * 16), -48); 
    
    Vector1x64 t1 = shift (p, 36 - 11);
    Vector1x64 t2 = shift (p, 16 - 5);
    
    p = __builtin_ia32_por (t1, p);
    p = __builtin_ia32_por (t2, p);
    p = __builtin_ia32_pand (p, (Vector1x64)c.mmx_565_rgb);
    
    pixel = __builtin_ia32_pmullw ((Vector4x16)p, (Vector4x16)c.mmx_565_unpack_multiplier);
    return __builtin_ia32_psrlw (pixel, 8);
}

static __inline__ Vector4x16
expand8888 (Vector4x16 in, int pos)
{
    if (pos == 0)
	return (Vector4x16)__builtin_ia32_punpcklbw ((Vector8x8)in, (Vector8x8)c.mmx_zero);
    else
	return (Vector4x16)__builtin_ia32_punpckhbw ((Vector8x8)in, (Vector8x8)c.mmx_zero);
}

static __inline__ Vector4x16
pack565 (Vector4x16 pixel, Vector4x16 target, int pos)
{
    Vector1x64 p = (Vector1x64)pixel;
    Vector1x64 t = (Vector1x64)target;
    Vector1x64 r, g, b;
    
    r = __builtin_ia32_pand (p, (Vector1x64)c.mmx_565_r);
    g = __builtin_ia32_pand (p, (Vector1x64)c.mmx_565_g);
    b = __builtin_ia32_pand (p, (Vector1x64)c.mmx_565_b);
    
    r = shift (r, - (32 - 8) + pos * 16);
    g = shift (g, - (16 - 3) + pos * 16);
    b = shift (b, - (0  + 3) + pos * 16);

    if (pos == 0)
	t = __builtin_ia32_pand (t, (Vector1x64)c.mmx_mask_0);
    else if (pos == 1)
	t = __builtin_ia32_pand (t, (Vector1x64)c.mmx_mask_1);
    else if (pos == 2)
	t = __builtin_ia32_pand (t, (Vector1x64)c.mmx_mask_2);
    else if (pos == 3)
	t = __builtin_ia32_pand (t, (Vector1x64)c.mmx_mask_3);
    
    p = __builtin_ia32_por (r, t);
    p = __builtin_ia32_por (g, p);
    
    return (Vector4x16)__builtin_ia32_por (b, p);
}

static __inline__ void
emms (void)
{
    __asm__ __volatile__ ("emms");
}

void
fbCompositeSolid_nx8888mmx (CARD8	op,
			    PicturePtr pSrc,
			    PicturePtr pMask,
			    PicturePtr pDst,
			    INT16	xSrc,
			    INT16	ySrc,
			    INT16	xMask,
			    INT16	yMask,
			    INT16	xDst,
			    INT16	yDst,
			    CARD16	width,
			    CARD16	height)
{
    CARD32	src;
    CARD32	*dstLine, *dst;
    CARD16	w;
    FbStride	dstStride;
    Vector4x16	vsrc, vsrca;

    CHECKPOINT();
    
    fbComposeGetSolid(pSrc, src);
    
    if (src >> 24 == 0)
	return;
    
    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    
    vsrc = load8888 (src);
    vsrca = expand_alpha (vsrc);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	w = width;

	CHECKPOINT();
	
	while (w && (unsigned long)dst & 7)
	{
	    *dst = (ullong) pack8888(over(vsrc, vsrca, load8888(*dst)), (Vector4x16)c.mmx_zero);
	    
	    w--;
	    dst++;
	}

	while (w >= 2)
	{
	    Vector4x16 vdest;
	    Vector4x16 dest0, dest1;

	    vdest = *(Vector4x16 *)dst;
	    
	    dest0 = over(vsrc, vsrca, expand8888(vdest, 0));
	    dest1 = over(vsrc, vsrca, expand8888(vdest, 1));
	    
	    *(Vector8x8 *)dst = (Vector8x8)pack8888(dest0, dest1);
	    
	    dst += 2;
	    w -= 2;
	}

	CHECKPOINT();
	
	while (w)
	{
	    *dst = (ullong) pack8888(over(vsrc, vsrca, load8888(*dst)), (Vector4x16)c.mmx_zero);
	    
	    w--;
	    dst++;
	}
    }
    
    emms();
}

void
fbCompositeSolid_nx0565mmx (CARD8	op,
			    PicturePtr pSrc,
			    PicturePtr pMask,
			    PicturePtr pDst,
			    INT16	xSrc,
			    INT16	ySrc,
			    INT16	xMask,
			    INT16	yMask,
			    INT16	xDst,
			    INT16	yDst,
			    CARD16	width,
			    CARD16	height)
{
    CARD32	src;
    CARD16	*dstLine, *dst;
    CARD16	w;
    FbStride	dstStride;
    Vector4x16	vsrc, vsrca;

    CHECKPOINT();
    
    fbComposeGetSolid(pSrc, src);
    
    if (src >> 24 == 0)
	return;
    
    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);
    
    vsrc = load8888 (src);
    vsrca = expand_alpha (vsrc);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	w = width;

	CHECKPOINT();
	
	while (w && (unsigned long)dst & 7)
	{
	    ullong d = *dst;
	    Vector4x16 vdest = expand565 ((Vector4x16)d, 0);
	    vdest = pack565(over(vsrc, vsrca, vdest), vdest, 0);
	    *dst = (ullong)vdest;
	    
	    w--;
	    dst++;
	}

	while (w >= 4)
	{
	    Vector4x16 vdest;

	    vdest = *(Vector4x16 *)dst;
	    
	    vdest = pack565 (over(vsrc, vsrca, expand565(vdest, 0)), vdest, 0);
	    vdest = pack565 (over(vsrc, vsrca, expand565(vdest, 1)), vdest, 1);
	    vdest = pack565 (over(vsrc, vsrca, expand565(vdest, 2)), vdest, 2);
	    vdest = pack565 (over(vsrc, vsrca, expand565(vdest, 3)), vdest, 3);
	    
	    *(Vector8x8 *)dst = (Vector8x8)vdest;
	    
	    dst += 4;
	    w -= 4;
	}

	CHECKPOINT();
	
	while (w)
	{
	    ullong d = *dst;
	    Vector4x16 vdest = expand565 ((Vector4x16)d, 0);
	    vdest = pack565(over(vsrc, vsrca, vdest), vdest, 0);
	    *dst = (ullong)vdest;
	    
	    w--;
	    dst++;
	}
    }
    
    emms();
}

void
fbCompositeSolidMask_nx8888x8888Cmmx (CARD8	op,
				      PicturePtr pSrc,
				      PicturePtr pMask,
				      PicturePtr pDst,
				      INT16	xSrc,
				      INT16	ySrc,
				      INT16	xMask,
				      INT16	yMask,
				      INT16	xDst,
				      INT16	yDst,
				      CARD16	width,
				      CARD16	height)
{
    CARD32	src, srca;
    CARD32	*dstLine;
    CARD32	*maskLine;
    FbStride	dstStride, maskStride;
    Vector4x16	vsrc, vsrca;

    CHECKPOINT();
    
    fbComposeGetSolid(pSrc, src);
    
    srca = src >> 24;
    if (srca == 0)
	return;
    
    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD32, maskStride, maskLine, 1);
    
    vsrc = load8888(src);
    vsrca = expand_alpha(vsrc);
    
    while (height--)
    {
	int twidth = width;
	CARD32 *p = (CARD32 *)maskLine;
	CARD32 *q = (CARD32 *)dstLine;
	
	while (twidth && (unsigned long)q & 7)
	{
	    CARD32 m = *(CARD32 *)p;
	    
	    if (m)
	    {
		Vector4x16 vdest = load8888(*q);
		vdest = in_over(vsrc, vsrca, load8888(m), vdest);
		*q = (ullong)pack8888(vdest, (Vector4x16)c.mmx_zero);
	    }
	    
	    twidth--;
	    p++;
	    q++;
	}
	
	while (twidth >= 2)
	{
	    CARD32 m0, m1;
	    m0 = *p;
	    m1 = *(p + 1);
	    
	    if (m0 | m1)
	    {
		Vector4x16 dest0, dest1;
		Vector4x16 vdest = *(Vector4x16 *)q;
		
		dest0 = in_over(vsrc, vsrca, load8888(m0),
				expand8888 (vdest, 0));
		dest1 = in_over(vsrc, vsrca, load8888(m1),
				expand8888 (vdest, 1));
		
		*(Vector8x8 *)q = (Vector8x8)pack8888(dest0, dest1);
	    }
	    
	    p += 2;
	    q += 2;
	    twidth -= 2;
	}
	
	while (twidth)
	{
	    CARD32 m = *(CARD32 *)p;
	    
	    if (m)
	    {
		Vector4x16 vdest = load8888(*q);
		vdest = in_over(vsrc, vsrca, load8888(m), vdest);
		*q = (ullong)pack8888(vdest, (Vector4x16)c.mmx_zero);
	    }
	    
	    twidth--;
	    p++;
	    q++;
	}
	
	dstLine += dstStride;
	maskLine += maskStride;
    }
    
    emms();
}

void
fbCompositeSolidMask_nx8x8888mmx (CARD8      op,
				  PicturePtr pSrc,
				  PicturePtr pMask,
				  PicturePtr pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height)
{
    CARD32	src, srca;
    CARD32	*dstLine, *dst;
    CARD8	*maskLine, *mask;
    FbStride	dstStride, maskStride;
    CARD16	w;
    Vector4x16	vsrc, vsrca;
    ullong	srcsrc;
    
    CHECKPOINT();
    
    fbComposeGetSolid(pSrc, src);
    
    srca = src >> 24;
    if (srca == 0)
	return;

    srcsrc = (unsigned long long)src << 32 | src;
    
    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD8, maskStride, maskLine, 1);
    
    vsrc = load8888 (src);
    vsrca = expand_alpha (vsrc);
    
    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	CHECKPOINT();
	
	while (w && (unsigned long)dst & 7)
	{
	    ullong m = *mask;
	    
	    if (m)
	    {
		Vector4x16 vdest = in_over(vsrc, vsrca, expand_alpha_rev ((Vector4x16)m), load8888(*dst));
		*dst = (ullong)pack8888(vdest, (Vector4x16)c.mmx_zero);
	    }
	    
	    w--;
	    mask++;
	    dst++;
	}

	CHECKPOINT();
	
	while (w >= 2)
	{
	    ullong m0, m1;
	    m0 = *mask;
	    m1 = *(mask + 1);

	    if (srca == 0xff && (m0 & m1) == 0xff)
	    {
		*(unsigned long long *)dst = srcsrc;
	    }
	    else if (m0 | m1)
	    {
		Vector4x16 vdest;
		Vector4x16 dest0, dest1;

		vdest = *(Vector4x16 *)dst;
		
		dest0 = in_over(vsrc, vsrca, expand_alpha_rev ((Vector4x16)m0), expand8888(vdest, 0));
		dest1 = in_over(vsrc, vsrca, expand_alpha_rev ((Vector4x16)m1), expand8888(vdest, 1));
		
		*(Vector8x8 *)dst = (Vector8x8)pack8888(dest0, dest1);
	    }
	    
	    mask += 2;
	    dst += 2;
	    w -= 2;
	}

	CHECKPOINT();
	
	while (w)
	{
	    ullong m = *mask;
	    
	    if (m)
	    {
		Vector4x16 vdest = load8888(*dst);
		vdest = in_over(vsrc, vsrca, expand_alpha_rev ((Vector4x16)m), vdest);
		*dst = (ullong)pack8888(vdest, (Vector4x16)c.mmx_zero);
	    }
	    
	    w--;
	    mask++;
	    dst++;
	}
    }
    
    emms();
}


void
fbCompositeSolidMask_nx8x0565mmx (CARD8      op,
				  PicturePtr pSrc,
				  PicturePtr pMask,
				  PicturePtr pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height)
{
    CARD32	src, srca;
    CARD16	*dstLine, *dst;
    CARD8	*maskLine, *mask;
    FbStride	dstStride, maskStride;
    CARD16	w;
    Vector4x16	vsrc, vsrca;
    unsigned long long srcsrcsrcsrc, src16;
    
    CHECKPOINT();
    
    fbComposeGetSolid(pSrc, src);
    
    srca = src >> 24;
    if (srca == 0)
	return;
    
    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD8, maskStride, maskLine, 1);
    
    vsrc = load8888 (src);
    vsrca = expand_alpha (vsrc);

    src16 = (ullong)pack565(vsrc, (Vector4x16)c.mmx_zero, 0);

    srcsrcsrcsrc = (ullong)src16 << 48 | (ullong)src16 << 32 |
	(ullong)src16 << 16 | (ullong)src16;
    
    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	CHECKPOINT();
	
	while (w && (unsigned long)dst & 7)
	{
	    ullong m = *mask;
	    
	    if (m)
	    {
		ullong d = *dst;
		Vector4x16 vd = (Vector4x16)d;
		Vector4x16 vdest = in_over(vsrc, vsrca, expand_alpha_rev ((Vector4x16)m), expand565(vd, 0));
		*dst = (ullong)pack565(vdest, (Vector4x16)c.mmx_zero, 0);
	    }
	    
	    w--;
	    mask++;
	    dst++;
	}

	CHECKPOINT();
	
	while (w >= 4)
	{
	    ullong m0, m1, m2, m3;
	    m0 = *mask;
	    m1 = *(mask + 1);
	    m2 = *(mask + 2);
	    m3 = *(mask + 3);

	    if (srca == 0xff && (m0 & m1 & m2 & m3) == 0xff)
	    {
		*(unsigned long long *)dst = srcsrcsrcsrc;
	    }
	    else if (m0 | m1 | m2 | m3)
	    {
		Vector4x16 vdest;
		Vector4x16 vm0, vm1, vm2, vm3;

		vdest = *(Vector4x16 *)dst;

		vm0 = (Vector4x16)m0;
		vdest = pack565(in_over(vsrc, vsrca, expand_alpha_rev(vm0), expand565(vdest, 0)), vdest, 0);
		vm1 = (Vector4x16)m1;
		vdest = pack565(in_over(vsrc, vsrca, expand_alpha_rev(vm1), expand565(vdest, 1)), vdest, 1);
		vm2 = (Vector4x16)m2;
		vdest = pack565(in_over(vsrc, vsrca, expand_alpha_rev(vm2), expand565(vdest, 2)), vdest, 2);
		vm3 = (Vector4x16)m3;
		vdest = pack565(in_over(vsrc, vsrca, expand_alpha_rev(vm3), expand565(vdest, 3)), vdest, 3);
		
		*(Vector4x16 *)dst = vdest;
	    }
	    
	    w -= 4;
	    mask += 4;
	    dst += 4;
	}

	CHECKPOINT();
	
	while (w)
	{
	    ullong m = *mask;
	    
	    if (m)
	    {
		ullong d = *dst;
		Vector4x16 vd = (Vector4x16)d;
		Vector4x16 vdest = in_over(vsrc, vsrca, expand_alpha_rev ((Vector4x16)m), expand565(vd, 0));
		*dst = (ullong)pack565(vdest, (Vector4x16)c.mmx_zero, 0);
	    }
	    
	    w--;
	    mask++;
	    dst++;
	}
    }
    
    emms();
}

void
fbCompositeSrc_8888RevNPx0565mmx (CARD8      op,
				  PicturePtr pSrc,
				  PicturePtr pMask,
				  PicturePtr pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height)
{
    CARD16	*dstLine, *dst;
    CARD32	*srcLine, *src;
    FbStride	dstStride, srcStride;
    CARD16	w;
    
    CHECKPOINT();
    
    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);
    fbComposeGetStart (pSrc, xSrc, ySrc, CARD32, srcStride, srcLine, 1);

    assert (pSrc->pDrawable == pMask->pDrawable);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	CHECKPOINT();
	
	while (w && (unsigned long)dst & 7)
	{
	    Vector4x16 vsrc = load8888 (*src);
	    ullong d = *dst;
	    Vector4x16 vdest = expand565 ((Vector4x16)d, 0);
	    
	    vdest = pack565(over_rev_non_pre(vsrc, vdest), vdest, 0);
	    
	    *dst = (ullong)vdest;
	    
	    w--;
	    dst++;
	    src++;
	}

	CHECKPOINT();
	
	while (w >= 4)
	{
	    CARD32 s0, s1, s2, s3;
	    unsigned char a0, a1, a2, a3;

	    s0 = *src;
	    s1 = *(src + 1);
	    s2 = *(src + 2);
	    s3 = *(src + 3);

	    a0 = (s0 >> 24);
	    a1 = (s1 >> 24);
	    a2 = (s2 >> 24);
	    a3 = (s3 >> 24);
	    
	    if ((a0 & a1 & a2 & a3) == 0xFF)
	    {
		Vector4x16 vdest;
		vdest = pack565(invert_colors(load8888(s0)), (Vector4x16)c.mmx_zero, 0);
		vdest = pack565(invert_colors(load8888(s1)), vdest, 1);
		vdest = pack565(invert_colors(load8888(s2)), vdest, 2);
		vdest = pack565(invert_colors(load8888(s3)), vdest, 3);

		*(Vector4x16 *)dst = vdest;
	    }
	    else if (a0 | a1 | a2 | a3)
	    {
		Vector4x16 vdest = *(Vector4x16 *)dst;

		vdest = pack565(over_rev_non_pre(load8888(s0), expand565(vdest, 0)), vdest, 0);
	        vdest = pack565(over_rev_non_pre(load8888(s1), expand565(vdest, 1)), vdest, 1);
		vdest = pack565(over_rev_non_pre(load8888(s2), expand565(vdest, 2)), vdest, 2);
		vdest = pack565(over_rev_non_pre(load8888(s3), expand565(vdest, 3)), vdest, 3);

		*(Vector4x16 *)dst = vdest;
	    }
	    
	    w -= 4;
	    dst += 4;
	    src += 4;
	}

	CHECKPOINT();
	
	while (w)
	{
	    Vector4x16 vsrc = load8888 (*src);
	    ullong d = *dst;
	    Vector4x16 vdest = expand565 ((Vector4x16)d, 0);
	    
	    vdest = pack565(over_rev_non_pre(vsrc, vdest), vdest, 0);
	    
	    *dst = (ullong)vdest;
	    
	    w--;
	    dst++;
	    src++;
	}
    }

    emms();
}

/* "888RevNP" is GdkPixbuf's format: ABGR, non premultiplied */

void
fbCompositeSrc_8888RevNPx8888mmx (CARD8      op,
				  PicturePtr pSrc,
				  PicturePtr pMask,
				  PicturePtr pDst,
				  INT16      xSrc,
				  INT16      ySrc,
				  INT16      xMask,
				  INT16      yMask,
				  INT16      xDst,
				  INT16      yDst,
				  CARD16     width,
				  CARD16     height)
{
    CARD32	*dstLine, *dst;
    CARD32	*srcLine, *src;
    FbStride	dstStride, srcStride;
    CARD16	w;
    
    CHECKPOINT();
    
    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    fbComposeGetStart (pSrc, xSrc, ySrc, CARD32, srcStride, srcLine, 1);

    assert (pSrc->pDrawable == pMask->pDrawable);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w && (unsigned long)dst & 7)
	{
	    Vector4x16 s = load8888 (*src);
	    Vector4x16 d = load8888 (*dst);
	    
	    *dst = (ullong)pack8888 (over_rev_non_pre (s, d), (Vector4x16)c.mmx_zero);
	    
	    w--;
	    dst++;
	    src++;
	}

	while (w >= 2)
	{
	    ullong s0, s1;
	    unsigned char a0, a1;
	    Vector4x16 d0, d1;

	    s0 = *src;
	    s1 = *(src + 1);

	    a0 = (s0 >> 24);
	    a1 = (s1 >> 24);
	    
	    if ((a0 & a1) == 0xFF)
	    {
		d0 = invert_colors(load8888(s0));
		d1 = invert_colors(load8888(s1));

		*(Vector8x8 *)dst = pack8888 (d0, d1);
	    }
	    else if (a0 | a1)
	    {
		Vector4x16 vdest = *(Vector4x16 *)dst;

		d0 = over_rev_non_pre (load8888(s0), expand8888 (vdest, 0));
		d1 = over_rev_non_pre (load8888(s1), expand8888 (vdest, 1));
	    
		*(Vector8x8 *)dst = pack8888 (d0, d1);
	    }
	    
	    w -= 2;
	    dst += 2;
	    src += 2;
	}
	
	while (w)
	{
	    Vector4x16 s = load8888 (*src);
	    Vector4x16 d = load8888 (*dst);
	    
	    *dst = (ullong)pack8888 (over_rev_non_pre (s, d), (Vector4x16)c.mmx_zero);
	    
	    w--;
	    dst++;
	    src++;
	}
    }

    emms();
}

void
fbCompositeSolidMask_nx8888x0565Cmmx (CARD8      op,
				      PicturePtr pSrc,
				      PicturePtr pMask,
				      PicturePtr pDst,
				      INT16      xSrc,
				      INT16      ySrc,
				      INT16      xMask,
				      INT16      yMask,
				      INT16      xDst,
				      INT16      yDst,
				      CARD16     width,
				      CARD16     height)
{
    CARD32	src, srca;
    CARD16	*dstLine;
    CARD32	*maskLine;
    FbStride	dstStride, maskStride;
    Vector4x16  vsrc, vsrca;
    
    CHECKPOINT();
    
    fbComposeGetSolid(pSrc, src);
    
    srca = src >> 24;
    if (srca == 0)
	return;
    
    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD32, maskStride, maskLine, 1);
    
    vsrc = load8888 (src);
    vsrca = expand_alpha (vsrc);
    
    while (height--)
    {
	int twidth = width;
	CARD32 *p = (CARD32 *)maskLine;
	CARD16 *q = (CARD16 *)dstLine;
	
	while (twidth && ((unsigned long)q & 7))
	{
	    CARD32 m = *(CARD32 *)p;
	    
	    if (m)
	    {
		ullong d = *q;
		Vector4x16 vdest = expand565 ((Vector4x16)d, 0);
		vdest = pack565 (in_over (vsrc, vsrca, load8888 (m), vdest), vdest, 0);
		*q = (ullong)vdest;
	    }
	    
	    twidth--;
	    p++;
	    q++;
	}
	
	while (twidth >= 4)
	{
	    CARD32 m0, m1, m2, m3;
	    
	    m0 = *p;
	    m1 = *(p + 1);
	    m2 = *(p + 2);
	    m3 = *(p + 3);
	    
	    if ((m0 | m1 | m2 | m3))
	    {
		Vector4x16 vdest = *(Vector4x16 *)q;
		
		vdest = pack565(in_over(vsrc, vsrca, load8888(m0), expand565(vdest, 0)), vdest, 0);
		vdest = pack565(in_over(vsrc, vsrca, load8888(m1), expand565(vdest, 1)), vdest, 1);
		vdest = pack565(in_over(vsrc, vsrca, load8888(m2), expand565(vdest, 2)), vdest, 2);
		vdest = pack565(in_over(vsrc, vsrca, load8888(m3), expand565(vdest, 3)), vdest, 3);
		
		*(Vector4x16 *)q = vdest;
	    }
	    twidth -= 4;
	    p += 4;
	    q += 4;
	}
	
	while (twidth)
	{
	    CARD32 m;
	    
	    m = *(CARD32 *)p;
	    if (m)
	    {
		ullong d = *q;
		Vector4x16 vdest = expand565((Vector4x16)d, 0);
		vdest = pack565 (in_over(vsrc, vsrca, load8888(m), vdest), vdest, 0);
		*q = (ullong)vdest;
	    }
	    
	    twidth--;
	    p++;
	    q++;
	}
	
	maskLine += maskStride;
	dstLine += dstStride;
    }
    
    emms ();
}

void
fbCompositeSrcAdd_8000x8000mmx (CARD8	op,
				PicturePtr pSrc,
				PicturePtr pMask,
				PicturePtr pDst,
				INT16      xSrc,
				INT16      ySrc,
				INT16      xMask,
				INT16      yMask,
				INT16      xDst,
				INT16      yDst,
				CARD16     width,
				CARD16     height)
{
    CARD8	*dstLine, *dst;
    CARD8	*srcLine, *src;
    FbStride	dstStride, srcStride;
    CARD16	w;
    CARD8	s, d;
    CARD16	t;
    
    CHECKPOINT();
    
    fbComposeGetStart (pSrc, xSrc, ySrc, CARD8, srcStride, srcLine, 1);
    fbComposeGetStart (pDst, xDst, yDst, CARD8, dstStride, dstLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w && (unsigned long)dst & 7)
	{
	    s = *src;
	    d = *dst;
	    t = d + s;
	    s = t | (0 - (t >> 8));
	    *dst = s;
	    
	    dst++;
	    src++;
	    w--;
	}
	
	while (w >= 8)
	{
	    __asm__ __volatile__ (
		"movq (%0), %%mm2\n\t"
		"movq (%1), %%mm3\n\t"
		"paddusb %%mm2, %%mm3\n\t"
		"movq %%mm3, (%1)\n\t"
		: /* no output */ : "r" (src), "r" (dst));
	    
	    dst += 8;
	    src += 8;
	    w -= 8;
	}
	
	while (w)
	{
	    s = *src;
	    d = *dst;
	    t = d + s;
	    s = t | (0 - (t >> 8));
	    *dst = s;
	    
	    dst++;
	    src++;
	    w--;
	}
    }

    emms();
}

void
fbCompositeSrcAdd_8888x8888mmx (CARD8		op,
				PicturePtr	pSrc,
				PicturePtr	pMask,
				PicturePtr	 pDst,
				INT16		 xSrc,
				INT16      ySrc,
				INT16      xMask,
				INT16      yMask,
				INT16      xDst,
				INT16      yDst,
				CARD16     width,
				CARD16     height)
{
    CARD32	*dstLine, *dst;
    CARD32	*srcLine, *src;
    FbStride	dstStride, srcStride;
    CARD16	w;
    
    CHECKPOINT();
    
    fbComposeGetStart (pSrc, xSrc, ySrc, CARD32, srcStride, srcLine, 1);
    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    
    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;
	
	while (w && (unsigned long)dst & 7)
	{
	    __asm__ __volatile__ (
		"movd %0, %%mm2\n\t"
		"movd %1, %%mm3\n\t"
		"paddusb %%mm2, %%mm3\n\t"
		"movd %%mm3, %1\n\t"
		: /* no output */ : "m" (*src), "m" (*dst));
	    
	    dst++;
	    src++;
	    w--;
	}
	
	while (w >= 2)
	{
	    __asm__ __volatile__ (
		"movq (%0), %%mm2\n\t"
		"movq (%1), %%mm3\n\t"
		"paddusb %%mm2, %%mm3\n\t"
		"movq %%mm3, (%1)\n\t"
		: /* no output */ : "r" (src), "r" (dst));
	    
	    dst += 2;
	    src += 2;
	    w -= 2;
	}
	
	if (w)
	{
	    __asm__ __volatile__ (
		"movd %0, %%mm2\n\t"
		"movd %1, %%mm3\n\t"
		"paddusb %%mm2, %%mm3\n\t"
		"movd %%mm3, %1\n\t"
		: /* no output */ : "m" (*src), "m" (*dst));
	}
    }

    emms();
}

#define GetStart(drw,x,y,type,stride,line,bpp) {\
    FbBits	*__bits__;									\
    FbStride	__stride__;									\
    int		__xoff__,__yoff__;								\
												\
    fbGetDrawable((drw),__bits__,__stride__,bpp,__xoff__,__yoff__);				\
    (stride) = __stride__ * sizeof (FbBits) / sizeof (type);					\
    (line) = ((type *) __bits__) + (stride) * ((y) - __yoff__) + ((x) - __xoff__);		\
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
    ullong	fill;
    Vector8x8	vfill;
    CARD32	byte_width;
    CARD8	*byte_line;
    FbBits      *bits;
    int		xoff, yoff;
    
    CHECKPOINT();

    fbGetDrawable(pDraw, bits, stride, bpp, xoff, yoff);

    if (bpp == 16 && (xor >> 16 != (xor & 0xffff)))
	return FALSE;

    if (bpp != 16 && bpp != 32)
	return FALSE;
    
    if (bpp == 16)
    {
	stride = stride * sizeof (FbBits) / 2;
	byte_line = (CARD8 *)(((CARD16 *)bits) + stride * (y - yoff) + (x - xoff));
	byte_width = 2 * width;
	stride *= 2;
    }
    else
    {
	stride = stride * sizeof (FbBits) / 4;
	byte_line = (CARD8 *)(((CARD32 *)bits) + stride * (y - yoff) + (x - xoff));
	byte_width = 4 * width;
	stride *= 4;
    }

    fill = ((ullong)xor << 32) | xor;
    vfill = (Vector8x8)fill;
    
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
	
	while (w >= 4 && ((unsigned int)d & 7))
	{
	    *(CARD32 *)d = xor;

	    w -= 4;
	    d += 4;
	}
	
	while (w >= 64)
	{
	    __asm__ __volatile  (
		"movq %0, (%1)\n\t"
		"movq %0, 8(%1)\n\t"
		"movq %0, 16(%1)\n\t"
		"movq %0, 24(%1)\n\t"
		"movq %0, 32(%1)\n\t"
		"movq %0, 40(%1)\n\t"
		"movq %0, 48(%1)\n\t"
		"movq %0, 56(%1)\n\t"
		: /* no output */
		: "y" (vfill), "r" (d)
		: "memory");
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
    
    emms();
    return TRUE;
}

Bool
fbHaveMMX (void)
{
    static Bool initialized = FALSE;
    static Bool mmx_present;

    if (!initialized)
    {
	int tmp; /* static variables are accessed through %ebx,
		  * but we mess around with the registers below,
		  * so we need a temporary variable that can
		  * be accessed directly.
		  */
	
	__asm__ __volatile__ (
/* Check if bit 21 in flags word is writeable */

	    "pusha			        \n\t"
	    "pushfl				\n\t"
	    "popl	%%eax			\n\t"
	    "movl	%%eax, %%ebx		\n\t"
	    "xorl	$0x00200000, %%eax	\n\t"
	    "pushl	%%eax			\n\t"
	    "popfl				\n\t"
	    "pushfl				\n\t"
	    "popl	%%eax			\n\t"
	    
	    "cmpl	%%eax, %%ebx		\n\t"
	    
	    "je		.notfound		\n\t"
	    
/* OK, we have CPUID */
	    
	    "movl	$1, %%eax		\n\t"
	    "cpuid				\n\t"
	    
	    "test	$0x00800000, %%edx	\n\t"
	    "jz		.notfound		\n\t"
	    
	    "movl	$1, %0			\n\t"
	    "jmp	.out			\n\t"
	    
	    ".notfound:				\n\t"
	    "movl  	$0, %0			\n\t"
	    
	    ".out:				\n\t"
	    "popa			        \n\t"
	    :
	    "=m" (tmp)
	    : /* no input */);
	
	initialized = TRUE;

	mmx_present = tmp;
    }
    
    return mmx_present;
}


#endif /* RENDER */
#endif /* USE_GCC34_MMX */
