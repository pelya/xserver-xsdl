/* $Xorg: panoramiXprocs.c,v 1.5 2000/08/17 19:47:57 cpqbld Exp $ */
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
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "windowstr.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "opaque.h"
#include "inputstr.h"
#include "migc.h"
#include "misc.h"
#include "dixstruct.h"
#include "panoramiX.h"

extern Bool noPanoramiXExtension;
extern Bool PanoramiXVisibilityNotifySent;
extern Bool PanoramiXMapped;
extern int PanoramiXNumScreens;
extern int PanoramiXPixWidth;
extern int PanoramiXPixHeight;
extern PanoramiXWindow *PanoramiXWinRoot;
extern PanoramiXGC   *PanoramiXGCRoot;
extern PanoramiXCmap *PanoramiXCmapRoot;
extern PanoramiXPmap *PanoramiXPmapRoot;
extern PanoramiXData *panoramiXdataPtr;
extern PanoramiXCDT   PanoramiXColorDepthTable[MAXSCREENS];
extern ScreenInfo *GlobalScrInfo;
extern int (* SavedProcVector[256])();
extern void (* ReplySwapVector[256])();
extern WindowPtr *WindowTable;
extern char *ConnectionInfo;
extern int connBlockScreenStart;

extern XID clientErrorValue;

extern void Swap32Write();

extern long defaultScreenSaverTime;
extern long defaultScreenSaverInterval;
extern int  defaultScreenSaverBlanking;
extern int  defaultScreenSaverAllowExposures;
static ClientPtr onlyClient;
static Bool grabbingClient = FALSE;
#ifdef __alpha /* THIS NEEDS TO BE LONG !!!! Change driver! */
int	*checkForInput[2];
#else
long	*checkForInput[2];
#endif
extern int connBlockScreenStart;

extern int (* InitialVector[3]) ();
extern int (* ProcVector[256]) ();
extern int (* SwappedProcVector[256]) ();
extern void (* EventSwapVector[128]) ();
extern void (* ReplySwapVector[256]) ();
extern void Swap32Write(), SLHostsExtend(), SQColorsExtend(), 
WriteSConnectionInfo();
extern void WriteSConnSetupPrefix();
extern char *ClientAuthorized();
extern Bool InsertFakeRequest();
static void KillAllClients();
static void DeleteClientFromAnySelections();
extern void ProcessWorkQueue();


static int nextFreeClientID; /* always MIN free client ID */

static int	nClients;	/* number active clients */

char isItTimeToYield;

/* Various of the DIX function interfaces were not designed to allow
 * the client->errorValue to be set on BadValue and other errors.
 * Rather than changing interfaces and breaking untold code we introduce
 * a new global that dispatch can use.
 */
XID clientErrorValue;   /* XXX this is a kludge */


#define SAME_SCREENS(a, b) (\
    (a.pScreen == b.pScreen))



extern int Ones();

int PanoramiXCreateWindow(register ClientPtr client)
{
    register WindowPtr 	pParent, pWin;
    REQUEST(xCreateWindowReq);
    int 		result, j = 0;
    unsigned 		len;
    Bool		FoundIt = FALSE;
    Window 		winID;
    Window 		parID;
    PanoramiXWindow 	*localWin;
    PanoramiXWindow 	*parentWin = PanoramiXWinRoot;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXCmap 	*pPanoramiXCmap = NULL;
    PanoramiXPmap         *pBackgndPmap = NULL;
    PanoramiXPmap         *pBorderPmap = NULL;
    VisualID            orig_visual;
    XID			orig_wid;
    int			orig_x, orig_y;
    register Mask	orig_mask;
    int 		cmap_offset = 0;
    int                 pback_offset = 0;
    int                 pbord_offset = 0;
    int                 class_index, this_class_index;
    int                 vid_index, this_vid_index;

    REQUEST_AT_LEAST_SIZE(xCreateWindowReq);
    
    len = client->req_len - (sizeof(xCreateWindowReq) >> 2);
    IF_RETURN((Ones((Mask)stuff->mask) != len), BadLength);
    orig_mask = stuff->mask;
    PANORAMIXFIND_ID(parentWin, stuff->parent);
    if (parentWin) {
	localWin = (PanoramiXWindow *)Xcalloc(sizeof(PanoramiXWindow));
	IF_RETURN(!localWin, BadAlloc);
    } else {
	return BadWindow;
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->mask & CWBackPixmap)) {
	XID pmapID;

	pback_offset = Ones((Mask)stuff->mask & (CWBackPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pback_offset);
	if (pmapID) {
	    pBackgndPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBackgndPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->mask & CWBorderPixmap)) {
	XID pmapID;

	pbord_offset = Ones((Mask)stuff->mask & (CWBorderPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pbord_offset);
	if (pmapID) {
	    pBorderPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBorderPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->mask & CWColormap)) {
	Colormap cmapID;

	cmap_offset = Ones((Mask)stuff->mask & (CWColormap - 1));
	cmapID = *((CARD32 *) &stuff[1] + cmap_offset);
	if (cmapID) {
	    pPanoramiXCmap = PanoramiXCmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXCmap, cmapID);
	}
    }
    orig_x = stuff->x;
    orig_y = stuff->y;
    orig_wid = stuff->wid;
    orig_visual = stuff->visual;
    for (j = 0; j <= PanoramiXNumScreens - 1; j++) {
	winID = j ? FakeClientID(client->index) : orig_wid;
	localWin->info[j].id = winID;
    }
    localWin->FreeMe = FALSE;
    localWin->visibility = VisibilityNotViewable;
    localWin->VisibilitySent = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXWin, PanoramiXWinRoot);
    pPanoramiXWin->next = localWin;
    if ( (stuff->visual != CopyFromParent) && (stuff->depth != 0))
      {
       /* Find the correct visual for this screen */
	for (class_index = 0; class_index < PanoramiXColorDepthTable[0].numVisuals; 
class_index++)
	  {
	    for (vid_index = 0; vid_index < PanoramiXColorDepthTable[0].panoramiXScreenMap[class_index].vmap[stuff->depth].numVids; vid_index++)
	      {
		if ( stuff->visual == PanoramiXColorDepthTable[0].panoramiXScreenMap[class_index].vmap[stuff->depth].vid[vid_index] )
		{
		  this_class_index = class_index;
		  this_vid_index   = vid_index;
	          FoundIt = TRUE;
		  break;
		}
	       }
	   }
      }
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	if (parentWin == PanoramiXWinRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	stuff->wid = localWin->info[j].id;
	parID = (XID)(parentWin->info[j].id);
	pParent = (WindowPtr)SecurityLookupWindow(parID, client,SecurityReadAccess);
	IF_RETURN((!pParent),BadWindow);
	stuff->parent = parID;
	if ( (orig_visual != CopyFromParent) && (stuff->depth != 0) && FoundIt ) 
	  {
	    stuff->visual = PanoramiXColorDepthTable[j].panoramiXScreenMap[this_class_index].vmap[stuff->depth].vid[this_vid_index];
	  }
	if (pBackgndPmap)
	    *((CARD32 *) &stuff[1] + pback_offset) = pBackgndPmap->info[j].id;
	if (pBorderPmap)
	    *((CARD32 *) &stuff[1] + pbord_offset) = pBorderPmap->info[j].id;
	if (pPanoramiXCmap)
	    *((CARD32 *) &stuff[1] + cmap_offset) = pPanoramiXCmap->info[j].id;
	stuff->mask = orig_mask;
	result = (*SavedProcVector[X_CreateWindow])(client);
	BREAK_IF(result != Success); 
    }
    if (result != Success) {
       pPanoramiXWin->next = NULL;
       if (localWin)
           Xfree(localWin);
    }
    return (result);
}



int PanoramiXChangeWindowAttributes(register ClientPtr client)
{
    register WindowPtr 	pWin;
    REQUEST(xChangeWindowAttributesReq);
    register int  	result;
    int 	  	len;
    int 	  	j;
    Window 	  	winID;
    Mask		orig_valueMask;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow   	*pPanoramiXWinback = NULL;
    PanoramiXCmap 	*pPanoramiXCmap = NULL;
    PanoramiXPmap	*pBackgndPmap = NULL;
    PanoramiXPmap	*pBorderPmap = NULL;
    int			cmap_offset = 0;
    int 		pback_offset = 0;
    int			pbord_offset = 0;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xChangeWindowAttributesReq);
    len = client->req_len - (sizeof(xChangeWindowAttributesReq) >> 2);
    IF_RETURN((len != Ones((Mask) stuff->valueMask)), BadLength);
    orig_valueMask = stuff->valueMask;
    winID = stuff->window;
    for (; pPanoramiXWin && (pPanoramiXWin->info[0].id != stuff->window);
						pPanoramiXWin = pPanoramiXWin->next)
	pPanoramiXWinback = pPanoramiXWin;
    pPanoramiXWin = PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, winID);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->valueMask & CWBackPixmap)) {
	XID pmapID;

	pback_offset = Ones((Mask)stuff->valueMask & (CWBackPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pback_offset);
	if (pmapID) {
	    pBackgndPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBackgndPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->valueMask & CWBorderPixmap)) {
	XID pmapID;

	pbord_offset = Ones((Mask)stuff->valueMask & (CWBorderPixmap - 1));
	pmapID = *((CARD32 *) &stuff[1] + pbord_offset);
	if (pmapID) {
	    pBorderPmap = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pBorderPmap, pmapID);
	}
    }
    if ((PanoramiXNumScreens - 1) && ((Mask)stuff->valueMask & CWColormap )) {
	Colormap cmapID;

	cmap_offset = Ones((Mask)stuff->valueMask & (CWColormap - 1));
	cmapID = *((CARD32 *) &stuff[1] + cmap_offset);
	if (cmapID) {
	    pPanoramiXCmap = PanoramiXCmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXCmap, cmapID);
	}
    }
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	stuff->valueMask = orig_valueMask;  
	if (pBackgndPmap)
	    *((CARD32 *) &stuff[1] + pback_offset) = pBackgndPmap->info[j].id;
	if (pBorderPmap)
	    *((CARD32 *) &stuff[1] + pbord_offset) = pBorderPmap->info[j].id;
	if (pPanoramiXCmap)
	    *((CARD32 *) &stuff[1] + cmap_offset) = pPanoramiXCmap->info[j].id;
	result = (*SavedProcVector[X_ChangeWindowAttributes])(client);
        BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXWinback && 
	        pPanoramiXWin && pPanoramiXWin->FreeMe) {
	pPanoramiXWinback->next = pPanoramiXWin->next;
	Xfree(pPanoramiXWin);
    } 
    return (result);
}


int PanoramiXDestroyWindow(ClientPtr client)
{
    REQUEST(xResourceReq);
    int         	j, result;
    PanoramiXWindow   	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow   	*pPanoramiXWinback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXWin && (pPanoramiXWin->info[0].id != stuff->id);
						pPanoramiXWin = pPanoramiXWin->next)
	pPanoramiXWinback = pPanoramiXWin;
    IF_RETURN(!pPanoramiXWin,BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_DestroyWindow])(client);
        BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXWinback && 
	        pPanoramiXWin && pPanoramiXWin->FreeMe) {
	pPanoramiXWinback->next = pPanoramiXWin->next;
	Xfree(pPanoramiXWin);
    } 
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXDestroySubwindows(ClientPtr client)
{
    REQUEST(xResourceReq);
    int         	j,result;
    PanoramiXWindow   	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow   	*pPanoramiXWinback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXWin && (pPanoramiXWin->info[0].id != stuff->id);
						pPanoramiXWin = pPanoramiXWin->next)
	pPanoramiXWinback = pPanoramiXWin;
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_DestroySubwindows])(client);
    }
    if ((result == Success) && pPanoramiXWinback && 
	        pPanoramiXWin && pPanoramiXWin->FreeMe) {
	pPanoramiXWinback->next = pPanoramiXWin->next;
	Xfree(pPanoramiXWin);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXChangeSaveSet(ClientPtr client)
{
    REQUEST(xChangeSaveSetReq);
    int         	j, result;
    PanoramiXWindow   	*pPanoramiXWin = PanoramiXWinRoot;

    REQUEST_SIZE_MATCH(xChangeSaveSetReq);
    if (!stuff->window) 
	result = (* SavedProcVector[X_ChangeSaveSet])(client);
    else {
      PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
      IF_RETURN(!pPanoramiXWin, BadWindow);
      FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_ChangeSaveSet])(client);
      }
    }
    return (result);
}


int PanoramiXReparentWindow(register ClientPtr client)
{
    register WindowPtr 	pWin, pParent;
    REQUEST(xReparentWindowReq);
    register int 	result;
    int 		j;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow 	*pPanoramiXPar = PanoramiXWinRoot;

    REQUEST_SIZE_MATCH(xReparentWindowReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PANORAMIXFIND_ID(pPanoramiXPar, stuff->parent);
    IF_RETURN(!pPanoramiXPar, BadWindow);
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXPar), j) {
	stuff->window = pPanoramiXWin->info[j].id;
	stuff->parent = pPanoramiXPar->info[j].id;
	result = (*SavedProcVector[X_ReparentWindow])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXMapWindow(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j,result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;
    Window		winID;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility */
    pWin = (WindowPtr)SecurityLookupWindow(stuff->id, client, 
SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	  pPanoramiXWin->VisibilitySent = FALSE;
    }
    pPanoramiXWin = PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	 winID = pPanoramiXWin->info[j].id;
         pWin = (WindowPtr) SecurityLookupWindow(winID, 
client,SecurityReadAccess);
	 IF_RETURN((!pWin), BadWindow);
	 stuff->id = winID;
	 result = (*SavedProcVector[X_MapWindow])(client);
    }
    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    return (result);
}


int PanoramiXMapSubwindows(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j,result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility values */
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }

    pPanoramiXWin = PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (*SavedProcVector[X_MapSubwindows])(client);
    }
    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXUnmapWindow(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j, result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;
   
    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility values */
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pWin->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }

    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	 stuff->id = pPanoramiXWin->info[j].id;
	 result = (*SavedProcVector[X_UnmapWindow])(client);
    }

    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pChild->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    PANORAMIX_FREE(client);
    return (client->noClientException);
}


int PanoramiXUnmapSubwindows(register ClientPtr client)
{
    REQUEST(xResourceReq);
    int 		j, result;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    register WindowPtr  pWin, pChild;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    /* initialize visibility values */
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    IF_RETURN(!pWin, BadWindow);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pWin->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }

    PANORAMIXFIND_ID(pPanoramiXWin, stuff->id);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    PanoramiXMapped = TRUE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    for (j = 0; j <= (PanoramiXNumScreens - 1); j++)
    {
	stuff->id = pPanoramiXWin->info[j].id;
	result = (*SavedProcVector[X_UnmapSubwindows])(client);
    }

    /* clean up */
    PanoramiXMapped = FALSE;
    PanoramiXVisibilityNotifySent = FALSE;
    pPanoramiXWin->VisibilitySent = FALSE;
    pWin = (WindowPtr) SecurityLookupWindow(stuff->id, 
client,SecurityReadAccess);
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib){
      pPanoramiXWin = PanoramiXWinRoot;
      PANORAMIXFIND_ID(pPanoramiXWin, pWin->drawable.id);
      if (pPanoramiXWin)
	pPanoramiXWin->VisibilitySent = FALSE;
    }
    PANORAMIX_FREE(client);
    return (client->noClientException);
}


int PanoramiXConfigureWindow(register ClientPtr client)
{
    register WindowPtr	pWin;
    REQUEST(xConfigureWindowReq);
    register int 	result;
    unsigned 		len, i, things;
    XID 		changes[32];
    register Mask	orig_mask;
    int 		j, sib_position;
    Window 		winID;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXWindow	*pPanoramiXSib = NULL;
    int 		x_off = 0, y_off = 0;
    XID 		*pStuff;
    XID 		*origStuff, *modStuff;
    Mask 		local_mask;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xConfigureWindowReq);
    len = client->req_len - (sizeof(xConfigureWindowReq) >> 2);
    things = Ones((Mask)stuff->mask);
    IF_RETURN((things != len), BadLength);
    orig_mask = stuff->mask;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    if (!pPanoramiXWin) {
	client->errorValue = stuff->window;
	return (BadWindow);
    }
    if (things > 0) {
        pStuff = (XID *) ALLOCATE_LOCAL(things * sizeof(XID));
        memcpy((char *) pStuff, (char *) &stuff[1], things * sizeof(XID));
        local_mask = (CWSibling | CWX | CWY) & ((Mask) stuff->mask);
        if (local_mask & CWSibling) {
       	   sib_position = Ones((Mask) stuff->mask & (CWSibling - 1));
    	   pPanoramiXSib = PanoramiXWinRoot;
	   PANORAMIXFIND_ID(pPanoramiXSib, *(pStuff + sib_position));
        }
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	    winID = pPanoramiXWin->info[j].id;
	    pWin = (WindowPtr)SecurityLookupWindow(winID, client,SecurityReadAccess);
	    if (!pWin) {
		client->errorValue = pPanoramiXWin->info[0].id;
		return (BadWindow);
	    }
	    stuff->window = winID;
	    if (pWin->parent 
		&& (pWin->parent->drawable.id == PanoramiXWinRoot->info[j].id)) {
	        x_off = panoramiXdataPtr[j].x;
	        y_off = panoramiXdataPtr[j].y;
	    }
	    modStuff = (XID *) &stuff[1];
	    origStuff = pStuff;
	    i = things;
	    if (local_mask & CWX) {
	        *modStuff++ = *origStuff++ - x_off;
	        i--;
	    }
	    if (local_mask & CWY) {
	       *modStuff++ = *origStuff++ - y_off;
	       i--;
	    }
	    for ( ; i; i--) 
	       *modStuff++ = *origStuff++;
	    if (pPanoramiXSib)
	       *((XID *) &stuff[1] + sib_position) = pPanoramiXSib->info[j].id;
	    stuff->mask = orig_mask;
	    result = (*SavedProcVector[X_ConfigureWindow])(client);
        }
    DEALLOCATE_LOCAL(pStuff);
    PANORAMIX_FREE(client);
    return (result);
    } else
	return (client->noClientException);
}


int PanoramiXCirculateWindow(register ClientPtr client)
{
    REQUEST(xCirculateWindowReq);
    int 	  j,result;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xCirculateWindowReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (*SavedProcVector[X_CirculateWindow])(client);
    }
    return (result);
}


int PanoramiXGetGeometry(register ClientPtr client)
{
    xGetGeometryReply 	 rep;
    register DrawablePtr pDraw;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    VERIFY_GEOMETRABLE (pDraw, stuff->id, client);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.root = WindowTable[pDraw->pScreen->myNum]->drawable.id;
    rep.depth = pDraw->depth;

    if (stuff->id == PanoramiXWinRoot->info[0].id) {
	xConnSetup *setup = (xConnSetup *) ConnectionInfo;
	xWindowRoot *root  = (xWindowRoot *)
				    (ConnectionInfo + connBlockScreenStart);

	rep.width = root->pixWidth;
	rep.height = root->pixHeight;
    } else {
	rep.width = pDraw->width;
	rep.height = pDraw->height;
    }

    /* XXX - Because the pixmap-implementation of the multibuffer extension 
     *       may have the buffer-id's drawable resource value be a pointer
     *       to the buffer's window instead of the buffer itself
     *       (this happens if the buffer is the displayed buffer),
     *       we also have to check that the id matches before we can
     *       truly say that it is a DRAWABLE_WINDOW.
     */

    if ((pDraw->type == UNDRAWABLE_WINDOW) ||
        ((pDraw->type == DRAWABLE_WINDOW) && (stuff->id == pDraw->id))) {
        register WindowPtr pWin = (WindowPtr)pDraw;
	rep.x = pWin->origin.x - wBorderWidth (pWin);
	rep.y = pWin->origin.y - wBorderWidth (pWin);
	rep.borderWidth = pWin->borderWidth;
    } else { 			/* DRAWABLE_PIXMAP or DRAWABLE_BUFFER */
	rep.x = rep.y = rep.borderWidth = 0;
    }
    WriteReplyToClient(client, sizeof(xGetGeometryReply), &rep);
    return (client->noClientException);
}


int PanoramiXChangeProperty(ClientPtr client)
{	      
    int  	  result, j;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;
    REQUEST(xChangePropertyReq);

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xChangePropertyReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_ChangeProperty])(client);
	if (result != Success) {
	    stuff->window = pPanoramiXWin->info[0].id;
	    break;
        }
    }
    return (result);
}


int PanoramiXDeleteProperty(ClientPtr client)
{	      
    int  	  result, j;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;
    REQUEST(xDeletePropertyReq);

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xDeletePropertyReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->window = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_DeleteProperty])(client);
	BREAK_IF(result != Success);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXSendEvent(ClientPtr client)
{	      
    int  	  result, j; 
    BYTE	  orig_type;
    Mask	  orig_eventMask;
    PanoramiXWindow *pPanoramiXWin = PanoramiXWinRoot;
    REQUEST(xSendEventReq);

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xSendEventReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->destination);
    orig_type = stuff->event.u.u.type;
    orig_eventMask = stuff->eventMask;
    if (!pPanoramiXWin) {
	noPanoramiXExtension = TRUE;
	result = (* SavedProcVector[X_SendEvent])(client);
	noPanoramiXExtension = FALSE;
    }
    else {
       noPanoramiXExtension = FALSE;
       FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	 stuff->destination = pPanoramiXWin->info[j].id;
	 stuff->eventMask = orig_eventMask;
	 stuff->event.u.u.type = orig_type;
	 if (!j) 
	     noPanoramiXExtension = TRUE;
	 result = (* SavedProcVector[X_SendEvent])(client);
	 noPanoramiXExtension = FALSE;
       }
    }
    return (result);
}


int PanoramiXCreatePixmap(register ClientPtr client)
{
    PixmapPtr 		 pMap;
    register DrawablePtr pDraw;
    REQUEST(xCreatePixmapReq);
     DepthPtr 		 pDepth;
    int 		 result, j;
    Pixmap 		 pmapID;
    PanoramiXWindow 	 *pPanoramiXWin;
    PanoramiXPmap 	 *pPanoramiXPmap;
    PanoramiXPmap	 *localPmap;
    XID			 orig_pid;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xCreatePixmapReq);
    client->errorValue = stuff->pid;

    localPmap =(PanoramiXPmap *) Xcalloc(sizeof(PanoramiXPmap));
    IF_RETURN(!localPmap, BadAlloc);

    pDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE, 
						  SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);

    pPanoramiXWin = (pDraw->type == DRAWABLE_PIXMAP)
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadWindow);

    orig_pid = stuff->pid;
    FOR_NSCREENS_OR_ONCE(pPanoramiXPmap, j) {
	pmapID = j ? FakeClientID(client->index) : orig_pid;
	localPmap->info[j].id = pmapID;
    }
    localPmap->FreeMe = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXPmap, PanoramiXPmapRoot);
    pPanoramiXPmap->next = localPmap;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->pid = localPmap->info[j].id;
	stuff->drawable = pPanoramiXWin->info[j].id;
	result = (* SavedProcVector[X_CreatePixmap])(client);
	BREAK_IF(result != Success);
    }
    if (result != Success) {
	pPanoramiXPmap->next = NULL;
	if (localPmap)
	    Xfree(localPmap);
    }
    return (result);
}


int PanoramiXFreePixmap(ClientPtr client)
{
    PixmapPtr 	pMap;
    int         result, j;
    PanoramiXPmap *pPanoramiXPmap = PanoramiXPmapRoot;
    PanoramiXPmap *pPanoramiXPmapback = NULL;
    REQUEST(xResourceReq);

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXPmap && (pPanoramiXPmap->info[0].id != stuff->id);
					    pPanoramiXPmap = pPanoramiXPmap->next)
	pPanoramiXPmapback = pPanoramiXPmap;
    if (!pPanoramiXPmap)
	result = (* SavedProcVector[X_FreePixmap])(client);
    else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	    stuff->id = pPanoramiXPmap->info[j].id;
	    result = (* SavedProcVector[X_FreePixmap])(client);
        }
        if ((result == Success) && pPanoramiXPmapback &&
		 pPanoramiXPmap && pPanoramiXPmap->FreeMe ) {
    	    pPanoramiXPmapback->next = pPanoramiXPmap->next;
	    Xfree(pPanoramiXPmap);
        }
    }
    return (result);
}


int PanoramiXCreateGC(register ClientPtr client)
{
    int 	       result, j;
    GC 		      *pGC;
    DrawablePtr        pDraw;
    unsigned 	       len, i;
    REQUEST(xCreateGCReq);
    GContext 	       GCID;
    PanoramiXWindow   *pPanoramiXWin;
    PanoramiXGC	      *localGC;
    PanoramiXGC       *pPanoramiXGC;
    PanoramiXPmap     *pPanoramiXTile = NULL, *pPanoramiXStip = NULL;
    PanoramiXPmap     *pPanoramiXClip = NULL;
    int		       tile_offset, stip_offset, clip_offset;
    XID		       orig_GC;

    REQUEST_AT_LEAST_SIZE(xCreateGCReq);
    client->errorValue = stuff->gc;
    pDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						  SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);
    pPanoramiXWin = (pDraw->type == DRAWABLE_PIXMAP)
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);

    len = client->req_len -  (sizeof(xCreateGCReq) >> 2);
    IF_RETURN((len != Ones((Mask)stuff->mask)), BadLength);
    localGC = (PanoramiXGC *) Xcalloc(sizeof(PanoramiXGC));
    IF_RETURN(!localGC, BadAlloc);
    orig_GC = stuff->gc;
    if ((Mask)stuff->mask & GCTile) {
	XID tileID;

	tile_offset = Ones((Mask)stuff->mask & (GCTile - 1));
	tileID = *((CARD32 *) &stuff[1] + tile_offset);
	if (tileID) {
	    pPanoramiXTile = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXTile, tileID);
	}
    }
    if ((Mask)stuff->mask & GCStipple) {
	XID stipID;

	stip_offset = Ones((Mask)stuff->mask & (GCStipple - 1));
	stipID = *((CARD32 *) &stuff[1] + stip_offset);
	if (stipID) {
	    pPanoramiXStip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXStip, stipID);
	}
    }
    if ((Mask)stuff->mask & GCClipMask) {
	XID clipID;

	clip_offset = Ones((Mask)stuff->mask & (GCClipMask - 1));
	clipID = *((CARD32 *) &stuff[1] + clip_offset);
	if (clipID) {
	    pPanoramiXClip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXClip, clipID);
	}
    }
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	GCID = j ? FakeClientID(client->index) : orig_GC;
	localGC->info[j].id = GCID;
    }
    localGC->FreeMe = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXGC, PanoramiXGCRoot);
    pPanoramiXGC->next = localGC;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->gc = localGC->info[j].id;
	stuff->drawable = pPanoramiXWin->info[j].id;
	if (pPanoramiXTile)
	    *((CARD32 *) &stuff[1] + tile_offset) = pPanoramiXTile->info[j].id;
	if (pPanoramiXStip)
	    *((CARD32 *) &stuff[1] + stip_offset) = pPanoramiXStip->info[j].id;
	if (pPanoramiXClip)
	    *((CARD32 *) &stuff[1] + clip_offset) = pPanoramiXClip->info[j].id;
	result = (* SavedProcVector[X_CreateGC])(client);
	BREAK_IF(result != Success);
    }
    if (result != Success) {
       pPanoramiXGC->next = NULL;
       Xfree(localGC);
    }
    return (result);
}


int PanoramiXChangeGC(ClientPtr client)
{
    GC 		*pGC;
    REQUEST(xChangeGCReq);
    int 	result, j;
    unsigned 	len;
    PanoramiXGC   *pPanoramiXGC = PanoramiXGCRoot;
    PanoramiXPmap	*pPanoramiXTile = NULL, *pPanoramiXStip = NULL;
    PanoramiXPmap	*pPanoramiXClip = NULL;
    int		tile_offset, stip_offset, clip_offset;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xChangeGCReq);
    VERIFY_GC(pGC, stuff->gc, client);
    len = client->req_len -  (sizeof(xChangeGCReq) >> 2);
    IF_RETURN((len != Ones((Mask)stuff->mask)), BadLength);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    if ((Mask)stuff->mask & GCTile) {
	XID tileID;

	tile_offset = Ones((Mask)stuff->mask & (GCTile -1) );
	tileID = *((CARD32 *) &stuff[1] + tile_offset);
	if (tileID) {
	    pPanoramiXTile = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXTile, tileID);
	}
    }
    if ((Mask)stuff->mask & GCStipple) {
	XID stipID;

	stip_offset = Ones((Mask)stuff->mask & (GCStipple -1 ));
	stipID = *((CARD32 *) &stuff[1] + stip_offset);
	if (stipID) {
	    pPanoramiXStip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXStip, stipID);
	}
    }
    if ((Mask)stuff->mask & GCClipMask) {
	XID clipID;

	clip_offset = Ones((Mask)stuff->mask & (GCClipMask -1));
	clipID = *((CARD32 *) &stuff[1] + clip_offset);
	if (clipID) {
	    pPanoramiXClip = PanoramiXPmapRoot;
	    PANORAMIXFIND_ID(pPanoramiXClip, clipID);
	}
    }
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->gc = pPanoramiXGC->info[j].id;
	if (pPanoramiXTile)
	    *((CARD32 *) &stuff[1] + tile_offset) = pPanoramiXTile->info[j].id;
	if (pPanoramiXStip)
	    *((CARD32 *) &stuff[1] + stip_offset) = pPanoramiXStip->info[j].id;
	if (pPanoramiXClip)
	    *((CARD32 *) &stuff[1] + clip_offset) = pPanoramiXClip->info[j].id;
	result = (* SavedProcVector[X_ChangeGC])(client);
	BREAK_IF(result != Success);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXCopyGC(ClientPtr client)
{
    int 	j, result;
    PanoramiXGC	*pPanoramiXGCSrc = PanoramiXGCRoot;
    PanoramiXGC	*pPanoramiXGCDst = PanoramiXGCRoot;
    REQUEST(xCopyGCReq);

    REQUEST_SIZE_MATCH(xCopyGCReq);
    PANORAMIXFIND_ID(pPanoramiXGCSrc, stuff->srcGC);
    IF_RETURN(!pPanoramiXGCSrc, BadGC);
    PANORAMIXFIND_ID(pPanoramiXGCDst, stuff->dstGC);
    IF_RETURN(!pPanoramiXGCDst, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGCDst, j) {
	stuff->srcGC = pPanoramiXGCSrc->info[j].id;
	stuff->dstGC = pPanoramiXGCDst->info[j].id;
	result = (* SavedProcVector[X_CopyGC])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXSetDashes(ClientPtr client)
{
    GC 		*pGC;
    REQUEST(xSetDashesReq);
    int 	result, j;
    PanoramiXGC   *pPanoramiXGC = PanoramiXGCRoot;

    REQUEST_FIXED_SIZE(xSetDashesReq, stuff->nDashes);
    VERIFY_GC(pGC, stuff->gc, client);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_SetDashes])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXSetClipRectangles(register ClientPtr client)
{
    int 	result;
    register GC *pGC;
    REQUEST(xSetClipRectanglesReq);
    int		j;
    PanoramiXGC	*pPanoramiXGC = PanoramiXGCRoot;

    REQUEST_AT_LEAST_SIZE(xSetClipRectanglesReq);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_SetClipRectangles])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXFreeGC(ClientPtr client)
{
    register GC *pGC;
    REQUEST(xResourceReq);
    int		result, j;
    PanoramiXGC	*pPanoramiXGC = PanoramiXGCRoot;
    PanoramiXGC	*pPanoramiXGCback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    for (; pPanoramiXGC && (pPanoramiXGC->info[0].id != stuff->id); 
						pPanoramiXGC = pPanoramiXGC->next)
	pPanoramiXGCback = pPanoramiXGC;
    IF_RETURN(!pPanoramiXGC, BadGC);
    FOR_NSCREENS_OR_ONCE(pPanoramiXGC, j) {
	stuff->id = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_FreeGC])(client);
	BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXGCback &&
	         pPanoramiXGC && pPanoramiXGC->FreeMe) {
	pPanoramiXGCback->next = pPanoramiXGC->next;
	if (pPanoramiXGC)
	   Xfree(pPanoramiXGC);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXClearToBackground(register ClientPtr client)
{
    REQUEST(xClearAreaReq);
    register WindowPtr  pWin;
    int 		result, j;
    Window 		winID;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    int 		orig_x, orig_y;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xClearAreaReq);
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    IF_RETURN(!pPanoramiXWin, BadWindow);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	winID = pPanoramiXWin->info[j].id;
	pWin = (WindowPtr) SecurityLookupWindow(winID, client, SecurityReadAccess);
	if (!pWin) {
	    client->errorValue = pPanoramiXWin->info[0].id;
	    return (BadWindow);
	}
	stuff->window = winID;
	if (pWin->drawable.id == PanoramiXWinRoot->info[j].id) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}
	result = (*SavedProcVector[X_ClearArea])(client);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXCopyArea(ClientPtr client)
{
    int			j, result;
    Window		srcID, dstID;
    DrawablePtr		pSrc, pDst;
    GContext		GCID;
    GC			*pGC;
    PanoramiXWindow 	*pPanoramiXSrcRoot;
    PanoramiXWindow 	*pPanoramiXDstRoot;
    PanoramiXWindow	*pPanoramiXSrc;
    PanoramiXWindow	*pPanoramiXDst;
    PanoramiXGC		*pPanoramiXGC = PanoramiXGCRoot;
    REQUEST(xCopyAreaReq);
    int           srcx = stuff->srcX, srcy = stuff->srcY;
    int           dstx = stuff->dstX, dsty = stuff->dstY;

    REQUEST_SIZE_MATCH(xCopyAreaReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pDst, pGC, client);
    if (stuff->dstDrawable != stuff->srcDrawable) {
        VERIFY_DRAWABLE(pSrc, stuff->srcDrawable, client);
        if ((pDst->pScreen != pSrc->pScreen) || (pDst->depth != pSrc->depth)) {
            client->errorValue = stuff->dstDrawable;
            return (BadMatch);
        }
    } else {
        pSrc = pDst;
    } 
    pPanoramiXSrcRoot = (pSrc->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXDstRoot = (pDst->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXSrc = pPanoramiXSrcRoot;
    pPanoramiXDst = pPanoramiXDstRoot;
    PANORAMIXFIND_ID(pPanoramiXSrc, stuff->srcDrawable);
    IF_RETURN(!pPanoramiXSrc, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXDst, stuff->dstDrawable);
    IF_RETURN(!pPanoramiXDst, BadDrawable);
    GCID = stuff->gc;
    PANORAMIXFIND_ID(pPanoramiXGC, GCID);
    IF_RETURN(!pPanoramiXGC, BadGC);

    FOR_NSCREENS_OR_ONCE(pPanoramiXSrc, j) {
	stuff->dstDrawable = pPanoramiXDst->info[j].id;
	stuff->srcDrawable = pPanoramiXSrc->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
 	if (pPanoramiXSrc == pPanoramiXSrcRoot) {	
	    stuff->srcX = srcx - panoramiXdataPtr[j].x;
	    stuff->srcY = srcy - panoramiXdataPtr[j].y;
	}
 	if (pPanoramiXDst == pPanoramiXDstRoot) {	
	    stuff->dstX = dstx - panoramiXdataPtr[j].x;
	    stuff->dstY = dsty - panoramiXdataPtr[j].y;
	}
	result = (* SavedProcVector[X_CopyArea])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}

int PanoramiXCopyPlane(ClientPtr client)
{
    int 	  SrcScr = -1, DstScr = -1;
    PixmapPtr 	  pMap = NULL;
    Pixmap	  pmapID;
    PanoramiXRect   SrcRect, DstRect;
    int 	  i, j, k;
    Window 	  srcID, dstID;
    DrawablePtr   pSrc, pDst;
    GContext 	  GCID;
    GContext      GCIDbase;
    GC		  *pGC;
    PanoramiXWindow *pPanoramiXSrc;
    PanoramiXWindow *pPanoramiXDst;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    RegionPtr 	  *PanoramiXRgnPtrs;
    RegionPtr 	  *FetchRgnPtrs = NULL;
    RegionPtr 	  pRgn;
    REQUEST(xCopyPlaneReq);
    int           srcx = stuff->srcX, srcy = stuff->srcY;
    int           dstx = stuff->dstX, dsty = stuff->dstY;
    int           width = stuff->width, height = stuff->height;

    REQUEST_SIZE_MATCH(xCopyPlaneReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pDst, pGC, client);
    if (stuff->dstDrawable != stuff->srcDrawable) {
        VERIFY_DRAWABLE(pSrc, stuff->srcDrawable, client);
        if (pDst->pScreen != pSrc->pScreen) {
            client->errorValue = stuff->dstDrawable;
            return (BadMatch);
        }
    } else {
        pSrc = pDst;
    }

    /*
     *	Check to see if stuff->bitPlane has exactly ONE good bit set
     */


    if(stuff->bitPlane == 0 || (stuff->bitPlane & (stuff->bitPlane - 1)) ||
       (stuff->bitPlane > (1L << (pSrc->depth - 1))))
    {
       client->errorValue = stuff->bitPlane;
       return(BadValue);
    }

    pPanoramiXSrc = (pSrc->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXDst = (pDst->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXSrc, stuff->srcDrawable);
    IF_RETURN(!pPanoramiXSrc, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXDst, stuff->dstDrawable);
    IF_RETURN(!pPanoramiXDst, BadDrawable);
    GCIDbase = stuff->gc;
    PANORAMIXFIND_ID(pPanoramiXGC, GCIDbase);
    IF_RETURN(!pPanoramiXGC, BadGC);
    
    /*
     *	Unless both are pixmaps, need to do special things to accomodate
     *	being on multiple screens, different screens, etc.
     */
    
    if (pSrc->type != DRAWABLE_PIXMAP) {
	SrcRect.x = pSrc->x + srcx;
	SrcRect.y = pSrc->y + srcy;
	SrcRect.width = width;
	SrcRect.height = height;
	for (SrcScr = PanoramiXNumScreens - 1; SrcScr>=0; SrcScr-- )
	    if (RECTA_SUBSUMES_RECTB(panoramiXdataPtr[SrcScr], SrcRect))
		break;
    }
    if (pDst->type != DRAWABLE_PIXMAP) {
	DstRect.x = pDst->x + dstx;
	DstRect.y = pDst->y + dsty;
	DstRect.width = width;
	DstRect.height = height;
	for (DstScr = PanoramiXNumScreens - 1; DstScr>=0; DstScr--)
	    if (RECTA_SUBSUMES_RECTB(panoramiXdataPtr[DstScr], DstRect))
		break;
    } 
	
    /*
     *	If source is on multiple screens, different screen from destination,
     *	destination is on multiple screens, or destination is a pixmap,
     *	need to get info into local pixmap for subsequent transfer.
     */

             
    if ((pSrc->type != DRAWABLE_PIXMAP) && 
	  (SrcScr < 0 || DstScr < 0 || SrcScr != DstScr 
		|| pDst->type == DRAWABLE_PIXMAP)) {
	unsigned char save_alu;
	RegionRec     tempReg;
	RegionPtr     pCompositeClip;
	PanoramiXPmap   *pPanoramiXPmap = PanoramiXPmapRoot;

	pMap = (PixmapPtr) (*pSrc->pScreen->CreatePixmap)(pSrc->pScreen,
				    width, height, pGC->depth);
	PANORAMIXFIND_LAST(pPanoramiXPmap, PanoramiXPmapRoot);
	pPanoramiXPmap->next = 
		    (PanoramiXPmap *)Xcalloc(sizeof(PanoramiXPmap));
	pPanoramiXPmap = pPanoramiXPmap->next;
	pmapID = FakeClientID(0);
	AddResource(pmapID, RT_PIXMAP, (pointer)pMap);
	for (j = PanoramiXNumScreens - 1; j>=0; j--)
	    pPanoramiXPmap->info[j].id = pmapID;
	tempReg.extents.x1 = 0;
	tempReg.extents.y1 = 0;
	tempReg.extents.x2 = width;
	tempReg.extents.y2 = height;
	tempReg.data = NULL;
	FetchRgnPtrs = 
	    (RegionPtr *) ALLOCATE_LOCAL(PanoramiXNumScreens * sizeof(RegionPtr));
	i = 0;
	FOR_NSCREENS_OR_ONCE(pPanoramiXSrc, j) {
	    if ((SrcScr >= 0) && pPanoramiXSrc)
		j = SrcScr;
	    srcID = pPanoramiXSrc->info[j].id;
	    pSrc = (DrawablePtr) SecurityLookupIDByClass(client, srcID, RC_DRAWABLE, 
				 			 SecurityReadAccess);
	    GCID = pPanoramiXGC->info[j].id;
	    pGC = (GC *) LookupIDByType(GCID, RT_GC);
	    pMap->drawable.pScreen = pSrc->pScreen;
	    pGC->pScreen = pSrc->pScreen;
	    save_alu = pGC->alu;
	    pGC->alu = GXcopy;
	    pCompositeClip = ((miPrivGC*)
		    (pGC->devPrivates[miGCPrivateIndex].ptr))->pCompositeClip;
	    ((miPrivGC*)(pGC->devPrivates[miGCPrivateIndex].ptr))->pCompositeClip = 
&tempReg;
	    FetchRgnPtrs[i++] = (*pGC->ops->CopyPlane)(pSrc, (DrawablePtr) pMap,
			 pGC, srcx, srcy, width, height, 0, 0, stuff->bitPlane);
	    pGC->alu = save_alu;
	    ((miPrivGC*) (pGC->devPrivates[miGCPrivateIndex].ptr))->pCompositeClip = 
pCompositeClip;
	    if (SrcScr >= 0)
		j = 0;
	}
    }

    if (pMap) {
	pSrc = (DrawablePtr) pMap;
	srcx = 0;
	srcy = 0;
    }
    PanoramiXRgnPtrs = 
	    (RegionPtr *) ALLOCATE_LOCAL(PanoramiXNumScreens * sizeof(RegionPtr));
    k = 0;
	/* if src and dst are entirely on one screen, 
	   then we only need one simple transfer */
    if ((DstScr >= 0) && (pMap || (SrcScr >=0))) {
	dstID = pPanoramiXDst->info[DstScr].id;
	pDst = (DrawablePtr) SecurityLookupIDByClass(client, dstID, RC_DRAWABLE,
						     SecurityReadAccess);
	GCID = pPanoramiXGC->info[DstScr].id;
	pGC = (GC *) LookupIDByType(GCID, RT_GC);
	ValidateGC(pDst, pGC);
	if (pMap) {
	    pMap->drawable.pScreen = pDst->pScreen;
	} else {
	    srcID = pPanoramiXSrc->info[SrcScr].id;
	    if (srcID != dstID) {
		pSrc = (DrawablePtr) SecurityLookupIDByClass(client, srcID, RC_DRAWABLE,
							     SecurityReadAccess);
	    } else 
		pSrc = pDst;
	}
	if (pMap)
	    PanoramiXRgnPtrs[k++] = (*pGC->ops->CopyPlane)(pSrc, pDst, pGC, 
			              srcx, srcy, width, height, dstx, dsty, 
				      1);
	else 
	    PanoramiXRgnPtrs[k++] = (*pGC->ops->CopyPlane)(pSrc, pDst, pGC, 
				      srcx, srcy, width, height, dstx, dsty,
				      stuff->bitPlane);
    }else {
      FOR_NSCREENS_OR_ONCE(pPanoramiXDst, j) {
	if (DstScr >= 0) {
	    dstID = pPanoramiXDst->info[DstScr].id;
	    GCID = pPanoramiXGC->info[DstScr].id;
	} else {
	    dstID = pPanoramiXDst->info[j].id;
	    GCID = pPanoramiXGC->info[j].id;
	}
	pDst = (DrawablePtr) SecurityLookupIDByClass(client, dstID, RC_DRAWABLE,
						     SecurityReadAccess);
	pGC = (GC *) LookupIDByType(GCID, RT_GC);
	ValidateGC(pDst, pGC);
	if (pMap) {
	    pMap->drawable.pScreen = pDst->pScreen;
	} else {
	    srcID = pPanoramiXSrc->info[j].id;
	    if (srcID != dstID) {
		pSrc = (DrawablePtr) SecurityLookupIDByClass(client, srcID, RC_DRAWABLE,
							     SecurityReadAccess);
	    } else {
		pSrc = pDst;
	    }
	}
	if (pMap)
	    PanoramiXRgnPtrs[k++] = (*pGC->ops->CopyPlane)(pSrc, pDst, pGC, 
			              srcx, srcy, width, height, dstx, dsty, 
				      1); 
	else 
	    PanoramiXRgnPtrs[k++] = (*pGC->ops->CopyPlane)(pSrc, pDst, pGC, 
				      srcx, srcy, width, height, dstx, dsty,
				      stuff->bitPlane);
      }
    }
    
    if (pMap) {
	for (j = PanoramiXNumScreens - 1; j>=0; j--)
	    if (PanoramiXRgnPtrs[j])
		(*pDst->pScreen->RegionDestroy) (PanoramiXRgnPtrs[j]);
	DEALLOCATE_LOCAL(PanoramiXRgnPtrs);
	PanoramiXRgnPtrs = FetchRgnPtrs;
	k = i;
    }
    j = 1;
    i = 0;
    pRgn = PanoramiXRgnPtrs[i];
    while ((j < k) && pRgn && !REGION_NIL(pRgn)) {
	if (PanoramiXRgnPtrs[j]) {
	    (*pGC->pScreen->Intersect)(pRgn, pRgn, PanoramiXRgnPtrs[j++]);
	} else {
	    pRgn = PanoramiXRgnPtrs[i++];
	}
    }
    for (j = 0 ; j < k; j++) {
	pRgn = PanoramiXRgnPtrs[j];
	GCID = pPanoramiXGC->info[j].id;
	pGC = (GC *) LookupIDByType(GCID, RT_GC);
	if (pGC && pGC->graphicsExposures)
	    (*pDst->pScreen->SendGraphicsExpose) (client, pRgn,
					    stuff->dstDrawable, X_CopyPlane, 0);
	if (pRgn)
	    (*pDst->pScreen->RegionDestroy) (pRgn);
    }
    DEALLOCATE_LOCAL(PanoramiXRgnPtrs);
    if (pMap) {
	PanoramiXPmap *pPanoramiXPmap = PanoramiXPmapRoot;
	PanoramiXPmap *pback = PanoramiXPmapRoot;

	for (; pPanoramiXPmap && (pPanoramiXPmap->info[0].id != pmapID); 
					    pPanoramiXPmap = pPanoramiXPmap->next)
	    pback = pPanoramiXPmap;
	FreeResource(pPanoramiXPmap->info[0].id, RT_NONE);
	if (pback) {
	    pback->next = pPanoramiXPmap->next;
	    Xfree(pPanoramiXPmap);
	}
    }
    return (client->noClientException);
}


int PanoramiXPolyPoint(ClientPtr client)
{
    int 	  result, npoint, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int           x_off = 0, y_off = 0;
    xPoint 	  *origPts;
    xPoint	  *origPtr, *modPtr;
    REQUEST(xPolyPointReq);

    REQUEST_AT_LEAST_SIZE(xPolyPointReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, locDraw->id);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    npoint = ((client->req_len << 2) - sizeof(xPolyPointReq)) >> 2;
    if (npoint > 0) {
        origPts = (xPoint *) ALLOCATE_LOCAL(npoint * sizeof(xPoint));
        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	  if (pPanoramiXWin == PanoramiXWinRoot) {
	      x_off = panoramiXdataPtr[j].x;
	      y_off = panoramiXdataPtr[j].y;
	  }else {
	  if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	  }
	}
	  modPtr = (xPoint *) &stuff[1];
	  origPtr = origPts;
	  for (i = npoint; i; i--) {
	      modPtr->x = origPtr->x - x_off;
	      modPtr++->y = origPtr++->y - y_off;
	  }
	  stuff->drawable = pPanoramiXWin->info[j].id;
	  stuff->gc = pPanoramiXGC->info[j].id;
	  result = (* SavedProcVector[X_PolyPoint])(client);
	  BREAK_IF(result != Success);
        }
        DEALLOCATE_LOCAL(origPts);
        return (result);
    }else
	return (client->noClientException);

}


int PanoramiXPolyLine(ClientPtr client)
{
    int 	  result, npoint, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int           x_off = 0, y_off = 0;
    xPoint 	  *origPts;
    xPoint	  *origPtr, *modPtr;
    REQUEST(xPolyLineReq);

    REQUEST_AT_LEAST_SIZE(xPolyLineReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, locDraw->id);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != locDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
	 PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    npoint = ((client->req_len << 2) - sizeof(xPolyLineReq)) >> 2;
    if (npoint > 0){
        origPts = (xPoint *) ALLOCATE_LOCAL(npoint * sizeof(xPoint));
        memcpy((char *) origPts, (char *) &stuff[1], npoint * sizeof(xPoint));
        FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	  if (pPanoramiXWin == PanoramiXWinRoot) {
	      x_off = panoramiXdataPtr[j].x;
	      y_off = panoramiXdataPtr[j].y;
	  }else {
	    if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	    }
	  }
	  modPtr = (xPoint *) &stuff[1];
	  origPtr = origPts;
	  for (i = npoint; i; i--) {
	      modPtr->x = origPtr->x - x_off;
	      modPtr++->y = origPtr++->y - y_off;
	  }
	  stuff->drawable = pPanoramiXWin->info[j].id;
	  stuff->gc = pPanoramiXGC->info[j].id;
	  result = (* SavedProcVector[X_PolyLine])(client);
	  BREAK_IF(result != Success);
        }
        DEALLOCATE_LOCAL(origPts);
        return (result);
   }else
	return (client->noClientException);
}


int PanoramiXPolySegment(ClientPtr client)
{
    int		  result, nsegs, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int		  x_off = 0, y_off = 0;
    xSegment 	  *origSegs;
    xSegment 	  *origPtr, *modPtr;
    REQUEST(xPolySegmentReq);

    REQUEST_AT_LEAST_SIZE(xPolySegmentReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != locDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
         PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    nsegs = (client->req_len << 2) - sizeof(xPolySegmentReq);
    IF_RETURN((nsegs & 4), BadLength);
    nsegs >>= 3;
    if (nsegs > 0) {
        origSegs = (xSegment *) ALLOCATE_LOCAL(nsegs * sizeof(xSegment));
        memcpy((char *) origSegs, (char *) &stuff[1], nsegs * 
sizeof(xSegment));
        FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	  if (pPanoramiXWin == PanoramiXWinRoot) {
	      x_off = panoramiXdataPtr[j].x;
	      y_off = panoramiXdataPtr[j].y;
	  }else {
	    if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	    }
	  }
	  modPtr = (xSegment *) &stuff[1];
	  origPtr = origSegs;
	  for (i = nsegs; i; i--) {
	     modPtr->x1 = origPtr->x1 - x_off;
	     modPtr->y1 = origPtr->y1 - y_off;
	     modPtr->x2 = origPtr->x2 - x_off;
	     modPtr++->y2 = origPtr++->y2 - y_off;
	  }
	  stuff->drawable = pPanoramiXWin->info[j].id;
	  stuff->gc = pPanoramiXGC->info[j].id;
	  result = (* SavedProcVector[X_PolySegment])(client);
	  BREAK_IF(result != Success);
    	}
    DEALLOCATE_LOCAL(origSegs);
    return (result);
    }else
	  return (client->noClientException);
}


int PanoramiXPolyRectangle(ClientPtr client)
{
    int 	  result, nrects, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    int		  x_off = 0, y_off = 0;
    xRectangle 	  *origRecs;
    xRectangle 	  *origPtr, *modPtr;
    REQUEST(xPolyRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyRectangleReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    nrects = (client->req_len << 2) - sizeof(xPolyRectangleReq);
    IF_RETURN((nrects & 4), BadLength);
    nrects >>= 3;
    if (nrects > 0){
       origRecs = (xRectangle *) ALLOCATE_LOCAL(nrects * sizeof(xRectangle));
       memcpy((char *) origRecs, (char *) &stuff[1], nrects * 
sizeof(xRectangle));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	if (pPanoramiXWin == PanoramiXWinRoot) {
	    x_off = panoramiXdataPtr[j].x;
	    y_off = panoramiXdataPtr[j].y;
	}else {
	  if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	  }
	}
	modPtr = (xRectangle *) &stuff[1];
	origPtr = origRecs;
	for (i = nrects; i; i--) {
	    modPtr->x = origPtr->x - x_off;
	    modPtr->y = origPtr->y - y_off;
	    modPtr->width = origPtr->width - x_off;
	    modPtr++->height = origPtr++->height - y_off;
	}
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_PolyRectangle])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origRecs);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXPolyArc(ClientPtr client)
{
    int 	  result, narcs, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    GCPtr 	  locGC;
    int           x_off = 0, y_off = 0;
    xArc	  *origArcs;
    xArc	  *origPtr, *modPtr;
    REQUEST(xPolyArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyArcReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
                                            ? PanoramiXPmapRoot : 
PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    narcs = (client->req_len << 2) - sizeof(xPolyArcReq);
    IF_RETURN((narcs % sizeof(xArc)), BadLength);
    narcs /= sizeof(xArc);
    if (narcs > 0){
       origArcs = (xArc *) ALLOCATE_LOCAL(narcs * sizeof(xArc));
       memcpy((char *) origArcs, (char *) &stuff[1], narcs * sizeof(xArc));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
        if (pPanoramiXWin == PanoramiXWinRoot) {
            x_off = panoramiXdataPtr[j].x;
            y_off = panoramiXdataPtr[j].y;
        }else {
	  if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	  }
	}
        modPtr = (xArc *) &stuff[1];
        origPtr = origArcs;
        for (i = narcs; i; i--) {
            modPtr->x = origPtr->x - x_off;
            modPtr++->y = origPtr++->y - y_off;
        }
        stuff->drawable = pPanoramiXWin->info[j].id;
        stuff->gc = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_PolyArc])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origArcs);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXFillPoly(ClientPtr client)
{
    int 	  result, count, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    GCPtr 	  locGC;
    int		  x_off = 0, y_off = 0;
    DDXPointPtr	  locPts;
    DDXPointPtr	  origPts, modPts;
    REQUEST(xFillPolyReq);

    REQUEST_AT_LEAST_SIZE(xFillPolyReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    count = ((client->req_len << 2) - sizeof(xFillPolyReq)) >> 2;
    if (count > 0){
       locPts = (DDXPointPtr) ALLOCATE_LOCAL(count * sizeof(DDXPointRec));
       memcpy((char *) locPts, (char *) &stuff[1], count * 
sizeof(DDXPointRec));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	if (pPanoramiXWin == PanoramiXWinRoot) {
	    x_off = panoramiXdataPtr[j].x;
	    y_off = panoramiXdataPtr[j].y;
	}else {
	  if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	  }
	}
	modPts = (DDXPointPtr) &stuff[1];
	origPts = locPts;
	for (i = count; i; i--) {
	    modPts->x = origPts->x - x_off;
	    modPts++->y = origPts++->y - y_off;
	}
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	result = (* SavedProcVector[X_FillPoly])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(locPts);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXPolyFillRectangle(ClientPtr client)
{
    int 	  result, things, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    GCPtr 	  locGC;
    int		  x_off = 0, y_off = 0;
    xRectangle	  *origThings;
    xRectangle	  *origPtr, *modPtr;
    REQUEST(xPolyFillRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillRectangleReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
                                            ? PanoramiXPmapRoot : 
PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    things = (client->req_len << 2) - sizeof(xPolyFillRectangleReq);
    IF_RETURN((things & 4), BadLength);
    things >>= 3;
    if (things > 0){
       origThings = (xRectangle *) ALLOCATE_LOCAL(things * sizeof(xRectangle));
       memcpy((char *) origThings, (char *)&stuff[1], things * 
sizeof(xRectangle));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
        if (pPanoramiXWin == PanoramiXWinRoot) {
            x_off = panoramiXdataPtr[j].x;
            y_off = panoramiXdataPtr[j].y;
        }else {
	  if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	  }
	}
        modPtr = (xRectangle *) &stuff[1];
        origPtr = origThings;
        for (i = things; i; i--) {
            modPtr->x = origPtr->x - x_off;
            modPtr++->y = origPtr++->y - y_off;
        }
        stuff->drawable = pPanoramiXWin->info[j].id;
        stuff->gc = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_PolyFillRectangle])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origThings);
       return (result);
    }else
       return (client->noClientException);
}


int PanoramiXPolyFillArc(ClientPtr client)
{
    int 	  result, arcs, i, j;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   locDraw;
    GCPtr 	  locGC;
    int		  x_off = 0, y_off = 0;
    xArc	  *origArcs;
    xArc	  *origPtr, *modPtr;
    REQUEST(xPolyFillArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillArcReq);
    locDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						    SecurityReadAccess);
    IF_RETURN(!locDraw, BadDrawable);
    pPanoramiXWin = (locDraw->type == DRAWABLE_PIXMAP)
                                            ? PanoramiXPmapRoot : 
PanoramiXWinRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    arcs = (client->req_len << 2) - sizeof(xPolyFillArcReq);
    IF_RETURN((arcs % sizeof(xArc)), BadLength);
    arcs /= sizeof(xArc);
    if (arcs > 0) {
       origArcs = (xArc *) ALLOCATE_LOCAL(arcs * sizeof(xArc));
       memcpy((char *) origArcs, (char *)&stuff[1], arcs * sizeof(xArc));
       FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
        if (pPanoramiXWin == PanoramiXWinRoot) {
            x_off = panoramiXdataPtr[j].x;
            y_off = panoramiXdataPtr[j].y;
        }else {
	  if ( (locDraw->type == DRAWABLE_PIXMAP) &&
	       /* add special case check for root window */ 
	       (locDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	           x_off = panoramiXdataPtr[j].x;
	           y_off = panoramiXdataPtr[j].y;
	  }
	}
        modPtr = (xArc *) &stuff[1];
        origPtr = origArcs;
        for (i = arcs; i; i--) {
            modPtr->x = origPtr->x - x_off;
            modPtr++->y = origPtr++->y - y_off;
        }
        stuff->drawable = pPanoramiXWin->info[j].id;
        stuff->gc = pPanoramiXGC->info[j].id;
        result = (* SavedProcVector[X_PolyFillArc])(client);
	BREAK_IF(result != Success);
       }
       DEALLOCATE_LOCAL(origArcs);
       return (result);
    }else
       return (client->noClientException);
}


/* 64-bit server notes: the protocol restricts padding of images to
 * 8-, 16-, or 32-bits. We would like to have 64-bits for the server
 * to use internally. Removes need for internal alignment checking.
 * All of the PutImage functions could be changed individually, but
 * as currently written, they call other routines which require things
 * to be 64-bit padded on scanlines, so we changed things here.
 * If an image would be padded differently for 64- versus 32-, then
 * copy each scanline to a 64-bit padded scanline.
 * Also, we need to make sure that the image is aligned on a 64-bit
 * boundary, even if the scanlines are padded to our satisfaction.
 */

int PanoramiXPutImage(register ClientPtr client)
{
    register GC		 *pGC;
    register DrawablePtr pDraw;
    long 		 lengthProto, 	/* length of scanline protocl padded */
			 length; 	/* length of scanline server padded */
    char 		 *tmpImage;
    int			 j;
    PanoramiXWindow 	 *pPanoramiXWin;
    PanoramiXWindow 	 *pPanoramiXRoot;
    PanoramiXGC 		 *pPanoramiXGC = PanoramiXGCRoot;
    int			 orig_x, orig_y;
    int			 result;


    REQUEST(xPutImageReq);

    REQUEST_AT_LEAST_SIZE(xPutImageReq);
    pDraw = (DrawablePtr) SecurityLookupIDByClass(client, stuff->drawable, 
RC_DRAWABLE,
						  SecurityReadAccess);
    IF_RETURN(!pDraw, BadDrawable);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP) 
					    ? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin,BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->dstX;
    orig_y = stuff->dstY;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
      if (pPanoramiXWin == pPanoramiXRoot) {
    	  stuff->dstX = orig_x - panoramiXdataPtr[j].x;
	  stuff->dstY = orig_y - panoramiXdataPtr[j].y;
      }       
      if (pDraw->type == DRAWABLE_PIXMAP) { 
	  if (stuff->width > panoramiXdataPtr[j].width) 
    	      stuff->dstX = orig_x - panoramiXdataPtr[j].x;
	  if (stuff->height > panoramiXdataPtr[j].height)
	      stuff->dstY = orig_y - panoramiXdataPtr[j].y;
      }
      stuff->drawable = pPanoramiXWin->info[j].id;
      stuff->gc = pPanoramiXGC->info[j].id;
      result = (* SavedProcVector[X_PutImage])(client);
    }
    return(result);
}


typedef struct _SrcParts{
    int x1, y1, x2, y2, width, ByteWidth;
    char *buf;
} SrcPartsRec;


int PanoramiXGetImage(register ClientPtr client)
{
    register DrawablePtr pDraw;
    int			nlines, linesPerBuf;
    register int	height, linesDone;
    long		widthBytesLine, length;
#ifdef INTERNAL_VS_EXTERNAL_PADDING
    long		widthBytesLineProto, lengthProto;
    char 		*tmpImage;
#endif
    Mask		plane;
    char		*pBuf;
    xGetImageReply	xgi;
    int			j, k, ScrNum;
    DrawablePtr		locDraw;
    SrcPartsRec		srcParts;
    BoxRec		SrcBox;
    char		*BufPtr, *PartPtr;
    PanoramiXWindow	*pPanoramiXWin = PanoramiXWinRoot;

    REQUEST(xGetImageReq);

    height = stuff->height;
    REQUEST_SIZE_MATCH(xGetImageReq);
    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap)) {
	client->errorValue = stuff->format;
        return(BadValue);
    }
    VERIFY_DRAWABLE(pDraw, stuff->drawable, client);
    ScrNum = 0;
    if (stuff->drawable == PanoramiXWinRoot->info[0].id) {
       for (j = 0; j <= (PanoramiXNumScreens - 1); j++) {
	  ScrNum = j;
	  VERIFY_DRAWABLE(pDraw, pPanoramiXWin->info[ScrNum].id, client);
	  if (stuff->x < panoramiXdataPtr[ScrNum].x  && 
	      stuff->y < panoramiXdataPtr[ScrNum].y ) 
	      break;	
       }
    }
    if (pDraw->type == DRAWABLE_WINDOW) {
	if (!((WindowPtr) pDraw)->realized	    /* Check for viewable */
	    || pDraw->x + stuff->x < 0		    /* Check for on screen */
	    || pDraw->x + stuff->x + (int)stuff->width > PanoramiXPixWidth
	    || pDraw->y + stuff->y < 0
	    || pDraw->y + stuff->y + height > PanoramiXPixHeight
	    || stuff->x < - wBorderWidth((WindowPtr)pDraw)  /* Inside border */
	    || stuff->x + (int)stuff->width >
			wBorderWidth((WindowPtr)pDraw) + (int) pDraw->width
			+ panoramiXdataPtr[ScrNum].x
	    || stuff->y < -wBorderWidth((WindowPtr)pDraw) 
	    || stuff->y + height >
			wBorderWidth ((WindowPtr)pDraw) + (int) pDraw->height
			+ panoramiXdataPtr[ScrNum].y)
	    return(BadMatch);
        VERIFY_DRAWABLE(pDraw, stuff->drawable, client);
	xgi.visual = wVisual (((WindowPtr) pDraw));
	pPanoramiXWin = PanoramiXWinRoot;
	PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
	IF_RETURN(!pPanoramiXWin, BadWindow);
    } else {
	if (stuff->x < 0 || stuff->x + (int)stuff->width > pDraw->width
	    || stuff->y < 0 || stuff->y + height > pDraw->height)
	    return(BadMatch);
	xgi.visual = None;
    }
    xgi.type = X_Reply;
    xgi.sequenceNumber = client->sequence;
    xgi.depth = pDraw->depth;
    if (stuff->format == ZPixmap) {
	widthBytesLine 	= PixmapBytePad(stuff->width, pDraw->depth);
	length 		= widthBytesLine * height;

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	widthBytesLineProto = PixmapBytePadProto(stuff->width, pDraw->depth);
	lengthProto 	    = widthBytesLineProto * height;
#endif
    } else {
	widthBytesLine = BitmapBytePad(stuff->width);
	plane = ((Mask)1) << (pDraw->depth - 1);
	/* only planes asked for */
	length = widthBytesLine * height *
		 Ones(stuff->planeMask & (plane | (plane - 1)));

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	widthBytesLineProto = BitmapBytePadProto(stuff->width);
	lengthProto = widthBytesLineProto * height *
		 Ones(stuff->planeMask & (plane | (plane - 1)));
#endif
    }

#ifdef INTERNAL_VS_EXTERNAL_PADDING
    xgi.length = (lengthProto + 3) >> 2;
#else
    xgi.length = (length + 3) >> 2;
#endif

    if (widthBytesLine == 0 || height == 0) {
	linesPerBuf = 0;
    } else if (widthBytesLine >= IMAGE_BUFSIZE) {
	linesPerBuf = 1;
    } else {
	linesPerBuf = IMAGE_BUFSIZE / widthBytesLine;
	if (linesPerBuf > height)
	    linesPerBuf = height;
    }
    length = linesPerBuf * widthBytesLine;
    if (linesPerBuf < height) {

	/*
	 *	Have to make sure intermediate buffers don't need padding
	 */

	while ((linesPerBuf > 1)
		&& (length & ((1 << LOG2_BYTES_PER_SCANLINE_PAD)-1))) {
	    linesPerBuf--;
	    length -= widthBytesLine;
	}
	while (length & ((1 << LOG2_BYTES_PER_SCANLINE_PAD)-1)) {
	    linesPerBuf++;
	    length += widthBytesLine;
	}
    }
    IF_RETURN((!(pBuf = (char *) ALLOCATE_LOCAL(length))), BadAlloc);

#ifdef INTERNAL_VS_EXTERNAL_PADDING
    /*
     *	Check for protocol/server padding differences
     */

    if (widthBytesLine != widthBytesLineProto)
    	if (!(tmpImage = (char *) ALLOCATE_LOCAL(length))) {
	    DEALLOCATE_LOCAL(pBuf);
	    return (BadAlloc);
	}
#endif

    WriteReplyToClient(client, sizeof (xGetImageReply), &xgi);

    if (linesPerBuf == 0) {

	/*
	 *	Nothing to do
	 */

    } else if (stuff->format == ZPixmap) {
        linesDone = 0;
        while (height - linesDone > 0) {
	  nlines = min(linesPerBuf, height - linesDone);
	  if (pDraw->type == DRAWABLE_WINDOW) {
	    SrcBox.x1 = pDraw->x + stuff->x;
	    SrcBox.y1 = pDraw->y + stuff->y + linesDone;
	    SrcBox.x2 = SrcBox.x1 + stuff->width;
	    SrcBox.y2 = SrcBox.y1 + nlines;
	    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
		
		/*
		 *	If it isn't even on this screen, just continue.
		 */

		if ((SrcBox.x1 >= panoramiXdataPtr[j].x + panoramiXdataPtr[j].width)
		    || (SrcBox.x2 <= panoramiXdataPtr[j].x)
		    || (SrcBox.y1 >= panoramiXdataPtr[j].y+panoramiXdataPtr[j].height)
		    || (SrcBox.y2 <= panoramiXdataPtr[j].y))
		    continue;

		srcParts.x1 = max(SrcBox.x1 - panoramiXdataPtr[j].x, 0);
		srcParts.x2 = min(SrcBox.x2 - panoramiXdataPtr[j].x, 
						    panoramiXdataPtr[j].width);
		srcParts.y1 = max(SrcBox.y1 - panoramiXdataPtr[j].y, 0);
		srcParts.y2 = min(SrcBox.y2 - panoramiXdataPtr[j].y,
						    panoramiXdataPtr[j].height);
		srcParts.width = srcParts.x2 - srcParts.x1;
		srcParts.ByteWidth = PixmapBytePad(srcParts.width,pDraw->depth);
		srcParts.buf = (char *) Xalloc(nlines * srcParts.ByteWidth);
		locDraw = (DrawablePtr) SecurityLookupIDByClass(client,
								pPanoramiXWin->info[j].id,
								RC_DRAWABLE,
								SecurityReadAccess);
		(*pDraw->pScreen->GetImage)(locDraw,
					   srcParts.x1 - locDraw->x,
					   srcParts.y1 - locDraw->y,
					   srcParts.width,
					   srcParts.y2 - srcParts.y1,
					   stuff->format,
					   (unsigned long)stuff->planeMask,
					   srcParts.buf);
		BufPtr = pBuf
		    + srcParts.x1 - stuff->x - (pDraw->x - panoramiXdataPtr[j].x)
		    + widthBytesLine * (srcParts.y1 - stuff->y
				- (pDraw->y + linesDone - panoramiXdataPtr[j].y));
		PartPtr = srcParts.buf;
		for (k = (srcParts.y2 - srcParts.y1); k; k--) {
		    bcopy(PartPtr, BufPtr, srcParts.width);
		    BufPtr += widthBytesLine;
		    PartPtr += srcParts.ByteWidth;
		}
		Xfree(srcParts.buf);
	    }
	  } else {
	    (*pDraw->pScreen->GetImage) (pDraw, stuff->x, stuff->y + linesDone,
					 stuff->width, nlines, stuff->format, 
					 (unsigned long)stuff->planeMask, pBuf);
	  }

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	    /*
	     *	For 64-bit server, convert image to pad to 32 bits
	     */

	    if ( widthBytesLine != widthBytesLineProto ) {
		register char * bufPtr, * protoPtr;
		register int i;

		bzero(tmpImage,length);

		for (i = 0, bufPtr = pBuf, protoPtr = tmpImage; i < nlines;
		    bufPtr += widthBytesLine, protoPtr += widthBytesLineProto, 
			i++)
		    memmove(protoPtr,bufPtr,widthBytesLineProto);

	        /* 
		 *	Note this is NOT a call to WriteSwappedDataToClient,
		 *	as we do NOT byte swap
		 */

	        (void)WriteToClient(client, 
		    (int)(nlines * widthBytesLineProto), tmpImage);
	    } else
#endif
	    {

	        /*
		 *	Note this is NOT a call to WriteSwappedDataToClient,
                 *	as we do NOT byte swap
		 */

	        (void)WriteToClient(client, 
		     (int)(nlines * widthBytesLine), pBuf);
	    }
	    linesDone += nlines;
        }
    } else {						/* XYPixmap	*/
        for (; plane; plane >>= 1) {
	    if (stuff->planeMask & plane) {
	        linesDone = 0;
	        while (height - linesDone > 0) {
		    nlines = min(linesPerBuf, height - linesDone);
	            (*pDraw->pScreen->GetImage) (pDraw,
	                                         stuff->x,
				                 stuff->y + linesDone,
				                 stuff->width, 
				                 nlines,
				                 stuff->format,
				                 (unsigned long)plane,
				                 pBuf);

#ifdef INTERNAL_VS_EXTERNAL_PADDING
	    	    /*
		     *	For 64-bit server, convert image to pad to 32 bits
		     */

	    	    if (widthBytesLine != widthBytesLineProto) {
			register char * bufPtr, * protoPtr;
			register int i;

			bzero(tmpImage, length);

			for (i = 0,bufPtr = pBuf,protoPtr =tmpImage; i < nlines;
		    	      bufPtr += widthBytesLine,
			      protoPtr += widthBytesLineProto, 
			      i++)
		    	    bcopy(bufPtr, protoPtr, widthBytesLineProto);

		        /*
			 *	Note: NOT a call to WriteSwappedDataToClient,
		         *	  as we do NOT byte swap
			 */

		        (void)WriteToClient(client, 
				 (int)(nlines * widthBytesLineProto), tmpImage);
	    	    } else
#endif
		    {

		        /*
			 *	Note: NOT a call to WriteSwappedDataToClient,
		         *	  as we do NOT byte swap
			 */

		        (void)WriteToClient(client, 
				 (int)(nlines * widthBytesLine), pBuf);
		    }
		    linesDone += nlines;
		}
            }
	}
    }
    DEALLOCATE_LOCAL(pBuf);
#ifdef INTERNAL_VS_EXTERNAL_PADDING
    if (widthBytesLine != widthBytesLineProto) 
        DEALLOCATE_LOCAL(tmpImage);
#endif
    return (client->noClientException);
}


int 
PanoramiXPolyText8(register ClientPtr client)
{
    int 	  result, j;

    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC     *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr      pDraw;
    PixmapPtr        pPixmap;
    GC 		    *pGC;
    int		     orig_x, orig_y;
    REQUEST(xPolyTextReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
                      ? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != pDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
	 PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	stuff->x = orig_x;
	stuff->y = orig_y; 
	if (pPanoramiXWin == pPanoramiXRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	} else {
	  if ( (pDraw->type == DRAWABLE_PIXMAP) &&
                /* special case root window bitmap */ 
	        (pDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		 panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	         stuff->x = orig_x - panoramiXdataPtr[j].x;
	         stuff->y = orig_y - panoramiXdataPtr[j].y;
	  }
	}
	if (!j)
	    noPanoramiXExtension = TRUE;
	result = (*SavedProcVector[X_PolyText8])(client);
	noPanoramiXExtension = FALSE;
	BREAK_IF(result != Success);
    }
    return (result);
}

int 
PanoramiXPolyText16(register ClientPtr client)
{
    int 	  result, j;

    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   pDraw;
    GC 		  *pGC;
    int		  orig_x, orig_y;
    REQUEST(xPolyTextReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    /* In the case of Multibuffering, we need to make sure the drawable
       isn't really a pixmap associated to a drawable */ 
    if (!pPanoramiXWin && (stuff->drawable != pDraw->id)) {
         pPanoramiXWin = PanoramiXPmapRoot;
	 PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    }
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	stuff->x = orig_x;
	stuff->y = orig_y; 
	if (pPanoramiXWin == pPanoramiXRoot)  {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	} else {
	  if ( (pDraw->type == DRAWABLE_PIXMAP) &&
	       /* special case root window bitmap */ 
	        (pDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		 panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	         stuff->x = orig_x - panoramiXdataPtr[j].x;
	         stuff->y = orig_y - panoramiXdataPtr[j].y;
	  }
	}
	if (!j)
	    noPanoramiXExtension = TRUE;
	result = (*SavedProcVector[X_PolyText16])(client);
	noPanoramiXExtension = FALSE;
	BREAK_IF(result != Success);
    }
    return (result);
}



int PanoramiXImageText8(ClientPtr client)
{
    int 	  result, j;
    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   pDraw;
    GCPtr 	  pGC;
    int		  orig_x, orig_y;
    REQUEST(xImageTextReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	if (pPanoramiXWin == pPanoramiXRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}else {
	  if ( (pDraw->type == DRAWABLE_PIXMAP) &&
                /* special case root window bitmap */ 
	        (pDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		 panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	         stuff->x = orig_x - panoramiXdataPtr[j].x;
	         stuff->y = orig_y - panoramiXdataPtr[j].y;
	  }
	}
	result = (*SavedProcVector[X_ImageText8])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXImageText16(ClientPtr client)
{
    int 	  result, j;
    PanoramiXWindow *pPanoramiXRoot;
    PanoramiXWindow *pPanoramiXWin;
    PanoramiXGC 	  *pPanoramiXGC = PanoramiXGCRoot;
    DrawablePtr   pDraw;
    GCPtr 	  pGC;
    int		  orig_x, orig_y;
    REQUEST(xImageTextReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    pPanoramiXRoot = (pDraw->type == DRAWABLE_PIXMAP)
					? PanoramiXPmapRoot : PanoramiXWinRoot;
    pPanoramiXWin = pPanoramiXRoot;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->drawable);
    IF_RETURN(!pPanoramiXWin, BadDrawable);
    PANORAMIXFIND_ID(pPanoramiXGC, stuff->gc);
    IF_RETURN(!pPanoramiXGC, BadGC);
    orig_x = stuff->x;
    orig_y = stuff->y;
    FOR_NSCREENS_OR_ONCE((pPanoramiXWin && pPanoramiXGC), j) {
	stuff->drawable = pPanoramiXWin->info[j].id;
	stuff->gc = pPanoramiXGC->info[j].id;
	if (pPanoramiXWin == pPanoramiXRoot) {
	    stuff->x = orig_x - panoramiXdataPtr[j].x;
	    stuff->y = orig_y - panoramiXdataPtr[j].y;
	}else {
	  if ( (pDraw->type == DRAWABLE_PIXMAP) &&
                /* special case root window bitmap */ 
	        (pDraw->width == (panoramiXdataPtr[PanoramiXNumScreens-1].x +
		 panoramiXdataPtr[PanoramiXNumScreens-1].width)) ) {
	         stuff->x = orig_x - panoramiXdataPtr[j].x;
	         stuff->y = orig_y - panoramiXdataPtr[j].y;
	  }
	}
	result = (*SavedProcVector[X_ImageText16])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXCreateColormap(register ClientPtr client)
{
    VisualPtr		pVisual;
    ColormapPtr		pmap;
    Colormap		mid;
    register WindowPtr	pWin;
    ScreenPtr 		pScreen;
    DepthPtr 		 pDepth;

    REQUEST(xCreateColormapReq);

    int 		i, result;
    int                 vid_index, class_index;
    int                 this_vid_index, this_class_index, this_depth;
    int			j = 0;
    VisualID            orig_visual;
    Colormap		cmapID;
    PanoramiXWindow 	*pPanoramiXWin = PanoramiXWinRoot;
    PanoramiXCmap	*localCmap;
    PanoramiXCmap	*pPanoramiXCmap = PanoramiXCmapRoot;
    short		VisualClass;
    Bool		ClassKnown;
    Bool		FoundIt = FALSE;

    REQUEST_SIZE_MATCH(xCreateColormapReq);

    mid = stuff->mid;
    orig_visual = stuff->visual;
    j = 0;
    PANORAMIXFIND_ID(pPanoramiXWin, stuff->window);
    if (pPanoramiXWin) {
	localCmap = (PanoramiXCmap *)Xcalloc(sizeof(PanoramiXCmap));
	IF_RETURN(!localCmap, BadAlloc);
    } else {
	return BadWindow;
    }
    for (j = 0; j <= PanoramiXNumScreens - 1; j++) {
	cmapID = j ? FakeClientID(client->index) : mid;
	localCmap->info[j].id = cmapID;
    }
    localCmap->FreeMe = FALSE;
    PANORAMIXFIND_LAST(pPanoramiXCmap, PanoramiXCmapRoot);
    pPanoramiXCmap->next = localCmap;

    /* Use Screen 0 to get the matching Visual ID */
    pWin = (WindowPtr)SecurityLookupWindow(stuff->window, client,
                                           SecurityReadAccess);
    if ( stuff->visual != CopyFromParent)
      {
       /* Find the correct visual for screen 0 */
	for (class_index = 0; class_index < PanoramiXColorDepthTable[0].numVisuals; 
class_index++)
	  {
	    for (j = 0; j < PanoramiXColorDepthTable[0].panoramiXScreenMap[class_index
].numDepths; j++ )
	      {
		pDepth = (DepthPtr) &pWin->drawable.pScreen->allowedDepths[j];
		for (vid_index = 0; vid_index < PanoramiXColorDepthTable[0].panoramiXScreenMap[class_index].vmap[pDepth->depth].numVids; vid_index++)
		  {
		    if ( stuff->visual == PanoramiXColorDepthTable[0].panoramiXScreenMap[class_index].vmap[pDepth->depth].vid[vid_index] )
		      {
			this_class_index = class_index;
			this_vid_index   = vid_index;
			this_depth       = pDepth->depth;
			FoundIt          = TRUE;
			break;
		      }
		  }
	      }
	  }
      }
    if (!pWin)
         return(BadWindow);
    pScreen = pWin->drawable.pScreen;
    FOR_NSCREENS_OR_ONCE(pPanoramiXWin, j) {
	stuff->mid = localCmap->info[j].id;
	stuff->window = pPanoramiXWin->info[j].id;
        /* Look for the matching visual class, and use its
	   visual id for creating this colormap. */
	if ( orig_visual != CopyFromParent && FoundIt ) 
	  {
	    stuff->visual = PanoramiXColorDepthTable[j].panoramiXScreenMap[this_class_index].vmap[this_depth].vid[this_vid_index];
	  }
	result = (* SavedProcVector[X_CreateColormap])(client);
	BREAK_IF(result != Success);
    }
    if (result != Success) {
       pPanoramiXCmap->next = NULL ;
       if (localCmap)
           Xfree(localCmap);
    }
    return (result);
}


int PanoramiXFreeColormap(ClientPtr client)
{
    ColormapPtr	pmap;
    REQUEST(xResourceReq);
    int         result, j;
    PanoramiXCmap *pPanoramiXCmap = PanoramiXCmapRoot;
    PanoramiXCmap *pPanoramiXCmapback = NULL;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);

    for (; pPanoramiXCmap && (pPanoramiXCmap->info[0].id != stuff->id);
					    pPanoramiXCmap = pPanoramiXCmap->next)
        pPanoramiXCmapback = pPanoramiXCmap;
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
        stuff->id = pPanoramiXCmap->info[j].id;
        result = (* SavedProcVector[X_FreeColormap])(client);
	BREAK_IF(result != Success);
    }
    if ((result == Success) && pPanoramiXCmapback && 
	       pPanoramiXCmap && pPanoramiXCmap->FreeMe) {
        pPanoramiXCmapback->next = pPanoramiXCmap->next;
        Xfree(pPanoramiXCmap);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXInstallColormap(register ClientPtr client)
{
    ColormapPtr pcmp;
    REQUEST(xResourceReq);
    int 	result, j;
    PanoramiXCmap *pPanoramiXCmap = PanoramiXCmapRoot;

    REQUEST_SIZE_MATCH(xResourceReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->id);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
	stuff->id = pPanoramiXCmap->info[j].id;
	result = (* SavedProcVector[X_InstallColormap])(client);
	BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXUninstallColormap(register ClientPtr client)
{
    ColormapPtr pcmp;
    REQUEST(xResourceReq);
    int 	result, j;
    PanoramiXCmap *pPanoramiXCmap = PanoramiXCmapRoot;

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_SIZE_MATCH(xResourceReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->id);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
	stuff->id = pPanoramiXCmap->info[j].id;
	result = (* SavedProcVector[X_UninstallColormap])(client);
	BREAK_IF(result != Success);
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXAllocColor(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xAllocColorReq);

    REQUEST_SIZE_MATCH(xAllocColorReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    if (!pPanoramiXCmap){
	noPanoramiXExtension = TRUE;
	result = (* SavedProcVector[X_AllocColor])(client);
	noPanoramiXExtension = FALSE;
    }else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
            stuff->cmap = pPanoramiXCmap->info[j].id;
	    if (!j)
	        noPanoramiXExtension = TRUE;
            result = (* SavedProcVector[X_AllocColor])(client);
	    noPanoramiXExtension = FALSE;
            BREAK_IF(result != Success);
	}
    }
    return (result);
}


int PanoramiXAllocNamedColor(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xAllocNamedColorReq);

    REQUEST_FIXED_SIZE(xAllocNamedColorReq, stuff->nbytes);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
        stuff->cmap = pPanoramiXCmap->info[j].id;
	if (!j)
	    noPanoramiXExtension = TRUE;
        result = (* SavedProcVector[X_AllocNamedColor])(client);
	noPanoramiXExtension = FALSE;
        BREAK_IF(result != Success);
    }
    return (result);
}


int PanoramiXAllocColorCells(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xAllocColorCellsReq);

    REQUEST_SIZE_MATCH(xAllocColorCellsReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    if (!pPanoramiXCmap) {
	noPanoramiXExtension = TRUE;
	result = (* SavedProcVector[X_AllocColorCells])(client);
        noPanoramiXExtension = FALSE;
    }else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
            stuff->cmap = pPanoramiXCmap->info[j].id;
	    if (!j)
	        noPanoramiXExtension = TRUE;
            result = (* SavedProcVector[X_AllocColorCells])(client);
	    noPanoramiXExtension = FALSE;
	    /* Because id's are eventually searched for in
	       some client list, we don't check for success 
	       on fake id's last id will be real, we really 
	       only care about results related to real id's 
	     BREAK_IF(result != Success);
	     */
	}
    }
    return (result);
}


int PanoramiXFreeColors(register ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xFreeColorsReq);

    PanoramiXGC           *pPanoramiXFreeGC;
    PanoramiXGC           *pPanoramiXFreeGCback = NULL;
    PanoramiXWindow       *pPanoramiXFreeWin;
    PanoramiXWindow       *pPanoramiXFreeWinback = NULL;
    PanoramiXCmap         *pPanoramiXFreeCmap;
    PanoramiXCmap         *pPanoramiXFreeCmapback = NULL;
    PanoramiXPmap         *pPanoramiXFreePmap;
    PanoramiXPmap         *pPanoramiXFreePmapback = NULL;

    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    IF_RETURN(!pPanoramiXCmap, BadColor);
    FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
        stuff->cmap = pPanoramiXCmap->info[j].id;
        result = (* SavedProcVector[X_FreeColors])(client);
	/* Because id's are eventually searched for in
	   some client list, we don't check for success 
	   on fake id's last id will be real, we really 
	   only care about results related to real id's */
    }
    PANORAMIX_FREE(client);
    return (result);
}


int PanoramiXStoreColors(ClientPtr client)
{
    int           result, j;
    PanoramiXCmap	  *pPanoramiXCmap = PanoramiXCmapRoot;
    REQUEST(xStoreColorsReq);

    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);
    PANORAMIXFIND_ID(pPanoramiXCmap, stuff->cmap);
    if (!pPanoramiXCmap)
	result = (* SavedProcVector[X_StoreColors])(client);
    else {
        FOR_NSCREENS_OR_ONCE(pPanoramiXCmap, j) {
            stuff->cmap = pPanoramiXCmap->info[j].id;
            result = (* SavedProcVector[X_StoreColors])(client);
            BREAK_IF(result != Success);
	}
    }
    return (result);
}
