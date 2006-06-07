/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/Pci.c,v 1.82 2004/01/16 15:39:04 tsi Exp $ */
/*
 * Pci.c - New server PCI access functions
 *
 * The XFree86 server PCI access functions have been reimplemented as a
 * framework that allows each supported platform/OS to have their own
 * platform/OS specific pci driver.
 *
 * All of the public PCI access functions exported to the other parts of
 * the server are declared in Pci.h and defined herein.  These include:
 *	pciInit()              - Initialize PCI access functions
 *	pciReadLong()          - Read a 32 bit value from a device's cfg space
 *	pciReadWord()          - Read a 16 bit value from a device's cfg space
 *	pciReadByte()          - Read an 8 bit value from a device's cfg space
 *	pciWriteLong()         - Write a 32 bit value to a device's cfg space
 *	pciWriteWord()         - Write a 16 bit value to a device's cfg space
 *	pciWriteByte()         - Write an 8 bit value to a device's cfg space
 *	pciSetBitsLong()       - Write a 32 bit value against a mask
 *	pciSetBitsByte()       - Write an 8 bit value against a mask
 *	pciTag()               - Return tag for a given PCI bus, device, &
 *                               function
 *	pciBusAddrToHostAddr() - Convert a PCI address to a host address
 *	pciHostAddrToBusAddr() - Convert a host address to a PCI address
 *	xf86scanpci()          - Return info about all PCI devices
 *	xf86GetPciDomain()     - Return domain number from a PCITAG
 *	xf86MapDomainMemory()  - Like xf86MapPciMem() but can handle
 *                               domain/host address translation
 *	xf86MapDomainIO()      - Maps PCI I/O spaces
 *	xf86ReadDomainMemory() - Like xf86ReadPciBIOS() but can handle
 *                               domain/host address translation
 *
 * The actual PCI backend driver is selected by the pciInit() function
 * (see below)  using either compile time definitions, run-time checks,
 * or both.
 *
 * Certain generic functions are provided that make the implementation
 * of certain well behaved platforms (e.g. those supporting PCI config
 * mechanism 1 or some thing close to it) very easy.
 *
 * Less well behaved platforms/OS's can roll their own functions.
 *
 * To add support for another platform/OS, add a call to fooPciInit() within
 * pciInit() below under the correct compile time definition or run-time
 * conditional.
 *
 * The fooPciInit() procedure must do three things:
 *	1) Initialize the pciBusTable[] for all primary PCI buses including
 *	   the per domain PCI access functions (readLong, writeLong,
 *	   addrBusToHost, and addrHostToBus).
 *
 *	2) Add entries to pciBusTable[] for configured secondary buses.  This
 *	   step may be skipped if a platform is using the generic findFirst/
 *	   findNext functions because these procedures will automatically
 *	   discover and add secondary buses dynamically.
 *
 *      3) Overide default settings for global PCI access functions if
 *	   required. These include pciFindFirstFP, pciFindNextFP,
 *	   Of course, if you choose not to use one of the generic
 *	   functions, you will need to provide a platform specifc replacement.
 *
 * Gary Barton
 * Concurrent Computer Corporation
 * garyb@gate.net
 *
 */

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
 *
 * This software is derived from the original XFree86 PCI code
 * which includes the following copyright notices as well:
 *
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * This code is also based heavily on the code in FreeBSD-current, which was
 * written by Wolfgang Stanglmeier, and contains the following copyright:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Copyright (c) 1999-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <errno.h>
#include <signal.h>
#include <X11/Xarch.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include "Pci.h"

#include <pciaccess.h>

#if 0
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#define PCI_MFDEV_SUPPORT   1 /* Include PCI multifunction device support */
#define PCI_BRIDGE_SUPPORT  1 /* Include support for PCI-to-PCI bridges */

/*
 * Global data
 */

pciBusInfo_t  *pciBusInfo[MAX_PCI_BUSES] = { NULL, };
_X_EXPORT int            pciNumBuses = 0;     /* Actual number of PCI buses */
int            pciMaxBusNum = MAX_PCI_BUSES;
static Bool    inProbe = FALSE;

static pciConfigPtr pci_devp[MAX_PCI_DEVICES + 1] = {NULL, };

static int readPciBios( PCITAG Tag, CARD8* tmp, ADDRESS hostbase,
			unsigned char * buf, int len, PciBiosType BiosType );


/*
 * pciInit - choose correct platform/OS specific PCI init routine
 */
static void
pciInit(void)
{
    static int pciInitialized = 0;

    if (!pciInitialized) {
	pciInitialized = 1;

	/* XXX */
#if defined(DEBUGPCI)
	if (DEBUGPCI >= xf86Verbose) {
	    xf86Verbose = DEBUGPCI;
	}
#endif

	ARCH_PCI_INIT();
#if defined(ARCH_PCI_OS_INIT)
	if (pciNumBuses <= 0) {
	    ARCH_PCI_OS_INIT();
	}
#endif
    }
}

_X_EXPORT ADDRESS
pciBusAddrToHostAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
  int bus = PCI_BUS_FROM_TAG(tag);

  pciInit();

  if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciAddrBusToHost)
	  return (*pciBusInfo[bus]->funcs->pciAddrBusToHost)(tag, type, addr);
  else
	  return(addr);
}

_X_EXPORT ADDRESS
pciHostAddrToBusAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
  int bus = PCI_BUS_FROM_TAG(tag);

  pciInit();

  if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciAddrHostToBus)
	  return (*pciBusInfo[bus]->funcs->pciAddrHostToBus)(tag, type, addr);
  else
	  return(addr);
}

_X_EXPORT PCITAG
pciTag(int busnum, int devnum, int funcnum)
{
	return(PCI_MAKE_TAG(busnum,devnum,funcnum));
}

CARD32
pciByteSwap(CARD32 u)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN

  return lswapl(u);

#else /* !BIG_ENDIAN */

  return(u);
    
#endif
}

ADDRESS
pciAddrNOOP(PCITAG tag, PciAddrType type, ADDRESS addr)
{
	return(addr);
}

_X_EXPORT pciConfigPtr *
xf86scanpci(int flags)
{
    pciConfigPtr devp;
    pciBusInfo_t *busp;
    int          idx = 0, i;
    struct pci_device * dev;
    struct pci_device_iterator * iter;
    static Bool  done = FALSE;

    /*
     * if we haven't found PCI devices checking for pci_devp may
     * result in an endless recursion if platform/OS specific PCI
     * bus probing code calls this function from with in it.
     */
    if (done || pci_devp[0])
	return pci_devp;

    done = TRUE;

    pci_system_init();
    pciInit();

#ifdef XF86SCANPCI_WRAPPER
    XF86SCANPCI_WRAPPER(SCANPCI_INIT);
#endif

    iter = pci_slot_match_iterator_create(NULL);
    dev = pci_device_next(iter);

    /* Check if no devices, return now */
    if (dev == NULL) {
#ifdef XF86SCANPCI_WRAPPER
	XF86SCANPCI_WRAPPER(SCANPCI_TERM);
#endif
	return NULL;
    }


    while ((idx < MAX_PCI_DEVICES) && (dev != NULL)) {
	pci_device_probe(dev);

	devp = xcalloc(1, sizeof(pciDevice));
	if (!devp) {
	    xf86Msg(X_ERROR,
		"xf86scanpci: Out of memory after %d devices!!\n", idx);
	    return (pciConfigPtr *)NULL;
	}

	/* Identify pci device by bus, dev, func, and tag */
	devp->dev = dev;
	devp->busnum = PCI_MAKE_BUS(dev->domain, dev->bus);
	devp->devnum = dev->dev;
	devp->funcnum = dev->func;
	devp->tag = PCI_MAKE_TAG(devp->busnum, dev->dev, dev->func);

	/* Read config space for this device */
	for (i = 0; i < 17; i++) { /* PCI hdr plus 1st dev spec dword */
	    pci_device_cfg_read_u32(dev, & devp->cfgspc.dwords[i],
				    i * sizeof(CARD32));
	}

	/* Some broken devices don't implement this field... */
	if (devp->pci_header_type == 0xff)
	    devp->pci_header_type = 0;

	switch (devp->pci_header_type & 0x7f) {
	case 0:
	    break;

	case 1:
	case 2: {
	    const uint8_t control = devp->pci_bridge_control &
		     ~(PCI_PCI_BRIDGE_MASTER_ABORT_EN |
		       PCI_PCI_BRIDGE_SECONDARY_RESET);

	    /* Allow master aborts to complete normally on secondary buses */
	    if (!(devp->pci_bridge_control & PCI_PCI_BRIDGE_MASTER_ABORT_EN))
		break;
	    pci_device_cfg_write_u8(dev, & control,
				    PCI_PCI_BRIDGE_CONTROL_REG);

	    break;
	}
	default:
	    break;
	}

	xf86MsgVerb(X_INFO, 2, "PCI: %.2x:%02x:%1x: chip %04x,%04x"
		    " card %04x,%04x rev %02x class %02x,%02x,%02x hdr %02x\n",
		    devp->busnum, devp->devnum, devp->funcnum,
		    devp->pci_vendor, devp->_pci_device,
		    devp->pci_subsys_vendor, devp->pci_subsys_card,
		    devp->pci_rev_id, devp->pci_base_class,
		    devp->pci_sub_class, devp->pci_prog_if,
		    devp->pci_header_type);

	pci_devp[idx++] = devp;
	dev = pci_device_next(iter);
    }

    /* Restore modified data (in reverse order), and link buses */
    while (--idx >= 0) {
	devp = pci_devp[idx];
	switch (devp->pci_header_type & 0x7f) {
	case 0:
	    if ((devp->pci_base_class != PCI_CLASS_BRIDGE) ||
		(devp->pci_sub_class != PCI_SUBCLASS_BRIDGE_HOST))
		break;
	    pciBusInfo[devp->busnum]->bridge = devp;
	    pciBusInfo[devp->busnum]->primary_bus = devp->busnum;
	    break;

	case 1:
	case 2: {
	    uint8_t control;

	    i = PCI_SECONDARY_BUS_EXTRACT(devp->pci_pp_bus_register, devp->tag);
	    if (i > devp->busnum) {
		if (pciBusInfo[i]) {
		    pciBusInfo[i]->bridge = devp;
		    /*
                     * The back link needs to be set here, and is unlikely to
		     * change.
		     */
		    devp->businfo = pciBusInfo[i];
		}
#ifdef ARCH_PCI_PCI_BRIDGE
		ARCH_PCI_PCI_BRIDGE(devp);
#endif
	    }
	    if (!(devp->pci_bridge_control & PCI_PCI_BRIDGE_MASTER_ABORT_EN))
		break;
	    control = devp->pci_bridge_control & ~PCI_PCI_BRIDGE_SECONDARY_RESET;
	    pci_device_cfg_write_u8(devp->dev, & control, 
				    PCI_PCI_BRIDGE_CONTROL_REG);
	    break;
	}

	default:
	    break;
	}
    }

#ifdef XF86SCANPCI_WRAPPER
    XF86SCANPCI_WRAPPER(SCANPCI_TERM);
#endif

    /*
     * Lastly, link bridges to their secondary bus, after the architecture has
     * had a chance to modify these assignments.
     */
    for (idx = 0;  idx < pciNumBuses;  idx++) {
	if (!(busp = pciBusInfo[idx]) || !(devp = busp->bridge))
	    continue;
	devp->businfo = busp;
    }

    xf86MsgVerb(X_INFO, 2, "PCI: End of PCI scan\n");

    return pci_devp;
}

pciConfigPtr
xf86GetPciConfigFromTag(PCITAG Tag)
{
    pciConfigPtr pDev;
    int i = 0;

    for (i = 0 ; (pDev = pci_devp[i]) && i <= MAX_PCI_DEVICES; i++) {
	if (Tag == pDev->tag)
	    return pDev;
    }

    return NULL;	/* Bad data */
}

#ifdef INCLUDE_XF86_NO_DOMAIN

_X_EXPORT int
xf86GetPciDomain(PCITAG Tag)
{
    return 0;
}

_X_EXPORT pointer
xf86MapDomainMemory(int ScreenNum, int Flags, PCITAG Tag,
		    ADDRESS Base, unsigned long Size)
{
    return xf86MapVidMem(ScreenNum, Flags, Base, Size);
}

_X_EXPORT IOADDRESS
xf86MapDomainIO(int ScreenNum, int Flags, PCITAG Tag,
		IOADDRESS Base, unsigned long Size)
{
    return Base;
}

_X_EXPORT int
xf86ReadDomainMemory(PCITAG Tag, ADDRESS Base, int Len, unsigned char *Buf)
{
    int ret, length, rlength;

    /* Read in 64kB chunks */
    ret = 0;
    while ((length = Len) > 0) {
	if (length > 0x010000) length = 0x010000;
	rlength = xf86ReadBIOS(Base, 0, Buf, length);
	if (rlength < 0) {
	    ret = rlength;
	    break;
	}
	ret += rlength;
	if (rlength < length) break;
	Base += rlength;
	Buf += rlength;
	Len -= rlength;
    }

    return ret;
}

#endif /* INCLUDE_XF86_NO_DOMAIN */
