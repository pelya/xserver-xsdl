/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/stdPci.c,v 3.2 1999/12/06 03:55:13 robin Exp $ */

#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Privstr.h"
#include "xf86Pci.h"
#define NEED_OS_RAC_PROTOS
#include "xf86_OSlib.h"

#ifndef HAVE_PCI_SIZE_FUNC
#define xf86StdGetPciSizeFromOS xf86GetPciSizeFromOS
#endif

Bool
xf86StdGetPciSizeFromOS(PCITAG tag, int index, int* bits)
{
    return FALSE;
}
