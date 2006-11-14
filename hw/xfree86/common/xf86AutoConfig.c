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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "xf86Config.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/* Sections for the default built-in configuration. */

#define BUILTIN_MODULE_SECTION \
	"Section \"Module\"\n" \
	"\tLoad\t\"extmod\"\n" \
	"\tLoad\t\"dbe\"\n" \
	"\tLoad\t\"glx\"\n" \
	"\tLoad\t\"freetype\"\n" \
	"\tLoad\t\"type1\"\n" \
	"\tLoad\t\"record\"\n" \
	"\tLoad\t\"dri\"\n" \
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

static const char *
videoPtrToDriverName(pciVideoPtr info)
{
    /*
     * things not handled yet:
     * amd/cyrix/nsc
     * xgi
     */

    switch (info->vendor)
    {
	case 0x1142:		    return "apm";
	case 0xedd8:		    return "ark";
	case 0x1a03:		    return "ast";
	case 0x1002:		    return "ati";
	case 0x102c:		    return "chips";
	case 0x1013:		    return "cirrus";
	case 0x8086:
	    if ((info->chipType == 0x00d1) || (info->chipType == 0x7800))
		return "i740";
	    else return "i810";
	case 0x102b:		    return "mga";
	case 0x10c8:		    return "neomagic";
	case 0x105d:		    return "i128";
	case 0x10de: case 0x12d2:   return "nv";
	case 0x1163:		    return "rendition";
	case 0x5333:
	    switch (info->chipType)
	    {
		case 0x88d0: case 0x88d1: case 0x88f0: case 0x8811:
		case 0x8812: case 0x8814: case 0x8901:
		    return "s3";
		case 0x5631: case 0x883d: case 0x8a01: case 0x8a10:
		case 0x8c01: case 0x8c03: case 0x8904: case 0x8a13:
		    return "s3virge";
		default:
		    return "savage";
	    }
	case 0x1039:		    return "sis";
	case 0x126f:		    return "siliconmotion";
	case 0x121a:
	    if (info->chipType < 0x0003)
	        return "voodoo";
	    else
	        return "tdfx";
	case 0x3d3d:		    return "glint";
	case 0x1023:		    return "trident";
	case 0x100c:		    return "tseng";
	case 0x1106:		    return "via";
	case 0x15ad:		    return "vmware";
	default: break;
    }
    return NULL;
}

Bool
xf86AutoConfig(void)
{
    const char **p;
    char buf[1024];
    pciVideoPtr *pciptr, info = NULL;
    char *driver = NULL;
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

    if (info)
	driver = videoPtrToDriverName(info);

    AppendToConfig(BUILTIN_MODULE_SECTION);
    AppendToConfig(BUILTIN_MONITOR_SECTION);

    if (driver) {
	snprintf(buf, sizeof(buf), BUILTIN_DEVICE_SECTION_PRE,
		 driver, 0, driver);
	AppendToConfig(buf);
	ErrorF("New driver is \"%s\"\n", driver);
	buf[0] = '\t';
	AppendToConfig(BUILTIN_DEVICE_SECTION_POST);
	snprintf(buf, sizeof(buf), BUILTIN_SCREEN_SECTION,
		 driver, 0, driver, 0);
	AppendToConfig(buf);
    }

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

    if (ret != CONFIG_OK)
	xf86Msg(X_ERROR, "Error parsing the built-in default configuration.\n");

    return (ret == CONFIG_OK);
}
