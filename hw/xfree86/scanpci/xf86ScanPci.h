/* $XFree86: xc/programs/Xserver/hw/xfree86/scanpci/xf86ScanPci.h,v 1.2 2002/07/15 20:46:04 dawes Exp $ */



#ifndef SCANPCI_H_
#define SCANPCI_H_

#include "xf86PciData.h"

typedef void (*ScanPciDisplayCardInfoProcPtr)(int verbosity);

/*
 * Whoever loads this module needs to define these and initialise them
 * after loading.
 */

extern ScanPciDisplayCardInfoProcPtr xf86DisplayPCICardInfo;

void ScanPciDisplayPCICardInfo(int verbosity);

#endif
