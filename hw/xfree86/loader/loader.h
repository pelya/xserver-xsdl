/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loader.h,v 1.25 2001/02/22 23:17:09 dawes Exp $ */

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
#ifndef _LOADER_H
#define _LOADER_H

#include "sym.h"

#if defined(Lynx) && defined(sun)
#define const /**/
#endif

#if (defined(__i386__) || defined(__ix86)) && !defined(i386)
#define i386
#endif

#include <X11/Xosdefs.h>
#include <X11/Xfuncproto.h>
#include <X11/Xmd.h>

/* For LOOKUP definition */
#include "sym.h"

#define LD_UNKNOWN	-1
#define LD_ARCHIVE	0
#define LD_ELFOBJECT	1
#define LD_COFFOBJECT	2
#define LD_XCOFFOBJECT	3
#define LD_AOUTOBJECT   4
#define LD_AOUTDLOBJECT	5
#define LD_ELFDLOBJECT	6

#define LD_PROCESSED_ARCHIVE -1

/* #define UNINIT_SECTION */
#define HANDLE_IN_HASH_ENTRY

/*
 * COFF Section nmumbers
 */
#define N_TEXT       1
#define N_DATA       2
#define N_BSS        3
#define N_COMMENT    4

#define TestFree(a) if (a) { xfree (a); a = NULL; }

#define HASHDIV 10
#define HASHSIZE (1<<HASHDIV)

typedef struct _elf_reloc   *ELFRelocPtr;
typedef struct _elf_COMMON  *ELFCommonPtr;
typedef struct _coff_reloc  *COFFRelocPtr;
typedef struct _coff_COMMON *COFFCommonPtr;
typedef struct AOUT_RELOC   *AOUTRelocPtr;
typedef struct AOUT_COMMON  *AOUTCommonPtr;

typedef struct _LoaderReloc {
    int modtype;
    struct _LoaderReloc *next;
    COFFRelocPtr coff_reloc;
    ELFRelocPtr elf_reloc;
    AOUTRelocPtr aout_reloc;
} LoaderRelocRec, *LoaderRelocPtr;

typedef struct _loader_item *itemPtr;
typedef struct _loader_item {
	char	*name ;
	void	*address ;
	itemPtr	next ;
	int	handle ;
	int	module ;
	itemPtr	exports;
#if defined(__powerpc__)
	/*
	 * PowerPC file formats require special routines in some circumstances
	 * to assist in the linking process. See the specific loader for
	 * more details.
	 */
	union {
		unsigned short	plt[8];		/* ELF */
		unsigned short	glink[14];	/* XCOFF */
	} code ;
#endif
	} itemRec ;

/* The following structures provide an interface to GDB (note that GDB
   has copies of the definitions - if you change anything here make
   sure that the changes are also made to GDB */

typedef	struct {
	char          *name;       /* Name of this symbol */
	unsigned int  namelen;     /* Name of this module */
	void          *addr;	   /* Start address of the .text section */
} LDRCommon, *LDRCommonPtr;

typedef	struct x_LDRModuleRec{
	unsigned int  version;     /* Version of this struct */
	char          *name;       /* Name of this module */
	unsigned int  namelen;     /* Length of name */
	void          *text;	   /* Start address of the .text section */
	void          *data;	   /* Start address of the .data section */
	void          *rodata;	   /* Start address of the .rodata section */
	void          *bss;	   /* Start address of the .bss section */
        LDRCommonPtr  commons;     /* List of commmon symbols */
        int           commonslen;  /* Number of common symbols */
	struct x_LDRModuleRec *next; /* Next module record in chain */
} LDRModuleRec, *LDRModulePtr;

extern char		   DebuggerPresent;
extern LDRModulePtr	   ModList;
extern LDRCommonPtr	   ldrCommons;
extern int		   nCommons;

/*
 * The loader uses loader specific alloc/calloc/free functions that
 * are mapped to either to the regular Xserver functions, or in a couple
 * of special cases, mapped to the C library functions.
 */
#if !defined(PowerMAX_OS) && !(defined(linux) && (defined(__alpha__) || defined(__powerpc__) || defined(__ia64__))) && 0
#define xf86loadermalloc(size) xalloc(size)
#define xf86loaderrealloc(ptr,size) xrealloc(ptr,size)
#define xf86loadercalloc(num,size) xcalloc(num,size)
#define xf86loaderfree(ptr) xfree(ptr)
#define xf86loaderstrdup(ptr) xstrdup(ptr)
#else
/*
 * On Some OSes, xalloc() et al uses mmap to allocate space for large
 * allocation. This has the effect of placing the text section of some
 * modules very far away from the rest which are placed on the heap.
 * Certain relocations are limited in the size of the offsets that can be
 * handled, and this seperation causes these relocation to overflow. This
 * is fixed by just using the C library allocation functions for the loader
 * to ensure that all text sections are located on the heap. OSes that have
 * this problem are:
 *	PowerMAX_OS/PPC
 * 	Linux/Alpha
 * 	Linux/PPC
 *	Linux/IA-64
 */
#define xf86loadermalloc(size) malloc(size)
#define xf86loaderrealloc(ptr,size) realloc(ptr,size)
#define xf86loadercalloc(num,size) calloc(num,size)
#define xf86loaderfree(ptr) free(ptr)
#define xf86loaderstrdup(ptr) strdup(ptr)
#endif

typedef struct _loader *loaderPtr;

/*
 * _loader_funcs hold the entry points for a module format.
 */

typedef void * (*LoadModuleProcPtr)(loaderPtr modrec, int fd, LOOKUP **);
typedef void (*ResolveSymbolsProcPtr)(void *);
typedef int (*CheckForUnresolvedProcPtr)(void *);
typedef char * (*AddressToSectionProcPtr)(void *, unsigned long);
typedef void (*LoaderUnloadProcPtr)(void *);

typedef struct _loader_funcs {
	LoadModuleProcPtr	LoadModule;
	ResolveSymbolsProcPtr	ResolveSymbols;
	CheckForUnresolvedProcPtr CheckForUnresolved;
	AddressToSectionProcPtr AddressToSection;
	LoaderUnloadProcPtr	LoaderUnload;
	LoaderRelocRec		pRelocs; /* type specific relocations */
} loader_funcs;

/* Each module loaded has a loaderRec */
typedef struct _loader {
	int	handle;		/* Unique id used to remove symbols from
				   this module when it is unloaded */
	int	module;		/* Unique id to identify compilation units */
	char	*name;
	char	*cname;
	void	*private;	/* format specific data */
	loader_funcs	*funcs;	/* funcs for operating on this module */
	loaderPtr next;
} loaderRec;

/* Compiled-in version information */
typedef struct {
	INT32	xf86Version;
	INT32	ansicVersion;
	INT32	videodrvVersion;
	INT32	xinputVersion;
	INT32	extensionVersion;
	INT32	fontVersion;
} ModuleVersions;
extern ModuleVersions LoaderVersionInfo;

extern unsigned long LoaderOptions;

/* Internal Functions */

void LoaderAddSymbols(int, int, LOOKUP *);
void LoaderDefaultFunc(void);
void LoaderDuplicateSymbol(const char *, const int);
#if 0
void LoaderFixups(void);
#endif
void LoaderResolve(void);
int LoaderResolveSymbols(void);
int _LoaderHandleUnresolved(char *, char *);
void LoaderHashAdd(itemPtr);
itemPtr LoaderHashDelete(const char *);
itemPtr LoaderHashFind(const char *);
void LoaderHashTraverse(void *, int (*)(void *, itemPtr));
void LoaderPrintAddress(const char *);
void LoaderPrintItem(itemPtr);
void LoaderPrintSymbol(unsigned long);
void LoaderDumpSymbols(void);
char *_LoaderModuleToName(int);
int _LoaderAddressToSection(const unsigned long, const char **, const char **);
int LoaderOpen(const char *, const char *, int, int *, int *, int *);
int LoaderHandleOpen(int);

/*
 * File interface functions
 */
void *_LoaderFileToMem(int fd, unsigned long offset, int size, char *label);
void _LoaderFreeFileMem(void *addr, int size);
int _LoaderFileRead(int fd, unsigned int offset, void *addr, int size);

/*
 * Relocation list manipulation routines
 */
LoaderRelocPtr _LoaderGetRelocations(void *);

/*
 * object to name lookup routines
 */
char * _LoaderHandleToName(int handle);
char * _LoaderHandleToCanonicalName(int handle);

/*
 * Entry points for the different loader types
 */
#include "aoutloader.h"
#include "coffloader.h"
#include "elfloader.h"
#include "dlloader.h"
/* LD_ARCHIVE */
void *ARCHIVELoadModule(loaderPtr, int, LOOKUP **);

extern void _loader_debug_state(void);

#endif /* _LOADER_H */
