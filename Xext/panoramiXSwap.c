/* $Xorg: panoramiXSwap.c,v 1.4 2000/08/17 19:47:57 cpqbld Exp $ */
/*****************************************************************
Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.
******************************************************************/
/* $XFree86: xc/programs/Xserver/Xext/panoramiXSwap.c,v 3.8 2001/08/23 13:01:36 alanh Exp $ */

#include <stdio.h>
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "cursor.h"
#include "cursorstr.h"
#include "extnsionst.h"
#include "dixstruct.h"
#include "gc.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#if 0
#include <sys/workstation.h>
#include <X11/Xserver/ws.h> 
#endif
#include "panoramiX.h"
#include "panoramiXproto.h"
#include "panoramiXsrv.h"
#include "globals.h"


/*
 *	External references for data variables
 */

extern char *ConnectionInfo;
extern int connBlockScreenStart;

#if NeedFunctionPrototypes
#define PROC_EXTERN(pfunc)      extern int pfunc(ClientPtr)
#else
#define PROC_EXTERN(pfunc)      extern int pfunc()
#endif     

PROC_EXTERN(ProcPanoramiXQueryVersion);
PROC_EXTERN(ProcPanoramiXGetState);
PROC_EXTERN(ProcPanoramiXGetScreenCount);
PROC_EXTERN(ProcPanoramiXGetScreenSize);

PROC_EXTERN(ProcXineramaIsActive);
PROC_EXTERN(ProcXineramaQueryScreens);

static int
SProcPanoramiXQueryVersion (ClientPtr client)
{
	REQUEST(xPanoramiXQueryVersionReq);
	register int n;

	swaps(&stuff->length,n);
	REQUEST_SIZE_MATCH (xPanoramiXQueryVersionReq);
	return ProcPanoramiXQueryVersion(client);
}

static int
SProcPanoramiXGetState(ClientPtr client)
{
	REQUEST(xPanoramiXGetStateReq);
	register int n;

 	swaps (&stuff->length, n);	
	REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
	return ProcPanoramiXGetState(client);
}

static int 
SProcPanoramiXGetScreenCount(ClientPtr client)
{
	REQUEST(xPanoramiXGetScreenCountReq);
	register int n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
	return ProcPanoramiXGetScreenCount(client);
}

static int 
SProcPanoramiXGetScreenSize(ClientPtr client)
{
	REQUEST(xPanoramiXGetScreenSizeReq);
	register int n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
	return ProcPanoramiXGetScreenSize(client);
}


static int 
SProcXineramaIsActive(ClientPtr client)
{
	REQUEST(xXineramaIsActiveReq);
	register int n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xXineramaIsActiveReq);
	return ProcXineramaIsActive(client);
}


static int 
SProcXineramaQueryScreens(ClientPtr client)
{
	REQUEST(xXineramaQueryScreensReq);
	register int n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xXineramaQueryScreensReq);
	return ProcXineramaQueryScreens(client);
}


int
SProcPanoramiXDispatch (ClientPtr client)
{   REQUEST(xReq);
    switch (stuff->data)
    {
	case X_PanoramiXQueryVersion:
	     return SProcPanoramiXQueryVersion(client);
	case X_PanoramiXGetState:
	     return SProcPanoramiXGetState(client);
	case X_PanoramiXGetScreenCount:
	     return SProcPanoramiXGetScreenCount(client);
	case X_PanoramiXGetScreenSize:
	     return SProcPanoramiXGetScreenSize(client);
	case X_XineramaIsActive:
	     return SProcXineramaIsActive(client);
	case X_XineramaQueryScreens:
	     return SProcXineramaQueryScreens(client);
    }
    return BadRequest;
}
