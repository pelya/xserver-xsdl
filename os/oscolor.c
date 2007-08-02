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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define USE_RGB_BUILTIN 1

#if USE_RGB_BUILTIN

#include <X11/keysym.h>
#include "os.h"

static unsigned char
OsToLower (unsigned char a)
{
    if ((a >= XK_A) && (a <= XK_Z))
	return a + (XK_a - XK_A);
    else if ((a >= XK_Agrave) && (a <= XK_Odiaeresis))
	return a + (XK_agrave - XK_Agrave);
    else if ((a >= XK_Ooblique) && (a <= XK_Thorn))
	return a + (XK_oslash - XK_Ooblique);
    else
	return a;
}

static int
OsStrCaseCmp (const unsigned char *s1, const unsigned char *s2, int l2)
{
    unsigned char   c1, c2;

    for (;;)
    {
	c1 = OsToLower (*s1++);
	if (l2 == 0)
	    c2 = '\0';
	else
	    c2 = OsToLower (*s2++);
	if (!c1 || !c2)
	    break;
	if (c1 != c2)
	    break;
	l2--;
    }
    return c2 - c1;
}

typedef struct _builtinColor {
    unsigned char	red;
    unsigned char	green;
    unsigned char	blue;
    unsigned short	name;
} BuiltinColor;

#include "oscolor.h"

#define NUM_BUILTIN_COLORS  (sizeof (BuiltinColors) / sizeof (BuiltinColors[0]))

Bool
OsInitColors(void)
{
    return TRUE;
}

Bool
OsLookupColor(int		screen, 
	      char		*s_name,
	      unsigned int	len, 
	      unsigned short	*pred,
	      unsigned short	*pgreen,
	      unsigned short	*pblue)
{
    const BuiltinColor	*c;
    unsigned char	*name = (unsigned char *) s_name;
    int			low, mid, high;
    int			r;

    low = 0;
    high = NUM_BUILTIN_COLORS - 1;
    while (high >= low)
    {
	mid = (low + high) / 2;
	c = &BuiltinColors[mid];
	r = OsStrCaseCmp (&BuiltinColorNames[c->name], name, len);
	if (r == 0)
	{
	    *pred = c->red * 0x101;
	    *pgreen = c->green * 0x101;
	    *pblue = c->blue * 0x101;
	    return TRUE;
	}
	if (r < 0)
	    high = mid - 1;
	else
	    low = mid + 1;
    }
    return FALSE;
}

#else

/*
 * This file builds the server's internal database mapping color names to
 * RGB tuples by reading in an rgb.txt file.  This is still slightly foolish,
 * rgb.txt hasn't changed in years, we should really include a precompiled
 * version into the server.
 */

#include <stdio.h>
#include "os.h"
#include "opaque.h"

#define HASHSIZE 63

typedef struct _dbEntry * dbEntryPtr;
typedef struct _dbEntry {
  dbEntryPtr     link;
  unsigned short red;
  unsigned short green;
  unsigned short blue;
  char           name[1];	/* some compilers complain if [0] */
} dbEntry;

extern void CopyISOLatin1Lowered(
    unsigned char * /*dest*/,
    unsigned char * /*source*/,
    int /*length*/);

static dbEntryPtr hashTab[HASHSIZE];

static dbEntryPtr
lookup(char *name, int len, Bool create)
{
  unsigned int h = 0, g;
  dbEntryPtr   entry, *prev = NULL;
  char         *str = name;

  if (!(name = (char*)ALLOCATE_LOCAL(len +1))) return NULL;
  CopyISOLatin1Lowered((unsigned char *)name, (unsigned char *)str, len);
  name[len] = '\0';

  for(str = name; *str; str++) {
    h = (h << 4) + *str;
    if ((g = h) & 0xf0000000) h ^= (g >> 24);
    h &= g;
  }
  h %= HASHSIZE;

  if ( (entry = hashTab[h]) )
    {
      for( ; entry; prev = (dbEntryPtr*)entry, entry = entry->link )
	if (! strcmp(name, entry->name) ) break;
    }
  else
    prev = &(hashTab[h]);

  if (!entry && create && (entry = (dbEntryPtr)xalloc(sizeof(dbEntry) +len)))
    {
      *prev = entry;
      entry->link = NULL;
      strcpy( entry->name, name );
    }

  DEALLOCATE_LOCAL(name);

  return entry;
}

Bool
OsInitColors(void)
{
  FILE       *rgb;
  char       *path;
  char       line[BUFSIZ];
  char       name[BUFSIZ];
  int        red, green, blue, lineno = 0;
  dbEntryPtr entry;

  static Bool was_here = FALSE;

  if (!was_here)
    {
      path = (char*)ALLOCATE_LOCAL(strlen(rgbPath) +5);
      strcpy(path, rgbPath);
      strcat(path, ".txt");
      if (!(rgb = fopen(path, "r")))
        {
	   ErrorF( "Couldn't open RGB_DB '%s'\n", rgbPath );
	   DEALLOCATE_LOCAL(path);
	   return FALSE;
	}

      while(fgets(line, sizeof(line), rgb))
	{
	  lineno++;
	  if (sscanf(line,"%d %d %d %[^\n]\n", &red, &green, &blue, name) == 4)
	    {
	      if (red >= 0   && red <= 0xff &&
		  green >= 0 && green <= 0xff &&
		  blue >= 0  && blue <= 0xff)
		{
		  if ((entry = lookup(name, strlen(name), TRUE)))
		    {
		      entry->red   = (red * 65535)   / 255;
		      entry->green = (green * 65535) / 255;
		      entry->blue  = (blue  * 65535) / 255;
		    }
		}
	      else
		ErrorF("Value out of range: %s:%d\n", path, lineno);
	    }
	  else if (*line && *line != '#' && *line != '!')
	    ErrorF("Syntax Error: %s:%d\n", path, lineno);
	}
      
      fclose(rgb);
      DEALLOCATE_LOCAL(path);

      was_here = TRUE;
    }
  return TRUE;
}

Bool
OsLookupColor(int screen, char *name, unsigned int len, 
    unsigned short *pred, unsigned short *pgreen, unsigned short *pblue)
{
  dbEntryPtr entry;

  if ((entry = lookup(name, len, FALSE)))
    {
      *pred   = entry->red;
      *pgreen = entry->green;
      *pblue  = entry->blue;
      return TRUE;
    }

  return FALSE;
}

#endif /* USE_RGB_BUILTIN */
