/*
 * $Xorg: cfbrrop.h,v 1.4 2001/02/09 02:04:38 xorgcvs Exp $
 *
Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
 *
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef GXcopy
#include "X.h"
#endif

#define RROP_FETCH_GC(gc) \
    RROP_FETCH_GCPRIV(((cfbPrivGCPtr)(gc)->devPrivates[cfbGCPrivateIndex].ptr))

#ifndef RROP
#define RROP GXset
#endif

#if RROP == GXcopy
#define RROP_DECLARE	register unsigned long	rrop_xor;
#define RROP_FETCH_GCPRIV(devPriv)  rrop_xor = (devPriv)->xor;
#define RROP_SOLID(dst)	    (*(dst) = (rrop_xor))
#define RROP_SOLID_MASK(dst,mask) (*(dst) = (*(dst) & ~(mask)) | ((rrop_xor) & (mask)))
#define RROP_NAME(prefix)   RROP_NAME_CAT(prefix,Copy)
#endif /* GXcopy */

#if RROP == GXxor
#define RROP_DECLARE	register unsigned long	rrop_xor;
#define RROP_FETCH_GCPRIV(devPriv)  rrop_xor = (devPriv)->xor;
#define RROP_SOLID(dst)	    (*(dst) ^= (rrop_xor))
#define RROP_SOLID_MASK(dst,mask) (*(dst) ^= ((rrop_xor) & (mask)))
#define RROP_NAME(prefix)   RROP_NAME_CAT(prefix,Xor)
#endif /* GXxor */

#if RROP == GXand
#define RROP_DECLARE	register unsigned long	rrop_and;
#define RROP_FETCH_GCPRIV(devPriv)  rrop_and = (devPriv)->and;
#define RROP_SOLID(dst)	    (*(dst) &= (rrop_and))
#define RROP_SOLID_MASK(dst,mask) (*(dst) &= ((rrop_and) | ~(mask)))
#define RROP_NAME(prefix)   RROP_NAME_CAT(prefix,And)
#endif /* GXand */

#if RROP == GXor
#define RROP_DECLARE	register unsigned long	rrop_or;
#define RROP_FETCH_GCPRIV(devPriv)  rrop_or = (devPriv)->xor;
#define RROP_SOLID(dst)	    (*(dst) |= (rrop_or))
#define RROP_SOLID_MASK(dst,mask) (*(dst) |= ((rrop_or) & (mask)))
#define RROP_NAME(prefix)   RROP_NAME_CAT(prefix,Or)
#endif /* GXor */

#if RROP == GXnoop
#define RROP_DECLARE
#define RROP_FETCH_GCPRIV(devPriv)
#define RROP_SOLID(dst)
#define RROP_SOLID_MASK(dst,mask)
#define RROP_NAME(prefix)   RROP_NAME_CAT(prefix,Noop)
#endif /* GXnoop */

#if RROP ==  GXset
#define RROP_DECLARE	    register unsigned long	rrop_and, rrop_xor;
#define RROP_FETCH_GCPRIV(devPriv)  rrop_and = (devPriv)->and; \
				    rrop_xor = (devPriv)->xor;
#define RROP_SOLID(dst)	    (*(dst) = DoRRop (*(dst), rrop_and, rrop_xor))
#define RROP_SOLID_MASK(dst,mask)   (*(dst) = DoMaskRRop (*(dst), rrop_and, rrop_xor, (mask)))
#define RROP_NAME(prefix)   RROP_NAME_CAT(prefix,General)
#endif /* GXset */

#define RROP_UNROLL_CASE1(p,i)    case (i): RROP_SOLID((p) - (i));
#define RROP_UNROLL_CASE2(p,i)    RROP_UNROLL_CASE1(p,(i)+1) RROP_UNROLL_CASE1(p,i)
#define RROP_UNROLL_CASE4(p,i)    RROP_UNROLL_CASE2(p,(i)+2) RROP_UNROLL_CASE2(p,i)
#define RROP_UNROLL_CASE8(p,i)    RROP_UNROLL_CASE4(p,(i)+4) RROP_UNROLL_CASE4(p,i)
#define RROP_UNROLL_CASE16(p,i)   RROP_UNROLL_CASE8(p,(i)+8) RROP_UNROLL_CASE8(p,i)
#define RROP_UNROLL_CASE3(p)	RROP_UNROLL_CASE2(p,2) RROP_UNROLL_CASE1(p,1)
#define RROP_UNROLL_CASE7(p)	RROP_UNROLL_CASE4(p,4) RROP_UNROLL_CASE3(p)
#define RROP_UNROLL_CASE15(p)	RROP_UNROLL_CASE8(p,8) RROP_UNROLL_CASE7(p)
#define RROP_UNROLL_CASE31(p)	RROP_UNROLL_CASE16(p,16) RROP_UNROLL_CASE15(p)
#ifdef LONG64
#define RROP_UNROLL_CASE63(p)	RROP_UNROLL_CASE32(p,32) RROP_UNROLL_CASE31(p)
#endif /* LONG64 */

#define RROP_UNROLL_LOOP1(p,i) RROP_SOLID((p) + (i));
#define RROP_UNROLL_LOOP2(p,i) RROP_UNROLL_LOOP1(p,(i)) RROP_UNROLL_LOOP1(p,(i)+1)
#define RROP_UNROLL_LOOP4(p,i) RROP_UNROLL_LOOP2(p,(i)) RROP_UNROLL_LOOP2(p,(i)+2)
#define RROP_UNROLL_LOOP8(p,i) RROP_UNROLL_LOOP4(p,(i)) RROP_UNROLL_LOOP4(p,(i)+4)
#define RROP_UNROLL_LOOP16(p,i) RROP_UNROLL_LOOP8(p,(i)) RROP_UNROLL_LOOP8(p,(i)+8)
#define RROP_UNROLL_LOOP32(p,i) RROP_UNROLL_LOOP16(p,(i)) RROP_UNROLL_LOOP16(p,(i)+16)
#ifdef LONG64
#define RROP_UNROLL_LOOP64(p,i) RROP_UNROLL_LOOP32(p,(i)) RROP_UNROLL_LOOP32(p,(i)+32)
#endif /* LONG64 */

#if defined (FAST_CONSTANT_OFFSET_MODE) && defined (SHARED_IDCACHE) && (RROP == GXcopy)

#ifdef LONG64
#define RROP_UNROLL_SHIFT	6
#define RROP_UNROLL_CASE(p)	RROP_UNROLL_CASE63(p)
#define RROP_UNROLL_LOOP(p)	RROP_UNROLL_LOOP64(p,-64)
#else /* not LONG64 */
#define RROP_UNROLL_SHIFT	5
#define RROP_UNROLL_CASE(p)	RROP_UNROLL_CASE31(p)
#define RROP_UNROLL_LOOP(p)	RROP_UNROLL_LOOP32(p,-32)
#endif /* LONG64 */
#define RROP_UNROLL		(1<<RROP_UNROLL_SHIFT)
#define RROP_UNROLL_MASK	(RROP_UNROLL-1)

#define RROP_SPAN(pdst,nmiddle) {\
    int part = (nmiddle) & RROP_UNROLL_MASK; \
    (nmiddle) >>= RROP_UNROLL_SHIFT; \
    (pdst) += part * (sizeof (unsigned long) / sizeof (*pdst)); \
    switch (part) {\
	RROP_UNROLL_CASE((unsigned long *) (pdst)) \
    } \
    while (--(nmiddle) >= 0) { \
	(pdst) += RROP_UNROLL * (sizeof (unsigned long) / sizeof (*pdst)); \
	RROP_UNROLL_LOOP((unsigned long *) (pdst)) \
    } \
}
#else
#define RROP_SPAN(pdst,nmiddle) \
    while (--(nmiddle) >= 0) { \
	RROP_SOLID((unsigned long *) (pdst)); \
	(pdst) += sizeof (unsigned long) / sizeof (*pdst); \
    }
#endif

#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define RROP_NAME_CAT(prefix,suffix)	prefix##suffix
#else
#define RROP_NAME_CAT(prefix,suffix)	prefix/**/suffix
#endif
