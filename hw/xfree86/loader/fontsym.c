/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/fontsym.c,v 1.11 2002/12/09 17:30:12 dawes Exp $ */

#include "font.h"
#include "sym.h"
#include "fntfilst.h"
#include "fontenc.h"
#ifdef FONTENC_COMPATIBILITY
#include "fontencc.h"
#endif
#include "fntfilio.h"
#include "fntfil.h"
#include "fontutil.h"
#include "fontxlfd.h"
#define _FONTCACHE_SERVER_
#include "fontcache.h"

LOOKUP fontLookupTab[] = {

  SYMFUNC(TwoByteSwap)
  SYMFUNC(FourByteSwap)
  SYMFUNC(FontCouldBeTerminal)
  SYMFUNC(BufFileRead)
  SYMFUNC(BufFileWrite)
  SYMFUNC(CheckFSFormat)
  SYMFUNC(FontFileOpen)
  SYMFUNC(FontFilePriorityRegisterRenderer)
  SYMFUNC(FontFileRegisterRenderer)
  SYMFUNC(FontParseXLFDName)
  SYMFUNC(FontFileCloseFont)
  SYMFUNC(FontFileOpenBitmap)
  SYMFUNC(FontFileCompleteXLFD)
  SYMFUNC(FontFileCountDashes)
  SYMFUNC(FontFileFindNameInDir)
  SYMFUNC(FontFileClose)
  SYMFUNC(FontComputeInfoAccelerators)
  SYMFUNC(FontDefaultFormat)
  SYMFUNC(NameForAtom)
  SYMFUNC(BitOrderInvert)
  SYMFUNC(FontFileMatchRenderer)
  SYMFUNC(RepadBitmap)
  SYMFUNC(FontEncName)
  SYMFUNC(FontEncRecode)
  SYMFUNC(FontEncFind)
  SYMFUNC(FontMapFind)
  SYMFUNC(FontEncMapFind)
  SYMFUNC(FontEncFromXLFD)
  SYMFUNC(FontEncDirectory)
  SYMFUNC(FontMapReverse)
  SYMFUNC(FontMapReverseFree)
  SYMFUNC(CreateFontRec)
  SYMFUNC(DestroyFontRec)
  SYMFUNC(GetGlyphs)
  SYMFUNC(QueryGlyphExtents)
  
  SYMVAR(FontFileBitmapSources)

#ifdef FONTENC_COMPATIBILITY
  /* Obsolete backwards compatibility symbols -- fontencc.c */
  SYMFUNC(font_encoding_from_xlfd)
  SYMFUNC(font_encoding_find)
  SYMFUNC(font_encoding_recode)
  SYMFUNC(font_encoding_name)
  SYMFUNC(identifyEncodingFile)
#endif

  /* fontcache.c */
  SYMFUNC(FontCacheGetSettings)
  SYMFUNC(FontCacheGetStatistics)
  SYMFUNC(FontCacheChangeSettings)
  SYMFUNC(FontCacheOpenCache)
  SYMFUNC(FontCacheCloseCache)
  SYMFUNC(FontCacheSearchEntry)
  SYMFUNC(FontCacheGetEntry)
  SYMFUNC(FontCacheInsertEntry)
  SYMFUNC(FontCacheGetBitmap)

  { 0, 0 },

};
