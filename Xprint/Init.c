/* $Xorg: Init.c,v 1.5 2001/03/07 17:31:33 pookie Exp $ */
/*
(c) Copyright 1996 Hewlett-Packard Company
(c) Copyright 1996 International Business Machines Corp.
(c) Copyright 1996 Sun Microsystems, Inc.
(c) Copyright 1996 Novell, Inc.
(c) Copyright 1996 Digital Equipment Corp.
(c) Copyright 1996 Fujitsu Limited
(c) Copyright 1996 Hitachi, Ltd.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the names of the copyright holders shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from said
copyright holders.
*/
/*******************************************************************
**
**    *********************************************************
**    *
**    *  File:		printer/Init.c
**    *
**    *  Contents:
**    *                 The InitOutput routine here would presumably
**    *                 be called from the normal server's InitOutput
**    *                 after all display screens have been added.
**    *                 There is are ifdef'd routines suitable for
**    *                 use in building a printer-only server.  Turn
**    *                 on the "PRINTER_ONLY_SERVER" define if this is
**    *                 to be the only ddx-level driver.
**    *
**    *  Copyright:	Copyright 1993,1995 Hewlett-Packard Company
**    *
**    *********************************************************
** 
********************************************************************/
/* $XFree86: xc/programs/Xserver/Xprint/Init.c,v 1.13 2001/12/21 21:02:04 dawes Exp $ */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <locale.h>
#ifdef __hpux
#include <sys/sysmacros.h>
#endif

#include "X.h"
#define NEED_EVENTS 1
#include "Xproto.h"
#include <servermd.h>

#include "screenint.h"
#include "input.h"
#include "cursor.h"
#include "misc.h"
#include "windowstr.h"
#include "inputstr.h"

#include "gcstruct.h"
#include "fonts/fontstruct.h"
#include "errno.h"

typedef char *XPointer;
#define HAVE_XPointer 1

#define Status int
#include <Xresource.h>

#include "DiPrint.h"
#include "attributes.h"

#include "os.h"

static void GenericScreenInit(
    int index,
    ScreenPtr pScreen,
    int argc,
    char **argv);
static Bool InitPrintDrivers(
    int index,
    ScreenPtr pScreen,
    int argc,
    char **argv);

/*
 * The following two defines are used to build the name "X*printers", where
 * the "*" is replaced by the display number.  This is used to construct
 * the name of the default printers configuration file if the -XpFile
 * command line option was not specified.
 */
#define XNPRINTERSFILEPREFIX "/X"
#define XNPRINTERSFILESUFFIX "printers"
#define XPRINTERSFILENAME "Xprinters"

#define MODELDIRNAME "/models"
#define FONTDIRNAME "/fonts"

/*
 * The string LIST_QUEUES is fed to a shell to generate an ordered
 * list of available printers on the system. These string definitions
 * are taken from the file PrintSubSys.C within the code for the 
 * dtprintinfo program.
 */
#ifdef AIXV4
const char *LIST_QUEUES = "lsallq | grep -v '^bsh$' | sort";
#else
#ifdef hpux
const char *LIST_QUEUES = "LANG=C lpstat -v | "
                            "awk '"
                            " $2 == \"for\" "
                            "   { "
                            "      x = match($3, /:/); "
                            "      print substr($3, 1, x-1)"
                            "   }' | sort";
#else
#ifdef __osf__
  const char *LIST_QUEUES = "LANG=C lpstat -v | "
                            "nawk '"
                            " $2 == \"for\"    "
                            "   { print $4 }' "
                            "   | sort";
#else
#ifdef __uxp__
const char *LIST_QUEUES = "LANG=C lpstat -v | "
                            "nawk '"
                            " $4 == \"for\" "
                            "   { "
                            "      x = match($5, /:/); "
                            "      print substr($5, 1, x-1)"
                            "   }' | sort";
#else
#if defined(CSRG_BASED) || defined(linux) || defined(ISC) || defined(__GNUC__)
const char *LIST_QUEUES = "LANG=C lpc status | grep -v '^\t' | "
                            "sed -e /:/s/// | sort";
#else
const char *LIST_QUEUES = "LANG=C lpstat -v | "
                            "nawk '"
                            " $2 == \"for\" "
                            "   { "
                            "      x = match($3, /:/); "
                            "      print substr($3, 1, x-1)"
                            "   }' | sort";
#endif
#endif
#endif
#endif
#endif

#ifdef XPRASTERDDX

static
PixmapFormatRec	RasterPixmapFormats[] = {
    { 1, 1, BITMAP_SCANLINE_PAD }
};
#define NUMRASTFORMATS	(sizeof RasterPixmapFormats)/(sizeof RasterPixmapFormats[0])

#include "raster/Raster.h"

#endif

#ifdef XPPCLDDX

static
PixmapFormatRec	ColorPclPixmapFormats[] = {
    { 1, 1, BITMAP_SCANLINE_PAD },
    { 8, 8, BITMAP_SCANLINE_PAD },
    { 24,32, BITMAP_SCANLINE_PAD }
};

#define NUMCPCLFORMATS	(sizeof ColorPclPixmapFormats)/(sizeof ColorPclPixmapFormats[0])

#endif

#ifdef XPMONOPCLDDX

static
PixmapFormatRec	MonoPclPixmapFormats[] = {
    { 1, 1, BITMAP_SCANLINE_PAD }
};

#define NUMMPCLFORMATS	(sizeof MonoPclPixmapFormats)/(sizeof MonoPclPixmapFormats[0])

#endif

#if defined(XPPCLDDX) || defined(XPMONOPCLDDX)
#include "pcl/Pcl.h"
#endif

#ifdef XPPSDDX

static
PixmapFormatRec	PSPixmapFormats[] = {
    { 1, 1, BITMAP_SCANLINE_PAD },
    { 8, 8, BITMAP_SCANLINE_PAD },
    { 24,32, BITMAP_SCANLINE_PAD }
};

#define NUMPSFORMATS	(sizeof PSPixmapFormats)/(sizeof PSPixmapFormats[0])

#include "ps/Ps.h"

#endif

/*
 * The driverInitArray contains an entry for each driver the
 * server knows about. Each element contains pointers to pixmap formats, the
 * driver's initialization routine, and pointers to the driver's
 * attribute validation rec, and/or a driver function which 
 * returns the maximum medium width&height, and maximum resolution
 * given a printer name.  Either the validation rec OR the dimension
 * function can be NULL.  If the function is non-NULL then it
 * will be called, and will be passed the (possibly NULL) validation rec.
 * If the function is NULL, then XpGetMaxWidthHeightRes() is called.
 */
typedef struct _driverInitRec {
    char *driverName;
    pBFunc initFunc;
    XpValidatePoolsRec *pValRec;
    pVFunc dimensionsFunc;
    PixmapFormatRec *pFmts;
    int numFmts;
} driverInitRec;

static driverInitRec driverInits[] = {
#ifdef XPRASTERDDX
    {
	"XP-RASTER",
	InitializeRasterDriver,
	&RasterValidatePoolsRec,
	(pVFunc) NULL,
	RasterPixmapFormats,
	NUMRASTFORMATS
    },
#endif
#ifdef XPPCLDDX
    {
	"XP-PCL-COLOR",
	InitializeColorPclDriver,
	&PclValidatePoolsRec,
	(pVFunc) NULL,
	ColorPclPixmapFormats,
	NUMCPCLFORMATS
    },
#endif
#ifdef XPMONOPCLDDX
    {
	"XP-PCL-MONO",
	InitializeMonoPclDriver,
	&PclValidatePoolsRec,
	(pVFunc) NULL,
	MonoPclPixmapFormats,
	NUMMPCLFORMATS
    },
#endif
#ifdef XPPSDDX
    {
	"XP-POSTSCRIPT",
	InitializePsDriver,
	&PsValidatePoolsRec,
	(pVFunc) NULL,
	PSPixmapFormats,
	NUMPSFORMATS
    },
#endif
};
    

/*
 * The printerDb variable points to a list of PrinterDbEntry structs
 * which map printer names with screen numbers and driver names.
 */
typedef struct _printerDbEntry {
    struct _printerDbEntry *next;
    char *name;
    char *qualifier;
    int screenNum;
    char *driverName;
} PrinterDbEntry, *PrinterDbPtr;

static PrinterDbPtr printerDb = (PrinterDbPtr)NULL;

/*
 * The nameMap is a list used in initializing the attribute store
 * for each printer.  The list is freed once the printerDb is built
 * and the attribute stores for all printers have been initialized.
 */
typedef struct _nameMapEntry {
    struct _nameMapEntry *next;
    char *name;
    char *qualifier;
} NameMapEntry, *NameMapPtr;

static NameMapPtr nameMap = (NameMapPtr)NULL;

/*
 * The driverMap is a list which provides the mapping between driver names
 * and screen numbers. It is built and used 
 * by RehashPrinterList to correctly fill in the screenNum field in the
 * printerDb entries. The list is freed before RehashPrinterList terminates.
 */
typedef struct _driverMapping {
    struct _driverMapping *next;
    char *driverName;
    int screenNum;
} DriverMapEntry, *DriverMapPtr;

static const char configFilePath[] =
"/etc/dt/config/print:/usr/dt/config/print";

static const char printServerConfigDir[] = "XPSERVERCONFIGDIR";

static char *configFileName = (char *)NULL;
static Bool freeDefaultFontPath = FALSE;
static char *origFontPath = (char *)NULL;

/*
 * XprintOptions checks argv[i] to see if it is our command line
 * option specifying a configuration file name.  It returns the index
 * of the next option to process.
 */
int
XprintOptions(
    int argc,
    char **argv,
    int i)
{
    if(strcmp(argv[i], "-XpFile") == 0)
    {
	if ((i + 1) >= argc) {
	    ddxUseMsg ();
	    return i + 2;
	}
	configFileName = argv[i + 1];
	return i + 2;
    }
    else
	return i;
}

/************************************************************
 * GetInitFunc --
 *
 *      This routine is called from the InitPrintDrivers function.
 *      Given the name of a driver, return a pointer to the driver's
 *	initialization function.
 *
 * Results:
 *	Returns a pointer to the initialization function for the driver.
 * 
 *
 ************************************************************/

/*
typedef Bool (*pIFunc)();
static pIFunc
GetInitFunc(driverName)
*/

static pBFunc GetInitFunc(char *driverName)
{
    driverInitRec *pInitRec;
    int numDrivers = sizeof(driverInits)/sizeof(driverInitRec);
    int i;

    for(pInitRec = driverInits, i = 0; i < numDrivers; pInitRec++, i++)
    {
        if( !strcmp( driverName, pInitRec->driverName ) )
          return pInitRec->initFunc;
    }

    return 0;
}

static void
GetDimFuncAndRec(
    char *driverName, 
    XpValidatePoolsRec **pValRec,
    pVFunc *dimensionsFunc)
{
    driverInitRec *pInitRec;
    int numDrivers = sizeof(driverInits)/sizeof(driverInitRec);
    int i;

    for(pInitRec = driverInits, i = 0; i < numDrivers; pInitRec++, i++)
    {
        if( !strcmp( driverName, pInitRec->driverName ) )
	{
	  *dimensionsFunc = pInitRec->dimensionsFunc;
	  *pValRec = pInitRec->pValRec;
          return ;
	}
    }

    *dimensionsFunc = 0;
    *pValRec = 0;
    return;
}

static void
FreePrinterDb(void)
{
    PrinterDbPtr pCurEntry, pNextEntry;

    for(pCurEntry = printerDb, pNextEntry = 0;
	pCurEntry != (PrinterDbPtr)NULL; pCurEntry = pNextEntry)
    {
	pNextEntry = pCurEntry->next;
	if(pCurEntry->name != (char *)NULL)
	    xfree(pCurEntry->name);
	/*
	 * We don't free the driver name, because it's expected to simply
	 * be a pointer into the xrm database.
	 */
	xfree(pCurEntry);
    }
    printerDb = 0;
}

/*
 * AddPrinterDbName allocates an entry in the printerDb list, and
 * initializes the "name".  It returns TRUE if the element was 
 * successfully added, and FALSE if an allocation error ocurred.
 * XXX AddPrinterDbName needs to check for (and not add) duplicate names.
 */
static Bool
AddPrinterDbName(char *name)
{
    PrinterDbPtr pEntry = (PrinterDbPtr)xalloc(sizeof(PrinterDbEntry));

    if(pEntry == (PrinterDbPtr)NULL) return FALSE;
    pEntry->name = strdup(name);
    pEntry->qualifier = (char *)NULL;

    if(printerDb == (PrinterDbPtr)NULL)
    {
	pEntry->next = (PrinterDbPtr)NULL;
	printerDb = pEntry;
    }
    else
    {
	pEntry->next = printerDb;
	printerDb = pEntry;
    }
    return TRUE;
}

static void
AugmentPrinterDb(const char *command)
{
    FILE *fp;
    char name[256];

    fp = popen(command, "r");
    /* XXX is a 256 character limit overly restrictive for printer names? */
    while(fgets(name, 256, fp) != (char *)NULL && strlen(name))
    {
        name[strlen(name) - 1] = (char)'\0'; /* strip the \n */
        AddPrinterDbName(name);
    }
    pclose(fp);
}

/*
 * FreeNameMap frees all remaining memory associated with the nameMap.
 */
static void
FreeNameMap(void)
{
    NameMapPtr pEntry, pTmp;

    for(pEntry = nameMap, pTmp = (NameMapPtr)NULL; 
	pEntry != (NameMapPtr)NULL;
	pEntry = pTmp)
    {
	if(pEntry->name != (char *)NULL)
	    xfree(pEntry->name);
	if(pEntry->qualifier != (char *)NULL)
	    xfree(pEntry->qualifier);
	pTmp = pEntry->next;
	xfree(pEntry);
    }
    nameMap = (NameMapPtr)NULL;
}

/*
 * AddNameMap adds an element to the nameMap linked list.
 */
static Bool
AddNameMap(char *name, char *qualifier)
{
    NameMapPtr pEntry;

    if((pEntry = (NameMapPtr)xalloc(sizeof(NameMapEntry))) == (NameMapPtr)NULL)
	return FALSE;
    pEntry->name = name;
    pEntry->qualifier = qualifier;
    pEntry->next = nameMap;
    nameMap = pEntry;
    return TRUE;
}

/*
 * MergeNameMap - puts the "map" names (aka qualifiers, aliases) into
 * the printerDb.  This should be called once, after both the printerDb
 * and nameMap lists are complete. When/if MergeNameMap finds a map for
 * an entry in the printerDb, the qualifier is _moved_ (not copied) to
 * the printerDb. This means that the qualifier pointer in the nameMap
 * is NULLed out.
 */
static void
MergeNameMap(void)
{
    NameMapPtr pMap;
    PrinterDbPtr pDb;

    for(pMap = nameMap; pMap != (NameMapPtr)NULL; pMap = pMap->next)
    {
	for(pDb = printerDb; pDb != (PrinterDbPtr)NULL; pDb = pDb->next)
	{
	    if(!strcmp(pMap->name, pDb->name))
	    {
		pDb->qualifier = pMap->qualifier;
		pMap->qualifier = (char *)NULL;
	    }
	}
    }
}

/*
 * CreatePrinterAttrs causes the attribute stores to be built for
 * each printer in the printerDb.
 */
static void
CreatePrinterAttrs(void)
{
    PrinterDbPtr pDb;

    for(pDb = printerDb; pDb != (PrinterDbPtr)NULL; pDb = pDb->next)
    {
        XpBuildAttributeStore(pDb->name, (pDb->qualifier)? 
			      pDb->qualifier : pDb->name);
    }
}

#ifdef XPPSDDX
#define defaultDriver "XP-POSTSCRIPT"
#else
#ifdef XPPCLDDX
#define defaultDriver "XP-PCL-COLOR"
#else
#ifdef XPMONOPCLDDX
#define defaultDriver "XP-PCL-MONO"
#else
#define defaultDriver "XP-RASTER"
#endif
#endif
#endif

/*
 * StoreDriverNames -  queries the attribute store for the ddx-identifier.
 * if the ddx-identifier is not in the attribute database, then a default
 * ddx-identifier is store in both the attribute store for the printer,
 * and in the printerDb.
 * The ddx-identifier is stored in the printerDb for use in initializing
 * the screens.
 */
static void
StoreDriverNames(void)
{
    PrinterDbPtr pEntry;

    for(pEntry = printerDb; pEntry != (PrinterDbPtr)NULL; 
	pEntry = pEntry->next)
    {
        pEntry->driverName = (char*)XpGetPrinterAttribute(pEntry->name, 
							  "xp-ddx-identifier");
	if(pEntry->driverName == (char *)NULL || 
	   strlen(pEntry->driverName) == 0 ||
	   GetInitFunc(pEntry->driverName) == 0)
	{
	    if (pEntry->driverName && (strlen(pEntry->driverName) != 0)) {
	        ErrorF("Xp Extension: Can't load driver %s\n", 
		       pEntry->driverName);
	        ErrorF("              init function missing\n"); 
	    }

	    pEntry->driverName = defaultDriver;
	    XpAddPrinterAttribute(pEntry->name,
			          (pEntry->qualifier != (char *)NULL)?
				  pEntry->qualifier : pEntry->name,
				  "*xp-ddx-identifier", pEntry->driverName);
	}
    }
}

static char *
MbStrchr(
    char *str,
    int ch)
{
    size_t mbCurMax = MB_CUR_MAX;
    wchar_t targetChar, curChar;
    char tmpChar;
    int i, numBytes, byteLen;

    if(mbCurMax <= 1) return strchr(str, ch);

    tmpChar = (char)ch;
    mbtowc(&targetChar, &tmpChar, mbCurMax);
    for(i = 0, numBytes = 0, byteLen = strlen(str); i < byteLen; i += numBytes)
    {
        numBytes = mbtowc(&curChar, &str[i], mbCurMax);
        if(curChar == targetChar) return &str[i];
    }
    return (char *)NULL;
}

/*
 * GetConfigFileName - Looks for a "Xprinters" file in
 * $(XPRINTDIR)/$LANG/print, and then in $(XPRINTDIR)/C/print. If it
 * finds such a file, it returns the path to the file.  The returned
 * string must be freed by the caller.
 */
static char *
GetConfigFileName(void)
{
    /*
     * We need to find the system-wide file, if one exists.  This
     * file can be in either $(XPRINTDIR)/$LANG/print, or in
     * $(PRINTDIR)/C/print, and the file itself is named "Xprinters".
     */
    char *dirName, *filePath;
	
    /*
     * Check for a LANG-specific file.
     */
    if ((dirName = XpGetConfigDir(TRUE)) != 0)
    {
        filePath = (char *)xalloc(strlen(dirName) +
				  strlen(XPRINTERSFILENAME) + 2);

	if(filePath == (char *)NULL)
	{
	    xfree(dirName);
	    return (char *)NULL;
	}

	sprintf(filePath, "%s/%s", dirName, XPRINTERSFILENAME);
	xfree(dirName);
	if(access(filePath, R_OK) == 0)
	    return filePath;

	xfree(filePath);
    }

    if ((dirName = XpGetConfigDir(FALSE)) != 0)
    {
	filePath = (char *)xalloc(strlen(dirName) +
				  strlen(XPRINTERSFILENAME) + 2);
	if(filePath == (char *)NULL)
	{
	    xfree(dirName);
	    return (char *)NULL;
	}
	sprintf(filePath, "%s/%s", dirName, XPRINTERSFILENAME);
	xfree(dirName);
	if(access(filePath, R_OK) == 0)
	    return filePath;
	xfree(filePath);
    }
    return (char *)NULL;
}

/*
 * BuildPrinterDb - reads the config file if it exists, and if necessary
 * executes a command such as lpstat to generate a list of printers.
 * XXX
 * XXX BuildPrinterDb must be rewritten to allow 16-bit characters in 
 * XXX printer names.  The will involve replacing the use of strtok() and its
 * XXX related functions.
 * XXX At the same time, BuildPrinterDb and it's support routines should have
 * XXX allocation error checking added.
 * XXX
 */
static PrinterDbPtr
BuildPrinterDb(void)
{
    Bool defaultAugment = TRUE, freeConfigFileName;

    if(configFileName && access(configFileName, R_OK) != 0)
    {
	ErrorF("Xp Extension: Can't open file %s\n", configFileName);
    }
    if(!configFileName && (configFileName = GetConfigFileName()))
	freeConfigFileName = TRUE;
    else
	freeConfigFileName = FALSE;

    if(configFileName != (char *)NULL && access(configFileName, R_OK) == 0)
    {
	char line[256];
	FILE *fp = fopen(configFileName, "r");

	while(fgets(line, 256, fp) != (char *)NULL)
	{
	    char *tok, *ptr;
	    if((tok = strtok(line, " \t\012")) != (char *)NULL)
	    {
		if(tok[0] == (char)'#') continue;
		if(strcmp(tok, "Printer") == 0)
		{
		    while((tok = strtok((char *)NULL, " \t")) != (char *)NULL)
		    {
		        if ((ptr = MbStrchr(tok, '\012')) != 0)
		            *ptr = (char)'\0';
			AddPrinterDbName(tok);
		    }
		}
		else if(strcmp(tok, "Map") == 0)
		{
		    char *name, *qualifier;

		    if((tok = strtok((char *)NULL, " \t\012")) == (char *)NULL)
			continue;
		    name = strdup(tok);
		    if((tok = strtok((char *)NULL, " \t\012")) == (char *)NULL)
		    {
			xfree(name);
			continue;
		    }
		    qualifier = strdup(tok);
		    AddNameMap(name, qualifier);
		}
		else if(strcmp(tok, "Augment_Printer_List") == 0)
		{
		    if((tok = strtok((char *)NULL, " \t\012")) == (char *)NULL)
			continue;

		    if(strcmp(tok, "%default%") == 0)
			continue;
		    defaultAugment = FALSE;
		    if(strcmp(tok, "%none%") == 0)
			continue;
		    AugmentPrinterDb(tok);
		}
		else
		    break; /* XXX Generate an error? */
	    }
	}
	fclose(fp);
    }

    if(defaultAugment == TRUE)
    {
	AugmentPrinterDb(LIST_QUEUES);
    }

    MergeNameMap();
    FreeNameMap();

    /* Create the attribute stores for all printers */
    CreatePrinterAttrs();

    /*
     * Find the drivers for each printers, and store the driver names
     * in the printerDb
     */
    StoreDriverNames();

    if(freeConfigFileName)
    {
	xfree(configFileName);
	configFileName = (char *)NULL;
    }

    return printerDb;
}

static void
FreeDriverMap(DriverMapPtr driverMap)
{
    DriverMapPtr pCurEntry, pNextEntry;

    for(pCurEntry = driverMap, pNextEntry = (DriverMapPtr)NULL; 
	pCurEntry != (DriverMapPtr)NULL; pCurEntry = pNextEntry)
    {
	pNextEntry = pCurEntry->next;
	if(pCurEntry->driverName != (char *)NULL)
	    xfree(pCurEntry->driverName);
	xfree(pCurEntry);
    }
}

/*
 * XpRehashPrinterList rebuilds the list of printers known to the
 * server.  It first walks the printerDb to build a table mapping
 * driver names and screen numbers, since this is not an easy mapping
 * to change in the sample server. The normal configuration files are
 * then read & parsed to create the new list of printers. Printers
 * which require drivers other than those already initialized are
 * deleted from the printerDb.  This leaves attribute stores in place
 * for inaccessible printers, but those stores will be cleaned up in
 * the next rehash or server recycle.
 */
int
XpRehashPrinterList(void)
{
    PrinterDbPtr pEntry, pPrev;
    DriverMapPtr driverMap = (DriverMapPtr)NULL, pDrvEnt;
    int result;

    /* Build driverMap */
    for(pEntry = printerDb; pEntry != (PrinterDbPtr)NULL; pEntry = pEntry->next)
    {
	for(pDrvEnt = driverMap; pDrvEnt != (DriverMapPtr)NULL; 
	    pDrvEnt = pDrvEnt->next)
	{
	    if(!strcmp(pEntry->driverName, pDrvEnt->driverName))
		break;
	}

	if(pDrvEnt != (DriverMapPtr)NULL) 
	    continue;

	if((pDrvEnt = (DriverMapPtr)xalloc(sizeof(DriverMapEntry))) == 
	    (DriverMapPtr)NULL)
	{
	    FreeDriverMap(driverMap);
	    return BadAlloc;
	}
	pDrvEnt->driverName = strdup(pEntry->driverName);
	pDrvEnt->screenNum = pEntry->screenNum;
	pDrvEnt->next = driverMap;
	driverMap = pDrvEnt;
    }

    /* Free the old printerDb */
    FreePrinterDb();

    /* Free/Rehash attribute stores */
    if((result = XpRehashAttributes()) != Success)
	return result;

    /* Build a new printerDb */
    if(BuildPrinterDb() ==  (PrinterDbPtr)NULL)
        return BadAlloc;

    /* Walk PrinterDb & either store screenNum, or delete printerDb entry */
    for(pEntry = printerDb, pPrev = (PrinterDbPtr)NULL;
	pEntry != (PrinterDbPtr)NULL; pEntry = pEntry->next)
    {
	for(pDrvEnt = driverMap; pDrvEnt != (DriverMapPtr)NULL; 
	    pDrvEnt = pDrvEnt->next)
	{
	    if(!strcmp(printerDb->driverName, pDrvEnt->driverName))
		break;
	}

	/*
	 * Either store the screen number, or delete the printerDb entry.
	 * Deleting the entry leaves orphaned attribute stores, but they'll
	 * get cleaned up at the next rehash or server recycle.
	 */
	if(pDrvEnt != (DriverMapPtr)NULL) 
	{
	    pEntry->screenNum = pDrvEnt->screenNum;
	    pPrev = pEntry;
	}
	else {
	    if(pPrev)
	        pPrev->next = pEntry->next;
	    else
		pPrev = pEntry->next;
	    if(pEntry->name != (char *)NULL)
		xfree(pEntry->name);
	    xfree(pEntry);
	    pEntry = pPrev;
	}
    }

    FreeDriverMap(driverMap);

    return Success;
}

/*
 * ValidateFontDir looks for a valid font directory for the specified
 * printer model within the specified configuration directory. It returns
 * the directory name, or NULL if no valid font directory was found.
 * It is the caller's responsibility to free the returned font directory
 * name.
 */
static char *
ValidateFontDir(
    char *configDir, 
    char *modelName)
{
    char *pathName;

    if(!configDir || !modelName)
	return (char *)NULL;

    pathName = (char *)xalloc(strlen(configDir) + strlen(MODELDIRNAME) +
			      strlen(modelName) + strlen(FONTDIRNAME) + 
			      strlen("fonts.dir") + 5);
    if(!pathName)
	return (char *)NULL;
    sprintf(pathName, "%s/%s/%s/%s/%s", configDir, MODELDIRNAME, modelName,
	    FONTDIRNAME, "fonts.dir");
    if(access(pathName, R_OK) != 0)
    {
	xfree(pathName);
	return (char *)NULL;
    }
    pathName[strlen(pathName) - 9] = (char)'\0'; /* erase fonts.dir */
    return pathName;
}

/*
 * FindFontDir returns a pointer to the path name of the font directory
 * for the specified printer model name, if such a directory exists.
 * The directory contents are superficially checked for validity.
 * The caller must free the returned char *.
 *
 * We first look in the locale-specific model-config directory, and
 * then fall back to the C language model-config directory.
 */
static char *
FindFontDir(
    char *modelName)
{
    char *configDir, *fontDir;

    if(!modelName || !strlen(modelName))
        return (char *)NULL;
    
    configDir = XpGetConfigDir(TRUE);
    if ((fontDir = ValidateFontDir(configDir, modelName)) != 0)
    {
	xfree(configDir);
	return fontDir;
    }

    if(configDir) 
	xfree(configDir);
    configDir = XpGetConfigDir(FALSE);
    fontDir = ValidateFontDir(configDir, modelName);

    xfree(configDir);

    return fontDir;
}

/*
 * AddToFontPath adds the specified font path element to the global
 * defaultFontPath string. It adds the keyword "PRINTER:" to the front
 * of the path to denote that this is a printer-specific font path
 * element.
 */
static char PATH_PREFIX[] = "PRINTER:";
static int PATH_PREFIX_LEN = sizeof(PATH_PREFIX) - 1; /* same as strlen() */

static void
AddToFontPath(
    char *pathName)
{
    char *newPath;
    Bool freeOldPath;

    if(defaultFontPath == origFontPath)
	freeOldPath = FALSE;
    else
	freeOldPath = TRUE;

    newPath = (char *)xalloc(strlen(defaultFontPath) + strlen(pathName) + 
			     PATH_PREFIX_LEN + 2);

    sprintf(newPath, "%s%s,%s", PATH_PREFIX, pathName, defaultFontPath);

    if(freeOldPath)
	xfree(defaultFontPath);

    defaultFontPath = newPath;
    return;
}

/*
 * AugmentFontPath adds printer-model-specific font path elements to
 * the front of the font path global variable "defaultFontPath" (dix/globals.c).
 * We can't call SetFontPath() because the font code has not yet been 
 * initialized when InitOutput is called (from whence this routine is called).
 *
 * This utilizes the static variables origFontPath and 
 * freeDefaultFontPath to track the original contents of defaultFontPath,
 * and to properly free the modified version upon server recycle.
 */
static void
AugmentFontPath(void)
{
    char *modelID, **allIDs = (char **)NULL;
    PrinterDbPtr pDbEntry;
    int numModels, i;

    if(!origFontPath)
	origFontPath = defaultFontPath;

    if(freeDefaultFontPath)
    {
	xfree(defaultFontPath);
	defaultFontPath = origFontPath;
	freeDefaultFontPath = FALSE;
    }

    /*
     * Build a list of printer models to check for internal fonts.
     */
    for(pDbEntry = printerDb, numModels = 0; 
	pDbEntry != (PrinterDbPtr)NULL; 
	pDbEntry = pDbEntry->next)
    {
	modelID =
	    (char*)XpGetPrinterAttribute(pDbEntry->name,
					 "xp-model-identifier");

	if(modelID && strlen(modelID) != 0)
	{
	    /* look for current model in the list of allIDs */
	    for(i = 0; i < numModels; i++)
	    {
	        if(!strcmp(modelID, allIDs[i]))
	        {
		    modelID = (char *)NULL;
		    break;
	        }
	    }
	}

	/*
	 * If this printer's model-identifier isn't in the allIDs list,
	 * then add it to allIDs.
	 */
	if(modelID && strlen(modelID) != 0)
	{
	    allIDs = (char **)xrealloc(allIDs, (numModels+2) * sizeof(char *));
	    if(allIDs == (char **)NULL)
	        return;
	    allIDs[numModels] = modelID;
	    allIDs[numModels + 1] = (char *)NULL;
	    numModels++;
	}
    }

    /* for each model, check for a valid font directory, and add it to
     * the front of defaultFontPath.
     */
    for(i = 0; allIDs != (char **)NULL && allIDs[i] != (char *)NULL; i ++)
    {
	char *fontDir;
	if ((fontDir = FindFontDir(allIDs[i])) != 0)
	{
	    AddToFontPath(fontDir);
	    xfree(fontDir);
	    freeDefaultFontPath = TRUE;
	}
    }

    if(allIDs)
        xfree(allIDs);

    return;
}

/*
 * XpClientIsBitmapClient is called by the font code to find out if
 * a particular client should be granted access to bitmap fonts.
 * This function works by
 * calling XpContextOfClient (in Xserver/Xext/xprint.c) to determine
 * the context associated with the client, and then queries the context's
 * attributes to determine whether the bitmap fonts should be visible.
 * It looks at the value of the xp-listfonts-mode document/page attribute to 
 * see if xp-list-glyph-fonts has been left out of the mode list. Only
 * if the xp-listfonts-mode attribute exists, and it does not contain
 * xp-list-glyph-fonts does this function return FALSE. In any other
 * case the funtion returns TRUE, indicating that the bitmap fonts 
 * should be visible to the client.
 */
Bool
XpClientIsBitmapClient(
    ClientPtr client)
{
    XpContextPtr pContext;
    char *mode;

    if(!(pContext = XpContextOfClient(client)))
	return TRUE;

    /*
     * Check the page attributes, and if it's not defined there, then
     * check the document attributes.
     */
    mode = XpGetOneAttribute(pContext, XPPageAttr, "xp-listfonts-mode");
    if(!mode || !strlen(mode))
    {
        mode = XpGetOneAttribute(pContext, XPDocAttr, "xp-listfonts-mode");
        if(!mode || !strlen(mode))
	    return TRUE;
    }
    
    if(!strstr(mode, "xp-list-glyph-fonts"))
	return FALSE;

    return TRUE;
}
/*
 * XpClientIsPrintClient is called by the font code to find out if
 * a particular client has set a context which references a printer
 * which utilizes a particular font path.  This function works by
 * calling XpContextOfClient (in Xserver/Xext/xprint.c) to determine
 * the context associated with the client, and then looks up the
 * font directory for the context.  The font directory is then compared
 * with the directory specified in the FontPathElement which is passed in.
 */
Bool
XpClientIsPrintClient(
    ClientPtr client,
    FontPathElementPtr fpe)
{
    XpContextPtr pContext;
    char *modelID, *fontDir;

    if(!(pContext = XpContextOfClient(client)))
	return FALSE;

    if (!fpe)
	return TRUE;

    modelID = XpGetOneAttribute(pContext, XPPrinterAttr, "xp-model-identifier");
    if(!modelID || !strlen(modelID))
	return FALSE;
    
    if(!(fontDir = FindFontDir(modelID)))
	return FALSE;

    /*
     * The grunge here is to ignore the PATH_PREFIX at the front of the
     * fpe->name.
     */
    if(fpe->name_length < PATH_PREFIX_LEN || 
       (strlen(fontDir) != (unsigned)(fpe->name_length - PATH_PREFIX_LEN)) ||
       strncmp(fontDir, fpe->name + PATH_PREFIX_LEN, 
	       fpe->name_length - PATH_PREFIX_LEN))
    {
	xfree(fontDir);
	return FALSE;
    }
    xfree(fontDir);
    return TRUE;
}

static void
AddFormats(ScreenInfo *pScreenInfo, char *driverName)
{
    int i, j;
    driverInitRec *pInitRec;
    int numDrivers = sizeof(driverInits)/sizeof(driverInitRec);
    PixmapFormatRec *formats;
    int numfmts;

    for (pInitRec = driverInits, i = 0; i < numDrivers; pInitRec++, i++)
    {
        if ( !strcmp( driverName, pInitRec->driverName ) )
	    break;
    }
    if (i >= numDrivers)
	return;
    formats = pInitRec->pFmts;
    numfmts = pInitRec->numFmts;
    for (i = 0; i < numfmts && pScreenInfo->numPixmapFormats < MAXFORMATS; i++)
    {
	for (j = 0; j < pScreenInfo->numPixmapFormats; j++) {
	    if (pScreenInfo->formats[j].depth == formats[i].depth &&
		pScreenInfo->formats[j].bitsPerPixel == formats[i].bitsPerPixel &&
		pScreenInfo->formats[j].scanlinePad == formats[i].scanlinePad)
		break;
	}
	if (j == pScreenInfo->numPixmapFormats) {
	    pScreenInfo->formats[j] = formats[i];
	    pScreenInfo->numPixmapFormats++;
	}
    }
}

/************************************************************
 * PrinterInitOutput --
 *	This routine is to be called from a ddx's InitOutput
 *      during the server startup initialization, and when
 *      the server is to be reset.  The routine creates the
 *      screens associated with configured printers by calling
 *	dix:AddScreen.  The configuration information comes from a
 *      database read from the X*printers file.
 *
 * Results:
 *	The array of ScreenRec pointers referenced by
 *      pScreenInfo->screen is increased by the addition
 *      of the printer screen(s), as is the value of
 *      pScreenInfo->numScreens.  This is done via calls
 *      to AddScreen() in dix.
 *
 ************************************************************/

void
PrinterInitOutput(
     ScreenInfo *pScreenInfo,
     int argc,
     char **argv)
{
    PrinterDbPtr pDb, pDbEntry;
    int driverCount = 0, i;
    char **driverNames;
    char *configDir;

    /* 
     * this little test is just a warning at startup to make sure
     * that the config directory exists.
     *
     * what this ugly looking if says is that if both ways of
     * calling configDir works and both directories don't exist, 
     * then print an error saying we can't find the non-lang one.
     */
    if (((configDir = XpGetConfigDir(TRUE)) != NULL) && 
	(access(configDir, F_OK) == 0))
    {
        xfree(configDir);
    }
    else if (((configDir = XpGetConfigDir(FALSE)) != NULL) &&
	     (access(configDir, F_OK) == 0))
    {
        xfree(configDir);
    }
    else {
	ErrorF("Xp Extension: could not find config dir %s\n",
	       configDir ? configDir : XPRINTDIR);

	if (configDir) xfree(configDir);
    }

    if(printerDb != (PrinterDbPtr)NULL)
	FreePrinterDb();
	
    /*
     * Calling BuildPrinterDb serves to build the printer database,
     * and to initialize the attribute store for each printer.
     * The driver can, if it so desires, modify the attribute
     * store at a later time.
     */
    if((pDb = BuildPrinterDb()) ==  (PrinterDbPtr)NULL) return;

    /*
     * We now have to decide how many screens to initialize, and call
     * AddScreen for each one. The printerDb must be properly initialized
     * for at least one screen's worth of printers prior to calling AddScreen
     * because InitPrintDrivers reads the printerDb to determine which 
     * driver(s) to init on a particular screen.
     * We put each driver's printers on a different
     * screen, and call AddScreen for each screen/driver pair.
     */
    /* count the number of printers */
    for(pDbEntry = pDb, driverCount = 0; pDbEntry != (PrinterDbPtr)NULL; 
	pDbEntry = pDbEntry->next, driverCount++)
	    ;
    /*
     * Allocate memory for the worst case - a driver per printer
     */
    driverNames = (char **)xalloc(sizeof(char *) * driverCount);

    /*
     * Assign the driver for the first printer to the first screen
     */
    pDb->screenNum = screenInfo.numScreens;
    driverNames[0] = pDb->driverName;
    driverCount = 1;
    AddFormats(pScreenInfo, pDb->driverName);

    /*
     * For each printer, look to see if its driver is already assigned
     * to a screen, and if so copy that screen number into the printerDb.
     * Otherwise, assign a new screen number to the driver for this
     * printer.
     */
    for(pDbEntry = pDb; pDbEntry != (PrinterDbPtr)NULL; 
	pDbEntry = pDbEntry->next)
    {
	Bool foundMatch;

	for(i = 0, foundMatch = FALSE; i < driverCount; i++)
	{
	    if(!strcmp(driverNames[i], pDbEntry->driverName))
	    {
	        foundMatch = TRUE;
	        pDbEntry->screenNum = screenInfo.numScreens + i;
		break;
	    }
	}
	if(foundMatch == FALSE)
	{
	    driverNames[driverCount] = pDbEntry->driverName;
	    pDbEntry->screenNum = screenInfo.numScreens + driverCount;
	    AddFormats(pScreenInfo, pDbEntry->driverName);
	    driverCount++;
	}
    }
       
    for(i = 0; i < driverCount; i++)
    {
	int curScreen = screenInfo.numScreens;
        if(AddScreen(InitPrintDrivers, argc, argv) < 0)
	{
            PrinterDbPtr pPrev;
	    /* 
	     * AddScreen failed, so we pull the associated printers 
	     * from the list.
	     */
	    ErrorF("Xp Extension: Could not add screen for driver %s\n",
		   driverNames[i]);
            for(pPrev = pDbEntry = printerDb; pDbEntry != (PrinterDbPtr)NULL; 
		pDbEntry = pDbEntry->next)
            {
		if(pDbEntry->screenNum == curScreen)
		{
		    if(pPrev == printerDb)
		    {
			printerDb = pDbEntry->next;
			pPrev = printerDb;
		    }
		    else
			pPrev->next = pDbEntry->next;

		    xfree(pDbEntry->name);
		    xfree(pDbEntry);
		    pDbEntry = pPrev;
		}
		else 
		{
		    if(pDbEntry->screenNum > curScreen)
		        pDbEntry->screenNum--;
		    pPrev = pDbEntry;
		}
	    }
	}
    }

    xfree(driverNames);

    AugmentFontPath();

    if(pScreenInfo->numScreens > MAXSCREENS)
    {
	ErrorF("The number of printer screens requested ");
	ErrorF("exceeds the allowable limit of %d screens.\n", MAXSCREENS);
	ErrorF("Please reduce the number of requested printers in your ");
	ErrorF("\nX%sprinters file.", display);
	ErrorF("Server exiting...\n");
	exit(-1);
    }
}

/*
 * InitPrintDrivers is called from dix:AddScreen.  It in turn calls the
 * driver initialization routine for any and all drivers which are
 * implicated in supporting printers on the particular screen number
 * specified by the "index" parameter.  The printerDb variable is used
 * to determine which printers are to be associated with a particular
 * screen.
 */
static Bool
InitPrintDrivers(
    int index,
    ScreenPtr pScreen,
    int argc,
    char **argv)
{
    PrinterDbPtr pDb, pDb2;

    GenericScreenInit(index, pScreen, argc, argv);

    for(pDb = printerDb; pDb != (PrinterDbPtr)NULL; pDb = pDb->next)
    {
	if(pDb->screenNum == index)
	{
	    Bool callInit = TRUE;
	    for(pDb2 = printerDb; pDb2 != pDb; pDb2 = pDb2->next)
	    {
	        if(!strcmp(pDb->driverName, pDb2->driverName))
	        {
		    callInit = FALSE;
		    break;
	        }
	    }
	    if(callInit == TRUE)
	    {
	        pBFunc initFunc;
	        initFunc = GetInitFunc(pDb->driverName);
	        if(initFunc(index, pScreen, argc, argv) == FALSE)
	        {
		    /* XXX - What do I do if the driver's init fails? */
                }
	    }
	}
    }
    return TRUE;
}

void
_XpVoidNoop(void)
{
    return;
}

Bool
_XpBoolNoop(void)
{
    return TRUE;
}

/*
 * GenericScreenInit - The common initializations required by all
 * printer screens and drivers.  It sets the screen's cursor functions
 * to Noops, and computes the maximum screen (i.e. medium) dimensions.
 */

static void
GenericScreenInit(
     int index,
     ScreenPtr pScreen,
     int argc,
     char **argv)
{
    float fWidth, fHeight, maxWidth, maxHeight;
    unsigned short width, height;
    PrinterDbPtr pDb;
    int res, maxRes;
    
    /*
     * Set the cursor ops to no-op functions.
     */
    pScreen->DisplayCursor = (DisplayCursorProcPtr)_XpBoolNoop;
    pScreen->RealizeCursor = (RealizeCursorProcPtr)_XpBoolNoop;
    pScreen->UnrealizeCursor = (UnrealizeCursorProcPtr)_XpBoolNoop;
    pScreen->SetCursorPosition = (SetCursorPositionProcPtr)_XpBoolNoop;
    pScreen->ConstrainCursor = (ConstrainCursorProcPtr)_XpVoidNoop;
    pScreen->CursorLimits = (CursorLimitsProcPtr)_XpVoidNoop;
    pScreen->RecolorCursor = (RecolorCursorProcPtr)_XpVoidNoop;

    /*
     * Find the largest paper size for all the printers on the given
     * screen.
     */
    maxRes = 0;
    maxWidth = maxHeight = 0.0;
    for( pDb = printerDb; pDb != (PrinterDbPtr)NULL; pDb = pDb->next)
      {
	if(pDb->screenNum == index)
	{
	    XpValidatePoolsRec *pValRec;
	    pVFunc dimensionsFunc;

	    GetDimFuncAndRec(pDb->driverName, &pValRec, &dimensionsFunc);
	    if(dimensionsFunc != (pVFunc)NULL)
		dimensionsFunc(pDb->name, pValRec, &fWidth, &fHeight, &res);
	    else
	        XpGetMaxWidthHeightRes(pDb->name, pValRec, &fWidth, 
				       &fHeight, &res);
	    if( res > maxRes )
	      maxRes = res;
	    if( fWidth > maxWidth )
	      maxWidth = fWidth;
	    if( fHeight > maxHeight )
	      maxHeight = fHeight;
	  }
      }
    
    width = (unsigned short) (maxWidth * maxRes / 25.4);
    height = (unsigned short) (maxHeight * maxRes / 25.4);
    pScreen->width = pScreen->height = ( width > height ) ? width :
      height;
    
    pScreen->mmWidth = pScreen->mmHeight = ( maxWidth > maxHeight ) ?
                                           (unsigned short)(maxWidth + 0.5) : 
					   (unsigned short)(maxHeight + 0.5);
}

/*
 * QualifyName - takes an unqualified file name such as X6printers and
 * a colon-separated list of directory path names such as 
 * /etc/opt/dt:/opt/dt/config.
 * 
 * Returns a fully qualified file path name such as /etc/opt/dt/X6printers.
 * The returned value is malloc'd, and the caller is responsible for 
 * freeing the associated memory.
 */
static char *
QualifyName(
    char *fileName,
    char *searchPath)
{
    char * curPath = searchPath;
    char * nextPath;
    char * chance;
    FILE *pFile;

    if (fileName == NULL || searchPath == NULL)
      return NULL;

    while (1) {
      if ((nextPath = strchr(curPath, ':')) != NULL)
        *nextPath = 0;
  
      chance = (char *)xalloc(strlen(curPath) + strlen(fileName) + 2);
      sprintf(chance,"%s/%s",curPath,fileName);
  
      /* see if we can read from the file */
      if((pFile = fopen(chance, "r")) != (FILE *)NULL)
      {
	fclose(pFile);
        /* ... restore the colon, .... */
        if (nextPath)
	  *nextPath = ':';
  
        return chance;
      }
  
      xfree(chance);

      if (nextPath == NULL) /* End of path list? */
        break;
  
      /* try the next path */
      curPath = nextPath + 1;
    }
    return NULL;
}

/*
 * FillPrinterListEntry fills in a single XpDiListEntry element with data
 * derived from the supplied PrinterDbPtr element.
 *
 * XXX A smarter (i.e. future) version of this routine might inspect the
 * XXX "locale" parameter and attempt to match the "description" and
 * XXX "localeName" elements of the XpDiListEntry to the specified locale.
 */
static void
FillPrinterListEntry(
    XpDiListEntry *pEntry,
    PrinterDbPtr pDb,
    int localeLen,
    char *locale)
{
    static char *localeStr = (char *)NULL;

    if(localeStr == (char *)NULL)
	localeStr = strdup(setlocale(LC_ALL, (const char *)NULL));

    pEntry->name = pDb->name;
    pEntry->description =
	(char*)XpGetPrinterAttribute(pDb->name, "descriptor");
    pEntry->localeName = localeStr;
    pEntry->rootWinId = WindowTable[pDb->screenNum]->drawable.id;
}

/*
 * GetPrinterListInfo fills in the XpDiListEntry struct pointed to by the
 * parameter pEntry with the information regarding the printer specified
 * by the name and nameLen parameters.  The pointers placed in the 
 * XpDiListEntry structure MUST NOT be freed by the caller.  They are
 * pointers into existing long-lived databases.
 *
 */
static Bool
GetPrinterListInfo(
    XpDiListEntry *pEntry,
    int nameLen,
    char *name,
    int localeLen,
    char *locale)
{
    PrinterDbPtr pDb;

    for(pDb = printerDb; pDb != (PrinterDbPtr)NULL; pDb = pDb->next)
    {
	if (strlen(pDb->name) == (unsigned)nameLen
	&& !strncmp(pDb->name, name, nameLen))
	{
	    FillPrinterListEntry(pEntry, pDb, localeLen, locale);
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 * XpDiFreePrinterList is the approved method of releasing memory used
 * for a printer list.
 */
void
XpDiFreePrinterList(XpDiListEntry **list)
{
    int i;

    for(i = 0; list[i] != (XpDiListEntry *)NULL; i++)
	xfree(list[i]);
    xfree(list);
}

/*
 * XpDiGetPrinterList returns a pointer to a NULL-terminated array of
 * XpDiListEntry pointers.  Each entry structure contains the name, 
 * description, root window, and locale of a printer.  The call returns
 * either a list of all printers configured on the server, or it returns
 * the information for one specific printer depending on the values passed
 * in.  Non-NULL values passed in indicate that only the information for
 * the one specific printer is desired, while NULL values indicate that
 * the information for all printers is desired.
 */
XpDiListEntry **
XpDiGetPrinterList(
    int nameLen,
    char *name,
    int localeLen,
    char *locale)
{
    XpDiListEntry **pList;

    if(!nameLen || name == (char *)NULL)
    {
	int i;
        PrinterDbPtr pDb;

        for(pDb = printerDb, i = 0; pDb != (PrinterDbPtr)NULL; 
	    pDb = pDb->next, i++)
	    ;

	if((pList = (XpDiListEntry **)xalloc((i+1) * sizeof(XpDiListEntry *)))
	   == (XpDiListEntry **)NULL)
	    return pList;

	pList[i] = (XpDiListEntry *)NULL;
        for(pDb = printerDb, i = 0; pDb != (PrinterDbPtr)NULL; 
	    pDb = pDb->next, i++)
	{
	    if((pList[i] = (XpDiListEntry *)xalloc(sizeof(XpDiListEntry)))==
	       (XpDiListEntry *)NULL)
	    {
		XpDiFreePrinterList(pList);
		return (XpDiListEntry **)NULL;
	    }
            FillPrinterListEntry(pList[i], pDb, localeLen, locale);
	}
    }
    else
    {
	if((pList = (XpDiListEntry **)xalloc(2 * sizeof(XpDiListEntry *))) ==
	   (XpDiListEntry **)NULL)
	    return pList;

	if((pList[0] = (XpDiListEntry *)xalloc(sizeof(XpDiListEntry))) ==
           (XpDiListEntry *)NULL)
        {
	    xfree(pList);
	    return (XpDiListEntry **)NULL;
	}
	pList[1] = (XpDiListEntry *)NULL;
	if(GetPrinterListInfo(pList[0], nameLen, name, localeLen, locale) == 
	   FALSE)
	{
	    xfree(pList[0]);
	    pList[0] = (XpDiListEntry *)NULL;
	}
    }
    return pList;
}

WindowPtr
XpDiValidatePrinter(char *printerName, int printerNameLen)
{
    PrinterDbPtr pCurEntry;

    for(pCurEntry = printerDb;
	pCurEntry != (PrinterDbPtr)NULL; pCurEntry = pCurEntry->next)
    {
        if(strlen(pCurEntry->name) == (unsigned)printerNameLen &&
	   !strncmp(pCurEntry->name, printerName, printerNameLen))
	    return  WindowTable[pCurEntry->screenNum];
    }
    return (WindowPtr)NULL;
}

/*
 * XpDiGetDriverName takes a screen index and a printer name, and returns
 * a pointer to the name of the driver to be used for the specified printer
 * on the specified screen.
 */
char *
XpDiGetDriverName(int index, char *printerName)
{

    PrinterDbPtr pCurEntry;

    for(pCurEntry = printerDb;
	pCurEntry != (PrinterDbPtr)NULL; pCurEntry = pCurEntry->next)
    {
        if(pCurEntry->screenNum == index &&
	   !strcmp(pCurEntry->name, printerName))
	    return pCurEntry->driverName;
    }

    return (char *)NULL; /* XXX Should we supply a default driverName? */
}
