/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/pci.c,v 1.11 2001/10/01 13:44:13 eich Exp $ */

/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>

#include "xf86Pci.h"
#include "xf86.h"
#define _INT10_PRIVATE
#include "xf86int10.h"

int
mapPciRom(int pciEntity, unsigned char * address)
{
    struct pci_device * pvp = xf86GetPciInfoForEntity(pciEntity);
    int err;

    if (pvp == NULL) {
#ifdef DEBUG
	ErrorF("mapPciRom: no PCI info\n");
#endif
	return 0;
    }

    /* Read in entire PCI ROM */
    err = pci_device_read_rom( pvp, address );

#ifdef DEBUG
    if ( err != 0 )
	ErrorF("mapPciRom: no BIOS found\n");
#ifdef PRINT_PCI
    else
	dprint(address,0x20);
#endif
#endif

    return pvp->rom_size;
}
