/* $XFree86: xc/programs/Xserver/hw/xfree86/scanpci/xf86PciStr.h,v 1.1 2002/07/15 20:46:04 dawes Exp $ */

/*
 * Copyright © 2002 by The XFree86 Project, Inc
 */
 
/*
 * Structs used to hold the pre-parsed pci.ids data.  These are private
 * to the scanpci and pcidata modules.
 */

#ifndef _XF86_PCISTR_H
#define _XF86_PCISTR_H

typedef struct {
    unsigned short VendorID;
    unsigned short SubsystemID;
    const char *SubsystemName;
    unsigned short class;
} pciSubsystemInfo;

typedef struct {
    unsigned short DeviceID;
    const char *DeviceName;
    const pciSubsystemInfo **Subsystem;
    unsigned short class;
} pciDeviceInfo;

typedef struct {
    unsigned short VendorID;
    const char *VendorName;
    const pciDeviceInfo **Device;
} pciVendorInfo;

typedef struct {
    unsigned short VendorID;
    const char *VendorName;
    const pciSubsystemInfo **Subsystem;
} pciVendorSubsysInfo;

#endif /* _XF86_PCISTR_H */
