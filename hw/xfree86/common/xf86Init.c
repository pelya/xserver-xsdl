/*
 * $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Init.c,v 3.66.2.2 1997/06/29 08:43:36 dawes Exp $
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THOMAS ROELL BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/* $Xorg: xf86Init.c,v 1.3 2000/08/17 19:50:29 cpqbld Exp $ */

#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern int atoi();
#endif

#define NEED_EVENTS
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "input.h"
#include "servermd.h"
#include "scrnintstr.h"
#include "site.h"

#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"
#include "xf86Version.h"
#include "mipointer.h"

#ifdef XINPUT
#include "XI.h"
#include "XIproto.h"
#include "xf86_Config.h"
#include "xf86Xinput.h"
#else
#include "inputstr.h"
#endif

#include "opaque.h"

#ifdef XTESTEXT1
#include "atKeynames.h"
extern int xtest_command_key;
#endif /* XTESTEXT1 */

#ifdef __EMX__
#define seteuid(x) /*nothing*/
#define setruid(x) /*nothing*/
#endif

/* xf86Exiting is set while the screen is shutting down (even on a reset) */
Bool xf86Exiting = FALSE;
Bool xf86Resetting = FALSE;
Bool xf86ProbeFailed = TRUE;
Bool xf86FlipPixels = FALSE;
#ifdef XF86VIDMODE
Bool xf86VidModeEnabled = TRUE;
Bool xf86VidModeAllowNonLocal = FALSE;
#endif
#ifdef XF86MISC
Bool xf86MiscModInDevEnabled = TRUE;
Bool xf86MiscModInDevAllowNonLocal = FALSE;
#endif
Bool xf86AllowMouseOpenFail = FALSE;
PciProbeType xf86PCIFlags = PCIProbe1;
Bool xf86ScreensOpen = FALSE;
int xf86Verbose = 1;
Bool xf86fpFlag = FALSE;
Bool xf86coFlag = FALSE;
Bool xf86sFlag = FALSE;
Bool xf86ProbeOnly = FALSE;
char xf86ConfigFile[PATH_MAX] = "";
int  xf86bpp = -1;
xrgb xf86weight = { 0, 0, 0 } ;	/* RGB weighting at 16 bpp */
double xf86rGamma=1.0, xf86gGamma=1.0, xf86bGamma=1.0;
unsigned char xf86rGammaMap[256], xf86gGammaMap[256], xf86bGammaMap[256];
char *xf86ServerName = NULL;
Bool xf86BestRefresh = FALSE;

int   vgaIOBase = 0x3d0;
int   vgaCRIndex = 0x3d4;
int   vgaCRReg = 0x3d5;

static void xf86PrintBanner(
#if NeedFunctionPrototypes
	void
#endif
	);
static void xf86PrintConfig(
#if NeedFunctionPrototypes
	void
#endif
	);
#ifdef DO_CHECK_BETA
void xf86CheckBeta(
#if NeedFunctionPrototypes
	int,
	char *
#endif
	);
static int extraDays = 0;
static char *expKey = NULL;
#endif

extern ScrnInfoPtr xf86Screens[];
extern int xf86MaxScreens;
extern double pow();
#ifdef USE_XF86_SERVERLOCK
extern void xf86UnlockServer();
#endif
#ifdef __EMX__
extern void os2ServerVideoAccess();
#endif

xf86InfoRec xf86Info;
int         xf86ScreenIndex;

/*
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *      That includes vt-manager setup, querying all possible devices and
 *      collecting the pixmap formats.
 */

void
InitOutput(pScreenInfo, argc, argv)
     ScreenInfo	*pScreenInfo;
     int     	argc;
     char    	**argv;
{
  int                    i, j, scr_index;
  static int             numFormats = 0;
  static PixmapFormatRec formats[MAXFORMATS];
  static unsigned long   generation = 0;
  int                    any_screens = 0;
   
#ifdef __EMX__
  os2ServerVideoAccess();  /* See if we have access to the screen before doing anything */
#endif

  if (serverGeneration == 1) {

    if ((xf86ServerName = strrchr(argv[0], '/')) != 0)
      xf86ServerName++;
    else
      xf86ServerName = argv[0];

#ifdef DO_CHECK_BETA
    xf86CheckBeta(extraDays, expKey);
#endif

    xf86PrintBanner();

    xf86PrintConfig();

    xf86OpenConsole();

#if !defined(AMOEBA) && !defined(MINIX)
    /*
     * If VTInit was set, run that program with consoleFd as stdin and stdout
     */

    if (xf86Info.vtinit) {
      switch(fork()) {
        case -1:
          FatalError("Fork failed for VTInit (%s)\n", strerror(errno));
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
          execl("/bin/sh", "sh", "-c", xf86Info.vtinit, NULL);
          ErrorF("Warning: exec of /bin/sh failed for VTInit (%s)\n",
                 strerror(errno));
          exit(255);
          break;
        default:  /* parent */
          wait(NULL);
      }
    }
#endif /* !AMOEBA && !MINIX */

    /* Do this after XF86Config is read (it's normally in OsInit()) */
    OsInitColors();

    for (i=0; i<256; i++) {
       xf86rGammaMap[i] = (int)(pow(i/255.0,xf86rGamma)*255.0+0.5);
       xf86gGammaMap[i] = (int)(pow(i/255.0,xf86gGamma)*255.0+0.5);
       xf86bGammaMap[i] = (int)(pow(i/255.0,xf86bGamma)*255.0+0.5);
    }

    xf86Config(TRUE); /* Probe displays, and resolve modes */

#ifdef XKB
    xf86InitXkb();
#endif

    /*
     * collect all possible formats
     */
    formats[0].depth = 1;
    formats[0].bitsPerPixel = 1;
    formats[0].scanlinePad = BITMAP_SCANLINE_PAD;
    numFormats++;
  
    for ( i=0;
          i < xf86MaxScreens && xf86Screens[i] && xf86Screens[i]->configured;
          i++ )
      { 
	/*
	 * At least one probe function succeeded.
	 */
	any_screens = 1;

	/*
	 * add new pixmap format
	 */
	for ( j=0; j < numFormats; j++ ) {
	  
	  if (formats[j].depth == xf86Screens[i]->depth &&
	      formats[j].bitsPerPixel == xf86Screens[i]->bitsPerPixel)
	    break; /* found */
        }
	  
        if (j == numFormats) {   /* not already there */
	  formats[j].depth = xf86Screens[i]->depth;
	  formats[j].bitsPerPixel = xf86Screens[i]->bitsPerPixel;
	  formats[j].scanlinePad = BITMAP_SCANLINE_PAD;
	  numFormats++;
	  if ( numFormats > MAXFORMATS )
	    FatalError( "Too many pixmap formats! Exiting\n" );
        }
      }
    if (!any_screens)
      if (xf86ProbeFailed)
        ErrorF("\n *** None of the configured devices were detected.***\n\n");
      else
        ErrorF(
         "\n *** A configured device found, but display modes could not be resolved.***\n\n");
    if (xf86ProbeOnly)
    {
      xf86VTSema = FALSE;
      AbortDDX();
      fflush(stderr);
      exit(0);
    }
  }
  else {
    /*
     * serverGeneration != 1; some OSs have to do things here, too.
     */
    xf86OpenConsole();
  }

  /*
   * Install signal handler for unexpected signals
   */
  if (!xf86Info.notrapSignals)
  {
     xf86Info.caughtSignal=FALSE;
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


  /*
   * Use the previous collected parts to setup pScreenInfo
   */
  pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
  pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
  pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
  pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
  pScreenInfo->numPixmapFormats = numFormats;
  for ( i=0; i < numFormats; i++ ) pScreenInfo->formats[i] = formats[i];

  if (generation != serverGeneration)
    {
      xf86ScreenIndex = AllocateScreenPrivateIndex();
      generation = serverGeneration;
    }


  for ( i=0;
        i < xf86MaxScreens && xf86Screens[i] && xf86Screens[i]->configured;
        i++ )
    {    
      /*
       * On a server-reset, we have explicitely to remap all stuff ...
       * (At startuptime this is implicitely done by probing the device
       */
      if (serverGeneration != 1)
        {
          xf86Resetting = TRUE;
          xf86Exiting = FALSE;
#ifdef HAS_USL_VTS
          if (!xf86VTSema)
            ioctl(xf86Info.consoleFd,VT_RELDISP,VT_ACKACQ);
#endif
          xf86VTSema = TRUE;
          (xf86Screens[i]->EnterLeaveVT)(ENTER, i);
          xf86Resetting = FALSE;
        }
#ifdef SCO
        else {
          /*
           * Under SCO we must ack that we got the console at startup,
           * I think this is the safest way to assure it
           */
          static int once = 1;
          if (once) {
            once = 0;
            if (ioctl(xf86Info.consoleFd, VT_RELDISP, VT_ACKACQ) < 0)
              ErrorF("VT_ACKACQ failed");
          }
        }
#endif /* SCO */
      scr_index = AddScreen(xf86Screens[i]->Init, argc, argv);
      if (scr_index > -1)
      {
	screenInfo.screens[scr_index]->devPrivates[xf86ScreenIndex].ptr
	  = (pointer)xf86Screens[i];
      }

      /*
       * Here we have to let the driver getting access of the VT. Note that
       * this doesn't mean that the graphics board may access automatically
       * the monitor. If the monitor is shared this is done in xf86CrossScreen!
       */
      if (!xf86Info.sharedMonitor) (xf86Screens[i]->EnterLeaveMonitor)(ENTER);
    }

#ifndef AMOEBA
  RegisterBlockAndWakeupHandlers(xf86Block, xf86Wakeup, (void *)0);
#endif
}


/*
 * InitInput --
 *      Initialize all supported input devices...what else is there
 *      besides pointer and keyboard? Two DeviceRec's are allocated and
 *      registered as the system pointer and keyboard devices.
 */

void
InitInput(argc, argv)
     int     	  argc;
     char    	  **argv;
{
  xf86Info.vtRequestsPending = FALSE;
  xf86Info.inputPending = FALSE;
#ifdef XTESTEXT1
  xtest_command_key = KEY_Begin + MIN_KEYCODE;
#endif /* XTESTEXT1 */

  xf86Info.pKeyboard = AddInputDevice(xf86Info.kbdProc, TRUE); 
  xf86Info.pMouse =  AddInputDevice(xf86Info.mouseDev->mseProc, TRUE);
  RegisterKeyboardDevice((DevicePtr)xf86Info.pKeyboard); 
  RegisterPointerDevice((DevicePtr)xf86Info.pMouse); 

#ifdef XINPUT
  (xf86Info.pMouse)->public.devicePrivate = xf86Info.mouseLocal;
#if 0 /* Is this needed?? */
  ((LocalDevicePtr) xf86Info.mouseLocal)->dev = xf86Info.pMouse;
#endif
#else
  (xf86Info.pMouse)->public.devicePrivate = (pointer) xf86Info.mouseDev;
#endif
  
#ifdef XINPUT
  InitExtInput();
#endif

  miRegisterPointerDevice(screenInfo.screens[0], (DevicePtr) xf86Info.pMouse);
#ifdef XINPUT
  xf86XinputFinalizeInit(xf86Info.pMouse);
  xf86eqInit ((DevicePtr)xf86Info.pKeyboard, (DevicePtr)xf86Info.pMouse);
#else
  mieqInit (xf86Info.pKeyboard, xf86Info.pMouse);
#endif
}

/*
 * OsVendorInit --
 *      OS/Vendor-specific initialisations.  Called from OsInit(), which
 *      is called by dix before establishing the well known sockets.
 */
 
extern Bool OsDelayInitColors;

void
OsVendorInit()
{
#ifdef SIGCHLD
  signal(SIGCHLD, SIG_DFL);	/* Need to wait for child processes */
#endif
#ifdef USE_XF86_SERVERLOCK
  extern void xf86LockServer();
  static Bool been_here = FALSE;

  if (!been_here) {
    xf86LockServer();
    been_here = TRUE;
  }
#endif
  OsDelayInitColors = TRUE;
}

#ifdef DPMSExtension
extern CARD16 DPMSPowerLevel;

/*
 * DPMSSet --
 *	Device dependent DPMS mode setting hook.  This is called whenever
 *	the DPMS mode is to be changed.
 */
void
DPMSSet(CARD16 level)
{
    int i;

    /* For each screen, set the power saver level */
    for (i = 0; i < screenInfo.numScreens; i++) {
	(XF86SCRNINFO(screenInfo.screens[i])->DPMSSet)(level);
    }

    DPMSPowerLevel = level;
}

#if 0
/*
 * DPMSGet --
 *	Device dependent DPMS mode getting hook.  This returns the current
 *	DPMS mode, or -1 if DPMS is not supported.
 *
 *	This should hook in to the appropriate driver-level function, which
 *	will be added to the ScrnInfoRec.
 *
 *	NOTES:
 *	 1. the calling interface should be changed to specify which
 *	    screen to check.
 *	 2. It isn't clear that this function is ever used.
 */
CARD16
DPMSGet(CARD16 *level)
{
    int i;

    /* For each screen, set the power saver level */
    for (i = 0; i < screenInfo.numScreens; i++) {
	 ;
    }
}
#endif

/*
 * DPMSSupported --
 *	Return TRUE if any screen supports DPMS.
 */
Bool
DPMSSupported(void)
{
    int i;

    /* For each screen, check if DPMS is supported */
    for (i = 0; i < screenInfo.numScreens; i++) {
	if (XF86SCRNINFO(screenInfo.screens[i])->DPMSSet != (void (*)())NoopDDA)
	    return TRUE;
    }
    return FALSE;
}
#endif /* DPMSExtension */

/*
 * ddxGiveUp --
 *      Device dependent cleanup. Called by by dix before normal server death.
 *      For SYSV386 we must switch the terminal back to normal mode. No error-
 *      checking here, since there should be restored as much as possible.
 */

void
ddxGiveUp()
{
#ifdef USE_XF86_SERVERLOCK
  xf86UnlockServer();
#endif

  xf86CloseConsole();

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

#if 0
  if (xf86Exiting)
    return;
#endif

  xf86Exiting = TRUE;

  /*
   * try to deinitialize all input devices
   */
  if (xf86Info.pMouse) (xf86Info.mouseDev->mseProc)(xf86Info.pMouse, DEVICE_CLOSE);
  if (xf86Info.pKeyboard) (xf86Info.kbdProc)(xf86Info.pKeyboard, DEVICE_CLOSE);

  /*
   * try to restore the original video state
   */
#ifdef HAS_USL_VTS
  /* Need the sleep when starting X from within another X session */
  sleep(1);
#endif
  if (xf86VTSema && xf86ScreensOpen)
    for ( i=0;
          i < xf86MaxScreens && xf86Screens[i] && xf86Screens[i]->configured;
          i++ )
      (xf86Screens[i]->EnterLeaveVT)(LEAVE, i);

  /*
   * This is needed for a abnormal server exit, since the normal exit stuff
   * MUST also be performed (i.e. the vt must be left in a defined state)
   */
  ddxGiveUp();
}

void
OsVendorFatalError()
{
  ErrorF("\nWhen reporting a problem related to a server crash, please send\n"
	 "the full server output, not just the last messages\n\n");
}

/*
 * ddxProcessArgument --
 *	Process device-dependent command line args. Returns 0 if argument is
 *      not device dependent, otherwise Count of number of elements of argv
 *      that are part of a device dependent commandline option.
 */

/* ARGSUSED */
int
ddxProcessArgument (argc, argv, i)
     int argc;
     char *argv[];
     int i;
{
  if (getuid() == 0 && !strcmp(argv[i], "-xf86config"))
  {
    if (!argv[i+1])
      return 0;
    if (strlen(argv[i+1]) >= PATH_MAX)
      FatalError("XF86Config path name too long\n");
    strcpy(xf86ConfigFile, argv[i+1]);
    return 2;
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
    xf86VidModeEnabled = FALSE;
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
    xf86MiscModInDevEnabled = FALSE;
    return 1;
  }
  if (!strcmp(argv[i],"-allowNonLocalModInDev"))
  {
    xf86MiscModInDevAllowNonLocal = TRUE;
    return 1;
  }
  if (!strcmp(argv[i],"-allowMouseOpenFail"))
  {
    xf86AllowMouseOpenFail = TRUE;
    return 1;
  }
#endif
  if (!strcmp(argv[i],"-bestRefresh"))
  {
    xf86BestRefresh = TRUE;
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
    if (!xf86Verbose++)
      xf86Verbose = 2;
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
    xf86PrintConfig();
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
  /* Notice the -s flag, but allow it to pass to the dix layer */
  if (!strcmp(argv[i], "-s"))
  {
    xf86sFlag = TRUE;
    return 0;
  }
#ifndef XF86MONOVGA
  if (!strcmp(argv[i], "-bpp"))
  {
    int bpp;
    if (++i >= argc)
      return 0;
    if (sscanf(argv[i], "%d", &bpp) == 1)
    {
      xf86bpp = bpp;
      return 2;
    }
    else
    {
      ErrorF("Invalid bpp\n");
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
      xf86weight.red = red;
      xf86weight.green = green;
      xf86weight.blue = blue;
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
       if (gamma < 0.1 || gamma > 10) {
	  ErrorF("gamma out of range, only  0.1 < gamma_value < 10  is valid\n");
	  return 0;
       }
       if (!strcmp(argv[i-1], "-gamma")) 
	  xf86rGamma = xf86gGamma = xf86bGamma = 1.0 / gamma;
       else if (!strcmp(argv[i-1], "-rgamma")) xf86rGamma = 1.0 / gamma;
       else if (!strcmp(argv[i-1], "-ggamma")) xf86gGamma = 1.0 / gamma;
       else if (!strcmp(argv[i-1], "-bgamma")) xf86bGamma = 1.0 / gamma;
       return 2;
    }
  }
#endif /* XF86MONOVGA */
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
    ErrorF("-xf86config file       specify a configuration file\n");
  ErrorF("-probeonly             probe for devices, then exit\n");
  ErrorF("-verbose               verbose startup messages\n");
  ErrorF("-quiet                 minimal startup messages\n");
#ifndef XF86MONOVGA
  ErrorF("-bpp n                 set number of bits per pixel. Default: 8\n");
  ErrorF("-gamma f               set gamma value (0.1 < f < 10.0) Default: 1.0\n");
  ErrorF("-rgamma f              set gamma value for red phase\n");
  ErrorF("-ggamma f              set gamma value for green phase\n");
  ErrorF("-bgamma f              set gamma value for blue phase\n");
  ErrorF("-weight nnn            set RGB weighting at 16 bpp.  Default: 565\n");
#endif /* XF86MONOVGA */
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
  ErrorF("-bestRefresh           Chose modes with the best refresh rate\n");
  ErrorF(
   "-showconfig            show which drivers are included in the server\n");
  xf86UseMsg();
  ErrorF("\n");
}


#ifndef OSNAME
#define OSNAME "unknown"
#endif
#ifndef OSVENDOR
#define OSVENDOR ""
#endif

static void
xf86PrintBanner()
{
  ErrorF("\nXFree86 Version%s/ X Window System\n",XF86_VERSION);
  ErrorF("(protocol Version %d, revision %d, vendor release %d)\n",
         X_PROTOCOL, X_PROTOCOL_REVISION, VENDOR_RELEASE );
  ErrorF("Release Date: %s\n", XF86_DATE);
  ErrorF("\tIf the server is older than 6-12 months, or if your card is "
	 "newer\n"
	 "\tthan the above date, look for a newer version before "
	 "reporting\n"
	 "\tproblems.  (see http://www.XFree86.Org/FAQ)\n");
  ErrorF("Operating System: %s %s\n", OSNAME, OSVENDOR);
}

static void
xf86PrintConfig()
{
  int i;

  ErrorF("Configured drivers:\n");
  for (i = 0; i < xf86MaxScreens; i++)
    if (xf86Screens[i])
      (xf86Screens[i]->PrintIdent)();
}

