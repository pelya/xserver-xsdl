/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/aoutloader.c,v 1.20 2003/10/15 17:46:00 dawes Exp $ */

/*
 *
 * Copyright (c) 1997 Matthieu Herrb
 * Copyright 1995-1998 Metro Link, Inc.
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
 *
 * Modified 21/02/97 by Sebastien Marineau to support OS/2 a.out objects
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef __QNX__
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/stat.h>
#include <netinet/in.h>

#ifdef DBMALLOC
#include <debug/malloc.h>
#define Xalloc(size) malloc(size)
#define Xcalloc(size) calloc(1,(size))
#define Xfree(size) free(size)
#endif

#include "Xos.h"
#include "os.h"
#include "aout.h"

#include "sym.h"
#include "loader.h"
#include "aoutloader.h"

#ifndef LOADERDEBUG
#define LOADERDEBUG 0
#endif

#if LOADERDEBUG
#define AOUTDEBUG ErrorF
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */

typedef struct {
    int handle;
    int module;
    int fd;
    loader_funcs *funcs;
    AOUTHDR *header;		/* file header */
    unsigned char *text;	/* Start address of the text section */
    unsigned int textsize;	/* Size of the text section */
    unsigned char *data;	/* Start address of the data section */
    unsigned int datasize;	/* Size of the data section */
    unsigned char *bss;		/* Start address of the bss data */
    unsigned int bsssize;	/* Size of the bss section */
    struct relocation_info *txtrel;	/* Start address of the text relocation table */
    struct relocation_info *datarel;	/* Start address of the data relocation table */
    AOUT_nlist *symtab;		/* Start address of the symbol table */
    unsigned char *strings;	/* Start address of the string table */
    unsigned long strsize;	/* size of string table */
    unsigned char *common;	/* Start address of the common data */
    unsigned long comsize;	/* size of common data */
} AOUTModuleRec, *AOUTModulePtr;

/*
 * If an relocation is unable to be satisfied, then put it on a list
 * to try later after more modules have been loaded.
 */
typedef struct AOUT_RELOC {
    AOUTModulePtr file;
    struct relocation_info *rel;
    int type;			/* AOUT_TEXT or AOUT_DATA */
    struct AOUT_RELOC *next;
} AOUTRelocRec;

/*
 * Symbols with a section number of 0 (N_UNDF) but a value of non-zero
 * need to have space allocated for them.
 *
 * Gather all of these symbols together, and allocate one chunk when we
 * are done.
 */

typedef struct AOUT_COMMON {
    struct AOUT_nlist *sym;
    int index;
    struct AOUT_COMMON *next;
} AOUTCommonRec;

static AOUTCommonPtr listCOMMON = NULL;

/* prototypes for static functions */
static int AOUTHashCleanOut(void *, itemPtr);
static char *AOUTGetSymbolName(AOUTModulePtr, struct AOUT_nlist *);
static void *AOUTGetSymbolValue(AOUTModulePtr, int);
static AOUTCommonPtr AOUTAddCommon(struct AOUT_nlist *, int);
static LOOKUP *AOUTCreateCommon(AOUTModulePtr);
static LOOKUP *AOUT_GetSymbols(AOUTModulePtr);
static AOUTRelocPtr AOUTDelayRelocation(AOUTModulePtr, int,
					struct relocation_info_i386 *);
static AOUTRelocPtr AOUTCollectRelocations(AOUTModulePtr);
static void AOUT_Relocate(unsigned long *, unsigned long, int);
static AOUTRelocPtr AOUT_RelocateEntry(AOUTModulePtr, int,
				       struct relocation_info_i386 *);

/*
 * Return 1 if the symbol in item belongs to aoutfile
 */
static int
AOUTHashCleanOut(void *voidptr, itemPtr item)
{
    AOUTModulePtr aoutfile = (AOUTModulePtr) voidptr;

    return (aoutfile->handle == item->handle);
}

/*
 * Manage listResolv 
 */
static AOUTRelocPtr
AOUTDelayRelocation(AOUTModulePtr aoutfile, int type,
		    struct relocation_info *rel)
{
    AOUTRelocPtr reloc;

    if ((reloc = xf86loadermalloc(sizeof(AOUTRelocRec))) == NULL) {
	ErrorF("AOUTDelayRelocation() Unable to allocate memory\n");
	return NULL;
    }
    if ((unsigned long)rel < 0x200) {
	ErrorF("bug");
    }
    reloc->file = aoutfile;
    reloc->type = type;
    reloc->rel = rel;
    reloc->next = 0;
    return reloc;
}

/*
 * Manage listCOMMON
 */

static AOUTCommonPtr
AOUTAddCommon(struct AOUT_nlist *sym, int index)
{
    AOUTCommonPtr common;

    if ((common = xf86loadermalloc(sizeof(AOUTCommonRec))) == NULL) {
	ErrorF("AOUTAddCommon() Unable to allocate memory\n");
	return 0;
    }
    common->sym = sym;
    common->index = index;
    common->next = 0;
    return common;
}

static LOOKUP *
AOUTCreateCommon(AOUTModulePtr aoutfile)
{
    int numsyms = 0, size = 0, l = 0;
    int offset = 0;
    AOUTCommonPtr common;
    LOOKUP *lookup;

    if (listCOMMON == NULL)
	return NULL;

    common = listCOMMON;
    for (common = listCOMMON; common; common = common->next) {
	/* Ensure long word alignment */
	if ((common->sym->n_value & (sizeof(long) - 1)) != 0)
	    common->sym->n_value = (common->sym->n_value + (sizeof(long) - 1))
		    & ~(sizeof(long) - 1);

	/* accumulate the sizes */
	size += common->sym->n_value;
	numsyms++;
    }				/* while */

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTCreateCommon() %d entries (%d bytes) of COMMON data\n",
	      numsyms, size);
#endif

    if ((lookup = xf86loadermalloc((numsyms + 1) * sizeof(LOOKUP))) == NULL) {
	ErrorF("AOUTCreateCommon() Unable to allocate memory\n");
	return NULL;
    }

    aoutfile->comsize = size;
    if ((aoutfile->common = xf86loadercalloc(1, size)) == NULL) {
	ErrorF("AOUTCreateCommon() Unable to allocate memory\n");
	return NULL;
    }

    while (listCOMMON) {
	common = listCOMMON;
	lookup[l].symName = AOUTGetSymbolName(aoutfile, common->sym);
	lookup[l].offset = (funcptr) (aoutfile->common + offset);
#ifdef AOUTDEBUG
	AOUTDEBUG("Adding %p %s\n", (void *)lookup[l].offset,
		  lookup[l].symName);
#endif
	listCOMMON = common->next;
	offset += common->sym->n_value;
	xf86loaderfree(common);
	l++;
    }				/* while */
    /* listCOMMON == NULL */

    lookup[l].symName = NULL;	/* Terminate the list */
    return lookup;
}

/*
 * Symbol Table
 */

static char *
AOUTGetString(AOUTModulePtr aoutfile, int index)
{
    char *symname = (char *)&(aoutfile->strings[index]);

    if (symname[0] == '_') {
	symname++;
    }

    return symname;
}

/*
 * Return the name of a symbol 
 */
static char *
AOUTGetSymbolName(AOUTModulePtr aoutfile, struct AOUT_nlist *sym)
{
    char *symname = AOUTGetString(aoutfile, sym->n_un.n_strx);
    char *name;

    name = xf86loadermalloc(strlen(symname) + 1);
    if (!name)
	FatalError("AOUTGetSymbolName: Out of memory\n");

    strcpy(name, symname);

    return name;
}

/*
 * Return the value of a symbol in the loader's symbol table
 */
static void *
AOUTGetSymbolValue(AOUTModulePtr aoutfile, int index)
{
    void *symval = NULL;	/* value of the indicated symbol */
    itemPtr symbol = NULL;	/* name/value of symbol */
    char *name = NULL;

    name = AOUTGetSymbolName(aoutfile, aoutfile->symtab + index);

    if (name)
	symbol = LoaderHashFind(name);

    if (symbol)
	symval = (unsigned char *)symbol->address;

    xf86loaderfree(name);
    return symval;
}

/*
 * Perform the actual relocation 
 */
static void
AOUT_Relocate(unsigned long *destl, unsigned long val, int pcrel)
{
#ifdef AOUTDEBUG
    AOUTDEBUG("AOUT_Relocate %p : %08lx %s",
	      (void *)destl, *destl, pcrel == 1 ? "rel" : "abs");

#endif
    if (pcrel) {
	/* relative to PC */
	*destl = val - ((unsigned long)destl + sizeof(long));
    } else {
	*destl += val;
    }
#ifdef AOUTDEBUG
    AOUTDEBUG(" -> %08lx\n", *destl);
#endif
}

/*
 * Fix the relocation for text or data section
 */
static AOUTRelocPtr
AOUT_RelocateEntry(AOUTModulePtr aoutfile, int type,
		   struct relocation_info *rel)
{
    AOUTHDR *header = aoutfile->header;
    AOUT_nlist *symtab = aoutfile->symtab;
    int symnum;
    void *symval;
    unsigned long *destl;	/* address of the location to be modified */

    symnum = rel->r_symbolnum;
#ifdef AOUTDEBUG
    {
	char *name;

	if (rel->r_extern) {
	    AOUTDEBUG("AOUT_RelocateEntry: extern %s\n",
		      name = AOUTGetSymbolName(aoutfile, symtab + symnum));
	    xf86loaderfree(name);
	} else {
	    AOUTDEBUG("AOUT_RelocateEntry: intern\n");
	}
	AOUTDEBUG("  pcrel: %d", rel->r_pcrel);
	AOUTDEBUG("  length: %d", rel->r_length);
	AOUTDEBUG("  baserel: %d", rel->r_baserel);
	AOUTDEBUG("  jmptable: %d", rel->r_jmptable);
	AOUTDEBUG("  relative: %d", rel->r_relative);
	AOUTDEBUG("  copy: %d\n", rel->r_copy);
    }
#endif /* AOUTDEBUG */

    if (rel->r_length != 2) {
	ErrorF("AOUT_ReloateEntry: length != 2\n");
    }
    /* 
     * First find the address to modify
     */
    switch (type) {
    case AOUT_TEXT:
	/* Check that the relocation offset is in the text segment */
	if (rel->r_address > header->a_text) {
	    ErrorF("AOUT_RelocateEntry(): "
		   "text relocation out of text section\n");
	}
	destl = (unsigned long *)(aoutfile->text + rel->r_address);
	break;
    case AOUT_DATA:
	/* Check that the relocation offset is in the data segment */
	if (rel->r_address > header->a_data) {
	    ErrorF("AOUT_RelocateEntry():"
		   "data relocation out of data section\n");
	}
	destl = (unsigned long *)(aoutfile->data + rel->r_address);
	break;
    default:
	ErrorF("AOUT_RelocateEntry(): unknown section type %d\n", type);
	return 0;
    }				/* switch */

    /*
     * Now handle the relocation 
     */
    if (rel->r_extern) {
	/* Lookup the symbol in the loader's symbol table */
	symval = AOUTGetSymbolValue(aoutfile, symnum);
	if (symval != 0) {
	    /* we've got the value */
	    AOUT_Relocate(destl, (unsigned long)symval, rel->r_pcrel);
	    return 0;
	} else {
	    /* The symbol should be undefined */
	    switch (symtab[symnum].n_type & AOUT_TYPE) {
	    case AOUT_UNDF:
#ifdef AOUTDEBUG
		AOUTDEBUG("  extern AOUT_UNDEF\n");
#endif
		/* Add this relocation back to the global list */
		return AOUTDelayRelocation(aoutfile, type, rel);

	    default:
		ErrorF("AOUT_RelocateEntry():"
		       " impossible intern relocation type: %d\n",
		       symtab[symnum].n_type);
		return 0;
	    }			/* switch */
	}
    } else {
	/* intern */
	switch (rel->r_symbolnum) {
	case AOUT_TEXT:
#ifdef AOUTDEBUG
	    AOUTDEBUG("  AOUT_TEXT\n");
#endif
	    /* Only absolute intern text relocations need to be handled */
	    if (rel->r_pcrel == 0)
		AOUT_Relocate(destl, (unsigned long)aoutfile->text,
			      rel->r_pcrel);
	    return 0;
	case AOUT_DATA:
#ifdef AOUTDEBUG
	    AOUTDEBUG("  AOUT_DATA\n");
#endif
	    if (rel->r_pcrel == 0)
		AOUT_Relocate(destl, (unsigned long)aoutfile->data
			      - header->a_text, rel->r_pcrel);
	    else
		ErrorF("AOUT_RelocateEntry(): "
		       "don't know how to handle data pc-relative reloc\n");

	    return 0;
	case AOUT_BSS:
#ifdef AOUTDEBUG
	    AOUTDEBUG("  AOUT_BSS\n");
#endif
	    if (rel->r_pcrel == 0)
		AOUT_Relocate(destl, (unsigned long)aoutfile->bss
			      - header->a_text - header->a_data,
			      rel->r_pcrel);
	    else
		ErrorF("AOUT_RelocateEntry(): "
		       "don't know how to handle bss pc-relative reloc\n");

	    return 0;
	default:
	    ErrorF("AOUT_RelocateEntry():"
		   " unknown intern relocation type: %d\n", rel->r_symbolnum);
	    return 0;
	}			/* switch */
    }
}				/* AOUT_RelocateEntry */

static AOUTRelocPtr
AOUTCollectRelocations(AOUTModulePtr aoutfile)
{
    AOUTHDR *header = aoutfile->header;
    int i, nreloc;
    struct relocation_info *rel;
    AOUTRelocPtr reloc_head = NULL;
    AOUTRelocPtr tmp;

    /* Text relocations */
    if (aoutfile->text != NULL && aoutfile->txtrel != NULL) {
	nreloc = header->a_trsize / sizeof(struct relocation_info);

	for (i = 0; i < nreloc; i++) {
	    rel = aoutfile->txtrel + i;
	    tmp = AOUTDelayRelocation(aoutfile, AOUT_TEXT, rel);
	    if (tmp) {
		tmp->next = reloc_head;
		reloc_head = tmp;
	    }
	}			/* for */
    }
    /* Data relocations */
    if (aoutfile->data != NULL && aoutfile->datarel != NULL) {
	nreloc = header->a_drsize / sizeof(struct relocation_info);

	for (i = 0; i < nreloc; i++) {
	    rel = aoutfile->datarel + i;
	    tmp = AOUTDelayRelocation(aoutfile, AOUT_DATA, rel);
	    tmp->next = reloc_head;
	    reloc_head = tmp;
	}			/* for */
    }
    return reloc_head;
}				/* AOUTCollectRelocations */

/*
 * AOUT_GetSymbols()
 * 
 * add the symbols to the loader's symbol table
 */
static LOOKUP *
AOUT_GetSymbols(AOUTModulePtr aoutfile)
{
    int fd = aoutfile->fd;
    AOUTHDR *header = aoutfile->header;
    int nsyms, soff, i, l;
    char *symname;
    AOUT_nlist *s;
    LOOKUP *lookup, *lookup_common;
    AOUTCommonPtr tmp;

    aoutfile->symtab = (AOUT_nlist *) _LoaderFileToMem(fd,
						       AOUT_SYMOFF(header),
						       header->a_syms,
						       "symbols");
    nsyms = header->a_syms / sizeof(AOUT_nlist);
    lookup = xf86loadermalloc(nsyms * sizeof(LOOKUP));
    if (lookup == NULL) {
	ErrorF("AOUT_GetSymbols(): can't allocate memory\n");
	return NULL;
    }
    for (i = 0, l = 0; i < nsyms; i++) {
	s = aoutfile->symtab + i;
	soff = s->n_un.n_strx;
	if (soff == 0 || (s->n_type & AOUT_STAB) != 0)
	    continue;
	symname = AOUTGetSymbolName(aoutfile, s);
#ifdef AOUTDEBUG
	AOUTDEBUG("AOUT_GetSymbols(): %s %02x %02x %08lx\n",
		  symname, s->n_type, s->n_other, s->n_value);
#endif
	switch (s->n_type & AOUT_TYPE) {
	case AOUT_UNDF:
	    if (s->n_value != 0) {
		if (!LoaderHashFind(symname)) {
#ifdef AOUTDEBUG
		    AOUTDEBUG("Adding common %s\n", symname);
#endif
		    tmp = AOUTAddCommon(s, i);
		    if (tmp) {
			tmp->next = listCOMMON;
			listCOMMON = tmp;
		    }
		}
	    } else {
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding undef %s\n", symname);
#endif
	    }
	    xf86loaderfree(symname);
	    break;
	case AOUT_TEXT:
	    if (s->n_type & AOUT_EXT) {
		lookup[l].symName = symname;
		/* text symbols start at 0 */
		lookup[l].offset = (funcptr) (aoutfile->text + s->n_value);
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding text %s %p\n", symname,
			  (void *)lookup[l].offset);
#endif
		l++;
	    } else {
		xf86loaderfree(symname);
	    }
	    break;
	case AOUT_DATA:
	    if (s->n_type & AOUT_EXT) {
		lookup[l].symName = symname;
		/* data symbols are following text */
		lookup[l].offset = (funcptr) (aoutfile->data +
					      s->n_value - header->a_text);
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding data %s %p\n", symname,
			  (void *)lookup[l].offset);
#endif
		l++;
	    } else {
		xf86loaderfree(symname);
	    }
	    break;
	case AOUT_BSS:
	    if (s->n_type & AOUT_EXT) {
		lookup[l].symName = symname;
		/* bss symbols follow both text and data */
		lookup[l].offset = (funcptr) (aoutfile->bss + s->n_value
					      - (header->a_data
						 + header->a_text));
#ifdef AOUTDEBUG
		AOUTDEBUG("Adding bss %s %p\n", symname,
			  (void *)lookup[l].offset);
#endif
		l++;
	    } else {
		xf86loaderfree(symname);
	    }
	    break;
	case AOUT_FN:
#ifdef AOUTDEBUG
	    if (s->n_type & AOUT_EXT) {
		AOUTDEBUG("Ignoring AOUT_FN %s\n", symname);
	    } else {
		AOUTDEBUG("Ignoring AOUT_WARN %s\n", symname);
	    }
#endif
	    xf86loaderfree(symname);
	    break;
	default:
	    ErrorF("Unknown symbol type %x\n", s->n_type & AOUT_TYPE);
	    xf86loaderfree(symname);
	}			/* switch */
    }				/* for */
    lookup[l].symName = NULL;

    lookup_common = AOUTCreateCommon(aoutfile);
    if (lookup_common) {
	LOOKUP *p;

	for (i = 0, p = lookup_common; p->symName; i++, p++) ;
	memcpy(&(lookup[l]), lookup_common, i * sizeof(LOOKUP));

	xf86loaderfree(lookup_common);
	l += i;
	lookup[l].symName = NULL;
    }
    return lookup;
}				/* AOUT_GetSymbols */

/*
 * Public API for the a.out implementation of the loader
 */
void *
AOUTLoadModule(loaderPtr modrec, int aoutfd, LOOKUP ** ppLookup)
{
    AOUTModulePtr aoutfile = NULL;
    AOUTHDR *header;
    AOUTRelocPtr reloc, tail;
    void *v;

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTLoadModule(%s, %d, %d)\n",
	      modrec->name, modrec->handle, aoutfd);
#endif
    if ((aoutfile = xf86loadercalloc(1, sizeof(AOUTModuleRec))) == NULL) {
	ErrorF("Unable to allocate AOUTModuleRec\n");
	return NULL;
    }

    aoutfile->handle = modrec->handle;
    aoutfile->module = modrec->module;
    aoutfile->fd = aoutfd;
    v = aoutfile->funcs = modrec->funcs;

    /*
     *  Get the a.out header
     */
    aoutfile->header =
	    (AOUTHDR *) _LoaderFileToMem(aoutfd, 0, sizeof(AOUTHDR),
					 "header");
    header = (AOUTHDR *) aoutfile->header;

    /* 
     * Load the 6 other sections 
     */
    /* text */
    if (header->a_text != 0) {
	aoutfile->text = _LoaderFileToMem(aoutfile->fd,
					  AOUT_TXTOFF(header),
					  header->a_text, "text");
	aoutfile->textsize = header->a_text;
    } else {
	aoutfile->text = NULL;
    }
    /* data */
    if (header->a_data != 0) {
	aoutfile->data = _LoaderFileToMem(aoutfile->fd,
					  AOUT_DATOFF(header),
					  header->a_data, "data");
	aoutfile->datasize = header->a_data;
    } else {
	aoutfile->data = NULL;
    }
    /* bss */
    if (header->a_bss != 0) {
	aoutfile->bss = xf86loadercalloc(1, header->a_bss);
	aoutfile->bsssize = header->a_bss;
    } else {
	aoutfile->bss = NULL;
    }
    /* Text Relocations */
    if (header->a_trsize != 0) {
	aoutfile->txtrel = _LoaderFileToMem(aoutfile->fd,
					    AOUT_TRELOFF(header),
					    header->a_trsize, "txtrel");
    } else {
	aoutfile->txtrel = NULL;
    }
    /* Data Relocations */
    if (header->a_drsize != 0) {
	aoutfile->datarel = _LoaderFileToMem(aoutfile->fd,
					     AOUT_DRELOFF(header),
					     header->a_drsize, "datarel");
    } else {
	aoutfile->datarel = NULL;
    }
    /* String table */
    _LoaderFileRead(aoutfile->fd, AOUT_STROFF(header),
		    &(aoutfile->strsize), sizeof(int));
    if (aoutfile->strsize != 0) {
	aoutfile->strings = _LoaderFileToMem(aoutfile->fd,
					     AOUT_STROFF(header),
					     aoutfile->strsize, "strings");
    } else {
	aoutfile->strings = NULL;
    }
    /* load symbol table */
    *ppLookup = AOUT_GetSymbols(aoutfile);

    /* Do relocations */
    reloc = AOUTCollectRelocations(aoutfile);

    if (reloc) {
	for (tail = reloc; tail->next; tail = tail->next) ;
	tail->next = _LoaderGetRelocations(v)->aout_reloc;
	_LoaderGetRelocations(v)->aout_reloc = reloc;
    }

    return (void *)aoutfile;
}

void
AOUTResolveSymbols(void *mod)
{
    AOUTRelocPtr newlist, p, tmp;

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTResolveSymbols()\n");
#endif

    newlist = 0;
    for (p = _LoaderGetRelocations(mod)->aout_reloc; p;) {
	tmp = AOUT_RelocateEntry(p->file, p->type, p->rel);
	if (tmp) {
	    /* Failed to relocate.  Keep it in the list. */
	    tmp->next = newlist;
	    newlist = tmp;
	}
	tmp = p;
	p = p->next;
	xf86loaderfree(tmp);
    }
    _LoaderGetRelocations(mod)->aout_reloc = newlist;
}				/* AOUTResolveSymbols */

int
AOUTCheckForUnresolved(void *mod)
{
    int symnum;
    AOUTRelocPtr crel;
    char *name;
    int fatalsym = 0, flag;

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTCheckForUnResolved()\n");
#endif
    if ((crel = _LoaderGetRelocations(mod)->aout_reloc) == NULL)
	return 0;

    while (crel) {
	if (crel->type == AOUT_TEXT) {
	    /* Attempt to make unresolved  text references 
	     * point to a default function */
	    AOUT_Relocate((unsigned long *)(crel->file->text
					    + crel->rel->r_address),
			  (unsigned long)LoaderDefaultFunc,
			  crel->rel->r_pcrel);
	}
	symnum = crel->rel->r_symbolnum;
	name = AOUTGetSymbolName(crel->file, crel->file->symtab + symnum);
	flag = _LoaderHandleUnresolved(name,
				       _LoaderHandleToName(crel->file->
							   handle));
	xf86loaderfree(name);
	if (flag)
	    fatalsym = 1;
	crel = crel->next;
    }
    return fatalsym;
}

void
AOUTUnloadModule(void *modptr)
{
    AOUTModulePtr aoutfile = (AOUTModulePtr) modptr;
    AOUTRelocPtr relptr, *prevptr;

#ifdef AOUTDEBUG
    AOUTDEBUG("AOUTUnLoadModule(0x%p)\n", modptr);
#endif

/*
 * Delete any unresolved relocations
 */

    relptr = _LoaderGetRelocations(aoutfile->funcs)->aout_reloc;
    prevptr = &(_LoaderGetRelocations(aoutfile->funcs)->aout_reloc);

    while (relptr) {
	if (relptr->file == aoutfile) {
	    *prevptr = relptr->next;
	    xf86loaderfree(relptr);
	    relptr = *prevptr;
	} else {
	    prevptr = &(relptr->next);
	    relptr = relptr->next;
	}
    }				/* while */

    /* clean the symbols table */
    LoaderHashTraverse((void *)aoutfile, AOUTHashCleanOut);

#define CheckandFree(ptr,size)  if(ptr) _LoaderFreeFileMem((ptr),(size))

    CheckandFree(aoutfile->strings, aoutfile->strsize);
    CheckandFree(aoutfile->symtab, aoutfile->header->a_syms);
    CheckandFree(aoutfile->datarel, aoutfile->header->a_drsize);
    CheckandFree(aoutfile->txtrel, aoutfile->header->a_trsize);
    CheckandFree(aoutfile->data, aoutfile->header->a_data);
    CheckandFree(aoutfile->text, aoutfile->header->a_text);
    /* Free allocated sections */
    if (aoutfile->bss != NULL) {
	xf86loaderfree(aoutfile->bss);
    }
    if (aoutfile->common != NULL) {
	xf86loaderfree(aoutfile->common);
    }

    /* Free header */
    _LoaderFreeFileMem(aoutfile->header, sizeof(AOUTHDR));

    /* Free the module structure itself */
    xf86loaderfree(aoutfile);

    return;
}

char *
AOUTAddressToSection(void *modptr, unsigned long address)
{
    AOUTModulePtr aoutfile = (AOUTModulePtr) modptr;

    if (address >= (unsigned long)aoutfile->text &&
	address <= (unsigned long)aoutfile->text + aoutfile->textsize) {
	return "text";
    }
    if (address >= (unsigned long)aoutfile->data &&
	address <= (unsigned long)aoutfile->data + aoutfile->datasize) {
	return "data";
    }
    if (address >= (unsigned long)aoutfile->bss &&
	address <= (unsigned long)aoutfile->bss + aoutfile->bsssize) {
	return "bss";
    }

    return NULL;
}
