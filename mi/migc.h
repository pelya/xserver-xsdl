/* $Xorg: migc.h,v 1.4 2001/02/09 02:05:21 xorgcvs Exp $ */
/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

/* This structure has to line up with the mfb and cfb gc private structures so
 * that when it is superimposed on them, the three fields that migc.c needs to
 * see will be accessed correctly.  I know this is not beautiful, but it seemed
 * better than all the code duplication in cfb and mfb.
 */
typedef struct {
    unsigned char       pad1;
    unsigned char       pad2;
    unsigned char       pad3;
    unsigned		pad4:1;
    unsigned		freeCompClip:1;
    PixmapPtr		pRotatedPixmap;
    RegionPtr		pCompositeClip;
} miPrivGC;

extern int miGCPrivateIndex;

extern void miRegisterGCPrivateIndex(
#if NeedFunctionPrototypes
    int /*gcindex*/
#endif
);

extern void miChangeGC(
#if NeedFunctionPrototypes
    GCPtr  /*pGC*/,
    unsigned long /*mask*/
#endif
);

extern void miDestroyGC(
#if NeedFunctionPrototypes
    GCPtr  /*pGC*/
#endif
);

extern GCOpsPtr miCreateGCOps(
#if NeedFunctionPrototypes
    GCOpsPtr /*prototype*/
#endif
);

extern void miDestroyGCOps(
#if NeedFunctionPrototypes
    GCOpsPtr /*ops*/
#endif
);

extern void miDestroyClip(
#if NeedFunctionPrototypes
    GCPtr /*pGC*/
#endif
);

extern void miChangeClip(
#if NeedFunctionPrototypes
    GCPtr   /*pGC*/,
    int     /*type*/,
    pointer /*pvalue*/,
    int     /*nrects*/
#endif
);

extern void miCopyClip(
#if NeedFunctionPrototypes
    GCPtr /*pgcDst*/,
    GCPtr /*pgcSrc*/
#endif
);

extern void miCopyGC(
#if NeedFunctionPrototypes
    GCPtr /*pGCSrc*/,
    unsigned long /*changes*/,
    GCPtr /*pGCDst*/
#endif
);

extern void miComputeCompositeClip(
#if NeedFunctionPrototypes
    GCPtr       /*pGC*/,
    DrawablePtr /*pDrawable*/
#endif
);
