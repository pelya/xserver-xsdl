/**************************************************************
 *
 * Shared code for the Darwin X Server
 * running with Quartz or IOKit display mode
 *
 **************************************************************/
/*
 * Copyright (c) 2001-2003 Torrey T. Lyons. All Rights Reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */
/* $XFree86: xc/programs/Xserver/hw/darwin/darwin.c,v 1.56 2003/11/24 05:39:01 torrey Exp $ */

#include "X.h"
#include "Xproto.h"
#include "os.h"
#include "servermd.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "mibstore.h"		// mi backing store implementation
#include "mipointer.h"		// mi software cursor
#include "micmap.h"		// mi colormap code
#include "fb.h"			// fb framebuffer code
#include "site.h"
#include "globals.h"
#include "xf86Version.h"
#include "xf86Date.h"
#include "dix.h"

#include <sys/types.h>
#include <sys/time.h>
#include <sys/syslimits.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define NO_CFPLUGIN
#include <IOKit/IOKitLib.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/ev_keymap.h>

#include "darwin.h"
#include "darwinClut8.h"

/*
 * X server shared global variables
 */
int                     darwinScreensFound = 0;
int                     darwinScreenIndex = 0;
io_connect_t            darwinParamConnect = 0;
int                     darwinEventReadFD = -1;
int                     darwinEventWriteFD = -1;
int                     darwinMouseAccelChange = 1;
int                     darwinFakeButtons = 0;

// location of X11's (0,0) point in global screen coordinates
int                     darwinMainScreenX = 0;
int                     darwinMainScreenY = 0;

// parameters read from the command line or user preferences
unsigned int            darwinDesiredWidth = 0, darwinDesiredHeight = 0;
int                     darwinDesiredDepth = -1;
int                     darwinDesiredRefresh = -1;
char                    *darwinKeymapFile = "USA.keymapping";

// modifier masks for faking mouse buttons
int                     darwinFakeMouse2Mask = NX_COMMANDMASK;
int                     darwinFakeMouse3Mask = NX_ALTERNATEMASK;

static DeviceIntPtr     darwinPointer;
static DeviceIntPtr     darwinKeyboard;

// Common pixmap formats
static PixmapFormatRec formats[] = {
        { 1,    1,      BITMAP_SCANLINE_PAD },
        { 4,    8,      BITMAP_SCANLINE_PAD },
        { 8,    8,      BITMAP_SCANLINE_PAD },
        { 15,   16,     BITMAP_SCANLINE_PAD },
        { 16,   16,     BITMAP_SCANLINE_PAD },
        { 24,   32,     BITMAP_SCANLINE_PAD },
        { 32,   32,     BITMAP_SCANLINE_PAD }
};
const int NUMFORMATS = sizeof(formats)/sizeof(formats[0]);

#ifndef OSNAME
#define OSNAME " Darwin"
#endif
#ifndef OSVENDOR
#define OSVENDOR ""
#endif
#ifndef PRE_RELEASE
#define PRE_RELEASE XF86_VERSION_SNAP
#endif

void
DarwinPrintBanner()
{
#if PRE_RELEASE
  ErrorF("\n"
    "This is a pre-release version of XFree86, and is not supported in any\n"
    "way.  Bugs may be reported to XFree86@XFree86.Org and patches submitted\n"
    "to fixes@XFree86.Org.  Before reporting bugs in pre-release versions,\n"
    "please check the latest version in the XFree86 CVS repository\n"
    "(http://www.XFree86.Org/cvs)\n");
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
  ErrorF(" / X Window System\n");
  ErrorF("(protocol Version %d, revision %d, vendor release %d)\n",
         X_PROTOCOL, X_PROTOCOL_REVISION, VENDOR_RELEASE );
  ErrorF("Release Date: %s\n", XF86_DATE);
  ErrorF("\tIf the server is older than 6-12 months, or if your hardware is\n"
         "\tnewer than the above date, look for a newer version before\n"
         "\treporting problems.  (See http://www.XFree86.Org/FAQ)\n");
  ErrorF("Operating System:%s%s\n", OSNAME, OSVENDOR);
#if defined(BUILDERSTRING)
  ErrorF("%s \n",BUILDERSTRING);
#endif
}


/*
 * DarwinSaveScreen
 *  X screensaver support. Not implemented.
 */
static Bool DarwinSaveScreen(ScreenPtr pScreen, int on)
{
    // FIXME
    if (on == SCREEN_SAVER_FORCER) {
    } else if (on == SCREEN_SAVER_ON) {
    } else {
    }
    return TRUE;
}


/*
 * DarwinAddScreen
 *  This is a callback from dix during AddScreen() from InitOutput().
 *  Initialize the screen and communicate information about it back to dix.
 */
static Bool DarwinAddScreen(
    int         index,
    ScreenPtr   pScreen,
    int         argc,
    char        **argv )
{
    int         bitsPerRGB, i, dpi;
    static int  foundIndex = 0;
    Bool        ret;
    VisualPtr   visual;
    ColormapPtr pmap;
    DarwinFramebufferPtr dfb;

    // reset index of found screens for each server generation
    if (index == 0) foundIndex = 0;

    // allocate space for private per screen storage
    dfb = xalloc(sizeof(DarwinFramebufferRec));
    SCREEN_PRIV(pScreen) = dfb;

    // setup hardware/mode specific details
    ret = DarwinModeAddScreen(foundIndex, pScreen);
    foundIndex++;
    if (! ret)
        return FALSE;

    bitsPerRGB = dfb->bitsPerComponent;

    // reset the visual list
    miClearVisualTypes();

    // setup a single visual appropriate for our pixel type
    if (dfb->colorType == TrueColor) {
        if (!miSetVisualTypes( dfb->colorBitsPerPixel, TrueColorMask,
                               bitsPerRGB, TrueColor )) {
            return FALSE;
        }
    } else if (dfb->colorType == PseudoColor) {
        if (!miSetVisualTypes( dfb->colorBitsPerPixel, PseudoColorMask,
                               bitsPerRGB, PseudoColor )) {
            return FALSE;
        }
    } else if (dfb->colorType == StaticColor) {
        if (!miSetVisualTypes( dfb->colorBitsPerPixel, StaticColorMask,
                               bitsPerRGB, StaticColor )) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    miSetPixmapDepths();

    // machine independent screen init
    // setup _Screen structure in pScreen
    if (monitorResolution)
        dpi = monitorResolution;
    else
        dpi = 75;

    // initialize fb
    if (! fbScreenInit(pScreen,
                dfb->framebuffer,                 // pointer to screen bitmap
                dfb->width, dfb->height,          // screen size in pixels
                dpi, dpi,                         // dots per inch
                dfb->pitch/(dfb->bitsPerPixel/8), // pixel width of framebuffer
                dfb->bitsPerPixel))               // bits per pixel for screen
    {
        return FALSE;
    }

    // set the RGB order correctly for TrueColor
    if (dfb->bitsPerPixel > 8) {
        for (i = 0, visual = pScreen->visuals;  // someday we may have more than 1
            i < pScreen->numVisuals; i++, visual++) {
            if (visual->class == TrueColor) {
                visual->offsetRed = bitsPerRGB * 2;
                visual->offsetGreen = bitsPerRGB;
                visual->offsetBlue = 0;
                visual->redMask = ((1<<bitsPerRGB)-1) << visual->offsetRed;
                visual->greenMask = ((1<<bitsPerRGB)-1) << visual->offsetGreen;
                visual->blueMask = ((1<<bitsPerRGB)-1) << visual->offsetBlue;
            }
        }
    }

#ifdef RENDER
    if (! fbPictureInit(pScreen, 0, 0)) {
        return FALSE;
    }
#endif

#ifdef MITSHM
    ShmRegisterFbFuncs(pScreen);
#endif

    // this must be initialized (why doesn't X have a default?)
    pScreen->SaveScreen = DarwinSaveScreen;

    // finish mode dependent screen setup including cursor support
    if (!DarwinModeSetupScreen(index, pScreen)) {
        return FALSE;
    }

    // create and install the default colormap and
    // set pScreen->blackPixel / pScreen->white
    if (!miCreateDefColormap( pScreen )) {
        return FALSE;
    }

    /* Set the colormap to the statically defined one if we're in 8 bit
     * mode and we're using a fixed color map.  Essentially this translates
     * to Darwin/x86 in 8-bit mode.
     */
    if( (dfb->colorBitsPerPixel == 8) &&
                (dfb->colorType == StaticColor) )
    {
        pmap = miInstalledMaps[pScreen->myNum];
        visual = pmap->pVisual;
        for( i = 0; i < visual->ColormapEntries; i++ ) {
            pmap->red[i].co.local.red   = darwinClut8[i].red;
            pmap->red[i].co.local.green = darwinClut8[i].green;
            pmap->red[i].co.local.blue  = darwinClut8[i].blue;
        }
    }

    dixScreenOrigins[index].x = dfb->x;
    dixScreenOrigins[index].y = dfb->y;

    ErrorF("Screen %d added: %dx%d @ (%d,%d)\n",
            index, dfb->width, dfb->height, dfb->x, dfb->y);

    return TRUE;
}

/*
 =============================================================================

 mouse and keyboard callbacks

 =============================================================================
*/

/*
 * DarwinChangePointerControl
 *  Set mouse acceleration and thresholding
 *  FIXME: We currently ignore the threshold in ctrl->threshold.
 */
static void DarwinChangePointerControl(
    DeviceIntPtr    device,
    PtrCtrl         *ctrl )
{
    kern_return_t   kr;
    double          acceleration;

    if (!darwinMouseAccelChange)
        return;

    acceleration = ctrl->num / ctrl->den;
    kr = IOHIDSetMouseAcceleration( darwinParamConnect, acceleration );
    if (kr != KERN_SUCCESS)
        ErrorF( "Could not set mouse acceleration with kernel return = 0x%x.\n", kr );
}


/*
 * DarwinMouseProc
 *  Handle the initialization, etc. of a mouse
 */
static int DarwinMouseProc(
    DeviceIntPtr    pPointer,
    int             what )
{
    char map[6];

    switch (what) {

        case DEVICE_INIT:
            pPointer->public.on = FALSE;

            // Set button map.
            map[1] = 1;
            map[2] = 2;
            map[3] = 3;
            map[4] = 4;
            map[5] = 5;
            InitPointerDeviceStruct( (DevicePtr)pPointer,
                        map,
                        5,   // numbuttons (4 & 5 are scroll wheel)
                        miPointerGetMotionEvents,
                        DarwinChangePointerControl,
                        0 );
            break;

        case DEVICE_ON:
            pPointer->public.on = TRUE;
            AddEnabledDevice( darwinEventReadFD );
            return Success;

        case DEVICE_CLOSE:
        case DEVICE_OFF:
            pPointer->public.on = FALSE;
            RemoveEnabledDevice( darwinEventReadFD );
            return Success;
    }

    return Success;
}


/*
 * DarwinKeybdProc
 *  Callback from X
 */
static int DarwinKeybdProc( DeviceIntPtr pDev, int onoff )
{
    switch ( onoff ) {
        case DEVICE_INIT:
            DarwinKeyboardInit( pDev );
            break;
        case DEVICE_ON:
            pDev->public.on = TRUE;
            AddEnabledDevice( darwinEventReadFD );
            break;
        case DEVICE_OFF:
            pDev->public.on = FALSE;
            RemoveEnabledDevice( darwinEventReadFD );
            break;
        case DEVICE_CLOSE:
            break;
    }

    return Success;
}

/*
===========================================================================

 Utility routines

===========================================================================
*/

/*
 * DarwinFindLibraryFile
 *  Search for a file in the standard Library paths, which are (in order):
 *
 *      ~/Library/              user specific
 *      /Library/               host specific
 *      /Network/Library/       LAN specific
 *      /System/Library/        OS specific
 *
 *  A sub-path can be specified to search in below the various Library
 *  directories. Returns a new character string (owned by the caller)
 *  containing the full path to the first file found.
 */
static char * DarwinFindLibraryFile(
    const char *file,
    const char *pathext )
{
    // Library search paths
    char *pathList[] = {
        "",
        "/Network",
        "/System",
        NULL
    };
    char *home;
    char *fullPath;
    int i = 0;

    // Return the file name as is if it is already a fully qualified path.
    if (!access(file, F_OK)) {
        fullPath = xalloc(strlen(file)+1);
        strcpy(fullPath, file);
        return fullPath;
    }

    fullPath = xalloc(PATH_MAX);

    home = getenv("HOME");
    if (home) {
        snprintf(fullPath, PATH_MAX, "%s/Library/%s/%s", home, pathext, file);
        if (!access(fullPath, F_OK))
            return fullPath;
    }

    while (pathList[i]) {
        snprintf(fullPath, PATH_MAX, "%s/Library/%s/%s", pathList[i++],
                 pathext, file);
        if (!access(fullPath, F_OK))
            return fullPath;
    }

    xfree(fullPath);
    return NULL;
}


/*
 * DarwinParseModifierList
 *  Parse a list of modifier names and return a corresponding modifier mask
 */
static int DarwinParseModifierList(
    const char *constmodifiers) // string containing list of modifier names
{
    int result = 0;

    if (constmodifiers) {
        char *modifiers = strdup(constmodifiers);
        char *modifier;
        int nxkey;
        char *p = modifiers;

        while (p) {
            modifier = strsep(&p, " ,+&|/"); // allow lots of separators
            nxkey = DarwinModifierStringToNXKey(modifier);
            if (nxkey != -1)
                result |= DarwinModifierNXKeyToNXMask(nxkey);
            else
                ErrorF("fakebuttons: Unknown modifier \"%s\"\n", modifier);
        }
        free(modifiers);
    }
    return result;
}

/*
===========================================================================

 Functions needed to link against device independent X

===========================================================================
*/

/*
 * InitInput
 *  Register the keyboard and mouse devices
 */
void InitInput( int argc, char **argv )
{
    darwinPointer = AddInputDevice(DarwinMouseProc, TRUE);
    RegisterPointerDevice( darwinPointer );

    darwinKeyboard = AddInputDevice(DarwinKeybdProc, TRUE);
    RegisterKeyboardDevice( darwinKeyboard );

    DarwinEQInit( (DevicePtr)darwinKeyboard, (DevicePtr)darwinPointer );

    DarwinModeInitInput(argc, argv);
}


/*
 * DarwinAdjustScreenOrigins
 *  Shift all screens so the X11 (0, 0) coordinate is at the top
 *  left of the global screen coordinates.
 *
 *  Screens can be arranged so the top left isn't on any screen, so
 *  instead use the top left of the leftmost screen as (0,0). This
 *  may mean some screen space is in -y, but it's better that (0,0)
 *  be onscreen, or else default xterms disappear. It's better that
 *  -y be used than -x, because when popup menus are forced
 *  "onscreen" by dumb window managers like twm, they'll shift the
 *  menus down instead of left, which still looks funny but is an
 *  easier target to hit.
 */
void
DarwinAdjustScreenOrigins(ScreenInfo *pScreenInfo)
{
    int i, left, top;

    left = dixScreenOrigins[0].x;
    top  = dixScreenOrigins[0].y;

    /* Find leftmost screen. If there's a tie, take the topmost of the two. */
    for (i = 1; i < pScreenInfo->numScreens; i++) {
        if (dixScreenOrigins[i].x < left  ||
            (dixScreenOrigins[i].x == left &&
             dixScreenOrigins[i].y < top))
        {
            left = dixScreenOrigins[i].x;
            top = dixScreenOrigins[i].y;
        }
    }

    darwinMainScreenX = left;
    darwinMainScreenY = top;

    /* Shift all screens so that there is a screen whose top left
       is at X11 (0,0) and at global screen coordinate
       (darwinMainScreenX, darwinMainScreenY). */

    if (darwinMainScreenX != 0 || darwinMainScreenY != 0) {
        for (i = 0; i < pScreenInfo->numScreens; i++) {
            dixScreenOrigins[i].x -= darwinMainScreenX;
            dixScreenOrigins[i].y -= darwinMainScreenY;
            ErrorF("Screen %d placed at X11 coordinate (%d,%d).\n",
                   i, dixScreenOrigins[i].x, dixScreenOrigins[i].y);
        }
    }
}


/*
 * InitOutput
 *  Initialize screenInfo for all actually accessible framebuffers.
 *
 *  The display mode dependent code gets called three times. The mode
 *  specific InitOutput routines are expected to discover the number
 *  of potentially useful screens and cache routes to them internally.
 *  Inside DarwinAddScreen are two other mode specific calls.
 *  A mode specific AddScreen routine is called for each screen to
 *  actually initialize the screen with the ScreenPtr structure.
 *  After other screen setup has been done, a mode specific
 *  SetupScreen function can be called to finalize screen setup.
 */
void InitOutput( ScreenInfo *pScreenInfo, int argc, char **argv )
{
    int i;
    static unsigned long generation = 0;

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    // List how we want common pixmap formats to be padded
    pScreenInfo->numPixmapFormats = NUMFORMATS;
    for (i = 0; i < NUMFORMATS; i++)
        pScreenInfo->formats[i] = formats[i];

    // Allocate private storage for each screen's Darwin specific info
    if (generation != serverGeneration) {
        darwinScreenIndex = AllocateScreenPrivateIndex();
        generation = serverGeneration;
    }

    // Discover screens and do mode specific initialization
    DarwinModeInitOutput(argc, argv);

    // Add screens
    for (i = 0; i < darwinScreensFound; i++) {
        AddScreen( DarwinAddScreen, argc, argv );
    }

    DarwinAdjustScreenOrigins(pScreenInfo);
}


/*
 * OsVendorFataError
 */
void OsVendorFatalError( void )
{
    ErrorF( "   OsVendorFatalError\n" );
}


/*
 * OsVendorInit
 *  Initialization of Darwin OS support.
 */
void OsVendorInit(void)
{
    if (serverGeneration == 1) {
        DarwinPrintBanner();
    }

    // Find the full path to the keymapping file.
    if ( darwinKeymapFile ) {
        char *tempStr = DarwinFindLibraryFile(darwinKeymapFile, "Keyboards");
        if ( !tempStr ) {
            ErrorF("Could not find keymapping file %s.\n", darwinKeymapFile);
        } else {
            ErrorF("Using keymapping provided in %s.\n", tempStr);
        }
        darwinKeymapFile = tempStr;
    }

    if ( !darwinKeymapFile ) {
        ErrorF("Reading keymap from the system.\n");
    }
}


/*
 * ddxProcessArgument --
 *  Process device-dependent command line args. Returns 0 if argument is
 *  not device dependent, otherwise Count of number of elements of argv
 *  that are part of a device dependent commandline option.
 */
int ddxProcessArgument( int argc, char *argv[], int i )
{
    int numDone;

    if ((numDone = DarwinModeProcessArgument( argc, argv, i )))
        return numDone;

    if ( !strcmp( argv[i], "-fakebuttons" ) ) {
        darwinFakeButtons = TRUE;
        ErrorF( "Faking a three button mouse\n" );
        return 1;
    }

    if ( !strcmp( argv[i], "-nofakebuttons" ) ) {
        darwinFakeButtons = FALSE;
        ErrorF( "Not faking a three button mouse\n" );
        return 1;
    }

    if (!strcmp( argv[i], "-fakemouse2" ) ) {
        if ( i == argc-1 ) {
            FatalError( "-fakemouse2 must be followed by a modifer list\n" );
        }
        if (!strcasecmp(argv[i+1], "none") || !strcmp(argv[i+1], ""))
            darwinFakeMouse2Mask = 0;
        else
            darwinFakeMouse2Mask = DarwinParseModifierList(argv[i+1]);
        ErrorF("Modifier mask to fake mouse button 2 = 0x%x\n",
               darwinFakeMouse2Mask);
        return 2;
    }

    if (!strcmp( argv[i], "-fakemouse3" ) ) {
        if ( i == argc-1 ) {
            FatalError( "-fakemouse3 must be followed by a modifer list\n" );
        }
        if (!strcasecmp(argv[i+1], "none") || !strcmp(argv[i+1], ""))
            darwinFakeMouse3Mask = 0;
        else
            darwinFakeMouse3Mask = DarwinParseModifierList(argv[i+1]);
        ErrorF("Modifier mask to fake mouse button 3 = 0x%x\n",
               darwinFakeMouse3Mask);
        return 2;
    }

    if ( !strcmp( argv[i], "-keymap" ) ) {
        if ( i == argc-1 ) {
            FatalError( "-keymap must be followed by a filename\n" );
        }
        darwinKeymapFile = argv[i+1];
        return 2;
    }

    if ( !strcmp( argv[i], "-nokeymap" ) ) {
        darwinKeymapFile = NULL;
        return 1;
    }

    if ( !strcmp( argv[i], "-size" ) ) {
        if ( i >= argc-2 ) {
            FatalError( "-size must be followed by two numbers\n" );
        }
#ifdef OLD_POWERBOOK_G3
        ErrorF( "Ignoring unsupported -size option on old PowerBook G3\n" );
#else
        darwinDesiredWidth = atoi( argv[i+1] );
        darwinDesiredHeight = atoi( argv[i+2] );
        ErrorF( "Attempting to use width x height = %i x %i\n",
                darwinDesiredWidth, darwinDesiredHeight );
#endif
        return 3;
    }

    if ( !strcmp( argv[i], "-depth" ) ) {
        int     bitDepth;

        if ( i == argc-1 ) {
            FatalError( "-depth must be followed by a number\n" );
        }
#ifdef OLD_POWERBOOK_G3
        ErrorF( "Ignoring unsupported -depth option on old PowerBook G3\n");
#else
        bitDepth = atoi( argv[i+1] );
        if (bitDepth == 8)
            darwinDesiredDepth = 0;
        else if (bitDepth == 15)
            darwinDesiredDepth = 1;
        else if (bitDepth == 24)
            darwinDesiredDepth = 2;
        else
            FatalError( "Unsupported pixel depth. Use 8, 15, or 24 bits\n" );
        ErrorF( "Attempting to use pixel depth of %i\n", bitDepth );
#endif
        return 2;
    }

    if ( !strcmp( argv[i], "-refresh" ) ) {
        if ( i == argc-1 ) {
            FatalError( "-refresh must be followed by a number\n" );
        }
#ifdef OLD_POWERBOOK_G3
        ErrorF( "Ignoring unsupported -refresh option on old PowerBook G3\n");
#else
        darwinDesiredRefresh = atoi( argv[i+1] );
        ErrorF( "Attempting to use refresh rate of %i\n", darwinDesiredRefresh );
#endif
        return 2;
    }

    if (!strcmp( argv[i], "-showconfig" ) || !strcmp( argv[i], "-version" )) {
        DarwinPrintBanner();
        exit(0);
    }

    // XDarwinStartup uses this argument to indicate the IOKit X server
    // should be started. Ignore it here.
    if ( !strcmp( argv[i], "-iokit" ) ) {
        return 1;
    }

    return 0;
}


/*
 * ddxUseMsg --
 *  Print out correct use of device dependent commandline options.
 *  Maybe the user now knows what really to do ...
 */
void ddxUseMsg( void )
{
    ErrorF("\n");
    ErrorF("\n");
    ErrorF("Device Dependent Usage:\n");
    ErrorF("\n");
    ErrorF("-fakebuttons : fake a three button mouse with Command and Option keys.\n");
    ErrorF("-nofakebuttons : don't fake a three button mouse.\n");
    ErrorF("-fakemouse2 <modifiers> : fake middle mouse button with modifier keys.\n");
    ErrorF("-fakemouse3 <modifiers> : fake right mouse button with modifier keys.\n");
    ErrorF("  ex: -fakemouse2 \"option,shift\" = option-shift-click is middle button.\n");
    ErrorF("-keymap <file> : read the keymapping from a file instead of the kernel.\n");
    ErrorF("-version : show the server version.\n");
    ErrorF("\n");
#ifdef DARWIN_WITH_QUARTZ
    ErrorF("Quartz modes:\n");
    ErrorF("-fullscreen : run full screen in parallel with Mac OS X window server.\n");
    ErrorF("-rootless : run rootless inside Mac OS X window server.\n");
    ErrorF("-quartz : use default Mac OS X window server mode\n");
    ErrorF("\n");
    ErrorF("Options ignored in rootless mode:\n");
#endif
    ErrorF("-size <height> <width> : use a screen resolution of <height> x <width>.\n");
    ErrorF("-depth <8,15,24> : use this bit depth.\n");
    ErrorF("-refresh <rate> : use a monitor refresh rate of <rate> Hz.\n");
    ErrorF("\n");
}


/*
 * ddxGiveUp --
 *      Device dependent cleanup. Called by dix before normal server death.
 */
void ddxGiveUp( void )
{
    ErrorF( "Quitting XDarwin...\n" );

    DarwinModeGiveUp();
}


/*
 * AbortDDX --
 *      DDX - specific abort routine.  Called by AbortServer(). The attempt is
 *      made to restore all original setting of the displays. Also all devices
 *      are closed.
 */
void AbortDDX( void )
{
    ErrorF( "   AbortDDX\n" );
    /*
     * This is needed for a abnormal server exit, since the normal exit stuff
     * MUST also be performed (i.e. the vt must be left in a defined state)
     */
    ddxGiveUp();
}


#ifdef DPMSExtension
/*
 * DPMS extension stubs
 */
Bool DPMSSupported(void)
{
    return FALSE;
}

void DPMSSet(int level)
{
}

int DPMSGet(int *level)
{
    return -1;
}
#endif


#include "mivalidate.h" // for union _Validate used by windowstr.h
#include "windowstr.h"  // for struct _Window
#include "scrnintstr.h" // for struct _Screen

// This is copied from Xserver/hw/xfree86/common/xf86Helper.c.
// Quartz mode uses this when switching in and out of Quartz.
// Quartz or IOKit can use this when waking from sleep.
// Copyright (c) 1997-1998 by The XFree86 Project, Inc.

/*
 * xf86SetRootClip --
 *	Enable or disable rendering to the screen by
 *	setting the root clip list and revalidating
 *	all of the windows
 */

void
xf86SetRootClip (ScreenPtr pScreen, BOOL enable)
{
    WindowPtr	pWin = WindowTable[pScreen->myNum];
    WindowPtr	pChild;
    Bool	WasViewable = (Bool)(pWin->viewable);
    Bool	anyMarked = TRUE;
    RegionPtr	pOldClip = NULL, bsExposed;
#ifdef DO_SAVE_UNDERS
    Bool	dosave = FALSE;
#endif
    WindowPtr   pLayerWin;
    BoxRec	box;

    if (WasViewable)
    {
	for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib)
	{
	    (void) (*pScreen->MarkOverlappedWindows)(pChild,
						     pChild,
						     &pLayerWin);
	}
	(*pScreen->MarkWindow) (pWin);
	anyMarked = TRUE;
	if (pWin->valdata)
	{
	    if (HasBorder (pWin))
	    {
		RegionPtr	borderVisible;

		borderVisible = REGION_CREATE(pScreen, NullBox, 1);
		REGION_SUBTRACT(pScreen, borderVisible,
				&pWin->borderClip, &pWin->winSize);
		pWin->valdata->before.borderVisible = borderVisible;
	    }
	    pWin->valdata->before.resized = TRUE;
	}
    }

    /*
     * Use REGION_BREAK to avoid optimizations in ValidateTree
     * that assume the root borderClip can't change well, normally
     * it doesn't...)
     */
    if (enable)
    {
	box.x1 = 0;
	box.y1 = 0;
	box.x2 = pScreen->width;
	box.y2 = pScreen->height;
	REGION_RESET(pScreen, &pWin->borderClip, &box);
	REGION_BREAK (pWin->drawable.pScreen, &pWin->clipList);
    }
    else
    {
	REGION_EMPTY(pScreen, &pWin->borderClip);
	REGION_BREAK (pWin->drawable.pScreen, &pWin->clipList);
    }

    ResizeChildrenWinSize (pWin, 0, 0, 0, 0);

    if (WasViewable)
    {
	if (pWin->backStorage)
	{
	    pOldClip = REGION_CREATE(pScreen, NullBox, 1);
	    REGION_COPY(pScreen, pOldClip, &pWin->clipList);
	}

	if (pWin->firstChild)
	{
	    anyMarked |= (*pScreen->MarkOverlappedWindows)(pWin->firstChild,
							   pWin->firstChild,
							   (WindowPtr *)NULL);
	}
	else
	{
	    (*pScreen->MarkWindow) (pWin);
	    anyMarked = TRUE;
	}

#ifdef DO_SAVE_UNDERS
	if (DO_SAVE_UNDERS(pWin))
	{
	    dosave = (*pScreen->ChangeSaveUnder)(pLayerWin, pLayerWin);
	}
#endif /* DO_SAVE_UNDERS */

	if (anyMarked)
	    (*pScreen->ValidateTree)(pWin, NullWindow, VTOther);
    }

    if (pWin->backStorage &&
	((pWin->backingStore == Always) || WasViewable))
    {
	if (!WasViewable)
	    pOldClip = &pWin->clipList; /* a convenient empty region */
	bsExposed = (*pScreen->TranslateBackingStore)
			     (pWin, 0, 0, pOldClip,
			      pWin->drawable.x, pWin->drawable.y);
	if (WasViewable)
	    REGION_DESTROY(pScreen, pOldClip);
	if (bsExposed)
	{
	    RegionPtr	valExposed = NullRegion;

	    if (pWin->valdata)
		valExposed = &pWin->valdata->after.exposed;
	    (*pScreen->WindowExposures) (pWin, valExposed, bsExposed);
	    if (valExposed)
		REGION_EMPTY(pScreen, valExposed);
	    REGION_DESTROY(pScreen, bsExposed);
	}
    }
    if (WasViewable)
    {
	if (anyMarked)
	    (*pScreen->HandleExposures)(pWin);
#ifdef DO_SAVE_UNDERS
	if (dosave)
	    (*pScreen->PostChangeSaveUnder)(pLayerWin, pLayerWin);
#endif /* DO_SAVE_UNDERS */
	if (anyMarked && pScreen->PostValidateTree)
	    (*pScreen->PostValidateTree)(pWin, NullWindow, VTOther);
    }
    if (pWin->realized)
	WindowsRestructured ();
    FlushAllOutput ();
}
