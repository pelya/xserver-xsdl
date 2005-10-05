/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_pci.c,v 3.8 2002/04/09 15:59:37 tsi Exp $ */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
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

FILE *xf86OSLinuxPCIFile = NULL;

Bool
xf86GetPciSizeFromOS(PCITAG tag, int index, int* bits)
{
    char c[0x200];
    char *res;
    unsigned int bus, devfn, dev, fn;
    unsigned PCIADDR_TYPE size[7];
    unsigned int num;
    signed PCIADDR_TYPE Size;

    if (index > 7)
	return FALSE;
    
    if (!xf86OSLinuxPCIFile && \
        !(xf86OSLinuxPCIFile = fopen("/proc/bus/pci/devices","r")))
	return FALSE;
    do {
	res = fgets(c,0x1ff,xf86OSLinuxPCIFile);
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
		fseek(xf86OSLinuxPCIFile, 0L, SEEK_SET);
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
		fseek(xf86OSLinuxPCIFile, 0L, SEEK_SET);
		return TRUE;
	    }
	}
    } while (res);

    fseek(xf86OSLinuxPCIFile, 0L, SEEK_SET);
    return FALSE;
}



/* Query the kvirt address (64bit) of a BAR range from TAG */
Bool
xf86GetPciOffsetFromOS(PCITAG tag, int index, unsigned long* bases)
{
    FILE *file;
    char c[0x200];
    char *res;
    unsigned int bus, devfn, dev, fn;
    unsigned PCIADDR_TYPE offset[7];
    unsigned int num;

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
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         /* 7 PCI resource sizes, and then optionally a driver name */
                         "\t" PCIADDR_IGNORE_FMT
                         "\t" PCIADDR_IGNORE_FMT
                         "\t" PCIADDR_IGNORE_FMT
                         "\t" PCIADDR_IGNORE_FMT
                         "\t" PCIADDR_IGNORE_FMT
                         "\t" PCIADDR_IGNORE_FMT
                         "\t" PCIADDR_IGNORE_FMT,
                         &bus,&devfn,&offset[0],&offset[1],&offset[2],&offset[3],
                         &offset[4],&offset[5],&offset[6]);
            if (num != 9) {  /* apparantly not 2.3 style */
                fclose(file);
                return FALSE;
            }

            dev = devfn >> 3;
            fn = devfn & 0x7;
            if (tag == pciTag(bus,dev,fn)) {
                /* return the offset for the index requested */
                *bases = offset[index];
                fclose(file);
                return TRUE;
            }
        }
    } while (res);

    fclose(file);
    return FALSE;
}

/* Query the kvirt address (64bit) of a BAR range from size for a given TAG */
unsigned long
xf86GetOSOffsetFromPCI(PCITAG tag, int space, unsigned long base)
{
    FILE *file;
    char c[0x200];
    char *res;
    unsigned int bus, devfn, dev, fn;
    unsigned PCIADDR_TYPE offset[7];
    unsigned PCIADDR_TYPE size[7];
    unsigned int num;
    unsigned int ndx;

    if (!(file = fopen("/proc/bus/pci/devices","r")))
        return 0;
    do {
        res = fgets(c,0x1ff,file);
        if (res) {
            num = sscanf(res,
                         /*bus+dev vendorid deviceid irq */
                         "%02x%02x\t%*04x%*04x\t%*x"
                         /* 7 PCI resource base addresses */
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         /* 7 PCI resource sizes, and then optionally a driver name */
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT
                         "\t" PCIADDR_FMT,
                         &bus,&devfn,&offset[0],&offset[1],&offset[2],&offset[3],
                         &offset[4],&offset[5],&offset[6], &size[0], &size[1], &size[2],
		         &size[3], &size[4], &size[5], &size[6]);
            if (num != 16) {  /* apparantly not 2.3 style */
                fclose(file);
                return 0;
            }

            dev = devfn >> 3;
            fn = devfn & 0x7;
            if (tag == pciTag(bus,dev,fn)) {
                /* ok now look through all the BAR values of this device */
                for (ndx=0; ndx<7; ndx++) {
                    unsigned long savePtr;
                    /*
		     * remember to lop of the last 4bits of the BAR values as they are
		     * memory attributes
		     */
		    if (ndx == 6) 
			savePtr = (0xFFFFFFF0) & 
			    pciReadLong(tag, PCI_CMD_BIOS_REG);
		    else /* this the ROM bar */
			savePtr = (0xFFFFFFF0) & 
			    pciReadLong(tag, PCI_CMD_BASE_REG + (0x4 * ndx));

                    /* find the index of the incoming base */
                    if (base >= savePtr && base <= (savePtr + size[ndx])) {
                        fclose(file);
                        return (offset[ndx] & ~(0xFUL)) + (base - savePtr);
                    }
                }
            }
        }
    } while (res);

    fclose(file);
    return 0;

}
