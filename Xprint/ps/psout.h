/* $Xorg: psout.h,v 1.6 2001/02/09 02:04:37 xorgcvs Exp $ */
/*

Copyright 1996, 1998  The Open Group

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

*/
/*
 * (c) Copyright 1996 Hewlett-Packard Company
 * (c) Copyright 1996 International Business Machines Corp.
 * (c) Copyright 1996 Sun Microsystems, Inc.
 * (c) Copyright 1996 Novell, Inc.
 * (c) Copyright 1996 Digital Equipment Corp.
 * (c) Copyright 1996 Fujitsu Limited
 * (c) Copyright 1996 Hitachi, Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the names of the copyright holders
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from said copyright holders.
 */
/* $XFree86: xc/programs/Xserver/Xprint/ps/psout.h,v 1.5 2001/12/21 21:02:06 dawes Exp $ */

/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:          psout.h
**    *
**    *  Contents:      Include file for psout.c
**    *
**    *  Created By:    Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:     Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
**
********************************************************************/

#ifndef _psout_
#define _psout_

#include <stdio.h>

typedef enum PsCapEnum_  { PsCButt=0,   PsCRound, PsCSquare    } PsCapEnum;
typedef enum PsJoinEnum_ { PsJMiter=0,  PsJRound, PsJBevel     } PsJoinEnum;
typedef enum PsArcEnum_  { PsChord,     PsPieSlice             } PsArcEnum;
typedef enum PsRuleEnum_ { PsEvenOdd,   PsNZWinding            } PsRuleEnum;
typedef enum PsFillEnum_ { PsSolid=0, PsTile, PsStip, PsOpStip } PsFillEnum;

typedef struct PsPointRec_
{
  int  x;
  int  y;
} PsPointRec;

typedef PsPointRec *PsPointPtr;

typedef struct PsRectRec_
{
  int  x;
  int  y;
  int  w;
  int  h;
} PsRectRec;

typedef PsRectRec *PsRectPtr;

typedef struct PsArcRec_
{
  int       x;
  int       y;
  int       w;
  int       h;
  int       a1;
  int       a2;
  PsArcEnum style;
} PsArcRec;

typedef PsArcRec *PsArcPtr;

#define PSOUT_RECT    0
#define PSOUT_ARC     1
#define PSOUT_POINTS  2

typedef struct PsElmRec_
{
  int  type;
  int  nPoints;
  union
  {
    PsRectRec  rect;
    PsArcRec   arc;
    PsPointPtr points;
  } c;
} PsElmRec;

typedef PsElmRec *PsElmPtr;

typedef struct PsClipRec_
{
  int        nRects;
  PsRectPtr  rects;
  int        nElms;
  PsElmPtr   elms;
  int        nOutterClips;
  PsRectPtr  outterClips;
} PsClipRec;

typedef PsClipRec *PsClipPtr;

typedef struct PsOutRec_ *PsOutPtr;

extern PsOutPtr PsOut_BeginFile(FILE *fp, int orient, int count, int plex,
                              int res, int wd, int ht, Bool raw);
extern void PsOut_EndFile(PsOutPtr self, int closeFile);
extern void PsOut_BeginPage(PsOutPtr self, int orient, int count, int plex,
                            int res, int wd, int ht);
extern void PsOut_EndPage(PsOutPtr self);
extern void PsOut_DirtyAttributes(PsOutPtr self);
extern void PsOut_Comment(PsOutPtr self, char *comment);
extern void PsOut_Offset(PsOutPtr self, int x, int y);

extern void PsOut_Clip(PsOutPtr self, int clpTyp, PsClipPtr clpinf);

extern void PsOut_Color(PsOutPtr self, int clr);
extern void PsOut_FillRule(PsOutPtr self, PsRuleEnum rule);
extern void PsOut_LineAttrs(PsOutPtr self, int wd, PsCapEnum cap,
                            PsJoinEnum join, int nDsh, int *dsh, int dshOff,
                            int bclr);
extern void PsOut_TextAttrs(PsOutPtr self, char *fnam, int siz, int iso);
extern void PsOut_TextAttrsMtx(PsOutPtr self, char *fnam, float *mtx, int iso);

extern void PsOut_Polygon(PsOutPtr self, int nPts, PsPointPtr pts);
extern void PsOut_FillRect(PsOutPtr self, int x, int y, int w, int h);
extern void PsOut_FillArc(PsOutPtr self, int x, int y, int w, int h,
                          float ang1, float ang2, PsArcEnum style);

extern void PsOut_Lines(PsOutPtr self, int nPts, PsPointPtr pts);
extern void PsOut_Points(PsOutPtr self, int nPts, PsPointPtr pts);
extern void PsOut_DrawRect(PsOutPtr self, int x, int y, int w, int h);
extern void PsOut_DrawArc(PsOutPtr self, int x, int y, int w, int h,
                          float ang1, float ang2);

extern void PsOut_Text(PsOutPtr self, int x, int y, char *text, int textl,
                       int bclr);

extern void PsOut_BeginImage(PsOutPtr self, int bclr, int fclr, int x, int y,
                             int w, int h, int sw, int sh, int format);
extern void PsOut_BeginImageIM(PsOutPtr self, int bclr, int fclr, int x, int y,
                               int w, int h, int sw, int sh, int format);
extern void PsOut_EndImage(PsOutPtr self);
extern void PsOut_OutImageBytes(PsOutPtr self, int nBytes, char *bytes);

extern void PsOut_BeginFrame(PsOutPtr self, int xoff, int yoff, int x, int y,
                             int w, int h);
extern void PsOut_EndFrame(PsOutPtr self);

extern int  PsOut_BeginPattern(PsOutPtr self, void *tag, int w, int h,
                               PsFillEnum type, int bclr, int fclr);
extern void PsOut_EndPattern(PsOutPtr self);
extern void PsOut_SetPattern(PsOutPtr self, void *tag, PsFillEnum type);

extern void PsOut_RawData(PsOutPtr self, char *data, int len);
extern void PsOut_DownloadType1(PsOutPtr self, char *name, char *fname);

#ifdef BM_CACHE
extern void PsOut_BeginImageCache(PsOutPtr self, long cache_id);
extern void PsOut_EndImageCache(PsOutPtr self);
extern void PsOut_ImageCache(PsOutPtr self, int x, int y, long cache_id,
			     int bclr, int fclr);
#endif

extern FILE *PsOut_ChangeFile(PsOutPtr self, FILE *fp);


#endif
