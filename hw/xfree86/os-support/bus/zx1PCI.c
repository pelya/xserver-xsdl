/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/zx1PCI.c,v 1.1 2003/02/23 20:26:49 tsi Exp $ */
/*
 * Copyright (C) 2002-2003 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/*
 * This file contains the glue necessary for support of HP's ZX1 chipset.
 * Keep in mind that this chipset is used in both Itanium2 and PA-RISC
 * architectures.
 */

#include "zx1PCI.h"
#include "xf86.h"
#include "xf86_OSlib.h"
#include "Pci.h"

#define MIO_BASE		0xFED00000UL	/* mio register base */
#define MIO_SIZE		0x00002000UL	/* 8k, minimum */

/* ZX1 mio register definitions */
#define MIO_FUNCTION0		0x0000U

#define MODULE_INFO		0x0100U
#define STATUS_CONTROL		0x0108U
#define DILLON_PRESENT		  0x02UL

#define LMMIO_DIR_BASE0		0x0300U
#define LMMIO_DIR_MASK0		0x0308U
#define LMMIO_DIR_ROUTE0	0x0310U
#define LMMIO_DIR_BASE1		0x0318U
#define LMMIO_DIR_MASK1		0x0320U
#define LMMIO_DIR_ROUTE1	0x0328U
#define LMMIO_DIR_BASE2		0x0330U
#define LMMIO_DIR_MASK2		0x0338U
#define LMMIO_DIR_ROUTE2	0x0340U
#define LMMIO_DIR_BASE3		0x0348U
#define LMMIO_DIR_MASK3		0x0350U
#define LMMIO_DIR_ROUTE3	0x0358U
#define LMMIO_DIST_BASE		0x0360U
#define LMMIO_DIST_MASK		0x0368U
#define LMMIO_DIST_ROUTE	0x0370U
#define GMMIO_DIST_BASE		0x0378U
#define PORT_DISABLE		  0x02UL
#define MAP_TO_LMMIO		  0x04UL
#define GMMIO_DIST_MASK		0x0380U
#define GMMIO_DIST_ROUTE	0x0388U
#define IOS_DIST_BASE		0x0390U
#define IOS_DIST_MASK		0x0398U
#define IOS_DIST_ROUTE		0x03A0U
#define ROPE_CONFIG_BASE	0x03A8U
#define VGA_ROUTE		0x03B0U
#define VGA_ENABLE		  0x8000000000000000UL
#define VGA_LIGHT		  0x4000000000000000UL

#define IOS_DIR_BASE		0x03C0U
#define IOS_DIR_MASK		0x03C8U
#define IOS_DIR_ROUTE		0x03D0U
#define IOS_BASE		0x03D8U

#define MIO_FUNCTION1		0x1000U

#define ROPE_CONFIG		0x1040U
#define ROPE_D0			  0x0100UL
#define ROPE_D2			  0x0200UL
#define ROPE_D4			  0x0400UL
#define ROPE_D6			  0x0800UL
#define ROPE_Q0			  0x1000UL
#define ROPE_Q4			  0x2000UL

#define LBA_PORT0_CNTRL		0x1200U
#define LBA_PORT1_CNTRL		0x1208U
#define LBA_PORT2_CNTRL		0x1210U
#define LBA_PORT3_CNTRL		0x1218U
#define LBA_PORT4_CNTRL		0x1220U
#define LBA_PORT5_CNTRL		0x1228U
#define LBA_PORT6_CNTRL		0x1230U
#define LBA_PORT7_CNTRL		0x1238U
#define LBA_HARD_FAIL		  0x40UL

#define ROPE_PAGE_CONTROL	0x1418U

/*
 * Total ioa configuration space size is actually 128k, but we only need the
 * first 64k.
 */
#define IOA_SIZE		0x00010000UL

/* ZX1 ioa register definitions */
#define IOA_CONFIG_ADDR		0x0040U
#define IOA_CONFIG_DATA		0x0048U

#define IOA_SECONDARY_BUS	0x0058U
#define IOA_SUBORDINATE_BUS	0x0059U

#define IOA_CONTROL		0x0108U
#define IOA_FORWARD_VGA		  0x08UL
#define IOA_HARD_FAIL		  0x40UL

#define IOA_LMMIO_BASE		0x0200U
#define IOA_LMMIO_MASK		0x0208U
#define IOA_GMMIO_BASE		0x0210U
#define IOA_GMMIO_MASK		0x0218U
#define IOA_WLMMIO_BASE		0x0220U
#define IOA_WLMMIO_MASK		0x0228U
#define IOA_WGMMIO_BASE		0x0230U
#define IOA_WGMMIO_MASK		0x0238U
#define IOA_IOS_BASE		0x0240U
#define IOA_IOS_MASK		0x0248U
#define IOA_ELMMIO_BASE		0x0250U
#define IOA_ELMMIO_MASK		0x0258U
#define IOA_EIOS_BASE		0x0260U
#define IOA_EIOS_MASK		0x0268U

#define IOA_SLAVE_CONTROL	0x0278U
#define IOA_VGA_PEER_ENABLE	  0x2000UL
#define IOA_MSI_BASE		0x0280U
#define IOA_MSI_MASK		0x0288U

#define RANGE_ENABLE		0x01UL		/* In various base registers */

#define IO_MASK			((1UL << 16) - 1UL)
#define LMMIO_MASK		((1UL << 32) - 1UL)
#ifdef __ia64__
#define GMMIO_MASK		((1UL << 44) - 1UL)
#else	/* PA-RISC */
#define GMMIO_MASK		((1UL << 40) - 1UL)
#endif

#define PDH_START		0xFF000000UL
#define PDH_LAST		0xFFFFFFFFUL

static CARD8 *pZX1mio = NULL,
	     *pZX1ioa = NULL;

static INT8   zx1_ropemap[8];		/* One for each (potential) rope */
static CARD64 zx1_lbacntl[8];		/*  "   "   "        "       "   */
static int    zx1_busno[8], zx1_subno[8];

static pciBusFuncs_t zx1BusFuncs;
static int           zx1_fakebus = -1;
static Bool          zx1_hasvga = FALSE;

static pointer pZX1IoRes[8], pZX1MemRes[8];	/* Rope resources */

/* Non-PCI configuration space access macros */
#define MIO_BYTE(offset) \
    (*(volatile CARD8  *)(pointer)(pZX1mio + (offset)))
#define MIO_WORD(offset) \
    (*(volatile CARD16 *)(pointer)(pZX1mio + (offset)))
#define MIO_LONG(offset) \
    (*(volatile CARD32 *)(pointer)(pZX1mio + (offset)))
#define MIO_QUAD(offset) \
    (*(volatile CARD64 *)(pointer)(pZX1mio + (offset)))
#define IOA_BYTE(ioa, offset) \
    (*(volatile CARD8  *)(pointer)(pZX1ioa + ((offset) + ((ioa) << 13))))
#define IOA_WORD(ioa, offset) \
    (*(volatile CARD16 *)(pointer)(pZX1ioa + ((offset) + ((ioa) << 13))))
#define IOA_LONG(ioa, offset) \
    (*(volatile CARD32 *)(pointer)(pZX1ioa + ((offset) + ((ioa) << 13))))
#define IOA_QUAD(ioa, offset) \
    (*(volatile CARD64 *)(pointer)(pZX1ioa + ((offset) + ((ioa) << 13))))

/* Range definitions */
#define MAX_RANGE 16
static CARD64 bot[MAX_RANGE], top[MAX_RANGE], msk[MAX_RANGE], siz[MAX_RANGE];
static INT8 *pDecode[MAX_RANGE];
static int nRange = 0;

/* Track a resource range and assign a granularity to it */
static void
SetRange(CARD64 base, CARD64 last, CARD8 width)
{
    int i;

    bot[nRange] = base;
    top[nRange] = last;
    msk[nRange] = (CARD64)(-1L);
    if (base)
	msk[nRange] &= (base ^ (base - 1UL)) >> 1;
    if (last + 1UL)
	msk[nRange] &= (last ^ (last + 1UL)) >> 1;
    if (width < 64)
	msk[nRange] &= (1UL << width) - 1UL;

    /* Look for overlapping ranges */
    for (i = 0;  i < nRange;  i++) {
	if ((bot[i] > top[i]) ||
	    (top[nRange] < bot[i]) ||
	    (top[i] < bot[nRange]))
	    continue;

	/* Merge in overlapping range */
	if (bot[nRange] > bot[i])
	    bot[nRange] = bot[i];
	if (top[nRange] < top[i])
	    top[nRange] = top[i];

	/* Assign finer granularity */
	msk[nRange] &= msk[i];
	bot[i] = 1UL;
	top[i] = 0;
    }

    nRange++;
}

/* Lookup granularity associated with the range containing 'base' */
static int
GetRange(CARD64 base)
{
    int i;

    for (i = 0;  i < nRange;  i++) {
	if ((bot[i] > top[i]) ||
	    (base < bot[i]) ||
	    (base > top[i]))
	    continue;

	if (pDecode[i])
	    break;

	/* Allocate decoding array */
	msk[i]++;
	siz[i] = ((top[i] - bot[i] + 1UL) / msk[i]) + 1UL;
	pDecode[i] = xnfalloc(siz[i]);
	(void)memset(pDecode[i], -1, siz[i]);
	break;
    }

    return i;
}

/*
 * Verify that 'bus' is a rope's secondary bus and return the pciConfigPtr of
 * the associated fake PCI-to-PCI bridge.
 */
static pciConfigPtr
VerifyZX1Bus(int bus)
{
    pciConfigPtr pPCI;

    if ((bus < 0) || (bus >= pciNumBuses) ||
	!pciBusInfo[bus] || !(pPCI = pciBusInfo[bus]->bridge) ||
	(pPCI->busnum != zx1_fakebus) || (pPCI->funcnum != 0) ||
	(pPCI->devnum < 0x10) || (pPCI->devnum > 0x17))
	return NULL;

    return pPCI;
}

/*
 * This function is called to emulate the various settings in a P2P or CardBus
 * bridge's control register on a ZX1-based system.
 */
static CARD16
ControlZX1Bridge(int bus, CARD16 mask, CARD16 value)
{
    pciConfigPtr pPCI;
    CARD64 tmp1, tmp2, tmp3, ropenum;
    CARD16 current = 0;

    if ((pPCI = VerifyZX1Bus(bus))) {
	ropenum = pPCI->devnum & 0x07;

	/*
	 * Start with VGA enablement.  This preserves the "VGA-lite" bit
	 * in mio's VGA_ROUTE register, and the VPE bit in each ioa's
	 * SLAVE_CONTROL register.
	 */
	tmp1 = MIO_QUAD(VGA_ROUTE);
	tmp2 = IOA_QUAD(ropenum, IOA_CONTROL);
	if ((tmp1 & VGA_ENABLE) && ((tmp1 & 0x07UL) == ropenum)) {
	    current |= PCI_PCI_BRIDGE_VGA_EN;
	    if ((mask & PCI_PCI_BRIDGE_VGA_EN) &&
		!(value & PCI_PCI_BRIDGE_VGA_EN)) {
		MIO_QUAD(VGA_ROUTE) = tmp1 & ~VGA_ENABLE;
		tmp2 &= ~IOA_FORWARD_VGA;
		IOA_QUAD(ropenum, IOA_CONTROL) = tmp2;
	    }
	} else if (mask & value & PCI_PCI_BRIDGE_VGA_EN) {
	    if (!zx1_hasvga) {
		xf86MsgVerb(X_WARNING, 3,
		    "HP ZX1:  Attempt to enable VGA routing to bus %d"
		    " through rope %ld disallowed\n", bus, ropenum);
		value &= ~PCI_PCI_BRIDGE_VGA_EN;
	    } else {
		if (tmp1 & VGA_ENABLE) {
		    /*
		     * VGA is routed somewhere else.  Disable it.
		     */
		    MIO_QUAD(VGA_ROUTE) = 0UL;
		    tmp3 = IOA_QUAD(tmp1 & 0x07UL, IOA_CONTROL);
		    if (tmp3 & IOA_FORWARD_VGA)
			IOA_QUAD(tmp1 & 0x07UL, IOA_CONTROL) =
			    tmp3 & ~IOA_FORWARD_VGA;
		}
		if (!(tmp2 & IOA_FORWARD_VGA)) {
		    tmp2 |= IOA_FORWARD_VGA;
		    IOA_QUAD(ropenum, IOA_CONTROL) = tmp2;
		}
		tmp1 = (tmp1 & ~0x07UL) | ropenum | VGA_ENABLE;
		MIO_QUAD(VGA_ROUTE) = tmp1;
	    }
	}

	/* Move on to master abort failure enablement */
	tmp1 = MIO_QUAD((ropenum << 3) + LBA_PORT0_CNTRL);
	if ((tmp1 & LBA_HARD_FAIL) || (tmp2 & IOA_HARD_FAIL)) {
	    current |= PCI_PCI_BRIDGE_MASTER_ABORT_EN;
	    if ((mask & PCI_PCI_BRIDGE_MASTER_ABORT_EN) &&
		!(value & PCI_PCI_BRIDGE_MASTER_ABORT_EN)) {
		if (tmp1 & LBA_HARD_FAIL)
		    MIO_QUAD((ropenum << 3) + LBA_PORT0_CNTRL) =
			tmp1 & ~LBA_HARD_FAIL;
		if (tmp2 & IOA_HARD_FAIL) {
		    tmp2 &= ~IOA_HARD_FAIL;
		    IOA_QUAD(ropenum, IOA_CONTROL) = tmp2;
		}
	    }
	} else {
	    if (mask & value & PCI_PCI_BRIDGE_MASTER_ABORT_EN) {
		if (!(tmp1 & LBA_HARD_FAIL))
		    MIO_QUAD((ropenum << 3) + LBA_PORT0_CNTRL) =
			tmp1 | LBA_HARD_FAIL;
		if (!(tmp2 & IOA_HARD_FAIL)) {
		    tmp2 |= IOA_HARD_FAIL;
		    IOA_QUAD(ropenum, IOA_CONTROL) = tmp2;
		}
	    }
	}

	/* Put emulation of any other P2P bridge control here */
    }

    return (current & ~mask) | (value & mask);
}

/* Retrieves a list of the resources routed to a rope's secondary bus */
static void
GetZX1BridgeResources(int bus,
		      pointer *ppIoRes,
		      pointer *ppMemRes,
		      pointer *ppPmemRes)
{
    pciConfigPtr pPCI = VerifyZX1Bus(bus);

    if (ppIoRes) {
	xf86FreeResList(*ppIoRes);
	*ppIoRes =
	    pPCI ? xf86DupResList(pZX1IoRes[pPCI->devnum & 0x07]) : NULL;
    }

    if (ppMemRes) {
	xf86FreeResList(*ppMemRes);
	*ppMemRes =
	    pPCI ? xf86DupResList(pZX1MemRes[pPCI->devnum & 0x07]) : NULL;
    }

    if (ppPmemRes) {
	xf86FreeResList(*ppPmemRes);
	*ppPmemRes = NULL;
    }
}

/* The fake bus */
static CARD32
zx1FakeReadLong(PCITAG tag, int offset)
{
    FatalError("zx1FakeReadLong(0x%X, 0x%X) called\n", tag, offset);
}

static void
zx1FakeWriteLong(PCITAG tag, int offset, CARD32 val)
{
    FatalError("zx1FakeWriteLong(0x%X, 0x%X, 0x%08X) called\n",
	       tag, offset, val);
}

static void
zx1FakeSetBits(PCITAG tag, int offset, CARD32 mask, CARD32 bits)
{
    CARD32 val;

    val = zx1FakeReadLong(tag, offset);
    val &= ~mask;
    val |= bits;
    zx1FakeWriteLong(tag, offset, val);
}

static pciBusFuncs_t zx1FakeBusFuncs = {
    zx1FakeReadLong,
    zx1FakeWriteLong,
    zx1FakeSetBits
};

static pciBusInfo_t zx1FakeBus = {
    0,			/* configMech -- copied from bus 0 */
    0,			/* numDevices -- copied from bus 0 */
    FALSE,		/* secondary */
    0,			/* primary_bus -- dynamically set */
#ifdef PowerMAX_OS
    0,			/* ppc_io_base -- ignored */
    0,			/* ppc_io_size -- ignored */
#endif
    &zx1FakeBusFuncs,	/* funcs */
    NULL,		/* pciBusPriv -- none */
    NULL,		/* bridge -- dynamically set */
};

/*
 * This checks for, and validates, the presence of the ZX1 chipset, and sets
 * pZX1mio to a non-NULL pointer accordingly.  This function is called before
 * the server's PCI bus scan and returns TRUE if the chipset scan is to be
 * stopped, or FALSE if the scan is to move on to the next chipset.
 */
Bool
xf86PreScanZX1(void)
{
    resRange range;
    unsigned long mapSize = xf86getpagesize();
    unsigned long tmp, base, ioaaddr;
    unsigned long flagsd = 0, based = 0, lastd = 0, maskd = 0, routed = 0;
    unsigned long flags0 = 0, base0 = 0, last0 = 0, mask0 = 0, route0 = 0;
    unsigned long flags1 = 0, base1 = 0, last1 = 0, mask1 = 0, route1 = 0;
    unsigned long flags2 = 0, base2 = 0, last2 = 0, mask2 = 0, route2 = 0;
    unsigned long flags3 = 0, base3 = 0, last3 = 0, mask3 = 0, route3 = 0;
    unsigned long flagsg = 0, baseg = 0, lastg = 0, maskg = 0, routeg = 0;
    unsigned long flagsl = 0, basel = 0, lastl = 0;
    int i, rope;

    /* Map mio registers (minimum 8k) */
    if (mapSize < MIO_SIZE)
	mapSize = MIO_SIZE;

    if (!(pZX1mio = xf86MapVidMem(-1, VIDMEM_MMIO, MIO_BASE, mapSize)))
	return FALSE;

    /* Look for ZX1's SBA and IOC */
    if ((MIO_LONG(MIO_FUNCTION0 + PCI_ID_REG) != DEVID(HP, ZX1_SBA)) ||
	(MIO_LONG(MIO_FUNCTION1 + PCI_ID_REG) != DEVID(HP, ZX1_IOC))) {
	xf86UnMapVidMem(-1, pZX1mio, mapSize);
	pZX1mio = NULL;
	return FALSE;
    }

    /* Map rope configuration space */
    ioaaddr = MIO_QUAD(ROPE_CONFIG_BASE);
    if (!(ioaaddr & RANGE_ENABLE) ||			      /* No ropes */
	((ioaaddr = ioaaddr & ~RANGE_ENABLE) & 0x01FFFFUL) || /* Not aligned */
	!(pZX1ioa = xf86MapVidMem(-1, VIDMEM_MMIO, ioaaddr, IOA_SIZE))) {
	xf86UnMapVidMem(-1, pZX1mio, mapSize);
	pZX1mio = NULL;
	return TRUE;
    }

    for (i = 0;  i < 8;  i++) {
	zx1_ropemap[i] = i;
	zx1_lbacntl[i] = 0;
	xf86FreeResList(pZX1IoRes[i]);
	xf86FreeResList(pZX1MemRes[i]);
	pZX1IoRes[i] = pZX1MemRes[i] = NULL;
    }

    /*
     * Determine which of 8 possible ropes exist in the system.  This is done
     * by looking at their "coupling" to generate a list of candidates,
     * whittling this list down by factoring in ROPE_PAGE_CONTROL register
     * contents, then poking each candidate's configuration space to determine
     * its existence.
     */
    tmp = MIO_QUAD(ROPE_CONFIG);
    if (tmp & ROPE_D0)
	zx1_ropemap[1] = 0;
    if (tmp & ROPE_D2)
	zx1_ropemap[3] = 2;
    if (tmp & ROPE_D4)
	zx1_ropemap[5] = 4;
    if (tmp & ROPE_D6)
	zx1_ropemap[7] = 6;
    if (tmp & ROPE_Q0)
	zx1_ropemap[1] = zx1_ropemap[2] = zx1_ropemap[3] = 0;
    if (tmp & ROPE_Q4)
	zx1_ropemap[5] = zx1_ropemap[6] = zx1_ropemap[7] = 4;

    tmp = MIO_QUAD(ROPE_PAGE_CONTROL);
    for (i = 0;  i < 8;  i++, tmp >>= 8)
	if (!(CARD8)tmp)
	    zx1_ropemap[i] = -1;

    for (i = 0;  i < 8;  ) {
	if (zx1_ropemap[i] == i) {

	    /* Prevent hard-fails */
	    zx1_lbacntl[i] = MIO_QUAD((i << 3) + LBA_PORT0_CNTRL);
	    if (zx1_lbacntl[i] & LBA_HARD_FAIL)
		MIO_QUAD((i << 3) + LBA_PORT0_CNTRL) =
		    zx1_lbacntl[i] & ~LBA_HARD_FAIL;

	    /* Poke for an ioa */
	    tmp = IOA_LONG(i, PCI_ID_REG);
	    switch ((CARD32)tmp) {
	    case DEVID(HP, ELROY):	/* Expected vendor/device id's */
	    case DEVID(HP, ZX1_LBA):
		zx1_busno[i] =
		    (unsigned int)IOA_BYTE(i, IOA_SECONDARY_BUS);
		zx1_subno[i] =
		    (unsigned int)IOA_BYTE(i, IOA_SUBORDINATE_BUS);
		break;

	    default:
		if ((CARD16)(tmp + 1U) > (CARD16)1U)
		    xf86MsgVerb(X_NOTICE, 0,
			"HP ZX1:  Unexpected vendor/device id 0x%08X"
			" on rope %d\n", (CARD32)tmp, i);
		/* Nobody home, or not the "right" kind of rope guest */

		/*
		 * Restore hard-fail setting.  For "active" ropes, this is done
		 * later.
		 */
		if (zx1_lbacntl[i] & LBA_HARD_FAIL) {
		    MIO_QUAD((i << 3) + LBA_PORT0_CNTRL) = zx1_lbacntl[i];
		    zx1_lbacntl[i] = 0;
		}

		/* Ignore this rope and its couplings */
		do {
		    zx1_ropemap[i++] = -1;
		} while ((i < 8) && (zx1_ropemap[i] < i));
		continue;	/* Avoid over-incrementing 'i' */
	    }
	}
	i++;
    }

    /* Determine if VGA is currently routed */
    tmp = MIO_QUAD(VGA_ROUTE);
    if (tmp & VGA_ENABLE)
	zx1_hasvga = TRUE;

    /*
     * Decode mio resource "coarse" routing (i.e. ignoring VGA).  Due to the
     * rather unusual flexibility of this chipset, this is done in a number of
     * stages.  For each of I/O and memory, first decode the relevant registers
     * to generate ranges with an associated granularity.  Overlapping ranges
     * are merged into a larger range with the finer granularity.  Each
     * original range is then more thoroughly decoded using the granularity
     * associated with the merged range that contains it.  The result is then
     * converted into resource lists for the common layer.
     *
     * Note that this doesn't care whether or not read-only bits are actually
     * set as documented, nor that mask bits are contiguous.  This does,
     * however, factor in upper limits on I/O, LMMIO anf GMMIO addresses, and
     * thus assumes high-order address bits are ignored rather than decoded.
     * For example, an I/O address of 0x76543210 will be treated as 0x3210
     * rather than considered out-of-range.  In part, this handling is a
     * consequence of the fact that high-order mask bits are zeroes instead of
     * ones.
     */

    if ((tmp = MIO_QUAD(IOS_DIST_BASE)) & RANGE_ENABLE) {
	flagsd = RANGE_ENABLE;
	maskd = MIO_QUAD(IOS_DIST_MASK);
	based = tmp & maskd & (~RANGE_ENABLE & IO_MASK);
	lastd = based | (~maskd & IO_MASK);
	routed = MIO_QUAD(IOS_DIST_ROUTE) >> 58;
	SetRange(based, lastd, routed);
    }

    if ((tmp = MIO_QUAD(IOS_DIR_BASE)) & RANGE_ENABLE) {
	flags0 = RANGE_ENABLE;
	mask0 = MIO_QUAD(IOS_DIR_MASK);
	base0 = tmp & mask0 & (~RANGE_ENABLE & IO_MASK);
	last0 = base0 | (~mask0 & IO_MASK);
	route0 = MIO_QUAD(IOS_DIR_ROUTE) & 0x07U;
	SetRange(base0, last0, 64);
    }

    if (flagsd) {
	i = GetRange(based);
	for (tmp = based;  tmp <= lastd;  tmp += msk[i]) {
	    if ((tmp & maskd) == based) {
		base = (tmp - bot[i]) / msk[i];
		pDecode[i][base] = zx1_ropemap[(tmp >> routed) & 0x07U];
	    }
	}

	flagsd = 0;
    }

    if (flags0) {
	i = GetRange(base0);
	for (tmp = base0;  tmp <= last0;  tmp += msk[i]) {
	    if ((tmp & mask0) == base0) {
		base = (tmp - bot[i]) / msk[i];
		pDecode[i][base] = zx1_ropemap[route0];
	    }
	}

	flags0 = 0;
    }

    for (i = 0;  i < nRange;  i++) {
	if (!pDecode[i])
	    continue;

	rope = pDecode[i][0];
	for (base = tmp = 0;  ++tmp < siz[i];  ) {
	    if (rope == pDecode[i][tmp])
		continue;

	    if (rope >= 0) {
		RANGE(range, (base * msk[i]) + bot[i],
		    (tmp * msk[i]) + bot[i] - 1UL,
		    RANGE_TYPE(ResExcIoBlock, 0));
		pZX1IoRes[rope] =
		    xf86AddResToList(pZX1IoRes[rope], &range, -1);
	    }

	    base = tmp;
	    rope = pDecode[i][base];
	}

	xfree(pDecode[i]);
	pDecode[i] = NULL;
    }

    nRange = 0;

    /*
     * Move on to CPU memory access decoding.  For now, don't tell the common
     * layer about CPU memory ranges that are either relocated to 0 or
     * translated into PCI I/O.
     */

    SetRange(MIO_BASE, MIO_BASE + MIO_SIZE - 1UL, 64);		/* mio */
    SetRange(ioaaddr, ioaaddr + ((IOA_SIZE << 1) - 1UL), 64);	/* ioa */
    SetRange(PDH_START, PDH_LAST, 64);				/* PDH */

    SetRange(MIO_BASE, LMMIO_MASK, 64);			/* Completeness */

    if ((tmp = MIO_QUAD(LMMIO_DIST_BASE)) & RANGE_ENABLE) {
	flagsd = RANGE_ENABLE;
	maskd = MIO_QUAD(LMMIO_DIST_MASK);
	based = tmp & maskd & (~RANGE_ENABLE & LMMIO_MASK);
	lastd = based | (~maskd & LMMIO_MASK);
	routed = MIO_QUAD(LMMIO_DIST_ROUTE) >> 58;
	SetRange(based, lastd, routed);
    }

    if ((tmp = MIO_QUAD(LMMIO_DIR_BASE0)) & RANGE_ENABLE) {
	flags0 = RANGE_ENABLE;
	mask0 = MIO_QUAD(LMMIO_DIR_MASK0);
	base0 = tmp & mask0 & (~RANGE_ENABLE & LMMIO_MASK);
	last0 = base0 | (~mask0 & LMMIO_MASK);
	route0 = MIO_QUAD(LMMIO_DIR_ROUTE0) & 0x07U;
	SetRange(base0, last0, 64);
    }

    if ((tmp = MIO_QUAD(LMMIO_DIR_BASE1)) & RANGE_ENABLE) {
	flags1 = RANGE_ENABLE;
	mask1 = MIO_QUAD(LMMIO_DIR_MASK1);
	base1 = tmp & mask1 & (~RANGE_ENABLE & LMMIO_MASK);
	last1 = base1 | (~mask1 & LMMIO_MASK);
	route1 = MIO_QUAD(LMMIO_DIR_ROUTE1) & 0x07U;
	SetRange(base1, last1, 64);
    }

    if ((tmp = MIO_QUAD(LMMIO_DIR_BASE2)) & RANGE_ENABLE) {
	flags2 = RANGE_ENABLE;
	mask2 = MIO_QUAD(LMMIO_DIR_MASK2);
	base2 = tmp & mask2 & (~RANGE_ENABLE & LMMIO_MASK);
	last2 = base2 | (~mask2 & LMMIO_MASK);
	route2 = MIO_QUAD(LMMIO_DIR_ROUTE2) & 0x07U;
	SetRange(base2, last2, 64);
    }

    if ((tmp = MIO_QUAD(LMMIO_DIR_BASE3)) & RANGE_ENABLE) {
	flags3 = RANGE_ENABLE;
	mask3 = MIO_QUAD(LMMIO_DIR_MASK3);
	base3 = tmp & mask3 & (~RANGE_ENABLE & LMMIO_MASK);
	last3 = base3 | (~mask3 & LMMIO_MASK);
	route3 = MIO_QUAD(LMMIO_DIR_ROUTE3) & 0x07U;
	SetRange(base3, last3, 64);
    }

    if ((tmp = MIO_QUAD(GMMIO_DIST_BASE)) & RANGE_ENABLE) {
	flagsg = tmp & (RANGE_ENABLE | PORT_DISABLE | MAP_TO_LMMIO);
	maskg = MIO_QUAD(GMMIO_DIST_MASK);
	baseg = tmp & maskg &
	    (~(RANGE_ENABLE | PORT_DISABLE | MAP_TO_LMMIO) & GMMIO_MASK);
	lastg = baseg | (~maskg & GMMIO_MASK);
	tmp = routeg = MIO_QUAD(GMMIO_DIST_ROUTE) >> 58;
	if (!(flagsg & (PORT_DISABLE & MAP_TO_LMMIO)) && (tmp > 26))
	    tmp = 26;
	SetRange(baseg, lastg, tmp);
    }

    if ((tmp = MIO_QUAD(IOS_BASE)) & RANGE_ENABLE) {
	flagsl = RANGE_ENABLE;
	basel = tmp & (~RANGE_ENABLE & GMMIO_MASK);
	lastl = basel | 0x001FFFFFUL;
	SetRange(basel, lastl, 64);
    }

    if (flagsd) {
	i = GetRange(based);
	for (tmp = based;  tmp <= lastd;  tmp += msk[i]) {
	    if ((tmp & maskd) == based) {
		base = (tmp - bot[i]) / msk[i];
		pDecode[i][base] = zx1_ropemap[(tmp >> routed) & 0x07U];
	    }
	}

	flagsd = 0;
    }

    /* LMMIO distributed range does not address anything beyond 0xFED00000 */
    i = GetRange(MIO_BASE);
    for (tmp = MIO_BASE;  tmp <= LMMIO_MASK;  tmp += msk[i]) {
	base = (tmp - bot[i]) / msk[i];
	pDecode[i][base] = -1;
    }

    /* Dillon space can sometimes be redirected to rope 0 */
    tmp = MIO_QUAD(STATUS_CONTROL);
    if (!(tmp & DILLON_PRESENT)) {
	i = GetRange(PDH_START);
	for (tmp = PDH_START;  tmp <= PDH_LAST;  tmp += msk[i]) {
	    base = (tmp - bot[i]) / msk[i];
	    pDecode[i][base] = zx1_ropemap[0];
	}
    }

    if (flagsg) {
	unsigned long mask = (0x07UL << routeg) | maskg;

	i = GetRange(baseg);
	for (tmp = baseg;  tmp <= lastg;  tmp += msk[i]) {
	    if ((tmp & maskg) == baseg) {
		base = (tmp - bot[i]) / msk[i];

		if ((flagsg & MAP_TO_LMMIO) ||
		    (!(flagsg & PORT_DISABLE) &&
		     (tmp <= ((tmp & mask) | 0x03FFFFFFUL)))) {
		    pDecode[i][base] = -1;
		} else {
		    pDecode[i][base] = zx1_ropemap[(tmp >> routeg) & 0x07U];
		}
	    }
	}

	flagsg = 0;
    }

    if (flagsl) {
	i = GetRange(basel);
	for (tmp = basel;  tmp <= lastl;  tmp += msk[i]) {
	    base = (tmp - bot[i]) / msk[i];
	    pDecode[i][base] = -1;
	}

	flagsl = 0;
    }

    /* For now, assume directed LMMIO ranges don't overlap with each other */
    if (flags0) {
	i = GetRange(base0);
	for (tmp = base0;  tmp <= last0;  tmp += msk[i]) {
	    if ((tmp & mask0) == base0) {
		base = (tmp - bot[i]) / msk[i];
		pDecode[i][base] = zx1_ropemap[route0];
	    }
	}

	flags0 = 0;
    }

    if (flags1) {
	i = GetRange(base1);
	for (tmp = base1;  tmp <= last1;  tmp += msk[i]) {
	    if ((tmp & mask1) == base1) {
		base = (tmp - bot[i]) / msk[i];
		pDecode[i][base] = zx1_ropemap[route1];
	    }
	}

	flags1 = 0;
    }

    if (flags2) {
	i = GetRange(base2);
	for (tmp = base2;  tmp <= last2;  tmp += msk[i]) {
	    if ((tmp & mask2) == base2) {
		base = (tmp - bot[i]) / msk[i];
		pDecode[i][base] = zx1_ropemap[route2];
	    }
	}

	flags2 = 0;
    }

    if (flags3) {
	i = GetRange(base3);
	for (tmp = base3;  tmp <= last3;  tmp += msk[i]) {
	    if ((tmp & mask3) == base3) {
		base = (tmp - bot[i]) / msk[i];
		pDecode[i][base] = zx1_ropemap[route3];
	    }
	}

	flags3 = 0;
    }

    /* Claim iao config area */
    i = GetRange(ioaaddr);
    for (tmp = ioaaddr;  tmp < ioaaddr + (IOA_SIZE << 1);  tmp += msk[i]) {
	base = (tmp - bot[i]) / msk[i];
	pDecode[i][base] = -1;
    }

    /* Claim mio config area */
    i = GetRange(MIO_BASE);
    for (tmp = MIO_BASE;  tmp < (MIO_BASE + MIO_SIZE);  tmp += msk[i]) {
	base = (tmp - bot[i]) / msk[i];
	pDecode[i][base] = -1;
    }

    for (i = 0;  i < nRange;  i++) {
	if (!pDecode[i])
	    continue;

	rope = pDecode[i][0];
	for (base = tmp = 0;  ++tmp < siz[i];  ) {
	    if (rope == pDecode[i][tmp])
		continue;

	    if (rope >= 0) {
		RANGE(range, (base * msk[i]) + bot[i],
		    (tmp * msk[i]) + bot[i] - 1UL,
		    RANGE_TYPE(ResExcMemBlock, 0));
		pZX1MemRes[rope] =
		    xf86AddResToList(pZX1MemRes[rope], &range, -1);
	    }

	    base = tmp;
	    rope = pDecode[i][base];
	}

	xfree(pDecode[i]);
	pDecode[i] = NULL;
    }

    nRange = 0;

    return TRUE;
}

/* This is called to finalise the results of a PCI bus scan */
void
xf86PostScanZX1(void)
{
    pciConfigPtr pPCI, *ppPCI, *ppPCI2;
    pciBusInfo_t *pBusInfo;
    int i, idx;

    if (!pZX1mio)
	return;

    /*
     * Certain 2.4 & 2.5 Linux kernels add fake PCI devices.  Remove them to
     * prevent any possible interference with our PCI validation.
     *
     * Also, if VGA isn't routed on server entry, determine if VGA routing
     * needs to be enabled while the server is running.
     */
    idx = 0;
    ppPCI = ppPCI2 = xf86scanpci(0);	/* Recursion is only apparent */
    while ((pPCI = *ppPCI2++)) {
	switch (pPCI->pci_device_vendor) {
	case DEVID(HP, ZX1_SBA):
	case DEVID(HP, ZX1_IOC):
	case DEVID(HP, ZX1_LBA):
	    xfree(pPCI);		/* Remove it */
	    continue;

	default:
	    *ppPCI++ = pPCI;
	    idx++;

	    if (zx1_hasvga)
		continue;

	    switch (pPCI->pci_base_class) {
	    case PCI_CLASS_PREHISTORIC:
		if (pPCI->pci_sub_class == PCI_SUBCLASS_PREHISTORIC_VGA)
		    break;
		continue;

	    case PCI_CLASS_DISPLAY:
		if (pPCI->pci_sub_class == PCI_SUBCLASS_DISPLAY_VGA)
		    break;
		continue;

	    default:
		continue;
	    }

	    zx1_hasvga = TRUE;
	    continue;
	}
    }

    /*
     * Restore hard-fail settings and figure out the actual subordinate bus
     * numbers.
     */
    for (i = 0;  i < 8;  i++) {
	if (zx1_ropemap[i] != i)
	    continue;

	if (zx1_lbacntl[i] & LBA_HARD_FAIL)
	    MIO_QUAD((i << 3) + LBA_PORT0_CNTRL) = zx1_lbacntl[i];

	while ((zx1_busno[i] < zx1_subno[i]) && !pciBusInfo[zx1_subno[i]])
	    zx1_subno[i]--;

	if (zx1_fakebus <= zx1_subno[i])
	    zx1_fakebus = zx1_subno[i] + 1;
    }

    if (zx1_fakebus >= pciNumBuses) {
	if (zx1_fakebus >= pciMaxBusNum)
	    FatalError("HP ZX1:  No room for fake PCI bus\n");
	pciNumBuses = zx1_fakebus + 1;
    }

    /* Set up our extra bus functions */
    zx1BusFuncs = *(pciBusInfo[0]->funcs);
    zx1BusFuncs.pciControlBridge = ControlZX1Bridge;
    zx1BusFuncs.pciGetBridgeResources = GetZX1BridgeResources;

    /* Set up our own fake bus to act as the root segment */
    zx1FakeBus.configMech = pciBusInfo[0]->configMech;
    zx1FakeBus.numDevices = pciBusInfo[0]->numDevices;
    zx1FakeBus.primary_bus = zx1_fakebus;
    pciBusInfo[zx1_fakebus] = &zx1FakeBus;

    /* Add the fake bus' host bridge */
    if (++idx >= MAX_PCI_DEVICES)
	FatalError("HP ZX1:  No room for fake Host-to-PCI bridge\n");
    *ppPCI++ = zx1FakeBus.bridge = pPCI = xnfcalloc(1, sizeof(pciDevice));
    pPCI->tag = PCI_MAKE_TAG(zx1_fakebus, 0, 0);
    pPCI->busnum = zx1_fakebus;
 /* pPCI->devnum = pPCI->funcnum = 0; */
    pPCI->pci_device_vendor = DEVID(HP, ZX1_SBA);
    pPCI->pci_base_class = PCI_CLASS_BRIDGE;
 /* pPCI->pci_sub_class = PCI_SUBCLASS_BRIDGE_HOST; */
    pPCI->fakeDevice = TRUE;

#ifdef OLD_FORMAT
    xf86MsgVerb(X_INFO, 2, "PCI: BusID 0x%.2x,0x%02x,0x%1x "
		"ID 0x%04x,0x%04x Rev 0x%02x Class 0x%02x,0x%02x\n",
		pPCI->busnum, pPCI->devnum, pPCI->funcnum,
		pPCI->pci_vendor, pPCI->pci_device, pPCI->pci_rev_id,
		pPCI->pci_base_class, pPCI->pci_sub_class);
#else
    xf86MsgVerb(X_INFO, 2, "PCI: %.2x:%02x:%1x: chip %04x,%04x"
		" card %04x,%04x rev %02x class %02x,%02x,%02x hdr %02x\n",
		pPCI->busnum, pPCI->devnum, pPCI->funcnum,
		pPCI->pci_vendor, pPCI->pci_device,
		pPCI->pci_subsys_vendor, pPCI->pci_subsys_card,
		pPCI->pci_rev_id, pPCI->pci_base_class,
		pPCI->pci_sub_class, pPCI->pci_prog_if,
		pPCI->pci_header_type);
#endif

    /* Add a fake PCI-to-PCI bridge to represent each active rope */
    for (i = 0;  i < 8;  i++) {
	if ((zx1_ropemap[i] != i) || !(pBusInfo = pciBusInfo[zx1_busno[i]]))
	    continue;

	if (++idx >= MAX_PCI_DEVICES)
	    FatalError("HP ZX1:  No room for fake PCI-to-PCI bridge\n");
	*ppPCI++ = pPCI = xnfcalloc(1, sizeof(pciDevice));
	pPCI->busnum = zx1_fakebus;
	pPCI->devnum = i | 0x10;
     /* pPCI->funcnum = 0; */
	pPCI->tag = PCI_MAKE_TAG(zx1_fakebus, pPCI->devnum, 0);
	pPCI->pci_device_vendor = DEVID(HP, ZX1_LBA);
	pPCI->pci_base_class = PCI_CLASS_BRIDGE;
	pPCI->pci_sub_class = PCI_SUBCLASS_BRIDGE_PCI;
	pPCI->pci_header_type = 1;
	pPCI->pci_primary_bus_number = zx1_fakebus;
	pPCI->pci_secondary_bus_number = zx1_busno[i];
	pPCI->pci_subordinate_bus_number = zx1_subno[i];
	pPCI->fakeDevice = TRUE;

	pBusInfo->bridge = pPCI;
	pBusInfo->secondary = TRUE;
	pBusInfo->primary_bus = zx1_fakebus;

	/* Plug in chipset routines */
	pBusInfo->funcs = &zx1BusFuncs;

#ifdef OLD_FORMAT
	xf86MsgVerb(X_INFO, 2, "PCI: BusID 0x%.2x,0x%02x,0x%1x "
		    "ID 0x%04x,0x%04x Rev 0x%02x Class 0x%02x,0x%02x\n",
		    pPCI->busnum, pPCI->devnum, pPCI->funcnum,
		    pPCI->pci_vendor, pPCI->pci_device, pPCI->pci_rev_id,
		    pPCI->pci_base_class, pPCI->pci_sub_class);
#else
	xf86MsgVerb(X_INFO, 2, "PCI: %.2x:%02x:%1x: chip %04x,%04x"
		    " card %04x,%04x rev %02x class %02x,%02x,%02x hdr %02x\n",
		    pPCI->busnum, pPCI->devnum, pPCI->funcnum,
		    pPCI->pci_vendor, pPCI->pci_device,
		    pPCI->pci_subsys_vendor, pPCI->pci_subsys_card,
		    pPCI->pci_rev_id, pPCI->pci_base_class,
		    pPCI->pci_sub_class, pPCI->pci_prog_if,
		    pPCI->pci_header_type);
#endif
    }

    *ppPCI = NULL;	/* Terminate array */
}
