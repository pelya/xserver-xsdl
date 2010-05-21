/***********************************************************

Copyright 1987, 1998  The Open Group

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

#ifndef REGIONSTRUCT_H
#define REGIONSTRUCT_H

typedef struct pixman_region16 RegionRec, *RegionPtr;

#include "miscstruct.h"

/* Return values from RectIn() */

#define rgnOUT 0
#define rgnIN  1
#define rgnPART 2

#define NullRegion ((RegionPtr)0)

/*
 *   clip region
 */

typedef struct pixman_region16_data RegDataRec, *RegDataPtr;

extern _X_EXPORT BoxRec RegionEmptyBox;
extern _X_EXPORT RegDataRec RegionEmptyData;
extern _X_EXPORT RegDataRec RegionBrokenData;

#define RegionNil(reg) ((reg)->data && !(reg)->data->numRects)
/* not a region */
#define RegionNar(reg)	((reg)->data == &RegionBrokenData)
#define RegionNumRects(reg) ((reg)->data ? (reg)->data->numRects : 1)
#define RegionSize(reg) ((reg)->data ? (reg)->data->size : 0)
#define RegionRects(reg) ((reg)->data ? (BoxPtr)((reg)->data + 1) \
			               : &(reg)->extents)
#define RegionBoxptr(reg) ((BoxPtr)((reg)->data + 1))
#define RegionBox(reg,i) (&RegionBoxptr(reg)[i])
#define RegionTop(reg) RegionBox(reg, (reg)->data->numRects)
#define RegionEnd(reg) RegionBox(reg, (reg)->data->numRects - 1)
#define RegionSizeof(n) (sizeof(RegDataRec) + ((n) * sizeof(BoxRec)))

#define RegionCreate(_rect, _size) \
    RegionCreate(_rect, _size)

#define RegionCopy(dst, src) \
    RegionCopy(dst, src)

#define RegionDestroy(_pReg) \
    RegionDestroy(_pReg)

#define RegionIntersect(newReg, reg1, reg2) \
    RegionIntersect(newReg, reg1, reg2)

#define RegionUnion(newReg, reg1, reg2) \
    RegionUnion(newReg, reg1, reg2)

#define RegionSubtract(newReg, reg1, reg2) \
    RegionSubtract(newReg, reg1, reg2)

#define RegionInverse(newReg, reg1, invRect) \
    RegionInverse(newReg, reg1, invRect)

#define RegionTranslate(_pReg, _x, _y) \
    RegionTranslate(_pReg, _x, _y)

#define RegionContainsRect(_pReg, prect) \
    RegionContainsRect(_pReg, prect)

#define RegionContainsPoint(_pReg, _x, _y, prect) \
    RegionContainsPoint(_pReg, _x, _y, prect)

#define RegionAppend(dstrgn, rgn) \
    RegionAppend(dstrgn, rgn)

#define RegionValidate(badreg, pOverlap) \
    RegionValidate(badreg, pOverlap)

#define BitmapToRegion(_pScreen, pPix) \
    (*(_pScreen)->BitmapToRegion)(pPix) /* no mi version?! */

#define RegionFromRects(nrects, prect, ctype) \
    RegionFromRects(nrects, prect, ctype)

#define RegionEqual(_pReg1, _pReg2) \
    RegionEqual(_pReg1, _pReg2)

#define RegionBreak(_pReg) \
    RegionBreak(_pReg)

#define RegionInit(_pReg, _rect, _size) \
{ \
    if ((_rect) != NULL)				\
    { \
        (_pReg)->extents = *(_rect); \
        (_pReg)->data = (RegDataPtr)NULL; \
    } \
    else \
    { \
        (_pReg)->extents = RegionEmptyBox; \
        if (((_size) > 1) && ((_pReg)->data = \
                             (RegDataPtr)malloc(RegionSizeof(_size)))) \
        { \
            (_pReg)->data->size = (_size); \
            (_pReg)->data->numRects = 0; \
        } \
        else \
            (_pReg)->data = &RegionEmptyData; \
    } \
 }


#define RegionUninit(_pReg) \
{ \
    if ((_pReg)->data && (_pReg)->data->size) { \
	free((_pReg)->data); \
	(_pReg)->data = NULL; \
    } \
}

#define RegionReset(_pReg, _pBox) \
{ \
    (_pReg)->extents = *(_pBox); \
    RegionUninit(_pReg); \
    (_pReg)->data = (RegDataPtr)NULL; \
}

#define RegionNotEmpty(_pReg) \
    !RegionNil(_pReg)

#define RegionBroken(_pReg) \
    RegionNar(_pReg)

#define RegionEmpty(_pReg) \
{ \
    RegionUninit(_pReg); \
    (_pReg)->extents.x2 = (_pReg)->extents.x1; \
    (_pReg)->extents.y2 = (_pReg)->extents.y1; \
    (_pReg)->data = &RegionEmptyData; \
}

#define RegionExtents(_pReg) \
    (&(_pReg)->extents)

#define RegionNull(_pReg) \
{ \
    (_pReg)->extents = RegionEmptyBox; \
    (_pReg)->data = &RegionEmptyData; \
}

/* moved from mi.h */

extern _X_EXPORT void InitRegions (void);

extern _X_EXPORT RegionPtr RegionCreate(
    BoxPtr /*rect*/,
    int /*size*/);

extern _X_EXPORT void RegionDestroy(
    RegionPtr /*pReg*/);

extern _X_EXPORT Bool RegionCopy(
    RegionPtr /*dst*/,
    RegionPtr /*src*/);

extern _X_EXPORT Bool RegionIntersect(
    RegionPtr /*newReg*/,
    RegionPtr /*reg1*/,
    RegionPtr /*reg2*/);

extern _X_EXPORT Bool RegionUnion(
    RegionPtr /*newReg*/,
    RegionPtr /*reg1*/,
    RegionPtr /*reg2*/);

extern _X_EXPORT Bool RegionAppend(
    RegionPtr /*dstrgn*/,
    RegionPtr /*rgn*/);

extern _X_EXPORT Bool RegionValidate(
    RegionPtr /*badreg*/,
    Bool * /*pOverlap*/);

extern _X_EXPORT RegionPtr RegionFromRects(
    int /*nrects*/,
    xRectanglePtr /*prect*/,
    int /*ctype*/);

extern _X_EXPORT Bool RegionSubtract(
    RegionPtr /*regD*/,
    RegionPtr /*regM*/,
    RegionPtr /*regS*/);

extern _X_EXPORT Bool RegionInverse(
    RegionPtr /*newReg*/,
    RegionPtr /*reg1*/,
    BoxPtr /*invRect*/);

extern _X_EXPORT int RegionContainsRect(
    RegionPtr /*region*/,
    BoxPtr /*prect*/);

extern _X_EXPORT void RegionTranslate(
    RegionPtr /*pReg*/,
    int /*x*/,
    int /*y*/);

extern _X_EXPORT Bool RegionBreak(
    RegionPtr /*pReg*/);

extern _X_EXPORT Bool RegionContainsPoint(
    RegionPtr /*pReg*/,
    int /*x*/,
    int /*y*/,
    BoxPtr /*box*/);

extern _X_EXPORT Bool RegionEqual(
    RegionPtr /*pReg1*/,
    RegionPtr /*pReg2*/);

extern _X_EXPORT Bool RegionRectAlloc(
    RegionPtr /*pRgn*/,
    int /*n*/
);

#ifdef DEBUG
extern _X_EXPORT Bool RegionIsValid(
    RegionPtr /*prgn*/
);
#endif

extern _X_EXPORT void RegionPrint(
    RegionPtr /*pReg*/);

extern _X_EXPORT int RegionClipSpans(
    RegionPtr /*prgnDst*/,
    DDXPointPtr /*ppt*/,
    int * /*pwidth*/,
    int /*nspans*/,
    DDXPointPtr /*pptNew*/,
    int * /*pwidthNew*/,
    int /*fSorted*/
);

#endif /* REGIONSTRUCT_H */
