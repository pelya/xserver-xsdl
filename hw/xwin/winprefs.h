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
/* $XFree86: xc/programs/Xserver/hw/xwin/winprefs.h,v 1.1 2003/10/02 13:30:11 eich Exp $ */

/* Need to know how long paths can be... */
#include <limits.h>

#ifndef NAME_MAX
#define NAME_MAX PATH_MAX
#endif
#define MENU_MAX 128   /* Maximum string length of a menu name or item */
#define PARAM_MAX (4*PATH_MAX)  /* Maximim length of a parameter to a MENU */


/* Supported commands in a MENU {} statement */
typedef enum MENUCOMMANDTYPE
{
  CMD_EXEC,         /* /bin/sh -c the parameter            */
  CMD_MENU,         /* Display a popup menu named param    */
  CMD_SEPARATOR,    /* Menu separator                      */
  CMD_ALWAYSONTOP,  /* Toggle always-on-top mode           */
  CMD_RELOAD        /* Reparse the .XWINRC file            */
} MENUCOMMANDTYPE;

/* Where to place a system menu */
typedef enum MENUPOSITION
{
  AT_START,   /* Place menu at the top of the system menu   */
  AT_END      /* Put it at the bottom of the menu (default) */
} MENUPOSITION;

/* Menu item definitions */
typedef struct MENUITEM
{
  char text[MENU_MAX+1];   /* To be displayed in menu */
  MENUCOMMANDTYPE cmd;     /* What should it do? */
  char param[PARAM_MAX+1]; /* Any parameters? */
  unsigned long commandID; /* Windows WM_COMMAND ID assigned at runtime */
} MENUITEM;

/* A completely read in menu... */
typedef struct MENUPARSED
{
  char menuName[MENU_MAX+1]; /* What's it called in the text? */
  MENUITEM *menuItem;        /* Array of items */
  int menuItems;             /* How big's the array? */
} MENUPARSED;

/* To map between a window and a system menu to add for it */
typedef struct SYSMENUITEM
{
  char match[MENU_MAX+1];    /* String to look for to apply this sysmenu */
  char menuName[MENU_MAX+1]; /* Which menu to show? Used to set *menu */
  MENUPOSITION menuPos;      /* Where to place it (ignored in root) */
} SYSMENUITEM;

/* To redefine icons for certain window types */
typedef struct ICONITEM
{
  char match[MENU_MAX+1];             /* What string to search for? */
  char iconFile[PATH_MAX+NAME_MAX+2]; /* Icon location, WIN32 path */
  unsigned long hicon;                /* LoadImage() result */
} ICONITEM;

typedef struct WINMULTIWINDOWPREFS
{
  /* Menu information */
  MENUPARSED *menu; /* Array of created menus */
  int menuItems;      /* How big? */

  /* Taskbar menu settings */
  char rootMenuName[MENU_MAX+1];  /* Menu for taskbar icon */

  /* System menu addition menus */
  SYSMENUITEM *sysMenu;
  int sysMenuItems;

  /* Which menu to add to unmatched windows? */
  char defaultSysMenuName[MENU_MAX+1];
  MENUPOSITION defaultSysMenuPos;   /* Where to place it */

  /* Icon information */
  char iconDirectory[PATH_MAX+1]; /* Where do the .icos lie? (Win32 path) */
  char defaultIconName[NAME_MAX+1];   /* Replacement for x.ico */

  ICONITEM *icon;
  int iconItems;

} WINMULTIWINDOWPREFS;




/* Functions */
void
LoadPreferences();

void
SetupRootMenu (unsigned long hmenuRoot);

void
SetupSysMenu (unsigned long hwndIn);

void
HandleCustomWM_INITMENU(unsigned long hwndIn,
			unsigned long hmenuIn);

int
HandleCustomWM_COMMAND (unsigned long hwndIn,
			int           command);

int
winIconIsOverride (unsigned hiconIn);

unsigned long
winOverrideIcon (unsigned long longpWin);

unsigned long
winOverrideDefaultIcon();

