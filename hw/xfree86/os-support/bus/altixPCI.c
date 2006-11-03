/*
 * This file contains the glue necessary for support of SGI's Altix chipset.
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include "altixPCI.h"
#include "xf86.h"
#include "Pci.h"

/*
 * get_dev_on_bus - Return the first device we find on segnum, busnum
 *
 * Walk all the PCI devices and return the first one found on segnum, busnum.
 * There may be a better way to do this in some xf86* function I don't know
 * about.
 */
static pciDevice *get_dev_on_bus(unsigned int segnum, unsigned int busnum)
{
	pciDevice **pdev = xf86scanpci(0);
	int i;

	for (i = 0; pdev[i] != NULL; i++)
		if (PCI_DOM_FROM_TAG(pdev[i]->tag) == segnum &&
		    pdev[i]->busnum == busnum)
			return pdev[i];
	/* Should never get here... */
	ErrorF("No PCI device found on %04x:%02x??", segnum, busnum);
	return NULL;
}

/*
 * get_bridge_info - fill in the bridge info for bus_info based on pdev
 *
 * Find the parent bus for pdev if it exists, otherwise assume pdev *is*
 * the parent bus.  We need this on Altix because our bridges are transparent.
 */
static void get_bridge_info(pciBusInfo_t *bus_info, pciDevice *pdev)
{
	unsigned int parent_segnum, segnum = PCI_DOM_FROM_TAG(pdev->tag);
	unsigned int parent_busnum, parent_nodombus, busnum = pdev->busnum;
	unsigned int nodombus = PCI_BUS_NO_DOMAIN(PCI_BUS_FROM_TAG(pdev->tag));
	char bridge_path[] = "/sys/class/pci_bus/0000:00/bridge";
	char bridge_target[] = "../../../devices/pci0000:00";

	/* Path to this device's bridge */
	sprintf(bridge_path, "/sys/class/pci_bus/%04x:%02x/bridge", segnum,
		nodombus);

	if (readlink(bridge_path, bridge_target, strlen(bridge_target)) < 0) {
		perror("failed to dereference bridge link");
 		ErrorF("failed to dereference bridge link, aborting\n");
		exit(-1);
	}

	sscanf(bridge_target, "../../../devices/pci%04x:%02x", &parent_segnum,
	       &parent_nodombus);

	parent_busnum = PCI_MAKE_BUS(parent_segnum, parent_nodombus);

	/*
	 * If there's no bridge or the bridge points to the device, use
	 * pdev as the bridge
	 */
	if (segnum == parent_segnum && busnum == parent_busnum) {
		bus_info->bridge = pdev;
		bus_info->secondary = FALSE;
		bus_info->primary_bus = busnum;
	} else {
		bus_info->bridge = get_dev_on_bus(parent_segnum,
						  parent_busnum);
		bus_info->secondary = TRUE;
		bus_info->primary_bus = parent_busnum;
	}
	pdev->businfo = bus_info;
	pdev->pci_base_class = PCI_CLASS_DISPLAY;
	pdev->pci_sub_class = PCI_SUBCLASS_PREHISTORIC_VGA;
}

void xf86PreScanAltix(void)
{
	/* Nothing to see here... */
}

void xf86PostScanAltix(void)
{
	pciConfigPtr *pdev;
	pciBusInfo_t *bus_info;
	int prevBusNum, curBusNum, idx;

	/*
	 * Altix PCI bridges are invisible to userspace, so we make each device
	 * look like it's its own bridge unless it actually has a parent (as in
	 * the case of PCI to PCI bridges).
	 */
	bus_info = pciBusInfo[0];
	pdev = xf86scanpci(0);
	prevBusNum = curBusNum = pdev[0]->busnum;
	bus_info = pciBusInfo[curBusNum];
	bus_info->bridge = pdev[0];
	bus_info->secondary = FALSE;
	bus_info->primary_bus = curBusNum;

	/* Walk all the PCI devices, assigning their bridge info */
	for (idx = 0; pdev[idx] != NULL; idx++) {
		if (pdev[idx]->busnum == prevBusNum)
			continue; /* Already fixed up this bus */

		curBusNum = pdev[idx]->busnum;
		bus_info = pciBusInfo[curBusNum];

		/*
		 * Fill in bus_info for pdev.  The bridge field will either
		 * be pdev[idx] or a device on the parent bus.
		 */
		get_bridge_info(bus_info, pdev[idx]);
		prevBusNum = curBusNum;
	}
	return;
}
