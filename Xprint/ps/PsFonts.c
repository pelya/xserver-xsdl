/* $Xorg: PsFonts.c,v 1.6 2001/03/06 16:30:15 pookie Exp $ */
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
/* $XFree86: xc/programs/Xserver/Xprint/ps/PsFonts.c,v 1.7 2003/07/16 01:38:34 dawes Exp $ */

/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		PsFonts.c
**    *
**    *  Contents:	Font code for PS driver.
**    *
**    *  Created By:	Roger Helmendach (Liberty Systems)
**    *
**    *  Copyright:	Copyright 1996 The Open Group, Inc.
**    *
**    *********************************************************
** 
********************************************************************/

#include "regionstr.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "fontxlfd.h"

#include "Ps.h"

Bool
PsRealizeFont(
  ScreenPtr  pscr,
  FontPtr    pFont)
{
  return TRUE;
}

Bool
PsUnrealizeFont(
  ScreenPtr  pscr,
  FontPtr    pFont)
{
  return TRUE;
}

char *
PsGetFontName(FontPtr pFont)
{
  int         i;
  int         nprops = pFont->info.nprops;
  FontPropPtr props  = pFont->info.props;
  Atom        name   = MakeAtom("FONT", 4, True);
  Atom        value  = (Atom)0;

  for( i=0 ; i<nprops ; i++ )
  {
    if( (Atom)props[i].name == name )
      { value = props[i].value; break; }
  }
  if( !value ) return (char *)0;
  return NameForAtom(value);
}

int
PsGetFontSize(FontPtr pFont, float *mtx)
{
  FontScalableRec   vals;
  char             *name = PsGetFontName(pFont);
  int               value = 0;

  FontParseXLFDName(name, &vals, FONT_XLFD_REPLACE_NONE);
  if( vals.values_supplied&PIXELSIZE_ARRAY )
  {
    int  i;
    for( i=0 ; i<4 ; i++ )
      mtx[i] = (float)vals.pixel_matrix[i];
  }
  else
  {
    value = vals.pixel;
    if( !value ) value = 20;
  }
  return value;
}

char *
PsGetPSFontName(FontPtr pFont)
{
  int         i;
  int         nprops = pFont->info.nprops;
  FontPropPtr props  = pFont->info.props;
  Atom        name   = MakeAtom("PRINTER_RESIDENT_FONT", 21, True);
  Atom        value  = (Atom)0;

  for( i=0 ; i<nprops ; i++ )
  {
    if( (Atom)props[i].name == name )
      { value = props[i].value; break; }
  }
  if( !value ) return (char *)0; 
  return NameForAtom(value);
}

int
PsIsISOLatin1Encoding(FontPtr pFont)
{
  int          i;
  int          nprops = pFont->info.nprops;
  FontPropPtr  props  = pFont->info.props;
  Atom         reg = MakeAtom("CHARSET_REGISTRY", 16, True);
  Atom         enc = MakeAtom("CHARSET_ENCODING", 16, True);
  Atom         rv = 0, ev = 0;
  char        *rp = 0;
  char        *ep = 0;

  for( i=0 ; i<nprops ; i++ )
  {
    if( (Atom)props[i].name == reg ) rv = props[i].value;
    if( (Atom)props[i].name == enc ) ev = props[i].value;
  }
  if( rv ) rp = NameForAtom(rv);
  if( ev ) ep = NameForAtom(ev);
  if( (!rp) || (!ep) ) return(0);
  if( (char)tolower(rp[0])!='i' ||
      (char)tolower(rp[1])!='s' ||
      (char)tolower(rp[2])!='o' ||
      memcmp(&rp[3], "8859", 4)!=0 ||
      ep[0]!='1' ) return(0);
  return(1);
}
