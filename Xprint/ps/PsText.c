/* $Xorg: PsText.c,v 1.7 2001/02/09 02:04:36 xorgcvs Exp $ */
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

/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		PsText.c
**    *
**    *  Contents:	Character-drawing routines for the PS DDX
**    *
**    *  Created By:	Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:	Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
** 
********************************************************************/
/* $XFree86: xc/programs/Xserver/Xprint/ps/PsText.c,v 1.13 2003/10/29 22:11:55 tsi Exp $ */

#include "Ps.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "fntfilst.h"
#include <sys/stat.h>

static int readFontName(char *fileName, char *file_name, char *dlfnam)
{
    FILE        *file;
    struct stat statb;
    char        buf[256];
    char 	*front, *fn;

    file = fopen(fileName, "r");
    if(file)
    {
        if (fstat (fileno(file), &statb) == -1)
            return 0;
	while(fgets(buf, 255, file))
	{
	    if((fn = strstr(buf, " -")))
	    {
		strcpy(file_name, buf);
		file_name[fn - buf - 4] = '\0';
	        fn++;
	        if((front = strstr(fn, "normal-")))
	        {
		    fn[front - fn] = '\0';
	 	    if(strstr(dlfnam, fn))	
		    {
		        fclose(file);
		        return 1; 
		    }
	        }
	    }
	}
    }
    file_name[0] = '\0';
    fclose(file);
    return 0;
}

int
PsPolyText8(
  DrawablePtr pDrawable,
  GCPtr       pGC,
  int         x,
  int         y,
  int         count,
  char       *string)
{
  if( pDrawable->type==DRAWABLE_PIXMAP )
  {
    DisplayElmPtr   elm;
    PixmapPtr       pix  = (PixmapPtr)pDrawable;
    PsPixmapPrivPtr priv = (PsPixmapPrivPtr)pix->devPrivate.ptr;
    DisplayListPtr  disp;
    GCPtr           gc;

    if ((gc = PsCreateAndCopyGC(pDrawable, pGC)) == NULL) return x;

    disp = PsGetFreeDisplayBlock(priv);

    elm  = &disp->elms[disp->nelms];
    elm->type = Text8Cmd;
    elm->gc   = gc;
    elm->c.text8.x      = x;
    elm->c.text8.y      = y;
    elm->c.text8.count  = count;
    elm->c.text8.string = (char *)xalloc(count);
    memcpy(elm->c.text8.string, string, count);
    disp->nelms += 1;
  }
  else
  {
    char  *fnam, ffname[512], *dlfnam;
    FontDirectoryPtr    dir;
    char        file_name[MAXFONTNAMELEN];

    dir = pGC->font->fpe->private;
    sprintf(ffname, "%s%s", dir->directory, "fonts.dir"); 

    fnam = PsGetPSFontName(pGC->font); 
    if(!fnam){
	if(!(dlfnam = PsGetFontName(pGC->font))) 
	    return x;
	/* If Type1 font, try to download to printer first */
	if(strstr(ffname, "Type1") && readFontName(ffname, file_name, dlfnam)) 
	{
            int          siz;
            float        mtx[4];
            PsOutPtr     psOut;
            ColormapPtr  cMap;

	    if( PsUpdateDrawableGC(pGC, pDrawable, &psOut, &cMap)==FALSE ) 
		return x; 
	    sprintf(ffname, "%s%s%s", dir->directory, file_name, ".pfa");
	    PsOut_DownloadType1(psOut, file_name, ffname);
            PsOut_Offset(psOut, pDrawable->x, pDrawable->y);
       	    PsOut_Color(psOut, PsGetPixelColor(cMap, pGC->fgPixel)); 
       	    siz = PsGetFontSize(pGC->font, mtx);
       	    if( !siz ) PsOut_TextAttrsMtx(psOut, file_name, mtx, 1); 
            else PsOut_TextAttrs(psOut, file_name, siz, 1); 
       	    PsOut_Text(psOut, x, y, string, count, -1);
	    return x;
	}
	{
	    unsigned long n, i;
	    int w;
	    CharInfoPtr charinfo[255];  

	    GetGlyphs(pGC->font, (unsigned long)count, 
		(unsigned char *)string, Linear8Bit,&n, charinfo);
	    w = 0;
	    for (i=0; i < n; i++) w += charinfo[i]->metrics.characterWidth;
	    if (n != 0)
	        PsPolyGlyphBlt(pDrawable, pGC, x, y, n, 
			charinfo, FONTGLYPHS(pGC->font));
	    x += w;
	}  
    }else{
	int          iso;
	int          siz;
	float        mtx[4];
	PsOutPtr     psOut;
	ColormapPtr  cMap;

	if( PsUpdateDrawableGC(pGC, pDrawable, &psOut, &cMap)==FALSE ) return x;
	PsOut_Offset(psOut, pDrawable->x, pDrawable->y);
	PsOut_Color(psOut, PsGetPixelColor(cMap, pGC->fgPixel));
	siz = PsGetFontSize(pGC->font, mtx);
	iso = PsIsISOLatin1Encoding(pGC->font);
	if( !siz ) PsOut_TextAttrsMtx(psOut, fnam, mtx, iso);
	else       PsOut_TextAttrs(psOut, fnam, siz, iso);
	PsOut_Text(psOut, x, y, string, count, -1);
    }
  }
  return x;
}

int
PsPolyText16(
  DrawablePtr     pDrawable,
  GCPtr           pGC,
  int             x,
  int             y,
  int             count,
  unsigned short *string)
{
  if( pDrawable->type==DRAWABLE_PIXMAP )
  {
    DisplayElmPtr   elm;
    PixmapPtr       pix  = (PixmapPtr)pDrawable;
    PsPixmapPrivPtr priv = (PsPixmapPrivPtr)pix->devPrivate.ptr;
    DisplayListPtr  disp;
    GCPtr           gc;

    if ((gc = PsCreateAndCopyGC(pDrawable, pGC)) == NULL) return x;

    disp = PsGetFreeDisplayBlock(priv);

    elm  = &disp->elms[disp->nelms];
    elm->type = Text16Cmd;
    elm->gc   = gc;
    elm->c.text16.x      = x;
    elm->c.text16.y      = y;
    elm->c.text16.count  = count;
    elm->c.text16.string =
      (unsigned short *)xalloc(count*sizeof(unsigned short));
    memcpy(elm->c.text16.string, string, count*sizeof(unsigned short));
    disp->nelms += 1;
  }
  else
  {
    unsigned long n, i;
    int w;
    CharInfoPtr charinfo[255];  /* encoding only has 1 byte for count */

    GetGlyphs(pGC->font, (unsigned long)count, (unsigned char *)string,
              (FONTLASTROW(pGC->font) == 0) ? Linear16Bit : TwoD16Bit,
              &n, charinfo);
    w = 0;
    for (i=0; i < n; i++) w += charinfo[i]->metrics.characterWidth;
    if (n != 0)
	PsPolyGlyphBlt(pDrawable, pGC, x, y, n, charinfo, FONTGLYPHS(pGC->font));
    x += w;
  }
  return x;
}

void
PsImageText8(
  DrawablePtr pDrawable,
  GCPtr       pGC,
  int         x,
  int         y,
  int         count,
  char       *string)
{
  if( pDrawable->type==DRAWABLE_PIXMAP )
  {
    DisplayElmPtr   elm;
    PixmapPtr       pix  = (PixmapPtr)pDrawable;
    PsPixmapPrivPtr priv = (PsPixmapPrivPtr)pix->devPrivate.ptr;
    DisplayListPtr  disp;
    GCPtr           gc;

    if ((gc = PsCreateAndCopyGC(pDrawable, pGC)) == NULL) return;

    disp = PsGetFreeDisplayBlock(priv);

    elm  = &disp->elms[disp->nelms];
    elm->type = TextI8Cmd;
    elm->gc   = gc;
    elm->c.text8.x      = x;
    elm->c.text8.y      = y;
    elm->c.text8.count  = count;
    elm->c.text8.string = (char *)xalloc(count);
    memcpy(elm->c.text8.string, string, count);
    disp->nelms += 1;
  }
  else
  {
    int          iso;
    int          siz;
    float        mtx[4];
    char        *fnam;
    PsOutPtr     psOut;
    ColormapPtr  cMap;

    if( PsUpdateDrawableGC(pGC, pDrawable, &psOut, &cMap)==FALSE ) return;
    PsOut_Offset(psOut, pDrawable->x, pDrawable->y);
    PsOut_Color(psOut, PsGetPixelColor(cMap, pGC->fgPixel));
    fnam = PsGetPSFontName(pGC->font);
    if( !fnam ) fnam = "Times-Roman";
    siz = PsGetFontSize(pGC->font, mtx);
    iso = PsIsISOLatin1Encoding(pGC->font);
    if( !siz ) PsOut_TextAttrsMtx(psOut, fnam, mtx, iso);
    else       PsOut_TextAttrs(psOut, fnam, siz, iso);
    PsOut_Text(psOut, x, y, string, count, PsGetPixelColor(cMap, pGC->bgPixel));
  }
}

void
PsImageText16(
  DrawablePtr     pDrawable,
  GCPtr           pGC,
  int             x,
  int             y,
  int             count,
  unsigned short *string)
{
  if( pDrawable->type==DRAWABLE_PIXMAP )
  {
    DisplayElmPtr   elm;
    PixmapPtr       pix  = (PixmapPtr)pDrawable;
    PsPixmapPrivPtr priv = (PsPixmapPrivPtr)pix->devPrivate.ptr;
    DisplayListPtr  disp;
    GCPtr           gc;

    if ((gc = PsCreateAndCopyGC(pDrawable, pGC)) == NULL) return;

    disp = PsGetFreeDisplayBlock(priv);

    elm  = &disp->elms[disp->nelms];
    elm->type = TextI16Cmd;
    elm->gc   = gc;
    elm->c.text16.x      = x;
    elm->c.text16.y      = y;
    elm->c.text16.count  = count;
    elm->c.text16.string =
      (unsigned short *)xalloc(count*sizeof(unsigned short));
    memcpy(elm->c.text16.string, string, count*sizeof(unsigned short));
    disp->nelms += 1;
  }
  else
  {
    int   i;
    char *str;
    if( !count ) return;
    str = (char *)xalloc(count);
    for( i=0 ; i<count ; i++ ) str[i] = string[i];
    PsImageText8(pDrawable, pGC, x, y, count, str);
    free(str);
  }
}

void
PsImageGlyphBlt(
  DrawablePtr   pDrawable,
  GCPtr         pGC,
  int           x,
  int           y,
  unsigned int  nGlyphs,
  CharInfoPtr  *pCharInfo,
  pointer       pGlyphBase)
{
  /* NOT TO BE IMPLEMENTED */
}

void
PsPolyGlyphBlt(
  DrawablePtr   pDrawable,
  GCPtr         pGC,
  int           x,
  int           y,
  unsigned int  nGlyphs,
  CharInfoPtr  *pCharInfo,
  pointer       pGlyphBase)
{
    int width, height;
    PixmapPtr pPixmap = NullPixmap;
    int nbyLine;                        /* bytes per line of padded pixmap */
    FontPtr pfont;
    GCPtr pGCtmp;
    register int i;
    register int j;
    unsigned char *pbits;               /* buffer for PutImage */
    register unsigned char *pb;         /* temp pointer into buffer */
    register CharInfoPtr pci;           /* currect char info */
    register unsigned char *pglyph;     /* pointer bits in glyph */
    int gWidth, gHeight;                /* width and height of glyph */
    register int nbyGlyphWidth;         /* bytes per scanline of glyph */
    int nbyPadGlyph;                    /* server padded line of glyph */
    int w;
    XID gcvals[3];

    pfont = pGC->font;
    width = FONTMAXBOUNDS(pfont,rightSideBearing) -
            FONTMINBOUNDS(pfont,leftSideBearing);
    height = FONTMAXBOUNDS(pfont,ascent) +
             FONTMAXBOUNDS(pfont,descent);

    if ((width == 0) || (height == 0) )
        return;
    {
        int i;
        w = 0;
        for (i=0; i < nGlyphs; i++) w += pCharInfo[i]->metrics.characterWidth;
    }
    pGCtmp = GetScratchGC(1, pDrawable->pScreen);
    if (!pGCtmp)
    {
        (*pDrawable->pScreen->DestroyPixmap)(pPixmap);
        return;
    }

    gcvals[0] = GXcopy;
    gcvals[1] = pGC->fgPixel;
    gcvals[2] = pGC->bgPixel; 

    DoChangeGC(pGCtmp, GCFunction|GCForeground|GCBackground, gcvals, 0);

    
    nbyLine = BitmapBytePad(width);
    pbits = (unsigned char *)ALLOCATE_LOCAL(height*nbyLine);
    if (!pbits){
        PsDestroyPixmap(pPixmap);
        return;
    }
    while(nGlyphs--)
    {
        pci = *pCharInfo++;
        pglyph = FONTGLYPHBITS(pGlyphBase, pci);
        gWidth = GLYPHWIDTHPIXELS(pci);
        gHeight = GLYPHHEIGHTPIXELS(pci);
        if (gWidth && gHeight)
        {
            nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(pci);
            nbyPadGlyph = BitmapBytePad(gWidth);

            if (nbyGlyphWidth == nbyPadGlyph
#if GLYPHPADBYTES != 4
                && (((int) pglyph) & 3) == 0
#endif
                )
            {
                pb = pglyph;
            }
            else
            {
                for (i=0, pb = pbits; i<gHeight; i++, pb = pbits+(i*nbyPadGlyph))
                    for (j = 0; j < nbyGlyphWidth; j++)
                        *pb++ = *pglyph++;
                pb = pbits;
            }

	    PsPutImageMask((DrawablePtr)pDrawable, pGCtmp, 
		   1, x + pci->metrics.leftSideBearing, 
		   y - pci->metrics.ascent, gWidth, gHeight,
                   0, XYBitmap, (char *)pb);
	    x  += pci->metrics.characterWidth;
	}
    }
    DEALLOCATE_LOCAL(pbits);
    FreeScratchGC(pGCtmp);
}
