/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/pci.c,v 1.12 2002/04/04 14:05:51 eich Exp $ */

/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include "xf86Pci.h"
#include "xf86.h"
#include "xf86_ansic.h"
#define _INT10_PRIVATE
#include "xf86int10.h"

int
mapPciRom(int pciEntity, unsigned char * address)
{
    PCITAG tag;
    unsigned char *mem, *ptr;
    int length;
    
    pciVideoPtr pvp = xf86GetPciInfoForEntity(pciEntity);

    if (pvp == NULL) {
#ifdef DEBUG
	ErrorF("mapPciRom: no PCI info\n");
#endif
	return 0;
    }

    tag = pciTag(pvp->bus,pvp->device,pvp->func);
    length = 1 << pvp->biosSize;

    /* Read in entire PCI ROM */
    mem = ptr = xnfcalloc(length, 1);
    length = xf86ReadPciBIOS(0, tag, -1, ptr, length);
    if (length > 0)
	memcpy(address, ptr, length);
    /* unmap/close/disable PCI bios mem */
    xfree(mem);

#ifdef DEBUG
    if (!length)
	ErrorF("mapPciRom: no BIOS found\n");
#ifdef PRINT_PCI
    else
	dprint(address,0x20);
#endif
#endif

    return length;
}
