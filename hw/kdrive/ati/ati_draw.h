/*
 * $Id$
 *
 * Copyright © 2003 Eric Anholt
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

#ifdef USE_DRI

#define DMA_PACKET0( reg, n )						\
	(RADEON_CP_PACKET0 | ((n) << 16) | ((reg) >> 2))

#define RING_LOCALS	CARD32 *__head; int __count;

#define BEGIN_RING( n )							\
do {									\
	if (atis->indirectBuffer == NULL) {				\
		atis->indirectBuffer = ATIDMAGetBuffer();		\
		atis->indirectStart = 0;				\
	} else if ((atis->indirectBuffer->used + 4*(n)) >		\
	    atis->indirectBuffer->total) {				\
		ATIDMAFlushIndirect(1);					\
	}								\
	__head = (pointer)((char *)atis->indirectBuffer->address +	\
	    atis->indirectBuffer->used);				\
	__count = 0;							\
} while (0)

#define ADVANCE_RING() do {						\
	atis->indirectBuffer->used += __count * (int)sizeof(CARD32);	\
} while (0)

#define OUT_RING(x) do {						\
	MMIO_OUT32(&__head[__count++], 0, (x));				\
} while (0)

#define OUT_RING_REG(reg, val)						\
do {									\
	OUT_RING(DMA_PACKET0(reg, 0));					\
	OUT_RING(val);							\
} while (0)

drmBufPtr ATIDMAGetBuffer(void);
void ATIDMAFlushIndirect(Bool discard);
void ATIDMADispatchIndirect(Bool discard);
void ATIDMAStart(ScreenPtr pScreen);
void ATIDMAStop(ScreenPtr pScreen);

#endif /* USE_DRI */

#if 0
#define ATI_FALLBACK(x)		\
do {				\
	ErrorF x;		\
	return FALSE;		\
} while (0)
#else
#define ATI_FALLBACK(x) return FALSE
#endif

#endif /* _ATI_DRAW_H_ */
