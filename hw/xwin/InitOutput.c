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
/* $XFree86: xc/programs/Xserver/hw/xwin/InitOutput.c,v 1.34 2003/10/02 13:30:09 eich Exp $ */

#include "win.h"
#include "winmsg.h"
#ifdef XWIN_XF86CONFIG
#include "winconfig.h"
#endif
#include "winprefs.h"
#include "X11/Xlocale.h"
#include <mntent.h>


/*
 * References to external symbols
 */

extern int			g_iNumScreens;
extern winScreenInfo		g_ScreenInfo[];
extern int			g_iLastScreen;
extern char *			g_pszCommandLine;
extern Bool			g_fSilentFatalError;

extern char *			g_pszLogFile;
extern int			g_iLogVerbose;
Bool				g_fLogInited;

extern Bool			g_fXdmcpEnabled;
extern int			g_fdMessageQueue;
extern const char *		g_pszQueryHost;
extern HINSTANCE		g_hInstance;

#ifdef XWIN_CLIPBOARD
extern Bool			g_fUnicodeClipboard;
extern Bool			g_fClipboardLaunched;
extern Bool			g_fClipboardStarted;
extern pthread_t		g_ptClipboardProc;
extern HWND			g_hwndClipboard;
extern Bool			g_fClipboard;
#endif

extern HMODULE			g_hmodDirectDraw;
extern FARPROC			g_fpDirectDrawCreate;
extern FARPROC			g_fpDirectDrawCreateClipper;
  
extern HMODULE			g_hmodCommonControls;
extern FARPROC			g_fpTrackMouseEvent;
extern Bool			g_fNoHelpMessageBox;                     
extern Bool			g_fSilentDupError;                     
  
  
/*
 * Function prototypes
 */

#ifdef XWIN_CLIPBOARD
static void
winClipboardShutdown (void);
#endif

#if defined(DDXOSVERRORF)
void
OsVendorVErrorF (const char *pszFormat, va_list va_args);
#endif

void
winInitializeDefaultScreens (void);

static Bool
winCheckDisplayNumber (void);

void
winLogCommandLine (int argc, char *argv[]);

void
winLogVersionInfo (void);

Bool
winValidateArgs (void);


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

#ifdef XWIN_CLIPBOARD
static void
winClipboardShutdown (void)
{
  /* Close down clipboard resources */
  if (g_fClipboard && g_fClipboardLaunched && g_fClipboardStarted)
    {
      /* Synchronously destroy the clipboard window */
      if (g_hwndClipboard != NULL)
	{
	  SendMessage (g_hwndClipboard, WM_DESTROY, 0, 0);
	  /* NOTE: g_hwndClipboard is set to NULL in winclipboardthread.c */
	}
      else
	return;
      
      /* Wait for the clipboard thread to exit */
      if (g_ptClipboardProc)
	{
	  pthread_join (g_ptClipboardProc, NULL);
	  g_ptClipboardProc = 0;
	}
      else
	return;

      g_fClipboardLaunched = FALSE;
      g_fClipboardStarted = FALSE;

      winDebug ("winClipboardShutdown - Clipboard thread has exited.\n");
    }
}
#endif


#if defined(DDXBEFORERESET)
/*
 * Called right before KillAllClients when the server is going to reset,
 * allows us to shutdown our seperate threads cleanly.
 */

void
ddxBeforeReset (void)
{
  winDebug ("ddxBeforeReset - Hello\n");

#ifdef XWIN_CLIPBOARD
  winClipboardShutdown ();
#endif
}
#endif


/* See Porting Layer Definition - p. 57 */
void
ddxGiveUp (void)
{
  int		i;

#if CYGDEBUG
  winDebug ("ddxGiveUp\n");
#endif

  /* Perform per-screen deinitialization */
  for (i = 0; i < g_iNumScreens; ++i)
    {
      /* Delete the tray icon */
      if (!g_ScreenInfo[i].fNoTrayIcon && g_ScreenInfo[i].pScreen)
 	winDeleteNotifyIcon (winGetScreenPriv (g_ScreenInfo[i].pScreen));
    }

#ifdef XWIN_MULTIWINDOW
  /* Notify the worker threads we're exiting */
  winDeinitMultiWindowWM ();
#endif

  /* Close our handle to our message queue */
  if (g_fdMessageQueue != WIN_FD_INVALID)
    {
      /* Close /dev/windows */
      close (g_fdMessageQueue);

      /* Set the file handle to invalid */
      g_fdMessageQueue = WIN_FD_INVALID;
    }

  if (!g_fLogInited) {
    LogInit (g_pszLogFile, NULL);
    g_fLogInited = TRUE;
  }  
  LogClose ();

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
      g_fpTrackMouseEvent = (FARPROC) (void (*)(void))NoopDDA;
    }
  
  /* Free concatenated command line */
  if (g_pszCommandLine)
    {
      free (g_pszCommandLine);
      g_pszCommandLine = NULL;
    }

  /* Remove our keyboard hook if it is installed */
  winRemoveKeyboardHookLL ();

  /* Tell Windows that we want to end the app */
  PostQuitMessage (0);
}


/* See Porting Layer Definition - p. 57 */
void
AbortDDX (void)
{
#if CYGDEBUG
  winDebug ("AbortDDX\n");
#endif
  ddxGiveUp ();
}

/* hasmntopt is currently not implemented for cygwin */
const char *winCheckMntOpt(const struct mntent *mnt, const char *opt)
{
    const char *s;
    size_t len;
    if (mnt == NULL)
        return NULL;
    if (opt == NULL)
        return NULL;
    if (mnt->mnt_opts == NULL)
        return NULL;

    len = strlen(opt);
    s = strstr(mnt->mnt_opts, opt);
    if (s == NULL)
        return NULL;
    if ((s == mnt->mnt_opts || *(s-1) == ',') &&  (s[len] == 0 || s[len] == ','))
        return (char *)opt;
    return NULL;
}

void
winCheckMount(void)
{
  FILE *mnt;
  struct mntent *ent;

  enum { none = 0, sys_root, user_root, sys_tmp, user_tmp } 
    level = none, curlevel;
  BOOL binary = TRUE;

  mnt = setmntent("/etc/mtab", "r");
  if (mnt == NULL)
  {
    ErrorF("setmntent failed");
    return;
  }

  while ((ent = getmntent(mnt)) != NULL)
  {
    BOOL system = (strcmp(ent->mnt_type, "system") == 0);
    BOOL root = (strcmp(ent->mnt_dir, "/") == 0);
    BOOL tmp = (strcmp(ent->mnt_dir, "/tmp") == 0);
    
    if (system)
    {
      if (root)
        curlevel = sys_root;
      else if (tmp)
        curlevel = sys_tmp;
      else
        continue;
    }
    else
    {
      if (root)
        curlevel = user_root;
      else if (tmp) 
        curlevel = user_tmp;
      else
        continue;
    }

    if (curlevel <= level)
      continue;
    level = curlevel;

    if (winCheckMntOpt(ent, "binmode") == NULL)
      binary = 0;
    else
      binary = 1;
  }
    
  if (endmntent(mnt) != 1)
  {
    ErrorF("endmntent failed");
    return;
  }
  
 if (!binary) 
   winMsg(X_WARNING, "/tmp mounted int textmode\n"); 
}


void
OsVendorInit (void)
{
  /* Re-initialize global variables on server reset */
  winInitializeGlobals ();

#ifdef DDXOSVERRORF
  if (!OsVendorVErrorFProc)
    OsVendorVErrorFProc = OsVendorVErrorF;
#endif

  if (!g_fLogInited) {
    /* keep this order. If LogInit fails it calls Abort which then calls
     * ddxGiveUp where LogInit is called again and creates an infinite 
     * recursion. If we set g_fLogInited to TRUE before the init we 
     * avoid the second call 
     */  
    g_fLogInited = TRUE;
    LogInit (g_pszLogFile, NULL);
  }  
  LogSetParameter (XLOG_FLUSH, 1);
  LogSetParameter (XLOG_VERBOSITY, g_iLogVerbose);
  LogSetParameter (XLOG_FILE_VERBOSITY, 1);

  /* Log the version information */
  if (serverGeneration == 1)
    winLogVersionInfo ();

  winCheckMount();  

  /* Add a default screen if no screens were specified */
  if (g_iNumScreens == 0)
    {
      winDebug ("OsVendorInit - Creating bogus screen 0\n");

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


void
winUseMsg (void)
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
#ifdef XWIN_NATIVEGDI
	  "\t\t16 - Native GDI - experimental\n"
#endif
	  );

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
          "\tCygwin/X window.  This prevents ghost cursors appearing where\n"
	  "\tthe Windows cursor is drawn overtop of the X cursor\n");

  ErrorF ("-nodecoration\n"
          "\tDo not draw a window border, title bar, etc.  Windowed\n"
	  "\tmode only.\n");

#ifdef XWIN_MULTIWINDOWEXTWM
  ErrorF ("-mwextwm\n"
	  "\tRun the server in multi-window external window manager mode.\n");
#endif

  ErrorF ("-rootless\n"
	  "\tRun the server in rootless mode.\n");

#ifdef XWIN_MULTIWINDOW
  ErrorF ("-multiwindow\n"
	  "\tRun the server in multi-window mode.\n");
#endif

  ErrorF ("-multiplemonitors\n"
	  "\tEXPERIMENTAL: Use the entire virtual screen if multiple\n"
	  "\tmonitors are present.\n");

#ifdef XWIN_CLIPBOARD
  ErrorF ("-clipboard\n"
	  "\tRun the clipboard integration module.\n"
	  "\tDo not use at the same time as 'xwinclip'.\n");

  ErrorF ("-nounicodeclipboard\n"
	  "\tDo not use Unicode clipboard even if NT-based platform.\n");
#endif

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

#ifdef XWIN_EMULATEPSEUDO
  ErrorF ("-emulatepseudo\n"
	  "\tCreate a depth 8 PseudoColor visual when running in\n"
	  "\tdepths 15, 16, 24, or 32, collectively known as TrueColor\n"
	  "\tdepths.  The PseudoColor visual does not have correct colors,\n"
	  "\tand it may crash, but it at least allows you to run your\n"
	  "\tapplication in TrueColor modes.\n");
#endif

  ErrorF ("-[no]unixkill\n"
          "\tCtrl+Alt+Backspace exits the X Server.\n");

  ErrorF ("-[no]winkill\n"
          "\tAlt+F4 exits the X Server.\n");

#ifdef XWIN_XF86CONFIG
  ErrorF ("-config\n"
          "\tSpecify a configuration file.\n");

  ErrorF ("-keyboard\n"
	  "\tSpecify a keyboard device from the configuration file.\n");
#endif

#ifdef XKB
  ErrorF ("-xkbrules XKBRules\n"
	  "\tEquivalent to XKBRules in XF86Config files.\n");

  ErrorF ("-xkbmodel XKBModel\n"
	  "\tEquivalent to XKBModel in XF86Config files.\n");

  ErrorF ("-xkblayout XKBLayout\n"
	  "\tEquivalent to XKBLayout in XF86Config files.\n"
	  "\tFor example: -xkblayout de\n");

  ErrorF ("-xkbvariant XKBVariant\n"
	  "\tEquivalent to XKBVariant in XF86Config files.\n"
	  "\tFor example: -xkbvariant nodeadkeys\n");

  ErrorF ("-xkboptions XKBOptions\n"
	  "\tEquivalent to XKBOptions in XF86Config files.\n");
#endif

  ErrorF ("-logfile filename\n"
	  "\tWrite logmessages to <filename> instead of /tmp/Xwin.log.\n");

  ErrorF ("-logverbose verbosity\n"
	  "\tSet the verbosity of logmessages. [NOTE: Only a few messages\n"
	  "\trespect the settings yet]\n"
	  "\t\t0 - only print fatal error.\n"
	  "\t\t1 - print additional configuration information.\n"
	  "\t\t2 - print additional runtime information [default].\n"
	  "\t\t3 - print debugging and tracing information.\n");

  ErrorF ("-[no]keyhook\n"
	  "\tGrab special windows key combinations like Alt-Tab or the Menu "
          "key.\n These keys are discarded by default.\n");

  ErrorF ("-swcursor\n"
	  "\tDisable the usage of the windows cursor and use the X11 software "
	  "cursor instead\n");
}

/* See Porting Layer Definition - p. 57 */
void
ddxUseMsg(void)
{
  /* Set a flag so that FatalError won't give duplicate warning message */
  g_fSilentFatalError = TRUE;
  
  winUseMsg();  

  /* Log file will not be opened for UseMsg unless we open it now */
  if (!g_fLogInited) {
    LogInit (g_pszLogFile, NULL);
    g_fLogInited = TRUE;
  }  
  LogClose ();

  /* Notify user where UseMsg text can be found.*/
  if (!g_fNoHelpMessageBox)
    winMessageBoxF ("The Cygwin/X help text has been printed to "
		  "/tmp/XWin.log.\n"
		  "Please open /tmp/XWin.log to read the help text.\n",
		  MB_ICONINFORMATION);
}

/* ddxInitGlobals - called by |InitGlobals| from os/util.c */
void ddxInitGlobals(void)
{
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

  /* Log the command line */
  winLogCommandLine (argc, argv);

#if CYGDEBUG
  winDebug ("InitOutput\n");
#endif

  /* Validate command-line arguments */
  if (serverGeneration == 1 && !winValidateArgs ())
    {
      FatalError ("InitOutput - Invalid command-line arguments found.  "
		  "Exiting.\n");
    }

  /* Check for duplicate invocation on same display number.*/
  if (serverGeneration == 1 && !winCheckDisplayNumber ())
    {
      if (g_fSilentDupError)
        g_fSilentFatalError = TRUE;  
      FatalError ("InitOutput - Duplicate invocation on display "
		  "number: %s.  Exiting.\n", display);
    }

#ifdef XWIN_XF86CONFIG
  /* Try to read the xorg.conf-style configuration file */
  if (!winReadConfigfile ())
    winErrorFVerb (1, "InitOutput - Error reading config file\n");
#else
  winMsg(X_INFO, "XF86Config is not supported\n");
  winMsg(X_INFO, "See http://x.cygwin.com/docs/faq/cygwin-x-faq.html "
         "for more information\n");
#endif

  /* Load preferences from XWinrc file */
  LoadPreferences();

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
      winErrorFVerb (1, "InitOutput - Could not get pointer to function\n"
	      "\t_TrackMouseEvent in comctl32.dll.  Try installing\n"
	      "\tInternet Explorer 3.0 or greater if you have not\n"
	      "\talready.\n");

      /* Free the library since we won't need it */
      FreeLibrary (g_hmodCommonControls);
      g_hmodCommonControls = NULL;

      /* Set function pointer to point to no operation function */
      g_fpTrackMouseEvent = (FARPROC) (void (*)(void))NoopDDA;
    }

  /* Store the instance handle */
  g_hInstance = GetModuleHandle (NULL);

  /* Initialize each screen */
  for (i = 0; i < g_iNumScreens; ++i)
    {
      /* Initialize the screen */
      if (-1 == AddScreen (winScreenInit, argc, argv))
	{
	  FatalError ("InitOutput - Couldn't add screen %d", i);
	}
    }

#if defined(XWIN_CLIPBOARD) || defined(XWIN_MULTIWINDOW)

#if defined(XCSECURITY)
  /* Generate a cookie used by internal clients for authorization */
  if (g_fXdmcpEnabled)
    winGenerateAuthorization ();
#endif

  /* Perform some one time initialization */
  if (1 == serverGeneration)
    {
      /*
       * setlocale applies to all threads in the current process.
       * Apply locale specified in LANG environment variable.
       */
      setlocale (LC_ALL, "");
    }
#endif

#if CYGDEBUG || YES
  winDebug ("InitOutput - Returning.\n");
#endif
}


/*
 * winCheckDisplayNumber - Check if another instance of Cygwin/X is
 * already running on the same display number.  If no one exists,
 * make a mutex to prevent new instances from running on the same display.
 *
 * return FALSE if the display number is already used.
 */

static Bool
winCheckDisplayNumber ()
{
  int			nDisp;
  HANDLE		mutex;
  char			name[MAX_PATH];
  char *		pszPrefix = '\0';
  OSVERSIONINFO		osvi = {0};

  /* Check display range */
  nDisp = atoi (display);
  if (nDisp < 0 || nDisp > 65535)
    {
      ErrorF ("winCheckDisplayNumber - Bad display number: %d\n", nDisp);
      return FALSE;
    }

  /* Set first character of mutex name to null */
  name[0] = '\0';

  /* Get operating system version information */
  osvi.dwOSVersionInfoSize = sizeof (osvi);
  GetVersionEx (&osvi);

  /* Want a mutex shared among all terminals on NT > 4.0 */
  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT
      && osvi.dwMajorVersion >= 5)
    {
      pszPrefix = "Global\\";
    }

  /* Setup Cygwin/X specific part of name */
  sprintf (name, "%sCYGWINX_DISPLAY:%d", pszPrefix, nDisp);

  /* Windows automatically releases the mutex when this process exits */
  mutex = CreateMutex (NULL, FALSE, name);
  if (!mutex)
    {
      LPVOID lpMsgBuf;

      /* Display a fancy error message */
      FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		     FORMAT_MESSAGE_FROM_SYSTEM | 
		     FORMAT_MESSAGE_IGNORE_INSERTS,
		     NULL,
		     GetLastError (),
		     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		     (LPTSTR) &lpMsgBuf,
		     0, NULL);
      ErrorF ("winCheckDisplayNumber - CreateMutex failed: %s\n",
	      (LPSTR)lpMsgBuf);
      LocalFree (lpMsgBuf);

      return FALSE;
    }
  if (GetLastError () == ERROR_ALREADY_EXISTS)
    {
      ErrorF ("winCheckDisplayNumber - "
	      "Cygwin/X is already running on display %d\n",
	      nDisp);
      return FALSE;
    }

  return TRUE;
}
