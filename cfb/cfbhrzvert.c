/***********************************************************

Copyright 1987,1998  The Open Group

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
/* $Xorg: cfbhrzvert.c,v 1.4 2001/02/09 02:04:38 xorgcvs Exp $ */
#include "X.h"

#include "gc.h"
#include "window.h"
#include "pixmap.h"
#include "region.h"

#include "cfb.h"
#include "cfbmskbits.h"

/* horizontal solid line
   abs(len) > 1
*/
cfbHorzS(rop, and, xor, addrl, nlwidth, x1, y1, len)
register int rop;
register unsigned long and;
register unsigned long xor;
register unsigned long *addrl;	/* pointer to base of bitmap */
int nlwidth;		/* width in longwords of bitmap */
int x1;			/* initial point */ 
int y1;
int len;		/* length of line */
{
    register int nlmiddle;
    register unsigned long startmask;
    register unsigned long endmask;

    addrl = addrl + (y1 * nlwidth) + (x1 >> PWSH);

    /* all bits inside same longword */
    if ( ((x1 & PIM) + len) < PPW)
    {
	maskpartialbits(x1, len, startmask);
	*addrl = DoMaskRRop (*addrl, and, xor, startmask);
    }
    else
    {
	maskbits(x1, len, startmask, endmask, nlmiddle);
	if (rop == GXcopy)
	{
	    if (startmask)
	    {
		*addrl = (*addrl & ~startmask) | (xor & startmask);
		addrl++;
	    }
	    while (nlmiddle--)
	    	*addrl++ = xor;
	    if (endmask)
		*addrl = (*addrl & ~endmask) | (xor & endmask);
	}
	else
	{
	    if (startmask)
	    {
		*addrl = DoMaskRRop (*addrl, and, xor, startmask);
		addrl++;
	    }
	    if (rop == GXxor)
	    {
		while (nlmiddle--)
		    *addrl++ ^= xor;
	    }
	    else
	    {
		while (nlmiddle--)
		{
		    *addrl = DoRRop (*addrl, and, xor);
		    addrl++;
		}
	    }
	    if (endmask)
		*addrl = DoMaskRRop (*addrl, and, xor, endmask);
	}
    }
}

/* vertical solid line */

cfbVertS(rop, and, xor, addrl, nlwidth, x1, y1, len)
int rop;
register unsigned long and, xor;
register unsigned long *addrl;	/* pointer to base of bitmap */
register int nlwidth;	/* width in longwords of bitmap */
int x1, y1;		/* initial point */
register int len;	/* length of line */
{
#ifdef PIXEL_ADDR
    register PixelType    *bits = (PixelType *) addrl;

    nlwidth <<= PWSH;
    bits = bits + (y1 * nlwidth) + x1;

    /*
     * special case copy and xor to avoid a test per pixel
     */
    if (rop == GXcopy)
    {
	while (len--)
	{
	    *bits = xor;
	    bits += nlwidth;
	}
    }
    else if (rop == GXxor)
    {
	while (len--)
	{
	    *bits ^= xor;
	    bits += nlwidth;
	}
    }
    else
    {
	while (len--)
	{
	    *bits = DoRRop(*bits, and, xor);
	    bits += nlwidth;
	}
    }
#else /* !PIXEL_ADDR */
    addrl = addrl + (y1 * nlwidth) + (x1 >> PWSH);

    and |= ~cfbmask[x1 & PIM];
    xor &= cfbmask[x1 & PIM];

    while (len--)
    {
	*addrl = DoRRop (*addrl, and, xor);
	addrl += nlwidth;
    }
#endif
}
