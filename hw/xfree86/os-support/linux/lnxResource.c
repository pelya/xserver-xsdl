/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnxResource.c,v 3.19 2004/02/04 16:30:50 tsi Exp $ */

/* Resource information code */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Privstr.h"
#include "xf86Pci.h"
#include "xf86Resources.h"
#define NEED_OS_RAC_PROTOS
#include "xf86_OSlib.h"
#include "lnx.h"

/* Avoid Imakefile changes */
#include "bus/Pci.h"

resRange PciAvoid[] =
{
#if !defined(__sparc__) || !defined(INCLUDE_XF86_NO_DOMAIN)
    _PCI_AVOID_PC_STYLE,
#endif
    _END
};

#ifdef INCLUDE_XF86_NO_DOMAIN

#ifdef __alpha__

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
     * On the Alpha the first 16MB of every 128 Mb segment in
     * sparse address space are an image of the ISA bus range
     */
    if (_bus_base_sparse()) {
	RANGE(range, 0x00000000, 0x07ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x09000000, 0x0fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x11000000, 0x17ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x19000000, 0x1fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x21000000, 0x27ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x29000000, 0x2fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x31000000, 0x37ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x39000000, 0x3fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x41000000, 0x47ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x49000000, 0x4fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x51000000, 0x57ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x59000000, 0x5fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x61000000, 0x67ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x69000000, 0x6fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x71000000, 0x77ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x79000000, 0x7fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x81000000, 0x87ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x89000000, 0x8fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x91000000, 0x97ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0x99000000, 0x9fffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xa1000000, 0xa7ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xa9000000, 0xafffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xb1000000, 0xb7ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xb9000000, 0xbfffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xc1000000, 0xc7ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xc9000000, 0xcfffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xd1000000, 0xd7ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xd9000000, 0xdfffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xe1000000, 0xe7ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xe9000000, 0xefffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xf1000000, 0xf7ffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
	RANGE(range, 0xf9000000, 0xffffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
    } else {
	RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
	ret = xf86AddResToList(ret, &range, -1);
    }
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

#elif defined(__powerpc__) || \
      defined(__sparc__) || \
      defined(__mips__) || \
      defined(__sh__) || \
      defined(__mc68000__) || \
      defined(__arm__) || \
      defined(__s390__) || \
      defined(__hppa__)

 /*
  * XXX this isn't exactly correct but it will get the server working 
  * for now until we get something better.
  */
  
resPtr
xf86BusAccWindowsFromOS(void)
{
    resPtr ret = NULL;
    resRange range;

    RANGE(range, 0x00000000, 0xffffffff, ResExcMemBlock);
    ret = xf86AddResToList(ret, &range, -1);

#if defined(__sparc__) || defined(__powerpc__)
    RANGE(range, 0x00000000, 0x00ffffff, ResExcIoBlock);
#else
    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
#endif
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

#if defined(__sparc__) || defined(__powerpc__)
    RANGE(range, 0x00000000, 0x00ffffff, ResExcIoBlock);
#else
    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
#endif
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

#if defined(__sparc__) || defined(__powerpc__)
    RANGE(range, 0x00000000, 0x00ffffff, ResExcIoBlock);
#else
    RANGE(range, 0x00000000, 0x0000ffff, ResExcIoBlock);
#endif
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
#if defined(__sparc__) || defined(__powerpc__)
    RANGE(range, 0x00ffffff, 0x00ffffff, ResExcIoBlock);
#else
    RANGE(range, 0x0000ffff, 0x0000ffff, ResExcIoBlock);
#endif
    ret = xf86AddResToList(ret, &range, -1);

    return ret;
}

#else

#error : Put your platform dependent code here!!

#endif

#endif /* INCLUDE_XF86_NO_DOMAIN */
