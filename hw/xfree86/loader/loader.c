/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loader.c,v 1.63 2002/11/25 14:05:03 eich Exp $ */

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
    || defined(__x86_64__))
#include <malloc.h>
#endif
#include <stdarg.h>
#include "ar.h"
#include "elf.h"
#include "coff.h"

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
void * os2ldcalloc(size_t,size_t);
#endif

#ifdef HANDLE_IN_HASH_ENTRY
/*
 * handles are used to identify files that are loaded. Even archives
 * are counted as a single file.
 */
#define MAX_HANDLE 256
#define HANDLE_FREE 0
#define HANDLE_USED 1
static char freeHandles[MAX_HANDLE] ;
static int refCount[MAX_HANDLE] ;
#endif

#if defined(__sparc__) && defined(__GNUC__)
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
   { 0, 0 }
};
#endif
static LOOKUP SparcLookupTab[] = {
   SYMFUNCDOT(rem)
   SYMFUNCDOT(urem)
   SYMFUNCDOT(mul)
   SYMFUNCDOT(umul)
   SYMFUNCDOT(div)
   SYMFUNCDOT(udiv)
   { 0, 0 }
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
    __asm("" : "=r" (hwcap) : "0" (hwcap));
    if (hwcap) {
        if (*hwcap & HWCAP_SPARC_MULDIV)
    	    return 1;
    	else
    	    return 0;
    }
#endif
    f = fopen("/proc/cpuinfo","r");
    if (!f) return 0;
    while (fgets(buffer, 1024, f) != NULL) {
        if (!strncmp (buffer, "type", 4)) {
            p = strstr (buffer, "sun4");
            if (p && (p[4] == 'u' || p[4] == 'd' || p[4] == 'm')) {
                fclose(f);
                return 1;
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
    int			num;
    const char **	list;
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
/*ARGSUSED*/
static void ARCHIVEResolveSymbols(void *unused) {}
/*ARGSUSED*/
static int ARCHIVECheckForUnresolved(void *v) { return 0; }
/*ARGSUSED*/
static char *ARCHIVEAddressToSection(void *modptr, unsigned long address)
{ return NULL; }
/*ARGSUSED*/
static void ARCHIVEUnload(void *unused2) {}

/*
 * Array containing entry points for different formats.
 */

static loader_funcs funcs[] = {
	/* LD_ARCHIVE */
	{ARCHIVELoadModule,
	 ARCHIVEResolveSymbols,
	 ARCHIVECheckForUnresolved,
	 ARCHIVEAddressToSection,
	 ARCHIVEUnload, {0,0,0,0,0}},
	/* LD_ELFOBJECT */
	{ELFLoadModule,
	 ELFResolveSymbols,
	 ELFCheckForUnresolved,
	 ELFAddressToSection,
	 ELFUnloadModule, {0,0,0,0,0}},
	/* LD_COFFOBJECT */
	{COFFLoadModule,
	 COFFResolveSymbols,
	 COFFCheckForUnresolved,
	 COFFAddressToSection,
	 COFFUnloadModule, {0,0,0,0,0}},
	/* LD_XCOFFOBJECT */
	{COFFLoadModule,
	 COFFResolveSymbols,
	 COFFCheckForUnresolved,
	 COFFAddressToSection,
	 COFFUnloadModule, {0,0,0,0,0}},
	/* LD_AOUTOBJECT */
	{AOUTLoadModule,
	 AOUTResolveSymbols,
	 AOUTCheckForUnresolved,
	 AOUTAddressToSection,
	 AOUTUnloadModule, {0,0,0,0,0}},
	/* LD_AOUTDLOBJECT */
#ifdef DLOPEN_SUPPORT
	{DLLoadModule,
	 DLResolveSymbols,
	 DLCheckForUnresolved,
	 ARCHIVEAddressToSection,
	 DLUnloadModule, {0,0,0,0,0}},
#else
	{AOUTLoadModule,
	 AOUTResolveSymbols,
	 AOUTCheckForUnresolved,
	 AOUTAddressToSection,
	 AOUTUnloadModule, {0,0,0,0,0}},
#endif
	/* LD_ELFDLOBJECT */
#ifdef DLOPEN_SUPPORT
	{DLLoadModule,
	 DLResolveSymbols,
	 DLCheckForUnresolved,
	 ARCHIVEAddressToSection,
	 DLUnloadModule, {0,0,0,0,0}},
#else
	{ELFLoadModule,
	 ELFResolveSymbols,
	 ELFCheckForUnresolved,
	 ELFAddressToSection,
	 ELFUnloadModule, {0,0,0,0,0}},
#endif
	};

int	numloaders=sizeof(funcs)/sizeof(loader_funcs);


void
LoaderInit(void)
{
    const char *osname = NULL;

    LoaderAddSymbols(-1, -1, miLookupTab ) ;
    LoaderAddSymbols(-1, -1, xfree86LookupTab ) ;
    LoaderAddSymbols(-1, -1, dixLookupTab ) ;
    LoaderAddSymbols(-1, -1, fontLookupTab ) ;
    LoaderAddSymbols(-1, -1, extLookupTab );
#ifdef __sparc__
#ifdef linux
    if (sparcUseHWMulDiv())
	LoaderAddSymbols(-1, -1, SparcV89LookupTab ) ;
    else
#endif
	LoaderAddSymbols(-1, -1, SparcLookupTab ) ;
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
     || ( defined __x86_64__ && ! defined UseMMAP && ! defined DoMMAPedMerge))
    /*
     * The glibc malloc uses mmap for large allocations anyway. This breaks
     * some relocation types because the offset overflow. See loader.h for more
     * details. We need to turn off this behavior here.
     */
    mallopt(M_MMAP_MAX,0);
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
    unsigned char	buf[256]; /* long enough for the largest magic type */

    if( read(fd,buf,sizeof(buf)) < 0 ) {
	return -1;
    }

#ifdef DEBUG
    ErrorF("Checking module type %10s\n", buf );
    ErrorF("Checking module type %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3] );
#endif

    lseek(fd,offset,SEEK_SET);

    if (strncmp((char *) buf, ARMAG, SARMAG) == 0) {
	return LD_ARCHIVE;
    }

#if defined(AIAMAG)
    /* LynxOS PPC style archives */
    if (strncmp((char *) buf, AIAMAG, SAIAMAG) == 0) {
	return LD_ARCHIVE;
    }
#endif

    if (strncmp((char *) buf, ELFMAG, SELFMAG) == 0) {
	if( *((Elf32_Half *)(buf + ELFDLOFF)) == ELFDLMAG ) {
	    return LD_ELFDLOBJECT;
	} else {
	    return LD_ELFOBJECT;
	}
    }

    if( buf[0] == 0x4c && buf[1] == 0x01 ) {
	/* I386MAGIC */
	return LD_COFFOBJECT;
    }
    if( buf[0] == 0x01 && buf[1] == 0xdf ) {
	/* XCOFFMAGIC */
	return LD_COFFOBJECT;
	}
    if( buf[0] == 0x0d && buf[1] == 0x01 ) {
	/* ZCOFFMAGIC (LynxOS) */
	return LD_COFFOBJECT;
    }
    if( buf[0] == 0x00 && buf[1] == 0x86 && buf[2] == 0x01 && buf[3] == 0x07) {
        /* AOUTMAGIC */
        return LD_AOUTOBJECT;
    }
    if (buf[0] == 0x07 && buf[1] == 0x01 && (buf[2] == 0x64 || buf[2] == 0x86))
    {
        /* AOUTMAGIC, (Linux OMAGIC, old impure format, also used by OS/2 */
        return LD_AOUTOBJECT;
    }
    if (buf[0] == 0x07 && buf[1] == 0x01 && buf[2] == 0x00 && buf[3] == 0x00)
    {
        /* AOUTMAGIC, BSDI */
        return LD_AOUTOBJECT;
    }
    if ((buf[0] == 0xc0 && buf[1] == 0x86) || /* big endian form */
	(buf[3] == 0xc0 && buf[2] == 0x86)) { /* little endian form */
        /* i386 shared object */
        return LD_AOUTDLOBJECT;
    }

    return LD_UNKNOWN;
}


static int	offsetbias=0; /* offset into archive */
/*
 * _LoaderFileToMem() loads the contents of a file into memory using
 * the most efficient method for a platform.
 */
void *
_LoaderFileToMem(int fd, unsigned long offset,int size, char *label)
{
#ifdef UseMMAP
    unsigned long ret;	
# ifdef MmapPageAlign
    unsigned long pagesize;
    unsigned long new_size;
    unsigned long new_off;
    unsigned long new_off_bias;
# endif
# define MMAP_PROT	(PROT_READ|PROT_WRITE|PROT_EXEC)

# ifdef DEBUGMEM
    ErrorF("_LoaderFileToMem(%d,%u(%u),%d,%s)",fd,offset,offsetbias,size,label);
# endif
# ifdef MmapPageAlign
    pagesize = getpagesize();
    new_size = (size + pagesize - 1) / pagesize;
    new_size *= pagesize;
    new_off = (offset + offsetbias) / pagesize;
    new_off *= pagesize;
    new_off_bias = (offset + offsetbias) - new_off;
    if ((new_off_bias + size) > new_size) new_size += pagesize;
    ret = (unsigned long) mmap(0,new_size,MMAP_PROT,MAP_PRIVATE
#  ifdef __x86_64__
			       | MAP_32BIT
#  endif
			       ,
			       fd,new_off);
    if(ret == -1)
	FatalError("mmap() failed: %s\n", strerror(errno) );
    return (void *)(ret + new_off_bias);
# else
    ret = (unsigned long) mmap(0,size,MMAP_PROT,MAP_PRIVATE
#  ifdef __x86_64__
			       | MAP_32BIT
#  endif
			       ,
			       fd,offset + offsetbias);
    if(ret == -1)
	FatalError("mmap() failed: %s\n", strerror(errno) );
    return (void *)ret;
# endif
#else
    char *ptr;

# ifdef DEBUGMEM
    ErrorF("_LoaderFileToMem(%d,%u(%u),%d,%s)",fd,offset,offsetbias,size,label);
# endif

    if(size == 0){
# ifdef DEBUGMEM
	ErrorF("=NULL\n",ptr);
# endif
	return NULL;
    }

# ifndef __UNIXOS2__
    if( (ptr=xf86loadercalloc(size,1)) == NULL )
	FatalError("_LoaderFileToMem() malloc failed\n" );
# else
    if( (ptr=os2ldcalloc(size,1)) == NULL )
	FatalError("_LoaderFileToMem() malloc failed\n" );
# endif
# if defined(linux) && defined(__ia64__)
    {
	unsigned long page_size = getpagesize();
	unsigned long round;

	round = (unsigned long)ptr & (page_size-1);
	mprotect(ptr - round, (size+round+page_size-1) & ~(page_size-1),
		 PROT_READ|PROT_WRITE|PROT_EXEC);
    }
# endif

    if(lseek(fd,offset+offsetbias,SEEK_SET)<0)
	FatalError("\n_LoaderFileToMem() lseek() failed: %s\n",strerror(errno));

    if(read(fd,ptr,size)!=size)
	FatalError("\n_LoaderFileToMem() read() failed: %s\n",strerror(errno));

# if (defined(linux) || defined(__NetBSD__) || defined(__OpenBSD__)) \
    && defined(__powerpc__) 
    /*
     * Keep the instruction cache in sync with changes in the
     * main memory.
     */
    { 
	int i; 
	for (i = 0; i < size; i += 16) 
	    ppc_flush_icache(ptr+i); 
	ppc_flush_icache(ptr+size-1); 
    } 
# endif

# ifdef DEBUGMEM
    ErrorF("=%lx\n",ptr);
# endif

    return (void *)ptr;
#endif
}

/*
 * _LoaderFreeFileMem() free the memory in which a file was loaded.
 */
void
_LoaderFreeFileMem(void *addr, int size)
{
#if defined (UseMMAP) && defined (MmapPageAlign)
    unsigned long pagesize = getpagesize();
    memType i_addr = (memType)addr;
    unsigned long new_size;
#endif
#ifdef DEBUGMEM
    ErrorF("_LoaderFreeFileMem(%x,%d)\n",addr,size);
#endif
#ifdef UseMMAP
# if defined (MmapPageAlign)
    i_addr /=  pagesize;
    i_addr *=  pagesize;
    new_size = (size + pagesize - 1) / pagesize;
    new_size *= pagesize;
    if (((memType)addr - i_addr + size) > new_size) new_size += pagesize;
    munmap((void *)i_addr,new_size);
# else
    munmap((void *)addr,size);
# endif
#else
    if(size == 0)
	return;

    xf86loaderfree(addr);
#endif

    return;
}

int
_LoaderFileRead(int fd, unsigned int offset, void *buf, int size)
{
    if(lseek(fd,offset+offsetbias,SEEK_SET)<0)
	FatalError("_LoaderFileRead() lseek() failed: %s\n", strerror(errno) );

    if(read(fd,buf,size)!=size)
	FatalError("_LoaderFileRead() read() failed: %s\n", strerror(errno) );

    return size;
}

static loaderPtr listHead = (loaderPtr) 0 ;

static loaderPtr
_LoaderListPush()
{
  loaderPtr item = xf86loadercalloc(1, sizeof (struct _loader));
  item->next = listHead ;
  listHead = item;

  return item;
}

static loaderPtr
_LoaderListPop(int handle)
{
  loaderPtr item=listHead;
  loaderPtr *bptr=&listHead; /* pointer to previous node */

  while(item) {
	if( item->handle == handle ) {
	    *bptr=item->next;	/* remove this from the list */
	    return item;
	    }
	bptr=&(item->next);
	item=item->next;
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
  loaderPtr item=listHead;
  loaderPtr aritem=NULL;
  loaderPtr lastitem=NULL;

  if ( handle < 0 ) {
	return "(built-in)";
	}
  while(item) {
	if( item->handle == handle ) {
	    if( strchr(item->name,':') == NULL )
		aritem=item;
	    else
		lastitem=item;
	    }
	item=item->next;
	}

  if( aritem )
    return aritem->name;

  if( lastitem )
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
  loaderPtr item=listHead;
  loaderPtr lastitem=NULL;

  if ( handle < 0 ) {
	return "(built-in)";
  }
  while(item) {
	if( item->handle == handle ) {
		lastitem=item;
	}
	item=item->next;
  }

  if( lastitem )
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
  loaderPtr item=listHead;
  loaderPtr aritem=NULL;
  loaderPtr lastitem=NULL;

  if ( module < 0 ) {
	return "(built-in)";
  }
  while(item) {
	if( item->module == module ) {
	    if( strchr(item->name,':') == NULL )
		aritem=item;
	    else
		lastitem=item;
	}
	item=item->next;
  }

  if( aritem )
    return aritem->name;

  if( lastitem )
    return lastitem->name;

  return 0;
}

/*
 * _LoaderAddressToSection() will return the name of the file & section
 * that contains the given address.
 */
int
_LoaderAddressToSection(const unsigned long address, const char **module,
			const char ** section)
{
  loaderPtr item=listHead;

  while(item) {
	if( (*section=item->funcs->AddressToSection(item->private, address)) != NULL ) {
		*module=_LoaderModuleToName(item->module);
		return 1;
	}
	item=item->next;
  }

  return 0;
}


/*
 * Add a list of symbols to the referenced list.
 */

static void
AppendSymbol(symlist *list, const char *sym)
{
    list->list = xnfrealloc(list->list, (list->num + 1) * sizeof(char **));
    list->list[list->num] = sym;
    list->num++;
}

static void
AppendSymList(symlist *list, const char **syms)
{
    while (*syms) {
	AppendSymbol(list, *syms);
	syms++;
    }
}

static int
SymInList(symlist *list, char *sym)
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

void
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

void
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

void
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

void
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
    return(fatalsym);
}

/*
 * Handle an archive.
 */
void *
ARCHIVELoadModule(loaderPtr modrec, int arfd, LOOKUP **ppLookup)
{
    loaderPtr tmp = NULL;
    void *ret = NULL;
    unsigned char	magic[SARMAG];
    struct ar_hdr	hdr;
#if defined(__powerpc__) && defined(Lynx)
    struct fl_hdr	fhdr;
    char		name[255];
    int			namlen;
#endif
    unsigned int	size;
    unsigned int	offset;
    int	arnamesize, modnamesize;
    char	*slash, *longname;
    char		*nametable = NULL;
    int  		nametablelen = 0;
    LOOKUP *lookup_ret, *p;
    LOOKUP *myLookup = NULL; /* Does realloc behave if ptr == 0? */
    int modtype;
    int i;
    int numsyms = 0;
    int resetoff;

    /* lookup_ret = xf86loadermalloc(sizeof (LOOKUP *)); */

    arnamesize=strlen(modrec->name);

#if !(defined(__powerpc__) && defined(Lynx))
    read(arfd,magic,SARMAG);

    if(strncmp((const char *)magic,ARMAG,SARMAG) != 0 ) {
	ErrorF("ARCHIVELoadModule: wrong magic!!\n" );
	return NULL;
    }
    resetoff=SARMAG;
#else
    read(arfd,&fhdr,FL_HSZ);

    if(strncmp(fhdr.fl_magic,AIAMAG,SAIAMAG) != 0 ) {
	ErrorF("ARCHIVELoadModule: wrong magic!!\n" );
	return NULL;
    }
    resetoff=FL_HSZ;
#endif /* __powerpc__ && Lynx */

#ifdef DEBUGAR
    ErrorF("Looking for archive members starting at offset %o\n", offset );
#endif

    while( read(arfd,&hdr,sizeof(struct ar_hdr)) ) {

	longname = NULL;
	sscanf(hdr.ar_size,"%d",&size);
#if defined(__powerpc__) && defined(Lynx)
	sscanf(hdr.ar_namlen,"%d",&namlen);
	name[0]=hdr.ar_name[0];
	name[1]=hdr.ar_name[1];
	read(arfd,&name[2],namlen);
	name[namlen]='\0';
	offset=lseek(arfd,0,SEEK_CUR);
	if( offset&0x1 ) /* odd value */
		offset=lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
#endif
	offset=lseek(arfd,0,SEEK_CUR);

	/* Check for a Symbol Table */
	if( (hdr.ar_name[0] == '/' && hdr.ar_name[1] == ' ') ||
#if defined(__powerpc__) && defined(Lynx)
	    namlen == 0 ||
#endif
	    strncmp(hdr.ar_name, "__.SYMDEF", 9) == 0 ) {
	    /* If the file name is NULL, then it is a symbol table */
#ifdef DEBUGAR
	    ErrorF("Symbol Table Member '%16.16s', size %d, offset %d\n",
	           hdr.ar_name, size, offset );
	    ErrorF("Symbol table size %d\n", size );
#endif
	    offset=lseek(arfd,offset+size,SEEK_SET);
	    if( offset&0x1 ) /* odd value */
	        offset=lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
	    continue;
	}

	/* Check for a String Table */
	if( hdr.ar_name[0] == '/' && hdr.ar_name[1] == '/') { 
	    /* If the file name is '/', then it is a string table */
#ifdef DEBUGAR
	    ErrorF("String Table Member '%16.16s', size %d, offset %d\n",
	           hdr.ar_name, size, offset );
	    ErrorF("String table size %d\n", size );
#endif
	    nametablelen = size;
	    nametable=(char *)xf86loadermalloc(nametablelen);
	    read(arfd, nametable, size);
	    offset=lseek(arfd,0,SEEK_CUR);
	    /* offset=lseek(arfd,offset+size,SEEK_SET); */
	    if( offset&0x1 ) /* odd value */
		    offset=lseek(arfd,1,SEEK_CUR); /* make it an even boundary */
	    continue;
	}

	if (hdr.ar_name[0] == '/') {
	    /*  SYS V r4 style long member name */
	    int nameoffset = atol(&hdr.ar_name[1]);
	    char *membername;
	    if (!nametable) {
		ErrorF( "Missing string table whilst processing %s\n", 
			modrec->name ) ;
		offsetbias = 0;
		return NULL;
	    }
	    if (nameoffset > nametablelen) {
		ErrorF( "Invalid string table offset (%s) whilst processing %s\n", 
			hdr.ar_name, modrec->name ) ;
		offsetbias = 0;
		xf86loaderfree(nametable);
		return NULL;
	    }
	    membername = nametable + nameoffset;
	    slash=strchr(membername,'/');
	    if (slash)
		*slash = '\0';
	    longname = xf86loadermalloc(arnamesize + strlen(membername) + 2);
	    strcpy(longname,modrec->name);
	    strcat(longname,":");
	    strcat(longname,membername);
	} else if (hdr.ar_name[0] == '#' && hdr.ar_name[1] == '1' &&
		   hdr.ar_name[2] == '/') {
	    /* BSD 4.4 style long member name */
            if (sscanf(hdr.ar_name+3, "%d", &modnamesize) != 1) {
		ErrorF("Bad archive member %s\n", hdr.ar_name);
		offsetbias = 0;
		return NULL;
	    }
	    /* allocate space for fully qualified name */
	    longname = xf86loadermalloc(arnamesize + modnamesize + 2);
	    strcpy(longname,modrec->name);
	    strcat(longname,":");
	    i = read(arfd, longname+modnamesize+1, modnamesize);
	    if (i != modnamesize) {
		ErrorF("Bad archive member %d\n", hdr.ar_name);
		xf86loaderfree(longname);
		offsetbias = 0;
		return NULL;
	    }               
	    longname[i] = '\0';
	    offset += i;
	    size -= i;
	 } else {
	    /* Regular archive member */
#ifdef DEBUGAR
	    ErrorF("Member '%16.16s', size %d, offset %x\n",
#if !(defined(__powerpc__) && defined(Lynx))
		   hdr.ar_name,
#else
		   name,
#endif
		   size, offset );
#endif

	    slash=strchr(hdr.ar_name,'/');
	    if (slash == NULL) {
		/* BSD format without trailing slash */
		slash = strchr(hdr.ar_name,' ');
	    } 
	    /* SM: Make sure we do not overwrite other parts of struct */
        
	    if((slash - hdr.ar_name) > sizeof(hdr.ar_name)) 
                slash = hdr.ar_name + sizeof(hdr.ar_name) -1;
	    *slash='\000';
	}
	if( (modtype=_GetModuleType(arfd,offset)) < 0 ) {
	    ErrorF( "%s is an unrecognized module type\n", hdr.ar_name ) ;
	    offsetbias=0;
	    if (nametable)
		xf86loaderfree(nametable);
	    return NULL;
	}

	tmp=_LoaderListPush();

	tmp->handle = modrec->handle;
	tmp->module = moduleseq++;
	tmp->cname = xf86loadermalloc(strlen(modrec->cname) + 1);
	strcpy(tmp->cname, modrec->cname);
	tmp->funcs=&funcs[modtype];
	if (longname == NULL) {
	    modnamesize=strlen(hdr.ar_name);
	    tmp->name=(char *)xf86loadermalloc(arnamesize+modnamesize+2 );
	    strcpy(tmp->name,modrec->name);
	    strcat(tmp->name,":");
	    strcat(tmp->name,hdr.ar_name);
	    
	} else {
	    tmp->name = longname;
	}
	offsetbias=offset;

	if((tmp->private = funcs[modtype].LoadModule(tmp, arfd,
						     &lookup_ret))
	   == NULL) {
	    ErrorF( "Failed to load %s\n", hdr.ar_name ) ;
	    offsetbias=0;
	    if (nametable)
		xf86loaderfree(nametable);
	    return NULL;
	}

	offset=lseek(arfd,offset+size,SEEK_SET);
	if( offset&0x1 ) /* odd value */
		lseek(arfd,1,SEEK_CUR); /* make it an even boundary */

	if (tmp->private == (void *) -1L) {
	    ErrorF("Skipping \"%s\":  No symbols found\n", tmp->name);
	    continue;
	}
	else
	    ret = tmp->private;

	/* Add the lookup table returned from funcs.LoadModule to the
	 * one we're going to return.
	 */
	for (i = 0, p = lookup_ret; p && p->symName; i++, p++)
	    ;
	if (i) {
	    myLookup = xf86loaderrealloc(myLookup, (numsyms + i + 1)
					   * sizeof (LOOKUP));
	    if (!myLookup)
		continue; /* Oh well! */

	    memcpy(&(myLookup[numsyms]), lookup_ret, i * sizeof (LOOKUP));
	    numsyms += i;
	    myLookup[numsyms].symName = 0;
	}
	xf86loaderfree(lookup_ret);
    }
    /* xf86loaderfree(lookup_ret); */
    offsetbias=0;

    *ppLookup = myLookup;
    if (nametable)
	xf86loaderfree(nametable);

    return ret;
}

/*
 * Relocation list manipulation routines
 */

/*
 * _LoaderGetRelocations() Return the list of outstanding relocations
 */
LoaderRelocPtr
_LoaderGetRelocations(void *mod)
{
	loader_funcs	*formatrec = (loader_funcs *)mod;

	return  &(formatrec->pRelocs);
}

/*
 * Public Interface to the loader.
 */

int
LoaderOpen(const char *module, const char *cname, int handle,
	   int *errmaj, int *errmin, int *wasLoaded)
{
    loaderPtr tmp ;
    int new_handle, modtype ;
    int fd;
    LOOKUP *pLookup;

#if defined(DEBUG)
    ErrorF("LoaderOpen(%s)\n", module );
#endif

    /*
     * Check to see if the module is already loaded.
     * Only if we are loading it into an existing namespace.
     * If it is to be loaded into a new namespace, don't check.
     * Note: We only have one namespace.
     */
    if (handle >= 0) {
	tmp = listHead;
	while ( tmp ) {
#ifdef DEBUGLIST
	    ErrorF("strcmp(%x(%s),{%x} %x(%s))\n", module,module,&(tmp->name),
		   tmp->name,tmp->name );
#endif
	    if ( ! strcmp( module, tmp->name )) {
		refCount[tmp->handle]++;
		if (wasLoaded)
		    *wasLoaded = 1;
		xf86MsgVerb(X_INFO, 2, "Reloading %s\n", module);
		return tmp->handle;
	    }
	    tmp = tmp->next ;
	}
    }

    /*
     * OK, it's a new one. Add it.
     */
    xf86Msg(X_INFO, "Loading %s\n", module ) ;
    if (wasLoaded)
	*wasLoaded = 0;

    /*
     * Find a free handle.
     */
    new_handle = 1;
    while ( freeHandles[new_handle] && new_handle < MAX_HANDLE )
	new_handle ++ ;

    if ( new_handle == MAX_HANDLE ) {
	xf86Msg(X_ERROR, "Out of loader space\n" ) ; /* XXX */
	if(errmaj) *errmaj = LDR_NOSPACE;
	if(errmin) *errmin = LDR_NOSPACE;
	return -1 ;
    }

    freeHandles[new_handle] = HANDLE_USED ;
    refCount[new_handle] = 1;

    if( (fd=open(module, O_RDONLY)) < 0 ) {
	xf86Msg(X_ERROR, "Unable to open %s\n", module );
	freeHandles[new_handle] = HANDLE_FREE ;
	if(errmaj) *errmaj = LDR_NOMODOPEN;
	if(errmin) *errmin = errno;
	return -1 ;
    }

    if( (modtype=_GetModuleType(fd,0)) < 0 ) {
	xf86Msg(X_ERROR, "%s is an unrecognized module type\n", module ) ;
        freeHandles[new_handle] = HANDLE_FREE ;
	if(errmaj) *errmaj = LDR_UNKTYPE;
	if(errmin) *errmin = LDR_UNKTYPE;
	return -1;
    }

    tmp=_LoaderListPush();
    tmp->name = xf86loadermalloc(strlen(module) + 1);
    strcpy(tmp->name, module);
    tmp->cname = xf86loadermalloc(strlen(cname) + 1);
    strcpy(tmp->cname, cname);
    tmp->handle = new_handle;
    tmp->module = moduleseq++;
    tmp->funcs=&funcs[modtype];

    if((tmp->private = funcs[modtype].LoadModule(tmp,fd, &pLookup)) == NULL) {
	xf86Msg(X_ERROR, "Failed to load %s\n", module ) ;
	_LoaderListPop(new_handle);
        freeHandles[new_handle] = HANDLE_FREE ;
	if(errmaj) *errmaj = LDR_NOLOAD;
	if(errmin) *errmin = LDR_NOLOAD;
	return -1;
    }

    if (tmp->private != (void *) -1L) {
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

void *
LoaderSymbol(const char *sym)
{
    int i;
    itemPtr item = NULL;
    for (i = 0; i < numloaders; i++)
	funcs[i].ResolveSymbols(&funcs[i]);

    item = (itemPtr) LoaderHashFind(sym);

    if ( item )
	return item->address ;
    else
#ifdef DLOPEN_SUPPORT
	return(DLFindSymbol(sym));
#else
	return NULL;
#endif
}

int
LoaderResolveSymbols(void)
{
    int i;
    for(i=0;i<numloaders;i++)
	funcs[i].ResolveSymbols(&funcs[i]);
    return 0;
}

int
LoaderCheckUnresolved(int delay_flag )
{
  int i,ret=0;
  LoaderResolveOptions delayFlag = delay_flag;

  LoaderResolveSymbols();

  if (delayFlag == LD_RESOLV_NOW) {
     if (check_unresolved_sema > 0) 
	check_unresolved_sema--;
     else 
	xf86Msg(X_WARNING, "LoaderCheckUnresolved: not enough "
		"MAGIC_DONT_CHECK_UNRESOLVED\n");
  }

  if (!check_unresolved_sema ||  delayFlag == LD_RESOLV_FORCE)
	for(i=0;i<numloaders;i++)
	   if (funcs[i].CheckForUnresolved(&funcs[i]))
		ret=1;

  if (fatalReqSym)
    FatalError("Some required symbols were unresolved\n");

  return ret;
}

void xf86LoaderTrap(void);

void
xf86LoaderTrap(void)
{
}

void
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
  loaderRec fakeHead ;
  loaderPtr tmp = & fakeHead ;

  if ( handle < 0 || handle > MAX_HANDLE )
	return -1;

 /*
  * check the reference count, only free it if it goes to zero
  */
  if (--refCount[handle])
	return 0;
 /*
  * find the loaderRecs associated with this handle.
  */

  while( (tmp=_LoaderListPop(handle)) != NULL ) {
	if( strchr(tmp->name,':') == NULL ) {
		/* It is not a member of an archive */
		xf86Msg(X_INFO, "Unloading %s\n", tmp->name ) ;
	}
	tmp->funcs->LoaderUnload(tmp->private);
	xf86loaderfree(tmp->name);
	xf86loaderfree(tmp->cname);
	xf86loaderfree(tmp);
  }
  
  freeHandles[handle] = HANDLE_FREE ;

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
void _loader_debug_state()
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

