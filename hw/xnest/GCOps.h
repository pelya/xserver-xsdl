/* $Xorg: GCOps.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifndef XNESTGCOPS_H
#define XNESTGCOPS_H

void xnestFillSpans();
void xnestSetSpans();
void xnestGetSpans();
void xnestPutImage();
void xnestGetImage();
RegionPtr xnestCopyArea();
RegionPtr xnestCopyPlane();
void xnestQueryBestSize();
void xnestPolyPoint();
void xnestPolylines();
void xnestPolySegment();
void xnestPolyRectangle();
void xnestPolyArc();
void xnestFillPolygon();
void xnestPolyFillRect();
void xnestPolyFillArc();
int xnestPolyText8();
int xnestPolyText16();
void xnestImageText8();
void xnestImageText16();
void xnestImageGlyphBlt();
void xnestPolyGlyphBlt();
void xnestPushPixels();

#endif /* XNESTGCOPS_H */
