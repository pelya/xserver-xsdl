/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86pciBus.c,v 3.77 2003/11/03 05:11:03 tsi Exp $ */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
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

/*
 * This file contains the interfaces to the bus-specific code
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/X.h>
#include <pciaccess.h>
#include "os.h"
#include "Pci.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Resources.h"

/* Bus-specific headers */
#include "xf86Bus.h"

#define XF86_OS_PRIVS
#define NEED_OS_RAC_PROTOS
#include "xf86_OSproc.h"

#include "xf86RAC.h"

/* Bus-specific globals */
Bool pciSlotClaimed = FALSE;
static pciConfigPtr *xf86PciInfo = NULL;	/* Full PCI probe info */
struct pci_device ** xf86PciVideoInfo = NULL;	/* PCI probe for video hw */

/* PCI buses */
static PciBusPtr xf86PciBus = NULL;
/* Bus-specific probe/sorting functions */

/* PCI classes that get included in xf86PciVideoInfo */
#define PCIINFOCLASSES(c) \
    ( (((c) & 0x00ff0000) == (PCI_CLASS_PREHISTORIC << 16)) \
      || (((c) & 0x00ff0000) == (PCI_CLASS_DISPLAY << 16)) \
      || ((((c) & 0x00ffff00) \
	   == ((PCI_CLASS_MULTIMEDIA << 16) | (PCI_SUBCLASS_MULTIMEDIA_VIDEO << 8)))) \
      || ((((c) & 0x00ffff00) \
	   == ((PCI_CLASS_PROCESSOR << 16) | (PCI_SUBCLASS_PROCESSOR_COPROC << 8)))) )

/*
 * PCI classes that have messages printed always.  The others are only
 * have a message printed when the vendor/dev IDs are recognised.
 */
#define PCIALWAYSPRINTCLASSES(c) \
    ( (((c) & 0x00ffff00) \
       == ((PCI_CLASS_PREHISTORIC << 16) | (PCI_SUBCLASS_PREHISTORIC_VGA << 8))) \
      || (((c) & 0x00ff0000) == (PCI_CLASS_DISPLAY << 16)) \
      || ((((c) & 0x00ffff00) \
	   == ((PCI_CLASS_MULTIMEDIA << 16) | (PCI_SUBCLASS_MULTIMEDIA_VIDEO << 8)))) )

#define IS_VGA(c) \
    (((c) & 0x00ffff00) \
	 == ((PCI_CLASS_DISPLAY << 16) | (PCI_SUBCLASS_DISPLAY_VGA << 8)))

/*
 * PCI classes for which potentially destructive checking of the map sizes
 * may be done.  Any classes where this may be unsafe should be omitted
 * from this list.
 */
#define PCINONSYSTEMCLASSES(c) PCIALWAYSPRINTCLASSES(c)

/* 
 * PCI classes that use RAC 
 */
#define PCISHAREDIOCLASSES(c) \
    ( (((c) & 0x00ffff00) \
       == ((PCI_CLASS_PREHISTORIC << 16) | (PCI_SUBCLASS_PREHISTORIC_VGA << 8))) \
      || IS_VGA(c) )


#define B2M(tag,base) pciBusAddrToHostAddr(tag,PCI_MEM,base)
#define B2I(tag,base) pciBusAddrToHostAddr(tag,PCI_IO,base)

#define PCI_M_RANGE(range,tag,begin,end,type) \
	{ \
	    RANGE(range, B2M(tag, begin), B2M(tag, end), \
		  RANGE_TYPE(type, xf86GetPciDomain(tag))); \
	}
#define PCI_I_RANGE(range,tag,begin,end,type) \
	{ \
	    RANGE(range, B2I(tag, begin), B2I(tag, end), \
		  RANGE_TYPE(type, xf86GetPciDomain(tag))); \
	}

static PciBusPtr xf86GetPciBridgeInfo(void);

_X_EXPORT void
xf86FormatPciBusNumber(int busnum, char *buffer)
{
    /* 'buffer' should be at least 8 characters long */
    if (busnum < 256)
	sprintf(buffer, "%d", busnum);
    else
	sprintf(buffer, "%d@%d", busnum & 0x00ff, busnum >> 8);
}

static void
FindPCIVideoInfo(void)
{
    int i = 0, k;
    int num = 0;
    struct pci_device * info;
    struct pci_slot_match   m;
    struct pci_device_iterator * iter;


    xf86PciInfo = xf86scanpci(0);

    if (xf86PciInfo == NULL) {
	xf86PciVideoInfo = NULL;
	return;
    }

    xf86PciBus = xf86GetPciBridgeInfo();

    if ( (xf86IsolateDevice.bus != 0)
	 || (xf86IsolateDevice.device != 0) 
	 || (xf86IsolateDevice.func != 0) ) {
	m.domain = PCI_DOM_FROM_BUS( xf86IsolateDevice.bus );
	m.bus = PCI_BUS_NO_DOMAIN( xf86IsolateDevice.bus );
	m.dev = xf86IsolateDevice.device;
	m.func = xf86IsolateDevice.func;
    }
    else {
	m.domain = PCI_MATCH_ANY;
	m.bus = PCI_MATCH_ANY;
	m.dev = PCI_MATCH_ANY;
	m.func = PCI_MATCH_ANY;
    }

    iter = pci_slot_match_iterator_create( & m );

    while ( (info = pci_device_next( iter )) != NULL ) {
	if ( PCIINFOCLASSES( info->device_class ) ) {
	    num++;
	    xf86PciVideoInfo = xnfrealloc( xf86PciVideoInfo,
					   (sizeof( struct pci_device * ) 
					    * (num + 1)) );
	    xf86PciVideoInfo[num] = NULL;
	    xf86PciVideoInfo[num - 1] = info;

	    pci_device_probe(info);
	    info->user_data = 0;

#if 0 && defined(INCLUDE_XF86_NO_DOMAIN)
	    if ((PCISHAREDIOCLASSES( info->device_class ))
		&& (pcrp->pci_command & PCI_CMD_IO_ENABLE) 
		&& (pcrp->pci_prog_if == 0)) {
		int j;

		/*
		 * Attempt to ensure that VGA is actually routed to this
		 * adapter on entry.  This needs to be fixed when we finally
		 * grok host bridges (and multiple bus trees).
		 */
		j = pcrp->busnum;
		while (TRUE) {
		    PciBusPtr pBus = xf86PciBus;
		    while (pBus && j != pBus->secondary)
			pBus = pBus->next;
		    if (!pBus || !(pBus->brcontrol & PCI_PCI_BRIDGE_VGA_EN))
			break;
		    if (j == pBus->primary) {
			if (primaryBus.type == BUS_NONE) {
			    /* assumption: primary adapter is always VGA */
			    primaryBus.type = BUS_PCI;
			    primaryBus.id.pci.bus = pcrp->busnum;
			    primaryBus.id.pci.device = pcrp->devnum;
			    primaryBus.id.pci.func = pcrp->funcnum;
			} else if (primaryBus.type < BUS_last) {
			    xf86Msg(X_NOTICE,
				    "More than one primary device found\n");
			    primaryBus.type ^= (BusType)(-1);
			}
			break;
		    }
		    j = pBus->primary;
		}
	    }
#endif
	}
    }


    /* If we haven't found a primary device try a different heuristic */
    if (primaryBus.type == BUS_NONE && num) {
	for (i = 0;  i < num;  i++) {
	    uint16_t  command;

	    info = xf86PciVideoInfo[i];
	    pci_device_cfg_read_u16( info, & command, 4 );

	    if ( (command & PCI_CMD_MEM_ENABLE) 
		 && ((num == 1) || IS_VGA( info->device_class )) ) {
		if (primaryBus.type == BUS_NONE) {
		    primaryBus.type = BUS_PCI;
		    primaryBus.id.pci.bus = PCI_MAKE_BUS( info->domain, info->bus );
		    primaryBus.id.pci.device = info->dev;
		    primaryBus.id.pci.func = info->func;
		} else {
		    xf86Msg(X_NOTICE,
			    "More than one possible primary device found\n");
		    primaryBus.type ^= (BusType)(-1);
		}
	    }
	}
    }
    
    /* Print a summary of the video devices found */
    for (k = 0; k < num; k++) {
	const char *vendorname = NULL, *chipname = NULL;
	const char *prim = " ";
	Bool memdone = FALSE, iodone = FALSE;


	info = xf86PciVideoInfo[k];

	vendorname = pci_device_get_vendor_name( info );
	chipname = pci_device_get_device_name( info );

	if ((!vendorname || !chipname) &&
	    !PCIALWAYSPRINTCLASSES( info->device_class ))
	    continue;

	if (xf86IsPrimaryPci(info))
	    prim = "*";

	xf86Msg( X_PROBED, "PCI:%s(%u@%u:%u:%u) ", prim, info->domain,
		 info->bus, info->dev, info->func );

	if (vendorname)
	    xf86ErrorF("%s ", vendorname);
	else
	    xf86ErrorF("unknown vendor (0x%04x) ", info->vendor_id);

	if (chipname)
	    xf86ErrorF("%s ", chipname);
	else
	    xf86ErrorF("unknown chipset (0x%04x) ", info->device_id);

	xf86ErrorF("rev %d", info->revision);

	for (i = 0; i < 6; i++) {
	    struct pci_mem_region * r = & info->regions[i];

	    if ( r->size && ! r->is_IO ) {
		if (!memdone) {
		    xf86ErrorF(", Mem @ ");
		    memdone = TRUE;
		} else
		    xf86ErrorF(", ");
		xf86ErrorF("0x%08lx/%ld", r->base_addr, r->size);
	    }
	}

	for (i = 0; i < 6; i++) {
	    struct pci_mem_region * r = & info->regions[i];

	    if ( r->size && r->is_IO ) {
		if (!iodone) {
		    xf86ErrorF(", I/O @ ");
		    iodone = TRUE;
		} else
		    xf86ErrorF(", ");
		xf86ErrorF("0x%08lx/%ld", r->base_addr, r->size);
	    }
	}

	if ( info->rom_size ) {
	    xf86ErrorF(", BIOS @ 0x\?\?\?\?\?\?\?\?/%ld", info->rom_size);
	}

	xf86ErrorF("\n");
    }
}


/*
 * IO enable/disable related routines for PCI
 */
#define pArg ((pciArg*)arg)
#define SETBITS PCI_CMD_IO_ENABLE
static void
pciIoAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIoAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    pArg->ctrl |= SETBITS | PCI_CMD_MASTER_ENABLE;
    pci_device_cfg_write_u32( pArg->dev, & pArg->ctrl, PCI_CMD_STAT_REG );
}

static void
pciIoAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIoAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    pArg->ctrl &= ~SETBITS;
    pci_device_cfg_write_u32( pArg->dev, & pArg->ctrl, PCI_CMD_STAT_REG );
}

#undef SETBITS
#define SETBITS (PCI_CMD_IO_ENABLE | PCI_CMD_MEM_ENABLE)
static void
pciIo_MemAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIo_MemAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    pArg->ctrl |= SETBITS | PCI_CMD_MASTER_ENABLE;
    pci_device_cfg_write_u32( pArg->dev, & pArg->ctrl, PCI_CMD_STAT_REG );
}

static void
pciIo_MemAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciIo_MemAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    pArg->ctrl &= ~SETBITS;
    pci_device_cfg_write_u32( pArg->dev, & pArg->ctrl, PCI_CMD_STAT_REG );
}

#undef SETBITS
#define SETBITS (PCI_CMD_MEM_ENABLE)
static void
pciMemAccessEnable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciMemAccessEnable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    pArg->ctrl |= SETBITS | PCI_CMD_MASTER_ENABLE;
    pci_device_cfg_write_u32( pArg->dev, & pArg->ctrl, PCI_CMD_STAT_REG );
}

static void
pciMemAccessDisable(void* arg)
{
#ifdef DEBUG
    ErrorF("pciMemAccessDisable: 0x%05lx\n", *(PCITAG *)arg);
#endif
    pArg->ctrl &= ~SETBITS;
    pci_device_cfg_write_u32( pArg->dev, & pArg->ctrl, PCI_CMD_STAT_REG );
}
#undef SETBITS
#undef pArg


/* move to OS layer */
#define MASKBITS (PCI_PCI_BRIDGE_VGA_EN | PCI_PCI_BRIDGE_MASTER_ABORT_EN)
static void
pciBusAccessEnable(BusAccPtr ptr)
{
    struct pci_device * const dev = ptr->busdep.pci.dev;
    uint16_t ctrl;

#ifdef DEBUG
    ErrorF("pciBusAccessEnable: bus=%d\n", ptr->busdep.pci.bus);
#endif
    pci_device_cfg_read_u16( dev, & ctrl, PCI_PCI_BRIDGE_CONTROL_REG );
    if ((ctrl & MASKBITS) != PCI_PCI_BRIDGE_VGA_EN) {
	ctrl = (ctrl | PCI_PCI_BRIDGE_VGA_EN) &
	    ~(PCI_PCI_BRIDGE_MASTER_ABORT_EN | PCI_PCI_BRIDGE_SECONDARY_RESET);
	pci_device_cfg_write_u16( dev, & ctrl, PCI_PCI_BRIDGE_CONTROL_REG );
    }
}

/* move to OS layer */
static void
pciBusAccessDisable(BusAccPtr ptr)
{
    struct pci_device * const dev = ptr->busdep.pci.dev;
    uint16_t ctrl;

#ifdef DEBUG
    ErrorF("pciBusAccessDisable: bus=%d\n", ptr->busdep.pci.bus);
#endif
    pci_device_cfg_read_u16( dev, & ctrl, PCI_PCI_BRIDGE_CONTROL_REG );
    if (ctrl & MASKBITS) {
	ctrl &= ~(MASKBITS | PCI_PCI_BRIDGE_SECONDARY_RESET);
	pci_device_cfg_write_u16( dev, & ctrl, PCI_PCI_BRIDGE_CONTROL_REG );
    }
}
#undef MASKBITS

/* move to OS layer */
static void
pciDrvBusAccessEnable(BusAccPtr ptr)
{
    int bus = ptr->busdep.pci.bus;

#ifdef DEBUG
    ErrorF("pciDrvBusAccessEnable: bus=%d\n", bus);
#endif
    (*pciBusInfo[bus]->funcs->pciControlBridge)(bus,
						PCI_PCI_BRIDGE_VGA_EN,
						PCI_PCI_BRIDGE_VGA_EN);
}

/* move to OS layer */
static void
pciDrvBusAccessDisable(BusAccPtr ptr)
{
    int bus = ptr->busdep.pci.bus;

#ifdef DEBUG
    ErrorF("pciDrvBusAccessDisable: bus=%d\n", bus);
#endif
    (*pciBusInfo[bus]->funcs->pciControlBridge)(bus,
						PCI_PCI_BRIDGE_VGA_EN, 0);
}


static void
pciSetBusAccess(BusAccPtr ptr)
{
#ifdef DEBUG
    ErrorF("pciSetBusAccess: route VGA to bus %d\n", ptr->busdep.pci.bus);
#endif

    if (!ptr->primary && !ptr->current)
	return;
    
    if (ptr->current && ptr->current->disable_f)
	(*ptr->current->disable_f)(ptr->current);
    ptr->current = NULL;
    
    /* walk down */
    while (ptr->primary) {	/* No enable for root bus */
	if (ptr != ptr->primary->current) {
	    if (ptr->primary->current && ptr->primary->current->disable_f)
		(*ptr->primary->current->disable_f)(ptr->primary->current);
	    if (ptr->enable_f)
		(*ptr->enable_f)(ptr);
	    ptr->primary->current = ptr;
	}
	ptr = ptr->primary;
    }
}

/* move to OS layer */
static void
savePciState( struct pci_device * dev, pciSavePtr ptr )
{
    int i;

    pci_device_cfg_read_u32( dev, & ptr->command, PCI_CMD_STAT_REG );

    for ( i = 0; i < 6; i++ ) {
	pci_device_cfg_read_u32( dev, & ptr->base[i], 
				 PCI_CMD_BASE_REG + (i * 4) );
    }

    pci_device_cfg_read_u32( dev, & ptr->biosBase, PCI_CMD_BIOS_REG );
}

/* move to OS layer */
static void
restorePciState( struct pci_device * dev, pciSavePtr ptr)
{
    int i;
    
    /* disable card before setting anything */
    pci_device_cfg_write_bits( dev, PCI_CMD_MEM_ENABLE | PCI_CMD_IO_ENABLE, 0,
			       PCI_CMD_STAT_REG );

    pci_device_cfg_write_u32( dev, & ptr->biosBase, PCI_CMD_BIOS_REG );

    for ( i = 0; i < 6; i++ ) {
	pci_device_cfg_write_u32( dev, & ptr->base[i], 
				 PCI_CMD_BASE_REG + (i * 4) );
    }

    pci_device_cfg_write_u32( dev, & ptr->command, PCI_CMD_STAT_REG );
}

/* move to OS layer */
static void
savePciBusState(BusAccPtr ptr)
{
    struct pci_device * const dev = ptr->busdep.pci.dev;
    uint16_t temp;

    pci_device_cfg_read_u16( dev, & temp, PCI_PCI_BRIDGE_CONTROL_REG );
    ptr->busdep.pci.save.control = temp & ~PCI_PCI_BRIDGE_SECONDARY_RESET;

    /* Allow master aborts to complete normally on non-root buses */
    if ( ptr->busdep.pci.save.control & PCI_PCI_BRIDGE_MASTER_ABORT_EN ) {
	temp = ptr->busdep.pci.save.control & ~PCI_PCI_BRIDGE_MASTER_ABORT_EN;
	pci_device_cfg_read_u16( dev, & temp, PCI_PCI_BRIDGE_CONTROL_REG );
    }
}

/* move to OS layer */
#define MASKBITS (PCI_PCI_BRIDGE_VGA_EN | PCI_PCI_BRIDGE_MASTER_ABORT_EN)
static void
restorePciBusState(BusAccPtr ptr)
{
    struct pci_device * const dev = ptr->busdep.pci.dev;
    uint16_t ctrl;

    /* Only restore the bits we've changed (and don't cause resets) */
    pci_device_cfg_read_u16( dev, & ctrl, PCI_PCI_BRIDGE_CONTROL_REG );

    if ((ctrl ^ ptr->busdep.pci.save.control) & MASKBITS) {
	ctrl &= ~(MASKBITS | PCI_PCI_BRIDGE_SECONDARY_RESET);
	ctrl |= ptr->busdep.pci.save.control & MASKBITS;
	pci_device_cfg_write_u16( dev, & ctrl, PCI_PCI_BRIDGE_CONTROL_REG );
    }
}
#undef MASKBITS

/* move to OS layer */
static void
savePciDrvBusState(BusAccPtr ptr)
{
    int bus = ptr->busdep.pci.bus;

    ptr->busdep.pci.save.control =
	(*pciBusInfo[bus]->funcs->pciControlBridge)(bus, 0, 0);
    /* Allow master aborts to complete normally on this bus */
    (*pciBusInfo[bus]->funcs->pciControlBridge)(bus,
						PCI_PCI_BRIDGE_MASTER_ABORT_EN,
						0);
}

/* move to OS layer */
static void
restorePciDrvBusState(BusAccPtr ptr)
{
    int bus = ptr->busdep.pci.bus;

    (*pciBusInfo[bus]->funcs->pciControlBridge)(bus, (CARD16)(-1),
					        ptr->busdep.pci.save.control);
}


static void
disablePciBios(struct pci_device * dev)
{
    pci_device_cfg_write_bits(dev, PCI_CMD_BIOS_ENABLE, 0,
			       PCI_CMD_BIOS_REG);
}


/*
 * xf86Bus.c interface
 */

void
xf86PciProbe(void)
{
    FindPCIVideoInfo();
}

static void alignBridgeRanges(PciBusPtr PciBusBase, PciBusPtr primary);

static void
printBridgeInfo(PciBusPtr PciBus) 
{
    char primary[8], secondary[8], subordinate[8], brbus[8];

    xf86FormatPciBusNumber(PciBus->primary, primary);
    xf86FormatPciBusNumber(PciBus->secondary, secondary);
    xf86FormatPciBusNumber(PciBus->subordinate, subordinate);
    xf86FormatPciBusNumber(PciBus->brbus, brbus);

    xf86MsgVerb(X_INFO, 3, "Bus %s: bridge is at (%s:%d:%d), (%s,%s,%s),"
		" BCTRL: 0x%04x (VGA_EN is %s)\n",
		secondary, brbus, PciBus->brdev, PciBus->brfunc,
		primary, secondary, subordinate, PciBus->brcontrol,
		(PciBus->brcontrol & PCI_PCI_BRIDGE_VGA_EN) ?
		 "set" : "cleared");
    if (PciBus->preferred_io) {
	xf86MsgVerb(X_INFO, 3,
		    "Bus %s I/O range:\n", secondary);
	xf86PrintResList(3, PciBus->preferred_io);
    }
    if (PciBus->preferred_mem) {
	xf86MsgVerb(X_INFO, 3,
		    "Bus %s non-prefetchable memory range:\n", secondary);
	xf86PrintResList(3, PciBus->preferred_mem);
    }
    if (PciBus->preferred_pmem) {
	xf86MsgVerb(X_INFO, 3,
		    "Bus %s prefetchable memory range:\n", secondary);
	xf86PrintResList(3, PciBus->preferred_pmem);
    }
}

static PciBusPtr
xf86GetPciBridgeInfo(void)
{
    const pciConfigPtr *pcrpp;
    pciConfigPtr pcrp;
    pciBusInfo_t *pBusInfo;
    resRange range;
    PciBusPtr PciBus, PciBusBase = NULL;
    PciBusPtr *pnPciBus = &PciBusBase;
    int MaxBus = 0;
    int i, domain;
    int primary, secondary, subordinate;
    memType base, limit;

    resPtr pciBusAccWindows = xf86PciBusAccWindowsFromOS();

    if (xf86PciInfo == NULL)
	return NULL;

    /* Add each bridge */
    for (pcrpp = xf86PciInfo, pcrp = *pcrpp; pcrp; pcrp = *(++pcrpp)) {
	struct pci_device * const dev = 
	  pci_device_find_by_slot( PCI_DOM_FROM_BUS( pcrp->busnum ),
				   PCI_BUS_NO_DOMAIN( pcrp->busnum ),
				   pcrp->devnum, pcrp->funcnum );

	if (pcrp->busnum > MaxBus)
	    MaxBus = pcrp->busnum;

	if ( pcrp->pci_base_class == PCI_CLASS_BRIDGE ) {
	    const int sub_class = pcrp->pci_sub_class;

	    domain = xf86GetPciDomain(pcrp->tag);

	    switch (sub_class) {
	    case PCI_SUBCLASS_BRIDGE_PCI:
		/* something fishy about the header? If so: just ignore! */
		if ((pcrp->pci_header_type & 0x7f) != 0x01) {
		    xf86MsgVerb(X_WARNING, 3, "PCI-PCI bridge at %x:%x:%x has"
				" unexpected header:  0x%x",
				pcrp->busnum, pcrp->devnum,
				pcrp->funcnum, pcrp->pci_header_type);
		    break;
		}

		domain = pcrp->busnum & 0x0000FF00;
		primary = pcrp->busnum;
		secondary = domain | pcrp->pci_secondary_bus_number;
		subordinate = domain | pcrp->pci_subordinate_bus_number;

		/* Is this the correct bridge? If not, ignore it */
		pBusInfo = pcrp->businfo;
		if (pBusInfo && (pcrp != pBusInfo->bridge)) {
		    xf86MsgVerb(X_WARNING, 3, "PCI bridge mismatch for bus %x:"
				" %x:%x:%x and %x:%x:%x\n", secondary,
				pcrp->busnum, pcrp->devnum, pcrp->funcnum,
				pBusInfo->bridge->busnum,
				pBusInfo->bridge->devnum,
				pBusInfo->bridge->funcnum);
		    break;
		}

		if (pBusInfo && pBusInfo->funcs->pciGetBridgeBuses)
		    (*pBusInfo->funcs->pciGetBridgeBuses)(secondary,
							   &primary,
							   &secondary,
							   &subordinate);

		if (!pcrp->fakeDevice && (primary >= secondary)) {
		    xf86MsgVerb(X_WARNING, 3, "Misconfigured PCI bridge"
				" %x:%x:%x (%x,%x)\n",
				pcrp->busnum, pcrp->devnum, pcrp->funcnum,
				primary, secondary);
		    break;
		}
		
		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;

		PciBus->dev = dev;
		PciBus->primary = primary;
		PciBus->secondary = secondary;
		PciBus->subordinate = subordinate;

		PciBus->brbus = pcrp->busnum;
		PciBus->brdev = pcrp->devnum;
		PciBus->brfunc = pcrp->funcnum;

		PciBus->subclass = sub_class;

		/* The Intel bridges don't report as transparent
		   but guess what they are - from Linux kernel - airlied */
		if ((pcrp->pci_vendor == PCI_VENDOR_INTEL) && 
		   ((pcrp->_pci_device & 0xff00) == 0x2400)) {
			xf86MsgVerb(X_INFO, 3, "Intel Bridge workaround enabled\n");
			PciBus->interface = PCI_IF_BRIDGE_PCI_SUBTRACTIVE;
		} else {
			PciBus->interface = pcrp->pci_prog_if;
		}

		if (pBusInfo && pBusInfo->funcs->pciControlBridge)
		    PciBus->brcontrol =
			(*pBusInfo->funcs->pciControlBridge)(secondary, 0, 0);
		else
		    PciBus->brcontrol = pcrp->pci_bridge_control;

		if (pBusInfo && pBusInfo->funcs->pciGetBridgeResources) {
		    (*pBusInfo->funcs->pciGetBridgeResources)(secondary,
			(pointer *)&PciBus->preferred_io,
			(pointer *)&PciBus->preferred_mem,
			(pointer *)&PciBus->preferred_pmem);
		    break;
		}

		if ((pcrp->pci_command & PCI_CMD_IO_ENABLE) &&
		    (pcrp->pci_upper_io_base || pcrp->pci_io_base ||
		     pcrp->pci_upper_io_limit || pcrp->pci_io_limit)) {
		    base = (pcrp->pci_upper_io_base << 16) |
			((pcrp->pci_io_base & 0xf0u) << 8);
		    limit = (pcrp->pci_upper_io_limit << 16) |
			((pcrp->pci_io_limit & 0xf0u) << 8) | 0x0fff;
		    /*
		     * Deal with bridge ISA mode (256 wide ranges spaced 1K
		     * apart, but only in the first 64K).
		     */
		    if (pcrp->pci_bridge_control & PCI_PCI_BRIDGE_ISA_EN) {
			while ((base <= (CARD16)(-1)) && (base <= limit)) {
			    PCI_I_RANGE(range, pcrp->tag,
				base, base + (CARD8)(-1),
				ResIo | ResBlock | ResExclusive);
			    PciBus->preferred_io =
				xf86AddResToList(PciBus->preferred_io,
						 &range, -1);
			    base += 0x0400;
			}
		    }
		    if (base <= limit) {
			PCI_I_RANGE(range, pcrp->tag, base, limit,
			    ResIo | ResBlock | ResExclusive);
			PciBus->preferred_io =
			    xf86AddResToList(PciBus->preferred_io, &range, -1);
		    }
		}
		if (pcrp->pci_command & PCI_CMD_MEM_ENABLE) {
		  /*
		   * The P2P spec requires these next two, but some bridges
		   * don't comply.  Err on the side of caution, making the not
		   * so bold assumption that no bridge would ever re-route the
		   * bottom megabyte.
		   */
		  if (pcrp->pci_mem_base || pcrp->pci_mem_limit) {
                    base = pcrp->pci_mem_base & 0xfff0u;
                    limit = pcrp->pci_mem_limit & 0xfff0u;
		    if (base <= limit) {
			PCI_M_RANGE(range, pcrp->tag,
				    base << 16, (limit << 16) | 0x0fffff,
				    ResMem | ResBlock | ResExclusive);
			PciBus->preferred_mem =
			    xf86AddResToList(PciBus->preferred_mem, &range, -1);
		    }
		  }

		  if (pcrp->pci_prefetch_mem_base ||
		      pcrp->pci_prefetch_mem_limit ||
		      pcrp->pci_prefetch_upper_mem_base ||
		      pcrp->pci_prefetch_upper_mem_limit) {
                    base = pcrp->pci_prefetch_mem_base & 0xfff0u;
                    limit = pcrp->pci_prefetch_mem_limit & 0xfff0u;
#if defined(LONG64) || defined(WORD64)
		    base |= (memType)pcrp->pci_prefetch_upper_mem_base << 16;
		    limit |= (memType)pcrp->pci_prefetch_upper_mem_limit << 16;
#endif
		    if (base <= limit) {
			PCI_M_RANGE(range, pcrp->tag,
				    base << 16, (limit << 16) | 0xfffff,
				    ResMem | ResBlock | ResExclusive);
			PciBus->preferred_pmem =
			    xf86AddResToList(PciBus->preferred_pmem,
					     &range, -1);
		    }
		  }
		}
		break;

	    case PCI_SUBCLASS_BRIDGE_CARDBUS:
		/* something fishy about the header? If so: just ignore! */
		if ((pcrp->pci_header_type & 0x7f) != 0x02) {
		    xf86MsgVerb(X_WARNING, 3, "PCI-CardBus bridge at %x:%x:%x"
				" has unexpected header:  0x%x",
				pcrp->busnum, pcrp->devnum,
				pcrp->funcnum, pcrp->pci_header_type);
		    break;
		}

		domain = pcrp->busnum & 0x0000FF00;
		primary = pcrp->busnum;
		secondary = domain | pcrp->pci_cb_cardbus_bus_number;
		subordinate = domain | pcrp->pci_subordinate_bus_number;

		/* Is this the correct bridge?  If not, ignore it */
		pBusInfo = pcrp->businfo;
		if (pBusInfo && (pcrp != pBusInfo->bridge)) {
		    xf86MsgVerb(X_WARNING, 3, "CardBus bridge mismatch for bus"
				" %x: %x:%x:%x and %x:%x:%x\n", secondary,
				pcrp->busnum, pcrp->devnum, pcrp->funcnum,
				pBusInfo->bridge->busnum,
				pBusInfo->bridge->devnum,
				pBusInfo->bridge->funcnum);
		    break;
		}

		if (pBusInfo && pBusInfo->funcs->pciGetBridgeBuses)
		    (*pBusInfo->funcs->pciGetBridgeBuses)(secondary,
							   &primary,
							   &secondary,
							   &subordinate);

		if (primary >= secondary) {
		    if (pcrp->pci_cb_cardbus_bus_number != 0)
		        xf86MsgVerb(X_WARNING, 3, "Misconfigured CardBus"
				    " bridge %x:%x:%x (%x,%x)\n",
				    pcrp->busnum, pcrp->devnum, pcrp->funcnum,
				    primary, secondary);
		    break;
		}

		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;

		PciBus->dev = dev;
		PciBus->primary = primary;
		PciBus->secondary = secondary;
		PciBus->subordinate = subordinate;

		PciBus->brbus = pcrp->busnum;
		PciBus->brdev = pcrp->devnum;
		PciBus->brfunc = pcrp->funcnum;

		PciBus->subclass = sub_class;
		PciBus->interface = pcrp->pci_prog_if;

		if (pBusInfo && pBusInfo->funcs->pciControlBridge)
		    PciBus->brcontrol =
			(*pBusInfo->funcs->pciControlBridge)(secondary, 0, 0);
		else
		    PciBus->brcontrol = pcrp->pci_bridge_control;

		if (pBusInfo && pBusInfo->funcs->pciGetBridgeResources) {
		    (*pBusInfo->funcs->pciGetBridgeResources)(secondary,
			(pointer *)&PciBus->preferred_io,
			(pointer *)&PciBus->preferred_mem,
			(pointer *)&PciBus->preferred_pmem);
		    break;
		}

		if (pcrp->pci_command & PCI_CMD_IO_ENABLE) {
		    if (pcrp->pci_cb_iobase0) {
			base = PCI_CB_IOBASE(pcrp->pci_cb_iobase0);
			limit = PCI_CB_IOLIMIT(pcrp->pci_cb_iolimit0);

			/*
			 * Deal with bridge ISA mode (256-wide ranges spaced 1K
			 * apart (start to start), but only in the first 64K).
			 */
			if (pcrp->pci_bridge_control & PCI_PCI_BRIDGE_ISA_EN) {
			    while ((base <= (CARD16)(-1)) &&
				   (base <= limit)) {
				PCI_I_RANGE(range, pcrp->tag,
					    base, base + (CARD8)(-1),
					    ResIo | ResBlock | ResExclusive);
				PciBus->preferred_io =
				    xf86AddResToList(PciBus->preferred_io,
						     &range, -1);
				base += 0x0400;
			    }
			}

			if (base <= limit) {
			    PCI_I_RANGE(range, pcrp->tag, base, limit,
					ResIo | ResBlock | ResExclusive);
			    PciBus->preferred_io =
				xf86AddResToList(PciBus->preferred_io,
						 &range, -1);
			}
		    }

		    if (pcrp->pci_cb_iobase1) {
			base = PCI_CB_IOBASE(pcrp->pci_cb_iobase1);
			limit = PCI_CB_IOLIMIT(pcrp->pci_cb_iolimit1);

			/*
			 * Deal with bridge ISA mode (256-wide ranges spaced 1K
			 * apart (start to start), but only in the first 64K).
			 */
			if (pcrp->pci_bridge_control & PCI_PCI_BRIDGE_ISA_EN) {
			    while ((base <= (CARD16)(-1)) &&
				   (base <= limit)) {
				PCI_I_RANGE(range, pcrp->tag,
					    base, base + (CARD8)(-1),
					    ResIo | ResBlock | ResExclusive);
				PciBus->preferred_io =
				    xf86AddResToList(PciBus->preferred_io,
						     &range, -1);
				base += 0x0400;
			    }
			}

			if (base <= limit) {
			    PCI_I_RANGE(range, pcrp->tag, base, limit,
					ResIo | ResBlock | ResExclusive);
			    PciBus->preferred_io =
				xf86AddResToList(PciBus->preferred_io,
						 &range, -1);
			}
		    }
		}

		if (pcrp->pci_command & PCI_CMD_MEM_ENABLE) {
		    if ((pcrp->pci_cb_membase0) &&
			(pcrp->pci_cb_membase0 <= pcrp->pci_cb_memlimit0)) {
			PCI_M_RANGE(range, pcrp->tag,
				    pcrp->pci_cb_membase0 & ~0x0fff,
				    pcrp->pci_cb_memlimit0 | 0x0fff,
				    ResMem | ResBlock | ResExclusive);
			if (pcrp->pci_bridge_control &
			    PCI_CB_BRIDGE_CTL_PREFETCH_MEM0)
			    PciBus->preferred_pmem =
				xf86AddResToList(PciBus->preferred_pmem,
						 &range, -1);
			else
			    PciBus->preferred_mem =
				xf86AddResToList(PciBus->preferred_mem,
						 &range, -1);
		    }
		    if ((pcrp->pci_cb_membase1) &&
			(pcrp->pci_cb_membase1 <= pcrp->pci_cb_memlimit1)) {
			PCI_M_RANGE(range, pcrp->tag,
				    pcrp->pci_cb_membase1 & ~0x0fff,
				    pcrp->pci_cb_memlimit1 | 0x0fff,
				    ResMem | ResBlock | ResExclusive);
			if (pcrp->pci_bridge_control &
			    PCI_CB_BRIDGE_CTL_PREFETCH_MEM1)
			    PciBus->preferred_pmem =
				xf86AddResToList(PciBus->preferred_pmem,
						 &range, -1);
			else
			    PciBus->preferred_mem =
				xf86AddResToList(PciBus->preferred_mem,
						 &range, -1);
		    }
		}

		break;

	    case PCI_SUBCLASS_BRIDGE_ISA:
	    case PCI_SUBCLASS_BRIDGE_EISA:
	    case PCI_SUBCLASS_BRIDGE_MC:
		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;
		PciBus->dev = dev;
		PciBus->primary = pcrp->busnum;
		PciBus->secondary = PciBus->subordinate = -1;
		PciBus->brbus = pcrp->busnum;
		PciBus->brdev = pcrp->devnum;
		PciBus->brfunc = pcrp->funcnum;
		PciBus->subclass = sub_class;
		PciBus->brcontrol = PCI_PCI_BRIDGE_VGA_EN;
		break;

	    case PCI_SUBCLASS_BRIDGE_HOST:
		/* Is this the correct bridge?  If not, ignore bus info */
		pBusInfo = pcrp->businfo;

		if (!pBusInfo || pBusInfo == HOST_NO_BUS)
		    break;

		secondary = 0;
		/* Find "secondary" bus segment */
		while (pBusInfo != pciBusInfo[secondary])
			secondary++;
		if (pcrp != pBusInfo->bridge) {
		    xf86MsgVerb(X_WARNING, 3, "Host bridge mismatch for"
				" bus %x: %x:%x:%x and %x:%x:%x\n",
				pBusInfo->primary_bus,
				pcrp->busnum, pcrp->devnum, pcrp->funcnum,
				pBusInfo->bridge->busnum,
				pBusInfo->bridge->devnum,
				pBusInfo->bridge->funcnum);
		    pBusInfo = NULL;
		}

		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;

		PciBus->dev = dev;
		PciBus->primary = PciBus->secondary = secondary;
		PciBus->subordinate = pciNumBuses - 1;

		if (pBusInfo->funcs->pciGetBridgeBuses)
		    (*pBusInfo->funcs->pciGetBridgeBuses)
		        (secondary,
			   &PciBus->primary,
			   &PciBus->secondary,
			   &PciBus->subordinate);

		PciBus->brbus = pcrp->busnum;
		PciBus->brdev = pcrp->devnum;
		PciBus->brfunc = pcrp->funcnum;

		PciBus->subclass = sub_class;

		if (pBusInfo && pBusInfo->funcs->pciControlBridge)
		    PciBus->brcontrol =
			(*pBusInfo->funcs->pciControlBridge)(secondary, 0, 0);
		else
		    PciBus->brcontrol = PCI_PCI_BRIDGE_VGA_EN;

		if (pBusInfo && pBusInfo->funcs->pciGetBridgeResources) {
		    (*pBusInfo->funcs->pciGetBridgeResources)
			(secondary,
			 (pointer *)&PciBus->preferred_io,
			 (pointer *)&PciBus->preferred_mem,
			 (pointer *)&PciBus->preferred_pmem);
		    break;
		}

		PciBus->preferred_io =
		    xf86ExtractTypeFromList(pciBusAccWindows,
					    RANGE_TYPE(ResIo, domain));
		PciBus->preferred_mem =
		    xf86ExtractTypeFromList(pciBusAccWindows,
					    RANGE_TYPE(ResMem, domain));
		PciBus->preferred_pmem =
		    xf86ExtractTypeFromList(pciBusAccWindows,
					    RANGE_TYPE(ResMem, domain));
		break;

	    default:
		break;
	    }
	}
    }
    for (i = 0; i <= MaxBus; i++) { /* find PCI buses not attached to bridge */
	if (!pciBusInfo[i])
	    continue;
	for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next)
	    if (PciBus->secondary == i) break;
	if (!PciBus) {  /* We assume it's behind a HOST-PCI bridge */
	    /*
	     * Find the 'smallest' free HOST-PCI bridge, where 'small' is in
	     * the order of pciTag().
	     */
	    PCITAG minTag = 0xFFFFFFFF;
	    PciBusPtr PciBusFound = NULL;

	    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next) {
		const PCITAG tag = pciTag( PciBus->brbus, PciBus->brdev,
					   PciBus->brfunc );
		if ((PciBus->subclass == PCI_SUBCLASS_BRIDGE_HOST) &&
		    (PciBus->secondary == -1) &&
		    (tag < minTag) )  {
		    minTag = tag;
		    PciBusFound = PciBus;
		}
	    }

	    if (PciBusFound)
		PciBusFound->secondary = i;
	    else {  /* if nothing found it may not be visible: create new */
		/* Find a device on this bus */
		domain = 0;
		for (pcrpp = xf86PciInfo;  (pcrp = *pcrpp);  pcrpp++) {
		    if (pcrp->busnum == i) {
			domain = xf86GetPciDomain(pcrp->tag);
			break;
		    }
		}
		*pnPciBus = PciBus = xnfcalloc(1, sizeof(PciBusRec));
		pnPciBus = &PciBus->next;

		PciBus->dev = pci_device_find_by_slot( PCI_DOM_FROM_BUS( pcrp->busnum ),
						       PCI_BUS_NO_DOMAIN( pcrp->busnum ),
						       pcrp->devnum,
						       pcrp->funcnum );
		PciBus->primary = PciBus->secondary = i;
		PciBus->subclass = PCI_SUBCLASS_BRIDGE_HOST;
		PciBus->brcontrol = PCI_PCI_BRIDGE_VGA_EN;
		PciBus->preferred_io =
		    xf86ExtractTypeFromList(pciBusAccWindows,
					    RANGE_TYPE(ResIo, domain));
		PciBus->preferred_mem =
		    xf86ExtractTypeFromList(pciBusAccWindows,
					    RANGE_TYPE(ResMem, domain));
		PciBus->preferred_pmem =
		    xf86ExtractTypeFromList(pciBusAccWindows,
					    RANGE_TYPE(ResMem, domain));
	    }
	}
    }

    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next) {
	if (PciBus->primary == PciBus->secondary) {
	    alignBridgeRanges(PciBusBase, PciBus);
	}
    }

    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next) {
	switch (PciBus->subclass) {
	    case PCI_SUBCLASS_BRIDGE_PCI:
		if (PciBus->interface == PCI_IF_BRIDGE_PCI_SUBTRACTIVE)
		    xf86MsgVerb(X_INFO, 3, "Subtractive PCI-to-PCI bridge:\n");
		else
		    xf86MsgVerb(X_INFO, 3, "PCI-to-PCI bridge:\n");
		break;
	    case PCI_SUBCLASS_BRIDGE_CARDBUS:
		xf86MsgVerb(X_INFO, 3, "PCI-to-CardBus bridge:\n");
		break;
	    case PCI_SUBCLASS_BRIDGE_HOST:
		xf86MsgVerb(X_INFO, 3, "Host-to-PCI bridge:\n");
		break;
	    case PCI_SUBCLASS_BRIDGE_ISA:
		xf86MsgVerb(X_INFO, 3, "PCI-to-ISA bridge:\n");
		break;
	    case PCI_SUBCLASS_BRIDGE_EISA:
		xf86MsgVerb(X_INFO, 3, "PCI-to-EISA bridge:\n");
		break;
	    case PCI_SUBCLASS_BRIDGE_MC:
		xf86MsgVerb(X_INFO, 3, "PCI-to-MCA bridge:\n");
		break;
	    default:
		break;
	}
	printBridgeInfo(PciBus);
    }
    xf86FreeResList(pciBusAccWindows);
    return PciBusBase;
}

static void
alignBridgeRanges(PciBusPtr PciBusBase, PciBusPtr primary)
{
    PciBusPtr PciBus;

    for (PciBus = PciBusBase; PciBus; PciBus = PciBus->next) {
	if ((PciBus != primary) && (PciBus->primary != -1)
	    && (PciBus->primary == primary->secondary)) {
	    resPtr tmp;
	    tmp = xf86FindIntersectOfLists(primary->preferred_io,
					   PciBus->preferred_io);
	    xf86FreeResList(PciBus->preferred_io);
	    PciBus->preferred_io = tmp;
	    tmp = xf86FindIntersectOfLists(primary->preferred_pmem,
					   PciBus->preferred_pmem);
	    xf86FreeResList(PciBus->preferred_pmem);
	    PciBus->preferred_pmem = tmp;
	    tmp = xf86FindIntersectOfLists(primary->preferred_mem,
					   PciBus->preferred_mem);
	    xf86FreeResList(PciBus->preferred_mem);
	    PciBus->preferred_mem = tmp;

	    /* Deal with subtractive decoding */
	    switch (PciBus->subclass) {
	    case PCI_SUBCLASS_BRIDGE_PCI:
		if (PciBus->interface != PCI_IF_BRIDGE_PCI_SUBTRACTIVE)
		    break;
		/* Fall through */
#if 0	/* Not yet */
	    case PCI_SUBCLASS_BRIDGE_ISA:
	    case PCI_SUBCLASS_BRIDGE_EISA:
	    case PCI_SUBCLASS_BRIDGE_MC:
#endif
		if (!(PciBus->io = primary->io))
		    PciBus->io = primary->preferred_io;
		if (!(PciBus->mem = primary->mem))
		    PciBus->mem = primary->preferred_mem;
		if (!(PciBus->pmem = primary->pmem))
		    PciBus->pmem = primary->preferred_pmem;
	    default:
		break;
	    }

	    alignBridgeRanges(PciBusBase, PciBus);
	}
    }
}

void
initPciState(void)
{
    unsigned i;
    pciAccPtr pcaccp;

    if (xf86PciVideoInfo == NULL) {
	return;
    }

    for (i = 0 ; xf86PciVideoInfo[i] != NULL ; i++) {
	struct pci_device * const pvp = xf86PciVideoInfo[i];

	if (pvp->user_data == 0) {
	    pcaccp = xnfalloc( sizeof( pciAccRec ) );
	    pvp->user_data = (intptr_t) pcaccp;

	    pcaccp->busnum = PCI_MAKE_BUS(pvp->domain, pvp->bus);
	    pcaccp->devnum = pvp->dev; 
	    pcaccp->funcnum = pvp->func;
	    pcaccp->arg.dev = pvp;
	    pcaccp->ioAccess.AccessDisable = pciIoAccessDisable;
	    pcaccp->ioAccess.AccessEnable = pciIoAccessEnable;
	    pcaccp->ioAccess.arg = &pcaccp->arg;
	    pcaccp->io_memAccess.AccessDisable = pciIo_MemAccessDisable;
	    pcaccp->io_memAccess.AccessEnable = pciIo_MemAccessEnable;
	    pcaccp->io_memAccess.arg = &pcaccp->arg;
	    pcaccp->memAccess.AccessDisable = pciMemAccessDisable;
	    pcaccp->memAccess.AccessEnable = pciMemAccessEnable;
	    pcaccp->memAccess.arg = &pcaccp->arg;

	    pcaccp->ctrl = PCISHAREDIOCLASSES(pvp->device_class);

	    savePciState(pvp, &pcaccp->save);
	    pcaccp->arg.ctrl = pcaccp->save.command;
	}
    }
}

/*
 * initPciBusState() - fill out the BusAccRec for a PCI bus.
 * Theory: each bus is associated with one bridge connecting it
 * to its parent bus. The address of a bridge is therefore stored
 * in the BusAccRec of the bus it connects to. Each bus can
 * have several bridges connecting secondary buses to it. Only one
 * of these bridges can be open. Therefore the status of a bridge
 * associated with a bus is stored in the BusAccRec of the parent
 * the bridge connects to. The first member of the structure is
 * a pointer to a function that open access to this bus. This function
 * receives a pointer to the structure itself as argument. This
 * design should be common to BusAccRecs of any type of buses we
 * support. The remeinder of the structure is bus type specific.
 * In this case it contains a pointer to the structure of the
 * parent bus. Thus enabling access to a specific bus is simple:
 * 1. Close any bridge going to secondary buses.
 * 2. Climb down the ladder and enable any bridge on buses
 *    on the path from the CPU to this bus.
 */
 
void
initPciBusState(void)
{
    BusAccPtr pbap, pbap_tmp;
    PciBusPtr pbp = xf86PciBus;
    pciBusInfo_t *pBusInfo;

    while (pbp) {
	pbap = xnfcalloc(1,sizeof(BusAccRec));
	pbap->busdep.pci.bus = pbp->secondary;
	pbap->busdep.pci.primary_bus = pbp->primary;
	pbap->busdep_type = BUS_PCI;
	pbap->busdep.pci.dev = NULL;

	if ((pbp->secondary >= 0) && (pbp->secondary < pciNumBuses) &&
	    (pBusInfo = pciBusInfo[pbp->secondary]) &&
	    pBusInfo->funcs->pciControlBridge) {
	    pbap->type = BUS_PCI;
	    pbap->save_f = savePciDrvBusState;
	    pbap->restore_f = restorePciDrvBusState;
	    pbap->set_f = pciSetBusAccess;
	    pbap->enable_f = pciDrvBusAccessEnable;
	    pbap->disable_f = pciDrvBusAccessDisable;
	    savePciDrvBusState(pbap);
	} else switch (pbp->subclass) {
	case PCI_SUBCLASS_BRIDGE_HOST:
	    pbap->type = BUS_PCI;
	    pbap->set_f = pciSetBusAccess;
	    break;
	case PCI_SUBCLASS_BRIDGE_PCI:
	case PCI_SUBCLASS_BRIDGE_CARDBUS:
	    pbap->type = BUS_PCI;
	    pbap->save_f = savePciBusState;
	    pbap->restore_f = restorePciBusState;
	    pbap->set_f = pciSetBusAccess;
	    pbap->enable_f = pciBusAccessEnable;
	    pbap->disable_f = pciBusAccessDisable;
	    pbap->busdep.pci.dev = pci_device_find_by_slot(PCI_DOM_FROM_BUS(pbp->brbus),
							   PCI_BUS_NO_DOMAIN(pbp->brbus),
							   pbp->brdev,
							   pbp->brfunc);
	    savePciBusState(pbap);
	    break;
	case PCI_SUBCLASS_BRIDGE_ISA:
	case PCI_SUBCLASS_BRIDGE_EISA:
	case PCI_SUBCLASS_BRIDGE_MC:
	    pbap->type = BUS_ISA;
	    pbap->set_f = pciSetBusAccess;
	    break;
	}
	pbap->next = xf86BusAccInfo;
	xf86BusAccInfo = pbap;
	pbp = pbp->next;
    }

    pbap = xf86BusAccInfo;

    while (pbap) {
	pbap->primary = NULL;
	if (pbap->busdep_type == BUS_PCI
	    && pbap->busdep.pci.primary_bus > -1) {
	    pbap_tmp = xf86BusAccInfo;
	    while (pbap_tmp) {
		if (pbap_tmp->busdep_type == BUS_PCI &&
		    pbap_tmp->busdep.pci.bus == pbap->busdep.pci.primary_bus) {
		    /* Don't create loops */
		    if (pbap == pbap_tmp)
			break;
		    pbap->primary = pbap_tmp;
		    break;
		}
		pbap_tmp = pbap_tmp->next;
	    }
	}
	pbap = pbap->next;
    }
}

void 
PciStateEnter(void)
{
    unsigned i;

    if (xf86PciVideoInfo == NULL)
	return;

    for ( i = 0 ; xf86PciVideoInfo[i] != NULL ; i++ ) {
	pciAccPtr paccp = (pciAccPtr) xf86PciVideoInfo[i]->user_data;

 	if ( (paccp != NULL) && paccp->ctrl ) {
	    savePciState(paccp->arg.dev, &paccp->save);
	    restorePciState(paccp->arg.dev, &paccp->restore);
	    paccp->arg.ctrl = paccp->restore.command;
	}
    }
}

void
PciBusStateEnter(void)
{
    BusAccPtr pbap = xf86BusAccInfo;

    while (pbap) {
	if (pbap->save_f)
	    pbap->save_f(pbap);
	pbap = pbap->next;
    }
}

void 
PciStateLeave(void)
{
    unsigned i;

    if (xf86PciVideoInfo == NULL)
	return;

    for ( i = 0 ; xf86PciVideoInfo[i] != NULL ; i++ ) {
	pciAccPtr paccp = (pciAccPtr) xf86PciVideoInfo[i]->user_data;

 	if ( (paccp != NULL) && paccp->ctrl ) {
	    savePciState(paccp->arg.dev, &paccp->restore);
	    restorePciState(paccp->arg.dev, &paccp->save);
	}
    }
}

void
PciBusStateLeave(void)
{
    BusAccPtr pbap = xf86BusAccInfo;

    while (pbap) {
	if (pbap->restore_f)
	    pbap->restore_f(pbap);
	pbap = pbap->next;
    }
}

void 
DisablePciAccess(void)
{
    unsigned i;

    if (xf86PciVideoInfo == NULL)
	return;

    for ( i = 0 ; xf86PciVideoInfo[i] != NULL ; i++ ) {
	pciAccPtr paccp = (pciAccPtr) xf86PciVideoInfo[i]->user_data;

 	if ( (paccp != NULL) && paccp->ctrl ) {
	    pciIo_MemAccessDisable(paccp->io_memAccess.arg);
	}
    }
}

void
DisablePciBusAccess(void)
{
    BusAccPtr pbap = xf86BusAccInfo;

    while (pbap) {
	if (pbap->disable_f)
	    pbap->disable_f(pbap);
	if (pbap->primary)
	    pbap->primary->current = NULL;
	pbap = pbap->next;
    }
}

/*
 * If the slot requested is already in use, return -1.
 * Otherwise, claim the slot for the screen requesting it.
 */

_X_EXPORT int
xf86ClaimPciSlot(struct pci_device * d, DriverPtr drvp,
		 int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p = NULL;
    pciAccPtr paccp = (pciAccPtr) d->user_data;
    BusAccPtr pbap = xf86BusAccInfo;
    const unsigned bus = PCI_MAKE_BUS(d->domain, d->bus);
    
    int num;
    
    if (xf86CheckPciSlot(d)) {
	num = xf86AllocateEntity();
	p = xf86Entities[num];
	p->driver = drvp;
	p->chipset = chipset;
	p->busType = BUS_PCI;
	p->pciBusId.bus = bus;
	p->pciBusId.device = d->dev;
	p->pciBusId.func = d->func;
	p->active = active;
	p->inUse = FALSE;
	if (dev)
            xf86AddDevToEntity(num, dev);
	/* Here we initialize the access structure */
	p->access = xnfcalloc(1,sizeof(EntityAccessRec));
	if (paccp != NULL) {
	    p->access->fallback = & paccp->io_memAccess;
	    p->access->pAccess = & paccp->io_memAccess;
	    paccp->ctrl = TRUE; /* mark control if not already */
	}
	else {
	    p->access->fallback = &AccessNULL;
	    p->access->pAccess = &AccessNULL;
	}
	
	p->busAcc = NULL;
	while (pbap) {
	    if (pbap->type == BUS_PCI && pbap->busdep.pci.bus == bus)
		p->busAcc = pbap;
	    pbap = pbap->next;
	}

	/* in case bios is enabled disable it */
	disablePciBios( d );
	pciSlotClaimed = TRUE;

	if (active) {
	    /* Map in this domain's I/O space */
	   p->domainIO = xf86MapLegacyIO(dev);
	}
	
 	return num;
    } else
 	return -1;
}

/*
 * Parse a BUS ID string, and return the PCI bus parameters if it was
 * in the correct format for a PCI bus id.
 */

_X_EXPORT Bool
xf86ParsePciBusString(const char *busID, int *bus, int *device, int *func)
{
    /*
     * The format is assumed to be "bus[@domain]:device[:func]", where domain,
     * bus, device and func are decimal integers.  domain and func may be
     * omitted and assumed to be zero, although doing this isn't encouraged.
     */

    char *p, *s, *d;
    const char *id;
    int i;

    if (StringToBusType(busID, &id) != BUS_PCI)
	return FALSE;

    s = xstrdup(id);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return FALSE;
    }
    d = strpbrk(p, "@");
    if (d != NULL) {
	*(d++) = 0;
	for (i = 0; d[i] != 0; i++) {
	    if (!isdigit(d[i])) {
		xfree(s);
		return FALSE;
	    }
	}
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *bus = atoi(p);
    if (d != NULL && *d != 0)
	*bus += atoi(d) << 8;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return FALSE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *device = atoi(p);
    *func = 0;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	xfree(s);
	return TRUE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    xfree(s);
	    return FALSE;
	}
    }
    *func = atoi(p);
    xfree(s);
    return TRUE;
}

/*
 * Compare a BUS ID string with a PCI bus id.  Return TRUE if they match.
 */

_X_EXPORT Bool
xf86ComparePciBusString(const char *busID, int bus, int device, int func)
{
    int ibus, idevice, ifunc;

    if (xf86ParsePciBusString(busID, &ibus, &idevice, &ifunc)) {
	return bus == ibus && device == idevice && func == ifunc;
    } else {
	return FALSE;
    }
}

/*
 * xf86IsPrimaryPci() -- return TRUE if primary device
 * is PCI and bus, dev and func numbers match.
 */
 
_X_EXPORT Bool
xf86IsPrimaryPci( struct pci_device * pPci )
{
    const unsigned busnum = PCI_MAKE_BUS( pPci->domain, pPci->bus );

    return ((primaryBus.type == BUS_PCI)
	    && (busnum == primaryBus.id.pci.bus)
	    && (pPci->dev == primaryBus.id.pci.device)
	    && (pPci->func == primaryBus.id.pci.func));
}

/*
 * xf86GetPciInfoForEntity() -- Get the pciVideoRec of entity.
 */
_X_EXPORT struct pci_device *
xf86GetPciInfoForEntity(int entityIndex)
{
    EntityPtr p;
    
    if (entityIndex >= xf86NumEntities)
	return NULL;

    p = xf86Entities[entityIndex];
    if (p->busType == BUS_PCI) {
	const unsigned domain = PCI_DOM_FROM_BUS(p->pciBusId.bus);
	const unsigned bus = PCI_BUS_NO_DOMAIN(p->pciBusId.bus);
	struct pci_device ** ppPci;

	for (ppPci = xf86PciVideoInfo; *ppPci != NULL; ppPci++) {
	    if (domain == (*ppPci)->domain &&
		bus == (*ppPci)->bus &&
		p->pciBusId.device == (*ppPci)->dev &&
		p->pciBusId.func == (*ppPci)->func) 
	      return (*ppPci);
	}
    }

    return NULL;
}

_X_EXPORT int
xf86GetPciEntity(int bus, int dev, int func)
{
    int i;
    
    for (i = 0; i < xf86NumEntities; i++) {
	EntityPtr p = xf86Entities[i];
	if (p->busType != BUS_PCI) continue;
	
	if (p->pciBusId.bus == bus &&
	    p->pciBusId.device == dev &&
	    p->pciBusId.func == func) 
	    return i;
    }
    return -1;
}

/*
 * xf86CheckPciMemBase() checks that the memory base value matches one of the
 * PCI base address register values for the given PCI device.
 */
_X_EXPORT Bool
xf86CheckPciMemBase( struct pci_device * pPci, memType base )
{
    int i;

    for (i = 0; i < 6; i++)
	if (base == pPci->regions[i].base_addr)
	    return TRUE;
    return FALSE;
}

/*
 * Check if the slot requested is free.  If it is already in use, return FALSE.
 */

_X_EXPORT Bool
xf86CheckPciSlot( const struct pci_device * d )
{
    int i;
    EntityPtr p;
    const unsigned busnum = PCI_MAKE_BUS(d->domain, d->bus);

    for (i = 0; i < xf86NumEntities; i++) {
	p = xf86Entities[i];
	/* Check if this PCI slot is taken */
	if (p->busType == BUS_PCI && p->pciBusId.bus == busnum &&
	    p->pciBusId.device == d->dev && p->pciBusId.func == d->func)
	    return FALSE;
    }
    
    return TRUE;
}


/*
 * xf86FindPciVendorDevice() xf86FindPciClass(): These functions
 * are meant to be used by the pci bios emulation. Some bioses
 * need to see if there are _other_ chips of the same type around
 * so by setting pvp_exclude one pci device can be explicitely
 * _excluded if required.
 */
_X_EXPORT struct pci_device *
xf86FindPciDeviceVendor( CARD16 vendorID, CARD16 deviceID,
			 char n, const struct pci_device * exclude )
{
    struct pci_device * pvp;
    struct pci_device ** ppvp;

    n++;

    for (ppvp = xf86PciVideoInfo, pvp =*ppvp; pvp ; pvp = *(++ppvp)) {
	if ( (pvp != exclude) && (pvp->vendor_id == vendorID)
	     && (pvp->device_id == deviceID) ) {
	    if (!(--n)) break;
	}
    }

    return pvp;
}

_X_EXPORT struct pci_device *
xf86FindPciClass(CARD8 intf, CARD8 subClass, CARD16 _class,
		 char n, const struct pci_device * exclude)
{
    struct pci_device * pvp;
    struct pci_device ** ppvp;
    const uint32_t device_class = ( ((uint32_t)_class) << 16) 
      | ( ((uint32_t)subClass) << 8) | intf;

    n++;
    
    for (ppvp = xf86PciVideoInfo, pvp =*ppvp; pvp ; pvp = *(++ppvp)) {
	if ( (pvp != exclude) && (pvp->device_class == device_class) ) {
	    if (!(--n)) break;
	}
    }

    return pvp;
}

static void
pciTagConvertRange2Host(PCITAG tag, resRange *pRange)
{
    if (!(pRange->type & ResBus))
	return;

    switch(pRange->type & ResPhysMask) {
    case ResMem:
	switch(pRange->type & ResExtMask) {
	case ResBlock:
	    pRange->rBegin = pciBusAddrToHostAddr(tag,PCI_MEM, pRange->rBegin);
	    pRange->rEnd = pciBusAddrToHostAddr(tag,PCI_MEM, pRange->rEnd);
	    break;
	case ResSparse:
	    pRange->rBase = pciBusAddrToHostAddr(tag,PCI_MEM_SPARSE_BASE,
						  pRange->rBegin);
	    pRange->rMask = pciBusAddrToHostAddr(tag,PCI_MEM_SPARSE_MASK,
						pRange->rEnd);
	    break;
	}
	break;
    case ResIo:
	switch(pRange->type & ResExtMask) {
	case ResBlock:
	    pRange->rBegin = pciBusAddrToHostAddr(tag,PCI_IO, pRange->rBegin);
	    pRange->rEnd = pciBusAddrToHostAddr(tag,PCI_IO, pRange->rEnd);
	    break;
	case ResSparse:
	    pRange->rBase = pciBusAddrToHostAddr(tag,PCI_IO_SPARSE_BASE
						  , pRange->rBegin);
	    pRange->rMask = pciBusAddrToHostAddr(tag,PCI_IO_SPARSE_MASK
						, pRange->rEnd);
	    break;
	}
	break;
    }

    /* Set domain number */
    pRange->type &= ~(ResDomain | ResBus);
    pRange->type |= xf86GetPciDomain(tag) << 24;
}

void
pciConvertRange2Host(int entityIndex, resRange *pRange)
{
    const struct pci_device * const pvp = xf86GetPciInfoForEntity(entityIndex);

    if ( pvp != NULL ) {
	const PCITAG tag = PCI_MAKE_TAG( PCI_MAKE_BUS( pvp->domain, pvp->bus ),
					 pvp->dev, pvp->func );
	pciTagConvertRange2Host(tag, pRange);
    }
}
