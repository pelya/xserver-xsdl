/* $XFree86: xc/programs/Xserver/ilbm/ilbmimage.c,v 3.1 1998/03/20 21:08:02 hohndel Exp $ */
#include <stdio.h>

/* Modified jun 95 by Geert Uytterhoeven (Geert.Uytterhoeven@cs.kuleuven.ac.be)
   to use interleaved bitplanes instead of normal bitplanes */

#include "X.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "ilbm.h"
#include "maskbits.h"
#include "servermd.h"

void
ilbmPutImage(pDraw, pGC, depth, x, y, width, height, leftPad, format, pImage)
	DrawablePtr pDraw;
	GCPtr pGC;
	int depth, x, y, width, height;
	int leftPad;
	int format;
	char *pImage;
{
	PixmapPtr pPixmap;

#if 1
	fprintf(stderr, "ilbmPutImage()\n");
	fprintf(stderr, "\tdepth = %d, x = %d, y = %d, width = %d, height = %d, "
			  "leftPad = %d\n", depth, x, y, width, height, leftPad);
	switch (format) {
		case XYBitmap:
			fprintf(stderr, "\tformat = XYBitmap\n");
			break;
		case XYPixmap:
			fprintf(stderr, "\tformat = XYPixmap\n");
			break;
		case ZPixmap:
			fprintf(stderr, "\tformat = ZPixmap\n");
			break;
		default:
			fprintf(stderr, "\tformat = %d\n");
			break;
	}
#endif

	if ((width == 0) || (height == 0))
		return;

	if (format != ZPixmap || depth == 1 || pDraw->depth == 1) {
		if (format == XYBitmap) {
			char *ptmp;
			int realwidth;
			int size;
			int aux;
			int d, yy, xx;
			char *ss, *dd;

			realwidth = BitmapBytePad(width+leftPad);
			aux = depth*realwidth;
			size = height*aux;

#if 1
			fprintf(stderr, "\trealwidth = %d, aux = %d, size = %d\n", realwidth,
					  aux, size);
#endif

			if (!(ptmp = (char *)ALLOCATE_LOCAL(size)))
				return;

			/*
			 *		Convert from bitplanes to interleaved bitplanes
			 */

			ss = (char *)pImage;
			for (d = 0; d < depth; d++) {
				dd = ptmp+d*realwidth;
				for (yy = 0; yy < height; yy++) {
					for (xx = 0; xx < realwidth; xx++)
#if 1
					{
						fprintf(stderr, "*(%d) = *(%d)\n", (&dd[xx])-ptmp,
								  ss-(char *)pImage);
#endif
						dd[xx] = *(ss++);
#if 1
					}
#endif
					dd += aux;
				}
			}

			pPixmap = GetScratchPixmapHeader(pDraw->pScreen, width+leftPad, height,
														depth, depth,
														BitmapBytePad(width+leftPad),
														(pointer)ptmp);
			if (!pPixmap) {
				DEALLOCATE_LOCAL(ptmp);
				return;
			}
			pGC->fExpose = FALSE;
			(void)(*pGC->ops->CopyPlane)((DrawablePtr)pPixmap, pDraw, pGC, leftPad,
												  0, width, height, x, y, 1);
			DEALLOCATE_LOCAL(ptmp);
		} else {
#if 0
			/* XXX: bit plane order wronge ! */
			pPixmap->drawable.depth = 1;
			pPixmap->drawable.bitsPerPixel = 1;

			switch (pGC->alu) {
				case GXcopy:
					doBitBlt = ilbmDoBitbltCopy;
					break;
				case GXxor:
					doBitBlt = ilbmDoBitbltXor;
					break;
				case GXcopyInverted:
					doBitBlt = ilbmDoBitbltCopyInverted;
					break;
				case GXor:
					doBitBlt = ilbmDoBitbltOr;
					break;
				default:
					doBitBlt = ilbmDoBitbltGeneral;
					break;
			}

			for (plane = (1L << (pPixmap->drawable.depth - 1)); plane;
				  plane >>= 1) {
				(void)ilbmBitBlt((DrawablePtr)pPixmap, pDraw, pGC, leftPad, 0,
									  width, height, x, y, doBitBlt, plane);
				/* pDraw->devKind += sizeDst; */
			}
#else
			char *ptmp;
			int realwidth;
			int size;
			int aux;
			int d, yy, xx;
			char *ss, *dd;

			realwidth = BitmapBytePad(width+leftPad);
			aux = depth*realwidth;
			size = height*aux;

#if 1
			fprintf(stderr, "\trealwidth = %d, aux = %d, size = %d\n", realwidth,
					  aux, size);
#endif

			if (!(ptmp = (char *)ALLOCATE_LOCAL(size)))
				return;

			/*
			 *		Convert from bitplanes to interleaved bitplanes
			 */

			ss = (char *)pImage;
			for (d = 0; d < depth; d++) {
				dd = ptmp+d*realwidth;
				for (yy = 0; yy < height; yy++) {
					for (xx = 0; xx < realwidth; xx++)
#if 1
					{
						fprintf(stderr, "*(%d) = *(%d)\n", (&dd[xx])-ptmp,
								  ss-(char *)pImage);
#endif
						dd[xx] = *(ss++);
#if 1
					}
#endif
					dd += aux;
				}
			}

			pPixmap = GetScratchPixmapHeader(pDraw->pScreen, width+leftPad, height,
														depth, depth,
														BitmapBytePad(width+leftPad),
														(pointer)ptmp);
			if (!pPixmap) {
				DEALLOCATE_LOCAL(ptmp);
				return;
			}

			pGC->fExpose = FALSE;
			(void)(*pGC->ops->CopyArea)((DrawablePtr)pPixmap, pDraw, pGC, leftPad,
												 0, width, height, x, y);
			DEALLOCATE_LOCAL(ptmp);
#endif
		}

		pGC->fExpose = TRUE;
		FreeScratchPixmapHeader(pPixmap);
	} else {
		/* Chunky to planar conversion required */

		PixmapPtr pPixmap;
		ScreenPtr pScreen = pDraw->pScreen;
		int widthSrc;
		int start_srcshift;
		register int b;
		register int dstshift;
		register int shift_step;
		register PixelType dst;
		register PixelType srcbits;
		register PixelType *pdst;
		register PixelType *psrc;
		int start_bit;
		register int nl;
		register int h;
		register int d;
		int auxDst;
		PixelType *pdstBase;
		int widthDst;
		int depthDst;

		/* Create a tmp pixmap */
		pPixmap = (pScreen->CreatePixmap)(pScreen, width, height, depth);
		if (!pPixmap)
			return;

		ilbmGetPixelWidthAuxDepthAndPointer((DrawablePtr)pPixmap, widthDst,
														auxDst, depthDst, pdstBase);

		widthSrc = PixmapWidthInPadUnits(width, depth);
		/* XXX: if depth == 8, use fast chunky to planar assembly function.*/
		if (depth > 4) {
			start_srcshift = 24;
			shift_step = 8;
		} else {
			start_srcshift = 28;
			shift_step = 4;
		}

		for (d = 0; d < depth; d++, pdstBase += widthDst) {	/* @@@ NEXT PLANE @@@ */
			register PixelType *pdstLine = pdstBase;
			start_bit = start_srcshift + d;
			psrc = (PixelType *)pImage;
			h = height;

			while (h--) {
				pdst = pdstLine;
				pdstLine += auxDst;
				dstshift = PPW - 1;
				dst = 0;
				nl = widthSrc;
				while (nl--) {
					srcbits = *psrc++;
					for (b = start_bit; b >= 0; b -= shift_step) {
						dst |= ((srcbits >> b) & 1) << dstshift;
						if (--dstshift < 0) {
							dstshift = PPW - 1;
							*pdst++ = dst;
							dst = 0;
						}
					}
				}
				if (dstshift != PPW - 1)
					*pdst++ = dst;
			}
		} /* for (d = ...) */

		pGC->fExpose = FALSE;
		(void)(*pGC->ops->CopyArea)((DrawablePtr)pPixmap, pDraw, pGC, leftPad, 0,
											 width, height, x, y);
		pGC->fExpose = TRUE;
		(*pScreen->DestroyPixmap)(pPixmap);
	}
}

void
ilbmGetImage(pDrawable, sx, sy, width, height, format, planemask, pdstLine)
	DrawablePtr pDrawable;
	int sx, sy, width, height;
	unsigned int format;
	unsigned long planemask;
	char *pdstLine;
{
	BoxRec box;
	DDXPointRec ptSrc;
	RegionRec rgnDst;
	ScreenPtr pScreen;
	PixmapPtr pPixmap;

#if 1
	fprintf(stderr, "ilbmGetImage()\n");
	fprintf(stderr, "\tsx = %d, sy = %d, width = %d, height = %d, "
			  "planemask = 0x%08x\n", sx, sy, width, height, planemask);
	switch (format) {
		case XYBitmap:
			fprintf(stderr, "\tformat = XYBitmap\n");
			break;
		case XYPixmap:
			fprintf(stderr, "\tformat = XYPixmap\n");
			break;
		case ZPixmap:
			fprintf(stderr, "\tformat = ZPixmap\n");
			break;
		default:
			fprintf(stderr, "\tformat = %d\n");
			break;
	}
#endif

	if ((width == 0) || (height == 0))
		return;

	pScreen = pDrawable->pScreen;
	sx += pDrawable->x;
	sy += pDrawable->y;

	if (format == XYPixmap || pDrawable->depth == 1) {
		pPixmap = GetScratchPixmapHeader(pScreen, width, height, 1, 1,
													BitmapBytePad(width), (pointer)pdstLine);
		if (!pPixmap)
			return;

		ptSrc.x = sx;
		ptSrc.y = sy;
		box.x1 = 0;
		box.y1 = 0;
		box.x2 = width;
		box.y2 = height;
		REGION_INIT(pScreen, &rgnDst, &box, 1);

		pPixmap->drawable.depth = 1;
		pPixmap->drawable.bitsPerPixel = 1;
		/* dix layer only ever calls GetImage with 1 bit set in planemask
		 * when format is XYPixmap.
		 */
		ilbmDoBitblt(pDrawable, (DrawablePtr)pPixmap, GXcopy, &rgnDst, &ptSrc,
						 planemask);

		FreeScratchPixmapHeader(pPixmap);
		REGION_UNINIT(pScreen, &rgnDst);
	} else {
		/* Planar to chunky conversion required */

		PixelType *psrcBits;
		PixelType *psrcLine;
		PixelType startmask, endmask;
		int depthSrc;
		int widthSrc;
		int auxSrc;
		int sizeDst;
		int widthDst;
		register PixelType *psrc;
		register PixelType *pdst;
		register PixelType dst;
		register PixelType srcbits;
		register int d;
		register int b;
		register int dstshift;
		register int shift_step;
		register int start_endbit;
		int start_startbit;
		register int end_endbit;
		register int start_dstshift;
		register int nl;
		register int h;
		int nlmiddle;

		widthDst = PixmapWidthInPadUnits(width, pDrawable->depth);
		sizeDst = widthDst * height;

		/* Clear the dest image */
		bzero(pdstLine, sizeDst << 2);

		ilbmGetPixelWidthAuxDepthAndPointer(pDrawable, widthSrc, auxSrc,
														depthSrc, psrcBits);

		psrcBits = ilbmScanline(psrcBits, sx, sy, auxSrc);

		start_startbit = PPW - 1 - (sx & PIM);
		if ((sx & PIM) + width < PPW) {
			maskpartialbits(sx, width, startmask);
			nlmiddle = 0;
			endmask = 0;
			start_endbit = PPW - ((sx + width) & PIM);
		} else {
			maskbits(sx, width, startmask, endmask, nlmiddle);
			start_endbit = 0;
			end_endbit = PPW - ((sx + width) & PIM);
		}
		/* ZPixmap images have either 4 or 8 bits per pixel dependent on
		 * depth.
		 */
		if (depthSrc > 4) {
			start_dstshift = 24;
			shift_step = 8;
		} else {
			start_dstshift = 28;
			shift_step = 4;
		}
#define SHIFT_BITS(start_bit,end_bit) \
for (b = (start_bit); b >= (end_bit); b--) { \
	dst |= ((srcbits >> b) & 1) << dstshift; \
	if ((dstshift -= shift_step) < 0) { \
		dstshift = start_dstshift + d; \
		*pdst++ = dst; \
		dst = *pdst; \
	} \
} \

		for (d = 0; d < depthSrc; d++, psrcBits += widthSrc) {	/* @@@ NEXT PLANE @@@ */
			psrcLine = psrcBits;
			pdst = (PixelType *)pdstLine;
			h = height;

			while (h--) {
				psrc = psrcLine;
				psrcLine += auxSrc;
				dst = *pdst;
				dstshift = start_dstshift + d;

				if (startmask) {
					srcbits = *psrc++ & startmask;
					SHIFT_BITS(start_startbit, start_endbit);
				}

				nl = nlmiddle;
				while (nl--) {
					srcbits = *psrc++;
					SHIFT_BITS(PPW - 1, 0);
				}
				if (endmask) {
					srcbits = *psrc & endmask;
					SHIFT_BITS(PPW - 1, end_endbit);
				}

				if (dstshift != start_dstshift + d)
					*pdst++ = dst;
			} /* while (h--) */
		} /* for (d = ...) */
	}
}
