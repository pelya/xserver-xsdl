/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/axpPci.c,v 1.15 2002/12/12 04:12:02 dawes Exp $ */
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

#include <asm/unistd.h>
#include "../linux/lnx.h"	/* for _iobase */

/*
 * Alpha/Linux platform specific PCI access functions
 */
static CARD32 axpPciCfgRead(PCITAG tag, int off);
static void axpPciCfgWrite(PCITAG, int off, CARD32 val);
static void axpPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits);

static pciBusFuncs_t axpFuncs0 = {
/* pciReadLong      */	axpPciCfgRead,
/* pciWriteLong     */	axpPciCfgWrite,
/* pciSetBitsLong   */	axpPciCfgSetBits,
/* pciAddrHostToBus */	pciAddrNOOP,
/* pciAddrBusToHost */	pciAddrNOOP
};

typedef struct _axpDomainRec {
    int domain, hose;
    int root_bus;
    unsigned long dense_io,  sparse_io;
    unsigned long dense_mem, sparse_mem;
    IOADDRESS mapped_io;
} axpDomainRec, *axpDomainPtr;

#define MAX_DOMAINS (MAX_PCI_BUSES / 256)
static axpDomainPtr xf86DomainInfo[MAX_DOMAINS] = { NULL, };
static int	    pciNumDomains = 0;

/*
 * For debug, domain assignment can start downward from a fixed base 
 * (instead of up from 0) by defining FORCE_HIGH_DOMAINS. This allows
 * debug of large domain numbers and sparse domain numbering on systems
 * which don't have as many hoses.
 */
#if 0
# define FORCE_HIGH_DOMAINS MAX_DOMAINS /* assign domains downward from here */
#endif

/*
 * If FORCE_HIGH_DOMAINS is set, make sure it's not larger than the
 * max domain
 */
#if defined(FORCE_HIGH_DOMAINS) && (FORCE_HIGH_DOMAINS > MAX_DOMAINS)
# undef FORCE_HIGH_DOMAINS
# define FORCE_HIGH_DOMAINS MAX_DOMAINS
#endif

static int
axpSetupDomains(void)
{
    axpDomainRec axpDomain;
    int numDomains = 0;
    int hose;

#ifndef INCLUDE_XF86_NO_DOMAIN

#ifdef FORCE_HIGH_DOMAINS
    xf86Msg(X_WARNING, 
	    "DEBUG OPTION FORCE_HIGH_DOMAINS in use - DRI will *NOT* work\n");
    numDomains = FORCE_HIGH_DOMAINS;
#endif

    /*
     * Since each hose has a different address space, hoses are a perfect
     * overlay for domains, so set up one domain for each hose present
     * in the system. We have to loop through all possible hoses because
     * some systems allow sparse I/O controllers.
     */
    for(hose = 0; hose < MAX_DOMAINS; hose++) {
	axpDomain.root_bus = _iobase(IOBASE_ROOT_BUS, hose, -1, -1);
	if (axpDomain.root_bus < 0) continue;

	axpDomain.hose = hose;

#ifndef FORCE_HIGH_DOMAINS

	axpDomain.domain = axpDomain.hose = hose;
	numDomains = axpDomain.domain + 1;

#else /* FORCE_HIGH_DOMAINS */

	axpDomain.domain = numDomains - hose - 1;

	xf86Msg(X_WARNING, 
		"FORCE_HIGH_DOMAINS - assigned hose %d to domain %d\n",
		axpDomain.hose, axpDomain.domain);

#endif /* FORCE_HIGH_DOMAINS */

	axpDomain.dense_io   = _iobase(IOBASE_DENSE_IO,   hose, -1, -1);
	axpDomain.sparse_io  = _iobase(IOBASE_SPARSE_IO,  hose, -1, -1);
        axpDomain.mapped_io  = 0;
	axpDomain.dense_mem  = _iobase(IOBASE_DENSE_MEM,  hose, -1, -1);
	axpDomain.sparse_mem = _iobase(IOBASE_SPARSE_MEM, hose, -1, -1);

	xf86DomainInfo[axpDomain.domain] = xnfalloc(sizeof(axpDomainRec));
	*(xf86DomainInfo[axpDomain.domain]) = axpDomain;

	/*
	 * For now, only allow a single domain (hose) on sparse i/o systems.
	 *
	 * Allowing multiple domains on sparse systems would require:
	 *	1) either
	 *		a) revamping the sparse video mapping code to allow 
	 *		   for multiple unrelated address regions
	 *		  	-- OR -- 
	 *		b) implementing sparse mapping directly in 
	 *		   xf86MapDomainMemory
	 *	2) revaming read/write sparse routines to correctly handle
	 *	   the solution to 1)
	 *	3) implementing a sparse I/O system (mapping, inX/outX)
	 *	   independent of glibc, since the glibc version only
	 *	   supports hose 0
	 */
	if (axpDomain.sparse_io) {
	    if (_iobase(IOBASE_ROOT_BUS, hose + 1, -1, -1) >= 0) {
		/*
		 * It's a sparse i/o system with (at least) one more hose,
		 * show a message indicating that video is constrained to 
		 * hose 0
		 */
		xf86Msg(X_INFO, 
			"Sparse I/O system - constraining video to hose 0\n");
	    }
	    break;
	}
    }

#else /* INCLUDE_XF86_NO_DOMAIN */

    /*
     * domain support is not included, so just set up a single domain (0)
     * to represent the first hose so that axpPciInit will still have
     * be able to set up the root bus
     */
    xf86DomainInfo[0] = xnfalloc(sizeof(axpDomainRec));
    *(xf86DomainInfo[0]) = axpDomain;
    numDomains = 1;

#endif /* INCLUDE_XF86_NO_DOMAIN */

    return numDomains;
}

void  
axpPciInit()
{
    axpDomainPtr pDomain;
    int domain, bus;

    pciNumDomains = axpSetupDomains();

    for(domain = 0; domain < pciNumDomains; domain++) {
	if (!(pDomain = xf86DomainInfo[domain])) continue;

	/*
	 * Since any bridged buses will be behind a probed pci-pci bridge, 
	 * only set up the root bus for each domain (hose) and the bridged 
	 * buses will be set up as they are found.
	 */
	bus = PCI_MAKE_BUS(domain, 0);
	pciBusInfo[bus] = xnfalloc(sizeof(pciBusInfo_t));
	(void)memset(pciBusInfo[bus], 0, sizeof(pciBusInfo_t));

	pciBusInfo[bus]->configMech = PCI_CFG_MECH_OTHER;
	pciBusInfo[bus]->numDevices = 32;
	pciBusInfo[bus]->funcs = &axpFuncs0;
	pciBusInfo[bus]->pciBusPriv = pDomain;

	pciNumBuses = bus + 1;
    }

    pciFindFirstFP = pciGenFindFirst;
    pciFindNextFP  = pciGenFindNext;
}

/*
 * Alpha/Linux PCI configuration space access routines
 */
static int 
axpPciBusFromTag(PCITAG tag)
{
    pciBusInfo_t *pBusInfo;
    axpDomainPtr pDomain;
    int bus, dfn;

    bus = PCI_BUS_FROM_TAG(tag);
    if ((bus >= pciNumBuses) 
	|| !(pBusInfo = pciBusInfo[bus])
	|| !(pDomain = pBusInfo->pciBusPriv)
	|| (pDomain->domain != PCI_DOM_FROM_TAG(tag))) return -1;

    bus = PCI_BUS_NO_DOMAIN(bus) + pDomain->root_bus;
    dfn = PCI_DFN_FROM_TAG(tag);
    if (_iobase(IOBASE_HOSE, -1, bus, dfn) != pDomain->hose) return -1;

    return bus;
}

static CARD32
axpPciCfgRead(PCITAG tag, int off)
{
    int bus, dfn;
    CARD32 val = 0xffffffff;

    if ((bus = axpPciBusFromTag(tag)) >= 0) {
	dfn = PCI_DFN_FROM_TAG(tag);

	syscall(__NR_pciconfig_read, bus, dfn, off, 4, &val);
    }
    return(val);	
}

static void
axpPciCfgWrite(PCITAG tag, int off, CARD32 val)
{
    int bus, dfn;

    if ((bus = axpPciBusFromTag(tag)) >= 0) {
	dfn = PCI_DFN_FROM_TAG(tag);
	syscall(__NR_pciconfig_write, bus, dfn, off, 4, &val);
    }
}

static void
axpPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits)
{
    int bus, dfn;
    CARD32 val = 0xffffffff;

    if ((bus = axpPciBusFromTag(tag)) >= 0) {
	dfn = PCI_DFN_FROM_TAG(tag);

	syscall(__NR_pciconfig_read, bus, dfn, off, 4, &val);
	val = (val & ~mask) | (bits & mask);
	syscall(__NR_pciconfig_write, bus, dfn, off, 4, &val);
    }
}

#ifndef INCLUDE_XF86_NO_DOMAIN

/*
 * Alpha/Linux addressing domain support
 */

int
xf86GetPciDomain(PCITAG Tag)
{
    return PCI_DOM_FROM_TAG(Tag);
}

pointer
xf86MapDomainMemory(int ScreenNum, int Flags, PCITAG Tag,
		    ADDRESS Base, unsigned long Size)
{
    axpDomainPtr pDomain;
    int domain = PCI_DOM_FROM_TAG(Tag);

    if ((domain < 0) || (domain >= pciNumDomains) ||
	!(pDomain = xf86DomainInfo[domain])) 
	FatalError("%s called with invalid parameters\n", __FUNCTION__);

    /*
     * xf86MapVidMem already does what we need, but remember to subtract
     * _bus_base() (the physical dense memory root of hose 0) since 
     * xf86MapVidMem is expecting an offset relative to _bus_base() rather
     * than an actual physical address.
     */
    return xf86MapVidMem(ScreenNum, Flags, 
			 pDomain->dense_mem + Base - _bus_base(), Size);
}

IOADDRESS
xf86MapDomainIO(int ScreenNum, int Flags, PCITAG Tag,
		IOADDRESS Base, unsigned long Size)
{
    axpDomainPtr pDomain;
    int domain = PCI_DOM_FROM_TAG(Tag);

    if ((domain < 0) || (domain >= pciNumDomains) ||
	!(pDomain = xf86DomainInfo[domain])) 
	FatalError("%s called with invalid parameters\n", __FUNCTION__);

    /*
     * Use glibc inx/outx routines for sparse I/O, so just return the
     * base [this is ok since we also constrain sparse I/O systems to
     * a single domain in axpSetupDomains()]
     */
    if (pDomain->sparse_io) return Base;

    /*
     * I/O addresses on Alpha are really just different physical memory
     * addresses that the system corelogic turns into I/O commands on the
     * bus, so just use xf86MapVidMem to map I/O as well, but remember
     * to subtract _bus_base() (the physical dense memory root of hose 0)
     * since xf86MapVidMem is expecting an offset relative to _bus_base()
     * rather than an actual physical address.
     *
     * Map the entire I/O space (64kB) at once and only once.
     */
    if (!pDomain->mapped_io)
        pDomain->mapped_io = (IOADDRESS)xf86MapVidMem(ScreenNum, Flags, 
		   	            pDomain->dense_io - _bus_base(), 
                                    0x10000);

    return pDomain->mapped_io + Base;
}

int
xf86ReadDomainMemory(PCITAG Tag, ADDRESS Base, int Len, unsigned char *Buf)
{
    static unsigned long pagemask = 0;
    unsigned char *MappedAddr;
    unsigned long MapSize;
    ADDRESS MapBase;
    int i;

    if (!pagemask) pagemask = xf86getpagesize() - 1;

    /* Ensure page boundaries */
    MapBase = Base & ~pagemask;
    MapSize = ((Base + Len + pagemask) & ~pagemask) - MapBase;

    /*
     * VIDMEM_MMIO in order to get sparse mapping on sparse memory systems
     * so we can use mmio functions to read (that way we can really get byte
     * at a time reads on dense memory systems with byte/word instructions.
     */
    MappedAddr = xf86MapDomainMemory(-1, VIDMEM_READONLY | VIDMEM_MMIO, 
                                     Tag, MapBase, MapSize);

    for (i = 0; i < Len; i++) {
	*Buf++ = xf86ReadMmio8(MappedAddr, Base - MapBase + i);
    }
    
    xf86UnMapVidMem(-1, MappedAddr, MapSize);
    return Len;
}

resPtr
xf86PciBusAccWindowsFromOS(void)
{
    resPtr pRes = NULL;
    resRange range;
    int domain;

    for(domain = 0; domain < pciNumDomains; domain++) {
	if (!xf86DomainInfo[domain]) continue;

	RANGE(range, 0, 0xffffffffUL,
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);

	RANGE(range, 0, 0x0000ffffUL,
	      RANGE_TYPE(ResExcIoBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);		      
    }

    return pRes;
}

resPtr
xf86BusAccWindowsFromOS(void)
{
    return xf86PciBusAccWindowsFromOS();
}

resPtr
xf86AccResFromOS(resPtr pRes)
{
    resRange range;
    int domain;

    for(domain = 0; domain < pciNumDomains; domain++) {
	if (!xf86DomainInfo[domain]) continue;

	/*
	 * Fallback is to claim the following areas:
	 *
	 * 0x000c0000 - 0x000effff  location of VGA and other extensions ROMS
	 */

	RANGE(range, 0x000c0000, 0x000effff, 
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);

	/*
	 * Fallback would be to claim well known ports in the 0x0 - 0x3ff 
	 * range along with their sparse I/O aliases, but that's too 
	 * imprecise.  Instead claim a bare minimum here.
	 */
	RANGE(range, 0x00000000, 0x000000ff, 
	      RANGE_TYPE(ResExcIoBlock, domain)); /* For mainboard */
	pRes = xf86AddResToList(pRes, &range, -1);

	/*
	 * At minimum, the top and bottom resources must be claimed, so that
	 * resources that are (or appear to be) unallocated can be relocated.
	 */
	RANGE(range, 0x00000000, 0x00000000, 
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
	RANGE(range, 0xffffffff, 0xffffffff, 
	      RANGE_TYPE(ResExcMemBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
/*  	RANGE(range, 0x00000000, 0x00000000, 
	      RANGE_TYPE(ResExcIoBlock, domain));
        pRes = xf86AddResToList(pRes, &range, -1); */
	RANGE(range, 0xffffffff, 0xffffffff, 
	      RANGE_TYPE(ResExcIoBlock, domain));
	pRes = xf86AddResToList(pRes, &range, -1);
    }

    return pRes;
}

#endif /* !INCLUDE_XF86_NO_DOMAIN */

