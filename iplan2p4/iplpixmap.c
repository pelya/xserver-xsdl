/* $XFree86: xc/programs/Xserver/iplan2p4/iplpixmap.c,v 3.1 2001/12/17 20:00:46 dawes Exp $ */
/* $XConsortium: iplpixmap.c,v 5.14 94/04/17 20:28:56 dpw Exp $ */
/***********************************************************

Copyright (c) 1987  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* pixmap management
   written by drewry, september 1986

   on a monchrome device, a pixmap is a bitmap.
*/

/* Modified nov 94 by Martin Schaller (Martin_Schaller@maus.r.de) for use with
interleaved planes */

#include "Xmd.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "mi.h"
#include "ipl.h"
#include "iplmskbits.h"

extern unsigned long endtab[];

PixmapPtr
iplCreatePixmap (pScreen, width, height, depth)
    ScreenPtr	pScreen;
    int		width;
    int		height;
    int		depth;
{
    PixmapPtr pPixmap;
    int datasize;
    int paddedWidth;
    int ipad=INTER_PLANES*2 - 1;

    paddedWidth = PixmapBytePad(width, depth);
    paddedWidth = (paddedWidth + ipad) & ~ipad;
    datasize = height * paddedWidth;
    pPixmap = AllocatePixmap(pScreen, datasize);
    if (!pPixmap)
	return NullPixmap;
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.class = 0;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.depth = depth;
    pPixmap->drawable.bitsPerPixel = BitsPerPixel(depth);
    pPixmap->drawable.id = 0;
    pPixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pPixmap->drawable.x = 0;
    pPixmap->drawable.y = 0;
    pPixmap->drawable.width = width;
    pPixmap->drawable.height = height;
    pPixmap->devKind = paddedWidth;
    pPixmap->refcnt = 1;
#ifdef PIXPRIV
    pPixmap->devPrivate.ptr = datasize ?
		(pointer)((char *)pPixmap + pScreen->totalPixmapSize) : NULL;
#else
    pPixmap->devPrivate.ptr = (pointer)((long)(pPixmap + 1));
#endif
    return pPixmap;
}

Bool
iplDestroyPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    if(--pPixmap->refcnt)
	return TRUE;
    xfree(pPixmap);
    return TRUE;
}

PixmapPtr
iplCopyPixmap(pSrc)
    register PixmapPtr	pSrc;
{
    register PixmapPtr	pDst;
    int		size;
    ScreenPtr pScreen;

    size = pSrc->drawable.height * pSrc->devKind;
    pScreen = pSrc->drawable.pScreen;
    pDst = (*pScreen->CreatePixmap) (pScreen, pSrc->drawable.width, 
				pSrc->drawable.height, pSrc->drawable.depth);
    if (!pDst)
	return NullPixmap;
    memmove((char *)pDst->devPrivate.ptr, (char *)pSrc->devPrivate.ptr, size);
    return pDst;
}


/* replicates a pattern to be a full 32 bits wide.
   relies on the fact that each scnaline is longword padded.
   doesn't do anything if pixmap is not a factor of 32 wide.
   changes width field of pixmap if successful, so that the fast
	iplXRotatePixmap code gets used if we rotate the pixmap later.
	iplYRotatePixmap code gets used if we rotate the pixmap later.

   calculate number of times to repeat
   for each scanline of pattern
      zero out area to be filled with replicate
      left shift and or in original as many times as needed
*/

void
iplPadPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    register int width = pPixmap->drawable.width;
    register int h;
    register unsigned short mask;
    register unsigned short *p;
    register unsigned short bits; /* real pattern bits */
    register int i;
    int rep;                    /* repeat count for pattern */
 
    if (width >= INTER_PGSZ)
        return;

    rep = INTER_PGSZ/width;
/*    if (rep*width != INTER_PGSZ)
        return; */
 
    mask = iplendtab[width];
 
    p = (unsigned short *)(pPixmap->devPrivate.ptr);
    for (h=0; h < pPixmap->drawable.height * INTER_PLANES; h++)
    {
	*p &= mask;
	bits = *p ;
        for(i=1; i<rep; i++)
        {
#if (BITMAP_BIT_ORDER == MSBFirst) 
            bits >>= width;
#else
	    bits <<= width;
#endif
	    *p |= bits; 
        }
        p++;
    }    
    pPixmap->drawable.width = rep*width; /* PGSZ/(pPixmap->drawable.bitsPerPixel); */
}


#ifdef notdef
/*
 * ipl debugging routine -- assumes pixmap is 1 byte deep 
 */
static ipldumppixmap(pPix)
    PixmapPtr	pPix;
{
    unsigned int *pw;
    char *psrc, *pdst;
    int	i, j;
    char	line[66];

    ErrorF(  "pPixmap: 0x%x\n", pPix);
    ErrorF(  "%d wide %d high\n", pPix->drawable.width, pPix->drawable.height);
    if (pPix->drawable.width > 64)
    {
	ErrorF(  "too wide to see\n");
	return;
    }

    pw = (unsigned int *) pPix->devPrivate.ptr;
    psrc = (char *) pw;

/*
    for ( i=0; i<pPix->drawable.height; ++i )
	ErrorF( "0x%x\n", pw[i] );
*/

    for ( i = 0; i < pPix->drawable.height; ++i ) {
	pdst = line;
	for(j = 0; j < pPix->drawable.width; j++) {
	    *pdst++ = *psrc++ ? 'X' : ' ' ;
	}
	*pdst++ = '\n';
	*pdst++ = '\0';
	ErrorF( "%s", line);
    }
}
#endif /* notdef */

/* Rotates pixmap pPix by w pixels to the right on the screen. Assumes that
 * words are PGSZ bits wide, and that the least significant bit appears on the
 * left.
 */
void
iplXRotatePixmap(pPix, rw)
    PixmapPtr	pPix;
    register int rw;
{
    INTER_DECLAREG(*pw);
    INTER_DECLAREG(*pwFinal);
    INTER_DECLAREGP(t);
    int				rot;

    if (pPix == NullPixmap)
        return;

    switch (((DrawablePtr) pPix)->bitsPerPixel) {
	case INTER_PLANES:
	    break;
	case 1:
	    mfbXRotatePixmap(pPix, rw);
	    return;
	default:
	    ErrorF("iplXRotatePixmap: unsupported bitsPerPixel %d\n", ((DrawablePtr) pPix)->bitsPerPixel);
	    return;
    }
    pw = (unsigned short *)pPix->devPrivate.ptr;
    modulus (rw, (int) pPix->drawable.width, rot);
    if(pPix->drawable.width == 16)
    {
        pwFinal = pw + pPix->drawable.height * INTER_PLANES;
	while(pw < pwFinal)
	{
	    INTER_COPY(pw, t);
	    INTER_MSKINSM(iplendtab[rot], INTER_PPG-rot, t,
			  ~0, rot, t, pw)
	    INTER_NEXT_GROUP(pw);
	}
    }
    else
    {
        ErrorF("ipl internal error: trying to rotate odd-sized pixmap.\n");
#ifdef notdef
	register unsigned long *pwTmp;
	int size, tsize;

	tsize = PixmapBytePad(pPix->drawable.width - rot, pPix->drawable.depth);
	pwTmp = (unsigned long *) ALLOCATE_LOCAL(pPix->drawable.height * tsize);
	if (!pwTmp)
	    return;
	/* divide pw (the pixmap) in two vertically at (w - rot) and swap */
	tsize >>= 2;
	size = pPix->devKind >> SIZE0F(PixelGroup);
	iplQuickBlt((long *)pw, (long *)pwTmp,
		    0, 0, 0, 0,
		    (int)pPix->drawable.width - rot, (int)pPix->drawable.height,
		    size, tsize);
	iplQuickBlt((long *)pw, (long *)pw,
		    (int)pPix->drawable.width - rot, 0, 0, 0,
		    rot, (int)pPix->drawable.height,
		    size, size);
	iplQuickBlt((long *)pwTmp, (long *)pw,
		    0, 0, rot, 0,
		    (int)pPix->drawable.width - rot, (int)pPix->drawable.height,
		    tsize, size);
	DEALLOCATE_LOCAL(pwTmp);
#endif
    }
}

/* Rotates pixmap pPix by h lines.  Assumes that h is always less than
   pPix->drawable.height
   works on any width.
 */
void
iplYRotatePixmap(pPix, rh)
    register PixmapPtr	pPix;
    int	rh;
{
    int nbyDown;	/* bytes to move down to row 0; also offset of
			   row rh */
    int nbyUp;		/* bytes to move up to line rh; also
			   offset of first line moved down to 0 */
    char *pbase;
    char *ptmp;
    int	rot;

    if (pPix == NullPixmap)
	return;
    switch (((DrawablePtr) pPix)->bitsPerPixel) {
	case INTER_PLANES:
	    break;
	case 1:
	    mfbYRotatePixmap(pPix, rh);
	    return;
	default:
	    ErrorF("iplYRotatePixmap: unsupported bitsPerPixel %d\n", ((DrawablePtr) pPix)->bitsPerPixel);
	    return;
    }

    modulus (rh, (int) pPix->drawable.height, rot);
    pbase = (char *)pPix->devPrivate.ptr;

    nbyDown = rot * pPix->devKind;
    nbyUp = (pPix->devKind * pPix->drawable.height) - nbyDown;
    if(!(ptmp = (char *)ALLOCATE_LOCAL(nbyUp)))
	return;

    memmove(ptmp, pbase, nbyUp);		/* save the low rows */
    memmove(pbase, pbase+nbyUp, nbyDown);	/* slide the top rows down */
    memmove(pbase+nbyDown, ptmp, nbyUp);	/* move lower rows up to row rot */
    DEALLOCATE_LOCAL(ptmp);
}

void
iplCopyRotatePixmap(psrcPix, ppdstPix, xrot, yrot)
    register PixmapPtr psrcPix, *ppdstPix;
    int	xrot, yrot;
{
    register PixmapPtr pdstPix;

    if ((pdstPix = *ppdstPix) &&
	(pdstPix->devKind == psrcPix->devKind) &&
	(pdstPix->drawable.height == psrcPix->drawable.height))
    {
	memmove((char *)pdstPix->devPrivate.ptr,
		(char *)psrcPix->devPrivate.ptr,
	      psrcPix->drawable.height * psrcPix->devKind);
	pdstPix->drawable.width = psrcPix->drawable.width;
	pdstPix->drawable.depth = psrcPix->drawable.depth;
	pdstPix->drawable.bitsPerPixel = psrcPix->drawable.bitsPerPixel;
	pdstPix->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    }
    else
    {
	if (pdstPix)
	    /* FIX XBUG 6168 */
	    (*pdstPix->drawable.pScreen->DestroyPixmap)(pdstPix);
	*ppdstPix = pdstPix = iplCopyPixmap(psrcPix);
	if (!pdstPix)
	    return;
    }
    iplPadPixmap(pdstPix);
    if (xrot)
	iplXRotatePixmap(pdstPix, xrot);
    if (yrot)
	iplYRotatePixmap(pdstPix, yrot);
}
