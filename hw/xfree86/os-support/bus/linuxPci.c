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
#include "xf86_OSlib.h"
#include "Pci.h"

/**
 * \bug
 * The generation of the procfs file name for the domain != 0 case may not be 
 * correct.
 */
static int
linuxPciOpenFile(struct pci_device *dev, Bool write)
{
    static struct pci_device *last_dev = NULL;
    static int	fd = -1,is_write = 0;
    char		file[64];
    struct stat	ignored;
    static int is26 = -1;

    if (dev == NULL) {
	return -1;
    }

    if (is26 == -1) {
	is26 = (stat("/sys/bus/pci", &ignored) < 0) ? 0 : 1;
    }
	
    if (fd == -1 || (write && (!is_write)) || (last_dev != dev)) {
	if (fd != -1) {
	    close(fd);
	    fd = -1;
	}

	if (is26) {
	    sprintf(file,"/sys/bus/pci/devices/%04u:%02x:%02x.%01x/config",
		    dev->domain, dev->bus, dev->dev, dev->func);
	} else {
	    if (dev->domain == 0) {
		sprintf(file,"/proc/bus/pci/%02x", dev->bus);
		if (stat(file, &ignored) < 0) {
		    sprintf(file, "/proc/bus/pci/0000:%02x/%02x.%1x",
			    dev->bus, dev->dev, dev->func);
		} else {
		    sprintf(file, "/proc/bus/pci/%02x/%02x.%1x",
			    dev->bus, dev->dev, dev->func);
		}
	    } else {
		sprintf(file,"/proc/bus/pci/%02x%02x", dev->domain, dev->bus);
		if (stat(file, &ignored) < 0) {
		    sprintf(file, "/proc/bus/pci/%04x:%04x/%02x.%1x",
			    dev->domain, dev->bus, dev->dev, dev->func);
		} else {
		    sprintf(file, "/proc/bus/pci/%02x%02x/%02x.%1x",
			    dev->domain, dev->bus, dev->dev, dev->func);
		}
	    }
	}

	if (write) {
	    fd = open(file,O_RDWR);
	    if (fd != -1) is_write = TRUE;
	} else {
	    switch (is_write) {
	    case TRUE:
		fd = open(file,O_RDWR);
		if (fd > -1)
		    break;
	    default:
		fd = open(file,O_RDONLY);
		is_write = FALSE;
	    }
	}

	last_dev = dev;
    }

    return fd;
}

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

static pointer
linuxMapPci(int ScreenNum, int Flags, struct pci_device *dev,
	    ADDRESS Base, unsigned long Size, int mmap_ioctl)
{
    /* Align to page boundary */
    const ADDRESS realBase = Base & ~(getpagesize() - 1);
    const ADDRESS Offset = Base - realBase;

    do {
	unsigned char *result;
	int fd, mmapflags, prot;

	xf86InitVidMem();

	/* If dev is NULL, linuxPciOpenFile will return -1, and this routine
	 * will fail gracefully.
	 */
        prot = ((Flags & VIDMEM_READONLY) == 0);
        if (((fd = linuxPciOpenFile(dev, prot)) < 0) ||
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

static int
linuxOpenLegacy(struct pci_device *dev, char *name)
{
    static const char PREFIX[] = "/sys/class/pci_bus/%04x:%02x/%s";
    char path[sizeof(PREFIX) + 10];
    int fd = -1;

    while (dev != NULL) {
	snprintf(path, sizeof(path) - 1, PREFIX, dev->domain, dev->bus, name);
	fd = open(path, O_RDWR);
	if (fd >= 0) {
	    return fd;
	}

	dev = pci_device_get_parent_bridge(dev);
    }

    return fd;
}

/*
 * xf86MapDomainMemory - memory map PCI domain memory
 *
 * This routine maps the memory region in the domain specified by Tag and
 * returns a pointer to it.  The pointer is saved for future use if it's in
 * the legacy ISA memory space (memory in a domain between 0 and 1MB).
 */
pointer
xf86MapDomainMemory(int ScreenNum, int Flags, struct pci_device *dev,
		    ADDRESS Base, unsigned long Size)
{
    int fd = -1;
    pointer addr;

    /*
     * We use /proc/bus/pci on non-legacy addresses or if the Linux sysfs
     * legacy_mem interface is unavailable.
     */
    if ((Base > 1024*1024) || ((fd = linuxOpenLegacy(dev, "legacy_mem")) < 0))
	return linuxMapPci(ScreenNum, Flags, dev, Base, Size,
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
