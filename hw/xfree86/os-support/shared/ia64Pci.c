/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/ia64Pci.c,v 1.3 2003/02/23 20:26:49 tsi Exp $ */
/*
 * Copyright (C) 2002-2003 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */

/*
 * This file contains the glue needed to support various IA-64 chipsets.
 */

#include "460gxPCI.h"
#include "e8870PCI.h"
#include "zx1PCI.h"
#include "Pci.h"
#include "ia64Pci.h"

void
ia64ScanPCIWrapper(scanpciWrapperOpt flags)
{
    static IA64Chipset chipset = NONE_CHIPSET;
    
    if (flags == SCANPCI_INIT) {

	/* PCI configuration space probes should be done first */
	if (xorgProbe460GX(flags)) {
	    chipset = I460GX_CHIPSET;
	    xf86PreScan460GX();	
	    return;
	} else if (xorgProbeE8870(flags)) {
	    chipset = E8870_CHIPSET;
	    xf86PreScanE8870();
	    return;
	}
#ifdef OS_PROBE_PCI_CHIPSET
	chipset = OS_PROBE_PCI_CHIPSET(flags);
	switch (chipset) {
	    case ZX1_CHIPSET:
		xf86PreScanZX1();
		return;
	    default:
		return;
	}
#endif
    } else /* if (flags == SCANPCI_TERM) */ {

	switch (chipset) {
	    case I460GX_CHIPSET:
		xf86PostScan460GX();
		return;
	    case E8870_CHIPSET:
		xf86PostScanE8870();
		return;
	    case ZX1_CHIPSET:
		xf86PostScanZX1();
		return;
	    default:
		return;
	}
    }
}
