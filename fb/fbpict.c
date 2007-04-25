/*
 *
 * Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>

#include "fb.h"

#ifdef RENDER

#include "picturestr.h"
#include "mipict.h"
#include "fbpict.h"
#include "fbmmx.h"

typedef void	(*CompositeFunc) (CARD8      op,
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
				  CARD16     height);

CARD32
fbOver (CARD32 x, CARD32 y)
{
    CARD16  a = ~x >> 24;
    CARD16  t;
    CARD32  m,n,o,p;

    m = FbOverU(x,y,0,a,t);
    n = FbOverU(x,y,8,a,t);
    o = FbOverU(x,y,16,a,t);
    p = FbOverU(x,y,24,a,t);
    return m|n|o|p;
}

CARD32
fbOver24 (CARD32 x, CARD32 y)
{
    CARD16  a = ~x >> 24;
    CARD16  t;
    CARD32  m,n,o;

    m = FbOverU(x,y,0,a,t);
    n = FbOverU(x,y,8,a,t);
    o = FbOverU(x,y,16,a,t);
    return m|n|o;
}

CARD32
fbIn (CARD32 x, CARD8 y)
{
    CARD16  a = y;
    CARD16  t;
    CARD32  m,n,o,p;

    m = FbInU(x,0,a,t);
    n = FbInU(x,8,a,t);
    o = FbInU(x,16,a,t);
    p = FbInU(x,24,a,t);
    return m|n|o|p;
}

#define genericCombine24(a,b,c,d) (((a)*(c)+(b)*(d)))

/*
 * This macro does src IN mask OVER dst when src and dst are 0888.
 * If src has alpha, this will not work
 */
#define inOver0888(alpha, source, destval, dest) { \
 	CARD32 dstrb=destval&0xFF00FF; CARD32 dstag=(destval>>8)&0xFF00FF; \
 	CARD32 drb=((source&0xFF00FF)-dstrb)*alpha; CARD32 dag=(((source>>8)&0xFF00FF)-dstag)*alpha; \
	WRITE(dest, ((((drb>>8) + dstrb) & 0x00FF00FF) | ((((dag>>8) + dstag) << 8) & 0xFF00FF00))); \
    }

/*
 * This macro does src IN mask OVER dst when src and dst are 0565 and
 * mask is a 5-bit alpha value.  Again, if src has alpha, this will not
 * work.
 */
#define inOver0565(alpha, source, destval, dest) { \
 	CARD16 dstrb = destval & 0xf81f; CARD16 dstg  = destval & 0x7e0; \
 	CARD32 drb = ((source&0xf81f)-dstrb)*alpha; CARD32 dg=((source & 0x7e0)-dstg)*alpha; \
	WRITE(dest, ((((drb>>5) + dstrb)&0xf81f) | (((dg>>5)  + dstg) & 0x7e0))); \
    }


#define inOver2x0565(alpha, source, destval, dest) { \
 	CARD32 dstrb = destval & 0x07e0f81f; CARD32 dstg  = (destval & 0xf81f07e0)>>5; \
 	CARD32 drb = ((source&0x07e0f81f)-dstrb)*alpha; CARD32 dg=(((source & 0xf81f07e0)>>5)-dstg)*alpha; \
	WRITE(dest, ((((drb>>5) + dstrb)&0x07e0f81f) | ((((dg>>5)  + dstg)<<5) & 0xf81f07e0))); \
    }


#if IMAGE_BYTE_ORDER == LSBFirst
#define setupPackedReader(count,temp,where,workingWhere,workingVal) count=(long)where; \
					temp=count&3; \
					where-=temp; \
					workingWhere=(CARD32 *)where; \
                                        workingVal=READ(workingWhere++); \
					count=4-temp; \
					workingVal>>=(8*temp)
        #define readPacked(where,x,y,z) {if(!(x)) { (x)=4; y = READ(z++); } where=(y)&0xff; (y)>>=8; (x)--;}
	#define readPackedSource(where) readPacked(where,ws,workingSource,wsrc)
	#define readPackedDest(where) readPacked(where,wd,workingiDest,widst)
        #define writePacked(what) workingoDest>>=8; workingoDest|=(what<<24); ww--; if(!ww) { ww=4; WRITE (wodst++, workingoDest); } 
#else
	#warning "I havn't tested fbCompositeTrans_0888xnx0888() on big endian yet!"
	#define setupPackedReader(count,temp,where,workingWhere,workingVal) count=(long)where; \
					temp=count&3; \
					where-=temp; \
					workingWhere=(CARD32 *)where; \
                                        workingVal=READ(workingWhere)++; \
					count=4-temp; \
					workingVal<<=(8*temp)
        #define readPacked(where,x,y,z) {if(!(x)) { (x)=4; y = READ(z++); } where=(y)>>24; (y)<<=8; (x)--;}
	#define readPackedSource(where) readPacked(where,ws,workingSource,wsrc)
	#define readPackedDest(where) readPacked(where,wd,workingiDest,widst)
        #define writePacked(what) workingoDest<<=8; workingoDest|=what; ww--; if(!ww) { ww=4; WRITE(wodst++, workingoDest); } 
#endif

/*
 * Naming convention:
 *
 *  opSRCxMASKxDST
 */

void
fbCompositeSolidMask_nx8x8888 (CARD8      op,
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
    CARD32	*dstLine, *dst, d, dstMask;
    CARD8	*maskLine, *mask, m;
    FbStride	dstStride, maskStride;
    CARD16	w;

    fbComposeGetSolid(pSrc, src, pDst->format);

    dstMask = FbFullMask (pDst->pDrawable->depth);
    srca = src >> 24;
    if (src == 0)
	return;

    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD8, maskStride, maskLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	while (w--)
	{
	    m = READ(mask++);
	    if (m == 0xff)
	    {
		if (srca == 0xff)
		    WRITE(dst, src & dstMask);
		else
		    WRITE(dst, fbOver (src, READ(dst)) & dstMask);
	    }
	    else if (m)
	    {
		d = fbIn (src, m);
		WRITE(dst, fbOver (d, READ(dst)) & dstMask);
	    }
	    dst++;
	}
    }

    fbFinishAccess (pMask->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

void
fbCompositeSolidMask_nx8888x8888C (CARD8      op,
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
    CARD32	*dstLine, *dst, d, dstMask;
    CARD32	*maskLine, *mask, ma;
    FbStride	dstStride, maskStride;
    CARD16	w;
    CARD32	m, n, o, p;

    fbComposeGetSolid(pSrc, src, pDst->format);

    dstMask = FbFullMask (pDst->pDrawable->depth);
    srca = src >> 24;
    if (src == 0)
	return;

    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD32, maskStride, maskLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	while (w--)
	{
	    ma = READ(mask++);
	    if (ma == 0xffffffff)
	    {
		if (srca == 0xff)
		    WRITE(dst, src & dstMask);
		else
		    WRITE(dst, fbOver (src, READ(dst)) & dstMask);
	    }
	    else if (ma)
	    {
		d = READ(dst);
#define FbInOverC(src,srca,msk,dst,i,result) { \
    CARD16  __a = FbGet8(msk,i); \
    CARD32  __t, __ta; \
    CARD32  __i; \
    __t = FbIntMult (FbGet8(src,i), __a,__i); \
    __ta = (CARD8) ~FbIntMult (srca, __a,__i); \
    __t = __t + FbIntMult(FbGet8(dst,i),__ta,__i); \
    __t = (CARD32) (CARD8) (__t | (-(__t >> 8))); \
    result = __t << (i); \
}
		FbInOverC (src, srca, ma, d, 0, m);
		FbInOverC (src, srca, ma, d, 8, n);
		FbInOverC (src, srca, ma, d, 16, o);
		FbInOverC (src, srca, ma, d, 24, p);
		WRITE(dst, m|n|o|p);
	    }
	    dst++;
	}
    }

    fbFinishAccess (pMask->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

#define srcAlphaCombine24(a,b) genericCombine24(a,b,srca,srcia)
void
fbCompositeSolidMask_nx8x0888 (CARD8      op,
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
    CARD32	src, srca, srcia;
    CARD8	*dstLine, *dst, *edst;
    CARD8	*maskLine, *mask, m;
    FbStride	dstStride, maskStride;
    CARD16	w;
    CARD32 rs,gs,bs,rd,gd,bd;

    fbComposeGetSolid(pSrc, src, pDst->format);

    srca = src >> 24;
    srcia = 255-srca;
    if (src == 0)
	return;

    rs=src&0xff;
    gs=(src>>8)&0xff;
    bs=(src>>16)&0xff;
      
    fbComposeGetStart (pDst, xDst, yDst, CARD8, dstStride, dstLine, 3);
    fbComposeGetStart (pMask, xMask, yMask, CARD8, maskStride, maskLine, 1);

    while (height--)
    {
	/* fixme: cleanup unused */
	unsigned long wt, wd;
	CARD32 workingiDest;
	CARD32 *widst;
 	
	edst = dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;
 	
#ifndef NO_MASKED_PACKED_READ
	setupPackedReader(wd,wt,edst,widst,workingiDest);
#endif
 	
	while (w--)
	{
#ifndef NO_MASKED_PACKED_READ
	    readPackedDest(rd);
	    readPackedDest(gd);
	    readPackedDest(bd);
#else
	    rd = READ(edst++);
	    gd = READ(edst++);
	    bd = READ(edst++);
#endif
	    m = READ(mask++);
	    if (m == 0xff)
	    {
		if (srca == 0xff)
		{
		    WRITE(dst++, rs);
		    WRITE(dst++, gs);
		    WRITE(dst++, bs);
		}
		else
		{
		    WRITE(dst++, (srcAlphaCombine24(rs, rd)>>8));
		    WRITE(dst++, (srcAlphaCombine24(gs, gd)>>8));
		    WRITE(dst++, (srcAlphaCombine24(bs, bd)>>8));
		}
	    }
	    else if (m)
	    {
		int na=(srca*(int)m)>>8;
		int nia=255-na;
		WRITE(dst++, (genericCombine24(rs, rd, na, nia)>>8));
		WRITE(dst++, (genericCombine24(gs, gd, na, nia)>>8));
		WRITE(dst++, (genericCombine24(bs, bd, na, nia)>>8));
	    }
	    else
	    {
		dst+=3;
	    }
	}
    }
    
    fbFinishAccess (pMask->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

void
fbCompositeSolidMask_nx8x0565 (CARD8      op,
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
    CARD32	src, srca8, srca5;
    CARD16	*dstLine, *dst;
    CARD16	d;
    CARD32	t;
    CARD8	*maskLine, *mask, m;
    FbStride	dstStride, maskStride;
    CARD16	w,src16;
    
    fbComposeGetSolid(pSrc, src, pDst->format);
    
    if (src == 0)
	return;
      
    srca8 = (src >> 24);
    srca5 = (srca8 >> 3);
    src16 = cvt8888to0565(src);
     
    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD8, maskStride, maskLine, 1);
    
    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;
 
	while (w--)
	{
	    m = READ(mask++);
	    if (m == 0)
		dst++;
	    else if (srca5 == (0xff >> 3))
	    {
		if (m == 0xff)
		    WRITE(dst++, src16);
		else 
 		{
		    d = READ(dst);
		    m >>= 3;
		    inOver0565 (m, src16, d, dst++);
 		}
	    }
	    else
	    {
		d = READ(dst);
		if (m == 0xff) 
		{
		    t = fbOver24 (src, cvt0565to0888 (d));
		}
		else
		{
		    t = fbIn (src, m);
		    t = fbOver (t, cvt0565to0888 (d));
		}
		WRITE(dst++, cvt8888to0565 (t));
	    }
	}
    }
    
    fbFinishAccess (pMask->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

static void
fbCompositeSolidMask_nx8888x0565 (CARD8      op,
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
    CARD32	src, srca8, srca5;
    CARD16	*dstLine, *dst;
    CARD16	d;
    CARD32	*maskLine, *mask;
    CARD32	t;
    CARD8	m;
    FbStride	dstStride, maskStride;
    CARD16	w, src16;

    fbComposeGetSolid(pSrc, src, pDst->format);

    if (src == 0)
	return;

    srca8 = src >> 24;
    srca5 = srca8 >> 3;
    src16 = cvt8888to0565(src);

    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD32, maskStride, maskLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	while (w--)
	{
	    m = READ(mask++) >> 24;
	    if (m == 0)
		dst++;
	    else if (srca5 == (0xff >> 3))
	    {
		if (m == 0xff)
		    WRITE(dst++, src16);
		else
		{
		    d = READ(dst);
		    m >>= 3;
		    inOver0565 (m, src16, d, dst++);
		}
	    }
	    else
	    {
		if (m == 0xff) 
		{
		    d = READ(dst);
		    t = fbOver24 (src, cvt0565to0888 (d));
		    WRITE(dst++, cvt8888to0565 (t));
		}
		else
		{
		    d = READ(dst);
		    t = fbIn (src, m);
		    t = fbOver (t, cvt0565to0888 (d));
		    WRITE(dst++, cvt8888to0565 (t));
		}
	    }
	}
    }
}

void
fbCompositeSolidMask_nx8888x0565C (CARD8      op,
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
    CARD16	src16;
    CARD16	*dstLine, *dst;
    CARD32	d;
    CARD32	*maskLine, *mask, ma;
    FbStride	dstStride, maskStride;
    CARD16	w;
    CARD32	m, n, o;

    fbComposeGetSolid(pSrc, src, pDst->format);

    srca = src >> 24;
    if (src == 0)
	return;

    src16 = cvt8888to0565(src);

    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);
    fbComposeGetStart (pMask, xMask, yMask, CARD32, maskStride, maskLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	mask = maskLine;
	maskLine += maskStride;
	w = width;

	while (w--)
	{
	    ma = READ(mask++);
	    if (ma == 0xffffffff)
	    {
		if (srca == 0xff)
		{
		    WRITE(dst, src16);
		}
		else
		{
		    d = READ(dst);
		    d = fbOver24 (src, cvt0565to0888(d));
		    WRITE(dst, cvt8888to0565(d));
		}
	    }
	    else if (ma)
	    {
		d = READ(dst);
		d = cvt0565to0888(d);
		FbInOverC (src, srca, ma, d, 0, m);
		FbInOverC (src, srca, ma, d, 8, n);
		FbInOverC (src, srca, ma, d, 16, o);
		d = m|n|o;
		WRITE(dst, cvt8888to0565(d));
	    }
	    dst++;
	}
    }

    fbFinishAccess (pMask->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

void
fbCompositeSrc_8888x8888 (CARD8      op,
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
    CARD32	*dstLine, *dst, dstMask;
    CARD32	*srcLine, *src, s;
    FbStride	dstStride, srcStride;
    CARD8	a;
    CARD16	w;

    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);
    fbComposeGetStart (pSrc, xSrc, ySrc, CARD32, srcStride, srcLine, 1);

    dstMask = FbFullMask (pDst->pDrawable->depth);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w--)
	{
	    s = READ(src++);
	    a = s >> 24;
	    if (a == 0xff)
		WRITE(dst, s & dstMask);
	    else if (a)
		WRITE(dst, fbOver (s, READ(dst)) & dstMask);
	    dst++;
	}
    }

    fbFinishAccess (pSrc->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

void
fbCompositeSrc_8888x0888 (CARD8      op,
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
    CARD32	d;
    CARD32	*srcLine, *src, s;
    CARD8	a;
    FbStride	dstStride, srcStride;
    CARD16	w;

    fbComposeGetStart (pDst, xDst, yDst, CARD8, dstStride, dstLine, 3);
    fbComposeGetStart (pSrc, xSrc, ySrc, CARD32, srcStride, srcLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w--)
	{
	    s = READ(src++);
	    a = s >> 24;
	    if (a)
	    {
		if (a == 0xff)
		    d = s;
		else
		    d = fbOver24 (s, Fetch24(dst));
		Store24(dst,d);
	    }
	    dst += 3;
	}
    }

    fbFinishAccess (pSrc->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

void
fbCompositeSrc_8888x0565 (CARD8      op,
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
    CARD32	d;
    CARD32	*srcLine, *src, s;
    CARD8	a;
    FbStride	dstStride, srcStride;
    CARD16	w;

    fbComposeGetStart (pSrc, xSrc, ySrc, CARD32, srcStride, srcLine, 1);
    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w--)
	{
	    s = READ(src++);
	    a = s >> 24;
	    if (a)
	    {
		if (a == 0xff)
		    d = s;
		else
		{
		    d = READ(dst);
		    d = fbOver24 (s, cvt0565to0888(d));
		}
		WRITE(dst, cvt8888to0565(d));
	    }
	    dst++;
	}
    }

    fbFinishAccess (pDst->pDrawable);
    fbFinishAccess (pSrc->pDrawable);
}

void
fbCompositeSrcAdd_8000x8000 (CARD8	op,
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

    fbComposeGetStart (pSrc, xSrc, ySrc, CARD8, srcStride, srcLine, 1);
    fbComposeGetStart (pDst, xDst, yDst, CARD8, dstStride, dstLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w--)
	{
	    s = READ(src++);
	    if (s)
	    {
		if (s != 0xff)
		{
		    d = READ(dst);
		    t = d + s;
		    s = t | (0 - (t >> 8));
		}
		WRITE(dst, s);
	    }
	    dst++;
	}
    }

    fbFinishAccess (pDst->pDrawable);
    fbFinishAccess (pSrc->pDrawable);
}

void
fbCompositeSrcAdd_8888x8888 (CARD8	op,
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
    CARD32	s, d;
    CARD16	t;
    CARD32	m,n,o,p;

    fbComposeGetStart (pSrc, xSrc, ySrc, CARD32, srcStride, srcLine, 1);
    fbComposeGetStart (pDst, xDst, yDst, CARD32, dstStride, dstLine, 1);

    while (height--)
    {
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;

	while (w--)
	{
	    s = READ(src++);
	    if (s)
	    {
		if (s != 0xffffffff)
		{
		    d = READ(dst);
		    if (d)
		    {
			m = FbAdd(s,d,0,t);
			n = FbAdd(s,d,8,t);
			o = FbAdd(s,d,16,t);
			p = FbAdd(s,d,24,t);
			s = m|n|o|p;
		    }
		}
		WRITE(dst, s);
	    }
	    dst++;
	}
    }

    fbFinishAccess (pDst->pDrawable);
    fbFinishAccess (pSrc->pDrawable);
}

void
fbCompositeSrcAdd_1000x1000 (CARD8	op,
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
    FbBits	*dstBits, *srcBits;
    FbStride	dstStride, srcStride;
    int		dstBpp, srcBpp;
    int		dstXoff, dstYoff;
    int		srcXoff, srcYoff;

    fbGetDrawable(pSrc->pDrawable, srcBits, srcStride, srcBpp, srcXoff, srcYoff);

    fbGetDrawable(pDst->pDrawable, dstBits, dstStride, dstBpp, dstXoff, dstYoff);

    fbBlt (srcBits + srcStride * (ySrc + srcYoff),
	   srcStride,
	   xSrc + srcXoff,

	   dstBits + dstStride * (yDst + dstYoff),
	   dstStride,
	   xDst + dstXoff,

	   width,
	   height,

	   GXor,
	   FB_ALLONES,
	   srcBpp,

	   FALSE,
	   FALSE);

    fbFinishAccess(pDst->pDrawable);
    fbFinishAccess(pSrc->pDrawable);
}

void
fbCompositeSolidMask_nx1xn (CARD8      op,
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
    FbBits	*dstBits;
    FbStip	*maskBits;
    FbStride	dstStride, maskStride;
    int		dstBpp, maskBpp;
    int		dstXoff, dstYoff;
    int		maskXoff, maskYoff;
    FbBits	src;

    fbComposeGetSolid(pSrc, src, pDst->format);

    if ((src & 0xff000000) != 0xff000000)
    {
	fbCompositeGeneral  (op, pSrc, pMask, pDst,
			     xSrc, ySrc, xMask, yMask, xDst, yDst,
			     width, height);
	return;
    }
    fbGetStipDrawable (pMask->pDrawable, maskBits, maskStride, maskBpp, maskXoff, maskYoff);
    fbGetDrawable (pDst->pDrawable, dstBits, dstStride, dstBpp, dstXoff, dstYoff);

    switch (dstBpp) {
    case 32:
	break;
    case 24:
	break;
    case 16:
	src = cvt8888to0565(src);
	break;
    }

    src = fbReplicatePixel (src, dstBpp);

    fbBltOne (maskBits + maskStride * (yMask + maskYoff),
	      maskStride,
	      xMask + maskXoff,

	      dstBits + dstStride * (yDst + dstYoff),
	      dstStride,
	      (xDst + dstXoff) * dstBpp,
	      dstBpp,

	      width * dstBpp,
	      height,

	      0x0,
	      src,
	      FB_ALLONES,
	      0x0);

    fbFinishAccess (pDst->pDrawable);
    fbFinishAccess (pMask->pDrawable);
}

# define mod(a,b)	((b) == 1 ? 0 : (a) >= 0 ? (a) % (b) : (b) - (-a) % (b))

/*
 * Apply a constant alpha value in an over computation
 */
static void
fbCompositeSrcSrc_nxn  (CARD8	   op,
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
			CARD16     height);

static void
fbCompositeTrans_0565xnx0565(CARD8      op,
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
    CARD16	*srcLine, *src;
    FbStride	dstStride, srcStride;
    CARD16	w;
    FbBits	mask;
    CARD8	maskAlpha;
    CARD16	s_16, d_16;
    CARD32	s_32, d_32;
    
    fbComposeGetSolid (pMask, mask, pDst->format);
    maskAlpha = mask >> 27;
    
    if (!maskAlpha)
	return;
    if (maskAlpha == 0xff)
    {
	fbCompositeSrcSrc_nxn (PictOpSrc, pSrc, pMask, pDst,
			       xSrc, ySrc, xMask, yMask, xDst, yDst, 
			       width, height);
	return;
    }

    fbComposeGetStart (pSrc, xSrc, ySrc, CARD16, srcStride, srcLine, 1);
    fbComposeGetStart (pDst, xDst, yDst, CARD16, dstStride, dstLine, 1);

    while (height--)
    {
	CARD32 *isrc, *idst;
	dst = dstLine;
	dstLine += dstStride;
	src = srcLine;
	srcLine += srcStride;
	w = width;
	
	if(((long)src&1)==1)
	{
	    s_16 = READ(src++);
	    d_16 = READ(dst);
	    inOver0565(maskAlpha, s_16, d_16, dst++);
	    w--;
	}
	isrc=(CARD32 *)src;
	if(((long)dst&1)==0)
	{
	    idst=(CARD32 *)dst;
	    while (w>1)
	    {
		s_32 = READ(isrc++);
		d_32 = READ(idst);
		inOver2x0565(maskAlpha, s_32, d_32, idst++);
		w-=2;
	    }
	    dst=(CARD16 *)idst;
	}
	else
	{
	    while (w > 1)
	    {
		s_32 = READ(isrc++);
#if IMAGE_BYTE_ORDER == LSBFirst
		s_16=s_32&0xffff;
#else
		s_16=s_32>>16;
#endif
		d_16 = READ(dst);
		inOver0565 (maskAlpha, s_16, d_16, dst++);
#if IMAGE_BYTE_ORDER == LSBFirst
		s_16=s_32>>16;
#else
		s_16=s_32&0xffff;
#endif
		d_16 = READ(dst);
		inOver0565(maskAlpha, s_16, d_16, dst++);
		w-=2;
	    }
	}
	src=(CARD16 *)isrc;
	if(w!=0)
	{
	    s_16 = READ(src);
	    d_16 = READ(dst);
	    inOver0565(maskAlpha, s_16, d_16, dst);
	}
    }
    
    fbFinishAccess (pSrc->pDrawable);
    fbFinishAccess (pDst->pDrawable);
}

/* macros for "i can't believe it's not fast" packed pixel handling */
#define alphamaskCombine24(a,b) genericCombine24(a,b,maskAlpha,maskiAlpha)

static void
fbCompositeTrans_0888xnx0888(CARD8      op,
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
    CARD8	*dstLine, *dst,*idst;
    CARD8	*srcLine, *src;
    FbStride	dstStride, srcStride;
    CARD16	w;
    FbBits	mask;
    CARD16	maskAlpha,maskiAlpha;
    
    fbComposeGetSolid (pMask, mask, pDst->format);
    maskAlpha = mask >> 24;
    maskiAlpha= 255-maskAlpha;
    
    if (!maskAlpha)
 	return;
    /*
      if (maskAlpha == 0xff)
      {
      fbCompositeSrc_0888x0888 (op, pSrc, pMask, pDst,
      xSrc, ySrc, xMask, yMask, xDst, yDst, 
      width, height);
      return;
      }
    */
    
    fbComposeGetStart (pSrc, xSrc, ySrc, CARD8, srcStride, srcLine, 3);
    fbComposeGetStart (pDst, xDst, yDst, CARD8, dstStride, dstLine, 3);
    
    {
	unsigned long ws,wt;
	CARD32 workingSource;
	CARD32 *wsrc, *wdst, *widst;
	CARD32 rs, rd, nd;
	CARD8 *isrc;
	
	
	/* are xSrc and xDst at the same alignment?  if not, we need to be complicated :) */
	/* if(0==0) */
	if ((((xSrc * 3) & 3) != ((xDst * 3) & 3)) ||
	    ((srcStride & 3) != (dstStride & 3)))
	{
	    while (height--)
	    {
		dst = dstLine;
		dstLine += dstStride;
		isrc = src = srcLine;
		srcLine += srcStride;
		w = width*3;
		
		setupPackedReader(ws,wt,isrc,wsrc,workingSource);
		
		/* get to word aligned */
		switch(~(long)dst&3)
		{
		case 1:
		    readPackedSource(rs);
		    /* *dst++=alphamaskCombine24(rs, *dst)>>8; */
		    rd = READ(dst);  /* make gcc happy.  hope it doens't cost us too much performance*/
		    WRITE(dst++, alphamaskCombine24(rs, rd) >> 8);
		    w--; if(w==0) break;
		case 2:
		    readPackedSource(rs);
		    rd = READ(dst);
		    WRITE(dst++, alphamaskCombine24(rs, rd) >> 8);
		    w--; if(w==0) break;
		case 3:
		    readPackedSource(rs);
		    rd = READ(dst);
		    WRITE(dst++,alphamaskCombine24(rs, rd) >> 8);
		    w--; if(w==0) break;
		}
		wdst=(CARD32 *)dst;
		while (w>3)
		{
		    rs=READ(wsrc++);
		    /* FIXME: write a special readPackedWord macro, which knows how to 
		     * halfword combine
		     */
#if IMAGE_BYTE_ORDER == LSBFirst
		    rd=READ(wdst);
		    readPackedSource(nd);
		    readPackedSource(rs);
		    nd|=rs<<8;
		    readPackedSource(rs);
		    nd|=rs<<16;
		    readPackedSource(rs);
		    nd|=rs<<24;
#else
		    readPackedSource(nd);
		    nd<<=24;
		    readPackedSource(rs);
		    nd|=rs<<16;
		    readPackedSource(rs);
		    nd|=rs<<8;
		    readPackedSource(rs);
		    nd|=rs;
#endif
		    inOver0888(maskAlpha, nd, rd, wdst++);
		    w-=4;
		}
		src=(CARD8 *)wdst;
		switch(w)
		{
		case 3:
		    readPackedSource(rs);
		    rd=READ(dst);
		    WRITE(dst++,alphamaskCombine24(rs, rd)>>8);
		case 2:
		    readPackedSource(rs);
		    rd = READ(dst);  
		    WRITE(dst++, alphamaskCombine24(rs, rd)>>8);
		case 1:
		    readPackedSource(rs);
		    rd = READ(dst);  
		    WRITE(dst++, alphamaskCombine24(rs, rd)>>8);
		}
	    }
	}
	else
	{
	    while (height--)
	    {
		idst=dst = dstLine;
		dstLine += dstStride;
		src = srcLine;
		srcLine += srcStride;
		w = width*3;
		/* get to word aligned */
		switch(~(long)src&3)
		{
		case 1:
		    rd=alphamaskCombine24(READ(src++), READ(dst))>>8;
		    WRITE(dst++, rd);
		    w--; if(w==0) break;
		case 2:
		    rd=alphamaskCombine24(READ(src++), READ(dst))>>8;
		    WRITE(dst++, rd);
		    w--; if(w==0) break;
		case 3:
		    rd=alphamaskCombine24(READ(src++), READ(dst))>>8;
		    WRITE(dst++, rd);
		    w--; if(w==0) break;
		}
		wsrc=(CARD32 *)src;
		widst=(CARD32 *)dst;
		while(w>3)
		{
		    rs = READ(wsrc++);
		    rd = READ(widst);
		    inOver0888 (maskAlpha, rs, rd, widst++);
		    w-=4;
		}
		src=(CARD8 *)wsrc;
		dst=(CARD8 *)widst;
		switch(w)
		{
		case 3:
		    rd=alphamaskCombine24(READ(src++), READ(dst))>>8;
		    WRITE(dst++, rd);
		case 2:
		    rd=alphamaskCombine24(READ(src++), READ(dst))>>8;
		    WRITE(dst++, rd);
		case 1:
		    rd=alphamaskCombine24(READ(src++), READ(dst))>>8;
		    WRITE(dst++, rd);
		}
	    }
	}
    }
}

/*
 * Simple bitblt
 */

static void
fbCompositeSrcSrc_nxn  (CARD8	   op,
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
    FbBits	*dst;
    FbBits	*src;
    FbStride	dstStride, srcStride;
    int		srcXoff, srcYoff;
    int		dstXoff, dstYoff;
    int		srcBpp;
    int		dstBpp;
    Bool	reverse = FALSE;
    Bool	upsidedown = FALSE;
    
    fbGetDrawable(pSrc->pDrawable,src,srcStride,srcBpp,srcXoff,srcYoff);
    fbGetDrawable(pDst->pDrawable,dst,dstStride,dstBpp,dstXoff,dstYoff);
	
    fbBlt (src + (ySrc + srcYoff) * srcStride,
	   srcStride,
	   (xSrc + srcXoff) * srcBpp,
 
	   dst + (yDst + dstYoff) * dstStride,
	   dstStride,
	   (xDst + dstXoff) * dstBpp,

	   (width) * dstBpp,
	   (height),

	   GXcopy,
	   FB_ALLONES,
	   dstBpp,

	   reverse,
	   upsidedown);
    
    fbFinishAccess(pSrc->pDrawable);
    fbFinishAccess(pDst->pDrawable);
}

/*
 * Solid fill
void
fbCompositeSolidSrc_nxn  (CARD8	op,
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
    
}
 */

void
fbComposite (CARD8      op,
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
    RegionRec	    region;
    int		    n;
    BoxPtr	    pbox;
    CompositeFunc   func = NULL;
    Bool	    srcRepeat = pSrc->pDrawable && pSrc->repeat;
    Bool	    maskRepeat = FALSE;
    Bool	    srcAlphaMap = pSrc->alphaMap != 0;
    Bool	    maskAlphaMap = FALSE;
    Bool	    dstAlphaMap = pDst->alphaMap != 0;
    int		    x_msk, y_msk, x_src, y_src, x_dst, y_dst;
    int		    w, h, w_this, h_this;

#ifdef USE_MMX
    static Bool mmx_setup = FALSE;
    if (!mmx_setup) {
        fbComposeSetupMMX();
        mmx_setup = TRUE;
    }
#endif
        
    xDst += pDst->pDrawable->x;
    yDst += pDst->pDrawable->y;
    if (pSrc->pDrawable) {
        xSrc += pSrc->pDrawable->x;
        ySrc += pSrc->pDrawable->y;
    }
    if (pMask && pMask->pDrawable)
    {
	xMask += pMask->pDrawable->x;
	yMask += pMask->pDrawable->y;
	maskRepeat = pMask->repeat == RepeatNormal;
	maskAlphaMap = pMask->alphaMap != 0;
    }

    if (pSrc->pDrawable && (!pMask || pMask->pDrawable)
        && !pSrc->transform && !(pMask && pMask->transform)
        && !maskAlphaMap && !srcAlphaMap && !dstAlphaMap
        && (pSrc->filter != PictFilterConvolution)
        && (!pMask || pMask->filter != PictFilterConvolution))
    switch (op) {
    case PictOpSrc:
#ifdef USE_MMX
	if (!pMask && pSrc->format == pDst->format &&
	    pSrc->format != PICT_a8 && pSrc->pDrawable != pDst->pDrawable)
	{
	    func = fbCompositeCopyAreammx;
	}
	else
#endif
	    if (pMask == 0)
	    {
		if (pSrc->format == pDst->format)
		    func = fbCompositeSrcSrc_nxn;
	    }
	break;
    case PictOpOver:
	if (pMask)
	{
	    if (fbCanGetSolid(pSrc) &&
		!maskRepeat)
	    {
		if (PICT_FORMAT_COLOR(pSrc->format)) {
		    switch (pMask->format) {
		    case PICT_a8:
			switch (pDst->format) {
			case PICT_r5g6b5:
			case PICT_b5g6r5:
#ifdef USE_MMX
			    if (fbHaveMMX())
				func = fbCompositeSolidMask_nx8x0565mmx;
			    else
#endif
				func = fbCompositeSolidMask_nx8x0565;
			    break;
			case PICT_r8g8b8:
			case PICT_b8g8r8:
			    func = fbCompositeSolidMask_nx8x0888;
			    break;
			case PICT_a8r8g8b8:
			case PICT_x8r8g8b8:
			case PICT_a8b8g8r8:
			case PICT_x8b8g8r8:
#ifdef USE_MMX
			    if (fbHaveMMX())
				func = fbCompositeSolidMask_nx8x8888mmx;
			    else
#endif
				func = fbCompositeSolidMask_nx8x8888;
			    break;
			default:
			    break;
			}
			break;
		    case PICT_a8r8g8b8:
			if (pMask->componentAlpha) {
			    switch (pDst->format) {
			    case PICT_a8r8g8b8:
			    case PICT_x8r8g8b8:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSolidMask_nx8888x8888Cmmx;
				else
#endif
				    func = fbCompositeSolidMask_nx8888x8888C;
				break;
			    case PICT_r5g6b5:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSolidMask_nx8888x0565Cmmx;
				else
#endif
				    func = fbCompositeSolidMask_nx8888x0565C;
				break;
			    default:
				break;
			    }
			}
			else
			{
			    switch (pDst->format) {
                            case PICT_r5g6b5:
                                func = fbCompositeSolidMask_nx8888x0565;
                                break;
			    default:
				break;
                            }
			}
			break;
		    case PICT_a8b8g8r8:
			if (pMask->componentAlpha) {
			    switch (pDst->format) {
			    case PICT_a8b8g8r8:
			    case PICT_x8b8g8r8:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSolidMask_nx8888x8888Cmmx;
				else
#endif
				    func = fbCompositeSolidMask_nx8888x8888C;
				break;
			    case PICT_b5g6r5:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSolidMask_nx8888x0565Cmmx;
				else
#endif
				    func = fbCompositeSolidMask_nx8888x0565C;
				break;
			    default:
				break;
			    }
			}
			else
			{
			    switch (pDst->format) {
                            case PICT_b5g6r5:
                                func = fbCompositeSolidMask_nx8888x0565;
                                break;
			    default:
				break;
                            }
			}
			break;
		    case PICT_a1:
			switch (pDst->format) {
			case PICT_r5g6b5:
			case PICT_b5g6r5:
			case PICT_r8g8b8:
			case PICT_b8g8r8:
			case PICT_a8r8g8b8:
			case PICT_x8r8g8b8:
			case PICT_a8b8g8r8:
			case PICT_x8b8g8r8:
			    func = fbCompositeSolidMask_nx1xn;
			    break;
			default:
			    break;
			}
			break;
		    default:
			break;
		    }
		default:
		    break;
		}
	    }
	    else if (! srcRepeat) /* has mask and non-repeating source */
	    {
		if (pSrc->pDrawable == pMask->pDrawable &&
		    xSrc == xMask && ySrc == yMask &&
		    !pMask->componentAlpha && !maskRepeat)
		{
		    /* source == mask: non-premultiplied data */
		    switch (pSrc->format) {
		    case PICT_x8b8g8r8:
			switch (pMask->format) {
			case PICT_a8r8g8b8:
			case PICT_a8b8g8r8:
			    switch (pDst->format) {
			    case PICT_a8r8g8b8:
			    case PICT_x8r8g8b8:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSrc_8888RevNPx8888mmx;
#endif
				break;
			    case PICT_r5g6b5:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSrc_8888RevNPx0565mmx;
#endif
				break;
			    default:
				break;
			    }
			    break;
			default:
			    break;
			}
			break;
		    case PICT_x8r8g8b8:
			switch (pMask->format) {
			case PICT_a8r8g8b8:
			case PICT_a8b8g8r8:
			    switch (pDst->format) {
			    case PICT_a8b8g8r8:
			    case PICT_x8b8g8r8:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSrc_8888RevNPx8888mmx;
#endif
				break;
			    case PICT_r5g6b5:
#ifdef USE_MMX
				if (fbHaveMMX())
				    func = fbCompositeSrc_8888RevNPx0565mmx;
#endif
				break;
			    default:
				break;
			    }
			    break;
			default:
			    break;
			}
			break;
		    default:
			break;
		    }
		    break;
		}
		else
		{
		    /* non-repeating source, repeating mask => translucent window */
		    if (fbCanGetSolid(pMask))
		    {
			if (pSrc->format == PICT_x8r8g8b8 &&
			    pDst->format == PICT_x8r8g8b8 &&
			    pMask->format == PICT_a8)
			{
#ifdef USE_MMX
			    if (fbHaveMMX())
				func = fbCompositeSrc_8888x8x8888mmx;
#endif
			}
		    }
		}
		if (func != fbCompositeGeneral)
		    srcRepeat = FALSE;
	    }
	    else if (maskRepeat &&
		     pMask->pDrawable->width == 1 &&
		     pMask->pDrawable->height == 1)
	    {
		switch (pSrc->format) {
		case PICT_r5g6b5:
		case PICT_b5g6r5:
		    if (pDst->format == pSrc->format)
		        func = fbCompositeTrans_0565xnx0565;
		    break;
 		case PICT_r8g8b8:
 		case PICT_b8g8r8:
 		    if (pDst->format == pSrc->format)
 		        func = fbCompositeTrans_0888xnx0888;
 		    break;
		default:
		    break;
		}
		if (func != fbCompositeGeneral)
		    maskRepeat = FALSE;
	    }
	}
	else /* no mask */
	{
	    if (fbCanGetSolid(pSrc))
	    {
		/* no mask and repeating source */
		switch (pSrc->format) {
		case PICT_a8r8g8b8:
		    switch (pDst->format) {
		    case PICT_a8r8g8b8:
		    case PICT_x8r8g8b8:
#ifdef USE_MMX
			if (fbHaveMMX())
			{
			    srcRepeat = FALSE;
			    func = fbCompositeSolid_nx8888mmx;
			}
#endif
			break;
		    case PICT_r5g6b5:
#ifdef USE_MMX
			if (fbHaveMMX())
			{
			    srcRepeat = FALSE;
			    func = fbCompositeSolid_nx0565mmx;
			}
#endif
			break;
		    default:
			break;
		    }
		    break;
		default:
		    break;
		}
	    }
	    else if (! srcRepeat)
	    {
		/*
		 * Formats without alpha bits are just Copy with Over
		 */
		if (pSrc->format == pDst->format && !PICT_FORMAT_A(pSrc->format))
		{
#ifdef USE_MMX
		    if (fbHaveMMX() &&
			(pSrc->format == PICT_x8r8g8b8 || pSrc->format == PICT_x8b8g8r8))
			func = fbCompositeCopyAreammx;
		    else
#endif
			func = fbCompositeSrcSrc_nxn;
		}
		else switch (pSrc->format) {
		case PICT_a8r8g8b8:
		    switch (pDst->format) {
		    case PICT_a8r8g8b8:
		    case PICT_x8r8g8b8:
#ifdef USE_MMX
			if (fbHaveMMX())
			    func = fbCompositeSrc_8888x8888mmx;
			else
#endif
			    func = fbCompositeSrc_8888x8888;
			break;
		    case PICT_r8g8b8:
			func = fbCompositeSrc_8888x0888;
			break;
		    case PICT_r5g6b5:
#ifdef USE_MMX
			if (fbHaveMMX())
			    func = fbCompositeSrc_8888x0565mmx;
			else
#endif
			    func = fbCompositeSrc_8888x0565;
			break;
		    default:
			break;
		    }
		    break;
		case PICT_x8r8g8b8:
		    switch (pDst->format) {
		    case PICT_a8r8g8b8:
		    case PICT_x8r8g8b8:
#ifdef USE_MMX
			if (fbHaveMMX())
			    func = fbCompositeCopyAreammx;
#endif
			break;
		    default:
			break;
		    }
		case PICT_x8b8g8r8:
		    switch (pDst->format) {
		    case PICT_a8b8g8r8:
		    case PICT_x8b8g8r8:
#ifdef USE_MMX
			if (fbHaveMMX())
			    func = fbCompositeCopyAreammx;
#endif
			break;
		    default:
			break;
		    }
		    break;
		case PICT_a8b8g8r8:
		    switch (pDst->format) {
		    case PICT_a8b8g8r8:
		    case PICT_x8b8g8r8:
#ifdef USE_MMX
			if (fbHaveMMX())
			    func = fbCompositeSrc_8888x8888mmx;
			else
#endif
			    func = fbCompositeSrc_8888x8888;
			break;
		    case PICT_b8g8r8:
			func = fbCompositeSrc_8888x0888;
			break;
		    case PICT_b5g6r5:
#ifdef USE_MMX
			if (fbHaveMMX())
			    func = fbCompositeSrc_8888x0565mmx;
			else
#endif
			    func = fbCompositeSrc_8888x0565;
			break;
		    default:
			break;
		    }
		    break;
		default:
		    break;
		}
	    }
	}
	break;
    case PictOpAdd:
	if (pMask == 0)
	{
	    switch (pSrc->format) {
	    case PICT_a8r8g8b8:
		switch (pDst->format) {
		case PICT_a8r8g8b8:
#ifdef USE_MMX
		    if (fbHaveMMX())
			func = fbCompositeSrcAdd_8888x8888mmx;
		    else
#endif
			func = fbCompositeSrcAdd_8888x8888;
		    break;
		default:
		    break;
		}
		break;
	    case PICT_a8b8g8r8:
		switch (pDst->format) {
		case PICT_a8b8g8r8:
#ifdef USE_MMX
		    if (fbHaveMMX())
			func = fbCompositeSrcAdd_8888x8888mmx;
		    else
#endif
			func = fbCompositeSrcAdd_8888x8888;
		    break;
		default:
		    break;
		}
		break;
	    case PICT_a8:
		switch (pDst->format) {
		case PICT_a8:
#ifdef USE_MMX
		    if (fbHaveMMX())
			func = fbCompositeSrcAdd_8000x8000mmx;
		    else
#endif
			func = fbCompositeSrcAdd_8000x8000;
		    break;
		default:
		    break;
		}
		break;
	    case PICT_a1:
		switch (pDst->format) {
		case PICT_a1:
		    func = fbCompositeSrcAdd_1000x1000;
		    break;
		default:
		    break;
		}
		break;
	    default:
		break;
	    }
	}
	break;
    }

    if (!func) {
         /* no fast path, use the general code */
         fbCompositeGeneral(op, pSrc, pMask, pDst, xSrc, ySrc, xMask, yMask, xDst, yDst, width, height);
         return;
    }

    if (!miComputeCompositeRegion (&region,
 				   pSrc,
 				   pMask,
 				   pDst,
 				   xSrc,
 				   ySrc,
 				   xMask,
 				   yMask,
 				   xDst,
 				   yDst,
 				   width,
                                   height))
        return;

    n = REGION_NUM_RECTS (&region);
    pbox = REGION_RECTS (&region);
    while (n--)
    {
	h = pbox->y2 - pbox->y1;
	y_src = pbox->y1 - yDst + ySrc;
	y_msk = pbox->y1 - yDst + yMask;
	y_dst = pbox->y1;
	while (h)
	{
	    h_this = h;
	    w = pbox->x2 - pbox->x1;
	    x_src = pbox->x1 - xDst + xSrc;
	    x_msk = pbox->x1 - xDst + xMask;
	    x_dst = pbox->x1;
	    if (maskRepeat)
	    {
		y_msk = mod (y_msk - pMask->pDrawable->y, pMask->pDrawable->height);
		if (h_this > pMask->pDrawable->height - y_msk)
		    h_this = pMask->pDrawable->height - y_msk;
		y_msk += pMask->pDrawable->y;
	    }
	    if (srcRepeat)
	    {
		y_src = mod (y_src - pSrc->pDrawable->y, pSrc->pDrawable->height);
		if (h_this > pSrc->pDrawable->height - y_src)
		    h_this = pSrc->pDrawable->height - y_src;
		y_src += pSrc->pDrawable->y;
	    }
	    while (w)
	    {
		w_this = w;
		if (maskRepeat)
		{
		    x_msk = mod (x_msk - pMask->pDrawable->x, pMask->pDrawable->width);
		    if (w_this > pMask->pDrawable->width - x_msk)
			w_this = pMask->pDrawable->width - x_msk;
		    x_msk += pMask->pDrawable->x;
		}
		if (srcRepeat)
		{
		    x_src = mod (x_src - pSrc->pDrawable->x, pSrc->pDrawable->width);
		    if (w_this > pSrc->pDrawable->width - x_src)
			w_this = pSrc->pDrawable->width - x_src;
		    x_src += pSrc->pDrawable->x;
		}
		(*func) (op, pSrc, pMask, pDst,
			 x_src, y_src, x_msk, y_msk, x_dst, y_dst,
			 w_this, h_this);
		w -= w_this;
		x_src += w_this;
		x_msk += w_this;
		x_dst += w_this;
	    }
	    h -= h_this;
	    y_src += h_this;
	    y_msk += h_this;
	    y_dst += h_this;
	}
	pbox++;
    }
    REGION_UNINIT (pDst->pDrawable->pScreen, &region);
}

#endif /* RENDER */

Bool
fbPictureInit (ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{

#ifdef RENDER

    PictureScreenPtr    ps;

    if (!miPictureInit (pScreen, formats, nformats))
	return FALSE;
    ps = GetPictureScreen(pScreen);
    ps->Composite = fbComposite;
    ps->Glyphs = miGlyphs;
    ps->CompositeRects = miCompositeRects;
    ps->RasterizeTrapezoid = fbRasterizeTrapezoid;
    ps->AddTraps = fbAddTraps;
    ps->AddTriangles = fbAddTriangles;

#endif /* RENDER */

    return TRUE;
}

#ifdef USE_MMX
/* The CPU detection code needs to be in a file not compiled with
 * "-mmmx -msse", as gcc would generate CMOV instructions otherwise
 * that would lead to SIGILL instructions on old CPUs that don't have
 * it.
 */
#if !defined(__amd64__) && !defined(__x86_64__)

#ifdef HAVE_GETISAX
#include <sys/auxv.h>
#endif

enum CPUFeatures {
    NoFeatures = 0,
    MMX = 0x1,
    MMX_Extensions = 0x2, 
    SSE = 0x6,
    SSE2 = 0x8,
    CMOV = 0x10
};

static unsigned int detectCPUFeatures(void) {
    unsigned int features = 0;
    unsigned int result;

#ifdef HAVE_GETISAX
    if (getisax(&result, 1)) {
        if (result & AV_386_CMOV)
            features |= CMOV;
        if (result & AV_386_MMX)
            features |= MMX;
        if (result & AV_386_AMD_MMX)
            features |= MMX_Extensions;
        if (result & AV_386_SSE)
            features |= SSE;
        if (result & AV_386_SSE2)
            features |= SSE2;
    }
#else
    char vendor[13];
    vendor[0] = 0;
    vendor[12] = 0;
    /* see p. 118 of amd64 instruction set manual Vol3 */
    /* We need to be careful about the handling of %ebx and
     * %esp here. We can't declare either one as clobbered
     * since they are special registers (%ebx is the "PIC
     * register" holding an offset to global data, %esp the
     * stack pointer), so we need to make sure they have their
     * original values when we access the output operands.
     */
    __asm__ ("pushf\n"
             "pop %%eax\n"
             "mov %%eax, %%ecx\n"
             "xor $0x00200000, %%eax\n"
             "push %%eax\n"
             "popf\n"
             "pushf\n"
             "pop %%eax\n"
             "mov $0x0, %%edx\n"
             "xor %%ecx, %%eax\n"
             "jz 1\n"

             "mov $0x00000000, %%eax\n"
	     "push %%ebx\n"
             "cpuid\n"
             "mov %%ebx, %%eax\n"
	     "pop %%ebx\n"
	     "mov %%eax, %1\n"
             "mov %%edx, %2\n"
             "mov %%ecx, %3\n"
             "mov $0x00000001, %%eax\n"
	     "push %%ebx\n"
             "cpuid\n"
	     "pop %%ebx\n"
             "1:\n"
             "mov %%edx, %0\n"
             : "=r" (result), 
               "=m" (vendor[0]), 
               "=m" (vendor[4]), 
               "=m" (vendor[8])
             :
             : "%eax", "%ecx", "%edx"
        );

    if (result) {
        /* result now contains the standard feature bits */
        if (result & (1 << 15))
            features |= CMOV;
        if (result & (1 << 23))
            features |= MMX;
        if (result & (1 << 25))
            features |= SSE;
        if (result & (1 << 26))
            features |= SSE2;
        if ((features & MMX) && !(features & SSE) &&
            (strcmp(vendor, "AuthenticAMD") == 0 ||
             strcmp(vendor, "Geode by NSC") == 0)) {
            /* check for AMD MMX extensions */

            unsigned int result;            
            __asm__("push %%ebx\n"
                    "mov $0x80000000, %%eax\n"
                    "cpuid\n"
                    "xor %%edx, %%edx\n"
                    "cmp $0x1, %%eax\n"
                    "jge 2\n"
                    "mov $0x80000001, %%eax\n"
                    "cpuid\n"
                    "2:\n"
                    "pop %%ebx\n"
                    "mov %%edx, %0\n"
                    : "=r" (result)
                    :
                    : "%eax", "%ecx", "%edx"
                );
            if (result & (1<<22))
                features |= MMX_Extensions;
        }
    }
#endif /* HAVE_GETISAX */
    return features;
}

Bool
fbHaveMMX (void)
{
    static Bool initialized = FALSE;
    static Bool mmx_present;

    if (!initialized)
    {
        unsigned int features = detectCPUFeatures();
	mmx_present = (features & (MMX|MMX_Extensions)) == (MMX|MMX_Extensions);
        initialized = TRUE;
    }
    
    return mmx_present;
}
#endif /* __amd64__ */
#endif
