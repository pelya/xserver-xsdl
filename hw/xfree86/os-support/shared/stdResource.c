/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/stdResource.c,v 1.20 2002/01/25 21:56:20 tsi Exp $ */

/* Standard resource information code */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Privstr.h"
#include "xf86Pci.h"
#define NEED_OS_RAC_PROTOS
#include "xf86_OSlib.h"
#include "xf86Resources.h"

/* Avoid Imakefile changes */
#include "bus/Pci.h"

#ifdef USESTDRES
#define xf86StdBusAccWindowsFromOS xf86BusAccWindowsFromOS
#define xf86StdAccResFromOS xf86AccResFromOS
#define xf86StdPciBusAccWindowsFromOS xf86PciBusAccWindowsFromOS
#define xf86StdIsaBusAccWindowsFromOS xf86IsaBusAccWindowsFromOS

resRange PciAvoid[] = {_PCI_AVOID_PC_STYLE, _END};
#endif

#ifdef INCLUDE_XF86_NO_DOMAIN

resPtr
xf86StdBusAccWindowsFromOS(void)
{
    /* Fallback is to allow addressing of all memory space */
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    /* Fallback is to allow addressing of all I/O space */
    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

resPtr
xf86StdPciBusAccWindowsFromOS(void)
{
    /* Fallback is to allow addressing of all memory space */
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    /* Fallback is to allow addressing of all I/O space */
    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

#ifdef INCLUDE_UNUSED

resPtr
xf86StdIsaBusAccWindowsFromOS(void)
{
    /* Fallback is to allow addressing of all memory space */
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    /* Fallback is to allow addressing of all I/O space */
    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

#endif /* INCLUDE_UNUSED */

resPtr
xf86StdAccResFromOS(resPtr ret)
{
    resRange range;

    /*
     * Fallback is to claim the following areas:
     *
     * 0x00000000 - 0x0009ffff	low 640k host memory
     * 0x000c0000 - 0x000effff  location of VGA and other extensions ROMS
     * 0x000f0000 - 0x000fffff	system BIOS
     * 0x00100000 - 0x3fffffff	low 1G - 1MB host memory
     * 0xfec00000 - 0xfecfffff	default I/O APIC config space
     * 0xfee00000 - 0xfeefffff	default Local APIC config space
     * 0xffe00000 - 0xffffffff	high BIOS area (should this be included?)
     *
     * reference: Intel 440BX AGP specs
     *
     * The two APIC spaces appear to be BX-specific and should be dealt with
     * elsewhere.
     */

    /* Fallback is to claim 0x0 - 0x9ffff and 0x100000 - 0x7fffffff */
    RANGE(range, 0x00000000, 0x0009ffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0x000c0000, 0x000effff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0x000f0000, 0x000fffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0x00100000, 0x3fffffff,
	ResExcMemBlock | ResBios | ResEstimated);
    ret = xf86AddResToList(ret, &range, -1);
#if 0
    RANGE(range, 0xfec00000, 0xfecfffff, ResExcMemBlock | ResBios);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0xfee00000, 0xfeefffff, ResExcMemBlock | ResBios);
    ret = xf86AddResToList(ret, &range, -1);
#endif
    RANGE(range, 0xffe00000, 0xffffffff, ResExcMemBlock | ResBios);
    ret = xf86AddResToList(ret, &range, -1);

    /*
     * Fallback would be to claim well known ports in the 0x0 - 0x3ff range
     * along with their sparse I/O aliases, but that's too imprecise.  Instead
     * claim a bare minimum here.
     */
    RANGE(range, 0x00000000, 0x000000ff, ResExcIoBlock); /* For mainboard */
    ret = xf86AddResToList(ret, &range, -1);

    /*
     * At minimum, the top and bottom resources must be claimed, so that
     * resources that are (or appear to be) unallocated can be relocated.
     */
/*  RANGE(range, 0x00000000, 0x00000000, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0xffffffff, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0x00000000, 0x00000000, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1); */
    RANGE(range, 0x0000ffff, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);

    /* XXX add others */
    return ret;
}

#endif /* INCLUDE_XF86_NO_DOMAIN */
