/* $Xorg: xkbmisc.c,v 1.4 2000/08/17 19:46:44 cpqbld Exp $ */
/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be 
 used in advertising or publicity pertaining to distribution 
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability 
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.
 
 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL 
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, 
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE 
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/
/* $XFree86: xc/lib/xkbfile/xkbmisc.c,v 1.7 2003/07/16 02:31:10 dawes Exp $ */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xos.h>
#include <X11/Xfuncs.h>

#include <X11/X.h>
#define	NEED_EVENTS
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "inputstr.h"
#include "dix.h"
#include <X11/extensions/XKBstr.h>
#define XKBSRV_NEED_FILE_FUNCS	1
#include <X11/extensions/XKBsrv.h>
#include <X11/extensions/XKBgeom.h>
#include "xkb.h"

/***===================================================================***/

static Bool
XkbWriteSectionFromName(FILE *file,char *sectionName,char *name)
{
    fprintf(file,"    xkb_%-20s { include \"%s\" };\n",sectionName,name);
    ErrorF("    xkb_%-20s { include \"%s\" };\n",sectionName,name);
    return True;
}

Bool
XkbWriteXKBKeymapForNames(	FILE *			file,
				XkbComponentNamesPtr	names,
				Display *		dpy,
				XkbDescPtr		xkb,
				unsigned		want,
				unsigned		need)
{
    if (!names || (!names->keycodes && !names->types && !names->compat &&
                   !names->symbols && !names->geometry))
        return False;

    fprintf(file, "xkb_keymap \"%s\" {\n", names->keymap ? names->keymap :
                                                           "default");

    if (names->keycodes)
	XkbWriteSectionFromName(file, "keycodes", names->keycodes);
    if (names->types)
	XkbWriteSectionFromName(file, "types", names->types);
    if (names->compat)
	XkbWriteSectionFromName(file, "compatibility", names->compat);
    if (names->symbols)
	XkbWriteSectionFromName(file, "symbols", names->symbols);
    if (names->geometry)
	XkbWriteSectionFromName(file, "geometry", names->geometry);

    fprintf(file,"};\n");

    return True;
}

unsigned
XkbConvertGetByNameComponents(Bool toXkm,unsigned orig)
{
unsigned	rtrn;

    rtrn= 0;
    if (toXkm) {
	if (orig&XkbGBN_TypesMask)		rtrn|= XkmTypesMask;
	if (orig&XkbGBN_CompatMapMask)		rtrn|= XkmCompatMapMask;
	if (orig&XkbGBN_SymbolsMask)		rtrn|= XkmSymbolsMask;
	if (orig&XkbGBN_IndicatorMapMask)	rtrn|= XkmIndicatorsMask;
	if (orig&XkbGBN_KeyNamesMask)		rtrn|= XkmKeyNamesMask;
	if (orig&XkbGBN_GeometryMask)		rtrn|= XkmGeometryMask;
    }
    else {
	if (orig&XkmTypesMask)			rtrn|= XkbGBN_TypesMask;
	if (orig&XkmCompatMapMask)		rtrn|= XkbGBN_CompatMapMask;
	if (orig&XkmSymbolsMask)		rtrn|= XkbGBN_SymbolsMask;
	if (orig&XkmIndicatorsMask)		rtrn|= XkbGBN_IndicatorMapMask;
	if (orig&XkmKeyNamesMask)		rtrn|= XkbGBN_KeyNamesMask;
	if (orig&XkmGeometryMask)		rtrn|= XkbGBN_GeometryMask;
	if (orig!=0)				rtrn|= XkbGBN_OtherNamesMask;
    }
    return rtrn;
}

Bool
XkbDetermineFileType(XkbFileInfoPtr finfo,int format,int *opts_missing)
{
unsigned	present;
XkbDescPtr	xkb;

    if ((!finfo)||(!finfo->xkb))
	return False;
    if (opts_missing)
	*opts_missing= 0;
    xkb= finfo->xkb;
    present= 0;
    if ((xkb->names)&&(xkb->names->keys))	present|= XkmKeyNamesMask;
    if ((xkb->map)&&(xkb->map->types))		present|= XkmTypesMask;
    if (xkb->compat)				present|= XkmCompatMapMask;
    if ((xkb->map)&&(xkb->map->num_syms>1))	present|= XkmSymbolsMask;
    if (xkb->indicators)			present|= XkmIndicatorsMask;
    if (xkb->geom)				present|= XkmGeometryMask;
    if (!present)
	return False;
    else switch (present) {
	case XkmKeyNamesMask:	
	    finfo->type= 	XkmKeyNamesIndex;
	    finfo->defined= 	present;
	    return True;
	case XkmTypesMask:
	    finfo->type=	XkmTypesIndex;
	    finfo->defined= 	present;
	    return True;
	case XkmCompatMapMask:	
	    finfo->type=	XkmCompatMapIndex;
	    finfo->defined=	present;
	    return True;
	case XkmSymbolsMask:	
	    if (format!=XkbXKMFile) {
		finfo->type= 	XkmSymbolsIndex;
		finfo->defined=	present;
		return True;
	    }
	    break;
	case XkmGeometryMask:	
	    finfo->type=	XkmGeometryIndex;
	    finfo->defined=	present;
	    return True;
    }
    if ((present&(~XkmSemanticsLegal))==0) {
	if ((XkmSemanticsRequired&present)==XkmSemanticsRequired) {
	    if (opts_missing)
		*opts_missing= XkmSemanticsOptional&(~present);
	    finfo->type= 	XkmSemanticsFile;
	    finfo->defined=	present;
	    return True;
	}
    }
    else if ((present&(~XkmLayoutLegal))==0) {
	if ((XkmLayoutRequired&present)==XkmLayoutRequired) {
	    if (opts_missing)
		*opts_missing= XkmLayoutOptional&(~present);
	    finfo->type=	XkmLayoutFile;
	    finfo->defined=	present;
	    return True;
	}
    }
    else if ((present&(~XkmKeymapLegal))==0) {
	if ((XkmKeymapRequired&present)==XkmKeymapRequired) {
	    if (opts_missing)
		*opts_missing= XkmKeymapOptional&(~present);
	    finfo->type=	XkmKeymapFile;
	    finfo->defined=	present;
	    return True;
	}
    }
    return False;
}

/* all latin-1 alphanumerics, plus parens, slash, minus, underscore and */
/* wildcards */

static unsigned char componentSpecLegal[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
	0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

void
XkbEnsureSafeMapName(char *name)
{
   if (name==NULL)
        return;
    while (*name!='\0') {
	if ((componentSpecLegal[(*name)/8]&(1<<((*name)%8)))==0)
	    *name= '_';
        name++;
    }
    return;
}

/***====================================================================***/

#define	UNMATCHABLE(c)	(((c)=='(')||((c)==')')||((c)=='/'))

Bool
XkbNameMatchesPattern(char *name,char *ptrn)
{
    while (ptrn[0]!='\0') {
	if (name[0]=='\0') {
	    if (ptrn[0]=='*') {
		ptrn++;
		continue;
	    }
	    return False;
	}
	if (ptrn[0]=='?') {
	    if (UNMATCHABLE(name[0]))
		return False;
	}
	else if (ptrn[0]=='*') {
	    if ((!UNMATCHABLE(name[0]))&&XkbNameMatchesPattern(name+1,ptrn))
		return True;
	    return XkbNameMatchesPattern(name,ptrn+1);
	}
	else if (ptrn[0]!=name[0])
	    return False;
	name++;
	ptrn++;
    }
    /* if we get here, the pattern is exhausted (-:just like me:-) */
    return (name[0]=='\0');
}
