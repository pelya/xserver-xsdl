/* $TOG: panoramiX.h /main/4 1998/03/17 06:51:02 kaleb $ */
/****************************************************************
*                                                               *
*    Copyright (c) Digital Equipment Corporation, 1991, 1997    *
*                                                               *
*   All Rights Reserved.  Unpublished rights  reserved  under   *
*   the copyright laws of the United States.                    *
*                                                               *
*   The software contained on this media  is  proprietary  to   *
*   and  embodies  the  confidential  technology  of  Digital   *
*   Equipment Corporation.  Possession, use,  duplication  or   *
*   dissemination of the software and media is authorized only  *
*   pursuant to a valid written license from Digital Equipment  *
*   Corporation.                                                *
*                                                               *
*   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
*   by the U.S. Government is subject to restrictions  as  set  *
*   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
*   or  in  FAR 52.227-19, as applicable.                       *
*                                                               *
*****************************************************************/
/* $XFree86: xc/programs/Xserver/Xext/panoramiX.h,v 1.5 2001/01/03 02:54:17 keithp Exp $ */

/* THIS IS NOT AN X PROJECT TEAM SPECIFICATION */

/*  
 *	PanoramiX definitions
 */

#ifndef _PANORAMIX_H_
#define _PANORAMIX_H_

#include "panoramiXext.h"
#include "gcstruct.h"


typedef struct _PanoramiXData {
    int x;
    int y;
    int width;
    int height;
} PanoramiXData;

typedef struct _PanoramiXInfo {
    XID id ;
} PanoramiXInfo;

typedef struct {
    PanoramiXInfo info[MAXSCREENS];
    RESTYPE type;
    union {
	struct {
	    char   visibility;
	    char   class;
	} win;
	struct {
	    Bool shared;
	} pix;
#ifdef RENDER
	struct {
	    Bool root;
	} pict;
#endif
	char raw_data[4];
    } u;
} PanoramiXRes;

#define FOR_NSCREENS_FORWARD(j) for(j = 0; j < PanoramiXNumScreens; j++)
#define FOR_NSCREENS_BACKWARD(j) for(j = PanoramiXNumScreens - 1; j >= 0; j--)
#define FOR_NSCREENS(j) FOR_NSCREENS_FORWARD(j)

#define BREAK_IF(a) if ((a)) break
#define IF_RETURN(a,b) if ((a)) return (b)

#define FORCE_ROOT(a) { \
    int _j; \
    for (_j = PanoramiXNumScreens - 1; _j; _j--) \
        if ((a).root == WindowTable[_j]->drawable.id)   \
            break;                                      \
    (a).rootX += panoramiXdataPtr[_j].x;             \
    (a).rootY += panoramiXdataPtr[_j].y;             \
    (a).root = WindowTable[0]->drawable.id;          \
}

#define FORCE_WIN(a) {                                  \
    if ((win = PanoramiXFindIDOnAnyScreen(XRT_WINDOW, a))) { \
        (a) = win->info[0].id; /* Real ID */       	   \
    }                                                      \
}

#define FORCE_CMAP(a) {                                  \
    if ((win = PanoramiXFindIDOnAnyScreen(XRT_COLORMAP, a))) { \
        (a) = win->info[0].id; /* Real ID */       	   \
    }                                                      \
}

#define IS_SHARED_PIXMAP(r) (((r)->type == XRT_PIXMAP) && (r)->u.pix.shared)

#define SKIP_FAKE_WINDOW(a) if(!LookupIDByType(a, XRT_WINDOW)) return

#endif /* _PANORAMIX_H_ */
