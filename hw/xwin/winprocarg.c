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

#include "win.h"
#include "winconfig.h"
#include "winprefs.h"
#include "winmsg.h"

/*
 * References to external symbols
 */

extern int			g_iNumScreens;
extern winScreenInfo		g_ScreenInfo[];
extern int			g_iLastScreen;
extern Bool			g_fInitializedDefaultScreens;
#ifdef XWIN_CLIPBOARD
extern Bool			g_fUnicodeClipboard;
extern Bool			g_fClipboard;
#endif
extern int			g_iLogVerbose;
extern char *			g_pszLogFile;
extern Bool			g_fXdmcpEnabled;
extern char *			g_pszCommandLine;
extern Bool			g_fKeyboardHookLL;
extern Bool			g_fNoHelpMessageBox;                     
extern Bool			g_fSoftwareCursor;
extern Bool			g_fSilentDupError;


/*
 * Function prototypes
 */

void
winLogCommandLine (int argc, char *argv[]);

void
winLogVersionInfo (void);

#ifdef DDXOSVERRORF
void OsVendorVErrorF (const char *pszFormat, va_list va_args);
#endif

void
winInitializeDefaultScreens (void);


/*
 * Process arguments on the command line
 */

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

  winErrorFVerb (2, "winInitializeDefaultScreens - w %d h %d\n",
	  (int) dwWidth, (int) dwHeight);

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
#ifdef XWIN_EMULATEPSEUDO
      g_ScreenInfo[i].fEmulatePseudo = WIN_DEFAULT_EMULATE_PSEUDO;
#endif
      g_ScreenInfo[i].dwRefreshRate = WIN_DEFAULT_REFRESH;
      g_ScreenInfo[i].pfb = NULL;
      g_ScreenInfo[i].fFullScreen = FALSE;
      g_ScreenInfo[i].fDecoration = TRUE;
#ifdef XWIN_MULTIWINDOWEXTWM
      g_ScreenInfo[i].fMWExtWM = FALSE;
#endif
      g_ScreenInfo[i].fRootless = FALSE;
#ifdef XWIN_MULTIWINDOW
      g_ScreenInfo[i].fMultiWindow = FALSE;
#endif
      g_ScreenInfo[i].fMultipleMonitors = FALSE;
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

  winErrorFVerb (2, "winInitializeDefaultScreens - Returning\n");
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

      /* Initialize only if option is not -help */
      if (!IS_OPTION("-help") && !IS_OPTION("-h") && !IS_OPTION("--help") &&
          !IS_OPTION("-version") && !IS_OPTION("--version"))
	{

          /* Log the version information */
          winLogVersionInfo ();

          /* Log the command line */
          winLogCommandLine (argc, argv);

	  /*
	   * Initialize default screen settings.  We have to do this before
	   * OsVendorInit () gets called, otherwise we will overwrite
	   * settings changed by parameters such as -fullscreen, etc.
	   */
	  winErrorFVerb (2, "ddxProcessArgument - Initializing default "
			 "screens\n");
	  winInitializeDefaultScreens ();
	}
    }

#if CYGDEBUG
  winDebug ("ddxProcessArgument - arg: %s\n", argv[i]);
#endif

  /*
   * Look for the '-help' and similar options
   */ 
  if (IS_OPTION ("-help") || IS_OPTION("-h") || IS_OPTION("--help"))
    {
      /* Reset logfile. We don't need that helpmessage in the logfile */  
      g_pszLogFile = NULL;
      g_fNoHelpMessageBox = TRUE;
      UseMsg();
      exit (0);
      return 1;
    }

  if (IS_OPTION ("-version") || IS_OPTION("--version"))
    {
      /* Reset logfile. We don't need that versioninfo in the logfile */  
      g_pszLogFile = NULL;
      winLogVersionInfo ();
      exit (0);
      return 1;
    }

  /*
   * Look for the '-screen scr_num [width height]' argument
   */
  if (IS_OPTION ("-screen"))
    {
      int		iArgsProcessed = 1;
      int		nScreenNum;
      int		iWidth, iHeight;

#if CYGDEBUG
      winDebug ("ddxProcessArgument - screen - argc: %d i: %d\n",
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
	  winErrorFVerb (2, "ddxProcessArgument - screen - Found ``WxD'' arg\n");
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
	  winErrorFVerb (2, "ddxProcessArgument - screen - Found ``W D'' arg\n");
	  iArgsProcessed = 4;
	  g_ScreenInfo[nScreenNum].fUserGaveHeightAndWidth = TRUE;
	  g_ScreenInfo[nScreenNum].dwWidth = iWidth;
	  g_ScreenInfo[nScreenNum].dwHeight = iHeight;
	  g_ScreenInfo[nScreenNum].dwUserWidth = iWidth;
	  g_ScreenInfo[nScreenNum].dwUserHeight = iHeight;
	}
      else
	{
	  winErrorFVerb (2, "ddxProcessArgument - screen - Did not find size arg. "
		  "dwWidth: %d dwHeight: %d\n",
		  (int) g_ScreenInfo[nScreenNum].dwWidth,
		  (int) g_ScreenInfo[nScreenNum].dwHeight);
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
  if (IS_OPTION ("-engine"))
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
  if (IS_OPTION ("-fullscreen"))
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fFullScreen = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fFullScreen = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }

  /*
   * Look for the '-lesspointer' argument
   */
  if (IS_OPTION ("-lesspointer"))
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
  if (IS_OPTION ("-nodecoration"))
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

#ifdef XWIN_MULTIWINDOWEXTWM
  /*
   * Look for the '-mwextwm' argument
   */
  if (IS_OPTION ("-mwextwm"))
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fMWExtWM = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fMWExtWM = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }
#endif

  /*
   * Look for the '-rootless' argument
   */
  if (IS_OPTION ("-rootless"))
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

#ifdef XWIN_MULTIWINDOW
  /*
   * Look for the '-multiwindow' argument
   */
  if (IS_OPTION ("-multiwindow"))
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
#endif

  /*
   * Look for the '-multiplemonitors' argument
   */
  if (IS_OPTION ("-multiplemonitors")
      || IS_OPTION ("-multimonitors"))
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
  if (IS_OPTION ("-scrollbars"))
    {
      /* Is this parameter attached to a screen or is it global? */
      if (-1 == g_iLastScreen)
	{
	  int			j;

	  /* Parameter is for all screens */
	  for (j = 0; j < MAXSCREENS; j++)
	    {
	      g_ScreenInfo[j].fScrollbars = TRUE;
	    }
	}
      else
	{
	  /* Parameter is for a single screen */
	  g_ScreenInfo[g_iLastScreen].fScrollbars = TRUE;
	}

      /* Indicate that we have processed this argument */
      return 1;
    }


#ifdef XWIN_CLIPBOARD
  /*
   * Look for the '-clipboard' argument
   */
  if (IS_OPTION ("-clipboard"))
    {
      g_fClipboard = TRUE;

      /* Indicate that we have processed this argument */
      return 1;
    }
#endif


  /*
   * Look for the '-ignoreinput' argument
   */
  if (IS_OPTION ("-ignoreinput"))
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
  if (IS_OPTION ("-emulate3buttons"))
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
  if (IS_OPTION ("-depth"))
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
  if (IS_OPTION ("-refresh"))
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
  if (IS_OPTION ("-clipupdates"))
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

#ifdef XWIN_EMULATEPSEUDO
  /*
   * Look for the '-emulatepseudo' argument
   */
  if (IS_OPTION ("-emulatepseudo"))
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
#endif

  /*
   * Look for the '-nowinkill' argument
   */
  if (IS_OPTION ("-nowinkill"))
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
  if (IS_OPTION ("-winkill"))
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
  if (IS_OPTION ("-nounixkill"))
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
  if (IS_OPTION ("-unixkill"))
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
  if (IS_OPTION ("-notrayicon"))
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
  if (IS_OPTION ("-trayicon"))
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
      g_fXdmcpEnabled = TRUE;
      g_pszQueryHost = argv[++i];
      return 0; /* Let DIX parse this again */
    }

  /*
   * Look for the '-indirect' or '-broadcast' arguments
   */
  if (IS_OPTION ("-indirect")
      || IS_OPTION ("-broadcast"))
    {
      g_fXdmcpEnabled = TRUE;
      return 0; /* Let DIX parse this again */
    }

  /*
   * Look for the '-config' argument
   */
  if (IS_OPTION ("-config")
      || IS_OPTION ("-xf86config"))
    {
      CHECK_ARGS (1);
#ifdef XWIN_XF86CONFIG
      g_cmdline.configFile = argv[++i];
#else
      winMessageBoxF ("The %s option is not supported in this "
		      "release.\n"
		      "Ignoring this option and continuing.\n",
		      MB_ICONINFORMATION,
		      argv[i]);
#endif
      return 2;
    }

  /*
   * Look for the '-keyboard' argument
   */
  if (IS_OPTION ("-keyboard"))
    {
#ifdef XWIN_XF86CONFIG
      CHECK_ARGS (1);
      g_cmdline.keyboard = argv[++i];
#else
      winMessageBoxF ("The -keyboard option is not supported in this "
		      "release.\n"
		      "Ignoring this option and continuing.\n",
		      MB_ICONINFORMATION);
#endif
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

#ifdef XWIN_CLIPBOARD
  /*
   * Look for the '-nounicodeclipboard' argument
   */
  if (IS_OPTION ("-nounicodeclipboard"))
    {
      g_fUnicodeClipboard = FALSE;
      /* Indicate that we have processed the argument */
      return 1;
    }
#endif

#ifdef XKB
  /*
   * Look for the '-kb' argument
   */
  if (IS_OPTION ("-kb"))
    {
      g_cmdline.noXkbExtension = TRUE;  
      return 0; /* Let DIX parse this again */
    }

  if (IS_OPTION ("-xkbrules"))
    {
      CHECK_ARGS (1);
      g_cmdline.xkbRules = argv[++i];
      return 2;
    }
  if (IS_OPTION ("-xkbmodel"))
    {
      CHECK_ARGS (1);
      g_cmdline.xkbModel = argv[++i];
      return 2;
    }
  if (IS_OPTION ("-xkblayout"))
    {
      CHECK_ARGS (1);
      g_cmdline.xkbLayout = argv[++i];
      return 2;
    }
  if (IS_OPTION ("-xkbvariant"))
    {
      CHECK_ARGS (1);
      g_cmdline.xkbVariant = argv[++i];
      return 2;
    }
  if (IS_OPTION ("-xkboptions"))
    {
      CHECK_ARGS (1);
      g_cmdline.xkbOptions = argv[++i];
      return 2;
    }
#endif

  if (IS_OPTION ("-keyhook"))
    {
      g_fKeyboardHookLL = TRUE;
      return 1;
    }
  
  if (IS_OPTION ("-nokeyhook"))
    {
      g_fKeyboardHookLL = FALSE;
      return 1;
    }
  
  if (IS_OPTION ("-swcursor"))
    {
      g_fSoftwareCursor = TRUE;
      return 1;
    }
  
  if (IS_OPTION ("-silent-dup-error"))
    {
      g_fSilentDupError = TRUE;
      return 1;
    }
  return 0;
}


/*
 * winLogCommandLine - Write entire command line to the log file
 */

void
winLogCommandLine (int argc, char *argv[])
{
  int		i;
  int		iSize = 0;
  int		iCurrLen = 0;

#define CHARS_PER_LINE 60

  /* Bail if command line has already been logged */
  if (g_pszCommandLine)
    return;

  /* Count how much memory is needed for concatenated command line */
  for (i = 0, iCurrLen = 0; i < argc; ++i)
    if (argv[i])
      {
	/* Add a character for lines that overflow */
	if ((strlen (argv[i]) < CHARS_PER_LINE
	    && iCurrLen + strlen (argv[i]) > CHARS_PER_LINE)
	    || strlen (argv[i]) > CHARS_PER_LINE)
	  {
	    iCurrLen = 0;
	    ++iSize;
	  }
	
	/* Add space for item and trailing space */
	iSize += strlen (argv[i]) + 1;

	/* Update current line length */
	iCurrLen += strlen (argv[i]);
      }

  /* Allocate memory for concatenated command line */
  g_pszCommandLine = malloc (iSize + 1);
  if (!g_pszCommandLine)
    FatalError ("winLogCommandLine - Could not allocate memory for "
		"command line string.  Exiting.\n");
  
  /* Set first character to concatenated command line to null */
  g_pszCommandLine[0] = '\0';

  /* Loop through all args */
  for (i = 0, iCurrLen = 0; i < argc; ++i)
    {
      /* Add a character for lines that overflow */
      if ((strlen (argv[i]) < CHARS_PER_LINE
	   && iCurrLen + strlen (argv[i]) > CHARS_PER_LINE)
	  || strlen (argv[i]) > CHARS_PER_LINE)
      {
	iCurrLen = 0;
	
	/* Add line break if it fits */
	strncat (g_pszCommandLine, "\n", iSize - strlen (g_pszCommandLine));
      }
      
      strncat (g_pszCommandLine, argv[i], iSize - strlen (g_pszCommandLine));
      strncat (g_pszCommandLine, " ", iSize - strlen (g_pszCommandLine));

      /* Save new line length */
      iCurrLen += strlen (argv[i]);
    }

  ErrorF ("XWin was started with the following command line:\n\n"
	  "%s\n\n", g_pszCommandLine);
}


/*
 * winLogVersionInfo - Log Cygwin/X version information
 */

void
winLogVersionInfo (void)
{
  static Bool		s_fBeenHere = FALSE;

  if (s_fBeenHere)
    return;
  s_fBeenHere = TRUE;

  ErrorF ("Welcome to the XWin X Server\n");
  ErrorF ("Vendor: %s\n", VENDOR_STRING);
  ErrorF ("Release: %s\n\n", VERSION_STRING);
  ErrorF ("Contact: %s\n\n", VENDOR_CONTACT);
}
