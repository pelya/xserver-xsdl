/*
 * Copyright © 1998 Keith Packard
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

#ifdef __clang__
/* shift overflow is intentional */
#pragma clang diagnostic ignored "-Wshift-overflow"
#endif

/*
 *  Example: srcX = 13 dstX = 8	(FB unit 32 dstBpp 8)
 *
 *	**** **** **** **** **** **** **** ****
 *			^
 *	********  ********  ********  ********
 *		  ^
 *  leftShift = 12
 *  rightShift = 20
 *
 *  Example: srcX = 0 dstX = 8 (FB unit 32 dstBpp 8)
 *
 *	**** **** **** **** **** **** **** ****
 *	^		
 *	********  ********  ********  ********
 *		  ^
 *
 *  leftShift = 24
 *  rightShift = 8
 */

#define LoadBits {\
    if (leftShift) { \
	bitsRight = (src < srcEnd ? READ(src++) : 0); \
	bits = (FbStipLeft (bitsLeft, leftShift) | \
		FbStipRight(bitsRight, rightShift)); \
	bitsLeft = bitsRight; \
    } else \
	bits = (src < srcEnd ? READ(src++) : 0); \
}

void
fbBltOne(FbStip * src, FbStride srcStride,      /* FbStip units per scanline */
         int srcX,              /* bit position of source */
         FbBits * dst, FbStride dstStride,      /* FbBits units per scanline */
         int dstX,              /* bit position of dest */
         int dstBpp,            /* bits per destination unit */
         int width,             /* width in bits of destination */
         int height,            /* height in scanlines */
         FbBits fgand,          /* rrop values */
         FbBits fgxor, FbBits bgand, FbBits bgxor)
{
    const FbBits *fbBits;
    FbBits *srcEnd;
    int pixelsPerDst;           /* dst pixels per FbBits */
    int unitsPerSrc;            /* src patterns per FbStip */
    int leftShift, rightShift;  /* align source with dest */
    FbBits startmask, endmask;  /* dest scanline masks */
    FbStip bits = 0, bitsLeft, bitsRight;       /* source bits */
    FbStip left;
    FbBits mask;
    int nDst;                   /* dest longwords (w.o. end) */
    int w;
    int n, nmiddle;
    int dstS;                   /* stipple-relative dst X coordinate */
    Bool copy;                  /* accelerate dest-invariant */
    Bool transparent;           /* accelerate 0 nop */
    int srcinc;                 /* source units consumed */
    Bool endNeedsLoad = FALSE;  /* need load for endmask */
    int startbyte, endbyte;

    if (dstBpp == 24) {
        fbBltOne24(src, srcStride, srcX,
                   dst, dstStride, dstX, dstBpp,
                   width, height, fgand, fgxor, bgand, bgxor);
        return;
    }

    /*
     * Do not read past the end of the buffer!
     */
    srcEnd = src + height * srcStride;

    /*
     * Number of destination units in FbBits == number of stipple pixels
     * used each time
     */
    pixelsPerDst = FB_UNIT / dstBpp;

    /*
     * Number of source stipple patterns in FbStip 
     */
    unitsPerSrc = FB_STIP_UNIT / pixelsPerDst;

    copy = FALSE;
    transparent = FALSE;
    if (bgand == 0 && fgand == 0)
        copy = TRUE;
    else if (bgand == FB_ALLONES && bgxor == 0)
        transparent = TRUE;

    /*
     * Adjust source and dest to nearest FbBits boundary
     */
    src += srcX >> FB_STIP_SHIFT;
    dst += dstX >> FB_SHIFT;
    srcX &= FB_STIP_MASK;
    dstX &= FB_MASK;

    FbMaskBitsBytes(dstX, width, copy,
                    startmask, startbyte, nmiddle, endmask, endbyte);

    /*
     * Compute effective dest alignment requirement for
     * source -- must align source to dest unit boundary
     */
    dstS = dstX / dstBpp;
    /*
     * Compute shift constants for effective alignement
     */
    if (srcX >= dstS) {
        leftShift = srcX - dstS;
        rightShift = FB_STIP_UNIT - leftShift;
    }
    else {
        rightShift = dstS - srcX;
        leftShift = FB_STIP_UNIT - rightShift;
    }
    /*
     * Get pointer to stipple mask array for this depth
     */
    fbBits = 0;                 /* unused */
    if (pixelsPerDst <= 8)
        fbBits = fbStippleTable[pixelsPerDst];

    /*
     * Compute total number of destination words written, but 
     * don't count endmask 
     */
    nDst = nmiddle;
    if (startmask)
        nDst++;

    dstStride -= nDst;

    /*
     * Compute total number of source words consumed
     */

    srcinc = (nDst + unitsPerSrc - 1) / unitsPerSrc;

    if (srcX > dstS)
        srcinc++;
    if (endmask) {
        endNeedsLoad = nDst % unitsPerSrc == 0;
        if (endNeedsLoad)
            srcinc++;
    }

    srcStride -= srcinc;

    /*
     * Copy rectangle
     */
    while (height--) {
        w = nDst;               /* total units across scanline */
        n = unitsPerSrc;        /* units avail in single stipple */
        if (n > w)
            n = w;

        bitsLeft = 0;
        if (srcX > dstS)
            bitsLeft = READ(src++);
        if (n) {
            /*
             * Load first set of stipple bits
             */
            LoadBits;

            /*
             * Consume stipple bits for startmask
             */
            if (startmask) {
                mask = fbBits[FbLeftStipBits(bits, pixelsPerDst)];
                if (mask || !transparent)
                    FbDoLeftMaskByteStippleRRop(dst, mask,
                                                fgand, fgxor, bgand, bgxor,
                                                startbyte, startmask);
                bits = FbStipLeft(bits, pixelsPerDst);
                dst++;
                n--;
                w--;
            }
            /*
             * Consume stipple bits across scanline
             */
            for (;;) {
                w -= n;
                if (copy) {
                    while (n--) {
                        mask = fbBits[FbLeftStipBits(bits, pixelsPerDst)];
                        WRITE(dst, FbOpaqueStipple(mask, fgxor, bgxor));
                        dst++;
                        bits = FbStipLeft(bits, pixelsPerDst);
                    }
                }
                else {
                    while (n--) {
                        left = FbLeftStipBits(bits, pixelsPerDst);
                        if (left || !transparent) {
                            mask = fbBits[left];
                            WRITE(dst, FbStippleRRop(READ(dst), mask, fgand,
                                                     fgxor, bgand, bgxor));
                        }
                        dst++;
                        bits = FbStipLeft(bits, pixelsPerDst);
                    }
                }
                if (!w)
                    break;
                /*
                 * Load another set and reset number of available units
                 */
                LoadBits;
                n = unitsPerSrc;
                if (n > w)
                    n = w;
            }
        }
        /*
         * Consume stipple bits for endmask
         */
        if (endmask) {
            if (endNeedsLoad) {
                LoadBits;
            }
            mask = fbBits[FbLeftStipBits(bits, pixelsPerDst)];
            if (mask || !transparent)
                FbDoRightMaskByteStippleRRop(dst, mask, fgand, fgxor,
                                             bgand, bgxor, endbyte, endmask);
        }
        dst += dstStride;
        src += srcStride;
    }
}

/*
 * Crufty macros to initialize the mask array, most of this
 * is to avoid compile-time warnings about shift overflow
 */

#if BITMAP_BIT_ORDER == MSBFirst
#define Mask24Pos(x,r) ((x)*24-(r))
#else
#define Mask24Pos(x,r) ((x)*24-((r) ? 24 - (r) : 0))
#endif

#define Mask24Neg(x,r)	(Mask24Pos(x,r) < 0 ? -Mask24Pos(x,r) : 0)
#define Mask24Check(x,r)    (Mask24Pos(x,r) < 0 ? 0 : \
			     Mask24Pos(x,r) >= FB_UNIT ? 0 : Mask24Pos(x,r))

#define Mask24(x,r) (Mask24Pos(x,r) < FB_UNIT ? \
		     (Mask24Pos(x,r) < 0 ? \
		      0xffffffU >> Mask24Neg (x,r) : \
		      0xffffffU << Mask24Check(x,r)) : 0)

#define SelMask24(b,n,r)	((((b) >> n) & 1) * Mask24(n,r))

#define C2_24(b,r)  \
    (SelMask24(b,0,r) | \
     SelMask24(b,1,r))

#define FbStip24Len	    2
#if BITMAP_BIT_ORDER == MSBFirst
#define FbStip24New(rot)    (1 + (rot == 0))
#else
#define FbStip24New(rot)    (1 + (rot == 8))
#endif

const FbBits fbStipple24Bits[3][1 << FbStip24Len] = {
    /* rotate 0 */
    {
     C2_24(0, 0), C2_24(1, 0), C2_24(2, 0), C2_24(3, 0),
     },
    /* rotate 8 */
    {
     C2_24(0, 8), C2_24(1, 8), C2_24(2, 8), C2_24(3, 8),
     },
    /* rotate 16 */
    {
     C2_24(0, 16), C2_24(1, 16), C2_24(2, 16), C2_24(3, 16),
     }
};

#if BITMAP_BIT_ORDER == LSBFirst

#define FbMergeStip24Bits(left, right, new) \
	(FbStipLeft (left, new) | FbStipRight ((right), (FbStip24Len - (new))))

#define FbMergePartStip24Bits(left, right, llen, rlen) \
	(left | FbStipRight(right, llen))

#else

#define FbMergeStip24Bits(left, right, new) \
	((FbStipLeft (left, new) & ((1 << FbStip24Len) - 1)) | right)

#define FbMergePartStip24Bits(left, right, llen, rlen) \
	(FbStipLeft(left, rlen) | right)

#endif

#define fbFirstStipBits(len,stip) {\
    int	__len = (len); \
    if (len <= remain) { \
	stip = FbLeftStipBits(bits, len); \
    } else { \
	stip = FbLeftStipBits(bits, remain); \
	bits = (src < srcEnd ? READ(src++) : 0); \
	__len = (len) - remain; \
	stip = FbMergePartStip24Bits(stip, FbLeftStipBits(bits, __len), \
				     remain, __len); \
	remain = FB_STIP_UNIT; \
    } \
    bits = FbStipLeft (bits, __len); \
    remain -= __len; \
}

#define fbInitStipBits(offset,len,stip) {\
    bits = FbStipLeft (READ(src++),offset); \
    remain = FB_STIP_UNIT - offset; \
    fbFirstStipBits(len,stip); \
    stip = FbMergeStip24Bits (0, stip, len); \
}

#define fbNextStipBits(rot,stip) {\
    int	    __new = FbStip24New(rot); \
    FbStip  __right; \
    fbFirstStipBits(__new, __right); \
    stip = FbMergeStip24Bits (stip, __right, __new); \
    rot = FbNext24Rot (rot); \
}

/*
 * Use deep mask tables that incorporate rotation, pull
 * a variable number of bits out of the stipple and
 * reuse the right bits as needed for the next write
 *
 * Yes, this is probably too much code, but most 24-bpp screens
 * have no acceleration so this code is used for stipples, copyplane
 * and text
 */
void
fbBltOne24(FbStip * srcLine, FbStride srcStride,        /* FbStip units per scanline */
           int srcX,            /* bit position of source */
           FbBits * dst, FbStride dstStride,    /* FbBits units per scanline */
           int dstX,            /* bit position of dest */
           int dstBpp,          /* bits per destination unit */
           int width,           /* width in bits of destination */
           int height,          /* height in scanlines */
           FbBits fgand,        /* rrop values */
           FbBits fgxor, FbBits bgand, FbBits bgxor)
{
    FbStip *src, *srcEnd;
    FbBits leftMask, rightMask, mask;
    int nlMiddle, nl;
    FbStip stip, bits;
    int remain;
    int dstS;
    int firstlen;
    int rot0, rot;
    int nDst;

    /*
     * Do not read past the end of the buffer!
     */
    srcEnd = srcLine + height * srcStride;

    srcLine += srcX >> FB_STIP_SHIFT;
    dst += dstX >> FB_SHIFT;
    srcX &= FB_STIP_MASK;
    dstX &= FB_MASK;
    rot0 = FbFirst24Rot(dstX);

    FbMaskBits(dstX, width, leftMask, nlMiddle, rightMask);

    dstS = (dstX + 23) / 24;
    firstlen = FbStip24Len - dstS;

    nDst = nlMiddle;
    if (leftMask)
        nDst++;
    dstStride -= nDst;

    /* opaque copy */
    if (bgand == 0 && fgand == 0) {
        while (height--) {
            rot = rot0;
            src = srcLine;
            srcLine += srcStride;
            fbInitStipBits(srcX, firstlen, stip);
            if (leftMask) {
                mask = fbStipple24Bits[rot >> 3][stip];
                WRITE(dst, (READ(dst) & ~leftMask) |
                      (FbOpaqueStipple(mask,
                                       FbRot24(fgxor, rot), FbRot24(bgxor, rot))
                       & leftMask));
                dst++;
                fbNextStipBits(rot, stip);
            }
            nl = nlMiddle;
            while (nl--) {
                mask = fbStipple24Bits[rot >> 3][stip];
                WRITE(dst, FbOpaqueStipple(mask,
                                           FbRot24(fgxor, rot),
                                           FbRot24(bgxor, rot)));
                dst++;
                fbNextStipBits(rot, stip);
            }
            if (rightMask) {
                mask = fbStipple24Bits[rot >> 3][stip];
                WRITE(dst, (READ(dst) & ~rightMask) |
                      (FbOpaqueStipple(mask,
                                       FbRot24(fgxor, rot), FbRot24(bgxor, rot))
                       & rightMask));
            }
            dst += dstStride;
            src += srcStride;
        }
    }
    /* transparent copy */
    else if (bgand == FB_ALLONES && bgxor == 0 && fgand == 0) {
        while (height--) {
            rot = rot0;
            src = srcLine;
            srcLine += srcStride;
            fbInitStipBits(srcX, firstlen, stip);
            if (leftMask) {
                if (stip) {
                    mask = fbStipple24Bits[rot >> 3][stip] & leftMask;
                    WRITE(dst,
                          (READ(dst) & ~mask) | (FbRot24(fgxor, rot) & mask));
                }
                dst++;
                fbNextStipBits(rot, stip);
            }
            nl = nlMiddle;
            while (nl--) {
                if (stip) {
                    mask = fbStipple24Bits[rot >> 3][stip];
                    WRITE(dst,
                          (READ(dst) & ~mask) | (FbRot24(fgxor, rot) & mask));
                }
                dst++;
                fbNextStipBits(rot, stip);
            }
            if (rightMask) {
                if (stip) {
                    mask = fbStipple24Bits[rot >> 3][stip] & rightMask;
                    WRITE(dst,
                          (READ(dst) & ~mask) | (FbRot24(fgxor, rot) & mask));
                }
            }
            dst += dstStride;
        }
    }
    else {
        while (height--) {
            rot = rot0;
            src = srcLine;
            srcLine += srcStride;
            fbInitStipBits(srcX, firstlen, stip);
            if (leftMask) {
                mask = fbStipple24Bits[rot >> 3][stip];
                WRITE(dst, FbStippleRRopMask(READ(dst), mask,
                                             FbRot24(fgand, rot),
                                             FbRot24(fgxor, rot),
                                             FbRot24(bgand, rot),
                                             FbRot24(bgxor, rot), leftMask));
                dst++;
                fbNextStipBits(rot, stip);
            }
            nl = nlMiddle;
            while (nl--) {
                mask = fbStipple24Bits[rot >> 3][stip];
                WRITE(dst, FbStippleRRop(READ(dst), mask,
                                         FbRot24(fgand, rot),
                                         FbRot24(fgxor, rot),
                                         FbRot24(bgand, rot),
                                         FbRot24(bgxor, rot)));
                dst++;
                fbNextStipBits(rot, stip);
            }
            if (rightMask) {
                mask = fbStipple24Bits[rot >> 3][stip];
                WRITE(dst, FbStippleRRopMask(READ(dst), mask,
                                             FbRot24(fgand, rot),
                                             FbRot24(fgxor, rot),
                                             FbRot24(bgand, rot),
                                             FbRot24(bgxor, rot), rightMask));
            }
            dst += dstStride;
        }
    }
}

/*
 * Not very efficient, but simple -- copy a single plane
 * from an N bit image to a 1 bit image
 */

void
fbBltPlane(FbBits * src,
           FbStride srcStride,
           int srcX,
           int srcBpp,
           FbStip * dst,
           FbStride dstStride,
           int dstX,
           int width,
           int height,
           FbStip fgand,
           FbStip fgxor, FbStip bgand, FbStip bgxor, Pixel planeMask)
{
    FbBits *s;
    FbBits pm;
    FbBits srcMask;
    FbBits srcMaskFirst;
    FbBits srcMask0 = 0;
    FbBits srcBits;

    FbStip dstBits;
    FbStip *d;
    FbStip dstMask;
    FbStip dstMaskFirst;
    FbStip dstUnion;
    int w;
    int wt;
    int rot0;

    if (!width)
        return;

    src += srcX >> FB_SHIFT;
    srcX &= FB_MASK;

    dst += dstX >> FB_STIP_SHIFT;
    dstX &= FB_STIP_MASK;

    w = width / srcBpp;

    pm = fbReplicatePixel(planeMask, srcBpp);
    if (srcBpp == 24) {
        int tmpw = 24;

        rot0 = FbFirst24Rot(srcX);
        if (srcX + tmpw > FB_UNIT)
            tmpw = FB_UNIT - srcX;
        srcMaskFirst = FbRot24(pm, rot0) & FbBitsMask(srcX, tmpw);
    }
    else {
        rot0 = 0;
        srcMaskFirst = pm & FbBitsMask(srcX, srcBpp);
        srcMask0 = pm & FbBitsMask(0, srcBpp);
    }

    dstMaskFirst = FbStipMask(dstX, 1);
    while (height--) {
        d = dst;
        dst += dstStride;
        s = src;
        src += srcStride;

        srcMask = srcMaskFirst;
        if (srcBpp == 24)
            srcMask0 = FbRot24(pm, rot0) & FbBitsMask(0, srcBpp);
        srcBits = READ(s++);

        dstMask = dstMaskFirst;
        dstUnion = 0;
        dstBits = 0;

        wt = w;

        while (wt--) {
            if (!srcMask) {
                srcBits = READ(s++);
                if (srcBpp == 24)
                    srcMask0 = FbNext24Pix(srcMask0) & FbBitsMask(0, 24);
                srcMask = srcMask0;
            }
            if (!dstMask) {
                WRITE(d, FbStippleRRopMask(READ(d), dstBits,
                                           fgand, fgxor, bgand, bgxor,
                                           dstUnion));
                d++;
                dstMask = FbStipMask(0, 1);
                dstUnion = 0;
                dstBits = 0;
            }
            if (srcBits & srcMask)
                dstBits |= dstMask;
            dstUnion |= dstMask;
            if (srcBpp == FB_UNIT)
                srcMask = 0;
            else
                srcMask = FbScrRight(srcMask, srcBpp);
            dstMask = FbStipRight(dstMask, 1);
        }
        if (dstUnion)
            WRITE(d, FbStippleRRopMask(READ(d), dstBits,
                                       fgand, fgxor, bgand, bgxor, dstUnion));
    }
}
