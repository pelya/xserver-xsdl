/* $Xorg: Screen.c,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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
/* $XFree86: xc/programs/Xserver/hw/xnest/Screen.c,v 3.11 2003/01/10 13:29:40 eich Exp $ */

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "dix.h"
#include "mi.h"
#include "mibstore.h"
#include "colormapst.h"
#include "resource.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "XNGC.h"
#include "GCOps.h"
#include "Drawable.h"
#include "XNFont.h"
#include "Color.h"
#include "XNCursor.h"
#include "Visual.h"
#include "Events.h"
#include "Init.h"
#include "mipointer.h"
#include "Args.h"

extern Window xnestParentWindow;

Window xnestDefaultWindows[MAXSCREENS];
Window xnestScreenSaverWindows[MAXSCREENS];

#ifdef PIXPRIV
int xnestScreenGeneration = -1;
#endif

ScreenPtr xnestScreen(window)
     Window window;
{
  int i;
  
  for (i = 0; i < xnestNumScreens; i++)
    if (xnestDefaultWindows[i] == window) 
      return screenInfo.screens[i];

  return NULL;
}

static int offset(mask)
     unsigned long mask;
{
  int count;
  
  for (count = 0; !(mask & 1) && count < 32; count++)
    mask >>= 1;
  
  return count;
}

static Bool xnestSaveScreen(pScreen, what)
     ScreenPtr pScreen;
     int what;
{
  if (xnestSoftwareScreenSaver)
    return False;
  else {
    switch (what) {
    case SCREEN_SAVER_ON:
      XMapRaised(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
      xnestSetScreenSaverColormapWindow(pScreen);
      break;
      
    case SCREEN_SAVER_OFF:
      XUnmapWindow(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
      xnestSetInstalledColormapWindows(pScreen);
      break;
      
    case SCREEN_SAVER_FORCER:
      lastEventTime = GetTimeInMillis();
      XUnmapWindow(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
      xnestSetInstalledColormapWindows(pScreen);
      break;
      
    case SCREEN_SAVER_CYCLE:
      XUnmapWindow(xnestDisplay, xnestScreenSaverWindows[pScreen->myNum]);
      xnestSetInstalledColormapWindows(pScreen);
      break;
    }
    return True;
  }
}

static Bool
xnestCursorOffScreen (ppScreen, x, y)
    ScreenPtr   *ppScreen;
    int         *x, *y;
{
    return FALSE;
}

static void
xnestCrossScreen (pScreen, entering)
    ScreenPtr   pScreen;
    Bool        entering;
{
}

static miPointerScreenFuncRec xnestPointerCursorFuncs =
{
    xnestCursorOffScreen,
    xnestCrossScreen,
    miPointerWarpCursor
};

Bool xnestOpenScreen(index, pScreen, argc, argv)
     int index;
     register ScreenPtr pScreen;
     int argc;
     char *argv[];
{
  VisualPtr visuals;
  DepthPtr depths;
  int numVisuals, numDepths;
  int i, j, depthIndex;
  unsigned long valuemask;
  XSetWindowAttributes attributes;
  XWindowAttributes gattributes;
  XSizeHints sizeHints;

  if (!(AllocateWindowPrivate(pScreen, xnestWindowPrivateIndex,
			    sizeof(xnestPrivWin))  &&
	  AllocateGCPrivate(pScreen, xnestGCPrivateIndex, 
			    sizeof(xnestPrivGC)))) 
    return False;

#ifdef PIXPRIV
  if (xnestScreenGeneration != serverGeneration) {
      if ((xnestPixmapPrivateIndex = AllocatePixmapPrivateIndex()) < 0)
	  return False;
      xnestScreenGeneration = serverGeneration;
  }
  
  if (!AllocatePixmapPrivate(pScreen,xnestPixmapPrivateIndex,
			     sizeof (xnestPrivPixmap)))
      return False;
#endif
  visuals = (VisualPtr)xalloc(xnestNumVisuals * sizeof(VisualRec));
  numVisuals = 0;

  depths = (DepthPtr)xalloc(MAXDEPTH * sizeof(DepthRec));
  depths[0].depth = 1;
  depths[0].numVids = 0;
  depths[0].vids = (VisualID *)xalloc(MAXVISUALSPERDEPTH * sizeof(VisualID));
  numDepths = 1;

  for (i = 0; i < xnestNumVisuals; i++) {
    visuals[numVisuals].vid = FakeClientID(0);
    visuals[numVisuals].class = xnestVisuals[i].class;
    visuals[numVisuals].bitsPerRGBValue = xnestVisuals[i].bits_per_rgb;
    visuals[numVisuals].ColormapEntries = xnestVisuals[i].colormap_size;
    visuals[numVisuals].nplanes = xnestVisuals[i].depth;
    visuals[numVisuals].redMask = xnestVisuals[i].red_mask;
    visuals[numVisuals].greenMask = xnestVisuals[i].green_mask;
    visuals[numVisuals].blueMask = xnestVisuals[i].blue_mask;
    visuals[numVisuals].offsetRed = offset(xnestVisuals[i].red_mask);
    visuals[numVisuals].offsetGreen = offset(xnestVisuals[i].green_mask);
    visuals[numVisuals].offsetBlue = offset(xnestVisuals[i].blue_mask);
    
    depthIndex = UNDEFINED;
    for (j = 0; j < numDepths; j++)
      if (depths[j].depth == xnestVisuals[i].depth) {
	depthIndex = j;
	break;
      }
    
    if (depthIndex == UNDEFINED) {
      depthIndex = numDepths;
      depths[depthIndex].depth = xnestVisuals[i].depth;
      depths[depthIndex].numVids = 0;
      depths[depthIndex].vids = 
	(VisualID *)xalloc(MAXVISUALSPERDEPTH * sizeof(VisualID));
      numDepths++;
    }
    if (depths[depthIndex].numVids >= MAXVISUALSPERDEPTH) {
	FatalError("Visual table overflow");
    }
    depths[depthIndex].vids[depths[depthIndex].numVids] = 
      visuals[numVisuals].vid;
    depths[depthIndex].numVids++;
    
    numVisuals++;
  }

  if (xnestParentWindow != 0) {
    XGetWindowAttributes(xnestDisplay, xnestParentWindow, &gattributes);
    xnestWidth = gattributes.width;
    xnestHeight = gattributes.height;
  }

  /* myNum */
  /* id */
  miScreenInit(pScreen, NULL, xnestWidth, xnestHeight, 1, 1, xnestWidth,
	       visuals[xnestDefaultVisualIndex].nplanes, /* rootDepth */
	       numDepths, depths,
	       visuals[xnestDefaultVisualIndex].vid, /* root visual */
	       numVisuals, visuals);

  miInitializeBackingStore(pScreen);

  miDCInitialize(pScreen, &xnestPointerCursorFuncs);

  pScreen->mmWidth = xnestWidth * DisplayWidthMM(xnestDisplay, 
		       DefaultScreen(xnestDisplay)) / 
			 DisplayWidth(xnestDisplay, 
			   DefaultScreen(xnestDisplay));
  pScreen->mmHeight = xnestHeight * DisplayHeightMM(xnestDisplay, 
		        DefaultScreen(xnestDisplay)) /
			  DisplayHeight(xnestDisplay, 
			    DefaultScreen(xnestDisplay));

  pScreen->defColormap = (Colormap) FakeClientID(0);
  pScreen->minInstalledCmaps = MINCMAPS;
  pScreen->maxInstalledCmaps = MAXCMAPS;
  pScreen->backingStoreSupport = NotUseful;
  pScreen->saveUnderSupport = NotUseful;
  pScreen->whitePixel = xnestWhitePixel;
  pScreen->blackPixel = xnestBlackPixel;
  /* rgf */
  /* GCperDepth */
  /* PixmapPerDepth */
  pScreen->devPrivate = NULL;
  /* WindowPrivateLen */
  /* WindowPrivateSizes */
  /* totalWindowSize */
  /* GCPrivateLen */
  /* GCPrivateSizes */
  /* totalGCSize */

  /* Random screen procedures */

  pScreen->CloseScreen = xnestCloseScreen;
  pScreen->QueryBestSize = xnestQueryBestSize;
  pScreen->SaveScreen = xnestSaveScreen;
  pScreen->GetImage = xnestGetImage;
  pScreen->GetSpans = xnestGetSpans;
  pScreen->PointerNonInterestBox = (void (*)()) 0;
  pScreen->SourceValidate = (void (*)()) 0;

  /* Window Procedures */
  
  pScreen->CreateWindow = xnestCreateWindow;
  pScreen->DestroyWindow = xnestDestroyWindow;
  pScreen->PositionWindow = xnestPositionWindow;
  pScreen->ChangeWindowAttributes = xnestChangeWindowAttributes;
  pScreen->RealizeWindow = xnestRealizeWindow;
  pScreen->UnrealizeWindow = xnestUnrealizeWindow;
  pScreen->PostValidateTree = (void (*)()) 0;
  pScreen->WindowExposures = xnestWindowExposures;
  pScreen->PaintWindowBackground = xnestPaintWindowBackground;
  pScreen->PaintWindowBorder = xnestPaintWindowBorder;
  pScreen->CopyWindow = xnestCopyWindow;
  pScreen->ClipNotify = xnestClipNotify;

  /* Pixmap procedures */

  pScreen->CreatePixmap = xnestCreatePixmap;
  pScreen->DestroyPixmap = xnestDestroyPixmap;

  /* Backing store procedures */
  
  pScreen->SaveDoomedAreas = (void (*)()) 0;
  pScreen->RestoreAreas = (RegionPtr (*)()) 0;
  pScreen->ExposeCopy = (void (*)()) 0;
  pScreen->TranslateBackingStore = (RegionPtr (*)()) 0;
  pScreen->ClearBackingStore = (RegionPtr (*)()) 0;
  pScreen->DrawGuarantee = (void (*)()) 0;

  /* Font procedures */

  pScreen->RealizeFont = xnestRealizeFont;
  pScreen->UnrealizeFont = xnestUnrealizeFont;

  /* Cursor Procedures */

  pScreen->ConstrainCursor = xnestConstrainCursor;
  pScreen->CursorLimits = xnestCursorLimits;
  pScreen->DisplayCursor = xnestDisplayCursor;
  pScreen->RealizeCursor = xnestRealizeCursor;
  pScreen->UnrealizeCursor = xnestUnrealizeCursor;
  pScreen->RecolorCursor = xnestRecolorCursor;
  pScreen->SetCursorPosition = xnestSetCursorPosition;

  /* GC procedures */
  
  pScreen->CreateGC = xnestCreateGC;

  /* Colormap procedures */

  pScreen->CreateColormap = xnestCreateColormap;
  pScreen->DestroyColormap = xnestDestroyColormap;
  pScreen->InstallColormap = xnestInstallColormap;
  pScreen->UninstallColormap = xnestUninstallColormap;
  pScreen->ListInstalledColormaps = xnestListInstalledColormaps;
  pScreen->StoreColors = xnestStoreColors;
  pScreen->ResolveColor = xnestResolveColor;

   pScreen->BitmapToRegion = xnestPixmapToRegion;

  /* OS layer procedures */

  pScreen->BlockHandler = (void (*)())NoopDDA;
  pScreen->WakeupHandler = (void (*)())NoopDDA;
  pScreen->blockData = (pointer)0;
  pScreen->wakeupData = (pointer)0;
  if (!miScreenDevPrivateInit(pScreen, xnestWidth, NULL))
      return FALSE;

  /* devPrivates */

#define POSITION_OFFSET (pScreen->myNum * (xnestWidth + xnestHeight) / 32)
    
  if (xnestDoFullGeneration) {

    valuemask = CWBackPixel | CWEventMask | CWColormap;
    attributes.background_pixel = xnestWhitePixel;
    attributes.event_mask = xnestEventMask;
    attributes.colormap = xnestDefaultVisualColormap(xnestDefaultVisual(pScreen));
    
    if (xnestParentWindow != 0) {
      xnestDefaultWindows[pScreen->myNum] = xnestParentWindow;
      XSelectInput (xnestDisplay, xnestDefaultWindows[pScreen->myNum],
		    xnestEventMask);
    } else
      xnestDefaultWindows[pScreen->myNum] = 
	XCreateWindow(xnestDisplay, 
		      DefaultRootWindow(xnestDisplay),
		      xnestX + POSITION_OFFSET,
		      xnestY + POSITION_OFFSET,
		      xnestWidth, xnestHeight,
		      xnestBorderWidth,
		      pScreen->rootDepth,
		      InputOutput,
		      xnestDefaultVisual(pScreen),
		      valuemask, &attributes);

    if (!xnestWindowName)
      xnestWindowName = argv[0];
    
    sizeHints.flags = PPosition | PSize | PMaxSize;
    sizeHints.x = xnestX + POSITION_OFFSET;
    sizeHints.y = xnestY + POSITION_OFFSET;
    sizeHints.width = sizeHints.max_width = xnestWidth;
    sizeHints.height = sizeHints.max_height = xnestHeight;
    if (xnestUserGeometry & XValue || xnestUserGeometry & YValue)
      sizeHints.flags |= USPosition;
    if (xnestUserGeometry & WidthValue || xnestUserGeometry & HeightValue)
      sizeHints.flags |= USSize;
    XSetStandardProperties(xnestDisplay, 
			   xnestDefaultWindows[pScreen->myNum], 
			   xnestWindowName, 
			   xnestWindowName, 
			   xnestIconBitmap,
			   argv, argc, &sizeHints);
    
    XMapWindow(xnestDisplay, xnestDefaultWindows[pScreen->myNum]);

    valuemask = CWBackPixmap | CWColormap;
    attributes.background_pixmap = xnestScreenSaverPixmap;
    attributes.colormap = 
      DefaultColormap(xnestDisplay, DefaultScreen(xnestDisplay));
    xnestScreenSaverWindows[pScreen->myNum] =
      XCreateWindow(xnestDisplay, 
		    xnestDefaultWindows[pScreen->myNum],
		    0, 0, xnestWidth, xnestHeight, 0,
		    DefaultDepth(xnestDisplay, DefaultScreen(xnestDisplay)),
		    InputOutput, 
		    DefaultVisual(xnestDisplay, DefaultScreen(xnestDisplay)),
		    valuemask, &attributes);
  }
  
  if (!xnestCreateDefaultColormap(pScreen)) return False;
  
  return True;
}

Bool xnestCloseScreen(index, pScreen)
     int index;
     ScreenPtr pScreen;
{
  int i;
  
  for (i = 0; i < pScreen->numDepths; i++)
    xfree(pScreen->allowedDepths[i].vids);
  xfree(pScreen->allowedDepths);
  xfree(pScreen->visuals);
  xfree(pScreen->devPrivate);

  /*
    If xnestDoFullGeneration all x resources will be destroyed upon closing
    the display connection.  There is no need to generate extra protocol.
    */

  return True;
}
