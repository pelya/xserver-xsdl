/*
 * Copyright © 2004 Eric Anholt
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Eric Anholt not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Eric Anholt makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * ERIC ANHOLT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ERIC ANHOLT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Header$ */

#ifndef _ATI_DRAW_H_
#define _ATI_DRAW_H_

Bool
ATIGetOffsetPitch(ATIScreenInfo *atis, int bpp, CARD32 *pitch_offset,
    int offset, int pitch);

Bool
ATIGetPixmapOffsetPitch(PixmapPtr pPix, CARD32 *pitch_offset);

Bool
R128PrepareBlend(int op, PicturePtr pSrcPicture, PicturePtr pDstPicture,
    PixmapPtr pSrc, PixmapPtr pDst);

void
R128Blend(int srcX, int srcY, int dstX, int dstY, int width, int height);

void
R128DoneBlend(void);

Bool
R128CheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture);

Bool
R128PrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);

void
R128Composite(int srcX, int srcY, int maskX, int maskY, int dstX, int dstY,
    int w, int h);

void
R128DoneComposite(void);

Bool
R100CheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture);

Bool
R100PrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);

Bool
R200CheckComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture);

Bool
R200PrepareComposite(int op, PicturePtr pSrcPicture, PicturePtr pMaskPicture,
    PicturePtr pDstPicture, PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst);

void
RadeonComposite(int srcX, int srcY, int maskX, int maskY, int dstX,
    int dstY, int w, int h);

void
RadeonDoneComposite(void);

void
RadeonSwitchTo2D(ATIScreenInfo *atis);

void
RadeonSwitchTo3D(ATIScreenInfo *atis);

void
ATIWaitIdle(ATIScreenInfo *atis);

#if 0
#define ATI_FALLBACK(x)			\
do {					\
	ErrorF("%s: ", __FUNCTION__);	\
	ErrorF x;			\
	return FALSE;			\
} while (0)
#else
#define ATI_FALLBACK(x) return FALSE
#endif

#endif /* _ATI_DRAW_H_ */
