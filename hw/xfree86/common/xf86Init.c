/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Init.c,v 3.198 2003/02/26 09:21:38 dawes Exp $ */

/*
 * Copyright 1991-1999 by The XFree86 Project, Inc.
 *
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 */

#include <stdlib.h>

#define NEED_EVENTS
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "Xatom.h"
#include "input.h"
#include "servermd.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "site.h"
#include "mi.h"

#include "compiler.h"

#ifdef XFree86LOADER
#include "loaderProcs.h"
#endif
#ifdef XFreeXDGA
#include "dgaproc.h"
#endif

#define XF86_OS_PRIVS
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Config.h"
#include "xf86_OSlib.h"
#include "xf86Version.h"
#include "xf86Date.h"
#include "xf86Build.h"
#include "mipointer.h"
#ifdef XINPUT
#include "XI.h"
#include "XIproto.h"
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

#ifdef XTESTEXT1
#include "atKeynames.h"
extern int xtest_command_key;
#endif /* XTESTEXT1 */


/* forward declarations */

static void xf86PrintBanner(void);
static void xf86PrintMarkers(void);
static void xf86RunVtInit(void);

#ifdef DO_CHECK_BETA
static int extraDays = 0;
static char *expKey = NULL;
#endif

#ifdef __UNIXOS2__
extern void os2ServerVideoAccess();
#endif

void (*xf86OSPMClose)(void) = NULL;

#ifdef XFree86LOADER
static char *baseModules[] = {
	"bitmap",
	"pcidata",
	NULL
};
#endif

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

InputDriverRec xf86KEYBOARD = {
	1,
	"keyboard",
	NULL,
	NULL,
	NULL,
	NULL,
	0
};

static Bool
xf86CreateRootWindow(WindowPtr pWin)
{
  int ret = TRUE;
  int err = Success;
  ScreenPtr pScreen = pWin->drawable.pScreen;
  PropertyPtr pRegProp, pOldRegProp;
  CreateWindowProcPtr CreateWindow =
    (CreateWindowProcPtr)(pScreen->devPrivates[xf86CreateRootWindowIndex].ptr);

#ifdef DEBUG
  ErrorF("xf86CreateRootWindow(%p)\n", pWin);
#endif

  if ( pScreen->CreateWindow != xf86CreateRootWindow ) {
    /* Can't find hook we are hung on */
	xf86DrvMsg(pScreen->myNum, X_WARNING /* X_ERROR */,
		  "xf86CreateRootWindow %p called when not in pScreen->CreateWindow %p n",
		   xf86CreateRootWindow, pScreen->CreateWindow );
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
      for (pRegProp = xf86RegisteredPropertiesTable[pScreen->myNum];
	   pRegProp != NULL && err==Success;
	   pRegProp = pRegProp->next )
	{
	  Atom oldNameAtom = pRegProp->propertyName;
	  char *nameString;
	  /* propertyName was created before the screen existed,
	   * so the atom does not belong to any screen;
	   * we need to create a new atom with the same name.
	   */
	  nameString = NameForAtom(oldNameAtom);
	  pRegProp->propertyName = MakeAtom(nameString, strlen(nameString), TRUE);
	  err = ChangeWindowProperty(pWin,
				     pRegProp->propertyName, pRegProp->type,
				     pRegProp->format, PropModeReplace,
				     pRegProp->size, pRegProp->data,
				     FALSE
				     );
	}
      
      /* Look at err */
      ret &= (err==Success);
      
      /* free memory */
      pOldRegProp = xf86RegisteredPropertiesTable[pScreen->myNum];
      while (pOldRegProp!=NULL) { 	
	pRegProp = pOldRegProp->next;
	xfree(pOldRegProp);
	pOldRegProp = pRegProp;
      }  
      xf86RegisteredPropertiesTable[pScreen->myNum] = NULL;
    } else {
      xf86Msg(X_ERROR, "xf86CreateRootWindow unexpectedly called with "
	      "non-root window %p (parent %p)\n", pWin, pWin->parent);
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

void
InitOutput(ScreenInfo *pScreenInfo, int argc, char **argv)
{
  int                    i, j, k, scr_index;
  static unsigned long   generation = 0;
#ifdef XFree86LOADER
  char                   **modulelist;
  pointer                *optionlist;
#endif
  screenLayoutPtr	 layout;
  Pix24Flags		 screenpix24, pix24;
  MessageType		 pix24From = X_DEFAULT;
  Bool			 pix24Fail = FALSE;
  
#ifdef __UNIXOS2__
  os2ServerVideoAccess();  /* See if we have access to the screen before doing anything */
#endif

  xf86Initialising = TRUE;

  /* Do this early? */
  if (generation != serverGeneration) {
      xf86ScreenIndex = AllocateScreenPrivateIndex();
      xf86CreateRootWindowIndex = AllocateScreenPrivateIndex();
      xf86PixmapIndex = AllocatePixmapPrivateIndex();
      xf86RegisteredPropertiesTable=NULL;
      generation = serverGeneration;
  }

  if (serverGeneration == 1) {

    pScreenInfo->numScreens = 0;

    if ((xf86ServerName = strrchr(argv[0], '/')) != 0)
      xf86ServerName++;
    else
      xf86ServerName = argv[0];

#ifdef DO_CHECK_BETA
    xf86CheckBeta(extraDays, expKey);
#endif

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
      if (!xf86HandleConfigFile()) {
	xf86Msg(X_ERROR, "Error from xf86HandleConfigFile()\n");
	return;
      }
    }

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

    xf86OpenConsole();
    xf86OSPMClose = xf86OSPMOpen();
    
    /* Run an external VT Init program if specified in the config file */
    xf86RunVtInit();

    /* Do this after XF86Config is read (it's normally in OsInit()) */
    OsInitColors();

    /* Enable full I/O access */
    xf86EnableIO();

#ifdef XFree86LOADER
    /* Initialise the loader */
    LoaderInit();

    /* Tell the loader the default module search path */
    LoaderSetPath(xf86ModulePath);

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
    
#endif

    /* Do a general bus probe.  This will be a PCI probe for x86 platforms */
    xf86BusProbe();

    if (xf86DoProbe)
	DoProbe();

    if (xf86DoConfigure)
	DoConfigure();

    /* Initialise the resource broker */
    xf86ResourceBrokerInit();

#ifdef XFree86LOADER
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

    /* Setup the builtin input drivers */
    xf86AddInputDriver(&xf86KEYBOARD, NULL, 0);
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
#endif

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
     * Call each of the Identify functions.  The Identify functions print
     * out some identifying information, and anything else that might be
     * needed at this early stage.
     */

    for (i = 0; i < xf86NumDrivers; i++)
      /* The Identify function is mandatory, but if it isn't there continue */
      if (xf86DriverList[i]->Identify != NULL)
	xf86DriverList[i]->Identify(0);
      else {
        xf86Msg(X_WARNING, "Driver `%s' has no Identify function\n",
	       xf86DriverList[i]->driverName ? xf86DriverList[i]->driverName
					     : "noname");
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
      if (xf86DriverList[i]->Probe != NULL)
	xf86DriverList[i]->Probe(xf86DriverList[i], PROBE_DEFAULT);
      else {
        xf86MsgVerb(X_WARNING, 0,
		"Driver `%s' has no Probe function (ignoring)\n",
		xf86DriverList[i]->driverName ? xf86DriverList[i]->driverName
					     : "noname");
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
      OsCleanup();
      AbortDDX();
      fflush(stderr);
      exit(0);
    }

#ifdef XFree86LOADER
    /* Remove (unload) drivers that are not required */
    for (i = 0; i < xf86NumDrivers; i++)
	if (xf86DriverList[i] && xf86DriverList[i]->refCount <= 0)
	    xf86DeleteDriver(i);
#endif

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

    /*
      should we reopen it here? We need to deal with an already opened
      device. We could leave this to the OS layer. For now we simply
      close it here
    */
    if (xf86OSPMClose)
        xf86OSPMClose();
    if ((xf86OSPMClose = xf86OSPMOpen()) != NULL)
	xf86MsgVerb(3,X_INFO,"APM registered successfully\n");

    /* Make sure full I/O access is enabled */
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
#ifdef SCO
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
#endif /* SCO */

  for (i = 0; i < xf86NumScreens; i++) {    
   	xf86EnableAccess(xf86Screens[i]);
	/*
	 * Almost everything uses these defaults, and many of those that
	 * don't, will wrap them.
	 */
	xf86Screens[i]->EnableDisableFBAccess = xf86EnableDisableFBAccess;
	xf86Screens[i]->SetDGAMode = xf86SetDGAMode;
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

#ifdef XFree86LOADER
    if ((serverGeneration == 1) && LoaderCheckUnresolved(LD_RESOLV_IFDONE)) {
	/* For now, just a warning */
	xf86Msg(X_WARNING, "Some symbols could not be resolved!\n");
    }
#endif

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
#ifdef XTESTEXT1
    xtest_command_key = KEY_Begin + MIN_KEYCODE;
#endif /* XTESTEXT1 */

    if (serverGeneration == 1) {
	/* Call the PreInit function for each input device instance. */
	for (pDev = xf86ConfigLayout.inputs; pDev && pDev->identifier; pDev++) {
	    /* XXX The keyboard driver is a special case for now. */
	    if (!xf86NameCmp(pDev->driver, "keyboard")) {
		xf86Msg(X_INFO, "Keyboard \"%s\" handled by legacy driver\n",
			pDev->identifier);
		continue;
	    }
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
      xf86Info.pKeyboard = AddInputDevice(xf86Info.kbdProc, TRUE);
    }
    if (corePointer)
	xf86Info.pMouse = corePointer->dev;
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

  /* xf86WrapperInit() is called directly from OsInit() */
#ifdef SIGCHLD
  signal(SIGCHLD, SIG_DFL);	/* Need to wait for child processes */
#endif
  OsDelayInitColors = TRUE;
#ifdef XFree86LOADER
  loadableFonts = TRUE;
#endif

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

    if (xf86OSPMClose)
	xf86OSPMClose();
    xf86OSPMClose = NULL;

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
  if (xf86Info.pKeyboard)
    (xf86Info.kbdProc)(xf86Info.pKeyboard, DEVICE_CLOSE);

  /*
   * try to restore the original video state
   */
#ifdef HAS_USL_VTS
  /* Need the sleep when starting X from within another X session */
  sleep(1);
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
  ErrorF("\nWhen reporting a problem related to a server crash, please send\n"
	 "the full server output, not just the last messages.\n");
  if (xf86LogFile && xf86LogFileWasOpened)
    ErrorF("This can be found in the log file \"%s\".\n", xf86LogFile);
  ErrorF("Please report problems to %s.\n", BUILDERADDR);
  ErrorF("\n");
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

#ifdef DDXOSVERRORF
  static Bool beenHere = FALSE;

  if (!beenHere) {
    /*
     * This initialises our hook into VErrorF() for catching log messages
     * that are generated before OsInit() is called.
     */
    OsVendorVErrorFProc = OsVendorVErrorF;
    beenHere = TRUE;
  }
#endif

  /* First the options that are only allowed for root */
  if (getuid() == 0)
  {
    if (!strcmp(argv[i], "-modulepath"))
    {
      char *mp;
      if (!argv[i + 1])
	return 0;
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
      if (!argv[i + 1])
	return 0;
      lf = malloc(strlen(argv[i + 1]) + 1);
      if (!lf)
	FatalError("Can't allocate memory for LogFile\n");
      strcpy(lf, argv[i + 1]);
      xf86LogFile = lf;
      xf86LogFileFrom = X_CMDLINE;
      return 2;
    }
  }
  if (!strcmp(argv[i], "-xf86config"))
  {
    if (!argv[i + 1])
      return 0;
    if (getuid() != 0 && !xf86PathIsSafe(argv[i + 1])) {
      FatalError("\nInvalid argument for -xf86config\n"
	  "\tFor non-root users, the file specified with -xf86config must be\n"
	  "\ta relative path and must not contain any \"..\" elements.\n"
	  "\tUsing default XF86Config search path.\n\n");
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
#ifdef XFree86LOADER
    LoaderSetOptions(LDR_OPT_ABI_MISMATCH_NONFATAL);
#endif
    return 1;
  }
#ifdef DO_CHECK_BETA
  if (!strcmp(argv[i],"-extendExpiry"))
  {
    extraDays = atoi(argv[i + 1]);
    expKey = argv[i + 2];
    return 3;
  }
#endif
  if (!strcmp(argv[i],"-verbose"))
  {
    if (++i < argc && argv[i])
    {
      char *end;
      long val;
      val = strtol(argv[i], &end, 0);
      if (*end == '\0')
      {
	xf86Verbose = val;
	return 2;
      }
    }
    xf86Verbose++;
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
	xf86LogVerbose = val;
	return 2;
      }
    }
    xf86LogVerbose++;
    return 1;
  }
  if (!strcmp(argv[i],"-quiet"))
  {
    xf86Verbose = 0;
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
    if (++i >= argc)
      return 0;
    ErrorF("The -bpp option is no longer supported.\n"
	"\tUse -depth to set the color depth, and use -fbbpp if you really\n"
	"\tneed to force a non-default framebuffer (hardware) pixel format.\n");
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
    if (++i >= argc)
      return 0;
    if (sscanf(argv[i], "%d", &bpp) == 1)
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
    if (++i >= argc)
      return 0;
    if (sscanf(argv[i], "%d", &depth) == 1)
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
    if (++i >= argc)
      return 0;
    if (sscanf(argv[i], "%1d%1d%1d", &red, &green, &blue) == 3)
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
    if (++i >= argc)
      return 0;
    if (sscanf(argv[i], "%lf", &gamma) == 1) {
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
    if (++i >= argc)
      return 0;
    xf86LayoutName = argv[i];
    return 2;
  }
  if (!strcmp(argv[i], "-screen"))
  {
    if (++i >= argc)
      return 0;
    xf86ScreenName = argv[i];
    return 2;
  }
  if (!strcmp(argv[i], "-pointer"))
  {
    if (++i >= argc)
      return 0;
    xf86PointerName = argv[i];
    return 2;
  }
  if (!strcmp(argv[i], "-keyboard"))
  {
    if (++i >= argc)
      return 0;
    xf86KeyboardName = argv[i];
    return 2;
  }
  if (!strcmp(argv[i], "-nosilk"))
  {
    xf86silkenMouseDisableFlag = TRUE;
    return 1;
  }
  if (!strcmp(argv[i], "-scanpci"))
  {
    DoScanPci(argc, argv, i);
  }
  if (!strcmp(argv[i], "-probe"))
  {
    xf86DoProbe = TRUE;
#if 0
    DoProbe(argc, argv, i);
#endif
    return 1;
  }
  if (!strcmp(argv[i], "-configure"))
  {
    if (getuid() != 0) {
	ErrorF("The '-configure' option can only be used by root.\n");
	exit(1);
    }
    xf86DoConfigure = TRUE;
    xf86AllowMouseOpenFail = TRUE;
    return 1;
  }
  /* OS-specific processing */
  return xf86ProcessArgument(argc, argv, i);
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
  if (getuid() == 0)
  {
    ErrorF("-xf86config file       specify a configuration file\n");
    ErrorF("-modulepath paths      specify the module search path\n");
    ErrorF("-logfile file          specify a log file name\n");
    ErrorF("-configure             probe for devices and write an XF86Config\n");
  }
  else
  {
    ErrorF("-xf86config file       specify a configuration file, relative to the\n");
    ErrorF("                       XF86Config search path, only root can use absolute\n");
  }
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
#define PRE_RELEASE XF86_VERSION_SNAP
#endif

static void
xf86PrintBanner()
{
#if PRE_RELEASE
  ErrorF("\n"
    "This is a pre-release version of XFree86, and is not supported in any\n"
    "way.  Bugs may be reported to XFree86@XFree86.Org and patches submitted\n"
    "to fixes@XFree86.Org.  Before reporting bugs in pre-release versions,\n"
    "please check the latest version in the XFree86 CVS repository\n"
    "(http://www.XFree86.Org/cvs).\n");
#endif
  ErrorF("\nXFree86 Version %d.%d.%d", XF86_VERSION_MAJOR, XF86_VERSION_MINOR,
					XF86_VERSION_PATCH);
#if XF86_VERSION_SNAP > 0
  ErrorF(".%d", XF86_VERSION_SNAP);
#endif

#if XF86_VERSION_SNAP >= 900
  ErrorF(" (%d.%d.0 RC %d)", XF86_VERSION_MAJOR, XF86_VERSION_MINOR + 1,
				XF86_VERSION_SNAP - 900);
#endif

#ifdef XF86_CUSTOM_VERSION
  ErrorF(" (%s)", XF86_CUSTOM_VERSION);
#endif
  ErrorF("\nRelease Date: %s\n", XF86_DATE);
  ErrorF("X Protocol Version %d, Revision %d, %s\n",
         X_PROTOCOL, X_PROTOCOL_REVISION, XORG_RELEASE );
  ErrorF("Build Operating System:%s%s\n", OSNAME, OSVENDOR);
#ifdef BUILD_DATE
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
#if defined(BUILDERSTRING)
  ErrorF("%s \n",BUILDERSTRING);
#endif
  ErrorF("\tBefore reporting problems, check http://www.XFree86.Org/\n"
	 "\tto make sure that you have the latest version.\n");
#ifdef XFree86LOADER
  ErrorF("Module Loader present\n");
#endif
}

static void
xf86PrintMarkers()
{
    /* Show what the marker symbols mean */
  ErrorF("Markers: " X_PROBE_STRING " probed, "
		     X_CONFIG_STRING " from config file, "
		     X_DEFAULT_STRING " default setting,\n"
	 "         " X_CMDLINE_STRING " from command line, "
		     X_NOTICE_STRING " notice, "
		     X_INFO_STRING " informational,\n"
	 "         " X_WARNING_STRING " warning, "
		     X_ERROR_STRING " error, "
		     X_NOT_IMPLEMENTED_STRING " not implemented, "
		     X_UNKNOWN_STRING " unknown.\n");
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
          setuid(getuid());
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

#ifdef XFree86LOADER
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

#endif

/* Pixmap format stuff */

PixmapFormatPtr
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

int
xf86GetBppFromDepth(ScrnInfoPtr pScrn, int depth)
{
    PixmapFormatPtr format;

   
    format = xf86GetPixFormat(pScrn, depth);
    if (format)
	return format->bitsPerPixel;
    else
	return 0;
}

