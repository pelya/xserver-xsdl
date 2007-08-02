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

void xf86PreScanAltix(void)
{
	/* Nothing to see here... */
}

void xf86PostScanAltix(void)
{
	pciConfigPtr *pdev;
	int idx, free_idx;

	/*
	 * Some altix pci chipsets do not expose themselves as host
	 * bridges.
	 *
	 * Walk the devices looking for buses for which there is not a
	 * corresponding pciDevice entry (ie. pciBusInfo[]->bridge is NULL).
	 *
	 * It is assumed that this indicates a root bridge for which we will
	 * construct a fake pci host bridge device.
	 */

	pdev = xf86scanpci(0);
	for (idx = 0; pdev[idx] != NULL; idx++)
		;

	free_idx = idx;

	for (idx = 0; idx < free_idx; idx++) {
		pciConfigPtr dev, fakedev;
		pciBusInfo_t *businfo;

		dev = pdev[idx];
		businfo = pciBusInfo[dev->busnum];

		if (! businfo) {
			/* device has no bus ... should this be an error? */
			continue;
		}

		if (businfo->bridge) {
			/* bus has a device ... no need for fixup */
			continue;
		}

		if (free_idx >= MAX_PCI_DEVICES)
			FatalError("SN: No room for fake root bridge device\n");

		/*
		 * Construct a fake device and stick it at the end of the
		 * pdev array.  Make it look like a host bridge.
		 */
		fakedev = xnfcalloc(1, sizeof(pciDevice));
		fakedev->tag = PCI_MAKE_TAG(dev->busnum, 0, 0);;
		fakedev->busnum = dev->busnum;
		fakedev->devnum = 0;
		fakedev->funcnum = 0;
		fakedev->fakeDevice = 1;
		/* should figure out a better DEVID */
		fakedev->pci_device_vendor = DEVID(VENDOR_GENERIC, CHIP_VGA);
		fakedev->pci_base_class = PCI_CLASS_BRIDGE;

		businfo->secondary = 0;
		businfo->primary_bus = dev->busnum;
		businfo->bridge = fakedev;

		fakedev->businfo = businfo;

		pdev[free_idx++] = fakedev;
	}
}
