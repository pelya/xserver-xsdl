/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loaderProcs.h,v 1.19 2002/07/30 18:36:18 dawes Exp $ */

/*
 *
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

#ifndef _LOADERPROCS_H
#define _LOADERPROCS_H

#define IN_LOADER
#include "xf86Module.h"
#include "fontmod.h"

typedef struct module_desc {
	struct module_desc *child;
	struct module_desc *sib;
	struct module_desc *parent;
	struct module_desc *demand_next;
	char *name;
	char *filename;
	char *identifier;
	XID client_id;
	int in_use;
	int handle;
	ModuleSetupProc SetupProc;
	ModuleTearDownProc TearDownProc;
	void *TearDownData; /* returned from SetupProc */
	const char *path;
	const XF86ModuleVersionInfo *VersionInfo;
} ModuleDesc, *ModuleDescPtr;


/*
 * Extenal API for the loader 
 */

void LoaderInit(void);

ModuleDescPtr LoadDriver(const char *, const char *, int, pointer, int *,
			 int *);
ModuleDescPtr LoadModule(const char *, const char *, const char **,
			 const char **, pointer, const XF86ModReqInfo *,
			 int *, int *);
ModuleDescPtr LoadSubModule(ModuleDescPtr, const char *,
			    const char **, const char **, pointer,
			    const XF86ModReqInfo *, int *, int *);
ModuleDescPtr DuplicateModule(ModuleDescPtr mod, ModuleDescPtr parent);
void LoadFont (FontModule *);
void UnloadModule (ModuleDescPtr);
void UnloadSubModule (ModuleDescPtr);
void UnloadDriver (ModuleDescPtr);
void FreeModuleDesc (ModuleDescPtr mod);
ModuleDescPtr NewModuleDesc (const char *);
ModuleDescPtr AddSibling (ModuleDescPtr head, ModuleDescPtr new);
void LoaderSetPath(const char *path);
void LoaderSortExtensions(void);


void LoaderVReqSymLists(const char **, va_list args);
void LoaderVReqSymbols(const char *, va_list args);
void LoaderVRefSymLists(const char **, va_list args);
void LoaderVRefSymbols(const char *, va_list args);

void LoaderShowStack(void);
void *LoaderSymbolHandle(const char *, int);
int LoaderUnload(int);
unsigned long LoaderGetModuleVersion(ModuleDescPtr mod);

void LoaderResetOptions(void);
void LoaderSetOptions(unsigned long);
void LoaderClearOptions(unsigned long);

/* Options for LoaderSetOptions */
#define LDR_OPT_ABI_MISMATCH_NONFATAL		0x0001


#endif /* _LOADERPROCS_H */
