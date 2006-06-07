/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loader.c,v 1.71 2003/11/06 18:38:13 tsi Exp $ */

/*
 * Copyright 1995-1998 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#if defined(UseMMAP) || (defined(linux) && defined(__ia64__))
#include <sys/mman.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#if defined(linux) && \
    (defined(__alpha__) || defined(__powerpc__) || defined(__ia64__) \
    || defined(__amd64__))
#include <malloc.h>
#endif
#include <stdarg.h>

#include "os.h"
#include "sym.h"
#include "loader.h"
#include "loaderProcs.h"
#include "xf86.h"
#include "xf86Priv.h"

#include "compiler.h"

extern LOOKUP miLookupTab[];
extern LOOKUP xfree86LookupTab[];
extern LOOKUP dixLookupTab[];
extern LOOKUP fontLookupTab[];
extern LOOKUP extLookupTab[];

/*
#define DEBUG
#define DEBUGAR
#define DEBUGLIST
#define DEBUGMEM
*/

int check_unresolved_sema = 0;

#if defined(Lynx) && defined(sun)
/* Cross build machine doesn;t have strerror() */
#define strerror(err) "strerror unsupported"
#endif

#ifdef __UNIXOS2__
void *os2ldcalloc(size_t, size_t);
#endif

#ifdef HANDLE_IN_HASH_ENTRY
/*
 * handles are used to identify files that are loaded. Even archives
 * are counted as a single file.
 */
#define MAX_HANDLE 256
#define HANDLE_FREE 0
#define HANDLE_USED 1
static char freeHandles[MAX_HANDLE];
static int refCount[MAX_HANDLE];
#endif

#if defined(__sparc__) && defined(__GNUC__) && !defined(__FreeBSD__)
# define SYMFUNCDOT(func) { "." #func, (funcptr)&__sparc_dot_ ## func },
# if !defined(__OpenBSD__)
# define SYMFUNCDOT89(func) { "." #func, (funcptr)&func ## _sparcv89 },
# define DEFFUNCDOT(func) 					\
extern void __sparc_dot_ ## func (void) __asm__ ("." #func);	\
extern void func ## _sparcv89 (void);
# else
# define SYMFUNCDOT(func) { "." #func, (funcptr)&__sparc_dot_ ## func },
# define DEFFUNCDOT(func) 					\
extern void __sparc_dot_ ## func (void) __asm__ ("." #func);
#endif
DEFFUNCDOT(rem)
DEFFUNCDOT(urem)
DEFFUNCDOT(mul)
DEFFUNCDOT(umul)
DEFFUNCDOT(div)
DEFFUNCDOT(udiv)
#ifdef linux
static LOOKUP SparcV89LookupTab[] = {
    SYMFUNCDOT89(rem)
    SYMFUNCDOT89(urem)
    SYMFUNCDOT89(mul)
    SYMFUNCDOT89(umul)
    SYMFUNCDOT89(div)
    SYMFUNCDOT89(udiv)
    {0, 0}
};
#endif
static LOOKUP SparcLookupTab[] = {
    SYMFUNCDOT(rem)
    SYMFUNCDOT(urem)
    SYMFUNCDOT(mul)
    SYMFUNCDOT(umul)
    SYMFUNCDOT(div)
    SYMFUNCDOT(udiv)
    {0, 0}
};

#ifdef linux
#if defined(__GNUC__) && defined(__GLIBC__)
#define HWCAP_SPARC_MULDIV	8
extern unsigned long int _dl_hwcap;
#endif

static int
sparcUseHWMulDiv(void)
{
    FILE *f;
    char buffer[1024];
    char *p;

#if defined(__GNUC__) && defined(__GLIBC__)
    unsigned long *hwcap;

    __asm(".weak _dl_hwcap");

    hwcap = &_dl_hwcap;
  __asm("": "=r"(hwcap):"0"(hwcap));
    if (hwcap) {
	if (*hwcap & HWCAP_SPARC_MULDIV)
	    return 1;
	else
	    return 0;
    }
#endif
    f = fopen("/proc/cpuinfo", "r");
    if (!f)
	return 0;
    while (fgets(buffer, 1024, f) != NULL) {
	if (!strncmp(buffer, "type", 4)) {
	    p = strstr(buffer, "sun4");
	    if (p && (p[4] == 'u' || p[4] == 'd')) {
		fclose(f);
		return 1;
	    } else if (p && p[4] == 'm') {
		fclose(f);
		f = fopen("/proc/cpuinfo","r");
		if (!f) return 0;
		while (fgets(buffer, 1024, f) != NULL) {
		    if (!strncmp (buffer, "MMU type", 8)) {
		      p = strstr (buffer, "Cypress");
		      if (p) {
			fclose(f);
			return 1;
		      }
		    }
		}
	        fclose(f);
	        return 0;
	    }
	}
    }
    fclose(f);
    return 0;
}
#endif
#endif

/*
 * modules are used to identify compilation units (ie object modules).
 * Archives contain multiple modules, each of which is treated seperately.
 */
static int moduleseq = 0;

/*
 * GDB Interface
 * =============
 *
 * Linked list of loaded modules - gdb will traverse this to determine
 * whether it needs to add the symbols for the loaded module.
 */
LDRModulePtr ModList = 0;

/* Flag which gdb sets to let us know we're being debugged */
char DebuggerPresent = 0;

/* List of common symbols */
LDRCommonPtr ldrCommons;
int nCommons;

typedef struct {
    int num;
    const char **list;
} symlist;

/*
 * List of symbols that may be referenced, and which are allowed to be
 * unresolved providing that they don't appear on the "reqired" list.
 */
static symlist refList = { 0, NULL };

/* List of symbols that must not be unresolved */
static symlist reqList = { 0, NULL };

static int fatalReqSym = 0;

/* Prototypes for static functions. */
static int _GetModuleType(int, long);
static loaderPtr _LoaderListPush(void);
static loaderPtr _LoaderListPop(int);
 /*ARGSUSED*/ static char *
ARCHIVEAddressToSection(void *modptr, unsigned long address)
{
    return NULL;
}

/*
 * Array containing entry points for different formats.
 */

static loader_funcs funcs[] = {
    /* LD_ELFDLOBJECT */
    {DLLoadModule,
     DLResolveSymbols,
     DLCheckForUnresolved,
     ARCHIVEAddressToSection,
     DLUnloadModule},
};

int numloaders = sizeof(funcs) / sizeof(loader_funcs);

void
LoaderInit(void)
{
    const char *osname = NULL;

    char *ld_bind_now = getenv("LD_BIND_NOW");
    if (ld_bind_now && *ld_bind_now) {
        xf86Msg(X_ERROR, "LD_BIND_NOW is set, dlloader will NOT work!\n");
    }

    LoaderAddSymbols(-1, -1, miLookupTab);
    LoaderAddSymbols(-1, -1, xfree86LookupTab);
    LoaderAddSymbols(-1, -1, dixLookupTab);
    LoaderAddSymbols(-1, -1, fontLookupTab);
    LoaderAddSymbols(-1, -1, extLookupTab);
#if defined(__sparc__) && !defined(__FreeBSD__)
#ifdef linux
    if (sparcUseHWMulDiv())
	LoaderAddSymbols(-1, -1, SparcV89LookupTab);
    else
#endif
	LoaderAddSymbols(-1, -1, SparcLookupTab);
#endif

    xf86MsgVerb(X_INFO, 2, "Module ABI versions:\n");
    xf86ErrorFVerb(2, "\t%s: %d.%d\n", ABI_CLASS_ANSIC,
		   GET_ABI_MAJOR(LoaderVersionInfo.ansicVersion),
		   GET_ABI_MINOR(LoaderVersionInfo.ansicVersion));
    xf86ErrorFVerb(2, "\t%s: %d.%d\n", ABI_CLASS_VIDEODRV,
		   GET_ABI_MAJOR(LoaderVersionInfo.videodrvVersion),
		   GET_ABI_MINOR(LoaderVersionInfo.videodrvVersion));
    xf86ErrorFVerb(2, "\t%s : %d.%d\n", ABI_CLASS_XINPUT,
		   GET_ABI_MAJOR(LoaderVersionInfo.xinputVersion),
		   GET_ABI_MINOR(LoaderVersionInfo.xinputVersion));
    xf86ErrorFVerb(2, "\t%s : %d.%d\n", ABI_CLASS_EXTENSION,
		   GET_ABI_MAJOR(LoaderVersionInfo.extensionVersion),
		   GET_ABI_MINOR(LoaderVersionInfo.extensionVersion));
    xf86ErrorFVerb(2, "\t%s : %d.%d\n", ABI_CLASS_FONT,
		   GET_ABI_MAJOR(LoaderVersionInfo.fontVersion),
		   GET_ABI_MINOR(LoaderVersionInfo.fontVersion));

    LoaderGetOS(&osname, NULL, NULL, NULL);
    if (osname)
	xf86MsgVerb(X_INFO, 2, "Loader running on %s\n", osname);

#if defined(linux) && \
    (defined(__alpha__) || defined(__powerpc__) || defined(__ia64__) \
     || ( defined __amd64__ && ! defined UseMMAP && ! defined DoMMAPedMerge))
    /*
     * The glibc malloc uses mmap for large allocations anyway. This breaks
     * some relocation types because the offset overflow. See loader.h for more
     * details. We need to turn off this behavior here.
     */
    mallopt(M_MMAP_MAX, 0);
#endif
#if defined(__UNIXWARE__) && !defined(__GNUC__)
    /* For UnixWare we need to load the C Runtime libraries which are
     * normally auto-linked by the compiler. Otherwise we are bound to
     * see unresolved symbols when trying to use the type "long long".
     * Obviously, this does not apply if the GNU C compiler is used.
     */
    {
	int errmaj, errmin, wasLoaded; /* place holders */
	char *xcrtpath = DEFAULT_MODULE_PATH "/libcrt.a";
	char *uwcrtpath = "/usr/ccs/lib/libcrt.a";
	char *path;
	struct stat st;

	if(stat(xcrtpath, &st) < 0)
	    path = uwcrtpath; /* fallback: try to get libcrt.a from the uccs */
	else
	    path = xcrtpath; /* get the libcrt.a we compiled with */
	LoaderOpen (path, "libcrt", 0, &errmaj, &errmin, &wasLoaded);
    }
#endif
}

/*
 * Determine what type of object is being loaded.
 * This function is responsible for restoring the offset.
 * The fd and offset are used here so that when Archive processing
 * is enabled, individual elements of an archive can be evaluated
 * so the correct loader_funcs can be determined.
 */
static int
_GetModuleType(int fd, long offset)
{
    return LD_ELFDLOBJECT;
}

static loaderPtr listHead = (loaderPtr) 0;

static loaderPtr
_LoaderListPush()
{
    loaderPtr item = xf86loadercalloc(1, sizeof(struct _loader));

    item->next = listHead;
    listHead = item;

    return item;
}

static loaderPtr
_LoaderListPop(int handle)
{
    loaderPtr item = listHead;
    loaderPtr *bptr = &listHead;	/* pointer to previous node */

    while (item) {
	if (item->handle == handle) {
	    *bptr = item->next;	/* remove this from the list */
	    return item;
	}
	bptr = &(item->next);
	item = item->next;
    }

    return 0;
}

/*
 * _LoaderHandleToName() will return the name of the first module with a
 * given handle. This requires getting the last module on the LIFO with
 * the given handle.
 */
char *
_LoaderHandleToName(int handle)
{
    loaderPtr item = listHead;
    loaderPtr aritem = NULL;
    loaderPtr lastitem = NULL;

    if (handle < 0) {
	return "(built-in)";
    }
    while (item) {
	if (item->handle == handle) {
	    if (strchr(item->name, ':') == NULL)
		aritem = item;
	    else
		lastitem = item;
	}
	item = item->next;
    }

    if (aritem)
	return aritem->name;

    if (lastitem)
	return lastitem->name;

    return 0;
}

/*
 * _LoaderHandleToCanonicalName() will return the cname of the first module
 * with a given handle. This requires getting the last module on the LIFO with
 * the given handle.
 */
char *
_LoaderHandleToCanonicalName(int handle)
{
    loaderPtr item = listHead;
    loaderPtr lastitem = NULL;

    if (handle < 0) {
	return "(built-in)";
    }
    while (item) {
	if (item->handle == handle) {
	    lastitem = item;
	}
	item = item->next;
    }

    if (lastitem)
	return lastitem->cname;

    return NULL;
}

/*
 * _LoaderModuleToName() will return the name of the first module with a
 * given handle. This requires getting the last module on the LIFO with
 * the given handle.
 */
char *
_LoaderModuleToName(int module)
{
    loaderPtr item = listHead;
    loaderPtr aritem = NULL;
    loaderPtr lastitem = NULL;

    if (module < 0) {
	return "(built-in)";
    }
    while (item) {
	if (item->module == module) {
	    if (strchr(item->name, ':') == NULL)
		aritem = item;
	    else
		lastitem = item;
	}
	item = item->next;
    }

    if (aritem)
	return aritem->name;

    if (lastitem)
	return lastitem->name;

    return 0;
}

/*
 * _LoaderAddressToSection() will return the name of the file & section
 * that contains the given address.
 */
int
_LoaderAddressToSection(const unsigned long address, const char **module,
			const char **section)
{
    loaderPtr item = listHead;

    while (item) {
	if ((*section =
	     item->funcs->AddressToSection(item->private, address)) != NULL) {
	    *module = _LoaderModuleToName(item->module);
	    return 1;
	}
	item = item->next;
    }

    return 0;
}

/*
 * Add a list of symbols to the referenced list.
 */

static void
AppendSymbol(symlist * list, const char *sym)
{
    list->list = xnfrealloc(list->list, (list->num + 1) * sizeof(char **));
    /* copy the symbol, since it comes from a module 
       that can be unloaded later */
    list->list[list->num] = xnfstrdup(sym);
    list->num++;
}

static void
AppendSymList(symlist * list, const char **syms)
{
    while (*syms) {
	AppendSymbol(list, *syms);
	syms++;
    }
}

static int
SymInList(symlist * list, char *sym)
{
    int i;

    for (i = 0; i < list->num; i++)
	if (strcmp(list->list[i], sym) == 0)
	    return 1;

    return 0;
}

void
LoaderVRefSymbols(const char *sym0, va_list args)
{
    const char *s;

    if (sym0 == NULL)
	return;

    s = sym0;
    do {
	AppendSymbol(&refList, s);
	s = va_arg(args, const char *);
    } while (s != NULL);
}

_X_EXPORT void
LoaderRefSymbols(const char *sym0, ...)
{
    va_list ap;

    va_start(ap, sym0);
    LoaderVRefSymbols(sym0, ap);
    va_end(ap);
}

void
LoaderVRefSymLists(const char **list0, va_list args)
{
    const char **l;

    if (list0 == NULL)
	return;

    l = list0;
    do {
	AppendSymList(&refList, l);
	l = va_arg(args, const char **);
    } while (l != NULL);
}

_X_EXPORT void
LoaderRefSymLists(const char **list0, ...)
{
    va_list ap;

    va_start(ap, list0);
    LoaderVRefSymLists(list0, ap);
    va_end(ap);
}

void
LoaderVReqSymLists(const char **list0, va_list args)
{
    const char **l;

    if (list0 == NULL)
	return;

    l = list0;
    do {
	AppendSymList(&reqList, l);
	l = va_arg(args, const char **);
    } while (l != NULL);
}

_X_EXPORT void
LoaderReqSymLists(const char **list0, ...)
{
    va_list ap;

    va_start(ap, list0);
    LoaderVReqSymLists(list0, ap);
    va_end(ap);
}

void
LoaderVReqSymbols(const char *sym0, va_list args)
{
    const char *s;

    if (sym0 == NULL)
	return;

    s = sym0;
    do {
	AppendSymbol(&reqList, s);
	s = va_arg(args, const char *);
    } while (s != NULL);
}

_X_EXPORT void
LoaderReqSymbols(const char *sym0, ...)
{
    va_list ap;

    va_start(ap, sym0);
    LoaderVReqSymbols(sym0, ap);
    va_end(ap);
}

/* 
 * _LoaderHandleUnresolved() decides what to do with an unresolved
 * symbol.  Symbols that are not on the "referenced" or "required" lists
 * get a warning if they are unresolved.  Symbols that are on the "required"
 * list generate a fatal error if they are unresolved.
 */

int
_LoaderHandleUnresolved(char *symbol, char *module)
{
    int fatalsym = 0;

    if (xf86ShowUnresolved && !fatalsym) {
	if (SymInList(&reqList, symbol)) {
	    fatalReqSym = 1;
	    ErrorF("Required symbol %s from module %s is unresolved!\n",
		   symbol, module);
	}
	if (!SymInList(&refList, symbol)) {
	    ErrorF("Symbol %s from module %s is unresolved!\n",
		   symbol, module);
	}
    }
    return (fatalsym);
}

/*
 * Relocation list manipulation routines
 */

/*
 * Public Interface to the loader.
 */

int
LoaderOpen(const char *module, const char *cname, int handle,
	   int *errmaj, int *errmin, int *wasLoaded, int flags)
{
    loaderPtr tmp;
    int new_handle, modtype;
    int fd;
    LOOKUP *pLookup;

#if defined(DEBUG)
    ErrorF("LoaderOpen(%s)\n", module);
#endif

    /*
     * Check to see if the module is already loaded.
     * Only if we are loading it into an existing namespace.
     * If it is to be loaded into a new namespace, don't check.
     * Note: We only have one namespace.
     */
    if (handle >= 0) {
	tmp = listHead;
	while (tmp) {
#ifdef DEBUGLIST
	    ErrorF("strcmp(%x(%s),{%x} %x(%s))\n", module, module,
		   &(tmp->name), tmp->name, tmp->name);
#endif
	    if (!strcmp(module, tmp->name)) {
		refCount[tmp->handle]++;
		if (wasLoaded)
		    *wasLoaded = 1;
		xf86MsgVerb(X_INFO, 2, "Reloading %s\n", module);
		return tmp->handle;
	    }
	    tmp = tmp->next;
	}
    }

    /*
     * OK, it's a new one. Add it.
     */
    xf86Msg(X_INFO, "Loading %s\n", module);
    if (wasLoaded)
	*wasLoaded = 0;

    /*
     * Find a free handle.
     */
    new_handle = 1;
    while (freeHandles[new_handle] && new_handle < MAX_HANDLE)
	new_handle++;

    if (new_handle == MAX_HANDLE) {
	xf86Msg(X_ERROR, "Out of loader space\n");	/* XXX */
	if (errmaj)
	    *errmaj = LDR_NOSPACE;
	if (errmin)
	    *errmin = LDR_NOSPACE;
	return -1;
    }

    freeHandles[new_handle] = HANDLE_USED;
    refCount[new_handle] = 1;

    if ((fd = open(module, O_RDONLY)) < 0) {
	xf86Msg(X_ERROR, "Unable to open %s\n", module);
	freeHandles[new_handle] = HANDLE_FREE;
	if (errmaj)
	    *errmaj = LDR_NOMODOPEN;
	if (errmin)
	    *errmin = errno;
	return -1;
    }

    if ((modtype = _GetModuleType(fd, 0)) < 0) {
	xf86Msg(X_ERROR, "%s is an unrecognized module type\n", module);
	freeHandles[new_handle] = HANDLE_FREE;
	if (errmaj)
	    *errmaj = LDR_UNKTYPE;
	if (errmin)
	    *errmin = LDR_UNKTYPE;
	return -1;
    }

    tmp = _LoaderListPush();
    tmp->name = xf86loadermalloc(strlen(module) + 1);
    strcpy(tmp->name, module);
    tmp->cname = xf86loadermalloc(strlen(cname) + 1);
    strcpy(tmp->cname, cname);
    tmp->handle = new_handle;
    tmp->module = moduleseq++;
    tmp->funcs = &funcs[modtype];

    if ((tmp->private = funcs[modtype].LoadModule(tmp, fd, &pLookup, flags)) == NULL) {
	xf86Msg(X_ERROR, "Failed to load %s\n", module);
	_LoaderListPop(new_handle);
	freeHandles[new_handle] = HANDLE_FREE;
	if (errmaj)
	    *errmaj = LDR_NOLOAD;
	if (errmin)
	    *errmin = LDR_NOLOAD;
	return -1;
    }

    if (tmp->private != (void *)-1L) {
	LoaderAddSymbols(new_handle, tmp->module, pLookup);
	xf86loaderfree(pLookup);
    }

    close(fd);

    return new_handle;
}

int
LoaderHandleOpen(int handle)
{
    if (handle < 0 || handle >= MAX_HANDLE)
	return -1;

    if (freeHandles[handle] != HANDLE_USED)
	return -1;

    refCount[handle]++;
    return handle;
}

_X_EXPORT void *
LoaderSymbol(const char *sym)
{
    int i;
    itemPtr item = NULL;

    for (i = 0; i < numloaders; i++)
	funcs[i].ResolveSymbols(&funcs[i]);

    item = (itemPtr) LoaderHashFind(sym);

    if (item)
	return item->address;
    else
	return (DLFindSymbol(sym));
}

int
LoaderResolveSymbols(void)
{
    int i;

    for (i = 0; i < numloaders; i++)
	funcs[i].ResolveSymbols(&funcs[i]);
    return 0;
}

_X_EXPORT int
LoaderCheckUnresolved(int delay_flag)
{
    int i, ret = 0;
    LoaderResolveOptions delayFlag = (LoaderResolveOptions)delay_flag;

    LoaderResolveSymbols();

    if (delayFlag == LD_RESOLV_NOW) {
	if (check_unresolved_sema > 0)
	    check_unresolved_sema--;
	else
	    xf86Msg(X_WARNING, "LoaderCheckUnresolved: not enough "
		    "MAGIC_DONT_CHECK_UNRESOLVED\n");
    }

    if (!check_unresolved_sema || delayFlag == LD_RESOLV_FORCE)
	for (i = 0; i < numloaders; i++)
	    if (funcs[i].CheckForUnresolved(&funcs[i]))
		ret = 1;

    if (fatalReqSym)
	FatalError("Some required symbols were unresolved\n");

    return ret;
}

void xf86LoaderTrap(void);

void
xf86LoaderTrap(void)
{
}

_X_EXPORT void
LoaderDefaultFunc(void)
{
    ErrorF("\n\n\tThis should not happen!\n"
	   "\tAn unresolved function was called!\n");

    xf86LoaderTrap();

    FatalError("\n");
}

int
LoaderUnload(int handle)
{
    loaderRec fakeHead;
    loaderPtr tmp = &fakeHead;

    if (handle < 0 || handle >= MAX_HANDLE)
	return -1;

    /*
     * check the reference count, only free it if it goes to zero
     */
    if (--refCount[handle])
	return 0;
    /*
     * find the loaderRecs associated with this handle.
     */

    while ((tmp = _LoaderListPop(handle)) != NULL) {
	if (strchr(tmp->name, ':') == NULL) {
	    /* It is not a member of an archive */
	    xf86Msg(X_INFO, "Unloading %s\n", tmp->name);
	}
	tmp->funcs->LoaderUnload(tmp->private);
	xf86loaderfree(tmp->name);
	xf86loaderfree(tmp->cname);
	xf86loaderfree(tmp);
    }

    freeHandles[handle] = HANDLE_FREE;

    return 0;
}

void
LoaderDuplicateSymbol(const char *symbol, const int handle)
{
    ErrorF("Duplicate symbol %s in %s\n", symbol,
	   listHead ? listHead->name : "(built-in)");
    ErrorF("Also defined in %s\n", _LoaderHandleToName(handle));
    FatalError("Module load failure\n");
}

/* GDB Sync function */
void
_loader_debug_state()
{
}

unsigned long LoaderOptions = 0;

void
LoaderResetOptions(void)
{
    LoaderOptions = 0;
}

void
LoaderSetOptions(unsigned long opts)
{
    LoaderOptions |= opts;
}

void
LoaderClearOptions(unsigned long opts)
{
    LoaderOptions &= ~opts;
}

_X_EXPORT int
LoaderGetABIVersion(const char *abiclass)
{
    struct {
        const char *name;
        int version;
    } classes[] = {
        { ABI_CLASS_ANSIC,     LoaderVersionInfo.ansicVersion },
        { ABI_CLASS_VIDEODRV,  LoaderVersionInfo.videodrvVersion },
        { ABI_CLASS_XINPUT,    LoaderVersionInfo.xinputVersion },
        { ABI_CLASS_EXTENSION, LoaderVersionInfo.extensionVersion },
        { ABI_CLASS_FONT,      LoaderVersionInfo.fontVersion },
        { NULL,                0 }
    };
    int i;

    for(i = 0; classes[i].name; i++) {
        if(!strcmp(classes[i].name, abiclass)) {
            return classes[i].version;
        }
    }

    return 0;
}
