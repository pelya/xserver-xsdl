/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors: Alexander Gottwald	
 */
/* $XFree86: xc/programs/Xserver/hw/xwin/winconfig.c,v 1.1 2002/10/17 08:18:22 alanh Exp $ */

#include "win.h"
#include "winconfig.h"
#include "winmsg.h"
#include "globals.h"

#ifdef XKB
#define XKB_IN_SERVER
#include "XKBsrv.h"
#endif

#ifndef CONFIGPATH
#define CONFIGPATH  "%A," "%R," \
                    "/etc/X11/%R," "%P/etc/X11/%R," \
                    "%E," "%F," \
                    "/etc/X11/%F," "%P/etc/X11/%F," \
                    "%D/%X," \
                    "/etc/X11/%X-%M," "/etc/X11/%X," "/etc/%X," \
                    "%P/etc/X11/%X.%H," "%P/etc/X11/%X-%M," \
                    "%P/etc/X11/%X," \
                    "%P/lib/X11/%X.%H," "%P/lib/X11/%X-%M," \
                    "%P/lib/X11/%X"
#endif

XF86ConfigPtr g_xf86configptr = NULL;
WinCmdlineRec g_cmdline = {
  NULL,				/* configFile */
  NULL,				/* fontPath */
  NULL,				/* rgbPath */
  NULL,				/* keyboard */
#ifdef XKB
  FALSE,			/* noXkbExtension */
  NULL,				/* xkbMap */
#endif
  NULL,				/* screenname */
  NULL,				/* mousename */
  FALSE,			/* emulate3Buttons */
  0				/* emulate3Timeout */
};

winInfoRec g_winInfo = {
  {				/* keyboard */
   0,				/* leds */
   500,				/* delay */
   30				/* rate */
#ifdef XKB
   }
  ,
  {				/* xkb */
   FALSE,			/* disable */
   NULL,			/* rules */
   NULL,			/* model */
   NULL,			/* layout */
   NULL,			/* variant */
   NULL,			/* options */
   NULL,			/* initialMap */
   NULL,			/* keymap */
   NULL,			/* types */
   NULL,			/* compat */
   NULL,			/* keycodes */
   NULL,			/* symbols */
   NULL				/* geometry */
#endif
   }
  ,
  {
   FALSE,
   50}
};

serverLayoutRec g_winConfigLayout;

static Bool ParseOptionValue (int scrnIndex, pointer options,
			      OptionInfoPtr p);
static Bool configLayout (serverLayoutPtr, XF86ConfLayoutPtr, char *);
static Bool configImpliedLayout (serverLayoutPtr, XF86ConfScreenPtr);
static Bool GetBoolValue (OptionInfoPtr p, const char *s);

#define NULL_IF_EMPTY(x) (winNameCompare(x,"")?x:NULL)


Bool
winReadConfigfile ()
{
  Bool retval = TRUE;
  const char *filename;

  MessageType from = X_DEFAULT;
  char *xf86ConfigFile = NULL;

  if (g_cmdline.configFile)
    {
      from = X_CMDLINE;
      xf86ConfigFile = g_cmdline.configFile;
    }

  /* Parse config file into data structure */

  filename = xf86openConfigFile (CONFIGPATH, xf86ConfigFile, PROJECTROOT);
  if (filename)
    {
      winMsg (from, "Using config file: \"%s\"\n", filename);
    }
  else
    {
      winMsg (X_ERROR, "Unable to locate/open config file");
      if (xf86ConfigFile)
	ErrorF (": \"%s\"", xf86ConfigFile);
      ErrorF ("\n");
      return FALSE;
    }
  if ((g_xf86configptr = xf86readConfigFile ()) == NULL)
    {
      winMsg (X_ERROR, "Problem parsing the config file\n");
      return FALSE;
    }
  xf86closeConfigFile ();

  winMsg (X_NONE, "Markers: ");
  winMsg (X_PROBED, "probed, ");
  winMsg (X_CONFIG, "from config file, ");
  winMsg (X_DEFAULT, "default setting,\n         ");
  winMsg (X_CMDLINE, "from command line, ");
  winMsg (X_NOTICE, "notice, ");
  winMsg (X_INFO, "informational,\n         ");
  winMsg (X_WARNING, "warning, ");
  winMsg (X_ERROR, "error, ");
  winMsg (X_UNKNOWN, "unknown.\n");

  /* set options from data structure */

  if (g_xf86configptr->conf_layout_lst == NULL || g_cmdline.screenname != NULL)
    {
      if (g_cmdline.screenname == NULL)
	{
	  winMsg (X_WARNING,
		  "No Layout section. Using the first Screen section.\n");
	}
      if (!configImpliedLayout (&g_winConfigLayout,
				g_xf86configptr->conf_screen_lst))
	{
	  winMsg (X_ERROR, "Unable to determine the screen layout\n");
	  return FALSE;
	}
    }
  else
    {
      /* Check if layout is given in the config file */
      if (g_xf86configptr->conf_flags != NULL)
	{
	  char *dfltlayout = NULL;
	  pointer optlist = g_xf86configptr->conf_flags->flg_option_lst;

	  if (optlist && winFindOption (optlist, "defaultserverlayout"))
	    dfltlayout =
	      winSetStrOption (optlist, "defaultserverlayout", NULL);

	  if (!configLayout (&g_winConfigLayout,
			     g_xf86configptr->conf_layout_lst,
			     dfltlayout))
	    {
	      winMsg (X_ERROR, "Unable to determine the screen layout\n");
	      return FALSE;
	    }
	}
      else
	{
	  if (!configLayout (&g_winConfigLayout,
			     g_xf86configptr->conf_layout_lst,
			     NULL))
	    {
	      winMsg (X_ERROR, "Unable to determin the screen layout\n");
	      return FALSE;
	    }
	}
    }

  /* setup special config files */
  winConfigFiles ();
  return retval;
}


/* Set the keyboard configuration */

Bool
winConfigKeyboard (DeviceIntPtr pDevice)
{
  XF86ConfInputPtr		kbd = NULL;
  XF86ConfInputPtr		input_list = NULL;
  MessageType			from = X_DEFAULT;
  MessageType			kbdfrom = X_CONFIG;

  /* Setup defaults */
#ifdef XKB
  g_winInfo.xkb.disable = FALSE;
# ifdef PC98 /* japanese */	/* not implemented */
  g_winInfo.xkb.rules = "xfree98";
  g_winInfo.xkb.model = "pc98";
  g_winInfo.xkb.layout = "nex/jp";
  g_winInfo.xkb.variant = NULL;
  g_winInfo.xkb.options = NULL;
# else
  g_winInfo.xkb.rules = "xfree86";
  g_winInfo.xkb.model = "pc101";
  g_winInfo.xkb.layout = "us";
  g_winInfo.xkb.variant = NULL;
  g_winInfo.xkb.options = NULL;
# endif	/* PC98 */
  g_winInfo.xkb.initialMap = NULL;
  g_winInfo.xkb.keymap = NULL;
  g_winInfo.xkb.types = NULL;
  g_winInfo.xkb.compat = NULL;
  g_winInfo.xkb.keycodes = NULL;
  g_winInfo.xkb.symbols = NULL;
  g_winInfo.xkb.geometry = NULL;
#endif /* XKB */

  /* parse the configuration */

  if (g_cmdline.keyboard)
    kbdfrom = X_CMDLINE;

  /*
   * Until the layout code is finished, I search for the keyboard 
   * device and configure the server with it.
   */

  if (g_xf86configptr != NULL)
    input_list = g_xf86configptr->conf_input_lst;

  while (input_list != NULL)
    {
      if (winNameCompare (input_list->inp_driver, "keyboard") == 0)
	{
	  /* Check if device name matches requested name */
	  if (g_cmdline.keyboard && winNameCompare (input_list->inp_identifier,
						  g_cmdline.keyboard))
	    continue;
	  kbd = input_list;
	}
      input_list = input_list->list.next;
    }

  if (kbd != NULL)
    {
      if (kbd->inp_identifier)
	winMsg (kbdfrom, "Using keyboard \"%s\" as primary keyboard\n",
		kbd->inp_identifier);

#ifdef XKB
      from = X_DEFAULT;
      if (g_cmdline.noXkbExtension)
	{
	  from = X_CMDLINE;
	  g_winInfo.xkb.disable = TRUE;
	}
      else if (kbd->inp_option_lst)
	{
	  int b = winSetBoolOption (kbd->inp_option_lst, "XkbDisable", FALSE);
	  if (b)
	    {
	      from = X_CONFIG;
	      g_winInfo.xkb.disable = TRUE;
	    }
	}
      if (g_winInfo.xkb.disable)
	{
	  winMsg (from, "XkbExtension disabled\n");
	}
      else
	{
	  char *s;

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbRules", NULL)))
	    {
	      g_winInfo.xkb.rules = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: rules: \"%s\"\n", s);
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbModel", NULL)))
	    {
	      g_winInfo.xkb.model = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: model: \"%s\"\n", s);
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbLayout", NULL)))
	    {
	      g_winInfo.xkb.layout = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: layout: \"%s\"\n", s);
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbVariant", NULL)))
	    {
	      g_winInfo.xkb.variant = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: variant: \"%s\"\n", s);
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbOptions", NULL)))
	    {
	      g_winInfo.xkb.options = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: options: \"%s\"\n", s);
	    }

	  from = X_CMDLINE;
	  if (!XkbInitialMap)
	    {
	      s =
		winSetStrOption (kbd->inp_option_lst, "XkbInitialMap", NULL);
	      if (s)
		{
		  XkbInitialMap = NULL_IF_EMPTY (s);
		  from = X_CONFIG;
		}
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbKeymap", NULL)))
	    {
	      g_winInfo.xkb.keymap = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: keymap: \"%s\" "
		      " (overrides other XKB settings)\n", s);
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbCompat", NULL)))
	    {
	      g_winInfo.xkb.compat = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: compat: \"%s\"\n", s);
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbTypes", NULL)))
	    {
	      g_winInfo.xkb.types = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: types: \"%s\"\n", s);
	    }

	  if (
	      (s =
	       winSetStrOption (kbd->inp_option_lst, "XkbKeycodes", NULL)))
	    {
	      g_winInfo.xkb.keycodes = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: keycodes: \"%s\"\n", s);
	    }

	  if (
	      (s =
	       winSetStrOption (kbd->inp_option_lst, "XkbGeometry", NULL)))
	    {
	      g_winInfo.xkb.geometry = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: geometry: \"%s\"\n", s);
	    }

	  if ((s = winSetStrOption (kbd->inp_option_lst, "XkbSymbols", NULL)))
	    {
	      g_winInfo.xkb.symbols = NULL_IF_EMPTY (s);
	      winMsg (X_CONFIG, "XKB: symbols: \"%s\"\n", s);
	    }
	}
#endif
    }
  else
    {
      winMsg (X_ERROR, "No primary keyboard configured\n");
      winMsg (X_DEFAULT, "Using compiletime defaults for keyboard\n");
    }

  return TRUE;
}


Bool
winConfigMouse (DeviceIntPtr pDevice)
{
  MessageType			mousefrom = X_CONFIG;
  XF86ConfInputPtr		mouse = NULL;
  XF86ConfInputPtr		input_list = NULL;

  if (g_cmdline.mouse)
    mousefrom = X_CMDLINE;

  if (g_xf86configptr != NULL)
    input_list = g_xf86configptr->conf_input_lst;

  while (input_list != NULL)
    {
      if (winNameCompare (input_list->inp_driver, "mouse") == 0)
	{
	  /* Check if device name matches requested name */
	  if (g_cmdline.mouse && winNameCompare (input_list->inp_identifier,
					       g_cmdline.mouse))
	    continue;
	  mouse = input_list;
	}
      input_list = input_list->list.next;
    }

  if (mouse != NULL)
    {
      if (mouse->inp_identifier)
	winMsg (mousefrom, "Using pointer \"%s\" as primary pointer\n",
		mouse->inp_identifier);

      g_winInfo.pointer.emulate3Buttons =
	winSetBoolOption (mouse->inp_option_lst, "Emulate3Buttons", FALSE);
      if (g_cmdline.emulate3buttons)
	g_winInfo.pointer.emulate3Buttons = g_cmdline.emulate3buttons;

      g_winInfo.pointer.emulate3Timeout =
	winSetIntOption (mouse->inp_option_lst, "Emulate3Timeout", 50);
      if (g_cmdline.emulate3timeout)
	g_winInfo.pointer.emulate3Timeout = g_cmdline.emulate3timeout;
    }
  else
    {
      winMsg (X_ERROR, "No primary pointer configured\n");
      winMsg (X_DEFAULT, "Using compiletime defaults for pointer\n");
    }

  return TRUE;
}


Bool
winConfigFiles ()
{
  MessageType from;
  XF86ConfFilesPtr filesptr = NULL;

  /* set some shortcuts */
  if (g_xf86configptr != NULL)
    {
      filesptr = g_xf86configptr->conf_files;
    }


  /* Fontpath */
  from = X_DEFAULT;

  if (g_cmdline.fontPath)
    {
      from = X_CMDLINE;
      defaultFontPath = g_cmdline.fontPath;
    }
  else if (filesptr != NULL && filesptr->file_fontpath)
    {
      from = X_CONFIG;
      defaultFontPath = xstrdup (filesptr->file_fontpath);
    }
  winMsg (from, "FontPath set to \"%s\"\n", defaultFontPath);

  /* RGBPath */
  from = X_DEFAULT;
  if (g_cmdline.rgbPath)
    {
      from = X_CMDLINE;
      rgbPath = g_cmdline.rgbPath;
    }
  else if (filesptr != NULL && filesptr->file_rgbpath)
    {
      from = X_CONFIG;
      rgbPath = xstrdup (filesptr->file_rgbpath);
    }
  winMsg (from, "RgbPath set to \"%s\"\n", rgbPath);

  return TRUE;
}


Bool
winConfigOptions ()
{
  return TRUE;
}


Bool
winConfigScreens ()
{
  return TRUE;
}


char *
winSetStrOption (pointer optlist, const char *name, char *deflt)
{
  OptionInfoRec o;

  o.name = name;
  o.type = OPTV_STRING;
  if (ParseOptionValue (-1, optlist, &o))
    deflt = o.value.str;
  if (deflt)
    return xstrdup (deflt);
  else
    return NULL;
}


int
winSetBoolOption (pointer optlist, const char *name, int deflt)
{
  OptionInfoRec o;

  o.name = name;
  o.type = OPTV_BOOLEAN;
  if (ParseOptionValue (-1, optlist, &o))
    deflt = o.value.bool;
  return deflt;
}


int
winSetIntOption (pointer optlist, const char *name, int deflt)
{
  OptionInfoRec o;

  o.name = name;
  o.type = OPTV_INTEGER;
  if (ParseOptionValue (-1, optlist, &o))
    deflt = o.value.num;
  return deflt;
}


double
winSetRealOption (pointer optlist, const char *name, double deflt)
{
  OptionInfoRec o;

  o.name = name;
  o.type = OPTV_REAL;
  if (ParseOptionValue (-1, optlist, &o))
    deflt = o.value.realnum;
  return deflt;
}


/*
 * Compare two strings for equality. This is caseinsensitive  and
 * The characters '_', ' ' (space) and '\t' (tab) are treated as 
 * not existing.
 */

int
winNameCompare (const char *s1, const char *s2)
{
  char c1, c2;

  if (!s1 || *s1 == 0)
    {
      if (!s2 || *s2 == 0)
	return 0;
      else
	return 1;
    }

  while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
    s1++;
  while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
    s2++;

  c1 = (isupper (*s1) ? tolower (*s1) : *s1);
  c2 = (isupper (*s2) ? tolower (*s2) : *s2);

  while (c1 == c2)
    {
      if (c1 == 0)
	return 0;
      s1++;
      s2++;

      while (*s1 == '_' || *s1 == ' ' || *s1 == '\t')
	s1++;
      while (*s2 == '_' || *s2 == ' ' || *s2 == '\t')
	s2++;

      c1 = (isupper (*s1) ? tolower (*s1) : *s1);
      c2 = (isupper (*s2) ? tolower (*s2) : *s2);
    }
  return (c1 - c2);
}


/*
 * Find the named option in the list. 
 * @return the pointer to the option record, or NULL if not found.
 */

XF86OptionPtr
winFindOption (XF86OptionPtr list, const char *name)
{
  while (list)
    {
      if (winNameCompare (list->opt_name, name) == 0)
	return list;
      list = list->list.next;
    }
  return NULL;
}


/*
 * Find the Value of an named option.
 * @return The option value or NULL if not found.
 */

char *
winFindOptionValue (XF86OptionPtr list, const char *name)
{
  list = winFindOption (list, name);
  if (list)
    {
      if (list->opt_val)
	return (list->opt_val);
      else
	return "";
    }
  return (NULL);
}


/*
 * Parse the option.
 */

static Bool
ParseOptionValue (int scrnIndex, pointer options, OptionInfoPtr p)
{
  char *s, *end;

  if ((s = winFindOptionValue (options, p->name)) != NULL)
    {
      switch (p->type)
	{
	case OPTV_INTEGER:
	  if (*s == '\0')
	    {
	      winDrvMsg (scrnIndex, X_WARNING,
			 "Option \"%s\" requires an integer value\n",
			 p->name);
	      p->found = FALSE;
	    }
	  else
	    {
	      p->value.num = strtoul (s, &end, 0);
	      if (*end == '\0')
		{
		  p->found = TRUE;
		}
	      else
		{
		  winDrvMsg (scrnIndex, X_WARNING,
			     "Option \"%s\" requires an integer value\n",
			     p->name);
		  p->found = FALSE;
		}
	    }
	  break;
	case OPTV_STRING:
	  if (*s == '\0')
	    {
	      winDrvMsg (scrnIndex, X_WARNING,
			 "Option \"%s\" requires an string value\n", p->name);
	      p->found = FALSE;
	    }
	  else
	    {
	      p->value.str = s;
	      p->found = TRUE;
	    }
	  break;
	case OPTV_ANYSTR:
	  p->value.str = s;
	  p->found = TRUE;
	  break;
	case OPTV_REAL:
	  if (*s == '\0')
	    {
	      winDrvMsg (scrnIndex, X_WARNING,
			 "Option \"%s\" requires a floating point value\n",
			 p->name);
	      p->found = FALSE;
	    }
	  else
	    {
	      p->value.realnum = strtod (s, &end);
	      if (*end == '\0')
		{
		  p->found = TRUE;
		}
	      else
		{
		  winDrvMsg (scrnIndex, X_WARNING,
			     "Option \"%s\" requires a floating point value\n",
			     p->name);
		  p->found = FALSE;
		}
	    }
	  break;
	case OPTV_BOOLEAN:
	  if (GetBoolValue (p, s))
	    {
	      p->found = TRUE;
	    }
	  else
	    {
	      winDrvMsg (scrnIndex, X_WARNING,
			 "Option \"%s\" requires a boolean value\n", p->name);
	      p->found = FALSE;
	    }
	  break;
	case OPTV_FREQ:
	  if (*s == '\0')
	    {
	      winDrvMsg (scrnIndex, X_WARNING,
			 "Option \"%s\" requires a frequency value\n",
			 p->name);
	      p->found = FALSE;
	    }
	  else
	    {
	      double freq = strtod (s, &end);
	      int units = 0;

	      if (end != s)
		{
		  p->found = TRUE;
		  if (!winNameCompare (end, "Hz"))
		    units = 1;
		  else if (!winNameCompare (end, "kHz") ||
			   !winNameCompare (end, "k"))
		    units = 1000;
		  else if (!winNameCompare (end, "MHz") ||
			   !winNameCompare (end, "M"))
		    units = 1000000;
		  else
		    {
		      winDrvMsg (scrnIndex, X_WARNING,
				 "Option \"%s\" requires a frequency value\n",
				 p->name);
		      p->found = FALSE;
		    }
		  if (p->found)
		    freq *= (double) units;
		}
	      else
		{
		  winDrvMsg (scrnIndex, X_WARNING,
			     "Option \"%s\" requires a frequency value\n",
			     p->name);
		  p->found = FALSE;
		}
	      if (p->found)
		{
		  p->value.freq.freq = freq;
		  p->value.freq.units = units;
		}
	    }
	  break;
	case OPTV_NONE:
	  /* Should never get here */
	  p->found = FALSE;
	  break;
	}
      if (p->found)
	{
	  winDrvMsgVerb (scrnIndex, X_CONFIG, 2, "Option \"%s\"", p->name);
	  if (!(p->type == OPTV_BOOLEAN && *s == 0))
	    {
	      winErrorFVerb (2, " \"%s\"", s);
	    }
	  winErrorFVerb (2, "\n");
	}
    }
  else if (p->type == OPTV_BOOLEAN)
    {
      /* Look for matches with options with or without a "No" prefix. */
      char *n, *newn;
      OptionInfoRec opt;

      n = winNormalizeName (p->name);
      if (!n)
	{
	  p->found = FALSE;
	  return FALSE;
	}
      if (strncmp (n, "no", 2) == 0)
	{
	  newn = n + 2;
	}
      else
	{
	  free (n);
	  n = malloc (strlen (p->name) + 2 + 1);
	  if (!n)
	    {
	      p->found = FALSE;
	      return FALSE;
	    }
	  strcpy (n, "No");
	  strcat (n, p->name);
	  newn = n;
	}
      if ((s = winFindOptionValue (options, newn)) != NULL)
	{
	  if (GetBoolValue (&opt, s))
	    {
	      p->value.bool = !opt.value.bool;
	      p->found = TRUE;
	    }
	  else
	    {
	      winDrvMsg (scrnIndex, X_WARNING,
			 "Option \"%s\" requires a boolean value\n", newn);
	      p->found = FALSE;
	    }
	}
      else
	{
	  p->found = FALSE;
	}
      if (p->found)
	{
	  winDrvMsgVerb (scrnIndex, X_CONFIG, 2, "Option \"%s\"", newn);
	  if (*s != 0)
	    {
	      winErrorFVerb (2, " \"%s\"", s);
	    }
	  winErrorFVerb (2, "\n");
	}
      free (n);
    }
  else
    {
      p->found = FALSE;
    }
  return p->found;
}


static Bool
configLayout (serverLayoutPtr servlayoutp, XF86ConfLayoutPtr conf_layout,
	      char *default_layout)
{
#if 0
#pragma warn UNIMPLEMENTED
#endif
  return TRUE;
}


static Bool
configImpliedLayout (serverLayoutPtr servlayoutp,
		     XF86ConfScreenPtr conf_screen)
{
#if 0
#pragma warn UNIMPLEMENTED
#endif
  return TRUE;
}


static Bool
GetBoolValue (OptionInfoPtr p, const char *s)
{
  if (*s == 0)
    {
      p->value.bool = TRUE;
    }
  else
    {
      if (winNameCompare (s, "1") == 0)
	p->value.bool = TRUE;
      else if (winNameCompare (s, "on") == 0)
	p->value.bool = TRUE;
      else if (winNameCompare (s, "true") == 0)
	p->value.bool = TRUE;
      else if (winNameCompare (s, "yes") == 0)
	p->value.bool = TRUE;
      else if (winNameCompare (s, "0") == 0)
	p->value.bool = FALSE;
      else if (winNameCompare (s, "off") == 0)
	p->value.bool = FALSE;
      else if (winNameCompare (s, "false") == 0)
	p->value.bool = FALSE;
      else if (winNameCompare (s, "no") == 0)
	p->value.bool = FALSE;
    }
  return TRUE;
}


char *
winNormalizeName (const char *s)
{
  char *ret, *q;
  const char *p;

  if (s == NULL)
    return NULL;

  ret = malloc (strlen (s) + 1);
  for (p = s, q = ret; *p != 0; p++)
    {
      switch (*p)
	{
	case '_':
	case ' ':
	case '\t':
	  continue;
	default:
	  if (isupper (*p))
	    *q++ = tolower (*p);
	  else
	    *q++ = *p;
	}
    }
  *q = '\0';
  return ret;
}
