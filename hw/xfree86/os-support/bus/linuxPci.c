/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/linuxPci.c,v 1.10 2002/11/17 18:42:01 alanh Exp $ */
/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <stdio.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "Pci.h"

/*
 * linux platform specific PCI access functions -- using /proc/bus/pci
 * needs kernel version 2.2.x
 */
static CARD32 linuxPciCfgRead(PCITAG tag, int off);
static void linuxPciCfgWrite(PCITAG, int off, CARD32 val);
static void linuxPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits);

static pciBusFuncs_t linuxFuncs0 = {
/* pciReadLong      */	linuxPciCfgRead,
/* pciWriteLong     */	linuxPciCfgWrite,
/* pciSetBitsLong   */	linuxPciCfgSetBits,
/* pciAddrHostToBus */	pciAddrNOOP,
/* pciAddrBusToHost */	pciAddrNOOP
};

static pciBusInfo_t linuxPci0 = {
/* configMech  */	PCI_CFG_MECH_OTHER,
/* numDevices  */	32,
/* secondary   */	FALSE,
/* primary_bus */	0,
#ifdef PowerMAX_OS
/* ppc_io_base */	0,
/* ppc_io_size */	0,
#endif
/* funcs       */	&linuxFuncs0,
/* pciBusPriv  */	NULL,
/* bridge      */	NULL
};

void
linuxPciInit()
{
	struct stat st;
	if ((xf86Info.pciFlags == PCIForceNone) ||
	    (-1 == stat("/proc/bus/pci", &st))) {
		/* when using this as default for all linux architectures,
		   we'll need a fallback for 2.0 kernels here */
		return;
	}
	pciNumBuses    = 1;
	pciBusInfo[0]  = &linuxPci0;
	pciFindFirstFP = pciGenFindFirst;
	pciFindNextFP  = pciGenFindNext;
}

static int
linuxPciOpenFile(PCITAG tag)
{
	static int	lbus,ldev,lfunc,fd = -1;
	int		bus, dev, func;
	char		file[32];

	bus  = PCI_BUS_FROM_TAG(tag);
	dev  = PCI_DEV_FROM_TAG(tag);
	func = PCI_FUNC_FROM_TAG(tag);
	if (fd == -1 || bus != lbus || dev != ldev || func != lfunc) {
		if (fd != -1)
			close(fd);
		if (bus < 256)
			sprintf(file, "/proc/bus/pci/%02x/%02x.%1x",
				bus, dev, func);
		else
			sprintf(file, "/proc/bus/pci/%04x/%02x.%1x",
				bus, dev, func);
		fd = open(file,O_RDWR);
		lbus  = bus;
		ldev  = dev;
		lfunc = func;
	}
	return fd;
}

static CARD32
linuxPciCfgRead(PCITAG tag, int off)
{
	int	fd;
	CARD32	val = 0xffffffff;

	if (-1 != (fd = linuxPciOpenFile(tag))) {
		lseek(fd,off,SEEK_SET);
		read(fd,&val,4);
	}
	return PCI_CPU(val);
}

static void
linuxPciCfgWrite(PCITAG tag, int off, CARD32 val)
{
	int	fd;

	if (-1 != (fd = linuxPciOpenFile(tag))) {
		lseek(fd,off,SEEK_SET);
		val = PCI_CPU(val);
		write(fd,&val,4);
	}
}

static void
linuxPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits)
{
	int	fd;
	CARD32	val = 0xffffffff;

	if (-1 != (fd = linuxPciOpenFile(tag))) {
		lseek(fd,off,SEEK_SET);
		read(fd,&val,4);
		val = PCI_CPU(val);
		val = (val & ~mask) | (bits & mask);
		val = PCI_CPU(val);
		lseek(fd,off,SEEK_SET);
		write(fd,&val,4);
	}
}

#ifndef INCLUDE_XF86_NO_DOMAIN

/*
 * Compiling the following simply requires the presence of <linux/pci.c>.
 * Actually running this is another matter altogether...
 *
 * This scheme requires that the kernel allow mmap()'ing of a host bridge's I/O
 * and memory spaces through its /proc/bus/pci/BUS/DFN entry.  Which one is
 * determined by a prior ioctl().
 *
 * For the sparc64 port, this means 2.4.12 or later.  For ppc, this
 * functionality is almost, but not quite there yet.  Alpha and other kernel
 * ports to multi-domain architectures still need to implement this.
 *
 * This scheme is also predicated on the use of an IOADDRESS compatible type to
 * designate I/O addresses.  Although IOADDRESS is defined as an unsigned
 * integral type, it is actually the virtual address of, i.e. a pointer to, the
 * I/O port to access.  And so, the inX/outX macros in "compiler.h" need to be
 * #define'd appropriately (as is done on SPARC's).
 *
 * Another requirement to port this scheme to another multi-domain architecture
 * is to add the appropriate entries in the pciControllerSizes array below.
 *
 * TO DO:  Address the deleterious reaction some host bridges have to master
 *         aborts.  This is already done for secondary PCI buses, but not yet
 *         for accesses to primary buses (except for the SPARC port, where
 *         master aborts are avoided during PCI scans).
 */

#include <linux/pci.h>

#ifndef PCIIOC_BASE		/* Ioctls for /proc/bus/pci/X/Y nodes. */
#define PCIIOC_BASE		('P' << 24 | 'C' << 16 | 'I' << 8)

/* Get controller for PCI device. */
#define PCIIOC_CONTROLLER	(PCIIOC_BASE | 0x00)
/* Set mmap state to I/O space. */
#define PCIIOC_MMAP_IS_IO	(PCIIOC_BASE | 0x01)
/* Set mmap state to MEM space. */
#define PCIIOC_MMAP_IS_MEM	(PCIIOC_BASE | 0x02)
/* Enable/disable write-combining. */
#define PCIIOC_WRITE_COMBINE	(PCIIOC_BASE | 0x03)

#endif

/* This probably shouldn't be Linux-specific */
static pciConfigPtr
xf86GetPciHostConfigFromTag(PCITAG Tag)
{
    int bus = PCI_BUS_FROM_TAG(Tag);
    pciBusInfo_t *pBusInfo;

    while ((bus < pciNumBuses) && (pBusInfo = pciBusInfo[bus])) {
	if (bus == pBusInfo->primary_bus)
	    return pBusInfo->bridge;
	bus = pBusInfo->primary_bus;
    }

    return NULL;	/* Bad data */
}

/*
 * This is ugly, but until I can extract this information from the kernel,
 * it'll have to do.  The default I/O space size is 64K, and 4G for memory.
 * Anything else needs to go in this table.  (PowerPC folk take note.)
 *
 * Note that Linux/SPARC userland is 32-bit, so 4G overflows to zero here.
 *
 * Please keep this table in ascending vendor/device order.
 */
static struct pciSizes {
    unsigned short vendor, device;
    unsigned long io_size, mem_size;
} pciControllerSizes[] = {
    {
	PCI_VENDOR_SUN, PCI_CHIP_PSYCHO,
	1U << 16, 1U << 31
    },
    {
	PCI_VENDOR_SUN, PCI_CHIP_SCHIZO,
	1U << 24, 1U << 31	/* ??? */
    },
    {
	PCI_VENDOR_SUN, PCI_CHIP_SABRE,
	1U << 24, (unsigned long)(1ULL << 32)
    },
    {
	PCI_VENDOR_SUN, PCI_CHIP_HUMMINGBIRD,
	1U << 24, (unsigned long)(1ULL << 32)
    }
};
#define NUM_SIZES (sizeof(pciControllerSizes) / sizeof(pciControllerSizes[0]))

static unsigned long
linuxGetIOSize(PCITAG Tag)
{
    pciConfigPtr pPCI;
    int          i;

    /* Find host bridge */
    if ((pPCI = xf86GetPciHostConfigFromTag(Tag))) {
	/* Look up vendor/device */
	for (i = 0;  i < NUM_SIZES;  i++) {
	    if (pPCI->pci_vendor > pciControllerSizes[i].vendor)
		continue;
	    if (pPCI->pci_vendor < pciControllerSizes[i].vendor)
		break;
	    if (pPCI->pci_device > pciControllerSizes[i].device)
		continue;
	    if (pPCI->pci_device < pciControllerSizes[i].device)
		break;
	    return pciControllerSizes[i].io_size;
	}
    }

    return 1U << 16;			/* Default to 64K */
}

static void
linuxGetSizes(PCITAG Tag, unsigned long *io_size, unsigned long *mem_size)
{
    pciConfigPtr pPCI;
    int          i;

    *io_size  = (1U << 16);			/* Default to 64K */
    *mem_size = (unsigned long)(1ULL << 32);	/* Default to 4G */

    /* Find host bridge */
    if ((pPCI = xf86GetPciHostConfigFromTag(Tag))) {
	/* Look up vendor/device */
	for (i = 0;  i < NUM_SIZES;  i++) {
	    if (pPCI->pci_vendor > pciControllerSizes[i].vendor)
		continue;
	    if (pPCI->pci_vendor < pciControllerSizes[i].vendor)
		break;
	    if (pPCI->pci_device > pciControllerSizes[i].device)
		continue;
	    if (pPCI->pci_device < pciControllerSizes[i].device)
		break;
	    *io_size  = pciControllerSizes[i].io_size;
	    *mem_size = pciControllerSizes[i].mem_size;
	    break;
	}
    }
}

int
xf86GetPciDomain(PCITAG Tag)
{
    pciConfigPtr pPCI;
    int fd, result;

    pPCI = xf86GetPciHostConfigFromTag(Tag);

    if (pPCI && (result = PCI_DOM_FROM_BUS(pPCI->busnum)))
	return result;

    if ((fd = linuxPciOpenFile(pPCI ? pPCI->tag : 0)) < 0)
	return 0;

    if ((result = ioctl(fd, PCIIOC_CONTROLLER, 0)) < 0)
	return 0;

    return result + 1;		/* Domain 0 is reserved */
}

static pointer
linuxMapPci(int ScreenNum, int Flags, PCITAG Tag,
	    ADDRESS Base, unsigned long Size, int mmap_ioctl)
{
    do {
	pciConfigPtr pPCI;
	unsigned char *result;
	ADDRESS realBase, Offset;
	int fd, mmapflags, prot;

	xf86InitVidMem();

	pPCI = xf86GetPciHostConfigFromTag(Tag);

	if (((fd = linuxPciOpenFile(pPCI ? pPCI->tag : 0)) < 0) ||
	    (ioctl(fd, mmap_ioctl, 0) < 0))
	    break;

/* Note:  IA-64 doesn't compile this and doesn't need to */
#ifdef __ia64__

# ifndef  MAP_WRITECOMBINED
#  define MAP_WRITECOMBINED 0x00010000
# endif
# ifndef  MAP_NONCACHED
#  define MAP_NONCACHED     0x00020000
# endif

	if (Flags & VIDMEM_FRAMEBUFFER)
	    mmapflags = MAP_SHARED | MAP_WRITECOMBINED;
	else
	    mmapflags = MAP_SHARED | MAP_NONCACHED

#else /* !__ia64__ */

	mmapflags = (Flags & VIDMEM_FRAMEBUFFER) / VIDMEM_FRAMEBUFFER;

	if (ioctl(fd, PCIIOC_WRITE_COMBINE, mmapflags) < 0)
	    break;

	mmapflags = MAP_SHARED;

#endif /* ?__ia64__ */

	/* Align to page boundary */
	realBase = Base & ~(getpagesize() - 1);
	Offset = Base - realBase;

	if (Flags & VIDMEM_READONLY)
	    prot = PROT_READ;
	else
	    prot = PROT_READ | PROT_WRITE;

	result = mmap(NULL, Size + Offset, prot, mmapflags, fd, realBase);

	if (!result || ((pointer)result == MAP_FAILED))
	    FatalError("linuxMapPci() mmap failure:  %s\n", strerror(errno));

	xf86MakeNewMapping(ScreenNum, Flags, realBase, Size + Offset, result);

	return result + Offset;
    } while (0);

    if (mmap_ioctl == PCIIOC_MMAP_IS_MEM)
	return xf86MapVidMem(ScreenNum, Flags, Base, Size);

    return NULL;
}

pointer
xf86MapDomainMemory(int ScreenNum, int Flags, PCITAG Tag,
		    ADDRESS Base, unsigned long Size)
{
    return linuxMapPci(ScreenNum, Flags, Tag, Base, Size, PCIIOC_MMAP_IS_MEM);
}

#define MAX_DOMAINS 257
static pointer DomainMmappedIO[MAX_DOMAINS];

/* This has no means of returning failure, so all errors are fatal */
IOADDRESS
xf86MapDomainIO(int ScreenNum, int Flags, PCITAG Tag,
		IOADDRESS Base, unsigned long Size)
{
    int domain = xf86GetPciDomain(Tag);

    if ((domain <= 0) || (domain >= MAX_DOMAINS))
	FatalError("xf86MapDomainIO():  domain out of range\n");

    /* Permanently map all of I/O space */
    if (!DomainMmappedIO[domain]) {
	DomainMmappedIO[domain] = linuxMapPci(ScreenNum, Flags, Tag,
					      0, linuxGetIOSize(Tag),
					      PCIIOC_MMAP_IS_IO);
	if (!DomainMmappedIO[domain])
	    FatalError("xf86MapDomainIO():  mmap() failure\n");
    }

    return (IOADDRESS)DomainMmappedIO[domain] + Base;
}

int
xf86ReadDomainMemory(PCITAG Tag, ADDRESS Base, int Len, unsigned char *Buf)
{
    unsigned char *ptr, *src;
    ADDRESS offset;
    unsigned long size;
    int len, pagemask = getpagesize() - 1;

    /* Ensure page boundaries */
    offset = Base & ~pagemask;
    size = ((Base + Len + pagemask) & ~pagemask) - offset;

    ptr = xf86MapDomainMemory(-1, VIDMEM_READONLY, Tag, offset, size);

    if (!ptr)
	return -1;

    /* Using memcpy() here can hang the system */
    src = ptr + (Base - offset);
    for (len = Len;  len-- > 0;)
	*Buf++ = *src++;

    xf86UnMapVidMem(-1, ptr, size);

    return Len;
}

resPtr
xf86BusAccWindowsFromOS(void)
{
    pciConfigPtr  *ppPCI, pPCI;
    resPtr        pRes = NULL;
    resRange      range;
    unsigned long io_size, mem_size;
    int           domain;

    if ((ppPCI = xf86scanpci(0))) {
	for (;  (pPCI = *ppPCI);  ppPCI++) {
	    if ((pPCI->pci_base_class != PCI_CLASS_BRIDGE) ||
		(pPCI->pci_sub_class  != PCI_SUBCLASS_BRIDGE_HOST))
		continue;

	    domain = xf86GetPciDomain(pPCI->tag);
	    linuxGetSizes(pPCI->tag, &io_size, &mem_size);

	    RANGE(range, 0, (ADDRESS)(mem_size - 1),
		  RANGE_TYPE(ResExcMemBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);

	    RANGE(range, 0, (IOADDRESS)(io_size - 1),
		  RANGE_TYPE(ResExcIoBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);

	    if (domain <= 0)
		break;
	}
    }

    return pRes;
}

resPtr
xf86PciBusAccWindowsFromOS(void)
{
    pciConfigPtr  *ppPCI, pPCI;
    resPtr        pRes = NULL;
    resRange      range;
    unsigned long io_size, mem_size;
    int           domain;

    if ((ppPCI = xf86scanpci(0))) {
	for (;  (pPCI = *ppPCI);  ppPCI++) {
	    if ((pPCI->pci_base_class != PCI_CLASS_BRIDGE) ||
		(pPCI->pci_sub_class  != PCI_SUBCLASS_BRIDGE_HOST))
		continue;

	    domain = xf86GetPciDomain(pPCI->tag);
	    linuxGetSizes(pPCI->tag, &io_size, &mem_size);

	    RANGE(range, 0, (ADDRESS)(mem_size - 1),
		  RANGE_TYPE(ResExcMemBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);

	    RANGE(range, 0, (IOADDRESS)(io_size - 1),
		  RANGE_TYPE(ResExcIoBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);

	    if (domain <= 0)
		break;
	}
    }

    return pRes;
}


resPtr
xf86AccResFromOS(resPtr pRes)
{
    pciConfigPtr  *ppPCI, pPCI;
    resRange      range;
    unsigned long io_size, mem_size;
    int           domain;

    if ((ppPCI = xf86scanpci(0))) {
	for (;  (pPCI = *ppPCI);  ppPCI++) {
	    if ((pPCI->pci_base_class != PCI_CLASS_BRIDGE) ||
		(pPCI->pci_sub_class  != PCI_SUBCLASS_BRIDGE_HOST))
		continue;

	    domain = xf86GetPciDomain(pPCI->tag);
	    linuxGetSizes(pPCI->tag, &io_size, &mem_size);

	    /*
	     * At minimum, the top and bottom resources must be claimed, so
	     * that resources that are (or appear to be) unallocated can be
	     * relocated.
	     */
	    RANGE(range, 0x00000000u, 0x0009ffffu,
		  RANGE_TYPE(ResExcMemBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);
	    RANGE(range, 0x000c0000u, 0x000effffu,
		  RANGE_TYPE(ResExcMemBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);
	    RANGE(range, 0x000f0000u, 0x000fffffu,
		  RANGE_TYPE(ResExcMemBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);

	    RANGE(range, (ADDRESS)(mem_size - 1), (ADDRESS)(mem_size - 1),
		  RANGE_TYPE(ResExcMemBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);

	    RANGE(range, 0x00000000u, 0x00000000u,
		  RANGE_TYPE(ResExcIoBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);
	    RANGE(range, (IOADDRESS)(io_size - 1), (IOADDRESS)(io_size - 1),
		  RANGE_TYPE(ResExcIoBlock, domain));
	    pRes = xf86AddResToList(pRes, &range, -1);

	    if (domain <= 0)
		break;
	}
    }

    return pRes;
}

#endif /* !INCLUDE_XF86_NO_DOMAIN */
