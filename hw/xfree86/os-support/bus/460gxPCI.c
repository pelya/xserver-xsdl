/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/460gxPCI.c,v 1.2 2003/01/10 22:05:45 tsi Exp $ */
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
 * This file contains the glue necessary for support of Intel's 460GX chipset.
 */

#include "460gxPCI.h"
#include "xf86.h"
#include "Pci.h"

/* 460GX register definitions */
/* SAC at 0:10:0 */
#define CBN		0x0040
/* SAC at CBN:0:0 */
#define DEVNPRES	0x0070
/* SAC at CBN:1[0-7]:0 */
#define BUSNO		0x0048
#define SUBNO		0x0049
#define VGASE		0x0080
#define PCIS		0x0084
#define IOR		0x008C
#define IORD		0x008E	/* CBN:10:0 only */
/* PXB at CBN:1[0-7]:1 */
#define ERRCMD		0x0046

static int cbn_460gx = -1;
static CARD32 cbdevs_460gx = 0;
static CARD16 iord_460gx;
static int busno_460gx[8], subno_460gx[8];
static CARD8 pcis_460gx[8], ior_460gx[8];
static CARD8 has_err_460gx[8], err_460gx[8];
static CARD8 iomap_460gx[16];		/* One for each 4k */
static pciBusFuncs_t BusFuncs_460gx;

static pciConfigPtr
Verify460GXBus(int bus)
{
    pciConfigPtr pPCI;

    if ((bus < 0) || (bus >= pciNumBuses) ||
	!pciBusInfo[bus] || !(pPCI = pciBusInfo[bus]->bridge) ||
	(pPCI->busnum != cbn_460gx) || (pPCI->funcnum != 0) ||
	(pPCI->devnum < 0x10) || (pPCI->devnum > 0x17))
	return NULL;

    return pPCI;
}

/*
 * This function is called to emulate the various settings in a P2P or CardBus
 * bridge's control register using one of a 460GX's SAC host bridges.
 */
static CARD16
Control460GXBridge(int bus, CARD16 mask, CARD16 value)
{
    pciConfigPtr pPCI;
    PCITAG tag;
    CARD16 current = 0;
    CARD8  tmp;

    if ((pPCI = Verify460GXBus(bus))) {
	/* Start with VGA enablement */
	tmp = pciReadByte(pPCI->tag, VGASE);
	if (tmp & 0x01) {
	    current |= PCI_PCI_BRIDGE_VGA_EN;
	    if ((mask & PCI_PCI_BRIDGE_VGA_EN) &&
		!(value & PCI_PCI_BRIDGE_VGA_EN))
		pciWriteByte(pPCI->tag, VGASE, tmp & ~0x01);
	} else {
	    if (mask & value & PCI_PCI_BRIDGE_VGA_EN)
		pciWriteByte(pPCI->tag, VGASE, tmp | 0x01);
	}

	/* Move on to master abort failure enablement */
	if (has_err_460gx[pPCI->devnum - 0x10]) {
	    tag = PCI_MAKE_TAG(pPCI->busnum, pPCI->devnum, pPCI->funcnum + 1);
	    tmp = pciReadByte(tag, ERRCMD);
	    if (tmp & 0x01) {
		current |= PCI_PCI_BRIDGE_MASTER_ABORT_EN;
		if ((mask & PCI_PCI_BRIDGE_MASTER_ABORT_EN) &&
		    !(value & PCI_PCI_BRIDGE_MASTER_ABORT_EN))
		    pciWriteByte(tag, ERRCMD, tmp & ~0x01);
	    } else {
		if (mask & value & PCI_PCI_BRIDGE_MASTER_ABORT_EN)
		    pciWriteByte(tag, ERRCMD, tmp | 0x01);
	    }
	}

	/* Put emulation of any other P2P bridge control here */
    }

    return (current & ~mask) | (value & mask);
}

/*
 * Retrieve various bus numbers representing the connections provided by 460GX
 * host bridges.
 */
static void
Get460GXBridgeBusses(int bus, int *primary, int *secondary, int *subordinate)
{
    pciConfigPtr pPCI = Verify460GXBus(bus);
    int i;

    /* The returned bus numbers are initialised by the caller */

    if (!pPCI)
	return;

    i = pPCI->devnum - 0x10;

    /* These are not modified, so no need to re-read them */
    if (primary)
	*primary = pPCI->busnum;
    if (secondary)
	*secondary = busno_460gx[i];
    if (subordinate)
	*subordinate = subno_460gx[i];
}

/* Retrieves a list of the resources routed to a host bridge's secondary bus */
static void
Get460GXBridgeResources(int bus,
			pointer *ppIoRes,
			pointer *ppMemRes,
			pointer *ppPmemRes)
{
    pciConfigPtr pPCI = Verify460GXBus(bus);
    resRange range;
    unsigned int i, j;

    if (ppIoRes) {
	xf86FreeResList(*ppIoRes);
	*ppIoRes = NULL;

	if (pPCI) {
	    for (i = 0;  i <= 0x0F;  i++) {
		if (iomap_460gx[i] != pPCI->devnum)
		    continue;

		RANGE(range, i << 12, ((i + 1) << 12) - 1,
		      RANGE_TYPE(ResExcIoBlock, 0));
		*ppIoRes = xf86AddResToList(*ppIoRes, &range, -1);
	    }
	}
    }

    if (ppMemRes) {
	xf86FreeResList(*ppMemRes);
	*ppMemRes = NULL;

	if (pPCI) {
	    if (!(i = (pPCI->devnum - 0x10)))
		j = 127;	/* (4GB - 32M) / 32M */
	    else
		j = pcis_460gx[i - 1] & 0x7F;

	    i = pcis_460gx[i] & 0x7F;
	    if (i < j) {
		RANGE(range, i << 25, (j << 25) - 1,
		    RANGE_TYPE(ResExcMemBlock, 0));
		*ppMemRes = xf86AddResToList(*ppMemRes, &range, -1);
	    }
	}
    }

    if (ppPmemRes) {
	xf86FreeResList(*ppPmemRes);
	*ppPmemRes = NULL;
    }
}

/*
 * This checks for, and validates, the presence of the 460GX chipset, and sets
 * cbn_460gx to a positive value accordingly.  This function returns TRUE if
 * the chipset scan is to be stopped, or FALSE if the scan is to move on to the
 * next chipset.
 */
Bool
xf86PreScan460GX(void)
{
    pciBusInfo_t *pBusInfo;
    PCITAG tag;
    CARD32 tmp;
    int i, devno;

    /* Bus zero should already be set up */
    if (!(pBusInfo = pciBusInfo[0])) {
	cbn_460gx = -1;
	return FALSE;
    }

    /* First look for a 460GX's primary host bridge */
    tag = PCI_MAKE_TAG(0, 0x10, 0);
    if (pciReadLong(tag, PCI_ID_REG) != DEVID(INTEL, 460GX_SAC)) {
	cbn_460gx = -1;
	return FALSE;
    }

    /* Get CBN (Chipset bus number) */
    if (!(cbn_460gx = (unsigned int)pciReadByte(tag, CBN))) {
	/* Sanity check failed */
	cbn_460gx = -1;
	return TRUE;
    }

    if (pciNumBuses <= cbn_460gx)
	pciNumBuses = cbn_460gx + 1;

    /* Set up bus CBN */
    if (!pciBusInfo[cbn_460gx]) {
	pciBusInfo[cbn_460gx] = xnfalloc(sizeof(pciBusInfo_t));
	*pciBusInfo[cbn_460gx] = *pBusInfo;
    }

    tag = PCI_MAKE_TAG(cbn_460gx, 0, 0);
    if (pciReadLong(tag, PCI_ID_REG) != DEVID(INTEL, 460GX_SAC)) {
	/* Sanity check failed */
	cbn_460gx = -1;
	return TRUE;
    }

    /*
     * Find out which CBN devices the firmware thinks are present.  Of these,
     * we are only interested in devices 0x10 through 0x17.
     */
    cbdevs_460gx = pciReadLong(tag, DEVNPRES);

    for (i = 0, devno = 0x10;  devno <= 0x17;  i++, devno++) {
	tag = PCI_MAKE_TAG(cbn_460gx, devno, 0);
	if (pciReadLong(tag, PCI_ID_REG) != DEVID(INTEL, 460GX_SAC)) {
	    /* Sanity check failed */
	    cbn_460gx = -1;
	    return TRUE;
	}

	if (devno == 0x10)
	    iord_460gx = pciReadWord(tag, IORD);

	busno_460gx[i] = (unsigned int)pciReadByte(tag, BUSNO);
	subno_460gx[i] = (unsigned int)pciReadByte(tag, SUBNO);
	pcis_460gx[i] = pciReadByte(tag, PCIS);
	ior_460gx[i] = pciReadByte(tag, IOR);

	has_err_460gx[i] = err_460gx[i] = 0;	/* Insurance */

	tag = PCI_MAKE_TAG(cbn_460gx, devno, 1);
	tmp = pciReadLong(tag, PCI_ID_REG);
	switch (tmp) {
	case DEVID(INTEL, 460GX_PXB):
	case DEVID(INTEL, 460GX_WXB):
	    if (cbdevs_460gx & (1 << devno)) {
		/* Sanity check failed */
		cbn_460gx = -1;
		return TRUE;
	    }

	    /*
	     * XXX  I don't have WXB docs, but PCI register dumps indicate that
	     * the registers we are interested in are consistent with those of
	     * the PXB.
	     */
	    err_460gx[i] = pciReadByte(tag, ERRCMD);
	    has_err_460gx[i] = 1;
	    break;

	case DEVID(INTEL, 460GX_GXB_1):
	    if (cbdevs_460gx & (1 << devno)) {
		/* Sanity check failed */
		cbn_460gx = -1;
		return TRUE;
	    }

	    /*
	     * XXX  GXB isn't documented to have an ERRCMD register, nor any
	     * other means of failing master aborts.  For now, assume master
	     * aborts are always allowed to complete normally.
	     */
	    break;

	default:
	    if (((CARD16)(tmp + 1U) <= (CARD16)1U) &&
		(cbdevs_460gx & (1U << devno)))
		break;
	    /* Sanity check failed */
	    cbn_460gx = -1;
	    return TRUE;
	}
    }

    /* Allow master aborts to complete normally */
    for (i = 0, devno = 0x10;  devno <= 0x17;  i++, devno++) {
	if (!(err_460gx[i] & 0x01))
	    continue;

	pciWriteByte(PCI_MAKE_TAG(cbn_460gx, devno, 1),
		     ERRCMD, err_460gx[i] & ~0x01);
    }

    /*
     * The 460GX spec says that any access to busses higher than CBN will be
     * master-aborted.  It seems possible however that this is not the case in
     * all 460GX implementations.  For now, limit the bus scan to CBN, unless
     * we have already found a higher bus number.
     */
    for (i = 0;  subno_460gx[i] < cbn_460gx;  ) {
	if (++i < 8)
	    continue;

	pciMaxBusNum = cbn_460gx + 1;
	break;
    }

    return TRUE;
}

/* This does some 460GX-related processing after the PCI bus scan */
void
xf86PostScan460GX(void)
{
    pciConfigPtr pPCI, *ppPCI;
    pciBusInfo_t *pBusInfo;
    int i, j, devno;

    if (cbn_460gx <= 0)
	return;

    /* Set up our extra bus functions */
    BusFuncs_460gx = *(pciBusInfo[0]->funcs);
    BusFuncs_460gx.pciControlBridge = Control460GXBridge;
    BusFuncs_460gx.pciGetBridgeBusses = Get460GXBridgeBusses;
    BusFuncs_460gx.pciGetBridgeResources = Get460GXBridgeResources;

    /*
     * Mark all host bridges so that they are ignored by the upper-level
     * xf86GetPciBridgeInfo() function.  This marking is later clobbered by the
     * tail end of xf86scanpci() for those bridges that actually have bus
     * segments associated with them.
     */
    ppPCI = xf86scanpci(0);	/* Recursion is only apparent */
    while ((pPCI = *ppPCI++)) {
	if ((pPCI->pci_base_class == PCI_CLASS_BRIDGE) &&
	    (pPCI->pci_sub_class == PCI_SUBCLASS_BRIDGE_HOST))
	    pPCI->businfo = HOST_NO_BUS;
    }

    ppPCI = xf86scanpci(0);	/* Recursion is only apparent */
    j = 0;

    /*
     * Fix up CBN bus linkage.  This is somewhat arbitrary.  The bridge chosen
     * for this must be a CBN device so that bus CBN can be recognised as the
     * root segment.  It also cannot be any of the bus expanders (devices
     * CBN:0x10:0 through CBN:0x17:0 nor any of their functions).  For now, we
     * chose the SAC host bridge at CBN:0:0.
     */
    pBusInfo = pciBusInfo[cbn_460gx];
    pBusInfo->bridge = pciBusInfo[0]->bridge;	/* Just in case */
    while ((pPCI = *ppPCI++)) {
	if (pPCI->busnum < cbn_460gx)
	    continue;
	if (pPCI->busnum > cbn_460gx)
	    break;
	if (pPCI->devnum < 0)
	    continue;
	if (pPCI->devnum > 0)
	    break;
	if (pPCI->funcnum < 0)
	    continue;
	if (pPCI->funcnum > 0)
	    break;

	pBusInfo->bridge = pPCI;
	pBusInfo->secondary = FALSE;
	pBusInfo->primary_bus = cbn_460gx;
	break;
    }

    for (i = 0, devno = 0x10;  devno <= 0x17;  i++, devno++) {
	/* Restore ERRCMD registers */
	if (err_460gx[i] & 0x01)
	    pciWriteByte(PCI_MAKE_TAG(cbn_460gx, devno, 1),
			 ERRCMD, err_460gx[i]);

	if (!(cbdevs_460gx & (1 << devno))) {
	    while ((pPCI = *ppPCI++)) {
		if (pPCI->busnum < cbn_460gx)
		    continue;
		if (pPCI->busnum > cbn_460gx)
		    break;
		if (pPCI->devnum < devno)
		    continue;
		if (pPCI->devnum > devno)
		    break;
		if (pPCI->funcnum < 0)
		    continue;
		if (pPCI->funcnum > 0)
		    break;

		if ((pBusInfo = pciBusInfo[busno_460gx[i]]))
		    break;

		/* Fix bus linkage */
		pBusInfo->bridge = pPCI;
		pBusInfo->secondary = TRUE;
		pBusInfo->primary_bus = cbn_460gx;

		/* Plug in chipset routines */
		pBusInfo->funcs = &BusFuncs_460gx;
		break;
	    }
	}

	/* Decode IOR registers */
	for(;  j <= (ior_460gx[i] & 0x0F);  j++)
	    iomap_460gx[j] = devno;
    }

    /* The bottom 4k of I/O space is always routed to PCI0a */
    iomap_460gx[0] = 0x10;

    /* Decode IORD register */
    for (j = 1;  j <= 0x0F;  j++)
	if (iord_460gx & (1 << j))
	    iomap_460gx[j] = 0x10;
}
