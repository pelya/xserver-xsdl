/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_pci.c,v 3.9 2003/02/17 15:29:22 dawes Exp $ */

#include <stdio.h>
#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include "xf86Pci.h"

#ifdef __sparc__
#define PCIADDR_TYPE		long long
#define PCIADDR_IGNORE_FMT	"%*x"
#define PCIADDR_FMT		"%llx"
#else
#define PCIADDR_TYPE		long
#define PCIADDR_IGNORE_FMT	"%*x"
#define PCIADDR_FMT		"%lx"
#endif

Bool
xf86GetPciSizeFromOS(PCITAG tag, int index, int* bits)
{
    FILE *file;
    char c[0x200];
    char *res;
    unsigned int bus, devfn, dev, fn;
    unsigned PCIADDR_TYPE size[7];
    unsigned int num;
    signed PCIADDR_TYPE Size;

    if (index > 7)
	return FALSE;
    
    if (!(file = fopen("/proc/bus/pci/devices","r")))
	return FALSE;
    do {
	res = fgets(c,0x1ff,file);
	if (res) {
	    num = sscanf(res,
			 /*bus+dev vendorid deviceid irq */
			 "%02x%02x\t%*04x%*04x\t%*x"
			 /* 7 PCI resource base addresses */
			 "\t" PCIADDR_IGNORE_FMT
			 "\t" PCIADDR_IGNORE_FMT
			 "\t" PCIADDR_IGNORE_FMT
			 "\t" PCIADDR_IGNORE_FMT
			 "\t" PCIADDR_IGNORE_FMT
			 "\t" PCIADDR_IGNORE_FMT
			 "\t" PCIADDR_IGNORE_FMT
			 /* 7 PCI resource sizes, and then optionally a driver name */
			 "\t" PCIADDR_FMT
			 "\t" PCIADDR_FMT
			 "\t" PCIADDR_FMT
			 "\t" PCIADDR_FMT
			 "\t" PCIADDR_FMT
			 "\t" PCIADDR_FMT
			 "\t" PCIADDR_FMT,
			 &bus,&devfn,&size[0],&size[1],&size[2],&size[3],
			 &size[4],&size[5],&size[6]);
	    if (num != 9) {  /* apparantly not 2.3 style */ 
		fclose(file);
		return FALSE;
	    }
	    dev = devfn >> 3;
	    fn = devfn & 0x7;
	    if (tag == pciTag(bus,dev,fn)) {
		*bits = 0;
		if (size[index] != 0) {
		    Size = size[index] - ((PCIADDR_TYPE) 1);
		    while (Size & ((PCIADDR_TYPE) 0x01)) {
			Size = Size >> ((PCIADDR_TYPE) 1);
			(*bits)++;
		    }
		}
		fclose(file);
		return TRUE;
	    }
	}
    } while (res);

    fclose(file);
    return FALSE;
}
