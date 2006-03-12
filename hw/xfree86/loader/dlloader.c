/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/dlloader.c,v 1.13 2003/10/15 16:29:02 dawes Exp $ */

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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <X11/Xos.h>
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
    int flags;
} DLModuleRec, *DLModulePtr;

/* 
 * a list of loaded modules XXX can be improved
 */
typedef struct DLModuleList {
    DLModulePtr module;
    struct DLModuleList *next;
} DLModuleList;

DLModuleList *dlModuleList = NULL;

void *
DLFindSymbolLocal(pointer module, const char *name)
{
    DLModulePtr dlfile = module;
    void *p;
    char *n;

#ifdef NEED_UNDERSCORE_FOR_DLLSYM
    static const char symPrefix[] = "_";
#else
    static const char symPrefix[] = "";
#endif

    n = xf86loadermalloc(strlen(symPrefix) + strlen(name) + 1);
    sprintf(n, "%s%s", symPrefix, name);
    p = dlsym(dlfile->dlhandle, n);
    xf86loaderfree(n);

    return p;
}


/*
 * Search a symbol in the module list
 */
void *
DLFindSymbol(const char *name)
{
    DLModuleList *l;
    void *p;

    for (l = dlModuleList; l != NULL; l = l->next) {
        p = DLFindSymbolLocal(l->module, name);
	if (p)
	    return p;
    }

    return NULL;
}

/*
 * public interface
 */
void *
DLLoadModule(loaderPtr modrec, int fd, LOOKUP ** ppLookup, int flags)
{
    DLModulePtr dlfile;
    DLModuleList *l;
    int dlopen_flags;

    if ((dlfile = xf86loadercalloc(1, sizeof(DLModuleRec))) == NULL) {
	ErrorF("Unable  to allocate DLModuleRec\n");
	return NULL;
    }
    dlfile->handle = modrec->handle;
    if (flags & LD_FLAG_GLOBAL)
      dlopen_flags = DLOPEN_LAZY | DLOPEN_GLOBAL;
    else
      dlopen_flags = DLOPEN_LAZY;
    dlfile->dlhandle = dlopen(modrec->name, dlopen_flags);
    if (dlfile->dlhandle == NULL) {
	ErrorF("dlopen: %s\n", dlerror());
	xf86loaderfree(dlfile);
	return NULL;
    }

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
    DLModulePtr dlfile = (DLModulePtr) modptr;
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
