/* $DHD: xc/programs/Xserver/hw/xfree86/common/xf86AutoConfig.c,v 1.15 2003/09/24 19:39:36 dawes Exp $ */

/*
 * Copyright 2003 by David H. Dawes.
 * Copyright 2003 by X-Oz Technologies.
 * All rights reserved.
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
 * 
 * Author: David Dawes <dawes@XFree86.Org>.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86AutoConfig.c,v 1.2 2003/11/03 05:11:01 tsi Exp $ */

#include "xf86.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "xf86Config.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/*
 * Sections for the default built-in configuration.
 */

#define BUILTIN_MODULE_SECTION \
	"Section \"Module\"\n" \
	"\tLoad\t\"extmod\"\n" \
	"\tLoad\t\"dbe\"\n" \
	"\tLoad\t\"glx\"\n" \
	"\tLoad\t\"freetype\"\n" \
	"EndSection\n\n"

#define BUILTIN_DEVICE_NAME \
	"\"Builtin Default %s Device %d\""

#define BUILTIN_DEVICE_SECTION_PRE \
	"Section \"Device\"\n" \
	"\tIdentifier\t" BUILTIN_DEVICE_NAME "\n" \
	"\tDriver\t\"%s\"\n"

#define BUILTIN_DEVICE_SECTION_POST \
	"EndSection\n\n"

#define BUILTIN_DEVICE_SECTION \
	BUILTIN_DEVICE_SECTION_PRE \
	BUILTIN_DEVICE_SECTION_POST

#define BUILTIN_MONITOR_NAME \
	"\"Builtin Default Monitor\""

#define BUILTIN_MONITOR_SECTION \
	"Section \"Monitor\"\n" \
	"\tIdentifier\t" BUILTIN_MONITOR_NAME "\n" \
	"\tOption\t\"TargetRefresh\"\t\"75.0\"\n" \
	"EndSection\n\n"

#define BUILTIN_SCREEN_NAME \
	"\"Builtin Default %s Screen %d\""

#define BUILTIN_SCREEN_SECTION \
	"Section \"Screen\"\n" \
	"\tIdentifier\t" BUILTIN_SCREEN_NAME "\n" \
	"\tDevice\t" BUILTIN_DEVICE_NAME "\n" \
	"\tMonitor\t" BUILTIN_MONITOR_NAME "\n" \
	"EndSection\n\n"

#define BUILTIN_LAYOUT_SECTION_PRE \
	"Section \"ServerLayout\"\n" \
	"\tIdentifier\t\"Builtin Default Layout\"\n"

#define BUILTIN_LAYOUT_SCREEN_LINE \
	"\tScreen\t" BUILTIN_SCREEN_NAME "\n"

#define BUILTIN_LAYOUT_SECTION_POST \
	"EndSection\n\n"


#ifndef GET_CONFIG_CMD
#define GET_CONFIG_CMD			"getconfig"
#endif

#ifndef GETCONFIG_DIR
#define GETCONFIG_DIR			PROJECTROOT "/lib/X11/getconfig"
#endif

#define GETCONFIG_WHITESPACE		" \t\n"

static const char **builtinConfig = NULL;
static int builtinLines = 0;
static const char *deviceList[] = {
	"fbdev",
	"vesa",
	"vga",
	NULL
};

/*
 * A built-in config file is stored as an array of strings, with each string
 * representing a single line.  AppendToConfig() breaks up the string "s"
 * into lines, and appends those lines it to builtinConfig.
 */

static void
AppendToList(const char *s, const char ***list, int *lines)
{
    char *str, *newstr, *p;

    str = xnfstrdup(s);
    for (p = strtok(str, "\n"); p; p = strtok(NULL, "\n")) {
	(*lines)++;
	*list = xnfrealloc(*list, (*lines + 1) * sizeof(**list));
	newstr = xnfalloc(strlen(p) + 2);
	strcpy(newstr, p);
	strcat(newstr, "\n");
	(*list)[*lines - 1] = newstr;
	(*list)[*lines] = NULL;
    }
    xfree(str);
}

static void
FreeList(const char ***list, int *lines)
{
    int i;

    for (i = 0; i < *lines; i++) {
	if ((*list)[i])
	    xfree((*list)[i]);
    }
    xfree(*list);
    *list = NULL;
    *lines = 0;
}

static void
FreeConfig(void)
{
    FreeList(&builtinConfig, &builtinLines);
}

static void
AppendToConfig(const char *s)
{
    AppendToList(s, &builtinConfig, &builtinLines);
}

Bool
xf86AutoConfig(void)
{
    const char **p;
    char buf[1024];
    pciVideoPtr *pciptr, info = NULL;
    char *driver = NULL;
    FILE *gp = NULL;
    ConfigStatus ret;

    /* Find the primary device, and get some information about it. */
    if (xf86PciVideoInfo) {
	for (pciptr = xf86PciVideoInfo; (info = *pciptr); pciptr++) {
	    if (xf86IsPrimaryPci(info)) {
		break;
	    }
	}
	if (!info) {
	    ErrorF("Primary device is not PCI\n");
	}
    } else {
	ErrorF("xf86PciVideoInfo is not set\n");
    }

    if (info) {
	char *tmp;
	char *path = NULL, *a, *b;
	char *searchPath = NULL;

	/*
	 * Look for the getconfig program first in the xf86ModulePath
	 * directories, then in BINDIR.  If it isn't found in any of those
	 * locations, just use the normal search path.
	 */

	if (xf86ModulePath) {
	    a = xnfstrdup(xf86ModulePath);
	    b = strtok(a, ",");
	    while (b) {
		path = xnfrealloc(path,
				  strlen(b) + 1 + strlen(GET_CONFIG_CMD) + 1);
		sprintf(path, "%s/%s", b, GET_CONFIG_CMD);
		if (access(path, X_OK) == 0)
		    break;
		b = strtok(NULL, ",");
	    }
	    if (!b) {
		xfree(path);
		path = NULL;
	    }
	    xfree(a);
	}

#ifdef BINDIR
	if (!path) {
	    path = xnfstrdup(BINDIR "/" GET_CONFIG_CMD);
	    if (access(path, X_OK) != 0) {
		xfree(path);
		path = NULL;
	    }
	}
#endif

	if (!path)
	    path = xnfstrdup(GET_CONFIG_CMD);

	/*
	 * Build up the config file directory search path:
	 *
	 * /etc/X11
	 * PROJECTROOT/etc/X11
	 * xf86ModulePath
	 * PROJECTROOT/lib/X11/getconfig  (GETCONFIG_DIR)
	 */

	searchPath = xnfalloc(strlen("/etc/X11") + 1 +
			      strlen(PROJECTROOT "/etc/X11") + 1 +
			      (xf86ModulePath ? strlen(xf86ModulePath) : 0)
				+ 1 +
			      strlen(GETCONFIG_DIR) + 1);
	strcpy(searchPath, "/etc/X11," PROJECTROOT "/etc/X11,");
	if (xf86ModulePath && *xf86ModulePath) {
	    strcat(searchPath, xf86ModulePath);
	    strcat(searchPath, ",");
	}
	strcat(searchPath, GETCONFIG_DIR);

	ErrorF("xf86AutoConfig: Primary PCI is %d:%d:%d\n",
	       info->bus, info->device, info->func);

	snprintf(buf, sizeof(buf), "%s"
#ifdef DEBUG
		 " -D"
#endif
		 " -X %d"
		 " -I %s"
		 " -v 0x%04x -d 0x%04x -r 0x%02x -s 0x%04x"
		 " -b 0x%04x -c 0x%04x",
		 path,
		 (unsigned int)xorgGetVersion(),
		 searchPath,
		 info->vendor, info->chipType, info->chipRev,
		 info->subsysVendor, info->subsysCard,
		 info->class << 8 | info->subclass);
	ErrorF("Running \"%s\"\n", buf);
	gp = Popen(buf, "r");
	if (gp) {
	    if (fgets(buf, sizeof(buf) - 1, gp)) {
		buf[strlen(buf) - 1] = '\0';
		tmp = strtok(buf, GETCONFIG_WHITESPACE);
		if (tmp)
		    driver = xnfstrdup(tmp);
	    }
	}
	xfree(path);
	xfree(searchPath);
    }

    AppendToConfig(BUILTIN_MODULE_SECTION);
    AppendToConfig(BUILTIN_MONITOR_SECTION);

    if (driver) {
	snprintf(buf, sizeof(buf), BUILTIN_DEVICE_SECTION_PRE,
		 driver, 0, driver);
	AppendToConfig(buf);
	ErrorF("New driver is \"%s\"\n", driver);
	buf[0] = '\t';
	while (fgets(buf + 1, sizeof(buf) - 2, gp)) {
	    AppendToConfig(buf);
	    ErrorF("Extra line: %s", buf);
	}
	AppendToConfig(BUILTIN_DEVICE_SECTION_POST);
	snprintf(buf, sizeof(buf), BUILTIN_SCREEN_SECTION,
		 driver, 0, driver, 0);
	AppendToConfig(buf);
    }

    if (gp)
	Pclose(gp);

    for (p = deviceList; *p; p++) {
	snprintf(buf, sizeof(buf), BUILTIN_DEVICE_SECTION, *p, 0, *p);
	AppendToConfig(buf);
	snprintf(buf, sizeof(buf), BUILTIN_SCREEN_SECTION, *p, 0, *p, 0);
	AppendToConfig(buf);
    }

    AppendToConfig(BUILTIN_LAYOUT_SECTION_PRE);
    if (driver) {
	snprintf(buf, sizeof(buf), BUILTIN_LAYOUT_SCREEN_LINE, driver, 0);
	AppendToConfig(buf);
    }
    for (p = deviceList; *p; p++) {
	snprintf(buf, sizeof(buf), BUILTIN_LAYOUT_SCREEN_LINE, *p, 0);
	AppendToConfig(buf);
    }
    AppendToConfig(BUILTIN_LAYOUT_SECTION_POST);

#ifdef BUILTIN_EXTRA
    AppendToConfig(BUILTIN_EXTRA);
#endif

    if (driver)
	xfree(driver);

    xf86MsgVerb(X_DEFAULT, 0,
		"Using default built-in configuration (%d lines)\n",
		builtinLines);

    xf86MsgVerb(X_DEFAULT, 3, "--- Start of built-in configuration ---\n");
    for (p = builtinConfig; *p; p++)
	xf86ErrorFVerb(3, "\t%s", *p);
    xf86MsgVerb(X_DEFAULT, 3, "--- End of built-in configuration ---\n");
    
    xf86setBuiltinConfig(builtinConfig);
    ret = xf86HandleConfigFile(TRUE);
    FreeConfig();
    switch(ret) {
    case CONFIG_OK:
	return TRUE;
    default:
	xf86Msg(X_ERROR, "Error parsing the built-in default configuration.\n");
	return FALSE;
    }
}

