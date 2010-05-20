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

/* Bus-specific headers */
#include "xf86Bus.h"

#define XF86_OS_PRIVS
#include "xf86_OSproc.h"


/* Bus-specific globals */
Bool pciSlotClaimed = FALSE;

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

void
xf86FormatPciBusNumber(int busnum, char *buffer)
{
    /* 'buffer' should be at least 8 characters long */
    if (busnum < 256)
	sprintf(buffer, "%d", busnum);
    else
	sprintf(buffer, "%d@%d", busnum & 0x00ff, busnum >> 8);
}

/*
 * xf86Bus.c interface
 */

void
xf86PciProbe(void)
{
    int i = 0, k;
    int num = 0;
    struct pci_device *info;
    struct pci_device_iterator *iter;
    struct pci_device ** xf86PciVideoInfo = NULL;


    if (!xf86scanpci()) {
	xf86PciVideoInfo = NULL;
	return;
    }

    iter = pci_slot_match_iterator_create(& xf86IsolateDevice);
    while ((info = pci_device_next(iter)) != NULL) {
	if (PCIINFOCLASSES(info->device_class)) {
	    num++;
	    xf86PciVideoInfo = xnfrealloc(xf86PciVideoInfo,
					  (sizeof(struct pci_device *)
					   * (num + 1)));
	    xf86PciVideoInfo[num] = NULL;
	    xf86PciVideoInfo[num - 1] = info;

	    pci_device_probe(info);
#ifdef HAVE_PCI_DEVICE_IS_BOOT_VGA
	    if (pci_device_is_boot_vga(info)) {
                primaryBus.type = BUS_PCI;
                primaryBus.id.pci = info;
            }
#endif
	    info->user_data = 0;
	}
    }
    free(iter);

    /* If we haven't found a primary device try a different heuristic */
    if (primaryBus.type == BUS_NONE && num) {
	for (i = 0; i < num; i++) {
	    uint16_t  command;

	    info = xf86PciVideoInfo[i];
	    pci_device_cfg_read_u16(info, & command, 4);

	    if ((command & PCI_CMD_MEM_ENABLE) 
		&& ((num == 1) || IS_VGA(info->device_class))) {
		if (primaryBus.type == BUS_NONE) {
		    primaryBus.type = BUS_PCI;
		    primaryBus.id.pci = info;
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
	    !PCIALWAYSPRINTCLASSES(info->device_class))
	    continue;

	if (xf86IsPrimaryPci(info))
	    prim = "*";

	xf86Msg(X_PROBED, "PCI:%s(%u:%u:%u:%u) %04x:%04x:%04x:%04x ", prim,
		info->domain, info->bus, info->dev, info->func,
		info->vendor_id, info->device_id,
		info->subvendor_id, info->subdevice_id);

	if (vendorname)
	    xf86ErrorF("%s ", vendorname);

	if (chipname)
	    xf86ErrorF("%s ", chipname);

	xf86ErrorF("rev %d", info->revision);

	for (i = 0; i < 6; i++) {
	    struct pci_mem_region * r = & info->regions[i];

	    if ( r->size && ! r->is_IO ) {
		if (!memdone) {
		    xf86ErrorF(", Mem @ ");
		    memdone = TRUE;
		} else
		    xf86ErrorF(", ");
		xf86ErrorF("0x%08lx/%ld", (long)r->base_addr, (long)r->size);
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
		xf86ErrorF("0x%08lx/%ld", (long)r->base_addr, (long)r->size);
	    }
	}

	if ( info->rom_size ) {
	    xf86ErrorF(", BIOS @ 0x\?\?\?\?\?\?\?\?/%ld", (long)info->rom_size);
	}

	xf86ErrorF("\n");
    }
    free(xf86PciVideoInfo);
}

/*
 * If the slot requested is already in use, return -1.
 * Otherwise, claim the slot for the screen requesting it.
 */

int
xf86ClaimPciSlot(struct pci_device * d, DriverPtr drvp,
		 int chipset, GDevPtr dev, Bool active)
{
    EntityPtr p = NULL;
    int num;
    
    if (xf86CheckPciSlot(d)) {
	num = xf86AllocateEntity();
	p = xf86Entities[num];
	p->driver = drvp;
	p->chipset = chipset;
	p->bus.type = BUS_PCI;
	p->bus.id.pci = d;
	p->active = active;
	p->inUse = FALSE;
	if (dev)
            xf86AddDevToEntity(num, dev);
	pciSlotClaimed = TRUE;

	if (active) {
	    /* Map in this domain's I/O space */
	   p->domainIO = xf86MapLegacyIO(d);
	}
	
 	return num;
    } else
 	return -1;
}

/*
 * Unclaim PCI slot, e.g. if probing failed, so that a different driver can claim.
 */
void
xf86UnclaimPciSlot(struct pci_device *d)
{
    int i;

    for (i = 0; i < xf86NumEntities; i++) {
	const EntityPtr p = xf86Entities[i];

	if ((p->bus.type == BUS_PCI) && (p->bus.id.pci == d)) {
	    /* Probably the slot should be deallocated? */
	    p->bus.type = BUS_NONE;
	    return;
	}
    }
}

/*
 * Parse a BUS ID string, and return the PCI bus parameters if it was
 * in the correct format for a PCI bus id.
 */

Bool
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
	free(s);
	return FALSE;
    }
    d = strpbrk(p, "@");
    if (d != NULL) {
	*(d++) = 0;
	for (i = 0; d[i] != 0; i++) {
	    if (!isdigit(d[i])) {
		free(s);
		return FALSE;
	    }
	}
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    free(s);
	    return FALSE;
	}
    }
    *bus = atoi(p);
    if (d != NULL && *d != 0)
	*bus += atoi(d) << 8;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	free(s);
	return FALSE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    free(s);
	    return FALSE;
	}
    }
    *device = atoi(p);
    *func = 0;
    p = strtok(NULL, ":");
    if (p == NULL || *p == 0) {
	free(s);
	return TRUE;
    }
    for (i = 0; p[i] != 0; i++) {
	if (!isdigit(p[i])) {
	    free(s);
	    return FALSE;
	}
    }
    *func = atoi(p);
    free(s);
    return TRUE;
}

/*
 * Compare a BUS ID string with a PCI bus id.  Return TRUE if they match.
 */

Bool
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
 
Bool
xf86IsPrimaryPci(struct pci_device *pPci)
{
    return ((primaryBus.type == BUS_PCI) && (pPci == primaryBus.id.pci));
}

/*
 * xf86GetPciInfoForEntity() -- Get the pciVideoRec of entity.
 */
struct pci_device *
xf86GetPciInfoForEntity(int entityIndex)
{
    EntityPtr p;
    
    if (entityIndex >= xf86NumEntities)
	return NULL;

    p = xf86Entities[entityIndex];
    return (p->bus.type == BUS_PCI) ? p->bus.id.pci : NULL;
}

/*
 * xf86CheckPciMemBase() checks that the memory base value matches one of the
 * PCI base address register values for the given PCI device.
 */
Bool
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

Bool
xf86CheckPciSlot(const struct pci_device *d)
{
    int i;

    for (i = 0; i < xf86NumEntities; i++) {
	const EntityPtr p = xf86Entities[i];

	if ((p->bus.type == BUS_PCI) && (p->bus.id.pci == d)) {
	    return FALSE;
	}
    }
    return TRUE;
}

#define END_OF_MATCHES(m) \
    (((m).vendor_id == 0) && ((m).device_id == 0) && ((m).subvendor_id == 0))

Bool
xf86PciAddMatchingDev(DriverPtr drvp)
{
    const struct pci_id_match * const devices = drvp->supported_devices;
    int j;
    struct pci_device *pPci;
    struct pci_device_iterator *iter;
    int numFound = 0;


    iter = pci_id_match_iterator_create(NULL);
    while ((pPci = pci_device_next(iter)) != NULL) {
    /* Determine if this device is supported by the driver.  If it is,
     * add it to the list of devices to configure.
     */
    for (j = 0 ; ! END_OF_MATCHES(devices[j]) ; j++) {
        if ( PCI_ID_COMPARE( devices[j].vendor_id, pPci->vendor_id )
         && PCI_ID_COMPARE( devices[j].device_id, pPci->device_id )
         && ((devices[j].device_class_mask & pPci->device_class)
             == devices[j].device_class) ) {
        if (xf86CheckPciSlot(pPci)) {
            GDevPtr pGDev = xf86AddBusDeviceToConfigure(
                    drvp->driverName, BUS_PCI, pPci, -1);
            if (pGDev != NULL) {
            /* After configure pass 1, chipID and chipRev are
             * treated as over-rides, so clobber them here.
             */
            pGDev->chipID = -1;
            pGDev->chipRev = -1;
            }

            numFound++;
        }

        break;
        }
    }
    }

    pci_iterator_destroy(iter);

    return (numFound != 0);
}

Bool
xf86PciProbeDev(DriverPtr drvp)
{
    int i, j;
    struct pci_device * pPci;
    Bool foundScreen = FALSE;
    const struct pci_id_match * const devices = drvp->supported_devices;
    GDevPtr *devList;
    const unsigned numDevs = xf86MatchDevice(drvp->driverName, & devList);

    for ( i = 0 ; i < numDevs ; i++ ) {
       struct pci_device_iterator *iter;
       unsigned device_id;


       /* Find the pciVideoRec associated with this device section.
        */
       iter = pci_id_match_iterator_create(NULL);
       while ((pPci = pci_device_next(iter)) != NULL) {
           if (devList[i]->busID && *devList[i]->busID) {
               if (xf86ComparePciBusString(devList[i]->busID,
                                           ((pPci->domain << 8)
                                            | pPci->bus),
                                           pPci->dev,
                                           pPci->func)) {
                   break;
               }
           }
           else if (xf86IsPrimaryPci(pPci)) {
               break;
           }
       }

       pci_iterator_destroy(iter);

       if (pPci == NULL) {
           continue;
       }
       device_id = (devList[i]->chipID > 0)
         ? devList[i]->chipID : pPci->device_id;


       /* Once the pciVideoRec is found, determine if the device is supported
        * by the driver.  If it is, probe it!
        */
       for ( j = 0 ; ! END_OF_MATCHES( devices[j] ) ; j++ ) {
           if ( PCI_ID_COMPARE( devices[j].vendor_id, pPci->vendor_id )
                && PCI_ID_COMPARE( devices[j].device_id, device_id )
                && ((devices[j].device_class_mask & pPci->device_class)
                     == devices[j].device_class) ) {
               int  entry;

               /* Allow the same entity to be used more than once for
                * devices with multiple screens per entity.  This assumes
                * implicitly that there will be a screen == 0 instance.
                *
                * FIXME Need to make sure that two different drivers don't
                * FIXME claim the same screen > 0 instance.
                */
               if ((devList[i]->screen == 0) && !xf86CheckPciSlot(pPci))
                   continue;

               DebugF("%s: card at %d:%d:%d is claimed by a Device section\n",
                      drvp->driverName, pPci->bus, pPci->dev, pPci->func);

               /* Allocate an entry in the lists to be returned */
               entry = xf86ClaimPciSlot(pPci, drvp, device_id,
                                         devList[i], devList[i]->active);

               if ((entry == -1) && (devList[i]->screen > 0)) {
                   unsigned k;

                   for (k = 0; k < xf86NumEntities; k++ ) {
                       EntityPtr pEnt = xf86Entities[k];
                       if (pEnt->bus.type != BUS_PCI)
                           continue;
                       if (pEnt->bus.id.pci == pPci) {
                           entry = k;
                           xf86AddDevToEntity(k, devList[i]);
                           break;
                       }
                   }
               }

               if (entry != -1) {
                   if ((*drvp->PciProbe)(drvp, entry, pPci,
                                         devices[j].match_data)) {
                       foundScreen = TRUE;
                   } else
                       xf86UnclaimPciSlot(pPci);
               }

               break;
           }
       }
    }
    free(devList);

    return foundScreen;
}

void
xf86PciIsolateDevice(char *argument)
{
    int bus, device, func;

    if (sscanf(argument, "PCI:%d:%d:%d", &bus, &device, &func) == 3) {
        xf86IsolateDevice.domain = PCI_DOM_FROM_BUS(bus);
        xf86IsolateDevice.bus = PCI_BUS_NO_DOMAIN(bus);
        xf86IsolateDevice.dev = device;
        xf86IsolateDevice.func = func;
    } else
        FatalError("Invalid isolated device specification\n");
}

static Bool
pciDeviceHasBars(struct pci_device *pci)
{
    int i;

    for (i = 0; i < 6; i++)
	if (pci->regions[i].size)
	    return TRUE;

    if (pci->rom_size)
	return TRUE;

    return FALSE;
}

struct Inst {
    struct pci_device *	pci;
    GDevPtr		dev;
    Bool		foundHW;  /* PCIid in list of supported chipsets */
    Bool		claimed;  /* BusID matches with a device section */
    int 		chip;
    int 		screen;
};


/**
 * Find set of unclaimed devices matching a given vendor ID.
 *
 * Used by drivers to find as yet unclaimed devices matching the specified
 * vendor ID.
 *
 * \param driverName     Name of the driver.  This is used to find Device
 *                       sections in the config file.
 * \param vendorID       PCI vendor ID of associated devices.  If zero, then
 *                       the true vendor ID must be encoded in the \c PCIid
 *                       fields of the \c PCIchipsets entries.
 * \param chipsets       Symbol table used to associate chipset names with
 *                       PCI IDs.
 * \param devList        List of Device sections parsed from the config file.
 * \param numDevs        Number of entries in \c devList.
 * \param drvp           Pointer the driver's control structure.
 * \param foundEntities  Returned list of entity indicies associated with the
 *                       driver.
 *
 * \returns
 * The number of elements in returned in \c foundEntities on success or zero
 * on failure.
 *
 * \todo
 * This function does a bit more than short description says.  Fill in some
 * more of the details of its operation.
 *
 * \todo
 * The \c driverName parameter is redundant.  It is the same as
 * \c DriverRec::driverName.  In a future version of this function, remove
 * that parameter.
 */
int
xf86MatchPciInstances(const char *driverName, int vendorID,
		      SymTabPtr chipsets, PciChipsets *PCIchipsets,
		      GDevPtr *devList, int numDevs, DriverPtr drvp,
		      int **foundEntities)
{
    int i,j;
    struct pci_device * pPci;
    struct pci_device_iterator *iter;
    struct Inst *instances = NULL;
    int numClaimedInstances = 0;
    int allocatedInstances = 0;
    int numFound = 0;
    SymTabRec *c;
    PciChipsets *id;
    int *retEntities = NULL;

    *foundEntities = NULL;


    /* Each PCI device will contribute at least one entry.  Each device
     * section can contribute at most one entry.  The sum of the two is
     * guaranteed to be larger than the maximum possible number of entries.
     * Do this calculation and memory allocation once now to eliminate the
     * need for realloc calls inside the loop.
     */
    if (!(xf86DoConfigure && xf86DoConfigurePass1)) {
	unsigned max_entries = numDevs;

	iter = pci_slot_match_iterator_create(NULL);
	while ((pPci = pci_device_next(iter)) != NULL) {
	    max_entries++;
	}

	pci_iterator_destroy(iter);
	instances = xnfalloc(max_entries * sizeof(struct Inst));
    }

    iter = pci_slot_match_iterator_create(NULL);
    while ((pPci = pci_device_next(iter)) != NULL) {
	unsigned device_class = pPci->device_class;
	Bool foundVendor = FALSE;


	/* Convert the pre-PCI 2.0 device class for a VGA adapter to the
	 * 2.0 version of the same class.
	 */
	if ( device_class == 0x00000101 ) {
	    device_class = 0x00030000;
	}


	/* Find PCI devices that match the given vendor ID.  The vendor ID is
	 * either specified explicitly as a parameter to the function or
	 * implicitly encoded in the high bits of id->PCIid.
	 *
	 * The first device with a matching vendor is recorded, even if the
	 * device ID doesn't match.  This is done because the Device section
	 * in the xorg.conf file can over-ride the device ID.  A matching PCI
	 * ID might not be found now, but after the device ID over-ride is
	 * applied there /might/ be a match.
	 */
	for (id = PCIchipsets; id->PCIid != -1; id++) {
	    const unsigned vendor_id = ((id->PCIid & 0xFFFF0000) >> 16)
		| vendorID;
	    const unsigned device_id = (id->PCIid & 0x0000FFFF);
	    const unsigned match_class = 0x00030000 | id->PCIid;

	    if ((vendor_id == pPci->vendor_id)
		|| ((vendorID == PCI_VENDOR_GENERIC) && (match_class == device_class))) {
		if (!foundVendor && (instances != NULL)) {
		    ++allocatedInstances;
		    instances[allocatedInstances - 1].pci = pPci;
		    instances[allocatedInstances - 1].dev = NULL;
		    instances[allocatedInstances - 1].claimed = FALSE;
		    instances[allocatedInstances - 1].foundHW = FALSE;
		    instances[allocatedInstances - 1].screen = 0;
		}

		foundVendor = TRUE;

		if ( (device_id == pPci->device_id)
		     || ((vendorID == PCI_VENDOR_GENERIC)
			 && (match_class == device_class)) ) {
		    if ( instances != NULL ) {
			instances[allocatedInstances - 1].foundHW = TRUE;
			instances[allocatedInstances - 1].chip = id->numChipset;
		    }


		    if ( xf86DoConfigure && xf86DoConfigurePass1 ) {
			if (xf86CheckPciSlot(pPci)) {
			    GDevPtr pGDev =
			      xf86AddBusDeviceToConfigure(drvp->driverName,
							  BUS_PCI, pPci, -1);
			    if (pGDev) {
				/* After configure pass 1, chipID and chipRev
				 * are treated as over-rides, so clobber them
				 * here.
				 */
				pGDev->chipID = -1;
				pGDev->chipRev = -1;
			    }

			    numFound++;
			}
		    }
		    else {
			numFound++;
		    }

		    break;
		}
	    }
	}
    }

    pci_iterator_destroy(iter);


    /* In "probe only" or "configure" mode (signaled by instances being NULL),
     * our work is done.  Return the number of detected devices.
     */
    if ( instances == NULL ) {
	return numFound;
    }


    /*
     * This may be debatable, but if no PCI devices with a matching vendor
     * type is found, return zero now.  It is probably not desirable to
     * allow the config file to override this.
     */
    if (allocatedInstances <= 0) {
	free(instances);
	return 0;
    }


    DebugF("%s instances found: %d\n", driverName, allocatedInstances);

   /*
    * Check for devices that need duplicated instances.  This is required
    * when there is more than one screen per entity.
    *
    * XXX This currently doesn't work for cases where the BusID isn't
    * specified explicitly in the config file.
    */

    for (j = 0; j < numDevs; j++) {
        if (devList[j]->screen > 0 && devList[j]->busID
	    && *devList[j]->busID) {
	    for (i = 0; i < allocatedInstances; i++) {
	        pPci = instances[i].pci;
	        if (xf86ComparePciBusString(devList[j]->busID,
					    PCI_MAKE_BUS( pPci->domain, pPci->bus ),
					    pPci->dev,
					    pPci->func)) {
		    allocatedInstances++;
		    instances[allocatedInstances - 1] = instances[i];
		    instances[allocatedInstances - 1].screen = devList[j]->screen;
		    numFound++;
		    break;
		}
	    }
	}
    }

    for (i = 0; i < allocatedInstances; i++) {
	GDevPtr dev = NULL;
	GDevPtr devBus = NULL;

	pPci = instances[i].pci;
	for (j = 0; j < numDevs; j++) {
	    if (devList[j]->busID && *devList[j]->busID) {
		if (xf86ComparePciBusString(devList[j]->busID,
					    PCI_MAKE_BUS( pPci->domain, pPci->bus ),
					    pPci->dev,
					    pPci->func) &&
		    devList[j]->screen == instances[i].screen) {

		    if (devBus)
                        xf86MsgVerb(X_WARNING,0,
			    "%s: More than one matching Device section for "
			    "instances\n\t(BusID: %s) found: %s\n",
			    driverName, devList[j]->busID,
			    devList[j]->identifier);
		    else
			devBus = devList[j];
		}
	    } else {
		/*
		 * if device section without BusID is found
		 * only assign to it to the primary device.
		 */
		if (xf86IsPrimaryPci(pPci)) {
		    xf86Msg(X_PROBED, "Assigning device section with no busID"
			    " to primary device\n");
		    if (dev || devBus)
			xf86MsgVerb(X_WARNING, 0,
			    "%s: More than one matching Device section "
			    "found: %s\n", driverName, devList[j]->identifier);
		    else
			dev = devList[j];
		}
	    }
	}
	if (devBus) dev = devBus;  /* busID preferred */
	if (!dev) {
	    if (xf86CheckPciSlot(pPci) && pciDeviceHasBars(pPci)) {
		xf86MsgVerb(X_WARNING, 0, "%s: No matching Device section "
			    "for instance (BusID PCI:%u@%u:%u:%u) found\n",
			    driverName, pPci->domain, pPci->bus, pPci->dev,
			    pPci->func);
	    }
	} else {
	    numClaimedInstances++;
	    instances[i].claimed = TRUE;
	    instances[i].dev = dev;
	}
    }
    DebugF("%s instances found: %d\n", driverName, numClaimedInstances);
    /*
     * Now check that a chipset or chipID override in the device section
     * is valid.  Chipset has precedence over chipID.
     * If chipset is not valid ignore BusSlot completely.
     */
    for (i = 0; i < allocatedInstances && numClaimedInstances > 0; i++) {
	MessageType from = X_PROBED;

	if (!instances[i].claimed) {
	    continue;
	}
	if (instances[i].dev->chipset) {
	    for (c = chipsets; c->token >= 0; c++) {
		if (xf86NameCmp(c->name, instances[i].dev->chipset) == 0)
		    break;
	    }
	    if (c->token == -1) {
		instances[i].claimed = FALSE;
		numClaimedInstances--;
		xf86MsgVerb(X_WARNING, 0, "%s: Chipset \"%s\" in Device "
			    "section \"%s\" isn't valid for this driver\n",
			    driverName, instances[i].dev->chipset,
			    instances[i].dev->identifier);
	    } else {
		instances[i].chip = c->token;

		for (id = PCIchipsets; id->numChipset >= 0; id++) {
		    if (id->numChipset == instances[i].chip)
			break;
		}
		if(id->numChipset >=0){
		    xf86Msg(X_CONFIG,"Chipset override: %s\n",
			     instances[i].dev->chipset);
		    from = X_CONFIG;
		} else {
		    instances[i].claimed = FALSE;
		    numClaimedInstances--;
		    xf86MsgVerb(X_WARNING, 0, "%s: Chipset \"%s\" in Device "
				"section \"%s\" isn't a valid PCI chipset\n",
				driverName, instances[i].dev->chipset,
				instances[i].dev->identifier);
		}
	    }
	} else if (instances[i].dev->chipID > 0) {
	    for (id = PCIchipsets; id->numChipset >= 0; id++) {
		if (id->PCIid == instances[i].dev->chipID)
		    break;
	    }
	    if (id->numChipset == -1) {
		instances[i].claimed = FALSE;
		numClaimedInstances--;
		xf86MsgVerb(X_WARNING, 0, "%s: ChipID 0x%04X in Device "
			    "section \"%s\" isn't valid for this driver\n",
			    driverName, instances[i].dev->chipID,
			    instances[i].dev->identifier);
	    } else {
		instances[i].chip = id->numChipset;

		xf86Msg( X_CONFIG,"ChipID override: 0x%04X\n",
			 instances[i].dev->chipID);
		from = X_CONFIG;
	    }
	} else if (!instances[i].foundHW) {
	    /*
	     * This means that there was no override and the PCI chipType
	     * doesn't match one that is supported
	     */
	    instances[i].claimed = FALSE;
	    numClaimedInstances--;
	}
	if (instances[i].claimed == TRUE){
	    for (c = chipsets; c->token >= 0; c++) {
		if (c->token == instances[i].chip)
		    break;
	    }
	    xf86Msg(from,"Chipset %s found\n",
		    c->name);
	}
    }

    /*
     * Of the claimed instances, check that another driver hasn't already
     * claimed its slot.
     */
    numFound = 0;
    for (i = 0; i < allocatedInstances && numClaimedInstances > 0; i++) {
	if (!instances[i].claimed)
	    continue;
	pPci = instances[i].pci;


        /*
	 * Allow the same entity to be used more than once for devices with
	 * multiple screens per entity.  This assumes implicitly that there
	 * will be a screen == 0 instance.
	 *
	 * XXX Need to make sure that two different drivers don't claim
	 * the same screen > 0 instance.
	 */
        if (instances[i].screen == 0 && !xf86CheckPciSlot( pPci ))
	    continue;

	DebugF("%s: card at %d:%d:%d is claimed by a Device section\n",
	       driverName, pPci->bus, pPci->dev, pPci->func);

	/* Allocate an entry in the lists to be returned */
	numFound++;
	retEntities = xnfrealloc(retEntities, numFound * sizeof(int));
	retEntities[numFound - 1] = xf86ClaimPciSlot( pPci, drvp,
						      instances[i].chip,
						      instances[i].dev,
						      instances[i].dev->active);
        if (retEntities[numFound - 1] == -1 && instances[i].screen > 0) {
	    for (j = 0; j < xf86NumEntities; j++) {
	        EntityPtr pEnt = xf86Entities[j];
	        if (pEnt->bus.type != BUS_PCI)
		    continue;
	        if (pEnt->bus.id.pci == pPci) {
		    retEntities[numFound - 1] = j;
		    xf86AddDevToEntity(j, instances[i].dev);
		    break;
		}
	    }
	}
    }
    free(instances);
    if (numFound > 0) {
	*foundEntities = retEntities;
    }

    return numFound;
}

/*
 * xf86ConfigPciEntityInactive() -- This function can be used
 * to configure an inactive entity as well as to reconfigure an
 * previously active entity inactive. If the entity has been
 * assigned to a screen before it will be removed. If p_chip is
 * non-NULL all static resources listed there will be registered.
 */
static void
xf86ConfigPciEntityInactive(EntityInfoPtr pEnt, PciChipsets *p_chip,
			    EntityProc init, EntityProc enter,
			    EntityProc leave, pointer private)
{
    ScrnInfoPtr pScrn;

    if ((pScrn = xf86FindScreenForEntity(pEnt->index)))
	xf86RemoveEntityFromScreen(pScrn,pEnt->index);

    /* shared resources are only needed when entity is active: remove */
    xf86SetEntityFuncs(pEnt->index,init,enter,leave,private);
}

ScrnInfoPtr
xf86ConfigPciEntity(ScrnInfoPtr pScrn, int scrnFlag, int entityIndex,
			  PciChipsets *p_chip, void *dummy, EntityProc init,
			  EntityProc enter, EntityProc leave, pointer private)
{
    EntityInfoPtr pEnt = xf86GetEntityInfo(entityIndex);
    if (!pEnt) return pScrn;

    if (!(pEnt->location.type == BUS_PCI)
	|| !xf86GetPciInfoForEntity(entityIndex)) {
	free(pEnt);
	return pScrn;
    }
    if (!pEnt->active) {
	xf86ConfigPciEntityInactive(pEnt, p_chip, init,  enter,
				    leave,  private);
	free(pEnt);
	return pScrn;
    }

    if (!pScrn)
	pScrn = xf86AllocateScreen(pEnt->driver,scrnFlag);
    if (xf86IsEntitySharable(entityIndex)) {
        xf86SetEntityShared(entityIndex);
    }
    xf86AddEntityToScreen(pScrn,entityIndex);
    if (xf86IsEntityShared(entityIndex)) {
        return pScrn;
    }
    free(pEnt);

    xf86SetEntityFuncs(entityIndex,init,enter,leave,private);

    return pScrn;
}

/*
 *  OBSOLETE ! xf86ConfigActivePciEntity() is an obsolete function.
 *             It is likely to be removed. Don't use!
 */
Bool
xf86ConfigActivePciEntity(ScrnInfoPtr pScrn, int entityIndex,
                          PciChipsets *p_chip, void *dummy, EntityProc init,
                          EntityProc enter, EntityProc leave, pointer private)
{
    EntityInfoPtr pEnt = xf86GetEntityInfo(entityIndex);
    if (!pEnt) return FALSE;

    if (!pEnt->active || !(pEnt->location.type == BUS_PCI)) {
        free(pEnt);
        return FALSE;
    }
    xf86AddEntityToScreen(pScrn,entityIndex);

    free(pEnt);
    if (!xf86SetEntityFuncs(entityIndex,init,enter,leave,private))
        return FALSE;

    return TRUE;
}
