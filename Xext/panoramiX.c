/* $Xorg: panoramiX.c,v 1.5 2000/08/17 19:47:57 cpqbld Exp $ */
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

#define NEED_REPLIES
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

static unsigned char PanoramiXReqCode = 0;
/*
 *	PanoramiX data declarations
 */

int 		PanoramiXPixWidth;
int 		PanoramiXPixHeight;
int 		PanoramiXNumScreens;

PanoramiXData 	*panoramiXdataPtr;
PanoramiXWindow *PanoramiXWinRoot;
PanoramiXGC 	*PanoramiXGCRoot;
PanoramiXCmap 	*PanoramiXCmapRoot;
PanoramiXPmap 	*PanoramiXPmapRoot;

PanoramiXEdge   panoramiXEdgePtr[MAXSCREENS];
RegionRec   	PanoramiXScreenRegion[MAXSCREENS];
PanoramiXCDT	PanoramiXColorDepthTable[MAXSCREENS];
PanoramiXDepth  PanoramiXLargestScreenDepth;

int (* SavedProcVector[256]) ();
ScreenInfo *GlobalScrInfo;

static int panoramiXGeneration;
static int ProcPanoramiXDispatch(); 
/*
 *	Function prototypes
 */

static void locate_neighbors(int);
static void PanoramiXResetProc(ExtensionEntry*);

/*
 *	External references for data variables
 */

extern int SProcPanoramiXDispatch();
extern Bool noPanoramiXExtension;
extern Bool PanoramiXVisibilityNotifySent;
extern WindowPtr *WindowTable;
#if 0
extern ScreenArgsRec screenArgs[MAXSCREENS];
#endif
extern int defaultBackingStore;
extern char *ConnectionInfo;
extern int connBlockScreenStart;
extern int (* ProcVector[256]) ();

/*
 *	Server dispatcher function replacements
 */

int PanoramiXCreateWindow(), 	PanoramiXChangeWindowAttributes();
int PanoramiXDestroyWindow(),	PanoramiXDestroySubwindows();
int PanoramiXChangeSaveSet(), 	PanoramiXReparentWindow();
int PanoramiXMapWindow(), 	PanoramiXMapSubwindows();
int PanoramiXUnmapWindow(), 	PanoramiXUnmapSubwindows();
int PanoramiXConfigureWindow(), PanoramiXCirculateWindow();
int PanoramiXGetGeometry(), 	PanoramiXChangeProperty();
int PanoramiXDeleteProperty(), 	PanoramiXSendEvent();
int PanoramiXCreatePixmap(), 	PanoramiXFreePixmap();
int PanoramiXCreateGC(), 	PanoramiXChangeGC();
int PanoramiXCopyGC();
int PanoramiXSetDashes(), 	PanoramiXSetClipRectangles();
int PanoramiXFreeGC(), 		PanoramiXClearToBackground();
int PanoramiXCopyArea(),	PanoramiXCopyPlane();
int PanoramiXPolyPoint(),	PanoramiXPolyLine();
int PanoramiXPolySegment(),	PanoramiXPolyRectangle();
int PanoramiXPolyArc(),		PanoramiXFillPoly();
int PanoramiXPolyFillArc(),	PanoramiXPolyFillRectangle();
int PanoramiXPutImage(), 	PanoramiXGetImage();
int PanoramiXPolyText8(),	PanoramiXPolyText16();	
int PanoramiXImageText8(),	PanoramiXImageText16();
int PanoramiXCreateColormap(),	PanoramiXFreeColormap();
int PanoramiXInstallColormap(),	PanoramiXUninstallColormap();
int PanoramiXAllocColor(),	PanoramiXAllocNamedColor();
int PanoramiXAllocColorCells();
int PanoramiXFreeColors(), 	PanoramiXStoreColors();

/*
 *	PanoramiXExtensionInit():
 *		Called from InitExtensions in main().  
 *		Register PanoramiXeen Extension
 *		Initialize global variables.
 */ 

void PanoramiXExtensionInit(int argc, char *argv[])
{
 int 	     	i, j, PhyScrNum, ArgScrNum;
 Bool	     	success = FALSE;
 ExtensionEntry *extEntry, *AddExtension();
 PanoramiXData    *panoramiXtempPtr;
 ScreenPtr	pScreen;
    
 if (!noPanoramiXExtension) 
 {
    GlobalScrInfo = &screenInfo;		/* For debug visibility */
    PanoramiXNumScreens = screenInfo.numScreens;
    if (PanoramiXNumScreens == 1) {		/* Only 1 screen 	*/
	noPanoramiXExtension = TRUE;
	return;
    }

    while (panoramiXGeneration != serverGeneration) {
	extEntry = AddExtension(PANORAMIX_PROTOCOL_NAME, 0,0, 
				ProcPanoramiXDispatch,
				SProcPanoramiXDispatch, PanoramiXResetProc, 
				StandardMinorOpcode);
	if (!extEntry) {
	    ErrorF("PanoramiXExtensionInit(): failed to AddExtension\n");
	    break;
 	}
	PanoramiXReqCode = (unsigned char)extEntry->base;

	/*
	 *	First make sure all the basic allocations succeed.  If not,
	 *	run in non-PanoramiXeen mode.
	 */

	panoramiXdataPtr = (PanoramiXData *) Xcalloc(PanoramiXNumScreens * sizeof(PanoramiXData));
	PanoramiXWinRoot = (PanoramiXWindow *) Xcalloc(sizeof(PanoramiXWindow));
	PanoramiXGCRoot  =  (PanoramiXGC *) Xcalloc(sizeof(PanoramiXGC));
	PanoramiXCmapRoot = (PanoramiXCmap *) Xcalloc(sizeof(PanoramiXCmap));
	PanoramiXPmapRoot = (PanoramiXPmap *) Xcalloc(sizeof(PanoramiXPmap));
	BREAK_IF(!(panoramiXdataPtr && PanoramiXWinRoot && PanoramiXGCRoot &&
		   PanoramiXCmapRoot && PanoramiXPmapRoot));

	panoramiXGeneration = serverGeneration;
	success = TRUE;
    }

    if (!success) {
	noPanoramiXExtension = TRUE;
	ErrorF("%s Extension failed to initialize\n", PANORAMIX_PROTOCOL_NAME);
	return;
    }
   
    /* Set up a default configuration base on horizontal ordering */
    for (i = PanoramiXNumScreens -1; i >= 0 ; i--) {
	panoramiXdataPtr[i].above = panoramiXdataPtr[i].below = -1;
	panoramiXdataPtr[i].left = panoramiXdataPtr[i].right = -1;
	panoramiXEdgePtr[i].no_edges = TRUE;
    }
    for (i = PanoramiXNumScreens - 1; i >= 0; i--) {
	 panoramiXdataPtr[i].left = i - 1;
	 panoramiXdataPtr[i].right = i + 1;
    }
    panoramiXdataPtr[PanoramiXNumScreens - 1].right = -1;

    /*
     *	Position the screens relative to each other based on
     *  command line options. 
     */

#if 0
    for (PhyScrNum = PanoramiXNumScreens - 1; PhyScrNum >= 0; PhyScrNum--) {
	if (wsRemapPhysToLogScreens)
	    i = wsPhysToLogScreens[PhyScrNum];
	else 
	    i = PhyScrNum;
	if (i < 0)
	    continue;
	panoramiXdataPtr[i].width = (screenInfo.screens[i])->width;
	panoramiXdataPtr[i].height = (screenInfo.screens[i])->height;
	if (screenArgs[i].flags & ARG_EDGE_L) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_left;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].left = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0) 
	        panoramiXdataPtr[j].right = i;
	    else {
		if ( i >= 1 ) 
		   panoramiXdataPtr[i - 1].right = -1;
	    }
	}
	if (screenArgs[i].flags & ARG_EDGE_R) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_right;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].right = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0) 
	        panoramiXdataPtr[j].left = i;
	}
	if (screenArgs[i].flags & ARG_EDGE_T) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_top;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].above = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0)
	        panoramiXdataPtr[j].below = i;
	}
	if (screenArgs[i].flags & ARG_EDGE_B) {
	    ArgScrNum = screenArgs[PhyScrNum].edge_bottom;
	    if (ArgScrNum < 0)
	       j = -1;
	    else { 
	       if (wsRemapPhysToLogScreens)
	           j = wsPhysToLogScreens[ArgScrNum];
	       else 
		   j = ArgScrNum;
	    }
	    panoramiXdataPtr[i].below = j;
	    panoramiXEdgePtr[i].no_edges = FALSE;
	    if ( j >= 0)
	        panoramiXdataPtr[j].above = i;
	}
    }
#else
    for (PhyScrNum = PanoramiXNumScreens - 1; PhyScrNum >= 0; PhyScrNum--) {
	i = PhyScrNum;
	if (i < 0)
	    continue;
	panoramiXdataPtr[i].width = (screenInfo.screens[i])->width;
	panoramiXdataPtr[i].height = (screenInfo.screens[i])->height;
    }
#endif
    
    /*
     *	Find the upper-left screen and then locate all the others
     */
    panoramiXtempPtr = panoramiXdataPtr;
    for (i = PanoramiXNumScreens; i; i--, panoramiXtempPtr++)
        if (panoramiXtempPtr->above == -1 && panoramiXtempPtr->left == -1)
            break;
    locate_neighbors(PanoramiXNumScreens - i);

    /*
     *	Put our processes into the ProcVector
     */

    for (i = 256; i--; )
	SavedProcVector[i] = ProcVector[i];

    ProcVector[X_CreateWindow] = PanoramiXCreateWindow;
    ProcVector[X_ChangeWindowAttributes] = PanoramiXChangeWindowAttributes;
    ProcVector[X_DestroyWindow] = PanoramiXDestroyWindow;
    ProcVector[X_DestroySubwindows] = PanoramiXDestroySubwindows;
    ProcVector[X_ChangeSaveSet] = PanoramiXChangeSaveSet;
    ProcVector[X_ReparentWindow] = PanoramiXReparentWindow;
    ProcVector[X_MapWindow] = PanoramiXMapWindow;
    ProcVector[X_MapSubwindows] = PanoramiXMapSubwindows;
    ProcVector[X_UnmapWindow] = PanoramiXUnmapWindow;
    ProcVector[X_UnmapSubwindows] = PanoramiXUnmapSubwindows;
    ProcVector[X_ConfigureWindow] = PanoramiXConfigureWindow;
    ProcVector[X_CirculateWindow] = PanoramiXCirculateWindow;
    ProcVector[X_GetGeometry] = PanoramiXGetGeometry;
    ProcVector[X_ChangeProperty] = PanoramiXChangeProperty;
    ProcVector[X_DeleteProperty] = PanoramiXDeleteProperty;
    ProcVector[X_SendEvent] = PanoramiXSendEvent;
    ProcVector[X_CreatePixmap] = PanoramiXCreatePixmap;
    ProcVector[X_FreePixmap] = PanoramiXFreePixmap;
    ProcVector[X_CreateGC] = PanoramiXCreateGC;
    ProcVector[X_ChangeGC] = PanoramiXChangeGC;
    ProcVector[X_CopyGC] = PanoramiXCopyGC;
    ProcVector[X_SetDashes] = PanoramiXSetDashes;
    ProcVector[X_SetClipRectangles] = PanoramiXSetClipRectangles;
    ProcVector[X_FreeGC] = PanoramiXFreeGC;
    ProcVector[X_ClearArea] = PanoramiXClearToBackground;
    ProcVector[X_CopyArea] = PanoramiXCopyArea;;
    ProcVector[X_CopyPlane] = PanoramiXCopyPlane;;
    ProcVector[X_PolyPoint] = PanoramiXPolyPoint;
    ProcVector[X_PolyLine] = PanoramiXPolyLine;
    ProcVector[X_PolySegment] = PanoramiXPolySegment;
    ProcVector[X_PolyRectangle] = PanoramiXPolyRectangle;
    ProcVector[X_PolyArc] = PanoramiXPolyArc;
    ProcVector[X_FillPoly] = PanoramiXFillPoly;
    ProcVector[X_PolyFillRectangle] = PanoramiXPolyFillRectangle;
    ProcVector[X_PolyFillArc] = PanoramiXPolyFillArc;
    ProcVector[X_PutImage] = PanoramiXPutImage;
    ProcVector[X_GetImage] = PanoramiXGetImage;
    ProcVector[X_PolyText8] = PanoramiXPolyText8;
    ProcVector[X_PolyText16] = PanoramiXPolyText16;
    ProcVector[X_ImageText8] = PanoramiXImageText8;
    ProcVector[X_ImageText16] = PanoramiXImageText16;
    ProcVector[X_CreateColormap] = PanoramiXCreateColormap;
    ProcVector[X_FreeColormap] = PanoramiXFreeColormap;
    ProcVector[X_InstallColormap] = PanoramiXInstallColormap;
    ProcVector[X_UninstallColormap] = PanoramiXUninstallColormap;
    ProcVector[X_AllocColor] = PanoramiXAllocColor;
    ProcVector[X_AllocNamedColor] = PanoramiXAllocNamedColor;
    ProcVector[X_AllocColorCells] = PanoramiXAllocColorCells;
    ProcVector[X_FreeColors] = PanoramiXFreeColors;
    ProcVector[X_StoreColors] = PanoramiXStoreColors;

  }
  else 
     return;
}
extern 
Bool PanoramiXCreateConnectionBlock(void)
{
    int i;
    int old_width, old_height;
    int width_mult, height_mult;
    xWindowRoot *root;
    xConnSetup *setup;

    /*
     *	Do normal CreateConnectionBlock but faking it for only one screen
     */

    if (!CreateConnectionBlock()) {
	return FALSE;
    }

    /*
     *  OK, change some dimensions so it looks as if it were one big screen
     */

    setup = (xConnSetup *) ConnectionInfo;
    setup->numRoots = 1;
    root = (xWindowRoot *) (ConnectionInfo + connBlockScreenStart);
    
    old_width = root->pixWidth;
    old_height = root->pixHeight;
    for (i = PanoramiXNumScreens - 1; i >= 0; i--) {
        if (panoramiXdataPtr[i].right == -1 )
            root->pixWidth = panoramiXdataPtr[i].x + panoramiXdataPtr[i].width;
        if (panoramiXdataPtr[i].below == -1)
            root->pixHeight = panoramiXdataPtr[i].y + panoramiXdataPtr[i].height;
    }
    PanoramiXPixWidth = root->pixWidth;
    PanoramiXPixHeight = root->pixHeight;
    width_mult = root->pixWidth / old_width;
    height_mult = root->pixHeight / old_height;
    root->mmWidth *= width_mult;
    root->mmHeight *= height_mult;
    return TRUE;
}

extern 
Bool PanoramiXCreateScreenRegion(pWin)
WindowPtr pWin;
{
   ScreenPtr   pScreen;
   BoxRec      box;
   int         i;
   Bool	       ret;
   
   pScreen = pWin->drawable.pScreen;
   for (i = 0; i < PanoramiXNumScreens; i++) {
        box.x1 = 0 - panoramiXdataPtr[i].x;
        box.x2 = box.x1 + PanoramiXPixWidth;
        box.y1 = 0 - panoramiXdataPtr[i].y;
        box.y2 = box.y1 + PanoramiXPixHeight;
        REGION_INIT(pScreen, &PanoramiXScreenRegion[i], &box, 1);
        ret = REGION_NOTEMPTY(pScreen, &PanoramiXScreenRegion[i]);
        if (!ret)
           return ret;
   }
   return ret;
}

extern
void PanoramiXDestroyScreenRegion(pWin)
WindowPtr pWin;
{
   ScreenPtr   pScreen;
   int         i;
   Bool	       ret;

   pScreen = pWin->drawable.pScreen;
   for (i = 0; i < PanoramiXNumScreens; i++) 
        REGION_DESTROY(pScreen, &PanoramiXScreenRegion[i]);
}

/* 
 *  Assign the Root window and colormap ID's in the PanoramiXScreen Root
 *  linked lists. Note: WindowTable gets setup in dix_main by 
 *  InitRootWindow, and GlobalScrInfo is screenInfo which gets setup
 *  by InitOutput.
 */
extern
void PanoramiXConsolidate(void)
{
    int 	i,j,k,v,d,n, thisMaxDepth;
    int		depthIndex;
    DepthPtr    pDepth, pLargeDepth; 
    VisualPtr   pVisual;
    VisualID	it;
    register WindowPtr pWin, pLargeWin;
    Bool        SameDepth;

    PanoramiXLargestScreenDepth.numDepths = (screenInfo.screens[PanoramiXNumScreens -1])->numDepths;
    PanoramiXLargestScreenDepth.screenNum = PanoramiXNumScreens - 1;
    SameDepth = TRUE;
    for (i = 0; i < 2; i++) 
    {
      for (j =0; j < 6; j++) 
      {
	PanoramiXColorDepthTable[i].panoramiXScreenMap[j].numDepths=0;
	for (n = 0; n < 6; n++)
	{
	  PanoramiXColorDepthTable[i].panoramiXScreenMap[j].listDepths[n]=0;
	}
	for (k = 0; k < 33; k++) 
	{
	   PanoramiXColorDepthTable[i].panoramiXScreenMap[j].vmap[k].numVids=0;
	   for (v = 0; v < 10; v++)
	   {
	     PanoramiXColorDepthTable[i].panoramiXScreenMap[j].vmap[k].vid[v]=0;
	    }
	}
      }
    }
    for (i = PanoramiXNumScreens - 1; i >= 0; i--) 
      {
	PanoramiXWinRoot->info[i].id = WindowTable[i]->drawable.id;
	PanoramiXCmapRoot->info[i].id = (screenInfo.screens[i])->defColormap;

	/* Create a Color-Depth-Table, this will help us deal
	   with mixing graphics boards and visuals, of course
	   given that the boards support multi-screen to begin 
	   with. Fill the panoramiXCDT table by screen, then 
	   visual type and allowable depths.
	*/
        pWin = WindowTable[i];
        if ( (screenInfo.screens[i])->numDepths > 
	          PanoramiXLargestScreenDepth.numDepths )
	  { 
	    PanoramiXLargestScreenDepth.numDepths = (screenInfo.screens[i])->numDepths;
	    PanoramiXLargestScreenDepth.screenNum = i; 
	    SameDepth = FALSE;
	  }
 	for (v = 0, pVisual = pWin->drawable.pScreen->visuals; 
	     v < pWin->drawable.pScreen->numVisuals; v++, pVisual++) 
	  {
	    PanoramiXColorDepthTable[i].panoramiXScreenMap[pVisual->class].numDepths = (screenInfo.screens[i])->numDepths;
	    for ( j = 0; j < (screenInfo.screens[i])->numDepths; j++) 
	      {
	      pDepth = (DepthPtr) &pWin->drawable.pScreen->allowedDepths[j]; 
	      PanoramiXColorDepthTable[i].panoramiXScreenMap[pVisual->class].listDepths[j] = pDepth->depth; 
	      for (d = 0; d < pDepth->numVids; d++)
		{
		  if (pVisual->vid == pDepth->vids[d])
		    {
		      PanoramiXColorDepthTable[i].
 			panoramiXScreenMap[pVisual->class].vmap[pDepth->depth].
 			vid[ 
			  PanoramiXColorDepthTable[i].
 			  panoramiXScreenMap[pVisual->class].
 			  vmap[pDepth->depth].numVids++ 
			] 
 		      = pDepth->vids[d];
  		    }
  		}
  	      }
	  }
	PanoramiXColorDepthTable[i].numVisuals = 6;
     } /* for each screen */
    /*  Fill in ColorDepthTable for mixed visuals with varying depth.
        Can't do that until we figure out how to handle mixed visuals
	and varying card visual/depth initialization. If we can decide
	how to map the relationship, then we can use this table to 
	shove the information into and use for cross-referencing when
	necessary.

	In the meantime, check to see if the screens are the same,
	if they don't then disable panoramiX, print out a message,
	don't support this mode. 
     */
}

/* Since locate_neighbors is recursive, a quick simple example 
   is in order.This mostly so you can see what the initial values are. 

   Given 3 screens:
   upperleft screen[0]
	panoramiXdataPtr[0].x = 0
	panoramiXdataPtr[0].y = 0
	panoramiXdataPtr[0].width  = 640
	panoramiXdataPtr[0].height = 480
	panoramiXdataPtr[0].below = -1
	panoramiXdataPtr[0].right = 1
	panoramiXdataPtr[0].above = -1
	panoramiXdataPtr[0].left = -1
   middle screen[1]
	panoramiXdataPtr[1].x = 0
	panoramiXdataPtr[1].y = 0
	panoramiXdataPtr[1].width  = 640
	panoramiXdataPtr[1].height = 480
	panoramiXdataPtr[1].below = -1
	panoramiXdataPtr[1].right = 2
	panoramiXdataPtr[1].above = -1
	panoramiXdataPtr[1].left = 0
   last right screen[2]
	panoramiXdataPtr[2].x = 0
	panoramiXdataPtr[2].y = 0
	panoramiXdataPtr[2].width  = 640
	panoramiXdataPtr[2].height = 480
	panoramiXdataPtr[2].below = -1
	panoramiXdataPtr[2].right = -1 
	panoramiXdataPtr[2].above = -1
	panoramiXdataPtr[2].left = 1
	
   Calling locate_neighbors(0) results in: 
	panoramiXdataPtr[0].x = 0
	panoramiXdataPtr[0].y = 0
	panoramiXdataPtr[1].x = 640
	panoramiXdataPtr[1].y = 0
	panoramiXdataPtr[2].x = 1280 
	panoramiXdataPtr[2].y = 0
*/

static void locate_neighbors(int i)
{
    int j;
    
    j = panoramiXdataPtr[i].right;
    if ((j != -1) && !panoramiXdataPtr[j].x && !panoramiXdataPtr[j].y) {
	panoramiXdataPtr[j].x = panoramiXdataPtr[i].x + panoramiXdataPtr[i].width;
	panoramiXdataPtr[j].y = panoramiXdataPtr[i].y;
	locate_neighbors(j);
    }
    j = panoramiXdataPtr[i].below;
    if ((j != -1) && !panoramiXdataPtr[j].x && !panoramiXdataPtr[j].y) {
	panoramiXdataPtr[j].y = panoramiXdataPtr[i].y + panoramiXdataPtr[i].height;
	panoramiXdataPtr[j].x = panoramiXdataPtr[i].x;
	locate_neighbors(j);
    }
}



/*
 *	PanoramiXResetProc()
 *		Exit, deallocating as needed.
 */

static void PanoramiXResetProc(extEntry)
    ExtensionEntry* extEntry;
{
    int		i;
    PanoramiXList *pPanoramiXList;
    PanoramiXList *tempList;

    for (pPanoramiXList = PanoramiXPmapRoot; pPanoramiXList; pPanoramiXList = tempList){
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    for (pPanoramiXList = PanoramiXCmapRoot; pPanoramiXList; pPanoramiXList = tempList){
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    for (pPanoramiXList = PanoramiXGCRoot; pPanoramiXList; pPanoramiXList = tempList) {
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    for (pPanoramiXList = PanoramiXWinRoot; pPanoramiXList; pPanoramiXList = tempList) {
	tempList = pPanoramiXList->next;
	Xfree(pPanoramiXList);
    }
    screenInfo.numScreens = PanoramiXNumScreens;
    for (i = 256; i--; )
	ProcVector[i] = SavedProcVector[i];
    Xfree(panoramiXdataPtr);
    
}


int
#if NeedFunctionPrototypes      
ProcPanoramiXQueryVersion (ClientPtr client)
#else
ProcPanoramiXQueryVersion (client)
    register ClientPtr  client;
#endif
{
    REQUEST(xPanoramiXQueryVersionReq);
    xPanoramiXQueryVersionReply		rep;
    register 	int			n;

    REQUEST_SIZE_MATCH (xPanoramiXQueryVersionReq);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.majorVersion = PANORAMIX_MAJOR_VERSION;
    rep.minorVersion = PANORAMIX_MINOR_VERSION;   
    if (client->swapped) { 
        swaps(&rep.sequenceNumber, n);
        swapl(&rep.length, n);     
    }
    WriteToClient(client, sizeof (xPanoramiXQueryVersionReply), (char *)&rep);
    return (client->noClientException);
}

int
#if NeedFunctionPrototypes      
ProcPanoramiXGetState(ClientPtr client)
#else
ProcPanoramiXGetState(client)
        register ClientPtr      client;
#endif
{
	REQUEST(xPanoramiXGetStateReq);
    	WindowPtr			pWin;
	xPanoramiXGetStateReply		rep;
	register int			n;
	
	REQUEST_SIZE_MATCH(xPanoramiXGetStateReq);
	pWin = LookupWindow (stuff->window, client);
	if (!pWin)
	     return BadWindow;
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
	rep.state = !noPanoramiXExtension;
    	if (client->swapped) {
	    swaps (&rep.sequenceNumber, n);
	    swapl (&rep.length, n);
	    swaps (&rep.state, n);
	}	
	WriteToClient (client, sizeof (xPanoramiXGetStateReply), (char *) &rep);
	return client->noClientException;

}

int 
#if NeedFunctionPrototypes      
ProcPanoramiXGetScreenCount(ClientPtr client)
#else
ProcPanoramiXGetScreenCount(client)
	register ClientPtr	client;
#endif
{
	REQUEST(xPanoramiXGetScreenCountReq);
    	WindowPtr			pWin;
	xPanoramiXGetScreenCountReply	rep;
	register int			n;

	REQUEST_SIZE_MATCH(xPanoramiXGetScreenCountReq);
	pWin = LookupWindow (stuff->window, client);
	if (!pWin)
	     return BadWindow;
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
	rep.ScreenCount = PanoramiXNumScreens;
    	if (client->swapped) {
	    swaps (&rep.sequenceNumber, n);
	    swapl (&rep.length, n);
	    swaps (&rep.ScreenCount, n);
	}	
	WriteToClient (client, sizeof (xPanoramiXGetScreenCountReply), (char *) &rep);
	return client->noClientException;
}

int 
#if NeedFunctionPrototypes      
ProcPanoramiXGetScreenSize(ClientPtr client)
#else
ProcPanoramiXGetScreenSize(client)
        register ClientPtr      client;
#endif
{
	REQUEST(xPanoramiXGetScreenSizeReq);
    	WindowPtr			pWin;
	xPanoramiXGetScreenSizeReply	rep;
	register int			n;
	
	REQUEST_SIZE_MATCH(xPanoramiXGetScreenSizeReq);
	pWin = LookupWindow (stuff->window, client);
	if (!pWin)
	     return BadWindow;
	rep.type = X_Reply;
	rep.length = 0;
	rep.sequenceNumber = client->sequence;
		/* screen dimensions */
	rep.width  = panoramiXdataPtr[stuff->screen].width; 
	rep.height = panoramiXdataPtr[stuff->screen].height; 
    	if (client->swapped) {
	    swaps (&rep.sequenceNumber, n);
	    swapl (&rep.length, n);
	    swaps (rep.width, n);
	    swaps (rep.height, n);
	}	
	WriteToClient (client, sizeof (xPanoramiXGetScreenSizeReply), (char *) &rep);
	return client->noClientException;
}


void PrintList(PanoramiXList *head)
{
    int i = 0;

    for (; head; i++, head = head->next)
	fprintf(stderr, "%2d  next = 0x%010lx,   id[0] = 0x%08x,   id[1] = 0x%08x\n",
	    i, head->next, head->info[0].id, head->info[1].id);
}
static int
#if NeedFunctionPrototypes      
ProcPanoramiXDispatch (ClientPtr client)
#else
ProcPanoramiXDispatch (client) 
    ClientPtr   client;
#endif
{   REQUEST(xReq);
    switch (stuff->data)
    {
	case X_PanoramiXQueryVersion:
	     return ProcPanoramiXQueryVersion(client);
	case X_PanoramiXGetState:
	     return ProcPanoramiXGetState(client);
	case X_PanoramiXGetScreenCount:
	     return ProcPanoramiXGetScreenCount(client);
	case X_PanoramiXGetScreenSize:
	     return ProcPanoramiXGetScreenSize(client);
    }
    return BadRequest;
}
