/* $Xorg: Font.c,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/hw/xnest/Font.c,v 3.5 2001/01/17 22:36:55 dawes Exp $ */

#include "X.h"
#include "Xatom.h"
#include "Xproto.h"
#include "misc.h"
#include "miscstruct.h"
#include "font.h"
#include "fontstruct.h"
#include "scrnintstr.h"

#include "Xnest.h"

#include "Display.h"
#include "XNFont.h"

int xnestFontPrivateIndex;

Bool xnestRealizeFont(pScreen, pFont)
    ScreenPtr pScreen;
    FontPtr pFont;
{
  pointer priv;
  Atom name_atom, value_atom;
  int nprops;
  FontPropPtr props;
  int i;
  char *name;

  FontSetPrivate(pFont, xnestFontPrivateIndex, NULL);

  if (requestingClient && XpClientIsPrintClient(requestingClient, NULL))
      return True;

  name_atom = MakeAtom("FONT", 4, True);
  value_atom = 0L;

  nprops = pFont->info.nprops;
  props = pFont->info.props;

  for (i = 0; i < nprops; i++)
    if (props[i].name == name_atom) {
      value_atom = props[i].value;
      break;
    }

  if (!value_atom) return False;

  name = (char *)NameForAtom(value_atom);

  if (!name) return False;

  priv = (pointer)xalloc(sizeof(xnestPrivFont));  
  FontSetPrivate(pFont, xnestFontPrivateIndex, priv);
  
  xnestFontPriv(pFont)->font_struct = XLoadQueryFont(xnestDisplay, name);

  if (!xnestFontStruct(pFont)) return False;
						     
  return True;
}


Bool xnestUnrealizeFont(pScreen, pFont)
    ScreenPtr pScreen;
    FontPtr pFont;
{
  if (xnestFontPriv(pFont)) {
    if (xnestFontStruct(pFont)) 
      XFreeFont(xnestDisplay, xnestFontStruct(pFont));
    xfree(xnestFontPriv(pFont));
    FontSetPrivate(pFont, xnestFontPrivateIndex, NULL);
  }
  return True;
}
