/* $TOG: InitOutput.c /main/20 1998/02/10 13:23:56 kaleb $ */
/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/hw/xwin/InitOutput.c,v 1.35 2003/10/08 11:13:02 eich Exp $ */

#include "win.h"
#include "winconfig.h"
#include "winprefs.h"

/*
 * General global variables
 */

int		g_iNumScreens = 0;
winScreenInfo	g_ScreenInfo[MAXSCREENS];
int		g_iLastScreen = -1;
int		g_fdMessageQueue = WIN_FD_INVALID;
int		g_iScreenPrivateIndex = -1;
int		g_iCmapPrivateIndex = -1;
int		g_iGCPrivateIndex = -1;
int		g_iPixmapPrivateIndex = -1;
int		g_iWindowPrivateIndex = -1;
unsigned long	g_ulServerGeneration = 0;
Bool		g_fInitializedDefaultScreens = FALSE;
DWORD		g_dwEnginesSupported = 0;
HINSTANCE	g_hInstance = 0;
HWND		g_hDlgDepthChange = NULL;
HWND		g_hDlgExit = NULL;
Bool		g_fCalledSetLocale = FALSE;
Bool		g_fCalledXInitThreads = FALSE;
int		g_iLogVerbose = 4;
char *		g_pszLogFile = WIN_LOG_FNAME;
Bool		g_fLogInited = FALSE;
const char *	g_pszQueryHost = NULL;


/*
 * Global variables for dynamically loaded libraries and
 * their function pointers
 */

HMODULE		g_hmodDirectDraw = NULL;
FARPROC		g_fpDirectDrawCreate = NULL;
FARPROC		g_fpDirectDrawCreateClipper = NULL;

HMODULE		g_hmodCommonControls = NULL;
FARPROC		g_fpTrackMouseEvent = (FARPROC) (void (*)())NoopDDA;


/* Function prototypes */

#ifdef DDXOSVERRORF
void OsVendorVErrorF (const char *pszFormat, va_list va_args);
#endif


/*
 * For the depth 24 pixmap we default to 32 bits per pixel, but
 * we change this pixmap format later if we detect that the display
 * is going to be running at 24 bits per pixel.
 *
 * FIXME: On second thought, don't DIBs only support 32 bits per pixel?
 * DIBs are the underlying bitmap used for DirectDraw surfaces, so it
 * seems that all pixmap formats with depth 24 would be 32 bits per pixel.
 * Confirm whether depth 24 DIBs can have 24 bits per pixel, then remove/keep
 * the bits per pixel adjustment and update this comment to reflect the
 * situation.  Harold Hunt - 2002/07/02
 */

static PixmapFormatRec g_PixmapFormats[] = {
  { 1,    1,      BITMAP_SCANLINE_PAD },
  { 4,    8,      BITMAP_SCANLINE_PAD },
  { 8,    8,      BITMAP_SCANLINE_PAD },
  { 15,   16,     BITMAP_SCANLINE_PAD },
  { 16,   16,     BITMAP_SCANLINE_PAD },
  { 24,   32,     BITMAP_SCANLINE_PAD },
#ifdef RENDER
  { 32,   32,     BITMAP_SCANLINE_PAD }
#endif
};

const int NUMFORMATS = sizeof (g_PixmapFormats) / sizeof (g_PixmapFormats[0]);


void
winInitializeDefaultScreens (void)
{
  int                   i;
  DWORD			dwWidth, dwHeight;

  /* Bail out early if default screens have already been initialized */
  if (g_fInitializedDefaultScreens)
    return;

  /* Zero the memory used for storing the screen info */
  ZeroMemory (g_ScreenInfo, MAXSCREENS * sizeof (winScreenInfo));

  /* Get default width and height */
  /*
   * NOTE: These defaults will cause the window to cover only
   * the primary monitor in the case that we have multiple monitors.
   */
  dwWidth = GetSystemMetrics (SM_CXSCREEN);
  dwHeight = GetSystemMetrics (SM_CYSCREEN);

  ErrorF ("winInitializeDefaultScreens - w %d h %d\n", dwWidth, dwHeight);

  /* Set a default DPI, if no parameter was passed */
  if (monitorResolution == 0)
    monitorResolution = WIN_DEFAULT_DPI;

  for (i = 0; i < MAXSCREENS; ++i)
    {
      g_ScreenInfo[i].dwScreen = i;
      g_ScreenInfo[i].dwWidth  = dwWidth;
      g_ScreenInfo[i].dwHeight = dwHeight;
      g_ScreenInfo[i].dwUserWidth  = dwWidth;
      g_ScreenInfo[i].dwUserHeight = dwHeight;
      g_ScreenInfo[i].fUserGaveHeightAndWidth
	=  WIN_DEFAULT_USER_GAVE_HEIGHT_AND_WIDTH;
      g_ScreenInfo[i].dwBPP = WIN_DEFAULT_BPP;
      g_ScreenInfo[i].dwClipUpdatesNBoxes = WIN_DEFAULT_CLIP_UPDATES_NBOXES;
      g_ScreenInfo[i].fEmulatePseudo = WIN_DEFAULT_EMULATE_PSEUDO;
      g_ScreenInfo[i].dwRefreshRate = WIN_DEFAULT_REFRESH;
      g_ScreenInfo[i].pfb = NULL;
      g_ScreenInfo[i].fFullScreen = FALSE;
      g_ScreenInfo[i].fDecoration = TRUE;
      g_ScreenInfo[i].fRootless = FALSE;
      g_ScreenInfo[i].fMultiWindow = FALSE;
      g_ScreenInfo[i].fMultipleMonitors = FALSE;
      g_ScreenInfo[i].fClipboard = FALSE;
      g_ScreenInfo[i].fLessPointer = FALSE;
      g_ScreenInfo[i].fScrollbars = FALSE;
      g_ScreenInfo[i].fNoTrayIcon = FALSE;
      g_ScreenInfo[i].iE3BTimeout = WIN_E3B_OFF;
      g_ScreenInfo[i].dwWidth_mm = (dwWidth / WIN_DEFAULT_DPI)
	* 25.4;
      g_ScreenInfo[i].dwHeight_mm = (dwHeight / WIN_DEFAULT_DPI)
	* 25.4;
      g_ScreenInfo[i].fUseWinKillKey = WIN_DEFAULT_WIN_KILL;
      g_ScreenInfo[i].fUseUnixKillKey = WIN_DEFAULT_UNIX_KILL;
      g_ScreenInfo[i].fIgnoreInput = FALSE;
      g_ScreenInfo[i].fExplicitScreen = FALSE;
    }

  /* Signal that the default screens have been initialized */
  g_fInitializedDefaultScreens = TRUE;

  ErrorF ("winInitializeDefaultScreens - Returning\n");
}


/* See Porting Layer Definition - p. 57 */
void
ddxGiveUp()
{
#if CYGDEBUG
  ErrorF ("ddxGiveUp\n");
#endif

  /* Notify the worker threads we're exiting */
  winDeinitClipboard ();
  winDeinitMultiWindowWM ();

  /* Close our handle to our message queue */
  if (g_fdMessageQueue != WIN_FD_INVALID)
    {
      /* Close /dev/windows */
      close (g_fdMessageQueue);

      /* Set the file handle to invalid */
      g_fdMessageQueue = WIN_FD_INVALID;
    }

  if (!g_fLogInited) {
    LogInit(g_pszLogFile, NULL);
    g_fLogInited = TRUE;
  }  
  LogClose();

  /*
   * At this point we aren't creating any new screens, so
   * we are guaranteed to not need the DirectDraw functions.
   */
  if (g_hmodDirectDraw != NULL)
    {
      FreeLibrary (g_hmodDirectDraw);
      g_hmodDirectDraw = NULL;
      g_fpDirectDrawCreate = NULL;
      g_fpDirectDrawCreateClipper = NULL;
    }

  /* Unload our TrackMouseEvent funtion pointer */
  if (g_hmodCommonControls != NULL)
    {
      FreeLibrary (g_hmodCommonControls);
      g_hmodCommonControls = NULL;
      g_fpTrackMouseEvent = (FARPROC) (void (*)())NoopDDA;
    }
  
  /* Tell Windows that we want to end the app */
  PostQuitMessage (0);
}


/* See Porting Layer Definition - p. 57 */
void
AbortDDX (void)
{
#if CYGDEBUG
  ErrorF ("AbortDDX\n");
#endif
  ddxGiveUp ();
}


void
OsVendorInit (void)
{
#ifdef DDXOSVERRORF
  if (!OsVendorVErrorFProc)
    OsVendorVErrorFProc = OsVendorVErrorF;
#endif

  if (!g_fLogInited) {
    LogInit(g_pszLogFile, NULL);
    g_fLogInited = TRUE;
  }  
  LogSetParameter(XLOG_FLUSH, 1);
  LogSetParameter(XLOG_VERBOSITY, g_iLogVerbose);

  /* Add a default screen if no screens were specified */
  if (g_iNumScreens == 0)
    {
      ErrorF ("OsVendorInit - Creating bogus screen 0\n");

      /* 
       * We need to initialize default screens if no arguments
       * were processed.  Otherwise, the default screens would
       * already have been initialized by ddxProcessArgument ().
       */
      winInitializeDefaultScreens ();

      /*
       * Add a screen 0 using the defaults set by 
       * winInitializeDefaultScreens () and any additional parameters
       * processed by ddxProcessArgument ().
       */
      g_iNumScreens = 1;
      g_iLastScreen = 0;

      /* We have to flag this as an explicit screen, even though it isn't */
      g_ScreenInfo[0].fExplicitScreen = TRUE;
    }
}


/* See Porting Layer Definition - p. 57 */
void
ddxUseMsg (void)
{
  ErrorF ("-depth bits_per_pixel\n"
	  "\tSpecify an optional bitdepth to use in fullscreen mode\n"
	  "\twith a DirectDraw engine.\n");

  ErrorF ("-emulate3buttons [timeout]\n"
	  "\tEmulate 3 button mouse with an optional timeout in\n"
	  "\tmilliseconds.\n");

  ErrorF ("-engine engine_type_id\n"
	  "\tOverride the server's automatically selected engine type:\n"
	  "\t\t1 - Shadow GDI\n"
	  "\t\t2 - Shadow DirectDraw\n"
	  "\t\t4 - Shadow DirectDraw4 Non-Locking\n"
	  "\t\t16 - Native GDI - experimental\n");

  ErrorF ("-fullscreen\n"
	  "\tRun the server in fullscreen mode.\n");
  
  ErrorF ("-refresh rate_in_Hz\n"
	  "\tSpecify an optional refresh rate to use in fullscreen mode\n"
	  "\twith a DirectDraw engine.\n");

  ErrorF ("-screen scr_num [width height]\n"
	  "\tEnable screen scr_num and optionally specify a width and\n"
	  "\theight for that screen.\n");

  ErrorF ("-lesspointer\n"
	  "\tHide the windows mouse pointer when it is over an inactive\n"
          "\tXFree86 window.  This prevents ghost cursors appearing where\n"
	  "\tthe Windows cursor is drawn overtop of the X cursor\n");

  ErrorF ("-nodecoration\n"
          "\tDo not draw a window border, title bar, etc.  Windowed\n"
	  "\tmode only.\n");

  ErrorF ("-rootless\n"
	  "\tEXPERIMENTAL: Run the server in pseudo-rootless mode.\n");

  ErrorF ("-multiwindow\n"
	  "\tEXPERIMENTAL: Run the server in multi-window mode.\n");

  ErrorF ("-multiplemonitors\n"
	  "\tEXPERIMENTAL: Use the entire virtual screen if multiple\n"
	  "\tmonitors are present.\n");

  ErrorF ("-clipboard\n"
	  "\tEXPERIMENTAL: Run the clipboard integration module.\n");

  ErrorF ("-scrollbars\n"
	  "\tIn windowed mode, allow screens bigger than the Windows desktop.\n"
	  "\tMoreover, if the window has decorations, one can now resize\n"
	  "\tit.\n");

  ErrorF ("-[no]trayicon\n"
          "\tDo not create a tray icon.  Default is to create one\n"
	  "\ticon per screen.  You can globally disable tray icons with\n"
	  "\t-notrayicon, then enable it for specific screens with\n"
	  "\t-trayicon for those screens.\n");

  ErrorF ("-clipupdates num_boxes\n"
	  "\tUse a clipping region to constrain shadow update blits to\n"
	  "\tthe updated region when num_boxes, or more, are in the\n"
	  "\tupdated region.  Currently supported only by `-engine 1'.\n");

  ErrorF ("-emulatepseudo\n"
	  "\tCreate a depth 8 PseudoColor visual when running in\n"
	  "\tdepths 15, 16, 24, or 32, collectively known as TrueColor\n"
	  "\tdepths.  The PseudoColor visual does not have correct colors,\n"
	  "\tand it may crash, but it at least allows you to run your\n"
	  "\tapplication in TrueColor modes.\n");

  ErrorF ("-[no]unixkill\n"
          "\tCtrl+Alt+Backspace exits the X Server.\n");

  ErrorF ("-[no]winkill\n"
          "\tAlt+F4 exits the X Server.\n");

  ErrorF ("-xf86config\n"
          "\tSpecify a configuration file.\n");

  ErrorF ("-keyboard\n"
	  "\tSpecify a keyboard device from the configuration file.\n");
}


/* See Porting Layer Definition - p. 57 */
/*
 * INPUT
 * argv: pointer to an array of null-terminated strings, one for
 *   each token in the X Server command line; the first token
 *   is 'XWin.exe', or similar.
 * argc: a count of the number of tokens stored in argv.
 * i: a zero-based index into argv indicating the current token being
 *   processed.
 *
 * OUTPUT
 * return: return the number of tokens processed correctly.
 *
 * NOTE
 * When looking for n tokens, check that i + n is less than argc.  Or,
 *   you may check if i is greater than or equal to argc, in which case
 *   you should display the UseMsg () and return 0.
 */

/* Check if enough arguments are given for the option */
#define CHECK_ARGS(count) if (i + count >= argc) { UseMsg (); return 0; }

/* Compare the current option with the string. */ 
#define IS_OPTION(name) (strcmp (argv[i], name) == 0)

int
ddxProcessArgument (int argc, char *argv[], int i)
{
  static Bool		s_fBeenHere = FALSE;

  /* Initialize once */
  if (!s_fBeenHere)
    {
#ifdef DDXOSVERRORF
      /*
       * This initialises our hook into VErrorF () for catching log messages
       * that are generated before OsInit () is called.
       */
      OsVendorVErrorFProc = OsVendorVErrorF;
#endif

      s_fBeenHere = TRUE;

      /*
       * Initialize default screen settings.  We have to do this before
       * OsVendorInit () gets called, otherwise we will overwrite
       * settings changed by parameters such as -fullscreen, etc.
       */
      ErrorF ("ddxProcessArgument - Initializing default screens\n");
      winInitializeDefaultScreens ();
    }

#if CYGDEBUG
  ErrorF ("ddxProcessArgument - arg: %s\n", argv[i]);
#endif
  
  /*
   * Look for the '-screen scr_num [width height]' argument
   */
  if (strcmp (argv[i], "-screen") == 0)
    {
      int		iArgsProcessed = 1;
      int		nScreenNum;
      int		iWidth, iHeight;

#if CYGDEBUG
      ErrorF ("ddxProcessArgument - screen - argc: %d i: %d\n",
	      argc, i);
#endif

      /* Display the usage message if the argument is malformed */
      if (i + 1 >= argc)
	{
	  return 0;
	}
      
      /* Grab screen number */
      nScreenNum = atoi (argv[i + 1]);

      /* Validate the specified screen number */
      if (nScreenNum < 0 || nScreenNum >= MAXSCREENS)
        {
          ErrorF ("ddxProcessArgument - screen - Invalid screen number %d\n",
		  nScreenNum);
          UseMsg ();
	  return 0;
        }

      /* Look for 'WxD' or 'W D' */
      if (i + 2 < argc
	  && 2 == sscanf (argv[i + 2], "%dx%d",
			  (int *) &iWidth,
			  (int *) &iHeight))
	{
	  ErrorF ("ddxProcessArgument - screen - Found ``WxD'' arg\n");
	  iArgsProcessed = 3;
	  g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = TRUE;
	  g_ScreenInfo[nScreenNum].dwWidth = iWidth;
	  g_ScreenInfo[nScreenNum].dwHeight = iHeight;
	  g_ScreenInfo[nScreenNum].dwUserWidth = iWidth;
	  g_ScreenInfo[nScreenNum].dwUserHeight = iHeight;
	}
      else if (i + 3 < argc
	       && 1 == sscanf (argv[i + 2], "%d",
			       (int *) &iWidth)
	       && 1 == sscanf (argv[i + 3], "%d",
			       (int *) &iHeight))
	{
	  ErrorF ("ddxProcessArgument - screen - Found ``W D'' arg\n");
	  iArgsProcessed = 4;
	  g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = TRUE;
	  g_ScreenInfo[nScreenNum].dwWidth = iWidth;
	  g_ScreenInfo[nScreenNum].dwHeight = iHeight;
	  g_ScreenInfo[nScreenNum].dwUserWidth = iWidth;
	  g_ScreenInfo[nScreenNum].dwUserHeight = iHeight;
	}
      else
	{
	  ErrorF ("ddxProcessArgument - screen - Did not find size arg. "
		  "dwWidth: %d dwHeight: %d\n",
		  g_ScreenInfo[nScreenNum].dwWidth,
		  g_ScreenInfo[nScreenNum].dwHeight);
	  iArgsProcessed = 2;
	  g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = FALSE;
	}

      /* Calculate the screen width and height in millimeters */
      if (g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth)
	{
	  g_ScreenInfo[nScreenNum].dwWidth_mm
	    = (g_ScreenInfo[nScreenNum].dwWidth
	       / monitorResolution) * 25.4;
	  g_ScreenInfo[nScreenNum].dwHeight_mm
	    = (g_ScreenInfo[nScreenNum].dwHeight
	       / monitorResolution) * 25.4;
	}

      /* Flag that this screen was explicity specified by the user */
      g_ScreenInfo[nScreenNum].fExplicitScreen = TRUE;

      /*
       * Keep track of the last screen number seen, as parameters seen
       * before a screen number apply to all screens, whereas parameters
       * seen after a screen number apply to that screen number only.
       */
      g_iLastScreen = nScreenNum;

      /* Keep a count of the number of screens */
      ++g_iNumScreens;

      return iArgsProcessed;
    }

  /*
   * Look for the '-engine n' argument
   */
  if (strcmp (argv[i], "-engine") == 0)
    {
      DWORD		dwEngine = 0;
      CARD8		c8OnBits = 0;
      
      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      /* Grab the argument */
      dwEngine = atoi (argv[i]);

      /* Count the one bits in the engine argument */
      c8OnBits = winCountBits (dwEngine);

      /* Argument should only have a single bit on */
      if (c8OnBits != 1)
	{
	  UseMsg ();
	  return 0;
	}

      /* Is this parameter attached to a screen or global? */
      if (-1 == g_iLastScreen)
	{
	  int		j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].dwEnginePreferred = dwEngine;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].dwEnginePreferred = dwEngine;
	}
      
      /* Indicate that we have processed the argument */
      return 2;
    }

  /*
   * Look for the '-fullscreen' argument
   */
  if (strcmp (argv[i], "-fullscreen") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fFullScreen = TRUE;

	      /*
	       * No scrollbars in fullscreen mode. Later, we may want to have
	       * a fullscreen with a bigger virtual screen?
	       */
	      g_ScreenInfo[j].fScrollbars = FALSE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fFullScreen = TRUE;

	  /*
	   * No scrollbars in fullscreen mode. Later, we may want to have
	   * a fullscreen with a bigger virtual screen?
	   */
	  g_ScreenInfo[g_iLastScreen].fScrollbars = FALSE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-lesspointer' argument
   */
  if (strcmp (argv[i], "-lesspointer") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fLessPointer = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
          g_ScreenInfo[g_iLastScreen].fLessPointer = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-nodecoration' argument
   */
  if (strcmp (argv[i], "-nodecoration") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fDecoration = FALSE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fDecoration = FALSE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-rootless' argument
   */
  if (strcmp (argv[i], "-rootless") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fRootless = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fRootless = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-multiwindow' argument
   */
  if (strcmp (argv[i], "-multiwindow") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fMultiWindow = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fMultiWindow = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-multiplemonitors' argument
   */
  if (strcmp (argv[i], "-multiplemonitors") == 0
      || strcmp (argv[i], "-multimonitors") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fMultipleMonitors = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fMultipleMonitors = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-scrollbars' argument
   */
  if (strcmp (argv[i], "-scrollbars") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      /* No scrollbar in fullscreen mode */
	      if (!g_ScreenInfo[j].fFullScreen)
		g_ScreenInfo[j].fScrollbars = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  if (!g_ScreenInfo[g_iLastScreen].fFullScreen)
	    {
	      /* No scrollbar in fullscreen mode */
	      g_ScreenInfo[g_iLastScreen].fScrollbars = TRUE;
	    }
	}

      /* Indicate that we have processed this argument */
      return 1;
    }



  /*
   * Look for the '-clipboard' argument
   */
  if (strcmp (argv[i], "-clipboard") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fClipboard = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fClipboard = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-ignoreinput' argument
   */
  if (strcmp (argv[i], "-ignoreinput") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fIgnoreInput = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fIgnoreInput = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-emulate3buttons' argument
   */
  if (strcmp (argv[i], "-emulate3buttons") == 0)
    {
      int	iArgsProcessed = 1;
      int	iE3BTimeout = WIN_DEFAULT_E3B_TIME;

      /* Grab the optional timeout value */
      if (i + 1 < argc
	  && 1 == sscanf (argv[i + 1], "%d",
			  &iE3BTimeout))
        {
	  /* Indicate that we have processed the next argument */
	  iArgsProcessed++;
        }
      else
	{
	  /*
	   * sscanf () won't modify iE3BTimeout if it doesn't find
	   * the specified format; however, I want to be explicit
	   * about setting the default timeout in such cases to
	   * prevent some programs (me) from getting confused.
	   */
	  iE3BTimeout = WIN_DEFAULT_E3B_TIME;
	}

      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].iE3BTimeout = iE3BTimeout;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].iE3BTimeout = iE3BTimeout;
	}

      /* Indicate that we have processed this argument */
      return iArgsProcessed;
    }

  /*
   * Look for the '-depth n' argument
   */
  if (strcmp (argv[i], "-depth") == 0)
    {
      DWORD		dwBPP = 0;
      
      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      /* Grab the argument */
      dwBPP = atoi (argv[i]);

      /* Is this parameter attached to a screen or global? */
      if (-1 == g_iLastScreen)
	{
	  int		j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].dwBPP = dwBPP;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].dwBPP = dwBPP;
	}
      
      /* Indicate that we have processed the argument */
      return 2;
    }

  /*
   * Look for the '-refresh n' argument
   */
  if (strcmp (argv[i], "-refresh") == 0)
    {
      DWORD		dwRefreshRate = 0;
      
      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      /* Grab the argument */
      dwRefreshRate = atoi (argv[i]);

      /* Is this parameter attached to a screen or global? */
      if (-1 == g_iLastScreen)
	{
	  int		j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].dwRefreshRate = dwRefreshRate;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].dwRefreshRate = dwRefreshRate;
	}
      
      /* Indicate that we have processed the argument */
      return 2;
    }

  /*
   * Look for the '-clipupdates num_boxes' argument
   */
  if (strcmp (argv[i], "-clipupdates") == 0)
    {
      DWORD		dwNumBoxes = 0;
      
      /* Display the usage message if the argument is malformed */
      if (++i >= argc)
	{
	  UseMsg ();
	  return 0;
	}

      /* Grab the argument */
      dwNumBoxes = atoi (argv[i]);

      /* Is this parameter attached to a screen or global? */
      if (-1 == g_iLastScreen)
	{
	  int		j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].dwClipUpdatesNBoxes = dwNumBoxes;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].dwClipUpdatesNBoxes = dwNumBoxes;
	}
      
      /* Indicate that we have processed the argument */
      return 2;
    }

  /*
   * Look for the '-emulatepseudo' argument
   */
  if (strcmp (argv[i], "-emulatepseudo") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fEmulatePseudo = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
          g_ScreenInfo[g_iLastScreen].fEmulatePseudo = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-nowinkill' argument
   */
  if (strcmp (argv[i], "-nowinkill") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fUseWinKillKey = FALSE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fUseWinKillKey = FALSE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-winkill' argument
   */
  if (strcmp (argv[i], "-winkill") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fUseWinKillKey = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fUseWinKillKey = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-nounixkill' argument
   */
  if (strcmp (argv[i], "-nounixkill") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fUseUnixKillKey = FALSE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fUseUnixKillKey = FALSE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-unixkill' argument
   */
  if (strcmp (argv[i], "-unixkill") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fUseUnixKillKey = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fUseUnixKillKey = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-notrayicon' argument
   */
  if (strcmp (argv[i], "-notrayicon") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fNoTrayIcon = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fNoTrayIcon = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-trayicon' argument
   */
  if (strcmp (argv[i], "-trayicon") == 0)
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fNoTrayIcon = FALSE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fNoTrayIcon = FALSE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-fp' argument
   */
  if (IS_OPTION ("-fp"))
    {
      CHECK_ARGS (1);
      g_cmdline.fontPath = argv[++i];
      return 0; /* Let DIX parse this again */
    }

  /*
   * Look for the '-co' argument
   */
  if (IS_OPTION ("-co"))
    {
      CHECK_ARGS (1);
      g_cmdline.rgbPath = argv[++i];
      return 0; /* Let DIX parse this again */
    }

  /*
   * Look for the '-query' argument
   */
  if (IS_OPTION ("-query"))
    {
      CHECK_ARGS (1);
      g_pszQueryHost = argv[++i];
      return 0; /* Let DIX parse this again */
    }

  /*
   * Look for the '-xf86config' argument
   */
  if (IS_OPTION ("-xf86config"))
    {
      CHECK_ARGS (1);
      g_cmdline.configFile = argv[++i];
      return 2;
    }

  /*
   * Look for the '-keyboard' argument
   */
  if (IS_OPTION ("-keyboard"))
    {
      CHECK_ARGS (1);
      g_cmdline.keyboard = argv[++i];
      return 2;
    }

  /*
   * Look for the '-logfile' argument
   */
  if (IS_OPTION ("-logfile"))
    {
      CHECK_ARGS (1);
      g_pszLogFile = argv[++i];
      return 2;
    }

  /*
   * Look for the '-logverbose' argument
   */
  if (IS_OPTION ("-logverbose"))
    {
      CHECK_ARGS (1);
      g_iLogVerbose = atoi(argv[++i]);
      return 2;
    }

  return 0;
}


#ifdef DDXTIME /* from ServerOSDefines */
CARD32
GetTimeInMillis (void)
{
  return GetTickCount ();
}
#endif /* DDXTIME */


/* See Porting Layer Definition - p. 20 */
/*
 * Do any global initialization, then initialize each screen.
 * 
 * NOTE: We use ddxProcessArgument, so we don't need to touch argc and argv
 */

void
InitOutput (ScreenInfo *screenInfo, int argc, char *argv[])
{
  int		i;
  int		iMaxConsecutiveScreen = 0;

#if CYGDEBUG
  ErrorF ("InitOutput\n");
#endif

  /* Try to read the XF86Config-style configuration file */
  if (!winReadConfigfile ())
    ErrorF ("InitOutput - Error reading config file\n");

  /* Setup global screen info parameters */
  screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
  screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
  screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
  screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;
  screenInfo->numPixmapFormats = NUMFORMATS;
  
  /* Describe how we want common pixmap formats padded */
  for (i = 0; i < NUMFORMATS; i++)
    {
      screenInfo->formats[i] = g_PixmapFormats[i];
    }

  /* Load pointers to DirectDraw functions */
  winGetDDProcAddresses ();
  
  /* Detect supported engines */
  winDetectSupportedEngines ();

  /* Load common controls library */
  g_hmodCommonControls = LoadLibraryEx ("comctl32.dll", NULL, 0);

  /* Load TrackMouseEvent function pointer */  
  g_fpTrackMouseEvent = GetProcAddress (g_hmodCommonControls,
					 "_TrackMouseEvent");
  if (g_fpTrackMouseEvent == NULL)
    {
      ErrorF ("InitOutput - Could not get pointer to function\n"
	      "\t_TrackMouseEvent in comctl32.dll.  Try installing\n"
	      "\tInternet Explorer 3.0 or greater if you have not\n"
	      "\talready.\n");

      /* Free the library since we won't need it */
      FreeLibrary (g_hmodCommonControls);
      g_hmodCommonControls = NULL;

      /* Set function pointer to point to no operation function */
      g_fpTrackMouseEvent = (FARPROC) (void (*)())NoopDDA;
    }

  /*
   * Check for a malformed set of -screen parameters.
   * Examples of malformed parameters:
   *	XWin -screen 1
   *	XWin -screen 0 -screen 2
   *	XWin -screen 1 -screen 2
   */
  for (i = 0; i < MAXSCREENS; i++)
    {
      if (g_ScreenInfo[i].fExplicitScreen)
	iMaxConsecutiveScreen = i + 1;
    }
  ErrorF ("InitOutput - g_iNumScreens: %d iMaxConsecutiveScreen: %d\n",
	  g_iNumScreens, iMaxConsecutiveScreen);
  if (g_iNumScreens < iMaxConsecutiveScreen)
    FatalError ("InitOutput - Malformed set of screen parameter(s).  "
		"Screens must be specified consecutively starting with "
		"screen 0.  That is, you cannot have only a screen 1, nor "
		"could you have screen 0 and screen 2.  You instead must have "
		"screen 0, or screen 0 and screen 1, respectively.  Of "
		"you can specify as many screens as you want from 0 up to "
		"%d.\n", MAXSCREENS - 1);

  /* Store the instance handle */
  g_hInstance = GetModuleHandle (NULL);

  /* Initialize each screen */
  for (i = 0; i < g_iNumScreens; i++)
    {
      /* Initialize the screen */
      if (-1 == AddScreen (winScreenInit, argc, argv))
	{
	  FatalError ("InitOutput - Couldn't add screen %d", i);
	}
    }

  LoadPreferences();

#if CYGDEBUG || YES
  ErrorF ("InitOutput - Returning.\n");
#endif
}
