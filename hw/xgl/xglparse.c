/*
 * Copyright © 2004 David Reveman
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * David Reveman not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * David Reveman makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * DAVID REVEMAN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL DAVID REVEMAN BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"

char *
xglParseFindNext (char *cur,
		  char *delim,
		  char *save,
		  char *last)
{
    while (*cur && !strchr (delim, *cur))
	*save++ = *cur++;
    
    *save = 0;
    *last = *cur;
    
    if (*cur)
	cur++;
    
    return cur;
}

void
xglParseScreen (xglScreenInfoPtr pScreenInfo,
		char	         *arg)
{
    char delim;
    char save[1024];
    int	 i, pixels, mm;
    
    pScreenInfo->width    = 0;
    pScreenInfo->height   = 0;
    pScreenInfo->widthMm  = 0;
    pScreenInfo->heightMm = 0;
    
    if (!arg)
	return;
    
    if (strlen (arg) >= sizeof (save))
	return;
    
    for (i = 0; i < 2; i++)
    {
	arg = xglParseFindNext (arg, "x/@XY", save, &delim);
	if (!save[0])
	    return;
	
	pixels = atoi (save);
	mm = 0;
	
	if (delim == '/')
	{
	    arg = xglParseFindNext (arg, "x@XY", save, &delim);
	    if (!save[0])
		return;
	    
	    mm = atoi (save);
	}
	
	if (i == 0)
	{
	    pScreenInfo->width   = pixels;
	    pScreenInfo->widthMm = mm;
	}
	else
	{
	    pScreenInfo->height   = pixels;
	    pScreenInfo->heightMm = mm;
	}
	
	if (delim != 'x')
	    return;
    }
}

void
xglUseMsg (void)
{
    ErrorF ("-screen WIDTH[/WIDTHMM]xHEIGHT[/HEIGHTMM] "
	    "specify screen characteristics\n");
    ErrorF ("-fullscreen            run fullscreen\n");
    ErrorF ("-vertextype [short|float] set vertex data type\n");
    ErrorF ("-vbostream             "
	    "use vertex buffer objects for streaming of vertex data\n");
    ErrorF ("-yinverted             Y is upside-down\n");
    ErrorF ("-pbomask [1|4|8|16|32] "
	    "set bpp's to use with pixel buffer objects\n");
}

int
xglProcessArgument (xglScreenInfoPtr pScreenInfo,
		    int		     argc,
		    char	     **argv,
		    int		     i)
{
    if (!strcmp (argv[i], "-screen"))
    {
	if ((i + 1) < argc)
	{
	    xglParseScreen (pScreenInfo, argv[i + 1]);
	}
	else
	    return 1;
	
	return 2;
    }
    else if (!strcmp (argv[i], "-fullscreen"))
    {
	pScreenInfo->fullscreen = TRUE;
	return 1;
    }
    else if (!strcmp (argv[i], "-vertextype"))
    {
	if ((i + 1) < argc)
	{
	    if (!strcasecmp (argv[i + 1], "short"))
		pScreenInfo->geometryDataType = GEOMETRY_DATA_TYPE_SHORT;
	    else if (!strcasecmp (argv[i + 1], "float"))
		pScreenInfo->geometryDataType = GEOMETRY_DATA_TYPE_FLOAT;
	}
	else
	    return 1;
	
	return 2;
    }
    else if (!strcmp (argv[i], "-vbostream"))
    {
	pScreenInfo->geometryUsage = GEOMETRY_USAGE_STREAM;
	return 1;
    }
    else if (!strcmp (argv[i], "-yinverted"))
    {
	pScreenInfo->yInverted = TRUE;
	return 1;
    }
    else if (!strcmp (argv[i], "-pbomask"))
    {
	if ((i + 1) < argc)
	{
	    pScreenInfo->pboMask = atoi (argv[i + 1]);
	}
	else
	    return 1;
	
	return 2;
    }
    
    return 0;
}
