/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bsd/bsdResource.c,v 1.8 2002/05/22 21:38:29 herrb Exp $ */

/* Resource information code */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Privstr.h"
#include "xf86Pci.h"
#include "xf86Resources.h"
#define NEED_OS_RAC_PROTOS
#include "xf86_OSlib.h"

/* Avoid Imakefile changes */
#include "bus/Pci.h"

resRange PciAvoid[] = {_PCI_AVOID_PC_STYLE, _END};

#ifdef INCLUDE_XF86_NO_DOMAIN

#if defined(__alpha__) || defined(__sparc64__)

resPtr
xf86BusAccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    RANGE(range, 0x00000000, 0xffffffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

resPtr
xf86PciBusAccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    /*
     * Only allow the upper half of the pci memory range to be used
     * for allocation. The lower half includes magic regions for DMA.
     * XXX this is not right for XP1000's and similar where we use the 
     * region 0x40000000-0xbfffffff for DMA but this only matters if
     * the bios screws up the pci region mappings.
     */
    RANGE(range, 0x80000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    RANGE(range, 0x00000000, 0xffffffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

#ifdef INCLUDE_UNUSED

resPtr
xf86IsaBusAccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    RANGE(range, 0x00000000, 0xffffffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

#endif /* INCLUDE_UNUSED */

resPtr
xf86AccResFromOS(resPtr ret)
{
    resRange range;

    /*
     * Fallback is to claim the following areas:
     *
     * 0x000c0000 - 0x000effff  location of VGA and other extensions ROMS
     */

    RANGE(range, 0x000c0000, 0x000effff, ResExcMemBlock);
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
    RANGE(range, 0x00000000, 0x00000000, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0xffffffff, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
/*  RANGE(range, 0x00000000, 0x00000000, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1); */
    RANGE(range, 0xffffffff, 0xffffffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);

    /* XXX add others */
    return ret;
}

#elif defined(__powerpc__)

resPtr
xf86BusAccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

resPtr
xf86PciBusAccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

#ifdef INCLUDE_UNUSED

resPtr
xf86IsaBusAccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    return ret;
}

#endif /* INCLUDE_UNUSED */

resPtr
xf86AccResFromOS(resPtr ret)
{
    resRange range;

    /*
     * At minimum, the top and bottom resources must be claimed, so that
     * resources that are (or appear to be) unallocated can be relocated.
     */
    RANGE(range, 0x00000000, 0x00000000, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0xffffffff, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0x00000000, 0x00000000, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);
    RANGE(range, 0x0000ffff, 0x0000ffff, ResExcIoBlock);
    ret = xf86AddResToList(ret, &range, -1);

    return ret;
}

#else

#error : Put your platform dependent code here!!

#endif

#endif /* INCLUDE_XF86_NO_DOMAIN */
