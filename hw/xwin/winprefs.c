/*
 * Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project
 * shall not be used in advertising or otherwise to promote the sale, use
 * or other dealings in this Software without prior written authorization
 * from the XFree86 Project.
 *
 * Authors:     Earle F. Philhower, III
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winprefs.c,v 1.1 2003/10/02 13:30:11 eich Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include "win.h"

/* Fixups to prevent collisions between Windows and X headers */
#define ATOM DWORD
#include <windows.h>

#include "winprefs.h"
#include "winmultiwindowclass.h"

/* Where will the custom menu commands start counting from? */
#define STARTMENUID WM_USER

/* From winmultiwindowflex.l, the real parser */
extern void parse_file (FILE *fp);

/* From winmultiwindowyacc.y, the pref structure loaded by the parser */
extern WINMULTIWINDOWPREFS pref;

/* The global X default icon */
extern HICON		g_hiconX;

/* Currently in use command ID, incremented each new menu item created */
static int g_cmdid = STARTMENUID;


/* Defined in DIX */
extern char *display;

/*
 * Creates or appends a menu from a MENUPARSED structure
 */
static HMENU
MakeMenu (char *name,
	  HMENU editMenu,
	  int editItem)
{
  int i;
  int item;
  MENUPARSED *m;
  HMENU hmenu;

  for (i=0; i<pref.menuItems; i++)
    {
      if (!strcmp(name, pref.menu[i].menuName))
	break;
    }
  
  /* Didn't find a match, bummer */
  if (i==pref.menuItems)
    {
      ErrorF("MakeMenu: Can't find menu %s\n", name);
      return NULL;
    }
  
  m = &(pref.menu[i]);

  if (editMenu)
    {
      hmenu = editMenu;
      item = editItem;
    }
  else
    {
      hmenu = CreatePopupMenu();
      item = 0;
    }

  /* Add the menu items */
  for (i=0; i<m->menuItems; i++)
    {
      /* Only assign IDs one time... */
      if ( m->menuItem[i].commandID == 0 )
	m->menuItem[i].commandID = g_cmdid++;

      switch (m->menuItem[i].cmd)
	{
	case CMD_EXEC:
	case CMD_ALWAYSONTOP:
	case CMD_RELOAD:
	  InsertMenu (hmenu,
		      item,
		      MF_BYPOSITION|MF_ENABLED|MF_STRING,
		      m->menuItem[i].commandID,
		      m->menuItem[i].text);
	  break;
	  
	case CMD_SEPARATOR:
	  InsertMenu (hmenu,
		      item,
		      MF_BYPOSITION|MF_SEPARATOR,
		      0,
		      NULL);
	  break;
	  
	case CMD_MENU:
	  /* Recursive! */
	  InsertMenu (hmenu,
		      item,
		      MF_BYPOSITION|MF_POPUP|MF_ENABLED|MF_STRING,
		      (UINT_PTR)MakeMenu (m->menuItem[i].param, 0, 0),
		      m->menuItem[i].text);
	  break;
	}

      /* If item==-1 (means to add at end of menu) don't increment) */
      if (item>=0)
	item++;
    }

  return hmenu;
}


/*
 * Callback routine that is executed once per window class.
 * Removes or creates custom window settings depending on LPARAM
 */
static BOOL CALLBACK
ReloadEnumWindowsProc (HWND hwnd, LPARAM lParam)
{
  char    szClassName[1024];
  HICON   hicon;

  if (!GetClassName (hwnd, szClassName, 1024))
    return TRUE;

  if (strncmp (szClassName, WINDOW_CLASS_X, strlen (WINDOW_CLASS_X)))
    /* Not one of our windows... */
    return TRUE;
  
  /* It's our baby, either clean or dirty it */
  if (lParam==FALSE) 
    {
      hicon = (HICON)GetClassLong(hwnd, GCL_HICON);

      /* Unselect any icon in the class structure */
      SetClassLong (hwnd, GCL_HICON, (LONG)LoadIcon (NULL, IDI_APPLICATION));

      /* If it's generated on-the-fly, get rid of it, will regen */
      if (!winIconIsOverride((unsigned long)hicon) && hicon!=g_hiconX)
	DestroyIcon (hicon);
      
      /* Remove any menu additions, use bRevert flag */
      GetSystemMenu (hwnd, TRUE);
      
      /* This window is now clean of our taint */
    }
  else
    {
      /* Make the icon default, dynamic, of from xwinrc */
      SetClassLong (hwnd, GCL_HICON, (LONG)g_hiconX);
      winUpdateIcon ((Window)GetProp (hwnd, WIN_WID_PROP));
      /* Update the system menu for this window */
      SetupSysMenu ((unsigned long)hwnd);

      /* That was easy... */
    }

  return TRUE;
}


/*
 * Removes any custom icons in classes, custom menus, etc.
 * Frees all members in pref structure.
 * Reloads the preferences file.
 * Set custom icons and menus again.
 */
static void
ReloadPrefs ()
{
  int i;

  /* First, iterate over all windows replacing their icon with system */
  /* default one and deleting any custom system menus                 */
  EnumWindows (ReloadEnumWindowsProc, FALSE);
  
  /* Now, free/clear all info from our prefs structure */
  for (i=0; i<pref.menuItems; i++)
    free (pref.menu[i].menuItem);
  free (pref.menu);
  pref.menu = NULL;
  pref.menuItems = 0;

  pref.rootMenuName[0] = 0;

  free (pref.sysMenu);
  pref.sysMenuItems = 0;

  pref.defaultSysMenuName[0] = 0;
  pref.defaultSysMenuPos = 0;

  pref.iconDirectory[0] = 0;
  pref.defaultIconName[0] = 0;

  for (i=0; i<pref.iconItems; i++)
    if (pref.icon[i].hicon)
      DestroyIcon ((HICON)pref.icon[i].hicon);
  free (pref.icon);
  pref.icon = NULL;
  pref.iconItems = 0;
  
  /* Free global default X icon */
  DestroyIcon (g_hiconX);

  /* Reset the custom command IDs */
  g_cmdid = STARTMENUID;

  /* Load the updated resource file */
  LoadPreferences();

  /* Define global icon, load it */
  g_hiconX = (HICON)winOverrideDefaultIcon();
  if (!g_hiconX)
    g_hiconX = LoadIcon (g_hInstance, MAKEINTRESOURCE(IDI_XWIN));
  
  /* Rebuild the icons and menus */
  EnumWindows (ReloadEnumWindowsProc, TRUE);

  /* Whew, done */
}

/*
 * Check/uncheck the ALWAYSONTOP items in this menu
 */
void
HandleCustomWM_INITMENU(unsigned long hwndIn,
			unsigned long hmenuIn)
{
  HWND    hwnd;
  HMENU   hmenu;
  DWORD   dwExStyle;
  int     i, j;

  hwnd = (HWND)hwndIn;
  hmenu = (HMENU)hmenuIn;
  if (!hwnd || !hmenu) 
    return;
  
  if (GetWindowLong (hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
    dwExStyle = MF_BYCOMMAND | MF_CHECKED;
  else
    dwExStyle = MF_BYCOMMAND | MF_UNCHECKED;

  for (i=0; i<pref.menuItems; i++)
    for (j=0; j<pref.menu[i].menuItems; j++)
      if (pref.menu[i].menuItem[j].cmd==CMD_ALWAYSONTOP)
	CheckMenuItem (hmenu, pref.menu[i].menuItem[j].commandID, dwExStyle );
  
}
    
/*
 * Searches for the custom WM_COMMAND command ID and performs action
 */
int
HandleCustomWM_COMMAND (unsigned long hwndIn,
			int           command)
{
  HWND hwnd;
  int i, j;
  MENUPARSED *m;
  DWORD			dwExStyle;

  hwnd = (HWND)hwndIn;

  if (!command)
    return 0;

  for (i=0; i<pref.menuItems; i++)
    {
      m = &(pref.menu[i]);
      for (j=0; j<m->menuItems; j++)
	{
	  if (command==m->menuItem[j].commandID)
	    {
	      /* Match! */
	      switch(m->menuItem[j].cmd)
		{
		case CMD_EXEC:
		  if (fork()==0)
		    {
		      struct rlimit rl;
		      unsigned long i;

		      /* Close any open descriptors except for STD* */
		      getrlimit (RLIMIT_NOFILE, &rl);
		      for (i = STDERR_FILENO+1; i < rl.rlim_cur; i++)
			close(i);

		      /* Disassociate any TTYs */
		      setsid();

		      execl ("/bin/sh",
			     "/bin/sh",
			     "-c",
			     m->menuItem[j].param,
			     NULL);
		      exit (0);
		    }
		  else
		    return 0;
		  break;
		  
		case CMD_ALWAYSONTOP:
		  if (!hwnd)
		    return 0;

		  /* Get extended window style */
		  dwExStyle = GetWindowLong (hwnd, GWL_EXSTYLE);
		  
		  /* Handle topmost windows */
		  if (dwExStyle & WS_EX_TOPMOST)
		    SetWindowPos (hwnd,
				  HWND_NOTOPMOST,
				  0, 0,
				  0, 0,
				  SWP_NOSIZE | SWP_NOMOVE);
		  else
		    SetWindowPos (hwnd,
				  HWND_TOPMOST,
				  0, 0,
				  0, 0,
				  SWP_NOSIZE | SWP_NOMOVE);
		  return 0;
		  
		case CMD_RELOAD:
		  ReloadPrefs();
		  return 0;

		default:
		  return 0;
	      }
	    } /* match */
	} /* for j */
    } /* for i */

  return 0;
}


/*
 * Add the default or a custom menu depending on the class match
 */
void
SetupSysMenu (unsigned long hwndIn)
{
  HWND    hwnd;
  HMENU	  sys;
  int     i;
  WindowPtr pWin;
  char *res_name, *res_class;

  hwnd = (HWND)hwndIn;
  if (!hwnd)
    return;

  pWin = GetProp (hwnd, WIN_WINDOW_PROP);
  
  sys = GetSystemMenu (hwnd, FALSE);
  if (!sys)
    return;

  if (pWin)
    {
      /* First see if there's a class match... */
      if (winMultiWindowGetClassHint (pWin, &res_name, &res_class))
	{
	  for (i=0; i<pref.sysMenuItems; i++)
	    {
	      if (!strcmp(pref.sysMenu[i].match, res_name) ||
		  !strcmp(pref.sysMenu[i].match, res_class) ) 
		{
		  free(res_name);
		  free(res_class);
  
		  MakeMenu (pref.sysMenu[i].menuName, sys,
			    pref.sysMenu[i].menuPos==AT_START?0:-1);
		  return;
		}
	    }
	  
	  /* No match, just free alloc'd strings */
	  free(res_name);
	  free(res_class);
	} /* Found wm_class */
    } /* if pwin */

  /* Fallback to system default */
  if (pref.defaultSysMenuName[0])
    {
      if (pref.defaultSysMenuPos==AT_START)
	MakeMenu (pref.defaultSysMenuName, sys, 0);
      else
	MakeMenu (pref.defaultSysMenuName, sys, -1);
    }
}


/*
 * Possibly add a menu to the toolbar icon
 */
void
SetupRootMenu (unsigned long hmenuRoot)
{
  HMENU root;

  root = (HMENU)hmenuRoot;
  if (!root)
    return;

  if (pref.rootMenuName[0])
    {
      MakeMenu(pref.rootMenuName, root, 0);
    }
}


/*
 * Check for and return an overridden default ICON specified in the prefs
 */
unsigned long
winOverrideDefaultIcon()
{
  HICON hicon;
  char fname[PATH_MAX+NAME_MAX+2];
  
  if (pref.defaultIconName[0])
    {
      /* Make sure we have a dir with trailing backslash */
      /* Note we are using _Windows_ paths here, not cygwin */
      strcpy (fname, pref.iconDirectory);
      if (pref.iconDirectory[0])
	if (fname[strlen(fname)-1]!='\\')
	  strcat (fname, "\\");
      strcat (fname, pref.defaultIconName);

      hicon = (HICON)LoadImage(NULL,
			       fname,
			       IMAGE_ICON,
			       0, 0,
			       LR_DEFAULTSIZE|LR_LOADFROMFILE);
      if (hicon==NULL)
	ErrorF ("winOverrideDefaultIcon: LoadIcon(%s) failed\n", fname);

      return (unsigned long)hicon;
    }

  return 0;
}


/*
 * Check for a match of the window class to one specified in the
 * ICONS{} section in the prefs file, and load the icon from a file
 */
unsigned long
winOverrideIcon (unsigned long longWin)
{
  WindowPtr pWin = (WindowPtr) longWin;
  char *res_name, *res_class;
  int i;
  HICON hicon;
  char fname[PATH_MAX+NAME_MAX+2];
  char *wmName;

  if (pWin==NULL)
    return 0;

  /* If we can't find the class, we can't override from default! */
  if (!winMultiWindowGetClassHint (pWin, &res_name, &res_class))
    return 0;

  winMultiWindowGetWMName (pWin, &wmName);
  
  for (i=0; i<pref.iconItems; i++) {
    if (!strcmp(pref.icon[i].match, res_name) ||
	!strcmp(pref.icon[i].match, res_class) ||
	(wmName && strstr(wmName, pref.icon[i].match))) 
      {
	free (res_name);
	free (res_class);
	if (wmName)
	  free (wmName);

	if (pref.icon[i].hicon)
	  return pref.icon[i].hicon;

	/* Make sure we have a dir with trailing backslash */
	/* Note we are using _Windows_ paths here, not cygwin */
	strcpy (fname, pref.iconDirectory);
	if (pref.iconDirectory[0])
	  if (fname[strlen(fname)-1]!='\\')
	    strcat (fname, "\\");
	strcat (fname, pref.icon[i].iconFile);

	hicon = (HICON)LoadImage(NULL,
				 fname,
				 IMAGE_ICON,
				 0, 0,
				 LR_DEFAULTSIZE|LR_LOADFROMFILE);
	if (hicon==NULL)
	  ErrorF ("winOverrideIcon: LoadIcon(%s) failed\n", fname);

	pref.icon[i].hicon = (unsigned long)hicon;
	return (unsigned long)hicon;
      }
  }
  
  /* Didn't find the icon, fail gracefully */
  free (res_name);
  free (res_class);
  if (wmName)
    free (wmName);

  return 0;
}


/*
 * Should we free this icon or leave it in memory (is it part of our
 * ICONS{} overrides)?
 */
int
winIconIsOverride(unsigned hiconIn)
{
  HICON hicon;
  int i;

  hicon = (HICON)hiconIn;

  if (!hicon)
    return 0;
  
  for (i=0; i<pref.iconItems; i++)
    if ((HICON)pref.icon[i].hicon == hicon)
      return 1;
  
  return 0;
}



/*
 * Try and open ~/.XWinrc and /usr/X11R6/lib/X11/system.XWinrc
 * Load it into prefs structure for use by other functions
 */
void
LoadPreferences ()
{
  char *home;
  char fname[PATH_MAX+NAME_MAX+2];
  FILE *prefFile;
  char szDisplay[512];
  char *szEnvDisplay;
  int i, j;
  char param[PARAM_MAX+1];
  char *srcParam, *dstParam;

  /* First, clear all preference settings */
  memset (&pref, 0, sizeof(pref));
  prefFile = NULL;

  /* Now try and find a ~/.xwinrc file */
  home = getenv ("HOME");
  if (home)
    {
      strcpy (fname, home);
      if (fname[strlen(fname)-1]!='/')
	strcat (fname, "/");
      strcat (fname, ".XWinrc");
      
      prefFile = fopen (fname, "r");
    }

  /* No home file found, check system default */
  if (!prefFile)
    prefFile = fopen (PROJECTROOT"/lib/X11/system.XWinrc", "r");

  /* If we could open it, then read the settings and close it */
  if (prefFile)
    {
      parse_file (prefFile);
      fclose (prefFile);
    }

  /* Setup a DISPLAY environment variable, need to allocate on heap */
  /* because putenv doesn't copy the argument... */
  snprintf (szDisplay, 512, "DISPLAY=127.0.0.1:%s.0", display);
  szEnvDisplay = (char *)(malloc (strlen(szDisplay)+1));
  if (szEnvDisplay)
    {
      strcpy (szEnvDisplay, szDisplay);
      putenv (szEnvDisplay);
    }

  /* Replace any "%display%" in menu commands with display string */
  snprintf (szDisplay, 512, "127.0.0.1:%s.0", display);
  for (i=0; i<pref.menuItems; i++)
    {
      for (j=0; j<pref.menu[i].menuItems; j++)
	{
	  if (pref.menu[i].menuItem[j].cmd==CMD_EXEC)
	    {
	      srcParam = pref.menu[i].menuItem[j].param;
	      dstParam = param;
	      while (*srcParam) {
		if (!strncmp(srcParam, "%display%", 9))
		  {
		    memcpy (dstParam, szDisplay, strlen(szDisplay));
		    dstParam += strlen(szDisplay);
		    srcParam += 9;
		  }
		else
		  {
		    *dstParam = *srcParam;
		    dstParam++;
		    srcParam++;
		  }
	      }
	      *dstParam = 0;
	      strcpy (pref.menu[i].menuItem[j].param, param);
	    } /* cmd==cmd_exec */
	} /* for all menuitems */
    } /* for all menus */

}

