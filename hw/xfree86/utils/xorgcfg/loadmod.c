/*
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
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
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Author: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 *
 */

#ifdef USE_MODULES
#include <setjmp.h>

#ifndef HAS_GLIBC_SIGSETJMP
#if defined(setjmp) && defined(__GNU_LIBRARY__) && \
    (!defined(__GLIBC__) || (__GLIBC__ < 2) || \
     ((__GLIBC__ == 2) && (__GLIBC_MINOR__ < 3)))
#define HAS_GLIBC_SIGSETJMP 1
#endif
#endif

#define LOADER_PRIVATE
#include "loader.h"

#define	True		1
#define False		0
#define XtPointer	char*
#define XtMalloc	malloc
#define XtCalloc	calloc
#define XtRealloc	realloc
#define XtFree		free
#define XtNew(t)	malloc(sizeof(t))
#define XtNewString(s)	((s) ? strdup(s) : NULL)

#define	pointer void*

/* XXX beware (or fix it) libc functions called here are the xf86 ones */

static void AddModuleOptions(char*, const OptionInfoRec*);
#if 0
void xf86AddDriver(DriverPtr, void*, int);
Bool xf86ServerIsOnlyDetecting(void);
void xf86AddInputDriver(InputDriverPtr, pointer, int);
void xf86AddModuleInfo(ModuleInfoPtr, void*);
Bool xf86LoaderCheckSymbol(const char*);
void xf86LoaderRefSymLists(const char **, ...);
void xf86LoaderReqSymLists(const char **, ...);
void xf86Msg(int, const char*, ...);
void xf86MsgVerb(int, int, const char*, ...);
void xf86PrintChipsets(const char*, const char*, SymTabPtr);
void xf86ErrorFVerb(int verb, const char *format, ...);
int xf86MatchDevice(const char*, GDevPtr**);
int xf86MatchPciInstances(const char*, int, SymTabPtr, PciChipsets*, GDevPtr*, int, DriverPtr,int**);
int xf86MatchIsaInstances(const char*, SymTabPtr, pointer*, DriverPtr, pointer, GDevPtr*, int, int**);
void *xf86LoadDrvSubModule(DriverPtr drv, const char*);
void xf86DrvMsg(int, int, const char*, ...);
Bool xf86IsPrimaryPci(pcVideoPtr*);
Bool xf86CheckPciSlot( const struct pci_device * );
#endif

extern char *loaderPath, **loaderList, **ploaderList;
xf86cfgModuleOptions *module_options;

extern int noverify, error_level;

int xf86ShowUnresolved = 1;

LOOKUP miLookupTab[]      = {{0,0}};
LOOKUP dixLookupTab[]     = {{0,0}};
LOOKUP extLookupTab[]     = {{0,0}};
LOOKUP xfree86LookupTab[] = {
       /* Loader functions */
   SYMFUNC(LoaderDefaultFunc)
   SYMFUNC(LoadSubModule)
   SYMFUNC(DuplicateModule)
   SYMFUNC(LoaderErrorMsg)
   SYMFUNC(LoaderCheckUnresolved)
   SYMFUNC(LoadExtension)
   SYMFUNC(LoaderReqSymbols)
   SYMFUNC(LoaderReqSymLists)
   SYMFUNC(LoaderRefSymbols)
   SYMFUNC(LoaderRefSymLists)
   SYMFUNC(UnloadSubModule)
   SYMFUNC(LoaderSymbol)
   SYMFUNC(LoaderListDirs)
   SYMFUNC(LoaderFreeDirList)
   SYMFUNC(LoaderGetOS)

    SYMFUNC(xf86AddDriver)
    SYMFUNC(xf86ServerIsOnlyDetecting)
    SYMFUNC(xf86AddInputDriver)
    SYMFUNC(xf86AddModuleInfo)
    SYMFUNC(xf86LoaderCheckSymbol)

    SYMFUNC(xf86LoaderRefSymLists)
    SYMFUNC(xf86LoaderReqSymLists)
    SYMFUNC(xf86Msg)
    SYMFUNC(xf86MsgVerb)
    SYMFUNC(ErrorF)
    SYMFUNC(xf86PrintChipsets)
    SYMFUNC(xf86ErrorFVerb)
    SYMFUNC(xf86MatchDevice)
    SYMFUNC(xf86MatchPciInstances)
    SYMFUNC(xf86MatchIsaInstances)
    SYMFUNC(Xfree)
    SYMFUNC(xf86LoadDrvSubModule)
    SYMFUNC(xf86DrvMsg)
    SYMFUNC(xf86IsPrimaryPci)
    SYMFUNC(xf86CheckPciSlot)
    SYMFUNC(XNFalloc)
    SYMFUNC(XNFrealloc)
    SYMFUNC(XNFcalloc)
    {0,0}
};

static DriverPtr driver;
static ModuleInfoPtr info;
static SymTabPtr chips;
static int vendor;
ModuleType module_type = GenericModule;

static void
AddModuleOptions(char *name, const OptionInfoRec *option)
{
    xf86cfgModuleOptions *ptr;
    const OptionInfoRec *tmp;
    SymTabPtr ctmp;
    int count;

    /* XXX If the module is already in the list, then it means that
     * it is now being properly loaded by xf86cfg and the "fake" entry
     * added in xf86cfgLoaderInitList() isn't required anymore.
     * Currently:
     *	ati and vmware are known to fail. */
    for (ptr = module_options; ptr; ptr = ptr->next)
	if (strcmp(name, ptr->name) == 0) {
	    fprintf(stderr, "Module %s already in list!\n", name);
	    return;
	}

    ptr = XtNew(xf86cfgModuleOptions);
    ptr->name = XtNewString(name);
    ptr->type = module_type;
    if (option) {
	for (count = 0, tmp = option; tmp->name != NULL; tmp++, count++)
	    ;
	++count;
	ptr->option = XtCalloc(1, count * sizeof(OptionInfoRec));
	for (count = 0, tmp = option; tmp->name != NULL; count++, tmp++) {
	    memcpy(&ptr->option[count], tmp, sizeof(OptionInfoRec));
	    ptr->option[count].name = XtNewString(tmp->name);
	    if (tmp->type == OPTV_STRING || tmp->type == OPTV_ANYSTR)
		ptr->option[count].value.str = XtNewString(tmp->value.str);
	}
    }
    else
	ptr->option = NULL;
    if (vendor != -1 && chips) {
	ptr->vendor = vendor;
	for (count = 0, ctmp = chips; ctmp->name; ctmp++, count++)
	    ;
	++count;
	ptr->chipsets = XtCalloc(1, count * sizeof(SymTabRec));
	for (count = 0, ctmp = chips; ctmp->name != NULL; count++, ctmp++) {
	    memcpy(&ptr->chipsets[count], ctmp, sizeof(SymTabRec));
	    ptr->chipsets[count].name = XtNewString(ctmp->name);
	}
    }
    else
	ptr->chipsets = NULL;

    ptr->next = module_options;
    module_options = ptr;
}

extern void xf86WrapperInit(void);

void
xf86cfgLoaderInit(void)
{
    LoaderInit();
    xf86WrapperInit();
}

void
xf86cfgLoaderInitList(int type)
{
    static const char *generic[] = {
	".",
	NULL
    };
    static const char *video[] = {
	"drivers",
	NULL
    };
    static const char *input[] = {
	"input",
	NULL
    };
    const char **subdirs;

    switch (type) {
	case GenericModule:
	    subdirs = generic;
	    break;
	case VideoModule:
	    subdirs = video;
	    break;
	case InputModule:
	    subdirs = input;
	    break;
	default:
	    fprintf(stderr, "Invalid value passed to xf86cfgLoaderInitList.\n");
	    subdirs = generic;
	    break;
    }
    LoaderSetPath(loaderPath);
    loaderList = LoaderListDirs(subdirs, NULL);

    /* XXX Xf86cfg isn't able to provide enough wrapper functions
     * to these drivers. Maybe the drivers could also be changed
     * to work better when being loaded "just for testing" */
    if (type == VideoModule) {
	module_type = VideoModule;
	AddModuleOptions("vmware", NULL);
	AddModuleOptions("ati", NULL);
	module_type = NullModule;
    }
}

void
xf86cfgLoaderFreeList(void)
{
    LoaderFreeDirList(loaderList);
}

int
xf86cfgCheckModule(void)
{
    int errmaj, errmin;
    ModuleDescPtr module;

    driver = NULL;
    chips = NULL;
    info = NULL;
    vendor = -1;
    module_type = GenericModule;

    if ((module = LoadModule(*ploaderList, NULL, NULL, NULL, NULL,
			     NULL, &errmaj, &errmin)) == NULL) {
	LoaderErrorMsg(NULL, *ploaderList, errmaj, errmin);
	return (0);
    }
    else if (driver && driver->AvailableOptions) {
	/* at least fbdev does not call xf86MatchPciInstances in Probe */
	if (driver->Identify)
	    (*driver->Identify)(-1);
	if (driver->Probe)
	    (*driver->Probe)(driver, PROBE_DETECT);
	AddModuleOptions(*ploaderList, (*driver->AvailableOptions)(-1, -1));
    }
    else if (info && info->AvailableOptions)
	AddModuleOptions(*ploaderList, (*info->AvailableOptions)(NULL));

    if (!noverify) {
	XF86ModuleData *initdata = NULL;
	char *p;

	p = XtMalloc(strlen(*ploaderList) + strlen("ModuleData") + 1);
	strcpy(p, *ploaderList);
	strcat(p, "ModuleData");
	initdata = LoaderSymbol(p);
	if (initdata) {
	    XF86ModuleVersionInfo *vers;

	    vers = initdata->vers;
	    if (vers && strcmp(*ploaderList, vers->modname)) {
		/* This was a problem at some time for some video drivers */
		CheckMsg(CHECKER_FILE_MODULE_NAME_MISMATCH,
			 "WARNING file/module name mismatch: \"%s\" \"%s\"\n",
			 *ploaderList, vers->modname);
		++error_level;
	    }
	}
	XtFree(p);
    }

    UnloadModule(module);

    return (1);
}

_X_EXPORT void
xf86AddDriver(DriverPtr drv, void *module, int flags)
{
    driver = drv;
    if (driver)
	driver->module = module;
    module_type = VideoModule;
}

_X_EXPORT Bool
xf86ServerIsOnlyDetecting(void)
{
    return (True);
}

_X_EXPORT void
xf86AddInputDriver(InputDriverPtr inp, void *module, int flags)
{
    module_type = InputModule;
}

_X_EXPORT void
xf86AddModuleInfo(ModuleInfoPtr inf, void *module)
{
    info = inf;
}

_X_EXPORT Bool
xf86LoaderCheckSymbol(const char *symbol)
{
    return LoaderSymbol(symbol) != NULL;
}

_X_EXPORT void
xf86LoaderRefSymLists(const char **list0, ...)
{
}

_X_EXPORT void
xf86LoaderReqSymLists(const char **list0, ...)
{
}

#if 0
void xf86Msg(int type, const char *format, ...)
{
}
#endif

/*ARGSUSED*/
_X_EXPORT void
xf86PrintChipsets(const char *name, const char *msg, SymTabPtr chipsets)
{
    vendor = 0;
    chips = chipsets;
}

_X_EXPORT int
xf86MatchDevice(const char *name, GDevPtr **gdev)
{
    *gdev = NULL;

    return (1);
}

_X_EXPORT int
xf86MatchPciInstances(const char *name, int VendorID, SymTabPtr chipsets, PciChipsets *PCIchipsets,
		      GDevPtr *devList, int numDevs, DriverPtr drvp, int **foundEntities)
{
    vendor = VendorID;
    if (chips == NULL)
	chips = chipsets;
    *foundEntities = NULL;

    return (0);
}

_X_EXPORT int
xf86MatchIsaInstances(const char *name, SymTabPtr chipsets, IsaChipsets *ISAchipsets, DriverPtr drvp,
		      FindIsaDevProc FindIsaDevice, GDevPtr *devList, int numDevs, int **foundEntities)
{
    *foundEntities = NULL;

    return (0);
}

/*ARGSUSED*/
_X_EXPORT void *
xf86LoadDrvSubModule(DriverPtr drv, const char *name)
{
    pointer ret;
    int errmaj = 0, errmin = 0;

    ret = LoadSubModule(drv->module, name, NULL, NULL, NULL, NULL,
			&errmaj, &errmin);
    if (!ret)
	LoaderErrorMsg(NULL, name, errmaj, errmin);
    return (ret);
}

_X_EXPORT Bool
xf86IsPrimaryPci(pciVideoPtr pPci)
{
    return (True);
}

_X_EXPORT Bool 
xf86CheckPciSlot( const struct pci_device * d )
{
    (void) d;
    return (False);
}
#endif
