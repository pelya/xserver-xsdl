/* $Xorg: Pclmap.h,v 1.3 2000/08/17 19:48:08 cpqbld Exp $ */
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

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
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/

#ifndef _PCLMAP_H_
#define _PCLMAP_H_

#ifdef XP_PCL_COLOR
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define NAME(subname) PclCr##subname
#define CATNAME(prefix,subname) prefix##Color##subname
#else
#define NAME(subname) PclCr/**/subname
#define CATNAME(prefix,subname) prefix/**/Color/**/subname
#endif
#endif /* XP_PCL_COLOR */

#ifdef XP_PCL_MONO
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define NAME(subname) PclMn##subname
#define CATNAME(prefix,subname) prefix##Mono##subname
#else
#define NAME(subname) PclMn/**/subname
#define CATNAME(prefix,subname) prefix/**/Mono/**/subname
#endif
#endif /* XP_PCL_MONO */

#ifdef XP_PCL_LJ3
#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define NAME(subname) PclLj3##subname
#define CATNAME(prefix,subname) prefix##Lj3##subname
#else
#define NAME(subname) PclLj3/**/subname
#define CATNAME(prefix,subname) prefix/**/Lj3/**/subname
#endif
#endif /* XP_PCL_LJ3 */

/* PclInit.c */
#define InitializePclDriver		CATNAME(Initialize, PclDriver)
#define PclCloseScreen			NAME(CloseScreen)
#define PclGetContextFromWindow		NAME(GetContextFromWindow)
#define PclScreenPrivateIndex	NAME(ScreenPrivateIndex)
#define PclWindowPrivateIndex	NAME(WindowPrivateIndex)
#define PclContextPrivateIndex	NAME(ContextPrivateIndex)
#define PclPixmapPrivateIndex	NAME(PixmapPrivateIndex)
#define PclGCPrivateIndex	NAME(GCPrivateIndex)

/* PclPrint.c */
#define PclStartJob			NAME(StartJob)
#define PclEndJob			NAME(EndJob)
#define PclStartPage			NAME(StartPage)
#define PclEndPage			NAME(EndPage)
#define PclStartDoc			NAME(StartDoc)
#define PclEndDoc			NAME(EndDoc)
#define PclDocumentData			NAME(DocumentData)
#define PclGetDocumentData		NAME(GetDocumentData)

/* PclWindow.c */
#define PclCreateWindow			NAME(CreateWindow)
#define PclMapWindow			NAME(MapWindow)
#define PclPositionWindow		NAME(PositionWindow)
#define PclUnmapWindow			NAME(UnmapWindow)
#define PclCopyWindow			NAME(CopyWindow)
#define PclChangeWindowAttributes	NAME(ChangeWindowAttributes)
#define PclPaintWindow			NAME(PaintWindow)
#define PclDestroyWindow		NAME(DestroyWindow)

/* PclGC.c */
#define PclCreateGC			NAME(CreateGC)
#define PclDestroyGC			NAME(DestroyGC)
#define PclGetDrawablePrivateStuff	NAME(GetDrawablePrivateStuff)
#define PclSetDrawablePrivateGC		NAME(SetDrawablePrivateGC)
#define PclSendPattern			NAME(SendPattern)
#define PclUpdateDrawableGC		NAME(UpdateDrawableGC)
#define PclComputeCompositeClip		NAME(ComputeCompositeClip)
#define PclValidateGC			NAME(ValidateGC)

/* PclAttr.c */
#define PclGetAttributes		NAME(GetAttributes)
#define PclGetOneAttribute		NAME(GetOneAttribute)
#define PclAugmentAttributes		NAME(AugmentAttributes)
#define PclSetAttributes		NAME(SetAttributes)

/* PclColor.c */
#define PclLookUp			NAME(LookUp)
#define PclCreateDefColormap		NAME(CreateDefColormap)
#define PclCreateColormap		NAME(CreateColormap)
#define PclDestroyColormap		NAME(DestroyColormap)
#define PclInstallColormap		NAME(InstallColormap)
#define PclUninstallColormap		NAME(UninstallColormap)
#define PclListInstalledColormaps	NAME(ListInstalledColormaps)
#define PclStoreColors			NAME(StoreColors)
#define PclResolveColor			NAME(ResolveColor)
#define PclFindPaletteMap		NAME(FindPaletteMap)
#define PclUpdateColormap		NAME(UpdateColormap)
#define PclReadMap			NAME(ReadMap)

/* PclPixmap.c */
#define PclCreatePixmap			NAME(CreatePixmap)
#define PclDestroyPixmap		NAME(DestroyPixmap)

/* PclArc.c */
#define PclDoArc			NAME(DoArc)
#define PclPolyArc			NAME(PolyArc)
#define PclPolyFillArc			NAME(PolyFillArc)

/* PclArea.c */
#define PclPutImage			NAME(PutImage)
#define PclCopyArea			NAME(CopyArea)
#define PclCopyPlane			NAME(CopyPlane)

/* PclLine */
#define PclPolyLine			NAME(PolyLine)
#define PclPolySegment			NAME(PolySegment)

/* PclPixel.c */
#define PclPolyPoint			NAME(PolyPoint)
#define PclPushPixels			NAME(PushPixels)

/* PclPolygon.c */
#define PclPolyRectangle		NAME(PolyRectangle)
#define PclFillPolygon			NAME(FillPolygon)
#define PclPolyFillRect			NAME(PolyFillRect)

/* PclSpans.c */
#define PclFillSpans			NAME(FillSpans)
#define PclSetSpans			NAME(SetSpans)

/* PclText.c */
#define PclPolyText8			NAME(PolyText8)
#define PclPolyText16			NAME(PolyText16)
#define PclImageText8			NAME(ImageText8)
#define PclImageText16			NAME(ImageText16)
#define PclImageGlyphBlt		NAME(ImageGlyphBlt)
#define PclPolyGlyphBlt			NAME(PolyGlyphBlt)
#define PclPolyGlyphBlt			NAME(PolyGlyphBlt)

/* PclFonts.c */
#define PclRealizeFont			NAME(RealizeFont)
#define PclUnrealizeFont		NAME(UnrealizeFont)

/* PclSFonts.c */
#define PclDownloadSoftFont8		NAME(DownloadSoftFont8)
#define PclDownloadSoftFont16		NAME(DownloadSoftFont16)
#define PclCreateSoftFontInfo		NAME(CreateSoftFontInfo)
#define PclDestroySoftFontInfo		NAME(DestroySoftFontInfo)

/* PclMisc.c */
#define PclQueryBestSize		NAME(QueryBestSize)
#define GetPropString			NAME(GetPropString)
#define SystemCmd			NAME(SystemCmd)
#define PclGetMediumDimensions		NAME(GetMediumDimensions)
#define PclGetReproducibleArea		NAME(GetReproducibleArea)
#define PclSpoolFigs			NAME(SpoolFigs)
#define PclSendData			NAME(SendData)

/* PclCursor.c */
#define PclConstrainCursor		NAME(ConstrainCursor)
#define PclCursorLimits			NAME(CursorLimits)
#define PclDisplayCursor		NAME(DisplayCursor)
#define PclRealizeCursor		NAME(RealizeCursor)
#define PclUnrealizeCursor		NAME(UnrealizeCursor)
#define PclRecolorCursor		NAME(RecolorCursor)
#define PclSetCursorPosition		NAME(SetCursorPosition)

#endif /* _PCLMAP_H_ */
