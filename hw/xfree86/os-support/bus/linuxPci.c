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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "Pci.h"
#include <dirent.h>

/*
 * linux platform specific PCI access functions -- using /proc/bus/pci
 * needs kernel version 2.2.x
 */
static CARD32 linuxPciCfgRead(PCITAG tag, int off);
static void linuxPciCfgWrite(PCITAG, int off, CARD32 val);
static void linuxPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits);
static ADDRESS linuxTransAddrBusToHost(PCITAG tag, PciAddrType type, ADDRESS addr);
#if defined(__powerpc__)
static ADDRESS linuxPpcBusAddrToHostAddr(PCITAG, PciAddrType, ADDRESS);
static ADDRESS linuxPpcHostAddrToBusAddr(PCITAG, PciAddrType, ADDRESS);
#endif

static CARD8 linuxPciCfgReadByte(PCITAG tag, int off);
static void linuxPciCfgWriteByte(PCITAG tag, int off, CARD8 val);
static CARD16 linuxPciCfgReadWord(PCITAG tag, int off);
static void linuxPciCfgWriteWord(PCITAG tag, int off, CARD16 val);
static int linuxPciHandleBIOS(PCITAG Tag, int basereg, unsigned char *buf, int len);
static Bool linuxDomainSupport(void);

static pciBusFuncs_t linuxFuncs0 = {
/* pciReadLong      */	linuxPciCfgRead,
/* pciWriteLong     */	linuxPciCfgWrite,
/* pciSetBitsLong   */	linuxPciCfgSetBits,
#if defined(__powerpc__)
/* pciAddrHostToBus */	linuxPpcHostAddrToBusAddr,
/* pciAddrBusToHost */	linuxPpcBusAddrToHostAddr,
#else
/* pciAddrHostToBus */	pciAddrNOOP,
/* linuxTransAddrBusToHost is busted on sparc64 but the PCI rework tree
 * makes it all moot, so we kludge it for now */
#if defined(__sparc__)
/* pciAddrBusToHost */  pciAddrNOOP,
#else
/* pciAddrBusToHost */	linuxTransAddrBusToHost,
#endif /* __sparc64__ */
#endif

/* pciControlBridge */		NULL,
/* pciGetBridgeBuses */		NULL,
/* pciGetBridgeResources */	NULL,

/* pciReadByte */	linuxPciCfgReadByte,
/* pciWriteByte */	linuxPciCfgWriteByte,

/* pciReadWord */	linuxPciCfgReadWord,
/* pciWriteWord */	linuxPciCfgWriteWord,
};

static pciBusInfo_t linuxPci0 = {
/* configMech  */	PCI_CFG_MECH_OTHER,
/* numDevices  */	32,
/* secondary   */	FALSE,
/* primary_bus */	0,
/* funcs       */	&linuxFuncs0,
/* pciBusPriv  */	NULL,
/* bridge      */	NULL
};

/* from lnx_pci.c. */
extern int lnxPciInit(void);

static Bool	domain_support = FALSE;

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
#ifndef INCLUDE_XF86_NO_DOMAIN
	domain_support = linuxDomainSupport();
#endif
	pciNumBuses    = 1;
	pciBusInfo[0]  = &linuxPci0;
	pciFindFirstFP = pciGenFindFirst;
	pciFindNextFP  = pciGenFindNext;
	pciSetOSBIOSPtr(linuxPciHandleBIOS);
        xf86MaxPciDevs = lnxPciInit();
}

static int
linuxPciOpenFile(PCITAG tag, Bool write)
{
	static int	ldomain, lbus,ldev,lfunc,fd = -1,is_write = 0;
	int		domain, bus, dev, func;
	char		file[64];
	struct stat	ignored;
	static int is26 = -1;

	domain = PCI_DOM_FROM_TAG(tag);
	bus  = PCI_BUS_NO_DOMAIN(PCI_BUS_FROM_TAG(tag));
	dev  = PCI_DEV_FROM_TAG(tag);
	func = PCI_FUNC_FROM_TAG(tag);
	if (is26 == -1) {
		if (stat("/sys/bus/pci",&ignored) < 0)
			is26 = 0;
		else
			is26 = 1;
	}
	
	if (!domain_support && domain > 0)
	    return -1;

	if (fd == -1 || (write && (!is_write)) || domain != ldomain
	    || bus != lbus || dev != ldev || func != lfunc) {
		if (fd != -1) {
			close(fd);
			fd = -1;
		}
		if (is26)
			sprintf(file,"/sys/bus/pci/devices/%04x:%02x:%02x.%01x/config",
				domain, bus, dev, func);
		else {
			if (bus < 256) {
				sprintf(file, "/proc/bus/pci/%04x:%02x", domain, bus);
				if (stat(file, &ignored) < 0) {
					if (domain == 0) 
						sprintf(file, "/proc/bus/pci/%02x/%02x.%1x",
							bus, dev, func);
					else
						goto bail;
				} else
					sprintf(file, "/proc/bus/pci/%04x:%02x/%02x.%1x",
						domain, bus, dev, func);
			} else {
				sprintf(file, "/proc/bus/pci/%04x:%04x", domain, bus);
				if (stat(file, &ignored) < 0) {
					if (domain == 0)
						sprintf(file, "/proc/bus/pci/%04x/%02x.%1x",
							bus, dev, func);
					else
						goto bail;
				} else
					sprintf(file, "/proc/bus/pci/%04x:%04x/%02x.%1x",
						domain, bus, dev, func);
			}
		}
		if (write) {
		    fd = open(file,O_RDWR);
		    if (fd != -1) is_write = TRUE;
		} else switch (is_write) {
			case TRUE:
			    fd = open(file,O_RDWR);
			    if (fd > -1)
				break;
			default:
			    fd = open(file,O_RDONLY);
			    is_write = FALSE;
		}
	bail:
		ldomain = domain;
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

	if (-1 != (fd = linuxPciOpenFile(tag,FALSE))) {
		lseek(fd,off,SEEK_SET);
		read(fd,&val,4);
	}
	return PCI_CPU(val);
}

static void
linuxPciCfgWrite(PCITAG tag, int off, CARD32 val)
{
	int	fd;

	if (-1 != (fd = linuxPciOpenFile(tag,TRUE))) {
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

	if (-1 != (fd = linuxPciOpenFile(tag,TRUE))) {
		lseek(fd,off,SEEK_SET);
		read(fd,&val,4);
		val = PCI_CPU(val);
		val = (val & ~mask) | (bits & mask);
		val = PCI_CPU(val);
		lseek(fd,off,SEEK_SET);
		write(fd,&val,4);
	}
}

/*
 * This function will convert a BAR address into a host address
 * suitable for passing into the mmap function of a /proc/bus
 * device.
 */
ADDRESS linuxTransAddrBusToHost(PCITAG tag, PciAddrType type, ADDRESS addr)
{
    ADDRESS ret = xf86GetOSOffsetFromPCI(tag, PCI_MEM|PCI_IO, addr);

    if (ret)
	return ret;

    /*
     * if it is not a BAR address, it must be legacy, (or wrong)
     * return it as is..
     */
    return addr;
}


#if defined(__powerpc__)

#ifndef __NR_pciconfig_iobase
#define __NR_pciconfig_iobase   200
#endif

static ADDRESS
linuxPpcBusAddrToHostAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
    if (type == PCI_MEM)
    {
	ADDRESS membase = syscall(__NR_pciconfig_iobase, 1,
		    PCI_BUS_FROM_TAG(tag), PCI_DFN_FROM_TAG(tag));
	return (addr + membase);
    }
    else if (type == PCI_IO)
    {
	ADDRESS iobase = syscall(__NR_pciconfig_iobase, 2,
		    PCI_BUS_FROM_TAG(tag), PCI_DFN_FROM_TAG(tag));
	return (addr + iobase);
    }
    else return addr;
}

static ADDRESS
linuxPpcHostAddrToBusAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
    if (type == PCI_MEM)
    {
	ADDRESS membase = syscall(__NR_pciconfig_iobase, 1,
		    PCI_BUS_FROM_TAG(tag), PCI_DFN_FROM_TAG(tag));
	return (addr - membase);
    }
    else if (type == PCI_IO)
    {
	ADDRESS iobase = syscall(__NR_pciconfig_iobase, 2,
		    PCI_BUS_FROM_TAG(tag), PCI_DFN_FROM_TAG(tag));
	return (addr - iobase);
    }
    else return addr;
}

#endif /* __powerpc__ */

static CARD8
linuxPciCfgReadByte(PCITAG tag, int off)
{
	int	fd;
	CARD8	val = 0xff;

	if (-1 != (fd = linuxPciOpenFile(tag,FALSE))) {
		lseek(fd,off,SEEK_SET);
		read(fd,&val,1);
	}

	return val;
}

static void
linuxPciCfgWriteByte(PCITAG tag, int off, CARD8 val)
{
	int	fd;

	if (-1 != (fd = linuxPciOpenFile(tag,TRUE))) {
		lseek(fd,off,SEEK_SET);
		write(fd, &val, 1);
	}
}

static CARD16
linuxPciCfgReadWord(PCITAG tag, int off)
{
	int	fd;
	CARD16	val = 0xff;

	if (-1 != (fd = linuxPciOpenFile(tag,FALSE))) {
		lseek(fd, off, SEEK_SET);
		read(fd, &val, 2);
	}

	return PCI_CPU16(val);
}

static void
linuxPciCfgWriteWord(PCITAG tag, int off, CARD16 val)
{
	int	fd;

	if (-1 != (fd = linuxPciOpenFile(tag,TRUE))) {
		lseek(fd, off, SEEK_SET);
		val = PCI_CPU16(val);
		write(fd, &val, 2);
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
static const struct pciSizes {
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

static const struct pciSizes *
linuxGetSizesStruct(PCITAG Tag)
{
    static const struct pciSizes default_size = {
	0, 0, 1U << 16, (unsigned long)(1ULL << 32)
    };
    pciConfigPtr pPCI;
    int          i;

    /* Find host bridge */
    if ((pPCI = xf86GetPciHostConfigFromTag(Tag))) {
	/* Look up vendor/device */
	for (i = 0;  i < NUM_SIZES;  i++) {
	    if ((pPCI->pci_vendor == pciControllerSizes[i].vendor)
		&& (pPCI->pci_device == pciControllerSizes[i].device)) {
		return & pciControllerSizes[i];
	    }
	}
    }

    /* Default to 64KB I/O and 4GB memory. */
    return & default_size;
}

static __inline__ unsigned long
linuxGetIOSize(PCITAG Tag)
{
    const struct pciSizes * const sizes = linuxGetSizesStruct(Tag);
    return sizes->io_size;
}

static __inline__ void
linuxGetSizes(PCITAG Tag, unsigned long *io_size, unsigned long *mem_size)
{
    const struct pciSizes * const sizes = linuxGetSizesStruct(Tag);

    *io_size  = sizes->io_size;
    *mem_size = sizes->mem_size;
}

static Bool
linuxDomainSupport(void)
{
    DIR *dir;
    struct dirent *dirent;
    char *end;

    if (!(dir = opendir("/proc/bus/pci")))
       return FALSE;
    while (1) {
	if (!(dirent = readdir(dir)))
	    return FALSE;
	strtol(dirent->d_name,&end,16);
	/* entry of the form xx or xxxx : x=[0..f] no domain */
	if (*end == '\0')
	    return FALSE;
	else if (*end == ':') {
	    /* ':' found immediately after: verify for xxxx:xx or xxxx:xxxx */
	    strtol(end + 1,&end,16);
	    if (*end == '\0')
		return TRUE;
	}
    }
    return FALSE;
} 

_X_EXPORT int
xf86GetPciDomain(PCITAG Tag)
{
    pciConfigPtr pPCI;
    int fd, result;

    pPCI = xf86GetPciHostConfigFromTag(Tag);

    if (pPCI && (result = PCI_DOM_FROM_BUS(pPCI->busnum)))
	return result + 1;

    if (!pPCI || pPCI->fakeDevice)
	return 1;		/* Domain 0 is reserved */

    if ((fd = linuxPciOpenFile(pPCI ? pPCI->tag : 0,FALSE)) < 0)
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

       prot = ((Flags & VIDMEM_READONLY) == 0);
       if (((fd = linuxPciOpenFile(Tag, prot)) < 0) ||
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
	    mmapflags = MAP_SHARED | MAP_NONCACHED;

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
	    return NULL;

	xf86MakeNewMapping(ScreenNum, Flags, realBase, Size + Offset, result);

	return result + Offset;
    } while (0);

    if (mmap_ioctl == PCIIOC_MMAP_IS_MEM)
	return xf86MapVidMem(ScreenNum, Flags, Base, Size);

    return NULL;
}

#define MAX_DOMAINS 257
static pointer DomainMmappedIO[MAX_DOMAINS];

static int
linuxOpenLegacy(PCITAG Tag, char *name)
{
#define PREFIX "/sys/class/pci_bus/%04x:%02x/%s"
    char *path;
    int domain, bus;
    pciBusInfo_t *pBusInfo;
    pciConfigPtr bridge = NULL;
    int fd;

    path = xalloc(strlen(PREFIX) + strlen(name));
    if (!path)
	return -1;

    for (;;) {
	domain = xf86GetPciDomain(Tag);
	bus = PCI_BUS_NO_DOMAIN(PCI_BUS_FROM_TAG(Tag));

	/* Domain 0 is reserved -- see xf86GetPciDomain() */
	if ((domain <= 0) || (domain >= MAX_DOMAINS))
	    FatalError("linuxOpenLegacy():  domain out of range\n");

	sprintf(path, PREFIX, domain - 1, bus, name);
	fd = open(path, O_RDWR);
	if (fd >= 0) {
	    xfree(path);
	    return fd;
	}

	pBusInfo = pciBusInfo[PCI_BUS_FROM_TAG(Tag)];
	if (!pBusInfo || (bridge == pBusInfo->bridge) ||
		!(bridge = pBusInfo->bridge)) {
	    xfree(path);
	    return -1;
	}

	Tag = bridge->tag;
    }

    xfree(path);
    return fd;
}

/*
 * xf86MapDomainMemory - memory map PCI domain memory
 *
 * This routine maps the memory region in the domain specified by Tag and
 * returns a pointer to it.  The pointer is saved for future use if it's in
 * the legacy ISA memory space (memory in a domain between 0 and 1MB).
 */
_X_EXPORT pointer
xf86MapDomainMemory(int ScreenNum, int Flags, PCITAG Tag,
		    ADDRESS Base, unsigned long Size)
{
    int domain = xf86GetPciDomain(Tag);
    int fd = -1;
    pointer addr;

    /*
     * We use /proc/bus/pci on non-legacy addresses or if the Linux sysfs
     * legacy_mem interface is unavailable.
     */
    if (Base >= 1024*1024)
	addr = linuxMapPci(ScreenNum, Flags, Tag, Base, Size,
			   PCIIOC_MMAP_IS_MEM);
    else if ((fd = linuxOpenLegacy(Tag, "legacy_mem")) < 0)
	addr = linuxMapPci(ScreenNum, Flags, Tag, Base, Size,
			   PCIIOC_MMAP_IS_MEM);
    else
	addr = mmap(NULL, Size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, Base);

    if (fd >= 0)
	close(fd);
    if (addr == NULL || addr == MAP_FAILED) {
	perror("mmap failure");
	FatalError("xf86MapDomainMem():  mmap() failure\n");
    }
    return addr;
}

/*
 * xf86MapDomainIO - map I/O space in this domain
 *
 * Each domain has a legacy ISA I/O space.  This routine will try to
 * map it using the Linux sysfs legacy_io interface.  If that fails,
 * it'll fall back to using /proc/bus/pci.
 *
 * If the legacy_io interface *does* exist, the file descriptor (fd below)
 * will be saved in the DomainMmappedIO array in the upper bits of the
 * pointer.  Callers will do I/O with small port numbers (<64k values), so
 * the platform I/O code can extract the port number and the fd, lseek to
 * the port number in the legacy_io file, and issue the read or write.
 *
 * This has no means of returning failure, so all errors are fatal
 */
_X_EXPORT IOADDRESS
xf86MapDomainIO(int ScreenNum, int Flags, PCITAG Tag,
		IOADDRESS Base, unsigned long Size)
{
    int domain = xf86GetPciDomain(Tag);
    int fd;

    if ((domain <= 0) || (domain >= MAX_DOMAINS))
	FatalError("xf86MapDomainIO():  domain out of range\n");

    if (DomainMmappedIO[domain])
	return (IOADDRESS)DomainMmappedIO[domain] + Base;

    /* Permanently map all of I/O space */
    if ((fd = linuxOpenLegacy(Tag, "legacy_io")) < 0) {
	    DomainMmappedIO[domain] = linuxMapPci(ScreenNum, Flags, Tag,
						  0, linuxGetIOSize(Tag),
						  PCIIOC_MMAP_IS_IO);
	    /* ia64 can't mmap legacy IO port space */
	    if (!DomainMmappedIO[domain])
		return Base;
    }
    else { /* legacy_io file exists, encode fd */
	DomainMmappedIO[domain] = (pointer)(fd << 24);
    }

    return (IOADDRESS)DomainMmappedIO[domain] + Base;
}

/*
 * xf86ReadDomainMemory - copy from domain memory into a caller supplied buffer
 */
_X_EXPORT int
xf86ReadDomainMemory(PCITAG Tag, ADDRESS Base, int Len, unsigned char *Buf)
{
    unsigned char *ptr, *src;
    ADDRESS offset;
    unsigned long size;
    int len, pagemask = getpagesize() - 1;

    unsigned int i, dom, bus, dev, func;
    unsigned int fd;
    char file[256];
    struct stat st;

    dom  = PCI_DOM_FROM_TAG(Tag);
    bus  = PCI_BUS_NO_DOMAIN(PCI_BUS_FROM_TAG(Tag));
    dev  = PCI_DEV_FROM_TAG(Tag);
    func = PCI_FUNC_FROM_TAG(Tag);
    sprintf(file, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/rom",
	    dom, bus, dev, func);

    /*
     * If the caller wants the ROM and the sysfs rom interface exists,
     * try to use it instead of reading it from /proc/bus/pci.
     */
    if (((Base & 0xfffff) == 0xC0000) && (stat(file, &st) == 0)) {
        if ((fd = open(file, O_RDWR)))
            Base = 0x0;

	/* enable the ROM first */
	write(fd, "1", 2);
	lseek(fd, 0, SEEK_SET);

    len = min(Len, st.st_size);

        /* copy the ROM until we hit Len, EOF or read error */
        for (; len && (size = read(fd, Buf, len)) > 0 ; Buf+=size, len-=size)
            ;

	write(fd, "0", 2);
	close(fd);

	return Len;
    }

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

int linuxPciHandleBIOS(PCITAG Tag, int basereg, unsigned char *buf, int len)
{
  unsigned int dom, bus, dev, func;
  unsigned int fd;
  char file[256];
  struct stat st;
  int ret;
  int sofar = 0;

  dom  = PCI_DOM_FROM_TAG(Tag);
  bus  = PCI_BUS_NO_DOMAIN(PCI_BUS_FROM_TAG(Tag));
  dev  = PCI_DEV_FROM_TAG(Tag);
  func = PCI_FUNC_FROM_TAG(Tag);
  sprintf(file, "/sys/bus/pci/devices/%04x:%02x:%02x.%1x/rom",
	  dom, bus, dev, func);

  if (stat(file, &st) == 0)
  {
    if ((fd = open(file, O_RDWR)))
      basereg = 0x0;
    
    /* enable the ROM first */
    write(fd, "1", 2);
    lseek(fd, 0, SEEK_SET);
    do {
        /* copy the ROM until we hit Len, EOF or read error */
    	ret = read(fd, buf+sofar, len-sofar);
    	if (ret <= 0)
		break;
	sofar += ret;
    } while (sofar < len);
    
    write(fd, "0", 2);
    close(fd);
    if (sofar < len)
    	xf86MsgVerb(X_INFO, 3, "Attempted to read BIOS %dKB from %s: got %dKB\n", len/1024, file, sofar/1024);
    return sofar;
  }
  return 0;
}

#ifdef __ia64__
static PCITAG ia64linuxPciFindFirst(void);
static PCITAG ia64linuxPciFindNext(void);

void   
ia64linuxPciInit()
{
    struct stat st;

    linuxPciInit();
	   
    if (!stat("/proc/sgi_sn/licenseID", &st) && pciNumBuses) {
       /* Be a little paranoid here and only use this code for Altix systems.
	* It is generic, so it should work on any system, but depends on
	* /proc/bus/pci entries for each domain/bus combination. Altix is
	* guaranteed a recent enough kernel to have them.
	*/
       pciFindFirstFP = ia64linuxPciFindFirst;
       pciFindNextFP  = ia64linuxPciFindNext;
    }
}

static DIR *busdomdir;
static DIR *devdir;
	       
static PCITAG
ia64linuxPciFindFirst(void)
{   
       busdomdir = opendir("/proc/bus/pci");
       devdir = NULL;

       return ia64linuxPciFindNext();
}   

static struct dirent *getnextbus(int *domain, int *bus)
{
    struct dirent *entry;
    int dombus;

    for (;;) {
	entry = readdir(busdomdir);
	if (entry == NULL) {
	    *domain = 0;
	    *bus = 0;
	    closedir(busdomdir);
	    return NULL;
	}
	if (sscanf(entry->d_name, "%04x:%02x", domain, bus) != 2)
	    continue;
	dombus = PCI_MAKE_BUS(*domain, *bus);

	if (pciNumBuses <= dombus)
	    pciNumBuses = dombus + 1;
	if (!pciBusInfo[dombus]) {
	    pciBusInfo[dombus] = xnfalloc(sizeof(pciBusInfo_t));
	    *pciBusInfo[dombus] = *pciBusInfo[0];
	}

	return entry;
    }
}

static PCITAG
ia64linuxPciFindNext(void)
{
    struct dirent *entry;
    char file[40];
    static int bus, dev, func, domain;
    PCITAG pciDeviceTag;
    CARD32 devid;

    for (;;) {
	if (devdir == NULL) {
	    entry = getnextbus(&domain, &bus);
	    if (!entry)
		return PCI_NOT_FOUND;
	    snprintf(file, 40, "/proc/bus/pci/%s", entry->d_name);
	    devdir = opendir(file);
	    if (!devdir)
		return PCI_NOT_FOUND;

	}

	entry = readdir(devdir);

	if (entry == NULL) {
	    closedir(devdir);
	    devdir = NULL;
	    continue;
	}

	if (sscanf(entry->d_name, "%02x . %01x", &dev, &func) == 2) {
	    CARD32 tmp;
	    int sec_bus, pri_bus;
	    unsigned char base_class, sub_class;

	    int pciBusNum = PCI_MAKE_BUS(domain, bus);
	    pciDeviceTag = PCI_MAKE_TAG(pciBusNum, dev, func);

	    /*
	     * Before checking for a specific devid, look for enabled
	     * PCI to PCI bridge devices.  If one is found, create and
	     * initialize a bus info record (if one does not already exist).
	     */
	    tmp = pciReadLong(pciDeviceTag, PCI_CLASS_REG);
	    base_class = PCI_CLASS_EXTRACT(tmp);
	    sub_class = PCI_SUBCLASS_EXTRACT(tmp);
	    if ((base_class == PCI_CLASS_BRIDGE) &&
		((sub_class == PCI_SUBCLASS_BRIDGE_PCI) ||
		 (sub_class == PCI_SUBCLASS_BRIDGE_CARDBUS))) {
		tmp = pciReadLong(pciDeviceTag, PCI_PCI_BRIDGE_BUS_REG);
		sec_bus = PCI_SECONDARY_BUS_EXTRACT(tmp, pciDeviceTag);
		pri_bus = PCI_PRIMARY_BUS_EXTRACT(tmp, pciDeviceTag);
#ifdef DEBUGPCI
		ErrorF("ia64linuxPciFindNext: pri_bus %d sec_bus %d\n",
		       pri_bus, sec_bus);
#endif
		if (pciBusNum != pri_bus) {
		    /* Some bridges do not implement the primary bus register */
		    if ((PCI_BUS_NO_DOMAIN(pri_bus) != 0) ||
			(sub_class != PCI_SUBCLASS_BRIDGE_CARDBUS))
			xf86Msg(X_WARNING,
				"ia64linuxPciFindNext:  primary bus mismatch on PCI"
				" bridge 0x%08lx (0x%02x, 0x%02x)\n",
				pciDeviceTag, pciBusNum, pri_bus);
		    pri_bus = pciBusNum;
	        }
		if ((pri_bus < sec_bus) && (sec_bus < pciMaxBusNum) &&
		    pciBusInfo[pri_bus]) {
		    /*
		     * Found a secondary PCI bus
		     */
		    if (!pciBusInfo[sec_bus]) {
			pciBusInfo[sec_bus] = xnfalloc(sizeof(pciBusInfo_t));

			/* Copy parents settings... */
			*pciBusInfo[sec_bus] = *pciBusInfo[pri_bus];
		    }

		    /* ...but not everything same as parent */
		    pciBusInfo[sec_bus]->primary_bus = pri_bus;
		    pciBusInfo[sec_bus]->secondary = TRUE;
		    pciBusInfo[sec_bus]->numDevices = 32;

		    if (pciNumBuses <= sec_bus)
			pciNumBuses = sec_bus + 1;
		}
	    }

	    devid = pciReadLong(pciDeviceTag, PCI_ID_REG);
	    if ((devid & pciDevidMask) == pciDevid)
		/* Yes - Return it.  Otherwise, next device */
		return pciDeviceTag;
	}
    }
}
#endif

