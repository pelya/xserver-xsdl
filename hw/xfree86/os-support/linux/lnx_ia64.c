/*
 * Copyright 2004, Egbert Eich
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * EGBERT EICH BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
 * NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Egbert Eich shall not
 * be used in advertising or otherwise to promote the sale, use or other deal-
 *ings in this Software without prior written authorization from Egbert Eich.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <stdlib.h>

#include "ia64Pci.h"
#include "Pci.h"

#if defined OS_PROBE_PCI_CHIPSET
IA64Chipset OS_PROBE_PCI_CHIPSET(scanpciWrapperOpt flags)
{
    struct stat unused;
    struct utsname utsName;

    if (!stat("/proc/bus/mckinley/zx1",&unused) 
	|| !stat("/proc/bus/mckinley/zx2",&unused))
	return ZX1_CHIPSET;

    if (!stat("/proc/sgi_sn/licenseID", &unused)) {
        int major, minor, patch;
        char *c;

	/* We need a 2.6.11 or better kernel for Altix support */
	uname(&utsName);
        c = utsName.release;
        
        major = atoi(c);
        c = strstr(c, ".") + 1;
        minor = atoi(c);
        c = strstr(c, ".") + 1;
        patch = atoi(c);
        
	if (major < 2 || (major == 2 && minor < 6) ||
            (major == 2 && minor == 6 && patch < 11)) {
	    ErrorF("Kernel 2.6.11 or better needed for Altix support\n");
	    return NONE_CHIPSET;
	}
	return ALTIX_CHIPSET;
    }

    return NONE_CHIPSET;
}
#endif
