/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/dlloader.c,v 1.11 2000/08/23 22:10:14 tsi Exp $ */


/*
 *
 * Copyright (c) 1997 The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * XFree86 Project, Inc. not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The Xfree86 Project, Inc. makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE XFREE86 PROJECT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE XFREE86 PROJECT, INC. BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.  */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "Xos.h"
#include "os.h"

#include "sym.h"
#include "loader.h"
#include "dlloader.h"

#ifdef DL_LAZY
#define DLOPEN_LAZY DL_LAZY
#else
#ifdef RTLD_LAZY
#define DLOPEN_LAZY RTLD_LAZY
#else
#ifdef __FreeBSD__
#define DLOPEN_LAZY 1
#else
#define DLOPEN_LAZY 0
#endif
#endif
#endif
#ifdef LD_GLOBAL
#define DLOPEN_GLOBAL LD_GLOBAL
#else
#ifdef RTLD_GLOBAL
#define DLOPEN_GLOBAL RTLD_GLOBAL
#else
#define DLOPEN_GLOBAL 0
#endif
#endif

#define DLOPEN_FLAGS ( DLOPEN_LAZY | DLOPEN_GLOBAL )

#if defined(CSRG_BASED) && !defined(__ELF__)
#define NEED_UNDERSCORE_FOR_DLLSYM
#endif

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */
typedef struct {
    int handle;
    void *dlhandle;
} DLModuleRec, *DLModulePtr;

/* 
 * a list of loaded modules XXX can be improved
 */
typedef struct DLModuleList {
    DLModulePtr module;
    struct DLModuleList *next;
} DLModuleList;

DLModuleList *dlModuleList = NULL;

/*
 * Search a symbol in the module list
 */
void *
DLFindSymbol(const char *name)
{
    DLModuleList *l;
    void *p;

#ifdef NEED_UNDERSCORE_FOR_DLLSYM
    char *n;

    n = xf86loadermalloc(strlen(name) + 2);
    sprintf(n, "_%s", name);
#endif
    
    (void)dlerror();	/* Clear out any previous error */
    for (l = dlModuleList; l != NULL; l = l->next) {
#ifdef NEED_UNDERSCORE_FOR_DLLSYM
        p = dlsym(l->module->dlhandle, n);
#else
        p = dlsym(l->module->dlhandle, name);
#endif
	if (dlerror() == NULL) {
#ifdef NEED_UNDERSCORE_FOR_DLLSYM
	    xf86loaderfree(n);
#endif
	    return p;
	}
    }
#ifdef NEED_UNDERSCORE_FOR_DLLSYM    
    xf86loaderfree(n);
#endif
    
    return NULL;
}

/*
 * public interface
 */
void *
DLLoadModule(loaderPtr modrec, int fd, LOOKUP **ppLookup)
{
    DLModulePtr dlfile;
    DLModuleList *l;

    if ((dlfile=xf86loadercalloc(1,sizeof(DLModuleRec)))==NULL) {
	ErrorF("Unable  to allocate DLModuleRec\n");
	return NULL;
    }
    dlfile->handle = modrec->handle;
    dlfile->dlhandle = dlopen(modrec->name, DLOPEN_FLAGS);
    if (dlfile->dlhandle == NULL) {
	ErrorF("dlopen: %s\n", dlerror());
	xf86loaderfree(dlfile);
	return NULL;
    }
    /* Add it to the module list */
    l = xf86loadermalloc(sizeof(DLModuleList));
    l->module = dlfile;
    l->next = dlModuleList;
    dlModuleList = l;
    *ppLookup = NULL;

    return (void *)dlfile;
}

void
DLResolveSymbols(void *mod)
{
    return;
}

int
DLCheckForUnresolved(void *mod)
{
    return 0;
}

void
DLUnloadModule(void *modptr)
{
    DLModulePtr dlfile = (DLModulePtr)modptr;
    DLModuleList *l, *p;

    /*  remove it from dlModuleList */
    if (dlModuleList->module == modptr) {
	l = dlModuleList;
	dlModuleList = l->next;
	xf86loaderfree(l);
    } else {
	p = dlModuleList;
	for (l = dlModuleList->next; l != NULL; l = l->next) {
	    if (l->module == modptr) {
		p->next = l->next;
		xf86loaderfree(l);
		break;
	    }
	    p = l;
	}
    }
    dlclose(dlfile->dlhandle);
    xf86loaderfree(modptr);
}
