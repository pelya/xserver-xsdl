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


/*
/*
 *	External references for data variables
 */

extern Bool noPanoramiXExtension;
extern Bool PanoramiXVisibilityNotifySent;
extern WindowPtr *WindowTable;
extern int defaultBackingStore;
extern char *ConnectionInfo;
extern int connBlockScreenStart;
extern int (* ProcVector[256]) ();

#if NeedFunctionPrototypes
#define PROC_EXTERN(pfunc)      extern int pfunc(ClientPtr)
#else
#define PROC_EXTERN(pfunc)      extern int pfunc()
#endif     

PROC_EXTERN(ProcPanoramiXQueryVersion);
PROC_EXTERN(ProcPanoramiXGetState);
PROC_EXTERN(ProcPanoramiXGetScreenCount);
PROC_EXTERN(PropPanoramiXGetScreenSize);

static int
#if NeedFunctionPrototypes      
SProcPanoramiXQueryVersion (ClientPtr client)
#else
SProcPanoramiXQueryVersion (client)
    register ClientPtr  client;
#endif
{
    register 	int	n;
    REQUEST(xPanoramiXQueryVersionReq);

    swaps(&stuff->length,n);
    REQUEST_SIZE_MATCH (xPanoramiXQueryVersionReq);
    return ProcPanoramiXQueryVersion(client);
}

static int
#if NeedFunctionPrototypes      
SProcPanoramiXGetState(ClientPtr client)
#else
SProcPanoramiXGetState(client)
        register ClientPtr      client;
#endif
{
	REQUEST(xPanoramiXGetStateReq);
	register int			n;

 	swaps (&stuff->length, n);	
	REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);

}

static int 
#if NeedFunctionPrototypes      
SProcPanoramiXGetScreenCount(ClientPtr client)
#else
SProcPanoramixGetScreenCount(client)
	register ClientPtr	client;
#endif
{
	REQUEST(xPanoramiXGetScreenCountReq);
	register int			n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
	return ProcPanoramiXGetScreenCount(client);
}

static int 
#if NeedFunctionPrototypes      
SProcPanoramiXGetScreenSize(ClientPtr client)
#else 
SProcPanoramiXGetScreenSize(client)
        register ClientPtr      client;
#endif 
{
	REQUEST(xPanoramiXGetScreenSizeReq);
    	WindowPtr			pWin;
	register int			n;

	swaps (&stuff->length, n);
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
	return ProcPanoramiXGetScreenSize(client);
}

int
#if NeedFunctionPrototypes      
SProcPanoramiXDispatch (ClientPtr client)
#else
SProcPanoramiXDispatch (client) 
    ClientPtr   client;
#endif
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
    return BadRequest;
    }
}
