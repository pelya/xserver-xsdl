/* $XFree86: xc/programs/Xserver/hw/xfree86/scanpci/xf86ScanPci.c,v 1.12 2002/07/15 20:46:04 dawes Exp $ */
/*
 * Display the Subsystem Vendor Id and Subsystem Id in order to identify
 * the cards installed in this computer
 *
 * Copyright 1995-2002 by The XFree86 Project, Inc.
 *
 * A lot of this comes from Robin Cutshaw's scanpci
 *
 */


/*
 * This file is used to build both the scanpci and pcidata modules.
 * The interfaces have changed compared with XFree86 4.2.0 and earlier.
 * The data is no longer exported directly.  Lookup functions are provided.
 * This means that the data format can change in the future without affecting
 * the exported interfaces.
 *
 * The namespaces for pcidata and scanpci clash, so both modules can't be
 * loaded at the same time.  The X server should only load the scanpci module
 * when run with the '-scanpci' flag.  The main difference between the
 * two modules is size.  pcidata only holds the subset of data that is
 * "interesting" to the X server.  "Interesting" is determined by the
 * PCI_VENDOR_* defines in ../common/xf86PciInfo.h.
 */


/* XXX This is including a lot of stuff that modules should not include! */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"
#include "xf86_OSproc.h"

#ifndef IN_MODULE
#include <ctype.h>
#include <stdlib.h>
#else
#include <xf86_ansic.h>
#endif

#ifndef PCIDATA
#define VENDOR_INCLUDE_NONVIDEO
#endif
#define INIT_SUBSYS_INFO
#define INIT_VENDOR_SUBSYS_INFO

#include "xf86PciStr.h"
#include "xf86PciIds.h"
#include "xf86ScanPci.h"

/*
 * PCI classes that have messages printed always.  The others are only
 * have a message printed when the vendor/dev IDs are recognised.
 */
#define PCIALWAYSPRINTCLASSES(b,s)					      \
    (((b) == PCI_CLASS_PREHISTORIC && (s) == PCI_SUBCLASS_PREHISTORIC_VGA) || \
     ((b) == PCI_CLASS_DISPLAY) ||					      \
     ((b) == PCI_CLASS_MULTIMEDIA && (s) == PCI_SUBCLASS_MULTIMEDIA_VIDEO))


#ifdef XFree86LOADER

#include "xf86Module.h"

#ifdef PCIDATA

static XF86ModuleVersionInfo pciDataVersRec = {
	"pcidata",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	NULL,
	{0, 0, 0, 0}
};

XF86ModuleData pcidataModuleData = { &pciDataVersRec, NULL, NULL };

#else 

static XF86ModuleVersionInfo scanPciVersRec = {
	"scanpci",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_VIDEODRV,
	ABI_VIDEODRV_VERSION,
	NULL,
	{0, 0, 0, 0}
};

XF86ModuleData scanpciModuleData = { &scanPciVersRec, NULL, NULL };

#endif /* PCIDATA */

#endif /* XFree86LOADER */

/* Initialisation/Close hooks, in case they're ever needed. */
Bool
ScanPciSetupPciIds(void)
{
    return TRUE;
}

void
ScanPciClosePciIds(void)
{
    return;
}

/*
 * The return value is the number of strings found, or -1 for an error.
 * Requested strings that aren't found are set to NULL.
 */

int
ScanPciFindPciNamesByDevice(unsigned short vendor, unsigned short device,
			 unsigned short svendor, unsigned short subsys,
			 const char **vname, const char **dname,
			 const char **svname, const char **sname)
{
    int i, j, k;
    const pciDeviceInfo **pDev;
    const pciSubsystemInfo **pSub;

    /* It's an error to not provide the Vendor */
    if (vendor == NOVENDOR)
	return -1;

    /* Initialise returns requested/provided to NULL */
    if (vname)
	*vname = NULL;
    if (device != NODEVICE && dname)
	*dname = NULL;
    if (svendor != NOVENDOR && svname)
	*svname = NULL;
    if (subsys != NOSUBSYS && sname)
	*sname = NULL;

    for (i = 0; pciVendorInfoList[i].VendorName; i++) {
	if (vendor == pciVendorInfoList[i].VendorID) {
	    if (vname) {
		*vname = pciVendorInfoList[i].VendorName;
	    }
	    if (device == NODEVICE) {
		return 1;
	    }
	    pDev = pciVendorInfoList[i].Device;
	    if (!pDev) {
		return 1;
	    }
	    for (j = 0; pDev[j]; j++) {
		if (device == pDev[j]->DeviceID) {
		    if (dname) {
			*dname = pDev[j]->DeviceName;
		    }
		    if (svendor == NOVENDOR) {
			return 2;
		    }
		    for (k = 0; pciVendorInfoList[k].VendorName; k++) {
			if (svendor == pciVendorInfoList[k].VendorID) {
			    if (svname) {
				*svname = pciVendorInfoList[k].VendorName;
			    }
			    if (subsys == NOSUBSYS) {
				return 3;
			    }
			    break;
			}
		    }
		    if (!pciVendorInfoList[k].VendorName) {
			return 2;
		    }
		    pSub = pDev[j]->Subsystem;
		    if (!pSub) {
			return 3;
		    }
		    for (k = 0; pSub[k]; k++) {
			if (svendor == pSub[k]->VendorID &&
			    subsys == pSub[k]->SubsystemID) {
			    if (sname)
				*sname = pSub[k]->SubsystemName;
			    return 4;
			}
		    }
		    /* No vendor/subsys match */
		    return 3;
		}
	    }
	    /* No device match */
	    return 1;
	}
    }
    /* No vendor match */
    return 0;
}

Bool
ScanPciFindPciNamesBySubsys(unsigned short svendor, unsigned short subsys,
			 const char **svname, const char **sname)
{
    int i, j;
    const pciSubsystemInfo **pSub;

    /* It's an error to not provide the Vendor */
    if (svendor == NOVENDOR)
	return -1;

    /* Initialise returns requested/provided to NULL */
    if (svname)
	*svname = NULL;
    if (subsys != NOSUBSYS && sname)
	*sname = NULL;

    for (i = 0; pciVendorSubsysInfoList[i].VendorName; i++) {
	if (svendor == pciVendorSubsysInfoList[i].VendorID) {
	    if (svname) {
		*svname = pciVendorSubsysInfoList[i].VendorName;
	    }
	    if (subsys == NOSUBSYS) {
		return 1;
	    }
	    pSub = pciVendorSubsysInfoList[i].Subsystem;
	    if (!pSub) {
		return 1;
	    }
	    for (j = 0; pSub[j]; j++) {
		if (subsys == pSub[j]->SubsystemID) {
		    if (sname) {
			*sname = pSub[j]->SubsystemName;
		    }
		}
	    }
	    /* No subsys match */
	    return 1;
	}
    }
    /* No vendor match */
    return 0;
}

CARD32
ScanPciFindPciClassBySubsys(unsigned short vendor, unsigned short subsys)
{
    int i, j;
    const pciSubsystemInfo **pSub;

    if (vendor == NOVENDOR || subsys == NOSUBSYS)
	return 0;

    for (i = 0; pciVendorSubsysInfoList[i].VendorName; i++) {
	if (vendor == pciVendorSubsysInfoList[i].VendorID) {
	    pSub = pciVendorSubsysInfoList[i].Subsystem;
	    if (!pSub) {
		return 0;
	    }
	    for (j = 0; pSub[j]; j++) {
		if (subsys == pSub[j]->SubsystemID) {
		    return pSub[j]->class;
		}
	    }
	    break;
	}
    }
    return 0;
}

CARD32
ScanPciFindPciClassByDevice(unsigned short vendor, unsigned short device)
{
    int i, j;
    const pciDeviceInfo **pDev;

    if (vendor == NOVENDOR || device == NODEVICE)
	return 0;

    for (i = 0; pciVendorInfoList[i].VendorName; i++) {
	if (vendor == pciVendorInfoList[i].VendorID) {
	    pDev = pciVendorInfoList[i].Device;
	    if (!pDev) {
		return 0;
	    }
	    for (j = 0; pDev[j]; j++) {
		if (device == pDev[j]->DeviceID) {
		    return pDev[j]->class;
		}
	    }
	    break;
	}
    }
    return 0;
}

#ifndef PCIDATA
void
ScanPciDisplayPCICardInfo(int verbosity)
{
    pciConfigPtr pcrp, *pcrpp;
    int i;

    xf86EnableIO();
    pcrpp = xf86scanpci(0);

    if (pcrpp == NULL) {
        xf86MsgVerb(X_NONE,0,"No PCI info available\n");
	return;
    }
    xf86MsgVerb(X_NONE,0,"Probing for PCI devices (Bus:Device:Function)\n\n");
    for (i = 0; (pcrp = pcrpp[i]); i++) {
	const char *svendorname = NULL, *subsysname = NULL;
	const char *vendorname = NULL, *devicename = NULL;
	Bool noCard = FALSE;
	const char *prefix1 = "", *prefix2 = "";

	xf86MsgVerb(X_NONE, -verbosity, "(%d:%d:%d) ",
		    pcrp->busnum, pcrp->devnum, pcrp->funcnum);

	/*
	 * Lookup as much as we can about the device.
	 */
	if (pcrp->pci_subsys_vendor || pcrp->pci_subsys_card) {
	    ScanPciFindPciNamesByDevice(pcrp->pci_vendor, pcrp->pci_device,
				     NOVENDOR, NOSUBSYS,
				     &vendorname, &devicename, NULL, NULL);
	} else {
	    ScanPciFindPciNamesByDevice(pcrp->pci_vendor, pcrp->pci_device,
				     pcrp->pci_subsys_vendor,
				     pcrp->pci_subsys_card,
				     &vendorname, &devicename,
				     &svendorname, &subsysname);
	}

	if (svendorname)
	    xf86MsgVerb(X_NONE, -verbosity, "%s ", svendorname);
	if (subsysname)
	    xf86MsgVerb(X_NONE, -verbosity, "%s ", subsysname);
	if (svendorname && !subsysname) {
	    if (pcrp->pci_subsys_card && pcrp->pci_subsys_card != NOSUBSYS) {
		xf86MsgVerb(X_NONE, -verbosity, "unknown card (0x%04x) ",
			    pcrp->pci_subsys_card);
	    } else {
		xf86MsgVerb(X_NONE, -verbosity, "card ");
	    }
	}
	if (!svendorname && !subsysname) {
	    /*
	     * We didn't find a text representation of the information 
	     * about the card.
	     */
	    if (pcrp->pci_subsys_vendor || pcrp->pci_subsys_card) {
		/*
		 * If there was information and we just couldn't interpret
		 * it, print it out as unknown, anyway.
		 */
		xf86MsgVerb(X_NONE, -verbosity,
			    "unknown card (0x%04x/0x%04x) ",
			    pcrp->pci_subsys_vendor, pcrp->pci_subsys_card);
	    } else
		noCard = TRUE;
	}
	if (!noCard) {
	    prefix1 = "using a ";
	    prefix2 = "using an ";
	}
	if (vendorname && devicename) {
	    xf86MsgVerb(X_NONE, -verbosity,"%s%s %s\n", prefix1, vendorname,
			devicename);
	} else if (vendorname) {
	    xf86MsgVerb(X_NONE, -verbosity,
			"%sunknown chip (DeviceId 0x%04x) from %s\n",
			prefix2, pcrp->pci_device, vendorname);
	} else {
	    xf86MsgVerb(X_NONE, -verbosity,
			"%sunknown chipset(0x%04x/0x%04x)\n",
			prefix2, pcrp->pci_vendor, pcrp->pci_device);
	}
    }
}
#endif

