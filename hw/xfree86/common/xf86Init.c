
/*
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 */
/*
 * Copyright (c) 1992-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdlib.h>

#undef HAS_UTSNAME
#if !defined(WIN32) && !defined(__UNIXOS2__)
#define HAS_UTSNAME 1
#include <sys/utsname.h>
#endif

#define NEED_EVENTS
#ifdef __UNIXOS2__
#define I_NEED_OS2_H
#endif
#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include "input.h"
#include "servermd.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "site.h"
#include "mi.h"

#include "compiler.h"

#include "loaderProcs.h"
#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

#define XF86_OS_PRIVS
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Config.h"
#include "xf86_OSlib.h"
#include "xorgVersion.h"
#include "xf86Date.h"
#include "xf86Build.h"
#include "mipointer.h"
#ifdef XINPUT
#include <X11/extensions/XI.h>
#include <X11/extensions/XIproto.h>
#else
#include "inputstr.h"
#endif
#include "xf86DDC.h"
#include "xf86Xinput.h"
#include "xf86InPriv.h"
#ifdef RENDER
#include "picturestr.h"
#endif

#include "globals.h"

#ifdef DPMSExtension
#define DPMS_SERVER
#include <X11/extensions/dpms.h>
#include "dpmsproc.h"
#endif


/* forward declarations */

static void xf86PrintBanner(void);
static void xf86PrintMarkers(void);
static void xf86RunVtInit(void);

#ifdef __UNIXOS2__
extern void os2ServerVideoAccess();
#endif

#ifdef XF86PM
void (*xf86OSPMClose)(void) = NULL;
#endif

static char *baseModules[] = {
	"pcidata",
	NULL
};

/* Common pixmap formats */

static PixmapFormatRec formats[MAXFORMATS] = {
	{ 1,	1,	BITMAP_SCANLINE_PAD },
	{ 4,	8,	BITMAP_SCANLINE_PAD },
	{ 8,	8,	BITMAP_SCANLINE_PAD },
	{ 15,	16,	BITMAP_SCANLINE_PAD },
	{ 16,	16,	BITMAP_SCANLINE_PAD },
	{ 24,	32,	BITMAP_SCANLINE_PAD },
#ifdef RENDER
	{ 32,	32,	BITMAP_SCANLINE_PAD },
#endif
};
#ifdef RENDER
static int numFormats = 7;
#else
static int numFormats = 6;
#endif
static Bool formatsDone = FALSE;

#ifdef USE_DEPRECATED_KEYBOARD_DRIVER
static InputDriverRec XF86KEYBOARD = {
	1,
	"keyboard",
	NULL,
	NULL,
	NULL,
	NULL,
	0
};
#endif

static Bool
xf86CreateRootWindow(WindowPtr pWin)
{
  int ret = TRUE;
  int err = Success;
  ScreenPtr pScreen = pWin->drawable.pScreen;
  RootWinPropPtr pProp;
  CreateWindowProcPtr CreateWindow =
    (CreateWindowProcPtr)(pScreen->devPrivates[xf86CreateRootWindowIndex].ptr);

#ifdef DEBUG
  ErrorF("xf86CreateRootWindow(%p)\n", pWin);
#endif

  if ( pScreen->CreateWindow != xf86CreateRootWindow ) {
    /* Can't find hook we are hung on */
	xf86DrvMsg(pScreen->myNum, X_WARNING /* X_ERROR */,
		  "xf86CreateRootWindow %p called when not in pScreen->CreateWindow %p n",
		   (void *)xf86CreateRootWindow,
		   (void *)pScreen->CreateWindow );
  }

  /* Unhook this function ... */
  pScreen->CreateWindow = CreateWindow;
  pScreen->devPrivates[xf86CreateRootWindowIndex].ptr = NULL;

  /* ... and call the previous CreateWindow fuction, if any */
  if (NULL!=pScreen->CreateWindow) {
    ret = (*pScreen->CreateWindow)(pWin);
  }

  /* Now do our stuff */
  if (xf86RegisteredPropertiesTable != NULL) {
    if (pWin->parent == NULL && xf86RegisteredPropertiesTable != NULL) {
      for (pProp = xf86RegisteredPropertiesTable[pScreen->myNum];
	   pProp != NULL && err==Success;
	   pProp = pProp->next )
	{
	  Atom prop;

	  prop = MakeAtom(pProp->name, strlen(pProp->name), TRUE);
	  err = ChangeWindowProperty(pWin,
				     prop, pProp->type,
				     pProp->format, PropModeReplace,
				     pProp->size, pProp->data,
				     FALSE
				     );
	}
      
      /* Look at err */
      ret &= (err==Success);
      
    } else {
      xf86Msg(X_ERROR, "xf86CreateRootWindow unexpectedly called with "
	      "non-root window %p (parent %p)\n",
	      (void *)pWin, (void *)pWin->parent);
      ret = FALSE;
    }
  }

#ifdef DEBUG
  ErrorF("xf86CreateRootWindow() returns %d\n", ret);
#endif
  return (ret);
}


/*
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *      That includes vt-manager setup, querying all possible devices and
 *      collecting the pixmap formats.
 */

static void
PostConfigInit(void)
{
    /*
     * Install signal handler for unexpected signals
     */
    xf86Info.caughtSignal=FALSE;
    if (!xf86Info.notrapSignals) {
       signal(SIGSEGV,xf86SigHandler);
       signal(SIGILL,xf86SigHandler);
#ifdef SIGEMT
       signal(SIGEMT,xf86SigHandler);
#endif
       signal(SIGFPE,xf86SigHandler);
#ifdef SIGBUS
       signal(SIGBUS,xf86SigHandler);
#endif
#ifdef SIGSYS
       signal(SIGSYS,xf86SigHandler);
#endif
#ifdef SIGXCPU
       signal(SIGXCPU,xf86SigHandler);
#endif
#ifdef SIGXFSZ
       signal(SIGXFSZ,xf86SigHandler);
#endif
#ifdef MEMDEBUG
       signal(SIGUSR2,xf86SigMemDebug);
#endif
    }

#ifdef XF86PM
    xf86OSPMClose = xf86OSPMOpen();
#endif
    
    /* Run an external VT Init program if specified in the config file */
    xf86RunVtInit();

    /* Do this after XF86Config is read (it's normally in OsInit()) */
    OsInitColors();
}

void
InitOutput(ScreenInfo *pScreenInfo, int argc, char **argv)
{
  int                    i, j, k, scr_index;
  static unsigned long   generation = 0;
  char                   **modulelist;
  pointer                *optionlist;
  screenLayoutPtr	 layout;
  Pix24Flags		 screenpix24, pix24;
  MessageType		 pix24From = X_DEFAULT;
  Bool			 pix24Fail = FALSE;
  Bool			 autoconfig = FALSE;
  
#ifdef __UNIXOS2__
  os2ServerVideoAccess();  /* See if we have access to the screen before doing anything */
#endif

  xf86Initialising = TRUE;

  /* Do this early? */
  if (generation != serverGeneration) {
      xf86ScreenIndex = AllocateScreenPrivateIndex();
      xf86CreateRootWindowIndex = AllocateScreenPrivateIndex();
      xf86PixmapIndex = AllocatePixmapPrivateIndex();
      generation = serverGeneration;
  }

  if (serverGeneration == 1) {

    pScreenInfo->numScreens = 0;

    if ((xf86ServerName = strrchr(argv[0], '/')) != 0)
      xf86ServerName++;
    else
      xf86ServerName = argv[0];

    xf86PrintBanner();
    xf86PrintMarkers();
    if (xf86LogFile)  {
	time_t t;
	const char *ct;
	t = time(NULL);
	ct = ctime(&t);
	xf86MsgVerb(xf86LogFileFrom, 0, "Log file: \"%s\", Time: %s",
		    xf86LogFile, ct);
    }

    /* Read and parse the config file */
    if (!xf86DoProbe && !xf86DoConfigure) {
      switch (xf86HandleConfigFile(FALSE)) {
      case CONFIG_OK:
	break;
      case CONFIG_PARSE_ERROR:
	xf86Msg(X_ERROR, "Error parsing the config file\n");
	return;
      case CONFIG_NOFILE:
	autoconfig = TRUE;
	break;
      }
    }

    if (!autoconfig)
	PostConfigInit();

    /* Initialise the loader */
    LoaderInit();

    /* Tell the loader the default module search path */
    LoaderSetPath(xf86ModulePath);

    if (xf86Info.ignoreABI) {
        LoaderSetOptions(LDR_OPT_ABI_MISMATCH_NONFATAL);
    }

#ifdef TESTING
    {
	char **list, **l;
	const char *subdirs[] = {
		"drivers",
		NULL
	};
	const char *patlist[] = {
		"(.*)_drv\\.so",
		"(.*)_drv\\.o",
		NULL
	};
	ErrorF("Getting module listing...\n");
	list = LoaderListDirs(NULL, NULL);
	if (list)
	    for (l = list; *l; l++)
		ErrorF("module: %s\n", *l);
	LoaderFreeDirList(list);
	ErrorF("Getting video driver listing...\n");
	list = LoaderListDirs(subdirs, NULL);
	if (list)
	    for (l = list; *l; l++)
		ErrorF("video driver: %s\n", *l);
	LoaderFreeDirList(list);
	ErrorF("Getting driver listing...\n");
	list = LoaderListDirs(NULL, patlist);
	if (list)
	    for (l = list; *l; l++)
		ErrorF("video driver: %s\n", *l);
	LoaderFreeDirList(list);
    }
#endif
	
    /* Force load mandatory base modules */
    if (!xf86LoadModules(baseModules, NULL))
	FatalError("Unable to load required base modules, Exiting...\n");
    
    xf86OpenConsole();

    /* Do a general bus probe.  This will be a PCI probe for x86 platforms */
    xf86BusProbe();

    if (xf86DoProbe)
	DoProbe();

    if (xf86DoConfigure)
	DoConfigure();

    if (autoconfig) {
	if (!xf86AutoConfig()) {
	    xf86Msg(X_ERROR, "Auto configuration failed\n");
	    return;
	}
	PostConfigInit();
    }

    /* Initialise the resource broker */
    xf86ResourceBrokerInit();

    /* Load all modules specified explicitly in the config file */
    if ((modulelist = xf86ModulelistFromConfig(&optionlist))) {
      xf86LoadModules(modulelist, optionlist);
      xfree(modulelist);
      xfree(optionlist);
    }

    /* Load all driver modules specified in the config file */
    if ((modulelist = xf86DriverlistFromConfig())) {
      xf86LoadModules(modulelist, NULL);
      xfree(modulelist);
    }

#ifdef USE_DEPRECATED_KEYBOARD_DRIVER
    /* Setup the builtin input drivers */
    xf86AddInputDriver(&XF86KEYBOARD, NULL, 0);
#endif
    /* Load all input driver modules specified in the config file. */
    if ((modulelist = xf86InputDriverlistFromConfig())) {
      xf86LoadModules(modulelist, NULL);
      xfree(modulelist);
    }

    /*
     * It is expected that xf86AddDriver()/xf86AddInputDriver will be
     * called for each driver as it is loaded.  Those functions save the
     * module pointers for drivers.
     * XXX Nothing keeps track of them for other modules.
     */
    /* XXX What do we do if not all of these could be loaded? */

    /*
     * At this point, xf86DriverList[] is all filled in with entries for
     * each of the drivers to try and xf86NumDrivers has the number of
     * drivers.  If there are none, return now.
     */

    if (xf86NumDrivers == 0) {
      xf86Msg(X_ERROR, "No drivers available.\n");
      return;
    }

    /*
     * Call each of the Identify functions and call the driverFunc to check
     * if HW access is required.  The Identify functions print out some
     * identifying information, and anything else that might be
     * needed at this early stage.
     */

    for (i = 0; i < xf86NumDrivers; i++) {
	xorgHWFlags flags;
      /* The Identify function is mandatory, but if it isn't there continue */
	if (xf86DriverList[i]->Identify != NULL)
	    xf86DriverList[i]->Identify(0);
	else {
	    xf86Msg(X_WARNING, "Driver `%s' has no Identify function\n",
		  xf86DriverList[i]->driverName ? xf86DriverList[i]->driverName
					     : "noname");
	}
	if (!xorgHWAccess
	    && (!xf86DriverList[i]->driverFunc
		|| !xf86DriverList[i]->driverFunc(NULL,
						  GET_REQUIRED_HW_INTERFACES,
						  &flags)
		|| NEED_IO_ENABLED(flags)))
	    xorgHWAccess = TRUE;
    }

    /* Enable full I/O access */
    if (xorgHWAccess) {
	if(!xf86EnableIO())
	    /* oops, we have failed */
	    xorgHWAccess = FALSE;
    }

    /*
     * Locate bus slot that had register IO enabled at server startup
     */

    xf86AccessInit();
    xf86FindPrimaryDevice();

    /*
     * Now call each of the Probe functions.  Each successful probe will
     * result in an extra entry added to the xf86Screens[] list for each
     * instance of the hardware found.
     */

    for (i = 0; i < xf86NumDrivers; i++) {
	xorgHWFlags flags;
	if (!xorgHWAccess) {
	    if (!xf86DriverList[i]->driverFunc
		|| !xf86DriverList[i]->driverFunc(NULL,
						 GET_REQUIRED_HW_INTERFACES,
						  &flags)
		|| NEED_IO_ENABLED(flags)) 
		continue;
	}
	    
	if (xf86DriverList[i]->Probe != NULL)
	    xf86DriverList[i]->Probe(xf86DriverList[i], PROBE_DEFAULT);
	else {
	    xf86MsgVerb(X_WARNING, 0,
			"Driver `%s' has no Probe function (ignoring)\n",
			xf86DriverList[i]->driverName
			? xf86DriverList[i]->driverName : "noname");
	}
	xf86SetPciVideo(NULL,NONE);
    }

    /*
     * If nothing was detected, return now.
     */

    if (xf86NumScreens == 0) {
      xf86Msg(X_ERROR, "No devices detected.\n");
      return;
    }

    /*
     * Match up the screens found by the probes against those specified
     * in the config file.  Remove the ones that won't be used.  Sort
     * them in the order specified.
     */

    /*
     * What is the best way to do this?
     *
     * For now, go through the screens allocated by the probes, and
     * look for screen config entry which refers to the same device
     * section as picked out by the probe.
     *
     */

    for (i = 0; i < xf86NumScreens; i++) {
      for (layout = xf86ConfigLayout.screens; layout->screen != NULL;
	   layout++) {
	  Bool found = FALSE;
	  for (j = 0; j < xf86Screens[i]->numEntities; j++) {
	
	      GDevPtr dev =
		xf86GetDevFromEntity(xf86Screens[i]->entityList[j],
				     xf86Screens[i]->entityInstanceList[j]);

	      if (dev == layout->screen->device) {
		  /* A match has been found */
		  xf86Screens[i]->confScreen = layout->screen;
		  found = TRUE;
		  break;
	      }
	  }
	  if (found) break;
      }
      if (layout->screen == NULL) {
	/* No match found */
	xf86Msg(X_ERROR,
	    "Screen %d deleted because of no matching config section.\n", i);
        xf86DeleteScreen(i--, 0);
      }
    }

    /*
     * If no screens left, return now.
     */

    if (xf86NumScreens == 0) {
      xf86Msg(X_ERROR,
	      "Device(s) detected, but none match those in the config file.\n");
      return;
    }

    xf86PostProbe();
    xf86EntityInit();

    /*
     * Sort the drivers to match the requested ording.  Using a slow
     * bubble sort.
     */
    for (j = 0; j < xf86NumScreens - 1; j++) {
	for (i = 0; i < xf86NumScreens - j - 1; i++) {
	    if (xf86Screens[i + 1]->confScreen->screennum <
		xf86Screens[i]->confScreen->screennum) {
		ScrnInfoPtr tmpScrn = xf86Screens[i + 1];
		xf86Screens[i + 1] = xf86Screens[i];
		xf86Screens[i] = tmpScrn;
	    }
	}
    }
    /* Fix up the indexes */
    for (i = 0; i < xf86NumScreens; i++) {
	xf86Screens[i]->scrnIndex = i;
    }

    /*
     * Call the driver's PreInit()'s to complete initialisation for the first
     * generation.
     */
    
    for (i = 0; i < xf86NumScreens; i++) {
	xf86EnableAccess(xf86Screens[i]);
	if (xf86Screens[i]->PreInit &&
	    xf86Screens[i]->PreInit(xf86Screens[i], 0))
	    xf86Screens[i]->configured = TRUE;
    }
    for (i = 0; i < xf86NumScreens; i++)
	if (!xf86Screens[i]->configured)
	    xf86DeleteScreen(i--, 0);
    
    /*
     * If no screens left, return now.
     */

    if (xf86NumScreens == 0) {
      xf86Msg(X_ERROR,
	      "Screen(s) found, but none have a usable configuration.\n");
      return;
    }

    /* This could be moved into a separate function */

    /*
     * Check that all screens have initialised the mandatory function
     * entry points.  Delete those which have not.
     */

#define WARN_SCREEN(func) \
    xf86Msg(X_ERROR, "Driver `%s' has no %s function, deleting.\n", \
	   xf86Screens[i]->name, (warned++, func))

    for (i = 0; i < xf86NumScreens; i++) {
      int warned = 0;
      if (xf86Screens[i]->name == NULL) {
	xf86Screens[i]->name = xnfalloc(strlen("screen") + 1 + 1);
	if (i < 10)
	  sprintf(xf86Screens[i]->name, "screen%c", i + '0');
	else
	  sprintf(xf86Screens[i]->name, "screen%c", i - 10 + 'A');
	xf86MsgVerb(X_WARNING, 0,
		    "Screen driver %d has no name set, using `%s'.\n",
		    i, xf86Screens[i]->name);
      }
      if (xf86Screens[i]->ScreenInit == NULL)
	WARN_SCREEN("ScreenInit");
      if (xf86Screens[i]->EnterVT == NULL)
	WARN_SCREEN("EnterVT");
      if (xf86Screens[i]->LeaveVT == NULL)
	WARN_SCREEN("LeaveVT");
      if (warned)
	xf86DeleteScreen(i--, 0);
    }

    /*
     * If no screens left, return now.
     */

    if (xf86NumScreens == 0) {
      xf86Msg(X_ERROR, "Screen(s) found, but drivers were unusable.\n");
      return;
    }

    /* XXX Should this be before or after loading dependent modules? */
    if (xf86ProbeOnly)
    {
      OsCleanup(TRUE);
      AbortDDX();
      fflush(stderr);
      exit(0);
    }

    /* Remove (unload) drivers that are not required */
    for (i = 0; i < xf86NumDrivers; i++)
	if (xf86DriverList[i] && xf86DriverList[i]->refCount <= 0)
	    xf86DeleteDriver(i);

    /*
     * At this stage we know how many screens there are.
     */

    for (i = 0; i < xf86NumScreens; i++)
      xf86InitViewport(xf86Screens[i]);

    /*
     * Collect all pixmap formats and check for conflicts at the display
     * level.  Should we die here?  Or just delete the offending screens?
     * Also, should this be done for -probeonly?
     */
    screenpix24 = Pix24DontCare;
    for (i = 0; i < xf86NumScreens; i++) {
	if (xf86Screens[i]->imageByteOrder !=
	    xf86Screens[0]->imageByteOrder)
	    FatalError("Inconsistent display bitmapBitOrder.  Exiting\n");
	if (xf86Screens[i]->bitmapScanlinePad !=
	    xf86Screens[0]->bitmapScanlinePad)
	    FatalError("Inconsistent display bitmapScanlinePad.  Exiting\n");
	if (xf86Screens[i]->bitmapScanlineUnit !=
	    xf86Screens[0]->bitmapScanlineUnit)
	    FatalError("Inconsistent display bitmapScanlineUnit.  Exiting\n");
	if (xf86Screens[i]->bitmapBitOrder !=
	    xf86Screens[0]->bitmapBitOrder)
	    FatalError("Inconsistent display bitmapBitOrder.  Exiting\n");

	/* Determine the depth 24 pixmap format the screens would like */
	if (xf86Screens[i]->pixmap24 != Pix24DontCare) {
	    if (screenpix24 == Pix24DontCare)
		screenpix24 = xf86Screens[i]->pixmap24;
	    else if (screenpix24 != xf86Screens[i]->pixmap24)
		FatalError("Inconsistent depth 24 pixmap format.  Exiting\n");
	}
    }
    /* check if screenpix24 is consistent with the config/cmdline */
    if (xf86Info.pixmap24 != Pix24DontCare) {
	pix24 = xf86Info.pixmap24;
	pix24From = xf86Info.pix24From;
	if (screenpix24 != Pix24DontCare && screenpix24 != xf86Info.pixmap24)
	    pix24Fail = TRUE;
    } else if (screenpix24 != Pix24DontCare) {
	pix24 = screenpix24;
	pix24From = X_PROBED;
    } else
	pix24 = Pix24Use32;

    if (pix24Fail)
	FatalError("Screen(s) can't use the required depth 24 pixmap format"
		   " (%d).  Exiting\n", PIX24TOBPP(pix24));

    /* Initialise the depth 24 format */
    for (j = 0; j < numFormats && formats[j].depth != 24; j++)
	;
    formats[j].bitsPerPixel = PIX24TOBPP(pix24);

    /* Collect additional formats */
    for (i = 0; i < xf86NumScreens; i++) {
	for (j = 0; j < xf86Screens[i]->numFormats; j++) {
	    for (k = 0; ; k++) {
		if (k >= numFormats) {
		    if (k >= MAXFORMATS)
			FatalError("Too many pixmap formats!  Exiting\n");
		    formats[k] = xf86Screens[i]->formats[j];
		    numFormats++;
		    break;
		}
		if (formats[k].depth == xf86Screens[i]->formats[j].depth) {
		    if ((formats[k].bitsPerPixel ==
			 xf86Screens[i]->formats[j].bitsPerPixel) &&
		        (formats[k].scanlinePad ==
			 xf86Screens[i]->formats[j].scanlinePad))
			break;
		    FatalError("Inconsistent pixmap format for depth %d."
			       "  Exiting\n", formats[k].depth);
		}
	    }
	}
    }
    formatsDone = TRUE;

    if (xf86Info.vtno >= 0 ) {
#define VT_ATOM_NAME         "XFree86_VT"
      Atom VTAtom=-1;
      CARD32  *VT = NULL;
      int  ret;

      /* This memory needs to stay available until the screen has been
	 initialized, and we can create the property for real.
      */
      if ( (VT = xalloc(sizeof(CARD32)))==NULL ) {
	FatalError("Unable to make VT property - out of memory. Exiting...\n");
      }
      *VT = xf86Info.vtno;
    
      VTAtom = MakeAtom(VT_ATOM_NAME, sizeof(VT_ATOM_NAME), TRUE);

      for (i = 0, ret = Success; i < xf86NumScreens && ret == Success; i++) {
	ret = xf86RegisterRootWindowProperty(xf86Screens[i]->scrnIndex,
					     VTAtom, XA_INTEGER, 32, 
					     1, VT );
	if (ret != Success)
	  xf86DrvMsg(xf86Screens[i]->scrnIndex, X_WARNING,
		     "Failed to register VT property\n");
      }
    }

    /* If a screen uses depth 24, show what the pixmap format is */
    for (i = 0; i < xf86NumScreens; i++) {
	if (xf86Screens[i]->depth == 24) {
	    xf86Msg(pix24From, "Depth 24 pixmap format is %d bpp\n",
		    PIX24TOBPP(pix24));
	    break;
	}
    }

#if BITMAP_SCANLINE_UNIT == 64
    /*
     * cfb24 doesn't currently work on architectures with a 64 bit
     * BITMAP_SCANLINE_UNIT, so check for 24 bit pixel size for pixmaps
     * or framebuffers.
     */
    {
	Bool usesCfb24 = FALSE;

	if (PIX24TOBPP(pix24) == 24)
	    usesCfb24 = TRUE;
	for (i = 0; i < xf86NumScreens; i++)
	    if (xf86Screens[i]->bitsPerPixel == 24)
		usesCfb24 = TRUE;
	if (usesCfb24) {
	    FatalError("24-bit pixel size is not supported on systems with"
			" 64-bit scanlines.\n");
	}
    }
#endif

#ifdef XKB
    xf86InitXkb();
#endif
    /* set up the proper access funcs */
    xf86PostPreInit();

    AddCallback(&ServerGrabCallback, xf86GrabServerCallback, NULL);
    
  } else {
    /*
     * serverGeneration != 1; some OSs have to do things here, too.
     */
    xf86OpenConsole();

#ifdef XF86PM
    /*
      should we reopen it here? We need to deal with an already opened
      device. We could leave this to the OS layer. For now we simply
      close it here
    */
    if (xf86OSPMClose)
        xf86OSPMClose();
    if ((xf86OSPMClose = xf86OSPMOpen()) != NULL)
	xf86MsgVerb(X_INFO, 3, "APM registered successfully\n");
#endif

    /* Make sure full I/O access is enabled */
    if (xorgHWAccess)
	xf86EnableIO();
  }

#if 0
  /*
   * Install signal handler for unexpected signals
   */
  xf86Info.caughtSignal=FALSE;
  if (!xf86Info.notrapSignals)
  {
     signal(SIGSEGV,xf86SigHandler);
     signal(SIGILL,xf86SigHandler);
#ifdef SIGEMT
     signal(SIGEMT,xf86SigHandler);
#endif
     signal(SIGFPE,xf86SigHandler);
#ifdef SIGBUS
     signal(SIGBUS,xf86SigHandler);
#endif
#ifdef SIGSYS
     signal(SIGSYS,xf86SigHandler);
#endif
#ifdef SIGXCPU
     signal(SIGXCPU,xf86SigHandler);
#endif
#ifdef SIGXFSZ
     signal(SIGXFSZ,xf86SigHandler);
#endif
  }
#endif

  /*
   * Use the previously collected parts to setup pScreenInfo
   */

  pScreenInfo->imageByteOrder = xf86Screens[0]->imageByteOrder;
  pScreenInfo->bitmapScanlinePad = xf86Screens[0]->bitmapScanlinePad;
  pScreenInfo->bitmapScanlineUnit = xf86Screens[0]->bitmapScanlineUnit;
  pScreenInfo->bitmapBitOrder = xf86Screens[0]->bitmapBitOrder;
  pScreenInfo->numPixmapFormats = numFormats;
  for (i = 0; i < numFormats; i++)
    pScreenInfo->formats[i] = formats[i];

  /* Make sure the server's VT is active */
    
  if (serverGeneration != 1) {
    xf86Resetting = TRUE;
    /* All screens are in the same state, so just check the first */
    if (!xf86Screens[0]->vtSema) {
#ifdef HAS_USL_VTS
      ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ);
#endif
      xf86AccessEnter();
      xf86EnterServerState(SETUP);
    } 
  }
#ifdef SCO325
  else {
    /*
     * Under SCO we must ack that we got the console at startup,
     * I think this is the safest way to assure it.
     */
    static int once = 1;
    if (once) {
      once = 0;
      if (ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)
        xf86Msg(X_WARNING, "VT_ACKACQ failed");
    }
  }
#endif /* SCO325 */

  for (i = 0; i < xf86NumScreens; i++) {    
   	xf86EnableAccess(xf86Screens[i]);
	/*
	 * Almost everything uses these defaults, and many of those that
	 * don't, will wrap them.
	 */
	xf86Screens[i]->EnableDisableFBAccess = xf86EnableDisableFBAccess;
	xf86Screens[i]->SetDGAMode = xf86SetDGAMode;
	xf86Screens[i]->DPMSSet = NULL;
	xf86Screens[i]->LoadPalette = NULL; 
	xf86Screens[i]->SetOverscan = NULL;
	xf86Screens[i]->DriverFunc = NULL;
	xf86Screens[i]->pScreen = NULL;
	scr_index = AddScreen(xf86Screens[i]->ScreenInit, argc, argv);
      if (scr_index == i) {
	/*
	 * Hook in our ScrnInfoRec, and initialise some other pScreen
	 * fields.
	 */
	screenInfo.screens[scr_index]->devPrivates[xf86ScreenIndex].ptr
	  = (pointer)xf86Screens[i];
	xf86Screens[i]->pScreen = screenInfo.screens[scr_index];
	/* The driver should set this, but make sure it is set anyway */
	xf86Screens[i]->vtSema = TRUE;
      } else {
	/* This shouldn't normally happen */
	FatalError("AddScreen/ScreenInit failed for driver %d\n", i);
      }

#ifdef DEBUG
      ErrorF("InitOutput - xf86Screens[%d]->pScreen = %p\n",
	     i, xf86Screens[i]->pScreen );
      ErrorF("xf86Screens[%d]->pScreen->CreateWindow = %p\n",
	     i, xf86Screens[i]->pScreen->CreateWindow );
#endif

      screenInfo.screens[scr_index]->devPrivates[xf86CreateRootWindowIndex].ptr
	= (void*)(xf86Screens[i]->pScreen->CreateWindow);
      xf86Screens[i]->pScreen->CreateWindow = xf86CreateRootWindow;

#ifdef RENDER
    if (PictureGetSubpixelOrder (xf86Screens[i]->pScreen) == SubPixelUnknown)
    {
	xf86MonPtr DDC = (xf86MonPtr)(xf86Screens[i]->monitor->DDC); 
	PictureSetSubpixelOrder (xf86Screens[i]->pScreen,
				 DDC ?
				 (DDC->features.input_type ?
				  SubPixelHorizontalRGB : SubPixelNone) :
				 SubPixelUnknown);
    }
#endif
#ifdef RANDR
    if (!xf86Info.disableRandR)
	xf86RandRInit (screenInfo.screens[scr_index]);
    xf86Msg(xf86Info.randRFrom, "RandR %s\n",
	    xf86Info.disableRandR ? "disabled" : "enabled");
#endif
#ifdef NOT_USED
      /*
       * Here we have to let the driver getting access of the VT. Note that
       * this doesn't mean that the graphics board may access automatically
       * the monitor. If the monitor is shared this is done in xf86CrossScreen!
       */
      if (!xf86Info.sharedMonitor) (xf86Screens[i]->EnterLeaveMonitor)(ENTER);
#endif
  }

  xf86PostScreenInit();

  xf86InitOrigins();

  xf86Resetting = FALSE;
  xf86Initialising = FALSE;

  RegisterBlockAndWakeupHandlers((BlockHandlerProcPtr)NoopDDA, xf86Wakeup,
				 NULL);
}


static InputDriverPtr
MatchInput(IDevPtr pDev)
{
    int i;

    for (i = 0; i < xf86NumInputDrivers; i++) {
	if (xf86InputDriverList[i] && xf86InputDriverList[i]->driverName &&
	    xf86NameCmp(pDev->driver, xf86InputDriverList[i]->driverName) == 0)
	    return xf86InputDriverList[i];
    }
    return NULL;
}


/*
 * InitInput --
 *      Initialize all supported input devices.
 */

void
InitInput(argc, argv)
     int     	  argc;
     char    	  **argv;
{
    IDevPtr pDev;
    InputDriverPtr pDrv;
    InputInfoPtr pInfo;
    static InputInfoPtr coreKeyboard = NULL, corePointer = NULL;

    xf86Info.vtRequestsPending = FALSE;
    xf86Info.inputPending = FALSE;

    if (serverGeneration == 1) {
	/* Call the PreInit function for each input device instance. */
	for (pDev = xf86ConfigLayout.inputs; pDev && pDev->identifier; pDev++) {
#ifdef USE_DEPRECATED_KEYBOARD_DRIVER
	    /* XXX The keyboard driver is a special case for now. */
	    if (!xf86NameCmp(pDev->driver, "keyboard")) {
		xf86MsgVerb(X_WARNING, 0, "*** WARNING the legacy keyboard driver \"keyboard\" is deprecated\n");
		xf86MsgVerb(X_WARNING, 0, "*** and will be removed in the next release of the Xorg server.\n");
		xf86MsgVerb(X_WARNING, 0, "*** Please consider using the the new \"kbd\" driver for \"%s\".\n",
			pDev->identifier);

		continue;
	    }
#endif

	    if ((pDrv = MatchInput(pDev)) == NULL) {
		xf86Msg(X_ERROR, "No Input driver matching `%s'\n", pDev->driver);
		/* XXX For now, just continue. */
		continue;
	    }
	    if (!pDrv->PreInit) {
		xf86MsgVerb(X_WARNING, 0,
		    "Input driver `%s' has no PreInit function (ignoring)\n",
		    pDrv->driverName);
		continue;
	    }
	    pInfo = pDrv->PreInit(pDrv, pDev, 0);
	    if (!pInfo) {
		xf86Msg(X_ERROR, "PreInit returned NULL for \"%s\"\n",
			pDev->identifier);
		continue;
	    } else if (!(pInfo->flags & XI86_CONFIGURED)) {
		xf86Msg(X_ERROR, "PreInit failed for input device \"%s\"\n",
			pDev->identifier);
		xf86DeleteInput(pInfo, 0);
		continue;
	    }
	    if (pInfo->flags & XI86_CORE_KEYBOARD) {
		if (coreKeyboard) {
		    xf86Msg(X_ERROR,
		      "Attempt to register more than one core keyboard (%s)\n",
		      pInfo->name);
		    pInfo->flags &= ~XI86_CORE_KEYBOARD;
		} else {
		    if (!(pInfo->flags & XI86_KEYBOARD_CAPABLE)) {
			/* XXX just a warning for now */
			xf86Msg(X_WARNING,
			    "%s: does not have core keyboard capabilities\n",
			    pInfo->name);
		    }
		    coreKeyboard = pInfo;
		}
	    }
	    if (pInfo->flags & XI86_CORE_POINTER) {
		if (corePointer) {
		    xf86Msg(X_ERROR,
			"Attempt to register more than one core pointer (%s)\n",
			pInfo->name);
		    pInfo->flags &= ~XI86_CORE_POINTER;
		} else {
		    if (!(pInfo->flags & XI86_POINTER_CAPABLE)) {
			/* XXX just a warning for now */
			xf86Msg(X_WARNING,
			    "%s: does not have core pointer capabilities\n",
			    pInfo->name);
		    }
		    corePointer = pInfo;
		}
	    }
	}
	if (!corePointer) {
	    xf86Msg(X_WARNING, "No core pointer registered\n");
	    /* XXX register a dummy core pointer */
	}
#ifdef NEW_KBD
	if (!coreKeyboard) {
	    xf86Msg(X_WARNING, "No core keyboard registered\n");
	    /* XXX register a dummy core keyboard */
	}
#endif
    }

    /* Initialise all input devices. */
    pInfo = xf86InputDevs;
    while (pInfo) {
	xf86ActivateDevice(pInfo);
	pInfo = pInfo->next;
    }

    if (coreKeyboard) {
      xf86Info.pKeyboard = coreKeyboard->dev;
      xf86Info.kbdEvents = NULL; /* to prevent the internal keybord driver usage*/
    }
    else {
#ifdef USE_DEPRECATED_KEYBOARD_DRIVER
      /* Only set this if we're allowing the old driver. */
	if (xf86Info.kbdProc != NULL) 
	    xf86Info.pKeyboard = AddInputDevice(xf86Info.kbdProc, TRUE);
#endif
    }
    if (corePointer)
	xf86Info.pMouse = corePointer->dev;
    if (xf86Info.pKeyboard)
      RegisterKeyboardDevice(xf86Info.pKeyboard); 

  miRegisterPointerDevice(screenInfo.screens[0], xf86Info.pMouse);
#ifdef XINPUT
  xf86eqInit ((DevicePtr)xf86Info.pKeyboard, (DevicePtr)xf86Info.pMouse);
#else
  mieqInit ((DevicePtr)xf86Info.pKeyboard, (DevicePtr)xf86Info.pMouse);
#endif
}

#ifndef SET_STDERR_NONBLOCKING
#define SET_STDERR_NONBLOCKING 1
#endif

/*
 * OsVendorInit --
 *      OS/Vendor-specific initialisations.  Called from OsInit(), which
 *      is called by dix before establishing the well known sockets.
 */
 
void
OsVendorInit()
{
  static Bool beenHere = FALSE;

  xf86WrapperInit();

#ifdef SIGCHLD
  signal(SIGCHLD, SIG_DFL);	/* Need to wait for child processes */
#endif
  OsDelayInitColors = TRUE;
  loadableFonts = TRUE;

  if (!beenHere)
    xf86LogInit();

#if SET_STDERR_NONBLOCKING
        /* Set stderr to non-blocking. */
#ifndef O_NONBLOCK
#if defined(FNDELAY)
#define O_NONBLOCK FNDELAY
#elif defined(O_NDELAY)
#define O_NONBLOCK O_NDELAY
#endif
#endif

#ifdef O_NONBLOCK
  if (!beenHere) {
#if !defined(__EMX__)
    if (geteuid() == 0 && getuid() != geteuid())
#endif
    {
      int status;

      status = fcntl(fileno(stderr), F_GETFL, 0);
      if (status != -1) {
	fcntl(fileno(stderr), F_SETFL, status | O_NONBLOCK);
      }
    }
  }
#endif
#endif

  beenHere = TRUE;
}

/*
 * ddxGiveUp --
 *      Device dependent cleanup. Called by by dix before normal server death.
 *      For SYSV386 we must switch the terminal back to normal mode. No error-
 *      checking here, since there should be restored as much as possible.
 */

void
ddxGiveUp()
{
    int i;

#ifdef XF86PM
    if (xf86OSPMClose)
	xf86OSPMClose();
    xf86OSPMClose = NULL;
#endif

    xf86AccessLeaveState();

    for (i = 0; i < xf86NumScreens; i++) {
	/*
	 * zero all access functions to
	 * trap calls when switched away.
	 */
	xf86Screens[i]->vtSema = FALSE;
	xf86Screens[i]->access = NULL;
	xf86Screens[i]->busAccess = NULL;
    }

#ifdef USE_XF86_SERVERLOCK
    xf86UnlockServer();
#endif
#ifdef XFreeXDGA
    DGAShutdown();
#endif

    xf86CloseConsole();

    xf86CloseLog();

    /* If an unexpected signal was caught, dump a core for debugging */
    if (xf86Info.caughtSignal)
	abort();
}



/*
 * AbortDDX --
 *      DDX - specific abort routine.  Called by AbortServer(). The attempt is
 *      made to restore all original setting of the displays. Also all devices
 *      are closed.
 */

void
AbortDDX()
{
  int i;

  /*
   * try to deinitialize all input devices
   */
  if (xf86Info.kbdProc && xf86Info.pKeyboard)
    (xf86Info.kbdProc)(xf86Info.pKeyboard, DEVICE_CLOSE);

  /*
   * try to restore the original video state
   */
#ifdef HAS_USL_VTS
  /* Need the sleep when starting X from within another X session */
  sleep(1);
#endif
#ifdef DPMSExtension /* Turn screens back on */
  if (DPMSPowerLevel != DPMSModeOn)
      DPMSSet(DPMSModeOn);
#endif
  if (xf86Screens) {
      if (xf86Screens[0]->vtSema)
	  xf86EnterServerState(SETUP);
      for (i = 0; i < xf86NumScreens; i++)
	  if (xf86Screens[i]->vtSema) {
	      /*
	       * if we are aborting before ScreenInit() has finished
	       * we might not have been wrapped yet. Therefore enable
	       * screen explicitely.
	       */
	      xf86EnableAccess(xf86Screens[i]);
	      (xf86Screens[i]->LeaveVT)(i, 0);
	  }
  }
  
  xf86AccessLeave();
  
  /*
   * This is needed for an abnormal server exit, since the normal exit stuff
   * MUST also be performed (i.e. the vt must be left in a defined state)
   */
  ddxGiveUp();
}

void
OsVendorFatalError()
{
#ifdef VENDORSUPPORT
    ErrorF("\nPlease refer to your Operating System Vendor support pages\n"
	   "at %s for support on this crash.\n",VENDORSUPPORT);
#else
    ErrorF("\nPlease consult the "XVENDORNAME" support \n"
	   "\t at "__VENDORDWEBSUPPORT__"\n for help. \n");
#endif
    if (xf86LogFile && xf86LogFileWasOpened)
	ErrorF("Please also check the log file at \"%s\" for additional "
              "information.\n", xf86LogFile);
    ErrorF("\n");
}

int
xf86SetVerbosity(int verb)
{
    int save = xf86Verbose;

    xf86Verbose = verb;
    LogSetParameter(XLOG_VERBOSITY, verb);
    return save;
}

int
xf86SetLogVerbosity(int verb)
{
    int save = xf86LogVerbose;

    xf86LogVerbose = verb;
    LogSetParameter(XLOG_FILE_VERBOSITY, verb);
    return save;
}

/*
 * ddxProcessArgument --
 *	Process device-dependent command line args. Returns 0 if argument is
 *      not device dependent, otherwise Count of number of elements of argv
 *      that are part of a device dependent commandline option.
 *
 */



/* ARGSUSED */
int
ddxProcessArgument(int argc, char **argv, int i)
{
  /*
   * Note: can't use xalloc/xfree here because OsInit() hasn't been called
   * yet.  Use malloc/free instead.
   */

#define CHECK_FOR_REQUIRED_ARGUMENT() \
    if (((i + 1) >= argc) || (!argv[i + 1])) { 				\
      ErrorF("Required argument to %s not specified\n", argv[i]); 	\
      UseMsg(); 							\
      FatalError("Required argument to %s not specified\n", argv[i]);	\
    }
  
  /* First the options that are only allowed for root */
  if (getuid() == 0 || geteuid() != 0)
  {
    if (!strcmp(argv[i], "-modulepath"))
    {
      char *mp;
      CHECK_FOR_REQUIRED_ARGUMENT();
      mp = malloc(strlen(argv[i + 1]) + 1);
      if (!mp)
	FatalError("Can't allocate memory for ModulePath\n");
      strcpy(mp, argv[i + 1]);
      xf86ModulePath = mp;
      xf86ModPathFrom = X_CMDLINE;
      return 2;
    }
    else if (!strcmp(argv[i], "-logfile"))
    {
      char *lf;
      CHECK_FOR_REQUIRED_ARGUMENT();
      lf = malloc(strlen(argv[i + 1]) + 1);
      if (!lf)
	FatalError("Can't allocate memory for LogFile\n");
      strcpy(lf, argv[i + 1]);
      xf86LogFile = lf;
      xf86LogFileFrom = X_CMDLINE;
      return 2;
    }
  } else if (!strcmp(argv[i], "-modulepath") || !strcmp(argv[i], "-logfile")) {
    FatalError("The '%s' option can only be used by root.\n", argv[i]);
  }
  if (!strcmp(argv[i], "-config") || !strcmp(argv[i], "-xf86config"))
  {
    CHECK_FOR_REQUIRED_ARGUMENT();
    if (getuid() != 0 && !xf86PathIsSafe(argv[i + 1])) {
      FatalError("\nInvalid argument for %s\n"
	  "\tFor non-root users, the file specified with %s must be\n"
	  "\ta relative path and must not contain any \"..\" elements.\n"
	  "\tUsing default "__XCONFIGFILE__" search path.\n\n",
	  argv[i], argv[i]);
    }
    xf86ConfigFile = argv[i + 1];
    return 2;
  }
  if (!strcmp(argv[i],"-showunresolved"))
  {
    xf86ShowUnresolved = TRUE;
    return 1;
  }
  if (!strcmp(argv[i],"-probeonly"))
  {
    xf86ProbeOnly = TRUE;
    return 1;
  }
  if (!strcmp(argv[i],"-flipPixels"))
  {
    xf86FlipPixels = TRUE;
    return 1;
  }
#ifdef XF86VIDMODE
  if (!strcmp(argv[i],"-disableVidMode"))
  {
    xf86VidModeDisabled = TRUE;
    return 1;
  }
  if (!strcmp(argv[i],"-allowNonLocalXvidtune"))
  {
    xf86VidModeAllowNonLocal = TRUE;
    return 1;
  }
#endif
#ifdef XF86MISC
  if (!strcmp(argv[i],"-disableModInDev"))
  {
    xf86MiscModInDevDisabled = TRUE;
    return 1;
  }
  if (!strcmp(argv[i],"-allowNonLocalModInDev"))
  {
    xf86MiscModInDevAllowNonLocal = TRUE;
    return 1;
  }
#endif
  if (!strcmp(argv[i],"-allowMouseOpenFail"))
  {
    xf86AllowMouseOpenFail = TRUE;
    return 1;
  }
  if (!strcmp(argv[i],"-bestRefresh"))
  {
    xf86BestRefresh = TRUE;
    return 1;
  }
  if (!strcmp(argv[i],"-ignoreABI"))
  {
    LoaderSetOptions(LDR_OPT_ABI_MISMATCH_NONFATAL);
    return 1;
  }
  if (!strcmp(argv[i],"-verbose"))
  {
    if (++i < argc && argv[i])
    {
      char *end;
      long val;
      val = strtol(argv[i], &end, 0);
      if (*end == '\0')
      {
	xf86SetVerbosity(val);
	return 2;
      }
    }
    xf86SetVerbosity(++xf86Verbose);
    return 1;
  }
  if (!strcmp(argv[i],"-logverbose"))
  {
    if (++i < argc && argv[i])
    {
      char *end;
      long val;
      val = strtol(argv[i], &end, 0);
      if (*end == '\0')
      {
	xf86SetLogVerbosity(val);
	return 2;
      }
    }
    xf86SetLogVerbosity(++xf86LogVerbose);
    return 1;
  }
  if (!strcmp(argv[i],"-quiet"))
  {
    xf86SetVerbosity(0);
    return 1;
  }
  if (!strcmp(argv[i],"-showconfig") || !strcmp(argv[i],"-version"))
  {
    xf86PrintBanner();
    exit(0);
  }
  /* Notice the -fp flag, but allow it to pass to the dix layer */
  if (!strcmp(argv[i], "-fp"))
  {
    xf86fpFlag = TRUE;
    return 0;
  }
  /* Notice the -co flag, but allow it to pass to the dix layer */
  if (!strcmp(argv[i], "-co"))
  {
    xf86coFlag = TRUE;
    return 0;
  }
  /* Notice the -bs flag, but allow it to pass to the dix layer */
  if (!strcmp(argv[i], "-bs"))
  {
    xf86bsDisableFlag = TRUE;
    return 0;
  }
  /* Notice the +bs flag, but allow it to pass to the dix layer */
  if (!strcmp(argv[i], "+bs"))
  {
    xf86bsEnableFlag = TRUE;
    return 0;
  }
  /* Notice the -s flag, but allow it to pass to the dix layer */
  if (!strcmp(argv[i], "-s"))
  {
    xf86sFlag = TRUE;
    return 0;
  }
  if (!strcmp(argv[i], "-bpp"))
  {
    ErrorF("The -bpp option is no longer supported.\n"
	"\tUse -depth to set the color depth, and use -fbbpp if you really\n"
	"\tneed to force a non-default framebuffer (hardware) pixel format.\n");
    if (++i >= argc)
      return 1;
    return 2;
  }
  if (!strcmp(argv[i], "-pixmap24"))
  {
    xf86Pix24 = Pix24Use24;
    return 1;
  }
  if (!strcmp(argv[i], "-pixmap32"))
  {
    xf86Pix24 = Pix24Use32;
    return 1;
  }
  if (!strcmp(argv[i], "-fbbpp"))
  {
    int bpp;
    CHECK_FOR_REQUIRED_ARGUMENT();
    if (sscanf(argv[++i], "%d", &bpp) == 1)
    {
      xf86FbBpp = bpp;
      return 2;
    }
    else
    {
      ErrorF("Invalid fbbpp\n");
      return 0;
    }
  }
  if (!strcmp(argv[i], "-depth"))
  {
    int depth;
    CHECK_FOR_REQUIRED_ARGUMENT();
    if (sscanf(argv[++i], "%d", &depth) == 1)
    {
      xf86Depth = depth;
      return 2;
    }
    else
    {
      ErrorF("Invalid depth\n");
      return 0;
    }
  }
  if (!strcmp(argv[i], "-weight"))
  {
    int red, green, blue;
    CHECK_FOR_REQUIRED_ARGUMENT();
    if (sscanf(argv[++i], "%1d%1d%1d", &red, &green, &blue) == 3)
    {
      xf86Weight.red = red;
      xf86Weight.green = green;
      xf86Weight.blue = blue;
      return 2;
    }
    else
    {
      ErrorF("Invalid weighting\n");
      return 0;
    }
  }
  if (!strcmp(argv[i], "-gamma")  || !strcmp(argv[i], "-rgamma") || 
      !strcmp(argv[i], "-ggamma") || !strcmp(argv[i], "-bgamma"))
  {
    double gamma;
    CHECK_FOR_REQUIRED_ARGUMENT();    
    if (sscanf(argv[++i], "%lf", &gamma) == 1) {
       if (gamma < GAMMA_MIN || gamma > GAMMA_MAX) {
	  ErrorF("gamma out of range, only  %.2f <= gamma_value <= %.1f"
		 " is valid\n", GAMMA_MIN, GAMMA_MAX);
	  return 0;
       }
       if (!strcmp(argv[i-1], "-gamma"))
	  xf86Gamma.red = xf86Gamma.green = xf86Gamma.blue = gamma;
       else if (!strcmp(argv[i-1], "-rgamma")) xf86Gamma.red = gamma;
       else if (!strcmp(argv[i-1], "-ggamma")) xf86Gamma.green = gamma;
       else if (!strcmp(argv[i-1], "-bgamma")) xf86Gamma.blue = gamma;
       return 2;
    }
  }
  if (!strcmp(argv[i], "-layout"))
  {
    CHECK_FOR_REQUIRED_ARGUMENT();
    xf86LayoutName = argv[++i];
    return 2;
  }
  if (!strcmp(argv[i], "-screen"))
  {
    CHECK_FOR_REQUIRED_ARGUMENT();
    xf86ScreenName = argv[++i];
    return 2;
  }
  if (!strcmp(argv[i], "-pointer"))
  {
    CHECK_FOR_REQUIRED_ARGUMENT();
    xf86PointerName = argv[++i];
    return 2;
  }
  if (!strcmp(argv[i], "-keyboard"))
  {
    CHECK_FOR_REQUIRED_ARGUMENT();    
    xf86KeyboardName = argv[++i];
    return 2;
  }
  if (!strcmp(argv[i], "-nosilk"))
  {
    xf86silkenMouseDisableFlag = TRUE;
    return 1;
  }
#ifdef HAVE_ACPI
  if (!strcmp(argv[i], "-noacpi"))
  {
    xf86acpiDisableFlag = TRUE;
    return 1;
  }
#endif
  if (!strcmp(argv[i], "-scanpci"))
  {
    DoScanPci(argc, argv, i);
  }
  if (!strcmp(argv[i], "-probe"))
  {
    xf86DoProbe = TRUE;
    return 1;
  }
  if (!strcmp(argv[i], "-configure"))
  {
    if (getuid() != 0 && geteuid() == 0) {
	ErrorF("The '-configure' option can only be used by root.\n");
	exit(1);
    }
    xf86DoConfigure = TRUE;
    xf86AllowMouseOpenFail = TRUE;
    return 1;
  }
  if (!strcmp(argv[i], "-isolateDevice"))
  {
    int bus, device, func;
    CHECK_FOR_REQUIRED_ARGUMENT();
    if (strncmp(argv[++i], "PCI:", 4)) {
       FatalError("Bus types other than PCI not yet isolable\n");
    }
    if (sscanf(argv[i], "PCI:%d:%d:%d", &bus, &device, &func) == 3) {
       xf86IsolateDevice.bus = bus;
       xf86IsolateDevice.device = device;
       xf86IsolateDevice.func = func;
       return 2;
    } else {
       FatalError("Invalid isolated device specification\n");
    }
  }
  /* OS-specific processing */
  return xf86ProcessArgument(argc, argv, i);
}

/* ddxInitGlobals - called by |InitGlobals| from os/util.c */
void ddxInitGlobals(void)
{
}

/*
 * ddxUseMsg --
 *	Print out correct use of device dependent commandline options.
 *      Maybe the user now knows what really to do ...
 */

void
ddxUseMsg()
{
  ErrorF("\n");
  ErrorF("\n");
  ErrorF("Device Dependent Usage\n");
  if (getuid() == 0 || geteuid() != 0)
  {
    ErrorF("-modulepath paths      specify the module search path\n");
    ErrorF("-logfile file          specify a log file name\n");
    ErrorF("-configure             probe for devices and write an "__XCONFIGFILE__"\n");
  }
  ErrorF("-config file           specify a configuration file, relative to the\n");
  ErrorF("                       "__XCONFIGFILE__" search path, only root can use absolute\n");
  ErrorF("-probeonly             probe for devices, then exit\n");
  ErrorF("-scanpci               execute the scanpci module and exit\n");
  ErrorF("-verbose [n]           verbose startup messages\n");
  ErrorF("-logverbose [n]        verbose log messages\n");
  ErrorF("-quiet                 minimal startup messages\n");
  ErrorF("-pixmap24              use 24bpp pixmaps for depth 24\n");
  ErrorF("-pixmap32              use 32bpp pixmaps for depth 24\n");
  ErrorF("-fbbpp n               set bpp for the framebuffer. Default: 8\n");
  ErrorF("-depth n               set colour depth. Default: 8\n");
  ErrorF("-gamma f               set gamma value (0.1 < f < 10.0) Default: 1.0\n");
  ErrorF("-rgamma f              set gamma value for red phase\n");
  ErrorF("-ggamma f              set gamma value for green phase\n");
  ErrorF("-bgamma f              set gamma value for blue phase\n");
  ErrorF("-weight nnn            set RGB weighting at 16 bpp.  Default: 565\n");
  ErrorF("-layout name           specify the ServerLayout section name\n");
  ErrorF("-screen name           specify the Screen section name\n");
  ErrorF("-keyboard name         specify the core keyboard InputDevice name\n");
  ErrorF("-pointer name          specify the core pointer InputDevice name\n");
  ErrorF("-nosilk                disable Silken Mouse\n");
  ErrorF("-flipPixels            swap default black/white Pixel values\n");
#ifdef XF86VIDMODE
  ErrorF("-disableVidMode        disable mode adjustments with xvidtune\n");
  ErrorF("-allowNonLocalXvidtune allow xvidtune to be run as a non-local client\n");
#endif
#ifdef XF86MISC
  ErrorF("-disableModInDev       disable dynamic modification of input device settings\n");
  ErrorF("-allowNonLocalModInDev allow changes to keyboard and mouse settings\n");
  ErrorF("                       from non-local clients\n");
  ErrorF("-allowMouseOpenFail    start server even if the mouse can't be initialized\n");
#endif
  ErrorF("-bestRefresh           choose modes with the best refresh rate\n");
  ErrorF("-ignoreABI             make module ABI mismatches non-fatal\n");
  ErrorF("-isolateDevice bus_id  restrict device resets to bus_id (PCI only)\n");
  ErrorF("-version               show the server version\n");
  /* OS-specific usage */
  xf86UseMsg();
  ErrorF("\n");
}


#ifndef OSNAME
#define OSNAME " unknown"
#endif
#ifndef OSVENDOR
#define OSVENDOR ""
#endif
#ifndef PRE_RELEASE
#define PRE_RELEASE XORG_VERSION_SNAP
#endif

static void
xf86PrintBanner()
{
#if PRE_RELEASE
  ErrorF("\n"
    "This is a pre-release version of the X server from " XVENDORNAME ".\n"
    "It is not supported in any way.\n"
    "Bugs may be filed in the bugzilla at http://bugs.freedesktop.org/.\n"
    "Select the \"xorg\" product for bugs you find in this release.\n"
    "Before reporting bugs in pre-release versions please check the\n"
    "latest version in the X.Org Foundation git repository.\n"
    "See http://wiki.x.org/wiki/GitPage for git access instructions.\n");
#endif
  ErrorF("\nX Window System Version %d.%d.%d",
	 XORG_VERSION_MAJOR,
	 XORG_VERSION_MINOR,
	 XORG_VERSION_PATCH);
#if XORG_VERSION_SNAP > 0
  ErrorF(".%d", XORG_VERSION_SNAP);
#endif

#if XORG_VERSION_SNAP >= 900
  /* When the minor number is 99, that signifies that the we are making
   * a release candidate for a major version.  (X.0.0)
   * When the patch number is 99, that signifies that the we are making
   * a release candidate for a minor version.  (X.Y.0)
   * When the patch number is < 99, then we are making a release
   * candidate for the next point release.  (X.Y.Z)
   */
#if XORG_VERSION_MINOR >= 99
  ErrorF(" (%d.0.0 RC %d)", XORG_VERSION_MAJOR+1, XORG_VERSION_SNAP - 900);
#elif XORG_VERSION_PATCH == 99
  ErrorF(" (%d.%d.0 RC %d)", XORG_VERSION_MAJOR, XORG_VERSION_MINOR + 1,
				XORG_VERSION_SNAP - 900);
#else
  ErrorF(" (%d.%d.%d RC %d)", XORG_VERSION_MAJOR, XORG_VERSION_MINOR,
 			 XORG_VERSION_PATCH + 1, XORG_VERSION_SNAP - 900);
#endif
#endif

#ifdef XORG_CUSTOM_VERSION
  ErrorF(" (%s)", XORG_CUSTOM_VERSION);
#endif
#ifndef XORG_DATE
#define XORG_DATE XF86_DATE
#endif
  ErrorF("\nRelease Date: %s\n", XORG_DATE);
  ErrorF("X Protocol Version %d, Revision %d, %s\n",
         X_PROTOCOL, X_PROTOCOL_REVISION, XORG_RELEASE );
  ErrorF("Build Operating System: %s %s\n", OSNAME, OSVENDOR);
#ifdef HAS_UTSNAME
  {
    struct utsname name;

    /* Linux & BSD state that 0 is success, SysV (including Solaris, HP-UX,
       and Irix) and Single Unix Spec 3 just say that non-negative is success.
       All agree that failure is represented by a negative number.
     */
    if (uname(&name) >= 0) {
      ErrorF("Current Operating System: %s %s %s %s %s\n",
	name.sysname, name.nodename, name.release, name.version, name.machine);
    }
  }
#endif
#if defined(BUILD_DATE) && (BUILD_DATE > 19000000)
  {
    struct tm t;
    char buf[100];

    bzero(&t, sizeof(t));
    bzero(buf, sizeof(buf));
    t.tm_mday = BUILD_DATE % 100;
    t.tm_mon = (BUILD_DATE / 100) % 100 - 1;
    t.tm_year = BUILD_DATE / 10000 - 1900;
    if (strftime(buf, sizeof(buf), "%d %B %Y", &t))
       ErrorF("Build Date: %s\n", buf);
  }
#endif
#if defined(CLOG_DATE) && (CLOG_DATE > 19000000)
  {
    struct tm t;
    char buf[100];

    bzero(&t, sizeof(t));
    bzero(buf, sizeof(buf));
    t.tm_mday = CLOG_DATE % 100;
    t.tm_mon = (CLOG_DATE / 100) % 100 - 1;
    t.tm_year = CLOG_DATE / 10000 - 1900;
    if (strftime(buf, sizeof(buf), "%d %B %Y", &t))
       ErrorF("Changelog Date: %s\n", buf);
  }
#endif
#if defined(BUILDERSTRING)
  ErrorF("%s \n",BUILDERSTRING);
#endif
  ErrorF("\tBefore reporting problems, check "__VENDORDWEBSUPPORT__"\n"
	 "\tto make sure that you have the latest version.\n");
  ErrorF("Module Loader present\n");
}

static void
xf86PrintMarkers()
{
  LogPrintMarkers();
}

static void
xf86RunVtInit(void)
{
    int i;

    /*
     * If VTInit was set, run that program with consoleFd as stdin and stdout
     */

    if (xf86Info.vtinit) {
      switch(fork()) {
      case -1:
          FatalError("xf86RunVtInit: fork failed (%s)\n", strerror(errno));
          break;
      case 0:  /* child */
	  if (setuid(getuid()) == -1) {
	      xf86Msg(X_ERROR, "xf86RunVtInit: setuid failed (%s)\n",
			 strerror(errno));
	      exit(255);
	  }
          /* set stdin, stdout to the consoleFd */
          for (i = 0; i < 2; i++) {
            if (xf86Info.consoleFd != i) {
              close(i);
              dup(xf86Info.consoleFd);
            }
          }
          execl("/bin/sh", "sh", "-c", xf86Info.vtinit, (void *)NULL);
          xf86Msg(X_WARNING, "exec of /bin/sh failed for VTInit (%s)\n",
                 strerror(errno));
          exit(255);
          break;
      default:  /* parent */
          wait(NULL);
      }
    }
}

/*
 * xf86LoadModules iterates over a list that is being passed in.
 */             
Bool
xf86LoadModules(char **list, pointer *optlist)
{
    int errmaj, errmin;
    pointer opt;
    int i;
    char *name;
    Bool failed = FALSE;

    if (!list)
	return TRUE;

    for (i = 0; list[i] != NULL; i++) {

#ifndef NORMALISE_MODULE_NAME
	name = xstrdup(list[i]);
#else
	/* Normalise the module name */
	name = xf86NormalizeName(list[i]);
#endif

	/* Skip empty names */
	if (name == NULL || *name == '\0')
	    continue;

	if (optlist)
	    opt = optlist[i];
	else
	    opt = NULL;

        if (!LoadModule(name, NULL, NULL, NULL, opt, NULL, &errmaj, &errmin)) {
	    LoaderErrorMsg(NULL, name, errmaj, errmin);
	    failed = TRUE;
	}
	xfree(name);
    }
    return !failed;
}

/* Pixmap format stuff */

_X_EXPORT PixmapFormatPtr
xf86GetPixFormat(ScrnInfoPtr pScrn, int depth)
{
    int i;
    static PixmapFormatRec format;	/* XXX not reentrant */

    /*
     * When the formats[] list initialisation isn't complete, check the
     * depth 24 pixmap config/cmdline options and screen-specified formats.
     */

    if (!formatsDone) {
	if (depth == 24) {
	    Pix24Flags pix24 = Pix24DontCare;

	    format.depth = 24;
	    format.scanlinePad = BITMAP_SCANLINE_PAD;
	    if (xf86Info.pixmap24 != Pix24DontCare)
		pix24 = xf86Info.pixmap24;
	    else if (pScrn->pixmap24 != Pix24DontCare)
		pix24 = pScrn->pixmap24;
	    if (pix24 == Pix24Use24)
		format.bitsPerPixel = 24;
	    else
		format.bitsPerPixel = 32;
	    return &format;
	}
    }
	
    for (i = 0; i < numFormats; i++)
	if (formats[i].depth == depth)
	   break;
    if (i != numFormats)
	return &formats[i];
    else if (!formatsDone) {
	/* Check for screen-specified formats */
	for (i = 0; i < pScrn->numFormats; i++)
	    if (pScrn->formats[i].depth == depth)
		break;
	if (i != pScrn->numFormats)
	    return &pScrn->formats[i];
    }
    return NULL;
}

_X_EXPORT int
xf86GetBppFromDepth(ScrnInfoPtr pScrn, int depth)
{
    PixmapFormatPtr format;

   
    format = xf86GetPixFormat(pScrn, depth);
    if (format)
	return format->bitsPerPixel;
    else
	return 0;
}

