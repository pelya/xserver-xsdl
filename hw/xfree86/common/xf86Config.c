/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Config.c,v 3.277 2003/10/15 22:51:48 dawes Exp $ */


/*
 * Loosely based on code bearing the following copyright:
 *
 *   Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 */

/*
 * Copyright 1992-2003 by The XFree86 Project, Inc.
 * Copyright 1997 by Metro Link, Inc.
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

/*
 *
 * Authors:
 *	Dirk Hohndel <hohndel@XFree86.Org>
 *	David Dawes <dawes@XFree86.Org>
 *      Marc La France <tsi@XFree86.Org>
 *      Egbert Eich <eich@XFree86.Org>
 *      ... and others
 */

#ifdef XF86DRI
#include <sys/types.h>
#include <grp.h>
#endif

#ifdef __UNIXOS2__
#define I_NEED_OS2_H
#endif

#include "xf86.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "xf86Config.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include "globals.h"

#ifdef XINPUT
#include "xf86Xinput.h"
extern DeviceAssocRec mouse_assoc;
#endif

#ifdef XKB
#define XKB_IN_SERVER
#include "XKBsrv.h"
#endif

#ifdef RENDER
#include "picture.h"
#endif

#if (defined(i386) || defined(__i386__)) && \
    (defined(__FreeBSD__) || defined(__NetBSD__) || defined(linux) || \
     (defined(SVR4) && !defined(sun)) || defined(__GNU__))
#define SUPPORT_PC98
#endif

/*
 * These paths define the way the config file search is done.  The escape
 * sequences are documented in parser/scan.c.
 */
#ifndef ROOT_CONFIGPATH
#define ROOT_CONFIGPATH	"%A," "%R," \
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
#ifndef USER_CONFIGPATH
#define USER_CONFIGPATH	"/etc/X11/%S," "%P/etc/X11/%S," \
			"/etc/X11/%G," "%P/etc/X11/%G," \
			"/etc/X11/%X-%M," "/etc/X11/%X," "/etc/%X," \
			"%P/etc/X11/%X.%H," "%P/etc/X11/%X-%M," \
			"%P/etc/X11/%X," \
			"%P/lib/X11/%X.%H," "%P/lib/X11/%X-%M," \
			"%P/lib/X11/%X"
#endif
#ifndef PROJECTROOT
#define PROJECTROOT	"/usr/X11R6"
#endif

static char *fontPath = NULL;

/* Forward declarations */
static Bool configScreen(confScreenPtr screenp, XF86ConfScreenPtr conf_screen,
			 int scrnum, MessageType from);
static Bool configMonitor(MonPtr monitorp, XF86ConfMonitorPtr conf_monitor);
static Bool configDevice(GDevPtr devicep, XF86ConfDevicePtr conf_device,
			 Bool active);
static Bool configInput(IDevPtr inputp, XF86ConfInputPtr conf_input,
			MessageType from);
static Bool configDisplay(DispPtr displayp, XF86ConfDisplayPtr conf_display);
static Bool addDefaultModes(MonPtr monitorp);
#ifdef XF86DRI
static Bool configDRI(XF86ConfDRIPtr drip);
#endif

/*
 * xf86GetPathElem --
 *	Extract a single element from the font path string starting at
 *	pnt.  The font path element will be returned, and pnt will be
 *	updated to point to the start of the next element, or set to
 *	NULL if there are no more.
 */
static char *
xf86GetPathElem(char **pnt)
{
  char *p1;

  p1 = *pnt;
  *pnt = index(*pnt, ',');
  if (*pnt != NULL) {
    **pnt = '\0';
    *pnt += 1;
  }
  return(p1);
}

/*
 * xf86ValidateFontPath --
 *	Validates the user-specified font path.  Each element that
 *	begins with a '/' is checked to make sure the directory exists.
 *	If the directory exists, the existence of a file named 'fonts.dir'
 *	is checked.  If either check fails, an error is printed and the
 *	element is removed from the font path.
 */

#define DIR_FILE "/fonts.dir"
static char *
xf86ValidateFontPath(char *path)
{
  char *tmp_path, *out_pnt, *path_elem, *next, *p1, *dir_elem;
  struct stat stat_buf;
  int flag;
  int dirlen;

  tmp_path = xcalloc(1,strlen(path)+1);
  out_pnt = tmp_path;
  path_elem = NULL;
  next = path;
  while (next != NULL) {
    path_elem = xf86GetPathElem(&next);
    if (*path_elem == '/') {
#ifndef __UNIXOS2__
      dir_elem = xnfcalloc(1, strlen(path_elem) + 1);
      if ((p1 = strchr(path_elem, ':')) != 0)
#else
    /* OS/2 must prepend X11ROOT */
      path_elem = (char*)__XOS2RedirRoot(path_elem);
      dir_elem = xnfcalloc(1, strlen(path_elem) + 1);
      if (p1 = strchr(path_elem+2, ':'))
#endif
	dirlen = p1 - path_elem;
      else
	dirlen = strlen(path_elem);
      strncpy(dir_elem, path_elem, dirlen);
      dir_elem[dirlen] = '\0';
      flag = stat(dir_elem, &stat_buf);
      if (flag == 0)
	if (!S_ISDIR(stat_buf.st_mode))
	  flag = -1;
      if (flag != 0) {
        xf86Msg(X_WARNING, "The directory \"%s\" does not exist.\n", dir_elem);
	xf86ErrorF("\tEntry deleted from font path.\n");
	xfree(dir_elem);
	continue;
      }
      else {
	p1 = xnfalloc(strlen(dir_elem)+strlen(DIR_FILE)+1);
	strcpy(p1, dir_elem);
	strcat(p1, DIR_FILE);
	flag = stat(p1, &stat_buf);
	if (flag == 0)
	  if (!S_ISREG(stat_buf.st_mode))
	    flag = -1;
#ifndef __UNIXOS2__
	xfree(p1);
#endif
	if (flag != 0) {
	  xf86Msg(X_WARNING,
		  "`fonts.dir' not found (or not valid) in \"%s\".\n", 
		  dir_elem);
	  xf86ErrorF("\tEntry deleted from font path.\n");
	  xf86ErrorF("\t(Run 'mkfontdir' on \"%s\").\n", dir_elem);
	  xfree(dir_elem);
	  continue;
	}
      }
      xfree(dir_elem);
    }

    /*
     * Either an OK directory, or a font server name.  So add it to
     * the path.
     */
    if (out_pnt != tmp_path)
      *out_pnt++ = ',';
    strcat(out_pnt, path_elem);
    out_pnt += strlen(path_elem);
  }
  return(tmp_path);
}


/*
 * use the datastructure that the parser provides and pick out the parts
 * that we need at this point
 */
char **
xf86ModulelistFromConfig(pointer **optlist)
{
    int count = 0;
    char **modulearray;
    pointer *optarray;
    XF86LoadPtr modp;
    
    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }
    
    if (xf86configptr->conf_modules) {
	/*
	 * Walk the list of modules in the "Module" section to determine how
	 * many we have.
	 */
	modp = xf86configptr->conf_modules->mod_load_lst;
	while (modp) {
	    count++;
	    modp = (XF86LoadPtr) modp->list.next;
	}
    }
    if (count == 0)
	return NULL;

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfalloc((count + 1) * sizeof(char*));
    optarray = xnfalloc((count + 1) * sizeof(pointer));
    count = 0;
    if (xf86configptr->conf_modules) {
	modp = xf86configptr->conf_modules->mod_load_lst;
	while (modp) {
	    modulearray[count] = modp->load_name;
	    optarray[count] = modp->load_opt;
	    count++;
	    modp = (XF86LoadPtr) modp->list.next;
	}
    }
    modulearray[count] = NULL;
    optarray[count] = NULL;
    if (optlist)
	*optlist = optarray;
    else
	xfree(optarray);
    return modulearray;
}


char **
xf86DriverlistFromConfig()
{
    int count = 0;
    int j;
    char **modulearray;
    screenLayoutPtr slp;
    
    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }
    
    /*
     * Walk the list of driver lines in active "Device" sections to
     * determine now many implicitly loaded modules there are.
     *
     */
    if (xf86ConfigLayout.screens) {
        slp = xf86ConfigLayout.screens;
        while ((slp++)->screen) {
	    count++;
        }
    }

    /*
     * Handle the set of inactive "Device" sections.
     */
    j = 0;
    while (xf86ConfigLayout.inactives[j++].identifier)
	count++;

    if (count == 0)
	return NULL;

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfalloc((count + 1) * sizeof(char*));
    count = 0;
    slp = xf86ConfigLayout.screens;
    while (slp->screen) {
	modulearray[count] = slp->screen->device->driver;
	count++;
	slp++;
    }

    j = 0;

    while (xf86ConfigLayout.inactives[j].identifier) 
	modulearray[count++] = xf86ConfigLayout.inactives[j++].driver;

    modulearray[count] = NULL;

    /* Remove duplicates */
    for (count = 0; modulearray[count] != NULL; count++) {
	int i;

	for (i = 0; i < count; i++)
	    if (xf86NameCmp(modulearray[i], modulearray[count]) == 0) {
		modulearray[count] = "";
		break;
	    }
    }
    return modulearray;
}


Bool
xf86BuiltinInputDriver(const char *name)
{
    if (xf86NameCmp(name, "keyboard") == 0)
	return TRUE;
    else
	return FALSE;
}


char **
xf86InputDriverlistFromConfig()
{
    int count = 0;
    char **modulearray;
    IDevPtr idp;
    
    /*
     * make sure the config file has been parsed and that we have a
     * ModulePath set; if no ModulePath was given, use the default
     * ModulePath
     */
    if (xf86configptr == NULL) {
        xf86Msg(X_ERROR, "Cannot access global config data structure\n");
        return NULL;
    }
    
    /*
     * Walk the list of driver lines in active "InputDevice" sections to
     * determine now many implicitly loaded modules there are.
     */
    if (xf86ConfigLayout.inputs) {
        idp = xf86ConfigLayout.inputs;
        while (idp->identifier) {
	    if (!xf86BuiltinInputDriver(idp->driver))
	        count++;
	    idp++;
        }
    }

    if (count == 0)
	return NULL;

    /*
     * allocate the memory and walk the list again to fill in the pointers
     */
    modulearray = xnfalloc((count + 1) * sizeof(char*));
    count = 0;
    idp = xf86ConfigLayout.inputs;
    while (idp->identifier) {
	if (!xf86BuiltinInputDriver(idp->driver)) {
	    modulearray[count] = idp->driver;
	    count++;
	}
	idp++;
    }
    modulearray[count] = NULL;

    /* Remove duplicates */
    for (count = 0; modulearray[count] != NULL; count++) {
	int i;

	for (i = 0; i < count; i++)
	    if (xf86NameCmp(modulearray[i], modulearray[count]) == 0) {
		modulearray[count] = "";
		break;
	    }
    }
    return modulearray;
}


/*
 * Generate a compiled-in list of driver names.  This is used to produce a
 * consistent probe order.  For the loader server, we also look for vendor-
 * provided modules, pre-pending them to our own list.
 */
static char **
GenerateDriverlist(char * dirname, char * drivernames)
{
    char *cp, **driverlist;
    int count;

    /* Count the number needed */
    count = 0;
    cp = drivernames;
    while (*cp) {
	while (*cp && isspace(*cp)) cp++;
	if (!*cp) break;
	count++;
	while (*cp && !isspace(*cp)) cp++;
    }

    if (!count)
	return NULL;

    /* Now allocate the array of pointers to 0-terminated driver names */
    driverlist = (char **)xnfalloc((count + 1) * sizeof(char *));
    count = 0;
    cp = drivernames;
    while (*cp) {
	while (*cp && isspace(*cp)) cp++;
	if (!*cp) break;
	driverlist[count++] = cp;
	while (*cp && !isspace(*cp)) cp++;
	if (!*cp) break;
	*cp++ = 0;
    }
    driverlist[count] = NULL;

#ifdef XFree86LOADER
    {
        const char *subdirs[] = {NULL, NULL};
        static const char *patlist[] = {"(.*)_drv\\.so", "(.*)_drv\\.o", NULL};
        char **dlist, **clist, **dcp, **ccp;
	int size;

        subdirs[0] = dirname;

        /* Get module list */
        dlist = LoaderListDirs(subdirs, patlist);
        if (!dlist) {
            xfree(driverlist);
            return NULL;        /* No modules, no list */
        }

        clist = driverlist;

        /* The resulting list cannot be longer than the module list */
        for (dcp = dlist, count = 0;  *dcp++;  count++);
        driverlist = (char **)xnfalloc((size = count + 1) * sizeof(char *));

        /* First, add modules not in compiled-in list */
        for (count = 0, dcp = dlist;  *dcp;  dcp++) {
            for (ccp = clist;  ;  ccp++) {
                if (!*ccp) {
                    driverlist[count++] = *dcp;
		    if (count >= size)
			driverlist = (char**)
			    xnfrealloc(driverlist, ++size * sizeof(char*));
                    break;
                }
                if (!strcmp(*ccp, *dcp))
                    break;
            }
        }

        /* Next, add compiled-in names that are also modules */
        for (ccp = clist;  *ccp;  ccp++) {
            for (dcp = dlist;  *dcp;  dcp++) {
                if (!strcmp(*ccp, *dcp)) {
                    driverlist[count++] = *ccp;
		    if (count >= size)
			driverlist = (char**)
			    xnfrealloc(driverlist, ++size * sizeof(char*));
                    break;
                }
            }
        }

        driverlist[count] = NULL;
        xfree(clist);
        xfree(dlist);
    }
#endif /* XFree86LOADER */

    return driverlist;
}


char **
xf86DriverlistFromCompile(void)
{
    static char **driverlist = NULL;
    static Bool generated = FALSE;

    /* This string is modified in-place */
    static char drivernames[] = DRIVERS;

    if (!generated) {
        generated = TRUE;
        driverlist = GenerateDriverlist("drivers", drivernames);
    }

    return driverlist;
}


char **
xf86InputDriverlistFromCompile(void)
{
    static char **driverlist = NULL;
    static Bool generated = FALSE;

    /* This string is modified in-place */
    static char drivernames[] = IDRIVERS;

    if (!generated) {
        generated = TRUE;
	driverlist = GenerateDriverlist("input", drivernames);
    }

    return driverlist;
}


/*
 * xf86ConfigError --
 *      Print a READABLE ErrorMessage!!!  All information that is 
 *      available is printed.
 */
static void
xf86ConfigError(char *msg, ...)
{
    va_list ap;

    ErrorF("\nConfig Error:\n");
    va_start(ap, msg);
    VErrorF(msg, ap);
    va_end(ap);
    ErrorF("\n");
    return;
}

static Bool
configFiles(XF86ConfFilesPtr fileconf)
{
  MessageType pathFrom = X_DEFAULT;

  /* FontPath */

  /* Try XF86Config FontPath first */
  if (!xf86fpFlag) {
   if (fileconf) {
    if (fileconf->file_fontpath) {
      char *f = xf86ValidateFontPath(fileconf->file_fontpath);
      pathFrom = X_CONFIG;
      if (*f)
        defaultFontPath = f;
      else {
	xf86Msg(X_WARNING,
	    "FontPath is completely invalid.  Using compiled-in default.\n");
        fontPath = NULL;
        pathFrom = X_DEFAULT;
      }
    } 
   } else {
      xf86Msg(X_WARNING,
	    "No FontPath specified.  Using compiled-in default.\n");
      pathFrom = X_DEFAULT;
   }
  } else {
    /* Use fontpath specified with '-fp' */
    if (fontPath)
    {
      fontPath = NULL;
    }
    pathFrom = X_CMDLINE;
  }
  if (!fileconf) {
      /* xf86ValidateFontPath will write into it's arg, but defaultFontPath
       could be static, so we make a copy. */
    char *f = xnfalloc(strlen(defaultFontPath) + 1);
    f[0] = '\0';
    strcpy (f, defaultFontPath);
    defaultFontPath = xf86ValidateFontPath(f);
    xfree(f);
  } else {
   if (fileconf) {
    if (!fileconf->file_fontpath) {
      /* xf86ValidateFontPath will write into it's arg, but defaultFontPath
       could be static, so we make a copy. */
     char *f = xnfalloc(strlen(defaultFontPath) + 1);
     f[0] = '\0';
     strcpy (f, defaultFontPath);
     defaultFontPath = xf86ValidateFontPath(f);
     xfree(f);
    }
   }
  }

  /* If defaultFontPath is still empty, exit here */

  if (! *defaultFontPath)
    FatalError("No valid FontPath could be found.");

  xf86Msg(pathFrom, "FontPath set to \"%s\"\n", defaultFontPath);

  /* RgbPath */

  pathFrom = X_DEFAULT;

  if (xf86coFlag)
    pathFrom = X_CMDLINE;
  else if (fileconf) {
    if (fileconf->file_rgbpath) {
      rgbPath = fileconf->file_rgbpath;
      pathFrom = X_CONFIG;
    }
  }

  xf86Msg(pathFrom, "RgbPath set to \"%s\"\n", rgbPath);

  if (fileconf && fileconf->file_inputdevs) {
      xf86InputDeviceList = fileconf->file_inputdevs;
      xf86Msg(X_CONFIG, "Input device list set to \"%s\"\n",
	  xf86InputDeviceList);
  }
  
  
#ifdef XFree86LOADER
  /* ModulePath */

  if (fileconf) {
    if (xf86ModPathFrom != X_CMDLINE && fileconf->file_modulepath) {
      xf86ModulePath = fileconf->file_modulepath;
      xf86ModPathFrom = X_CONFIG;
    }
  }

  xf86Msg(xf86ModPathFrom, "ModulePath set to \"%s\"\n", xf86ModulePath);
#endif

#if 0
  /* LogFile */
  /*
   * XXX The problem with this is that the log file is already open.
   * One option might be to copy the exiting contents to the new location.
   * and re-open it.  The down side is that the default location would
   * already have been overwritten.  Another option would be to start with
   * unique temporary location, then copy it once the correct name is known.
   * A problem with this is what happens if the server exits before that
   * happens.
   */
  if (xf86LogFileFrom == X_DEFAULT && fileconf->file_logfile) {
    xf86LogFile = fileconf->file_logfile;
    xf86LogFileFrom = X_CONFIG;
  }
#endif

  return TRUE;
}

typedef enum {
    FLAG_NOTRAPSIGNALS,
    FLAG_DONTVTSWITCH,
    FLAG_DONTZAP,
    FLAG_DONTZOOM,
    FLAG_DISABLEVIDMODE,
    FLAG_ALLOWNONLOCAL,
    FLAG_DISABLEMODINDEV,
    FLAG_MODINDEVALLOWNONLOCAL,
    FLAG_ALLOWMOUSEOPENFAIL,
    FLAG_VTINIT,
    FLAG_VTSYSREQ,
    FLAG_XKBDISABLE,
    FLAG_PCIPROBE1,
    FLAG_PCIPROBE2,
    FLAG_PCIFORCECONFIG1,
    FLAG_PCIFORCECONFIG2,
    FLAG_PCIFORCENONE,
    FLAG_PCIOSCONFIG,
    FLAG_SAVER_BLANKTIME,
    FLAG_DPMS_STANDBYTIME,
    FLAG_DPMS_SUSPENDTIME,
    FLAG_DPMS_OFFTIME,
    FLAG_PIXMAP,
    FLAG_PC98,
    FLAG_ESTIMATE_SIZES_AGGRESSIVELY,
    FLAG_NOPM,
    FLAG_XINERAMA,
    FLAG_ALLOW_DEACTIVATE_GRABS,
    FLAG_ALLOW_CLOSEDOWN_GRABS,
    FLAG_LOG,
    FLAG_RENDER_COLORMAP_MODE,
    FLAG_HANDLE_SPECIAL_KEYS,
    FLAG_RANDR
} FlagValues;
   
static OptionInfoRec FlagOptions[] = {
  { FLAG_NOTRAPSIGNALS,		"NoTrapSignals",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DONTVTSWITCH,		"DontVTSwitch",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DONTZAP,		"DontZap",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DONTZOOM,		"DontZoom",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DISABLEVIDMODE,	"DisableVidModeExtension",	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_ALLOWNONLOCAL,		"AllowNonLocalXvidtune",	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_DISABLEMODINDEV,	"DisableModInDev",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_MODINDEVALLOWNONLOCAL,	"AllowNonLocalModInDev",	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_ALLOWMOUSEOPENFAIL,	"AllowMouseOpenFail",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_VTINIT,		"VTInit",			OPTV_STRING,
	{0}, FALSE },
  { FLAG_VTSYSREQ,		"VTSysReq",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_XKBDISABLE,		"XkbDisable",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIPROBE1,		"PciProbe1"		,	OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIPROBE2,		"PciProbe2",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIFORCECONFIG1,	"PciForceConfig1",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIFORCECONFIG2,	"PciForceConfig2",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIFORCENONE,		"PciForceNone",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_PCIOSCONFIG,	        "PciOsConfig",   		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_SAVER_BLANKTIME,	"BlankTime"		,	OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_DPMS_STANDBYTIME,	"StandbyTime",			OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_DPMS_SUSPENDTIME,	"SuspendTime",			OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_DPMS_OFFTIME,		"OffTime",			OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_PIXMAP,		"Pixmap",			OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_PC98,			"PC98",				OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_ESTIMATE_SIZES_AGGRESSIVELY,"EstimateSizesAggressively",OPTV_INTEGER,
	{0}, FALSE },
  { FLAG_NOPM,			"NoPM",				OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_XINERAMA,		"Xinerama",			OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_ALLOW_DEACTIVATE_GRABS,"AllowDeactivateGrabs",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_ALLOW_CLOSEDOWN_GRABS, "AllowClosedownGrabs",		OPTV_BOOLEAN,
	{0}, FALSE },
  { FLAG_LOG,			"Log",				OPTV_STRING,
	{0}, FALSE },
  { FLAG_RENDER_COLORMAP_MODE,	"RenderColormapMode",		OPTV_STRING,
        {0}, FALSE },
  { FLAG_HANDLE_SPECIAL_KEYS,	"HandleSpecialKeys",		OPTV_STRING,
        {0}, FALSE },
  { FLAG_RANDR,			"RandR",			OPTV_BOOLEAN,
	{0}, FALSE },
  { -1,				NULL,				OPTV_NONE,
	{0}, FALSE },
};

#if defined(i386) || defined(__i386__)
static Bool
detectPC98(void)
{
#ifdef SUPPORT_PC98
    unsigned char buf[2];

    if (xf86ReadBIOS(0xf8000, 0xe80, buf, 2) != 2)
	return FALSE;
    if ((buf[0] == 0x98) && (buf[1] == 0x21))
	return TRUE;
    else
	return FALSE;
#else
    return FALSE;
#endif
}
#endif /* __i386__ */

static Bool
configServerFlags(XF86ConfFlagsPtr flagsconf, XF86OptionPtr layoutopts)
{
    XF86OptionPtr optp, tmp;
    int i;
    Pix24Flags pix24 = Pix24DontCare;
    Bool value;
    MessageType from;

    /*
     * Merge the ServerLayout and ServerFlags options.  The former have
     * precedence over the latter.
     */
    optp = NULL;
    if (flagsconf && flagsconf->flg_option_lst)
	optp = xf86optionListDup(flagsconf->flg_option_lst);
    if (layoutopts) {
	tmp = xf86optionListDup(layoutopts);
	if (optp)
	    optp = xf86optionListMerge(optp, tmp);
	else
	    optp = tmp;
    }

    xf86ProcessOptions(-1, optp, FlagOptions);

    xf86GetOptValBool(FlagOptions, FLAG_NOTRAPSIGNALS, &xf86Info.notrapSignals);
    xf86GetOptValBool(FlagOptions, FLAG_DONTVTSWITCH, &xf86Info.dontVTSwitch);
    xf86GetOptValBool(FlagOptions, FLAG_DONTZAP, &xf86Info.dontZap);
    xf86GetOptValBool(FlagOptions, FLAG_DONTZOOM, &xf86Info.dontZoom);

    xf86GetOptValBool(FlagOptions, FLAG_ALLOW_DEACTIVATE_GRABS,
		      &(xf86Info.grabInfo.allowDeactivate));
    xf86GetOptValBool(FlagOptions, FLAG_ALLOW_CLOSEDOWN_GRABS,
		      &(xf86Info.grabInfo.allowClosedown));

    /*
     * Set things up based on the config file information.  Some of these
     * settings may be overridden later when the command line options are
     * checked.
     */
#ifdef XF86VIDMODE
    if (xf86GetOptValBool(FlagOptions, FLAG_DISABLEVIDMODE, &value))
	xf86Info.vidModeEnabled = !value;
    if (xf86GetOptValBool(FlagOptions, FLAG_ALLOWNONLOCAL, &value))
	xf86Info.vidModeAllowNonLocal = value;
#endif

#ifdef XF86MISC
    if (xf86GetOptValBool(FlagOptions, FLAG_DISABLEMODINDEV, &value))
	xf86Info.miscModInDevEnabled = !value;
    if (xf86GetOptValBool(FlagOptions, FLAG_MODINDEVALLOWNONLOCAL, &value))
	xf86Info.miscModInDevAllowNonLocal = value;
#endif

    if (xf86GetOptValBool(FlagOptions, FLAG_ALLOWMOUSEOPENFAIL, &value))
	xf86Info.allowMouseOpenFail = value;

    if (xf86GetOptValBool(FlagOptions, FLAG_VTSYSREQ, &value)) {
#ifdef USE_VT_SYSREQ
	xf86Info.vtSysreq = value;
	xf86Msg(X_CONFIG, "VTSysReq %s\n", value ? "enabled" : "disabled");
#else
	if (value)
	    xf86Msg(X_WARNING, "VTSysReq is not supported on this OS\n");
#endif
    }

    if (xf86GetOptValBool(FlagOptions, FLAG_XKBDISABLE, &value)) {
#ifdef XKB
	noXkbExtension = value;
	xf86Msg(X_CONFIG, "Xkb %s\n", value ? "disabled" : "enabled");
#else
	if (!value)
	    xf86Msg(X_WARNING, "Xserver doesn't support XKB\n");
#endif
    }

    xf86Info.vtinit = xf86GetOptValString(FlagOptions, FLAG_VTINIT);

    if (xf86IsOptionSet(FlagOptions, FLAG_PCIPROBE1))
	xf86Info.pciFlags = PCIProbe1;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIPROBE2))
	xf86Info.pciFlags = PCIProbe2;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIFORCECONFIG1))
	xf86Info.pciFlags = PCIForceConfig1;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIFORCECONFIG2))
	xf86Info.pciFlags = PCIForceConfig2;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIOSCONFIG))
	xf86Info.pciFlags = PCIOsConfig;
    if (xf86IsOptionSet(FlagOptions, FLAG_PCIFORCENONE))
	xf86Info.pciFlags = PCIForceNone;

    xf86Info.pmFlag = TRUE;
    if (xf86GetOptValBool(FlagOptions, FLAG_NOPM, &value)) 
	xf86Info.pmFlag = !value;
    {
	const char *s;
	if ((s = xf86GetOptValString(FlagOptions, FLAG_LOG))) {
	    if (!xf86NameCmp(s,"flush")) {
		xf86Msg(X_CONFIG, "Flushing logfile enabled\n");
		xf86Info.log = LogFlush;
		LogSetParameter(XLOG_FLUSH, TRUE);
	    } else if (!xf86NameCmp(s,"sync")) {
		xf86Msg(X_CONFIG, "Syncing logfile enabled\n");
		xf86Info.log = LogSync;
		LogSetParameter(XLOG_SYNC, TRUE);
	    } else {
		xf86Msg(X_WARNING,"Unknown Log option\n");
	    }
        }
    }
    
#ifdef RENDER
    {
	const char *s;

	if ((s = xf86GetOptValString(FlagOptions, FLAG_RENDER_COLORMAP_MODE))){
	    int policy = PictureParseCmapPolicy (s);
	    if (policy == PictureCmapPolicyInvalid)
		xf86Msg(X_WARNING, "Unknown colormap policy \"%s\"\n", s);
	    else
	    {
		xf86Msg(X_CONFIG, "Render colormap policy set to %s\n", s);
		PictureCmapPolicy = policy;
	    }
	}
    }
#endif
    {
	const char *s;
	if ((s = xf86GetOptValString(FlagOptions, FLAG_HANDLE_SPECIAL_KEYS))) {
	    if (!xf86NameCmp(s,"always")) {
		xf86Msg(X_CONFIG, "Always handling special keys in DDX\n");
		xf86Info.ddxSpecialKeys = SKAlways;
	    } else if (!xf86NameCmp(s,"whenneeded")) {
		xf86Msg(X_CONFIG, "Special keys handled in DDX only if needed\n");
		xf86Info.ddxSpecialKeys = SKWhenNeeded;
	    } else if (!xf86NameCmp(s,"never")) {
		xf86Msg(X_CONFIG, "Never handling special keys in DDX\n");
		xf86Info.ddxSpecialKeys = SKNever;
	    } else {
		xf86Msg(X_WARNING,"Unknown HandleSpecialKeys option\n");
	    }
        }
    }
#ifdef RANDR
    xf86Info.disableRandR = FALSE;
    xf86Info.randRFrom = X_DEFAULT;
    if (xf86GetOptValBool(FlagOptions, FLAG_RANDR, &value)) {
	xf86Info.disableRandR = !value;
	xf86Info.randRFrom = X_CONFIG;
    }
#endif
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_ESTIMATE_SIZES_AGGRESSIVELY, &i);
    if (i >= 0)
	xf86Info.estimateSizesAggressively = i;
    else
	xf86Info.estimateSizesAggressively = 0;
	
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_SAVER_BLANKTIME, &i);
    if (i >= 0)
	ScreenSaverTime = defaultScreenSaverTime = i * MILLI_PER_MIN;

#ifdef DPMSExtension
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_STANDBYTIME, &i);
    if (i >= 0)
	DPMSStandbyTime = defaultDPMSStandbyTime = i * MILLI_PER_MIN;
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_SUSPENDTIME, &i);
    if (i >= 0)
	DPMSSuspendTime = defaultDPMSSuspendTime = i * MILLI_PER_MIN;
    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_DPMS_OFFTIME, &i);
    if (i >= 0)
	DPMSOffTime = defaultDPMSOffTime = i * MILLI_PER_MIN;
#endif

    i = -1;
    xf86GetOptValInteger(FlagOptions, FLAG_PIXMAP, &i);
    switch (i) {
    case 24:
	pix24 = Pix24Use24;
	break;
    case 32:
	pix24 = Pix24Use32;
	break;
    case -1:
	break;
    default:
	xf86ConfigError("Pixmap option's value (%d) must be 24 or 32\n", i);
	return FALSE;
    }
    if (xf86Pix24 != Pix24DontCare) {
	xf86Info.pixmap24 = xf86Pix24;
	xf86Info.pix24From = X_CMDLINE;
    } else if (pix24 != Pix24DontCare) {
	xf86Info.pixmap24 = pix24;
	xf86Info.pix24From = X_CONFIG;
    } else {
	xf86Info.pixmap24 = Pix24DontCare;
	xf86Info.pix24From = X_DEFAULT;
    }
#if defined(i386) || defined(__i386__)
    if (xf86GetOptValBool(FlagOptions, FLAG_PC98, &value)) {
	xf86Info.pc98 = value;
	if (value) {
	    xf86Msg(X_CONFIG, "Japanese PC98 architecture\n");
	}
    } else
	if (detectPC98()) {
	    xf86Info.pc98 = TRUE;
	    xf86Msg(X_PROBED, "Japanese PC98 architecture\n");
	}
#endif

#ifdef PANORAMIX
    from = X_DEFAULT;
    if (!noPanoramiXExtension)
      from = X_CMDLINE;
    else if (xf86GetOptValBool(FlagOptions, FLAG_XINERAMA, &value)) {
      noPanoramiXExtension = !value;
      from = X_CONFIG;
    }
    if (!noPanoramiXExtension)
      xf86Msg(from, "Xinerama: enabled\n");
#endif

    return TRUE;
}

/*
 * XXX This function is temporary, and will be removed when the keyboard
 * driver is converted into a regular input driver.
 */
static Bool
configInputKbd(IDevPtr inputp)
{
  char *s;
  MessageType from = X_DEFAULT;
  Bool customKeycodesDefault = FALSE;
  int verb = 0;

  /* Initialize defaults */
  xf86Info.xleds         = 0L;
  xf86Info.kbdDelay      = 500;
  xf86Info.kbdRate       = 30;
  
  xf86Info.kbdProc       = NULL;
  xf86Info.vtinit        = NULL;
  xf86Info.vtSysreq      = VT_SYSREQ_DEFAULT;
#if defined(SVR4) && defined(i386)
  xf86Info.panix106      = FALSE;
#endif
  xf86Info.kbdCustomKeycodes = FALSE;
#ifdef WSCONS_SUPPORT
  xf86Info.kbdFd 	   = -1;
#endif
#ifdef XKB
  if (!xf86IsPc98()) {
    xf86Info.xkbrules      = "xfree86";
    xf86Info.xkbmodel      = "pc105";
    xf86Info.xkblayout     = "us";
    xf86Info.xkbvariant    = NULL;
    xf86Info.xkboptions    = NULL;
  } else {
    xf86Info.xkbrules      = "xfree98";
    xf86Info.xkbmodel      = "pc98";
    xf86Info.xkblayout     = "nec/jp";
    xf86Info.xkbvariant    = NULL;
    xf86Info.xkboptions    = NULL;
  }
  xf86Info.xkbcomponents_specified = FALSE;
  /* Should discourage the use of these. */
  xf86Info.xkbkeymap     = NULL;
  xf86Info.xkbtypes      = NULL;
  xf86Info.xkbcompat     = NULL;
  xf86Info.xkbkeycodes   = NULL;
  xf86Info.xkbsymbols    = NULL;
  xf86Info.xkbgeometry   = NULL;
#endif

  s = xf86SetStrOption(inputp->commonOptions, "Protocol", "standard");
  if (xf86NameCmp(s, "standard") == 0) {
     xf86Info.kbdProc    = xf86KbdProc;
     xf86Info.kbdEvents  = xf86KbdEvents;
     xfree(s);
  } else if (xf86NameCmp(s, "xqueue") == 0) {
#ifdef XQUEUE
    xf86Info.kbdProc = xf86XqueKbdProc;
    xf86Info.kbdEvents = xf86XqueEvents;
    xf86Msg(X_CONFIG, "Xqueue selected for keyboard input\n");
#endif
    xfree(s);
#ifdef WSCONS_SUPPORT
  } else if (xf86NameCmp(s, "wskbd") == 0) {
     xf86Info.kbdProc    = xf86KbdProc;
     xf86Info.kbdEvents  = xf86WSKbdEvents;
     xfree(s);
     s = xf86SetStrOption(inputp->commonOptions, "Device", NULL);
     xf86Msg(X_CONFIG, "Keyboard: Protocol: wskbd\n");
     if (s == NULL) {
	 xf86ConfigError("A \"device\" option is required with"
			 " the \"wskbd\" keyboard protocol");
	 return FALSE;
     }
     xf86Info.kbdFd = open(s, O_RDWR | O_NONBLOCK | O_EXCL);
     if (xf86Info.kbdFd == -1) {
       xf86ConfigError("cannot open \"%s\"", s);
       xfree(s);
       return FALSE;
     }
     xfree(s);
     /* Find out keyboard type */
     if (ioctl(xf86Info.kbdFd, WSKBDIO_GTYPE, &xf86Info.wsKbdType) == -1) {
	     xf86ConfigError("cannot get keyboard type");
	     close(xf86Info.kbdFd);
	     return FALSE;
     }
     switch (xf86Info.wsKbdType) {
     case WSKBD_TYPE_PC_XT:
	     xf86Msg(X_PROBED, "Keyboard type: XT\n");
	     break;
     case WSKBD_TYPE_PC_AT:
	     xf86Msg(X_PROBED, "Keyboard type: AT\n");
	     break;
     case WSKBD_TYPE_USB:
	     xf86Msg(X_PROBED, "Keyboard type: USB\n");
	     break;
#ifdef WSKBD_TYPE_ADB
     case WSKBD_TYPE_ADB:
	     xf86Msg(X_PROBED, "Keyboard type: ADB\n");
	     break;
#endif
#ifdef WSKBD_TYPE_SUN
     case WSKBD_TYPE_SUN:
	     xf86Msg(X_PROBED, "Keyboard type: Sun\n");
	     break;
#endif
#ifdef WSKBD_TYPE_SUN5
     case WSKBD_TYPE_SUN5:
	     xf86Msg(X_PROBED, "Keyboard type: Sun5\n");
	     break;
#endif
     default:
	     xf86ConfigError("Unsupported wskbd type \"%d\"", 
			     xf86Info.wsKbdType);
	     close(xf86Info.kbdFd);
	     return FALSE;
     }
#endif
  } else {
    xf86ConfigError("\"%s\" is not a valid keyboard protocol name", s);
    xfree(s);
    return FALSE;
  }

  s = xf86SetStrOption(inputp->commonOptions, "AutoRepeat", NULL);
  if (s) {
    if (sscanf(s, "%d %d", &xf86Info.kbdDelay, &xf86Info.kbdRate) != 2) {
      xf86ConfigError("\"%s\" is not a valid AutoRepeat value", s);
      xfree(s);
      return FALSE;
    }
  xfree(s);
  }

  s = xf86SetStrOption(inputp->commonOptions, "XLeds", NULL);
  if (s) {
    char *l, *end;
    unsigned int i;
    l = strtok(s, " \t\n");
    while (l) {
      i = strtoul(l, &end, 0);
      if (*end == '\0')
	xf86Info.xleds |= 1L << (i - 1);
      else {
	xf86ConfigError("\"%s\" is not a valid XLeds value", l);
	xfree(s);
	return FALSE;
      }
      l = strtok(NULL, " \t\n");
    }
    xfree(s);
  }

#ifdef XKB
  from = X_DEFAULT;
  if (noXkbExtension)
    from = X_CMDLINE;
  else if (xf86FindOption(inputp->commonOptions, "XkbDisable")) {
    xf86Msg(X_WARNING, "KEYBOARD: XKB should be disabled in the "
	    "ServerFlags section instead\n"
	    "\tof in the \"keyboard\" InputDevice section.\n");
    noXkbExtension =
	xf86SetBoolOption(inputp->commonOptions, "XkbDisable", FALSE);
    from = X_CONFIG;
  }
  if (noXkbExtension)
    xf86Msg(from, "XKB: disabled\n");

#define NULL_IF_EMPTY(s) (s[0] ? s : (xfree(s), (char *)NULL))

  if (!noXkbExtension && !XkbInitialMap) {
    if ((s = xf86SetStrOption(inputp->commonOptions, "XkbKeymap", NULL))) {
      xf86Info.xkbkeymap = NULL_IF_EMPTY(s);
      xf86Msg(X_CONFIG, "XKB: keymap: \"%s\" "
		"(overrides other XKB settings)\n", xf86Info.xkbkeymap);
    } else {
      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbCompat", NULL))) {
	xf86Info.xkbcompat = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: compat: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbTypes", NULL))) {
	xf86Info.xkbtypes = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: types: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbKeycodes", NULL))) {
	xf86Info.xkbkeycodes = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: keycodes: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbGeometry", NULL))) {
	xf86Info.xkbgeometry = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: geometry: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbSymbols", NULL))) {
	xf86Info.xkbsymbols = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: symbols: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbRules", NULL))) {
	xf86Info.xkbrules = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: rules: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbModel", NULL))) {
	xf86Info.xkbmodel = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: model: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbLayout", NULL))) {
	xf86Info.xkblayout = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: layout: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbVariant", NULL))) {
	xf86Info.xkbvariant = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: variant: \"%s\"\n", s);
      }

      if ((s = xf86SetStrOption(inputp->commonOptions, "XkbOptions", NULL))) {
	xf86Info.xkboptions = NULL_IF_EMPTY(s);
	xf86Info.xkbcomponents_specified = TRUE;
	xf86Msg(X_CONFIG, "XKB: options: \"%s\"\n", s);
      }
    }
  }
#undef NULL_IF_EMPTY
#endif
#if defined(SVR4) && defined(i386)
  if ((xf86Info.panix106 =
	xf86SetBoolOption(inputp->commonOptions, "Panix106", FALSE))) {
    xf86Msg(X_CONFIG, "PANIX106: enabled\n");
  }
#endif

  /*
   * This was once a compile time option (ASSUME_CUSTOM_KEYCODES)
   * defaulting to 1 on Linux/PPC. It is no longer necessary, but for
   * backwards compatibility we provide 'Option "CustomKeycodes"'
   * and try to autoprobe on Linux/PPC.
   */
  from = X_DEFAULT;
  verb = 2;
#if defined(__linux__) && defined(__powerpc__)
  {
    FILE *f;

    f = fopen("/proc/sys/dev/mac_hid/keyboard_sends_linux_keycodes","r");
    if (f) {
      if (fgetc(f) == '0') {
	customKeycodesDefault = TRUE;
	from = X_PROBED;
	verb = 1;
      }
      fclose(f);
    }
  }
#endif
  if (xf86FindOption(inputp->commonOptions, "CustomKeycodes")) {
    from = X_CONFIG;
    verb = 1;
  }
  xf86Info.kbdCustomKeycodes =
	xf86SetBoolOption(inputp->commonOptions, "CustomKeycodes",
			  customKeycodesDefault);
  xf86MsgVerb(from, verb, "Keyboard: CustomKeycode %s\n",
		xf86Info.kbdCustomKeycodes ? "enabled" : "disabled");

  return TRUE;
}

/*
 * Locate the core input devices.  These can be specified/located in
 * the following ways, in order of priority:
 *
 *  1. The InputDevices named by the -pointer and -keyboard command line
 *     options.
 *  2. The "CorePointer" and "CoreKeyboard" InputDevices referred to by
 *     the active ServerLayout.
 *  3. The first InputDevices marked as "CorePointer" and "CoreKeyboard".
 *  4. The first InputDevices that use the 'mouse' and 'keyboard' or 'kbd'
 *     drivers.
 *  5. Default devices with an empty (default) configuration.  These defaults
 *     will reference the 'mouse' and 'keyboard' drivers.
 */

static Bool
checkCoreInputDevices(serverLayoutPtr servlayoutp, Bool implicitLayout)
{
    IDevPtr corePointer = NULL, coreKeyboard = NULL;
    Bool foundPointer = FALSE, foundKeyboard = FALSE;
    const char *pointerMsg = NULL, *keyboardMsg = NULL;
    IDevPtr indp;
    IDevRec Pointer, Keyboard;
    XF86ConfInputPtr confInput;
    XF86ConfInputRec defPtr, defKbd;
    int count = 0;
    MessageType from = X_DEFAULT;

    /*
     * First check if a core pointer or core keyboard have been specified
     * in the active ServerLayout.  If more than one is specified for either,
     * remove the core attribute from the later ones.
     */
    for (indp = servlayoutp->inputs; indp->identifier; indp++) {
	pointer opt1 = NULL, opt2 = NULL;
	if (indp->commonOptions &&
	    xf86CheckBoolOption(indp->commonOptions, "CorePointer", FALSE)) {
	    opt1 = indp->commonOptions;
	}
	if (indp->extraOptions &&
	    xf86CheckBoolOption(indp->extraOptions, "CorePointer", FALSE)) {
	    opt2 = indp->extraOptions;
	}
	if (opt1 || opt2) {
	    if (!corePointer) {
		corePointer = indp;
	    } else {
		if (opt1)
		    xf86ReplaceBoolOption(opt1, "CorePointer", FALSE);
		if (opt2)
		    xf86ReplaceBoolOption(opt2, "CorePointer", FALSE);
		xf86Msg(X_WARNING, "Duplicate core pointer devices.  "
			"Removing core pointer attribute from \"%s\"\n",
			indp->identifier);
	    }
	}
	opt1 = opt2 = NULL;
	if (indp->commonOptions &&
	    xf86CheckBoolOption(indp->commonOptions, "CoreKeyboard", FALSE)) {
	    opt1 = indp->commonOptions;
	}
	if (indp->extraOptions &&
	    xf86CheckBoolOption(indp->extraOptions, "CoreKeyboard", FALSE)) {
	    opt2 = indp->extraOptions;
	}
	if (opt1 || opt2) {
	    if (!coreKeyboard) {
		coreKeyboard = indp;
	    } else {
		if (opt1)
		    xf86ReplaceBoolOption(opt1, "CoreKeyboard", FALSE);
		if (opt2)
		    xf86ReplaceBoolOption(opt2, "CoreKeyboard", FALSE);
		xf86Msg(X_WARNING, "Duplicate core keyboard devices.  "
			"Removing core keyboard attribute from \"%s\"\n",
			indp->identifier);
	    }
	}
	count++;
    }

    confInput = NULL;

    /* 1. Check for the -pointer command line option. */
    if (xf86PointerName) {
	confInput = xf86findInput(xf86PointerName,
				  xf86configptr->conf_input_lst);
	if (!confInput) {
	    xf86Msg(X_ERROR, "No InputDevice section called \"%s\"\n",
		    xf86PointerName);
	    return FALSE;
	}
	from = X_CMDLINE;
	/*
	 * If one was already specified in the ServerLayout, it needs to be
	 * removed.
	 */
	if (corePointer) {
	    for (indp = servlayoutp->inputs; indp->identifier; indp++)
		if (indp == corePointer)
		    break;
	    for (; indp->identifier; indp++)
		indp[0] = indp[1];
	    count--;
	}
	corePointer = NULL;
	foundPointer = TRUE;
    }

    /* 2. ServerLayout-specified core pointer. */
    if (corePointer) {
	foundPointer = TRUE;
	from = X_CONFIG;
    }

    /* 3. First core pointer device. */
    if (!foundPointer) {
	XF86ConfInputPtr p;

	for (p = xf86configptr->conf_input_lst; p; p = p->list.next) {
	    if (p->inp_option_lst &&
		xf86CheckBoolOption(p->inp_option_lst, "CorePointer", FALSE)) {
		confInput = p;
		foundPointer = TRUE;
		from = X_DEFAULT;
		pointerMsg = "first core pointer device";
		break;
	    }
	}
    }

    /* 4. First pointer with 'mouse' as the driver. */
    if (!foundPointer) {
	confInput = xf86findInput(CONF_IMPLICIT_POINTER,
				  xf86configptr->conf_input_lst);
	if (!confInput) {
	    confInput = xf86findInputByDriver("mouse",
					      xf86configptr->conf_input_lst);
	}
	if (confInput) {
	    foundPointer = TRUE;
	    from = X_DEFAULT;
	    pointerMsg = "first mouse device";
	}
    }

    /* 5. Built-in default. */
    if (!foundPointer) {
	bzero(&defPtr, sizeof(defPtr));
	defPtr.inp_identifier = "<default pointer>";
	defPtr.inp_driver = "mouse";
	confInput = &defPtr;
	foundPointer = TRUE;
	from = X_DEFAULT;
	pointerMsg = "default mouse configuration";
    }

    /* Add the core pointer device to the layout, and set it to Core. */
    if (foundPointer && confInput) {
	foundPointer = configInput(&Pointer, confInput, from);
        if (foundPointer) {
	    count++;
	    indp = xnfrealloc(servlayoutp->inputs,
			      (count + 1) * sizeof(IDevRec));
	    indp[count - 1] = Pointer;
	    indp[count - 1].extraOptions =
				xf86addNewOption(NULL, "CorePointer", NULL);
	    indp[count].identifier = NULL;
	    servlayoutp->inputs = indp;
	}
    }

    if (!foundPointer) {
	/* This shouldn't happen. */
	xf86Msg(X_ERROR, "Cannot locate a core pointer device.\n");
	return FALSE;
    }

    confInput = NULL;

    /* 1. Check for the -keyboard command line option. */
    if (xf86KeyboardName) {
	confInput = xf86findInput(xf86KeyboardName,
				  xf86configptr->conf_input_lst);
	if (!confInput) {
	    xf86Msg(X_ERROR, "No InputDevice section called \"%s\"\n",
		    xf86KeyboardName);
	    return FALSE;
	}
	from = X_CMDLINE;
	/*
	 * If one was already specified in the ServerLayout, it needs to be
	 * removed.
	 */
	if (coreKeyboard) {
	    for (indp = servlayoutp->inputs; indp->identifier; indp++)
		if (indp == coreKeyboard)
		    break;
	    for (; indp->identifier; indp++)
		indp[0] = indp[1];
	    count--;
	}
	coreKeyboard = NULL;
	foundKeyboard = TRUE;
    }

    /* 2. ServerLayout-specified core keyboard. */
    if (coreKeyboard) {
	foundKeyboard = TRUE;
	from = X_CONFIG;
    }

    /* 3. First core keyboard device. */
    if (!foundKeyboard) {
	XF86ConfInputPtr p;

	for (p = xf86configptr->conf_input_lst; p; p = p->list.next) {
	    if (p->inp_option_lst &&
		xf86CheckBoolOption(p->inp_option_lst, "CoreKeyboard", FALSE)) {
		confInput = p;
		foundKeyboard = TRUE;
		from = X_DEFAULT;
		keyboardMsg = "first core keyboard device";
		break;
	    }
	}
    }

    /* 4. First keyboard with 'keyboard' or 'kbd' as the driver. */
    if (!foundKeyboard) {
	confInput = xf86findInput(CONF_IMPLICIT_KEYBOARD,
				  xf86configptr->conf_input_lst);
	if (!confInput) {
	    confInput = xf86findInputByDriver("keyboard",
					      xf86configptr->conf_input_lst);
	}
	if (!confInput) {
	    confInput = xf86findInputByDriver("kbd",
					      xf86configptr->conf_input_lst);
	}
	if (confInput) {
	    foundKeyboard = TRUE;
	    from = X_DEFAULT;
	    pointerMsg = "first keyboard device";
	}
    }

    /* 5. Built-in default. */
    if (!foundKeyboard) {
	bzero(&defKbd, sizeof(defKbd));
	defKbd.inp_identifier = "<default keyboard>";
	defKbd.inp_driver = "keyboard";
	confInput = &defKbd;
	foundKeyboard = TRUE;
	keyboardMsg = "default keyboard configuration";
	from = X_DEFAULT;
    }

    /* Add the core keyboard device to the layout, and set it to Core. */
    if (foundKeyboard && confInput) {
	foundKeyboard = configInput(&Keyboard, confInput, from);
        if (foundKeyboard) {
	    count++;
	    indp = xnfrealloc(servlayoutp->inputs,
			      (count + 1) * sizeof(IDevRec));
	    indp[count - 1] = Keyboard;
	    indp[count - 1].extraOptions =
				xf86addNewOption(NULL, "CoreKeyboard", NULL);
	    indp[count].identifier = NULL;
	    servlayoutp->inputs = indp;
	}
    }

    if (!foundKeyboard) {
	/* This shouldn't happen. */
	xf86Msg(X_ERROR, "Cannot locate a core keyboard device.\n");
	return FALSE;
    }

    if (pointerMsg) {
	xf86Msg(X_WARNING, "The core pointer device wasn't specified "
		"explicitly in the layout.\n"
		"\tUsing the %s.\n", pointerMsg);
    }

    if (keyboardMsg) {
	xf86Msg(X_WARNING, "The core keyboard device wasn't specified "
		"explicitly in the layout.\n"
		"\tUsing the %s.\n", keyboardMsg);
    }

    return TRUE;
}

/*
 * figure out which layout is active, which screens are used in that layout,
 * which drivers and monitors are used in these screens
 */
static Bool
configLayout(serverLayoutPtr servlayoutp, XF86ConfLayoutPtr conf_layout,
	     char *default_layout)
{
    XF86ConfAdjacencyPtr adjp;
    XF86ConfInactivePtr idp;
    XF86ConfInputrefPtr irp;
    int count = 0;
    int scrnum;
    XF86ConfLayoutPtr l;
    MessageType from;
    screenLayoutPtr slp;
    GDevPtr gdp;
    IDevPtr indp;
    int i = 0, j;

    if (!servlayoutp)
	return FALSE;

    /*
     * which layout section is the active one?
     *
     * If there is a -layout command line option, use that one, otherwise
     * pick the first one.
     */
    from = X_DEFAULT;
    if (xf86LayoutName != NULL)
	from = X_CMDLINE;
    else if (default_layout) {
	xf86LayoutName = default_layout;
	from = X_CONFIG;
    }
    if (xf86LayoutName != NULL) {
	if ((l = xf86findLayout(xf86LayoutName, conf_layout)) == NULL) {
	    xf86Msg(X_ERROR, "No ServerLayout section called \"%s\"\n",
		    xf86LayoutName);
	    return FALSE;
	}
	conf_layout = l;
    }
    xf86Msg(from, "ServerLayout \"%s\"\n", conf_layout->lay_identifier);
    adjp = conf_layout->lay_adjacency_lst;

    /*
     * we know that each screen is referenced exactly once on the left side
     * of a layout statement in the Layout section. So to allocate the right
     * size for the array we do a quick walk of the list to figure out how
     * many sections we have
     */
    while (adjp) {
        count++;
        adjp = (XF86ConfAdjacencyPtr)adjp->list.next;
    }
#ifdef DEBUG
    ErrorF("Found %d screens in the layout section %s",
           count, conf_layout->lay_identifier);
#endif
    slp = xnfcalloc(1, (count + 1) * sizeof(screenLayoutRec));
    slp[count].screen = NULL;
    /*
     * now that we have storage, loop over the list again and fill in our
     * data structure; at this point we do not fill in the adjacency
     * information as it is not clear if we need it at all
     */
    adjp = conf_layout->lay_adjacency_lst;
    count = 0;
    while (adjp) {
        slp[count].screen = xnfcalloc(1, sizeof(confScreenRec));
	if (adjp->adj_scrnum < 0)
	    scrnum = count;
	else
	    scrnum = adjp->adj_scrnum;
	if (!configScreen(slp[count].screen, adjp->adj_screen, scrnum,
			  X_CONFIG))
	    return FALSE;
	slp[count].x = adjp->adj_x;
	slp[count].y = adjp->adj_y;
	slp[count].refname = adjp->adj_refscreen;
	switch (adjp->adj_where) {
	case CONF_ADJ_OBSOLETE:
	    slp[count].where = PosObsolete;
	    slp[count].topname = adjp->adj_top_str;
	    slp[count].bottomname = adjp->adj_bottom_str;
	    slp[count].leftname = adjp->adj_left_str;
	    slp[count].rightname = adjp->adj_right_str;
	    break;
	case CONF_ADJ_ABSOLUTE:
	    slp[count].where = PosAbsolute;
	    break;
	case CONF_ADJ_RIGHTOF:
	    slp[count].where = PosRightOf;
	    break;
	case CONF_ADJ_LEFTOF:
	    slp[count].where = PosLeftOf;
	    break;
	case CONF_ADJ_ABOVE:
	    slp[count].where = PosAbove;
	    break;
	case CONF_ADJ_BELOW:
	    slp[count].where = PosBelow;
	    break;
	case CONF_ADJ_RELATIVE:
	    slp[count].where = PosRelative;
	    break;
	}
        count++;
        adjp = (XF86ConfAdjacencyPtr)adjp->list.next;
    }

    /* XXX Need to tie down the upper left screen. */

    /* Fill in the refscreen and top/bottom/left/right values */
    for (i = 0; i < count; i++) {
	for (j = 0; j < count; j++) {
	    if (slp[i].refname &&
		strcmp(slp[i].refname, slp[j].screen->id) == 0) {
		slp[i].refscreen = slp[j].screen;
	    }
	    if (slp[i].topname &&
		strcmp(slp[i].topname, slp[j].screen->id) == 0) {
		slp[i].top = slp[j].screen;
	    }
	    if (slp[i].bottomname &&
		strcmp(slp[i].bottomname, slp[j].screen->id) == 0) {
		slp[i].bottom = slp[j].screen;
	    }
	    if (slp[i].leftname &&
		strcmp(slp[i].leftname, slp[j].screen->id) == 0) {
		slp[i].left = slp[j].screen;
	    }
	    if (slp[i].rightname &&
		strcmp(slp[i].rightname, slp[j].screen->id) == 0) {
		slp[i].right = slp[j].screen;
	    }
	}
	if (slp[i].where != PosObsolete
	    && slp[i].where != PosAbsolute
	    && !slp[i].refscreen) {
	    xf86Msg(X_ERROR,"Screen %s doesn't exist: deleting placement\n",
		     slp[i].refname);
	    slp[i].where = PosAbsolute;
	    slp[i].x = 0;
	    slp[i].y = 0;
	}
    }

#ifdef LAYOUT_DEBUG
    ErrorF("Layout \"%s\"\n", conf_layout->lay_identifier);
    for (i = 0; i < count; i++) {
	ErrorF("Screen: \"%s\" (%d):\n", slp[i].screen->id,
	       slp[i].screen->screennum);
	switch (slp[i].where) {
	case PosObsolete:
	    ErrorF("\tObsolete format: \"%s\" \"%s\" \"%s\" \"%s\"\n",
		   slp[i].top, slp[i].bottom, slp[i].left, slp[i].right);
	    break;
	case PosAbsolute:
	    if (slp[i].x == -1)
		if (slp[i].screen->screennum == 0)
		    ErrorF("\tImplicitly left-most\n");
		else
		    ErrorF("\tImplicitly right of screen %d\n",
			   slp[i].screen->screennum - 1);
	    else
		ErrorF("\t%d %d\n", slp[i].x, slp[i].y);
	    break;
	case PosRightOf:
	    ErrorF("\tRight of \"%s\"\n", slp[i].refscreen->id);
	    break;
	case PosLeftOf:
	    ErrorF("\tLeft of \"%s\"\n", slp[i].refscreen->id);
	    break;
	case PosAbove:
	    ErrorF("\tAbove \"%s\"\n", slp[i].refscreen->id);
	    break;
	case PosBelow:
	    ErrorF("\tBelow \"%s\"\n", slp[i].refscreen->id);
	    break;
	case PosRelative:
	    ErrorF("\t%d %d relative to \"%s\"\n", slp[i].x, slp[i].y,
		   slp[i].refscreen->id);
	    break;
	}
    }
#endif
    /*
     * Count the number of inactive devices.
     */
    count = 0;
    idp = conf_layout->lay_inactive_lst;
    while (idp) {
        count++;
        idp = (XF86ConfInactivePtr)idp->list.next;
    }
#ifdef DEBUG
    ErrorF("Found %d inactive devices in the layout section %s",
           count, conf_layout->lay_identifier);
#endif
    gdp = xnfalloc((count + 1) * sizeof(GDevRec));
    gdp[count].identifier = NULL;
    idp = conf_layout->lay_inactive_lst;
    count = 0;
    while (idp) {
	if (!configDevice(&gdp[count], idp->inactive_device, FALSE))
	    return FALSE;
        count++;
        idp = (XF86ConfInactivePtr)idp->list.next;
    }
    /*
     * Count the number of input devices.
     */
    count = 0;
    irp = conf_layout->lay_input_lst;
    while (irp) {
        count++;
        irp = (XF86ConfInputrefPtr)irp->list.next;
    }
#ifdef DEBUG
    ErrorF("Found %d input devices in the layout section %s",
           count, conf_layout->lay_identifier);
#endif
    indp = xnfalloc((count + 1) * sizeof(IDevRec));
    indp[count].identifier = NULL;
    irp = conf_layout->lay_input_lst;
    count = 0;
    while (irp) {
	if (!configInput(&indp[count], irp->iref_inputdev, X_CONFIG))
	    return FALSE;
	indp[count].extraOptions = irp->iref_option_lst;
        count++;
        irp = (XF86ConfInputrefPtr)irp->list.next;
    }
    servlayoutp->id = conf_layout->lay_identifier;
    servlayoutp->screens = slp;
    servlayoutp->inactives = gdp;
    servlayoutp->inputs = indp;
    servlayoutp->options = conf_layout->lay_option_lst;
    from = X_DEFAULT;

    if (!checkCoreInputDevices(servlayoutp, FALSE))
	return FALSE;
    return TRUE;
}

/*
 * No layout section, so find the first Screen section and set that up as
 * the only active screen.
 */
static Bool
configImpliedLayout(serverLayoutPtr servlayoutp, XF86ConfScreenPtr conf_screen)
{
    MessageType from;
    XF86ConfScreenPtr s;
    screenLayoutPtr slp;
    IDevPtr indp;

    if (!servlayoutp)
	return FALSE;

    if (conf_screen == NULL) {
	xf86ConfigError("No Screen sections present\n");
	return FALSE;
    }

    /*
     * which screen section is the active one?
     *
     * If there is a -screen option, use that one, otherwise use the first
     * one.
     */

    from = X_CONFIG;
    if (xf86ScreenName != NULL) {
	if ((s = xf86findScreen(xf86ScreenName, conf_screen)) == NULL) {
	    xf86Msg(X_ERROR, "No Screen section called \"%s\"\n",
		    xf86ScreenName);
	    return FALSE;
	}
	conf_screen = s;
	from = X_CMDLINE;
    }

    /* We have exactly one screen */

    slp = xnfcalloc(1, 2 * sizeof(screenLayoutRec));
    slp[0].screen = xnfcalloc(1, sizeof(confScreenRec));
    slp[1].screen = NULL;
    if (!configScreen(slp[0].screen, conf_screen, 0, from))
	return FALSE;
    servlayoutp->id = "(implicit)";
    servlayoutp->screens = slp;
    servlayoutp->inactives = xnfcalloc(1, sizeof(GDevRec));
    servlayoutp->options = NULL;
    /* Set up an empty input device list, then look for some core devices. */
    indp = xnfalloc(sizeof(IDevRec));
    indp->identifier = NULL;
    servlayoutp->inputs = indp;
    if (!checkCoreInputDevices(servlayoutp, TRUE))
	return FALSE;
    
    return TRUE;
}

static Bool
configXvAdaptor(confXvAdaptorPtr adaptor, XF86ConfVideoAdaptorPtr conf_adaptor)
{
    int count = 0;
    XF86ConfVideoPortPtr conf_port;

    xf86Msg(X_CONFIG, "|   |-->VideoAdaptor \"%s\"\n",
	    conf_adaptor->va_identifier);
    adaptor->identifier = conf_adaptor->va_identifier;
    adaptor->options = conf_adaptor->va_option_lst;
    if (conf_adaptor->va_busid || conf_adaptor->va_driver) {
	xf86Msg(X_CONFIG, "|   | Unsupported device type, skipping entry\n");
	return FALSE;
    }

    /*
     * figure out how many videoport subsections there are and fill them in
     */
    conf_port = conf_adaptor->va_port_lst;
    while(conf_port) {
        count++;
        conf_port = (XF86ConfVideoPortPtr)conf_port->list.next;
    }
    adaptor->ports = xnfalloc((count) * sizeof(confXvPortRec));
    adaptor->numports = count;
    count = 0;
    conf_port = conf_adaptor->va_port_lst;
    while(conf_port) {
	adaptor->ports[count].identifier = conf_port->vp_identifier;
	adaptor->ports[count].options = conf_port->vp_option_lst;
        count++;
        conf_port = (XF86ConfVideoPortPtr)conf_port->list.next;
    }

    return TRUE;
}

static Bool
configScreen(confScreenPtr screenp, XF86ConfScreenPtr conf_screen, int scrnum,
	     MessageType from)
{
    int count = 0;
    XF86ConfDisplayPtr dispptr;
    XF86ConfAdaptorLinkPtr conf_adaptor;
    Bool defaultMonitor = FALSE;

    xf86Msg(from, "|-->Screen \"%s\" (%d)\n", conf_screen->scrn_identifier,
	    scrnum);
    /*
     * now we fill in the elements of the screen
     */
    screenp->id         = conf_screen->scrn_identifier;
    screenp->screennum  = scrnum;
    screenp->defaultdepth = conf_screen->scrn_defaultdepth;
    screenp->defaultbpp = conf_screen->scrn_defaultbpp;
    screenp->defaultfbbpp = conf_screen->scrn_defaultfbbpp;
    screenp->monitor    = xnfcalloc(1, sizeof(MonRec));
    /* If no monitor is specified, create a default one. */
    if (!conf_screen->scrn_monitor) {
	XF86ConfMonitorRec defMon;

	bzero(&defMon, sizeof(defMon));
	defMon.mon_identifier = "<default monitor>";
	/*
	 * TARGET_REFRESH_RATE may be defined to effectively limit the
	 * default resolution to the largest that has a "good" refresh
	 * rate.
	 */
#ifdef TARGET_REFRESH_RATE
	defMon.mon_option_lst = xf86ReplaceRealOption(defMon.mon_option_lst,
						      "TargetRefresh",
						      TARGET_REFRESH_RATE);
#endif
	if (!configMonitor(screenp->monitor, &defMon))
	    return FALSE;
	defaultMonitor = TRUE;
    } else {
	if (!configMonitor(screenp->monitor,conf_screen->scrn_monitor))
	    return FALSE;
    }
    screenp->device     = xnfcalloc(1, sizeof(GDevRec));
    configDevice(screenp->device,conf_screen->scrn_device, TRUE);
    screenp->device->myScreenSection = screenp;
    screenp->options = conf_screen->scrn_option_lst;
    
    /*
     * figure out how many display subsections there are and fill them in
     */
    dispptr = conf_screen->scrn_display_lst;
    while(dispptr) {
        count++;
        dispptr = (XF86ConfDisplayPtr)dispptr->list.next;
    }
    screenp->displays   = xnfalloc((count) * sizeof(DispRec));
    screenp->numdisplays = count;
    count = 0;
    dispptr = conf_screen->scrn_display_lst;
    while(dispptr) {
        configDisplay(&(screenp->displays[count]),dispptr);
        count++;
        dispptr = (XF86ConfDisplayPtr)dispptr->list.next;
    }

    /*
     * figure out how many videoadaptor references there are and fill them in
     */
    conf_adaptor = conf_screen->scrn_adaptor_lst;
    while(conf_adaptor) {
        count++;
        conf_adaptor = (XF86ConfAdaptorLinkPtr)conf_adaptor->list.next;
    }
    screenp->xvadaptors = xnfalloc((count) * sizeof(confXvAdaptorRec));
    screenp->numxvadaptors = 0;
    conf_adaptor = conf_screen->scrn_adaptor_lst;
    while(conf_adaptor) {
        if (configXvAdaptor(&(screenp->xvadaptors[screenp->numxvadaptors]),
			    conf_adaptor->al_adaptor))
    	    screenp->numxvadaptors++;
        conf_adaptor = (XF86ConfAdaptorLinkPtr)conf_adaptor->list.next;
    }

    if (defaultMonitor) {
	xf86Msg(X_WARNING, "No monitor specified for screen \"%s\".\n"
		"\tUsing a default monitor configuration.\n", screenp->id);
    }
    return TRUE;
}

static Bool
configMonitor(MonPtr monitorp, XF86ConfMonitorPtr conf_monitor)
{
    int count;
    DisplayModePtr mode,last = NULL;
    XF86ConfModeLinePtr cmodep;
    XF86ConfModesPtr modes;
    XF86ConfModesLinkPtr modeslnk = conf_monitor->mon_modes_sect_lst;
    Gamma zeros = {0.0, 0.0, 0.0};
    float badgamma = 0.0;
    
    xf86Msg(X_CONFIG, "|   |-->Monitor \"%s\"\n",
	    conf_monitor->mon_identifier);
    monitorp->id = conf_monitor->mon_identifier;
    monitorp->vendor = conf_monitor->mon_vendor;
    monitorp->model = conf_monitor->mon_modelname;
    monitorp->Modes = NULL;
    monitorp->Last = NULL;
    monitorp->gamma = zeros;
    monitorp->widthmm = conf_monitor->mon_width;
    monitorp->heightmm = conf_monitor->mon_height;
    monitorp->options = conf_monitor->mon_option_lst;

    /*
     * fill in the monitor structure
     */    
    for( count = 0 ; count < conf_monitor->mon_n_hsync; count++) {
        monitorp->hsync[count].hi = conf_monitor->mon_hsync[count].hi;
        monitorp->hsync[count].lo = conf_monitor->mon_hsync[count].lo;
    }
    monitorp->nHsync = conf_monitor->mon_n_hsync;
    for( count = 0 ; count < conf_monitor->mon_n_vrefresh; count++) {
        monitorp->vrefresh[count].hi = conf_monitor->mon_vrefresh[count].hi;
        monitorp->vrefresh[count].lo = conf_monitor->mon_vrefresh[count].lo;
    }
    monitorp->nVrefresh = conf_monitor->mon_n_vrefresh;

    /*
     * first we collect the mode lines from the UseModes directive
     */
    while(modeslnk)
    {
        modes = xf86findModes (modeslnk->ml_modes_str, 
			       xf86configptr->conf_modes_lst);
	modeslnk->ml_modes = modes;
	
	    
	/* now add the modes found in the modes
	   section to the list of modes for this
	   monitor unless it has been added before
	   because we are reusing the same section 
	   for another screen */
	if (xf86itemNotSublist(
			       (GenericListPtr)conf_monitor->mon_modeline_lst,
			       (GenericListPtr)modes->mon_modeline_lst)) {
	    conf_monitor->mon_modeline_lst = (XF86ConfModeLinePtr)
	        xf86addListItem(
				(GenericListPtr)conf_monitor->mon_modeline_lst,
				(GenericListPtr)modes->mon_modeline_lst);
	}
	modeslnk = modeslnk->list.next;
    }

    /*
     * we need to hook in the mode lines now
     * here both data structures use lists, only our internal one
     * is double linked
     */
    cmodep = conf_monitor->mon_modeline_lst;
    while( cmodep ) {
        mode = xnfalloc(sizeof(DisplayModeRec));
        memset(mode,'\0',sizeof(DisplayModeRec));
	mode->type       = 0;
        mode->Clock      = cmodep->ml_clock;
        mode->HDisplay   = cmodep->ml_hdisplay;
        mode->HSyncStart = cmodep->ml_hsyncstart;
        mode->HSyncEnd   = cmodep->ml_hsyncend;
        mode->HTotal     = cmodep->ml_htotal;
        mode->VDisplay   = cmodep->ml_vdisplay;
        mode->VSyncStart = cmodep->ml_vsyncstart;
        mode->VSyncEnd   = cmodep->ml_vsyncend;
        mode->VTotal     = cmodep->ml_vtotal;
        mode->Flags      = cmodep->ml_flags;
        mode->HSkew      = cmodep->ml_hskew;
        mode->VScan      = cmodep->ml_vscan;
        mode->name       = xnfstrdup(cmodep->ml_identifier);
        if( last ) {
            mode->prev = last;
            last->next = mode;
        }
        else {
            /*
             * this is the first mode
             */
            monitorp->Modes = mode;
            mode->prev = NULL;
        }
        last = mode;
        cmodep = (XF86ConfModeLinePtr)cmodep->list.next;
    }
    if(last){
      last->next = NULL;
    }
    monitorp->Last = last;

    /* add the (VESA) default modes */
    if (! addDefaultModes(monitorp) )
	return FALSE;

    if (conf_monitor->mon_gamma_red > GAMMA_ZERO)
	monitorp->gamma.red = conf_monitor->mon_gamma_red;
    if (conf_monitor->mon_gamma_green > GAMMA_ZERO)
	monitorp->gamma.green = conf_monitor->mon_gamma_green;
    if (conf_monitor->mon_gamma_blue > GAMMA_ZERO)
	monitorp->gamma.blue = conf_monitor->mon_gamma_blue;
    
    /* Check that the gamma values are within range */
    if (monitorp->gamma.red > GAMMA_ZERO &&
	(monitorp->gamma.red < GAMMA_MIN ||
	 monitorp->gamma.red > GAMMA_MAX)) {
	badgamma = monitorp->gamma.red;
    } else if (monitorp->gamma.green > GAMMA_ZERO &&
	(monitorp->gamma.green < GAMMA_MIN ||
	 monitorp->gamma.green > GAMMA_MAX)) {
	badgamma = monitorp->gamma.green;
    } else if (monitorp->gamma.blue > GAMMA_ZERO &&
	(monitorp->gamma.blue < GAMMA_MIN ||
	 monitorp->gamma.blue > GAMMA_MAX)) {
	badgamma = monitorp->gamma.blue;
    }
    if (badgamma > GAMMA_ZERO) {
	xf86ConfigError("Gamma value %.f is out of range (%.2f - %.1f)\n",
			badgamma, GAMMA_MIN, GAMMA_MAX);
	    return FALSE;
    }

    return TRUE;
}

static int
lookupVisual(const char *visname)
{
    int i;

    if (!visname || !*visname)
	return -1;

    for (i = 0; i <= DirectColor; i++) {
	if (!xf86nameCompare(visname, xf86VisualNames[i]))
	    break;
    }

    if (i <= DirectColor)
	return i;

    return -1;
}


static Bool
configDisplay(DispPtr displayp, XF86ConfDisplayPtr conf_display)
{
    int count = 0;
    XF86ModePtr modep;
    
    displayp->frameX0           = conf_display->disp_frameX0;
    displayp->frameY0           = conf_display->disp_frameY0;
    displayp->virtualX          = conf_display->disp_virtualX;
    displayp->virtualY          = conf_display->disp_virtualY;
    displayp->depth             = conf_display->disp_depth;
    displayp->fbbpp             = conf_display->disp_bpp;
    displayp->weight.red        = conf_display->disp_weight.red;
    displayp->weight.green      = conf_display->disp_weight.green;
    displayp->weight.blue       = conf_display->disp_weight.blue;
    displayp->blackColour.red   = conf_display->disp_black.red;
    displayp->blackColour.green = conf_display->disp_black.green;
    displayp->blackColour.blue  = conf_display->disp_black.blue;
    displayp->whiteColour.red   = conf_display->disp_white.red;
    displayp->whiteColour.green = conf_display->disp_white.green;
    displayp->whiteColour.blue  = conf_display->disp_white.blue;
    displayp->options           = conf_display->disp_option_lst;
    if (conf_display->disp_visual) {
	displayp->defaultVisual = lookupVisual(conf_display->disp_visual);
	if (displayp->defaultVisual == -1) {
	    xf86ConfigError("Invalid visual name: \"%s\"",
			    conf_display->disp_visual);
	    return FALSE;
	}
    } else {
	displayp->defaultVisual = -1;
    }
	
    /*
     * now hook in the modes
     */
    modep = conf_display->disp_mode_lst;
    while(modep) {
        count++;
        modep = (XF86ModePtr)modep->list.next;
    }
    displayp->modes = xnfalloc((count+1) * sizeof(char*));
    modep = conf_display->disp_mode_lst;
    count = 0;
    while(modep) {
        displayp->modes[count] = modep->mode_name;
        count++;
        modep = (XF86ModePtr)modep->list.next;
    }
    displayp->modes[count] = NULL;
    
    return TRUE;
}

static Bool
configDevice(GDevPtr devicep, XF86ConfDevicePtr conf_device, Bool active)
{
    int i;

    if (active)
	xf86Msg(X_CONFIG, "|   |-->Device \"%s\"\n",
		conf_device->dev_identifier);
    else
	xf86Msg(X_CONFIG, "|-->Inactive Device \"%s\"\n",
		conf_device->dev_identifier);
	
    devicep->identifier = conf_device->dev_identifier;
    devicep->vendor = conf_device->dev_vendor;
    devicep->board = conf_device->dev_board;
    devicep->chipset = conf_device->dev_chipset;
    devicep->ramdac = conf_device->dev_ramdac;
    devicep->driver = conf_device->dev_driver;
    devicep->active = active;
    devicep->videoRam = conf_device->dev_videoram;
    devicep->BiosBase = conf_device->dev_bios_base;
    devicep->MemBase = conf_device->dev_mem_base;
    devicep->IOBase = conf_device->dev_io_base;
    devicep->clockchip = conf_device->dev_clockchip;
    devicep->busID = conf_device->dev_busid;
    devicep->textClockFreq = conf_device->dev_textclockfreq;
    devicep->chipID = conf_device->dev_chipid;
    devicep->chipRev = conf_device->dev_chiprev;
    devicep->options = conf_device->dev_option_lst;
    devicep->irq = conf_device->dev_irq;
    devicep->screen = conf_device->dev_screen;

    for (i = 0; i < MAXDACSPEEDS; i++) {
	if (i < CONF_MAXDACSPEEDS)
            devicep->dacSpeeds[i] = conf_device->dev_dacSpeeds[i];
	else
	    devicep->dacSpeeds[i] = 0;
    }
    devicep->numclocks = conf_device->dev_clocks;
    if (devicep->numclocks > MAXCLOCKS)
	devicep->numclocks = MAXCLOCKS;
    for (i = 0; i < devicep->numclocks; i++) {
	devicep->clock[i] = conf_device->dev_clock[i];
    }
    devicep->claimed = FALSE;

    return TRUE;
}

#ifdef XF86DRI
static Bool
configDRI(XF86ConfDRIPtr drip)
{
    int                count = 0;
    XF86ConfBuffersPtr bufs;
    int                i;
    struct group       *grp;

    xf86ConfigDRI.group      = -1;
    xf86ConfigDRI.mode       = 0;
    xf86ConfigDRI.bufs_count = 0;
    xf86ConfigDRI.bufs       = NULL;

    if (drip) {
	if (drip->dri_group_name) {
	    if ((grp = getgrnam(drip->dri_group_name)))
		xf86ConfigDRI.group = grp->gr_gid;
	} else {
	    if (drip->dri_group >= 0)
		xf86ConfigDRI.group = drip->dri_group;
	}
	xf86ConfigDRI.mode = drip->dri_mode;
	for (bufs = drip->dri_buffers_lst; bufs; bufs = bufs->list.next)
	    ++count;
	
	xf86ConfigDRI.bufs_count = count;
	xf86ConfigDRI.bufs = xnfalloc(count * sizeof(*xf86ConfigDRI.bufs));
	
	for (i = 0, bufs = drip->dri_buffers_lst;
	     i < count;
	     i++, bufs = bufs->list.next) {
	    
	    xf86ConfigDRI.bufs[i].count = bufs->buf_count;
	    xf86ConfigDRI.bufs[i].size  = bufs->buf_size;
				/* FIXME: Flags not implemented.  These
                                   could be used, for example, to specify a
                                   contiguous block and/or write-combining
                                   cache policy. */
	    xf86ConfigDRI.bufs[i].flags = 0;
	}
    }

    return TRUE;
}
#endif

static Bool
configInput(IDevPtr inputp, XF86ConfInputPtr conf_input, MessageType from)
{
    xf86Msg(from, "|-->Input Device \"%s\"\n", conf_input->inp_identifier);
    inputp->identifier = conf_input->inp_identifier;
    inputp->driver = conf_input->inp_driver;
    inputp->commonOptions = conf_input->inp_option_lst;
    inputp->extraOptions = NULL;

    /* XXX This is required until the keyboard driver is converted */
    if (!xf86NameCmp(inputp->driver, "keyboard"))
	return configInputKbd(inputp);

    return TRUE;
}

static Bool
modeIsPresent(char * modename,MonPtr monitorp)
{
    DisplayModePtr knownmodes = monitorp->Modes;

    /* all I can think of is a linear search... */
    while(knownmodes != NULL)
    {
	if(!strcmp(modename,knownmodes->name) &&
	   !(knownmodes->type & M_T_DEFAULT))
	    return TRUE;
	knownmodes = knownmodes->next;
    }
    return FALSE;
}

static Bool
addDefaultModes(MonPtr monitorp)
{
    DisplayModePtr mode;
    DisplayModePtr last = monitorp->Last;
    int i = 0;

    while (xf86DefaultModes[i].name != NULL)
    {
	if ( ! modeIsPresent(xf86DefaultModes[i].name,monitorp) )
	    do
	    {
		mode = xnfalloc(sizeof(DisplayModeRec));
		memcpy(mode,&xf86DefaultModes[i],sizeof(DisplayModeRec));
		if (xf86DefaultModes[i].name)
		    mode->name = xnfstrdup(xf86DefaultModes[i].name);
		if( last ) {
		    mode->prev = last;
		    last->next = mode;
		}
		else {
		    /* this is the first mode */
		    monitorp->Modes = mode;
		    mode->prev = NULL;
		}
		last = mode;
		i++;
	    }
	    while((xf86DefaultModes[i].name != NULL) &&
		  (!strcmp(xf86DefaultModes[i].name,xf86DefaultModes[i-1].name)));
	else
	    i++;
    }
    monitorp->Last = last;

    return TRUE;
}

/*
 * load the config file and fill the global data structure
 */
ConfigStatus
xf86HandleConfigFile(Bool autoconfig)
{
    const char *filename;
    char *searchpath;
    MessageType from = X_DEFAULT;

    if (!autoconfig) {
	if (getuid() == 0)
	    searchpath = ROOT_CONFIGPATH;
	else
	    searchpath = USER_CONFIGPATH;

	if (xf86ConfigFile)
	    from = X_CMDLINE;

	filename = xf86openConfigFile(searchpath, xf86ConfigFile, PROJECTROOT);
	if (filename) {
	    xf86MsgVerb(from, 0, "Using config file: \"%s\"\n", filename);
	    xf86ConfigFile = xnfstrdup(filename);
	} else {
	    xf86Msg(X_ERROR, "Unable to locate/open config file");
	    if (xf86ConfigFile)
		xf86ErrorFVerb(0, ": \"%s\"", xf86ConfigFile);
	    xf86ErrorFVerb(0, "\n");
	    return CONFIG_NOFILE;
	}
    }
     
    if ((xf86configptr = xf86readConfigFile ()) == NULL) {
	xf86Msg(X_ERROR, "Problem parsing the config file\n");
	return CONFIG_PARSE_ERROR;
    }
    xf86closeConfigFile ();

    /* Initialise a few things. */

    /*
     * now we convert part of the information contained in the parser
     * structures into our own structures.
     * The important part here is to figure out which Screen Sections
     * in the XF86Config file are active so that we can piece together
     * the modes that we need later down the road.
     * And while we are at it, we'll decode the rest of the stuff as well
     */

    /* First check if a layout section is present, and if it is valid. */

    if (xf86configptr->conf_layout_lst == NULL || xf86ScreenName != NULL) {
	if (xf86ScreenName == NULL) {
	    xf86Msg(X_WARNING,
		    "No Layout section.  Using the first Screen section.\n");
	}
	if (!configImpliedLayout(&xf86ConfigLayout,
				 xf86configptr->conf_screen_lst)) {
            xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
	    return CONFIG_PARSE_ERROR;
	}
    } else {
	if (xf86configptr->conf_flags != NULL) {
	  char *dfltlayout = NULL;
 	  pointer optlist = xf86configptr->conf_flags->flg_option_lst;
	
	  if (optlist && xf86FindOption(optlist, "defaultserverlayout"))
	    dfltlayout = xf86SetStrOption(optlist, "defaultserverlayout", NULL);
	  if (!configLayout(&xf86ConfigLayout, xf86configptr->conf_layout_lst,
			  dfltlayout)) {
	    xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
	    return CONFIG_PARSE_ERROR;
	  }
	} else {
	  if (!configLayout(&xf86ConfigLayout, xf86configptr->conf_layout_lst,
			  NULL)) {
	    xf86Msg(X_ERROR, "Unable to determine the screen layout\n");
	    return CONFIG_PARSE_ERROR;
	  }
	}
    }

    /* Now process everything else */

    if (!configFiles(xf86configptr->conf_files) ||
        !configServerFlags(xf86configptr->conf_flags,
			   xf86ConfigLayout.options)
#ifdef XF86DRI
	|| !configDRI(xf86configptr->conf_dri)
#endif
       ) {
             ErrorF ("Problem when converting the config data structures\n");
             return CONFIG_PARSE_ERROR;
    }

    /*
     * Handle some command line options that can override some of the
     * ServerFlags settings.
     */
#ifdef XF86VIDMODE
    if (xf86VidModeDisabled)
	xf86Info.vidModeEnabled = FALSE;
    if (xf86VidModeAllowNonLocal)
	xf86Info.vidModeAllowNonLocal = TRUE;
#endif

#ifdef XF86MISC
    if (xf86MiscModInDevDisabled)
	xf86Info.miscModInDevEnabled = FALSE;
    if (xf86MiscModInDevAllowNonLocal)
	xf86Info.miscModInDevAllowNonLocal = TRUE;
#endif

    if (xf86AllowMouseOpenFail)
	xf86Info.allowMouseOpenFail = TRUE;

    return CONFIG_OK;
}


/* These make the equivalent parser functions visible to the common layer. */
Bool
xf86PathIsAbsolute(const char *path)
{
    return (xf86pathIsAbsolute(path) != 0);
}

Bool
xf86PathIsSafe(const char *path)
{
    return (xf86pathIsSafe(path) != 0);
}

