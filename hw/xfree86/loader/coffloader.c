/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/coffloader.c,v 1.18 2002/09/16 18:06:10 eich Exp $ */

/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef __QNX__
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif
#include <sys/stat.h>

#ifdef DBMALLOC
#include <debug/malloc.h>
#define Xalloc(size) malloc(size)
#define Xcalloc(size) calloc(1,(size))
#define Xfree(size) free(size)
#endif

#include "Xos.h"
#include "os.h"
#include "coff.h"

#include "sym.h"
#include "loader.h"
#include "coffloader.h"

#include "compiler.h"
/*
#ifndef LDTEST
#define COFFDEBUG ErrorF
#endif
*/

/*
 * This structure contains all of the information about a module
 * that has been loaded.
 */

typedef	struct {
	int	handle;
	long	module;		/* Id of the module used to find inter module calls */
	int	fd;
	loader_funcs	*funcs;
	FILHDR 	*header;	/* file header */
	AOUTHDR  *optheader;	/* optional file header */
	unsigned short	numsh;
	SCNHDR	*sections;	/* Start address of the section table */
	int	secsize;	/* size of the section table */
	unsigned char	**saddr;/* Start addresss of the sections table */
	unsigned char	**reladdr;/* Start addresss of the relocation table */
	unsigned char *strtab;	/* Start address of the string table */
	int	strsize;	/* size of the string table */
	unsigned char *text;	/* Start address of the .text section */
	int	txtndx;		/* index of the .text section */
	long	txtaddr;	/* offset of the .text section */
	int	txtsize;	/* size of the .text section */
	int	txtrelsize;	/* size of the .rel.text section */
	unsigned char *data;	/* Start address of the .data section */
	int	datndx;		/* index of the .data section */
	long	dataddr;	/* offset of the .data section */
	int	datsize;	/* size of the .data section */
	int	datrelsize;	/* size of the .rel.data section */
	unsigned char *bss;	/* Start address of the .bss section */
	int	bssndx;		/* index of the .bss section */
	long	bssaddr;	/* offset of the .bss section */
	int	bsssize;	/* size of the .bss section */
	SYMENT *symtab;		/* Start address of the .symtab section */
	int	symndx;		/* index of the .symtab section */
	int	symsize;	/* size of the .symtab section */
	unsigned char *common;	/* Start address of the .common section */
	int	comsize;	/* size of the .common section */
	long	toc;		/* Offset of the TOC csect */
	unsigned char *tocaddr;	/* Address of the TOC csect */
	}	COFFModuleRec, *COFFModulePtr;

/*
 * If any relocation is unable to be satisfied, then put it on a list
 * to try later after more modules have been loaded.
 */
typedef struct _coff_reloc {
	COFFModulePtr	file;
	RELOC		*rel;
	int		secndx;
	struct _coff_reloc	*next;
} COFFRelocRec;

/*
 * Symbols with a section number of 0 (N_UNDEF) but a value of non-zero
 * need to have space allocated for them.
 *
 * Gather all of these symbols together, and allocate one chunk when we
 * are done.
 */

typedef struct _coff_COMMON {
	SYMENT	*sym;
	int	index;
	struct _coff_COMMON	*next;
	} COFFCommonRec;

static COFFCommonPtr listCOMMON = NULL;

/* Prototypes for static functions */
static int COFFhashCleanOut(void *, itemPtr);
static char *COFFGetSymbolName(COFFModulePtr, int);
static COFFCommonPtr COFFAddCOMMON(SYMENT *, int);
static LOOKUP *COFFCreateCOMMON(COFFModulePtr);
static COFFRelocPtr COFFDelayRelocation(COFFModulePtr, int, RELOC *);
static SYMENT *COFFGetSymbol(COFFModulePtr, int);
static unsigned char *COFFGetSymbolValue(COFFModulePtr, int);
static COFFRelocPtr COFF_RelocateEntry(COFFModulePtr, int, RELOC *);
static LOOKUP *COFF_GetSymbols(COFFModulePtr);
static void COFFCollectSections(COFFModulePtr);
static COFFRelocPtr COFFCollectRelocations(COFFModulePtr);

/*
 * Utility Functions
 */


static int
COFFhashCleanOut(voidptr, item)
void *voidptr;
itemPtr item ;
{
  COFFModulePtr module = (COFFModulePtr) voidptr;
  return ( module->handle == item->handle ) ;
}


/*
 * Manage listResolv
 */
static COFFRelocPtr
COFFDelayRelocation(cofffile,secndx,rel)
COFFModulePtr	cofffile;
int		secndx;
RELOC		*rel;
{
    COFFRelocPtr reloc;

    if ((reloc = xf86loadermalloc(sizeof(COFFRelocRec))) == NULL) {
	ErrorF( "COFFDelayRelocation() Unable to allocate memory!!!!\n" );
	return 0;
    }

    reloc->file=cofffile;
    reloc->secndx=secndx;
    reloc->rel=rel;
    reloc->next = 0;

    return reloc;
}

/*
 * Manage listCOMMON
 */

static COFFCommonPtr
COFFAddCOMMON(sym,index)
SYMENT	*sym;
int index;
{
    COFFCommonPtr common;

    if ((common = xf86loadermalloc(sizeof(COFFCommonRec))) == NULL) {
	ErrorF( "COFFAddCOMMON() Unable to allocate memory!!!!\n" );
	return 0;
    }
    common->sym=sym;
    common->index=index;
    common->next=0;

    return common;
}

static LOOKUP *
COFFCreateCOMMON(cofffile)
COFFModulePtr	cofffile;
{
    int	numsyms=0,size=0,l=0;
    int	offset=0;
    LOOKUP	*lookup;
    COFFCommonPtr common;

    if (listCOMMON == NULL)
	return NULL;

    common=listCOMMON;
    for (common = listCOMMON; common; common = common->next) {
	/* Ensure long word alignment */
	if(   common->sym->n_value != 2
	   && common->sym->n_value != 1) /* But not for short and char ;-)(mr)*/
	if( common->sym->n_value%4 != 0 )
		common->sym->n_value+= 4-(common->sym->n_value%4);

	/* accumulate the sizes */
	size+=common->sym->n_value;
	numsyms++;
    }

#ifdef COFFDEBUG
    COFFDEBUG("COFFCreateCOMMON() %d entries (%d bytes) of COMMON data\n",
	       numsyms, size );
#endif

    if ((lookup = xf86loadermalloc((numsyms+1)*sizeof(LOOKUP))) == NULL) {
        ErrorF( "COFFCreateCOMMON() Unable to allocate memory!!!!\n" );
        return NULL;
    }

    cofffile->comsize=size;
    if ((cofffile->common = xf86loadercalloc(1,size)) == NULL) {
        ErrorF( "COFFCreateCOMMON() Unable to allocate memory!!!!\n" );
        return NULL;
    }

    /* Traverse the common list and create a lookup table with all the
     * common symbols.  Destroy the common list in the process.
     * See also ResolveSymbols.
     */
    while(listCOMMON) {
        common=listCOMMON;
        lookup[l].symName=COFFGetSymbolName(cofffile,common->index);
        lookup[l].offset=(funcptr)(cofffile->common+offset);
#ifdef COFFDEBUG
        COFFDEBUG("Adding %x %s\n", lookup[l].offset, lookup[l].symName );
#endif
        listCOMMON=common->next;
        offset+=common->sym->n_value;
        xf86loaderfree(common);
        l++;
    }
    /* listCOMMON == NULL */

    lookup[l].symName=NULL; /* Terminate the list */
    return lookup;
}

/*
 * Symbol Table
 */

/*
 * Get symbol name
 */
static char *
COFFGetSymbolName(cofffile, index)
COFFModulePtr	cofffile;
int		index;
{
    char	*name;
    SYMENT	*sym;

    sym=(SYMENT *)(((unsigned char *)cofffile->symtab)+(index*SYMESZ));

#ifdef COFFDEBUG
    COFFDEBUG("COFFGetSymbolName(%x,%x) %x",cofffile, index, sym->n_zeroes );
#endif

    name = xf86loadermalloc(sym->n_zeroes ? SYMNMLEN + 1
	   : strlen((const char *)&cofffile->strtab[(int)sym->n_offset-4]) + 1);
    if (!name)
	FatalError("COFFGetSymbolName: Out of memory\n");

    if( sym->n_zeroes )
	{
	    strncpy(name,sym->n_name,SYMNMLEN);
	    name[SYMNMLEN]='\000';
	}
    else {
	strcpy(name, (const char *)&cofffile->strtab[(int)sym->n_offset-4]);
        }
#ifdef COFFDEBUG
    COFFDEBUG(" %s\n", name );
#endif
    return name;
}

static SYMENT *
COFFGetSymbol(file, index)
COFFModulePtr	file;
int index;
{
    return (SYMENT *)(((unsigned char *)file->symtab)+(index*SYMESZ));
}

static unsigned char *
COFFGetSymbolValue(cofffile, index)
COFFModulePtr	cofffile;
int index;
{
    unsigned char *symval=0;	/* value of the indicated symbol */
    itemPtr symbol;		/* name/value of symbol */
    char	*symname;

    symname=COFFGetSymbolName(cofffile, index);

#ifdef COFFDEBUG
    COFFDEBUG("COFFGetSymbolValue() for %s=", symname );
#endif

    symbol = LoaderHashFind(symname);

    if( symbol )
	symval=(unsigned char *)symbol->address;

#ifdef COFFDEBUG
    COFFDEBUG("%x\n", symval );
#endif

    xf86loaderfree(symname);
    return symval;
}

#if defined(__powerpc__)
/*
 * This function returns the address of the glink routine for a symbol. This
 * address is used in cases where the function being called is not in the
 * same module as the calling function.
 */
static unsigned char *
COFFGetSymbolGlinkValue(cofffile, index)
COFFModulePtr	cofffile;
int index;
{
    unsigned char *symval=0;	/* value of the indicated symbol */
    itemPtr symbol;		/* name/value of symbol */
    char	*name;

    name=COFFGetSymbolName(cofffile, index);

#ifdef COFFDEBUG
    COFFDEBUG("COFFGetSymbolGlinkValue() for %s=", name );
#endif

    symbol = LoaderHashFind(name+1); /* Eat the '.' so we get the
					  Function descriptor instead */

/* Here we are building up a glink function that will change the TOC
 * pointer before calling a function that resides in a different module.
 * The following code is being used to implement this.

	1 00000000 3d80xxxx	lis   r12,hi16(funcdesc)
	2 00000004 618cxxxx	ori   r12,r12,lo16(funcdesc)
	3 00000008 90410014	st    r2,20(r1)	# save old TOC pointer
	4 0000000c 804c0000 	l     r2,0(r12)	# Get address of functions
	5 00000010 7c4903a6	mtctr  r2	# load destination address
	6 00000014 804c0004	l     r2,4(r12)	# get TOC of function
	7 00000018 4e800420	bctr		# branch to it

 */
    if( symbol ) {
	symval=(unsigned char *)&symbol->code.glink;
#ifdef COFFDEBUG
    COFFDEBUG("%x\n", symval );
    COFFDEBUG("glink_%s=%x\n", name,symval );
#endif
	symbol->code.glink[ 0]=0x3d80; /* lis r12 */
	symbol->code.glink[ 1]=((unsigned long)symbol->address&0xffff0000)>>16;
	symbol->code.glink[ 2]=0x618c; /* ori r12 */
	symbol->code.glink[ 3]=((unsigned long)symbol->address&0x0000ffff);
	symbol->code.glink[ 4]=0x9041; /* st r2,20(r1) */
	symbol->code.glink[ 5]=0x0014;
	symbol->code.glink[ 6]=0x804c; /* l r2,0(r12) */
	symbol->code.glink[ 7]=0x0000;
	symbol->code.glink[ 8]=0x7c49; /* mtctr r2 */
	symbol->code.glink[ 9]=0x03a6;
	symbol->code.glink[10]=0x804c; /* l r2,4(r12) */
	symbol->code.glink[11]=0x0004;
	symbol->code.glink[12]=0x4e80; /* bctr */
	symbol->code.glink[13]=0x0420;
	ppc_flush_icache(&symbol->code.glink[0]);
	ppc_flush_icache(&symbol->code.glink[12]);
    }

    xf86loaderfree(name);
    return symval;
}
#endif /* __powerpc__ */

/*
 * Fix all of the relocation for the given section.
 */
static COFFRelocPtr
COFF_RelocateEntry(cofffile, secndx, rel)
COFFModulePtr	cofffile;
int		secndx;	/* index of the target section */
RELOC		*rel;
{
    SYMENT	*symbol;	/* value of the indicated symbol */
    unsigned long *dest32;	/* address of the place being modified */
#if defined(__powerpc__)
    unsigned short *dest16;	/* address of the place being modified */
    itemPtr	symitem;	/* symbol structure from has table */
    char	*name;
#endif
    unsigned char *symval;	/* value of the indicated symbol */

/*
 * Note: Section numbers are 1 biased, while the cofffile->saddr[] array
 * of pointer is 0 biased, so alway have to account for the difference.
 */

/* 
 * Reminder: secndx is the section to which the relocation is applied.
 *           symbol->n_scnum is the section in which the symbol value resides.
 */

#ifdef COFFDEBUG
    COFFDEBUG("%x %d %o ",
	       rel->r_vaddr,rel->r_symndx,rel->r_type );
#if defined(__powerpc__)
    COFFDEBUG("[%x %x %x] ",
	       RELOC_RSIGN(*rel), RELOC_RFIXUP(*rel), RELOC_RLEN(*rel));
#endif
#endif
    symbol=COFFGetSymbol(cofffile,rel->r_symndx);
#ifdef COFFDEBUG
    COFFDEBUG("%d %x %d-%d\n", symbol->n_sclass, symbol->n_value, symbol->n_scnum, secndx );
#endif

/*
 * Check to see if the relocation offset is part of the .text segment.
 * If not, we must change the offset to be relative to the .data section
 * which is NOT contiguous.
 */ 
    switch(secndx+1) {  /* change the bias */
    case N_TEXT:
	if( (long)rel->r_vaddr < cofffile->txtaddr ||
	    (long)rel->r_vaddr > (long)(cofffile->txtaddr+cofffile->txtsize) ) {
		FatalError("Relocation against N_TEXT not in .text section\n");
	}
	dest32=(unsigned long *)((long)(cofffile->saddr[secndx])+
			((unsigned char *)rel->r_vaddr-cofffile->txtaddr));
	break;
    case N_DATA:
	if( (long)rel->r_vaddr < cofffile->dataddr ||
	    (long)rel->r_vaddr > (long)(cofffile->dataddr+cofffile->datsize) ) {
		FatalError("Relocation against N_DATA not in .data section\n");
	}
	dest32=(unsigned long *)((long)(cofffile->saddr[secndx])+
			((unsigned char *)rel->r_vaddr-cofffile->dataddr));
	break;
    case N_BSS:
	if( (long)rel->r_vaddr < cofffile->bssaddr ||
	    (long)rel->r_vaddr > (long)(cofffile->bssaddr+cofffile->bsssize) ) {
		FatalError("Relocation against N_TEXT not in .bss section\n");
	}
	dest32=(unsigned long *)((long)(cofffile->saddr[secndx])+
			((unsigned char *)rel->r_vaddr-cofffile->bssaddr));
	break;
    default:
	FatalError("Relocation against unknown section %d\n", secndx );
    }

    if( symbol->n_sclass == 0 )
	{
	    symval=(unsigned char *)(symbol->n_value+(*dest32)-symbol->n_type);
#ifdef COFFDEBUG
	    COFFDEBUG( "symbol->n_sclass==0\n" );
	    COFFDEBUG( "dest32=%x\t", dest32 );
	    COFFDEBUG( "symval=%x\t", symval );
	    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
	    *dest32=(unsigned long)symval;
	    return 0;
	}

    switch( rel->r_type )
	{
#if defined(i386)
	case R_DIR32:
	    symval=COFFGetSymbolValue(cofffile, rel->r_symndx);
	    if( symval ) {
#ifdef COFFDEBUG
		char *namestr;
		COFFDEBUG( "R_DIR32 %s\n",
			    namestr=COFFGetSymbolName(cofffile,rel->r_symndx) );
		xf86loaderfree(namestr);
		COFFDEBUG( "txtsize=%x\t", cofffile->txtsize );
		COFFDEBUG( "dest32=%x\t", dest32 );
		COFFDEBUG( "symval=%x\t", symval );
		COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		*dest32=(unsigned long)(symval+(*dest32)-symbol->n_value);
	    } else {
		switch( symbol->n_scnum ) {
		case N_UNDEF:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_DIR32 N_UNDEF\n" );
#endif
		    return COFFDelayRelocation(cofffile,secndx,rel);
		case N_ABS:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_DIR32 N_ABS\n" );
#endif
		    return 0;
		case N_DEBUG:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_DIR32 N_DEBUG\n" );
#endif
		    return 0;
		case N_COMMENT:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_DIR32 N_COMMENT\n" );
#endif
		    return 0;
		case N_TEXT:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_DIR32 N_TEXT\n" );
		    COFFDEBUG( "dest32=%x\t", dest32 );
		    COFFDEBUG( "symval=%x\t", symval );
		    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		    *dest32=(unsigned long)((*dest32)+
					   (unsigned long)(cofffile->saddr[N_TEXT-1]));
		    break;
		case N_DATA:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_DIR32 N_DATA\n" );
		    COFFDEBUG( "txtsize=%x\t", cofffile->txtsize );
		    COFFDEBUG( "dest32=%x\t", dest32 );
		    COFFDEBUG( "symval=%x\t", symval );
		    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		    *dest32=(unsigned long)((*dest32)+
					   ((unsigned long)(cofffile->saddr[N_DATA-1]))-
					   cofffile->dataddr);
		    break;
		case N_BSS:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_DIR32 N_BSS\n" );
		    COFFDEBUG( "dest32=%x\t", dest32 );
		    COFFDEBUG( "symval=%x\t", symval );
		    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		    *dest32=(unsigned long)((*dest32)+
					   (unsigned long)(cofffile->saddr[N_BSS-1])-
					   (cofffile->bssaddr));
		    break;
		default:
		    ErrorF("R_DIR32 with unexpected section %d\n",
			   symbol->n_scnum );
		}

	    }
#ifdef COFFDEBUG
	    COFFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_PCRLONG:
	    if( symbol->n_scnum == N_TEXT )
		break;

	    symval=COFFGetSymbolValue(cofffile, rel->r_symndx);
#ifdef COFFDEBUG
	    COFFDEBUG( "R_PCRLONG ");
	    COFFDEBUG( "dest32=%x\t", dest32 );
	    COFFDEBUG( "symval=%x\t", symval );
	    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
	    if( symval == 0 ) {
#ifdef COFFDEBUG
		char *name;
		COFFDEBUG( "***Unable to resolve symbol %s\n",
			    name=COFFGetSymbolName(cofffile,rel->r_symndx) );
		xf86loaderfree(name);
#endif
		return COFFDelayRelocation(cofffile,secndx,rel);
	    }
	    *dest32=(unsigned long)(symval-((long)dest32+sizeof(long)));

#ifdef COFFDEBUG
	    COFFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
	    break;
	case R_ABS:
	    /*
	     * Nothing to really do here.
	     * Usually, a dummy relocation for .file
	     */
	    break;
#endif /* i386 */
#if defined(__powerpc__)
	case R_POS:
	    /*
	     * Positive Relocation
	     */
	    if( RELOC_RLEN(*rel) != 0x1f )
		FatalError("R_POS with size != 32 bits" );
	    symval=COFFGetSymbolValue(cofffile, rel->r_symndx);
	    if( symval ) {
#ifdef COFFDEBUG
	    	COFFDEBUG( "R_POS ");
	    	COFFDEBUG( "dest32=%x\t", dest32 );
	    	COFFDEBUG( "symval=%x\t", symval );
	    	COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		*dest32=(unsigned long)(symval+(*dest32)-symbol->n_value);
		ppc_flush_icache(dest32);
	    } else {
		switch( symbol->n_scnum ) {
		case N_UNDEF:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_POS N_UNDEF\n" );
#endif
		    return COFFDelayRelocation(cofffile,secndx,rel);
		case N_ABS:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_POS N_ABS\n" );
#endif
		    return 0;
		case N_DEBUG:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_POS N_DEBUG\n" );
#endif
		    return 0;
		case N_COMMENT:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_POS N_COMMENT\n" );
#endif
		    return 0;
		case N_TEXT:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_POS N_TEXT\n" );
		    COFFDEBUG( "dest32=%x\t", dest32 );
		    COFFDEBUG( "symval=%x\t", symval );
		    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		    *dest32=(unsigned long)((*dest32)+
					   ((unsigned long)(cofffile->saddr[N_TEXT-1]))-
					   cofffile->txtaddr);
		    ppc_flush_icache(dest32);
		    break;
		case N_DATA:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_POS N_DATA\n" );
		    COFFDEBUG( "txtsize=%x\t", cofffile->txtsize );
		    COFFDEBUG( "dest32=%x\t", dest32 );
		    COFFDEBUG( "symval=%x\t", symval );
		    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		    *dest32=(unsigned long)((*dest32)+
					   ((unsigned long)(cofffile->saddr[N_DATA-1]))-
					   cofffile->dataddr);
		    ppc_flush_icache(dest32);
		    break;
		case N_BSS:
#ifdef COFFDEBUG
		    COFFDEBUG( "R_POS N_BSS\n" );
		    COFFDEBUG( "dest32=%x\t", dest32 );
		    COFFDEBUG( "symval=%x\t", symval );
		    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
		    *dest32=(unsigned long)((*dest32)+
					   (unsigned long)(cofffile->saddr[N_BSS-1])-
					   (cofffile->bssaddr));
		    ppc_flush_icache(dest32);
		    break;
		default:
		    ErrorF("R_POS with unexpected section %d\n",
			   symbol->n_scnum );
		}
	    }
#ifdef COFFDEBUG
	    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
	    COFFDEBUG( "\n" );
#endif
	    break;
	case R_TOC:
	    /*
	     * Relative to TOC
	     */
	    {
	    dest16=(unsigned short *)dest32;
	    if( RELOC_RLEN(*rel) != 0x0f )
		FatalError("R_TOC with size != 16 bits" );
#ifdef COFFDEBUG
	    COFFDEBUG( "R_TOC ");
	    COFFDEBUG( "dest16=%x\t", dest16 );
	    COFFDEBUG( "symbol=%x\t", symbol );
	    COFFDEBUG( "symbol->n_value=%x\t", symbol->n_value );
	    COFFDEBUG( "cofffile->toc=%x\t", cofffile->toc );
	    COFFDEBUG( "*dest16=%8.8x\t", *dest16 );
#endif
	    *dest16=(unsigned long)((symbol->n_value-cofffile->toc));
	    ppc_flush_icache(dest16);
	    }
#ifdef COFFDEBUG
	    COFFDEBUG( "*dest16=%8.8x\t", *dest16 );
	    COFFDEBUG( "\n" );
#endif
	    break;
	case R_BR:
	    /*
	     * Branch relative to self, non-modifiable
	     */

	    if( RELOC_RLEN(*rel) != 0x19 )
		FatalError("R_BR with size != 24 bits" );
	    name = COFFGetSymbolName(cofffile, rel->r_symndx);
	    symitem = LoaderHashFind(name);
	    if( symitem == 0 ) {
		name++;
		symitem = LoaderHashFind(name);
	    }
	    if( symitem && cofffile->module != symitem->module ) {
#ifdef COFFDEBUG
		COFFDEBUG("Symbol module %d != file module %d\n",
			symitem->module, cofffile->module );
#endif
		symval=COFFGetSymbolGlinkValue(cofffile, rel->r_symndx);
	    }
	    else
		symval=COFFGetSymbolValue(cofffile, rel->r_symndx);
	    if( symval == 0 ) {
#ifdef COFFDEBUG
		char *name;
		COFFDEBUG( "***Unable to resolve symbol %s\n",
			    name=COFFGetSymbolName(cofffile,rel->r_symndx) );
		xf86loaderfree(name);
#endif
		return COFFDelayRelocation(cofffile,secndx,rel);
	    }
#ifdef COFFDEBUG
	    COFFDEBUG( "R_BR ");
	    COFFDEBUG( "dest32=%x\t", dest32 );
	    COFFDEBUG( "symval=%x\t", symval );
	    COFFDEBUG( "*dest32=%8.8x\t", *dest32 );
#endif
	    {
		unsigned long val;
		val=((unsigned long)symval-(unsigned long)dest32);
#ifdef COFFDEBUG
	    COFFDEBUG( "val=%8.8x\n", val );
#endif
		val = val>>2;
		if( (val & 0x3f000000) != 0x3f000000 &&
		    (val & 0x3f000000) != 0x00000000 )  {
			FatalError( "R_BR offset %x too large\n", val<<2 );
			break;
		}
		val &= 0x00ffffff;
#ifdef COFFDEBUG
	    COFFDEBUG( "val=%8.8x\n", val );
#endif
		/*
		 * The address part contains the offset to the beginning
		 * of the .text section. Disreguard this since we have
		 * calculated the correct offset already.
		 */
		(*dest32)=((*dest32)&0xfc000003)|(val<<2);
#ifdef COFFDEBUG
	    COFFDEBUG( "*dest32=%8.8x\n", *dest32 );
#endif
		if( cofffile->module != symitem->module ) {
		    (*++dest32)=0x80410014;	/* lwz r2,20(r1) */
		}
		ppc_flush_icache(--dest32);
	    }

	    break;
#endif /* __powerpc__ */
	default:
	    ErrorF(
		   "COFF_RelocateEntry() Unsupported relocation type %o\n",
		   rel->r_type );
	    break;
	}
    return 0;
}

static COFFRelocPtr
COFFCollectRelocations(cofffile)
COFFModulePtr	cofffile;
{
    unsigned short	i,j;
    RELOC	*rel;
    SCNHDR	*sec;
    COFFRelocPtr reloc_head = NULL;
    COFFRelocPtr tmp;

    for(i=0; i<cofffile->numsh; i++ ) {
	if( cofffile->saddr[i] == NULL )
	    continue; /* Section not loaded!! */
	sec=&(cofffile->sections[i]);
	for(j=0;j<sec->s_nreloc;j++) {
	    rel=(RELOC *)(cofffile->reladdr[i]+(j*RELSZ));
	    tmp = COFFDelayRelocation(cofffile,i,rel);
	    tmp->next = reloc_head;
	    reloc_head = tmp;
	}
    }

    return reloc_head;
}

/*
 * COFF_GetSymbols()
 *
 * add the symbols to the symbol table maintained by the loader.
 */

static LOOKUP *
COFF_GetSymbols(cofffile)
COFFModulePtr	cofffile;
{
    SYMENT	*sym;
    AUXENT	*aux=NULL;
    int	i, l, numsyms;
    LOOKUP	*lookup, *lookup_common, *p;
    char	*symname;

/*
 * Load the symbols into memory
 */
    numsyms=cofffile->header->f_nsyms;

#ifdef COFFDEBUG
    COFFDEBUG("COFF_GetSymbols(): %d symbols\n", numsyms );
#endif

    cofffile->symsize=(numsyms*SYMESZ);
    cofffile->symtab=(SYMENT *)_LoaderFileToMem(cofffile->fd,cofffile->header->f_symptr,
						(numsyms*SYMESZ),"symbols");

    if ((lookup = xf86loadermalloc((numsyms+1)*sizeof(LOOKUP))) == NULL)
	return NULL;

    for(i=0,l=0; i<numsyms; i++)
	{
	    sym=(SYMENT *)(((unsigned char *)cofffile->symtab)+(i*SYMESZ));
	    symname=COFFGetSymbolName(cofffile,i);
	    if( sym->n_numaux > 0 )
		aux=(AUXENT *)(((unsigned char *)cofffile->symtab)+((i+1)*SYMESZ));
	    else
		aux=NULL;
#ifdef COFFDEBUG
	    COFFDEBUG("\t%d %d %x %x %d %d %s\n",
		       i, sym->n_scnum, sym->n_value, sym->n_type,
		       sym->n_sclass, sym->n_numaux, symname );
	    if( aux )
	    COFFDEBUG("aux=\t%d %x %x %x %x %x %x\n",
			aux->x_scnlen, aux->x_parmhash, aux->x_snhash,
			aux->x_smtyp, aux->x_smclas, aux->x_stab,
			aux->x_snstab );
#endif
	    i+=sym->n_numaux;
	    /*
	     * check for TOC csect before discarding C_HIDEXT below
	     */
	    if( aux && aux->x_smclas == XMC_TC0 ) {
		if( sym->n_scnum != N_DATA )
			FatalError("TOC not in N_DATA section");
		cofffile->toc=sym->n_value;
		cofffile->tocaddr=(cofffile->saddr[sym->n_scnum-1]+
			     sym->n_value-(cofffile->dataddr));
#ifdef COFFDEBUG
	        COFFDEBUG("TOC=%x\n", cofffile->toc );
	        COFFDEBUG("TOCaddr=%x\n", cofffile->tocaddr );
#endif
		continue;
	    }
	    if( sym->n_sclass == C_HIDEXT ) {
/*
		&& aux && !(aux->x_smclas == XMC_DS
		&& aux->x_smtyp == XTY_SD) ) ) {
*/
#ifdef COFFDEBUG
	    COFFDEBUG("Skipping C_HIDEXT class symbol %s\n", symname );
#endif
		continue;
	    }
	    switch( sym->n_scnum )
		{
		case N_UNDEF:
		    if( sym->n_value != 0 ) {
			char *name;
			COFFCommonPtr tmp;

			name = COFFGetSymbolName(cofffile,i);
#ifdef COFFDEBUG
			COFFDEBUG("Adding COMMON space for %s\n", name);
#endif
			if(!LoaderHashFind(name)) {
			    tmp = COFFAddCOMMON(sym,i);
			    if (tmp) {
				tmp->next = listCOMMON;
				listCOMMON = tmp;
			    }
			}
			xf86loaderfree(name);
		    }
		    xf86loaderfree(symname);
		    break;
		case N_ABS:
		case N_DEBUG:
		case N_COMMENT:
#ifdef COFFDEBUG
		    COFFDEBUG("Freeing %s, section %d\n",
			       symname, sym->n_scnum );
#endif
		    xf86loaderfree(symname);
		    break;
		case N_TEXT:
		    if( (sym->n_sclass == C_EXT || sym->n_sclass == C_HIDEXT)
		       && cofffile->saddr[sym->n_scnum-1]) {
			lookup[l].symName=symname;
			lookup[l].offset=(funcptr)
			    (cofffile->saddr[sym->n_scnum-1]+
			     sym->n_value-cofffile->txtaddr);
#ifdef COFFDEBUG
			COFFDEBUG("Adding %x %s\n",
				   lookup[l].offset, lookup[l].symName );
#endif
			l++;
		    }
		    else {
#ifdef COFFDEBUG
			COFFDEBUG( "TEXT Section not loaded %d\n",
				    sym->n_scnum-1 );
#endif
			xf86loaderfree(symname);
		    }
		    break;
		case N_DATA:
		    /*
		     * Note: COFF expects .data to be contiguous with
		     * .data, so that offsets for .data are relative to
		     * .text. We need to adjust for this, and make them
		     * relative to .data so that the relocation can be
		     * properly applied. This is needed becasue we allocate
		     * .data seperately from .text.
		     */
		    if( (sym->n_sclass == C_EXT || sym->n_sclass == C_HIDEXT)
		       && cofffile->saddr[sym->n_scnum-1]) {
			lookup[l].symName=symname;
			lookup[l].offset=(funcptr)
			    (cofffile->saddr[sym->n_scnum-1]+
			     sym->n_value-cofffile->dataddr);
#ifdef COFFDEBUG
			COFFDEBUG("Adding %x %s\n",
				   lookup[l].offset, lookup[l].symName );
#endif
			l++;
		    }
		    else {
#ifdef COFFDEBUG
			COFFDEBUG( "DATA Section not loaded %d\n",
				    sym->n_scnum-1 );
#endif
			xf86loaderfree(symname);
		    }
		    break;
		case N_BSS:
		    /*
		     * Note: COFF expects .bss to be contiguous with
		     * .data, so that offsets for .bss are relative to
		     * .text. We need to adjust for this, and make them
		     * relative to .bss so that the relocation can be
		     * properly applied. This is needed becasue we allocate
		     * .bss seperately from .text and .data.
		     */
		    if( (sym->n_sclass == C_EXT || sym->n_sclass == C_HIDEXT)
		       && cofffile->saddr[sym->n_scnum-1]) {
			lookup[l].symName=symname;
			lookup[l].offset=(funcptr)
			    (cofffile->saddr[sym->n_scnum-1]+
			     sym->n_value-cofffile->bssaddr);
#ifdef COFFDEBUG
			COFFDEBUG("Adding %x %s\n",
				   lookup[l].offset, lookup[l].symName );
#endif
			l++;
		    }
		    else {
#ifdef COFFDEBUG
			COFFDEBUG( "BSS Section not loaded %d\n",
				    sym->n_scnum-1 );
#endif
			xf86loaderfree(symname);
		    }
		    break;
		default:
		    ErrorF("Unknown Section number %d\n", sym->n_scnum );
		    xf86loaderfree(symname);
		    break;
		}
	}

    lookup[l].symName=NULL; /* Terminate the list */

    lookup_common = COFFCreateCOMMON(cofffile);
    if (lookup_common) {
	for (i = 0, p = lookup_common; p->symName; i++, p++)
	    ;
	memcpy(&(lookup[l]), lookup_common, i * sizeof (LOOKUP));

	xf86loaderfree(lookup_common);
	l += i;
	lookup[l].symName = NULL;
    }

/*
 * remove the COFF symbols that will show up in every module
 */
    for (i = 0, p = lookup; p->symName; i++, p++) {
	while (p->symName && (!strcmp(lookup[i].symName, ".text")
	       || !strcmp(lookup[i].symName, ".data")
	       || !strcmp(lookup[i].symName, ".bss")
	       )) {
	    memmove(&(lookup[i]), &(lookup[i+1]), (l-- - i) * sizeof (LOOKUP));
	}
    }

    return lookup;
}

#define SecOffset(index) cofffile->sections[index].s_scnptr
#define SecSize(index) cofffile->sections[index].s_size
#define SecAddr(index) cofffile->sections[index].s_paddr
#define RelOffset(index) cofffile->sections[index].s_relptr
#define RelSize(index) (cofffile->sections[index].s_nreloc*RELSZ)

/*
 * COFFCollectSections
 *
 * Do the work required to load each section into memory.
 */
static void
COFFCollectSections(cofffile)
COFFModulePtr	cofffile;
{
    unsigned short	i;

/*
 * Find and identify all of the Sections
 */

#ifdef COFFDEBUG
    COFFDEBUG("COFFCollectSections(): %d sections\n", cofffile->numsh );
#endif

    for( i=0; i<cofffile->numsh; i++) {
#ifdef COFFDEBUG
	COFFDEBUG("%d %s\n", i, cofffile->sections[i].s_name );
#endif
	/* .text */
	if( strcmp(cofffile->sections[i].s_name,
		   ".text" ) == 0 ) {
	    cofffile->text=_LoaderFileToMem(cofffile->fd,
					    SecOffset(i),SecSize(i),".text");
	    cofffile->saddr[i]=cofffile->text;
	    cofffile->txtndx=i;
	    cofffile->txtaddr=SecAddr(i);
	    cofffile->txtsize=SecSize(i);
	    cofffile->txtrelsize=RelSize(i);
	    cofffile->reladdr[i]=_LoaderFileToMem(cofffile->fd,
						  RelOffset(i), RelSize(i),".rel.text");
#ifdef COFFDEBUG
	    COFFDEBUG(".text starts at %x (%x bytes)\n", cofffile->text, cofffile->txtsize );
#endif
	    continue;
	}
	/* .data */
	if( strcmp(cofffile->sections[i].s_name,
		   ".data" ) == 0 ) {
	    cofffile->data=_LoaderFileToMem(cofffile->fd,
					    SecOffset(i),SecSize(i),".data");
	    cofffile->saddr[i]=cofffile->data;
	    cofffile->datndx=i;
	    cofffile->dataddr=SecAddr(i);
	    cofffile->datsize=SecSize(i);
	    cofffile->datrelsize=RelSize(i);
	    cofffile->reladdr[i]=_LoaderFileToMem(cofffile->fd,
						  RelOffset(i), RelSize(i),".rel.data");
#ifdef COFFDEBUG
	    COFFDEBUG(".data starts at %x (%x bytes)\n", cofffile->data, cofffile->datsize );
#endif
	    continue;
	}
	/* .bss */
	if( strcmp(cofffile->sections[i].s_name,
		   ".bss" ) == 0 ) {
	    if( SecSize(i) )
		cofffile->bss=xf86loadercalloc(1,SecSize(i));
	    else
		cofffile->bss=NULL;
	    cofffile->saddr[i]=cofffile->bss;
	    cofffile->bssndx=i;
	    cofffile->bssaddr=SecAddr(i);
	    cofffile->bsssize=SecSize(i);
#ifdef COFFDEBUG
	    COFFDEBUG(".bss starts at %x (%x bytes)\n", cofffile->bss, cofffile->bsssize );
#endif
	    continue;
	}
	/* .comment */
	if( strncmp(cofffile->sections[i].s_name,
		   ".comment",strlen(".comment") ) == 0 ) {
	    continue;
	}
	/* .stab */
	if( strcmp(cofffile->sections[i].s_name,
		   ".stab" ) == 0 ) {
	    continue;
	}
	/* .stabstr */
	if( strcmp(cofffile->sections[i].s_name,
		   ".stabstr" ) == 0 ) {
	    continue;
	}
	/* .stab.* */
	if( strncmp(cofffile->sections[i].s_name,
		   ".stab.", strlen(".stab.") ) == 0 ) {
	    continue;
	}
	ErrorF("COFF: Not loading %s\n", cofffile->sections[i].s_name );
    }
}

/*
 * Public API for the COFF implementation of the loader.
 */
void *
COFFLoadModule(modrec, cofffd, ppLookup)
loaderPtr	modrec;
int	cofffd;
LOOKUP **ppLookup;
{
    COFFModulePtr	cofffile;
    FILHDR *header;
    int stroffset; /* offset of string table */
    COFFRelocPtr coff_reloc, tail;
    void	*v;

#ifdef COFFDEBUG
    COFFDEBUG("COFFLoadModule(%s,%x,%x)\n",modrec->name,modrec->handle,cofffd);
#endif

    if ((cofffile = xf86loadercalloc(1,sizeof(COFFModuleRec))) == NULL) {
	ErrorF( "Unable to allocate COFFModuleRec\n" );
	return NULL;
    }

    cofffile->handle=modrec->handle;
    cofffile->module=modrec->module;
    cofffile->fd=cofffd;
    v=cofffile->funcs=modrec->funcs;

/*
 *  Get the COFF header
 */
    cofffile->header=(FILHDR *)_LoaderFileToMem(cofffd,0,sizeof(FILHDR),"header");
    header=(FILHDR *)cofffile->header;

    if( header->f_symptr == 0 || header->f_nsyms == 0 ) {
	ErrorF("No symbols found in module\n");
	_LoaderFreeFileMem(header,sizeof(FILHDR));
	xf86loaderfree(cofffile);
	return NULL;
    }
/*
 * Get the section table
 */
    cofffile->numsh=header->f_nscns;
    cofffile->secsize=(header->f_nscns*SCNHSZ);
    cofffile->sections=(SCNHDR *)_LoaderFileToMem(cofffd,FILHSZ+header->f_opthdr,
						  cofffile->secsize, "sections");
    cofffile->saddr=xf86loadercalloc(cofffile->numsh, sizeof(unsigned char *));
    cofffile->reladdr=xf86loadercalloc(cofffile->numsh, sizeof(unsigned char *));

/*
 * Load the optional header if we need it ?????
 */

/*
 * Load the rest of the desired sections
 */
    COFFCollectSections(cofffile);

/*
 * load the string table (must be done before we process symbols).
 */
    stroffset=header->f_symptr+(header->f_nsyms*SYMESZ);

    _LoaderFileRead(cofffd,stroffset,&(cofffile->strsize),sizeof(int));

    stroffset+=4; /* Move past the size */
    cofffile->strsize-=sizeof(int);	/* size includes itself, so reduce by 4 */
    cofffile->strtab=_LoaderFileToMem(cofffd,stroffset,cofffile->strsize,"strings");

/*
 * add symbols
 */
    *ppLookup = COFF_GetSymbols(cofffile);

/*
 * Do relocations
 */
    coff_reloc = COFFCollectRelocations(cofffile);
    if (coff_reloc) {
	for (tail = coff_reloc; tail->next; tail = tail->next)
	    ;
	tail->next = _LoaderGetRelocations(v)->coff_reloc;
	_LoaderGetRelocations(v)->coff_reloc = coff_reloc;
    }

    return (void *)cofffile;
}

void
COFFResolveSymbols(mod)
void *mod;
{
    COFFRelocPtr newlist, p, tmp;

    /* Try to relocate everything.  Build a new list containing entries
     * which we failed to relocate.  Destroy the old list in the process.
     */
    newlist = 0;
    for (p = _LoaderGetRelocations(mod)->coff_reloc; p; ) {
	tmp = COFF_RelocateEntry(p->file, p->secndx, p->rel);
	if (tmp) {
	    /* Failed to relocate.  Keep it in the list. */
	    tmp->next = newlist;
	    newlist = tmp;
	}
	tmp = p;
	p = p->next;
	xf86loaderfree(tmp);
    }
    _LoaderGetRelocations(mod)->coff_reloc = newlist;
}

int
COFFCheckForUnresolved( mod)
void	*mod;
{
    char	*name;
    COFFRelocPtr crel;
    int flag, fatalsym = 0;

    if ((crel = _LoaderGetRelocations(mod)->coff_reloc) == NULL)
	return 0;

    while( crel )
	{
	name = COFFGetSymbolName(crel->file, crel->rel->r_symndx);
        flag = _LoaderHandleUnresolved(name,
			_LoaderHandleToName(crel->file->handle));
        if (flag) fatalsym = 1;
	xf86loaderfree(name);
	crel=crel->next;
	}
    return fatalsym;
}

void
COFFUnloadModule(modptr)
void *modptr;
{
    COFFModulePtr	cofffile = (COFFModulePtr)modptr;
    COFFRelocPtr	relptr, reltptr, *brelptr;

/*
 * Delete any unresolved relocations
 */

    relptr=_LoaderGetRelocations(cofffile->funcs)->coff_reloc;
    brelptr=&(_LoaderGetRelocations(cofffile->funcs)->coff_reloc);

    while(relptr) {
	if( relptr->file == cofffile ) {
	    *brelptr=relptr->next;	/* take it out of the list */
	    reltptr=relptr;		/* save pointer to this node */
	    relptr=relptr->next;	/* advance the pointer */
	    xf86loaderfree(reltptr);		/* free the node */
	}
	else {
	    brelptr=&(relptr->next);
	    relptr=relptr->next;	/* advance the pointer */
        }
    }

/*
 * Delete any symbols in the symbols table.
 */

    LoaderHashTraverse((void *)cofffile, COFFhashCleanOut);

/*
 * Free the sections that were allocated.
 */
#define CheckandFree(ptr,size)	if(ptr) _LoaderFreeFileMem((ptr),(size))

    CheckandFree(cofffile->strtab,cofffile->strsize);
    CheckandFree(cofffile->symtab,cofffile->symsize);
    CheckandFree(cofffile->text,cofffile->txtsize);
    CheckandFree(cofffile->reladdr[cofffile->txtndx],cofffile->txtrelsize);
    CheckandFree(cofffile->data,cofffile->datsize);
    CheckandFree(cofffile->reladdr[cofffile->datndx],cofffile->datrelsize);
    CheckandFree(cofffile->bss,cofffile->bsssize);
    if( cofffile->common )
	xf86loaderfree(cofffile->common);
/*
 * Free the section table, and section pointer array
 */
    _LoaderFreeFileMem(cofffile->sections,cofffile->secsize);
    xf86loaderfree(cofffile->saddr);
    xf86loaderfree(cofffile->reladdr);
    _LoaderFreeFileMem(cofffile->header,sizeof(FILHDR));
/*
 * Free the COFFModuleRec
 */
    xf86loaderfree(cofffile);

    return;
}

char *
COFFAddressToSection(void *modptr, unsigned long address)
{
    COFFModulePtr cofffile = (COFFModulePtr)modptr;
    int i;

    for( i=1; i<cofffile->numsh; i++) {
        if( address >= (unsigned long)cofffile->saddr[i] &&
            address <= (unsigned long)cofffile->saddr[i]+SecSize(i) ) {
                return cofffile->sections[i].s_name;
                }
        }
return NULL;
}
