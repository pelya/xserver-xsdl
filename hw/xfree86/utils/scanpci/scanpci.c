/*
 * Copyright 2000 by Egbert Eich
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 * Copyright 2002 by David Dawes
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holder(s)
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  The above listed
 * copyright holder(s) make(s) no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM(S) ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XFree86: xc/programs/Xserver/hw/xfree86/etc/scanpci.c,v 3.92 2003/02/13 12:17:14 tsi Exp $ */

#include "X.h"
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "xf86PciInfo.h"
#include "xf86ScanPci.h"
#include "dummylib.h"

#include <stdarg.h>
#include <stdlib.h>
#ifdef __linux__
/* to get getopt on Linux */
#ifndef __USE_POSIX2
#define __USE_POSIX2
#endif
#endif
#include <unistd.h>

#if defined(ISC) || defined(Lynx)
extern char *optarg;
extern int optind, opterr;
#endif

pciVideoPtr *xf86PciVideoInfo = NULL;

static void usage(void);
static void identify_card(pciConfigPtr pcr, int verbose);
static void print_default_class(pciConfigPtr pcr);
static void print_bridge_pci_class(pciConfigPtr pcr);
static void print_mach64(pciConfigPtr pcr);
static void print_i128(pciConfigPtr pcr);
static void print_dc21050(pciConfigPtr pcr);
static void print_simba(pciConfigPtr pcr);
static void print_460gx_sac(pciConfigPtr pcr);
static void print_460gx_pxb(pciConfigPtr pcr);
static void print_460gx_gxb(pciConfigPtr pcr);

#define MAX_DEV_PER_VENDOR 40

typedef struct {
    unsigned int Vendor;
    struct {
	int DeviceID;
	void(*func)(pciConfigPtr);
    } Device[MAX_DEV_PER_VENDOR];
} pciVendorDevFuncInfo;

static pciVendorDevFuncInfo vendorDeviceFuncInfo[] = {
    { PCI_VENDOR_ATI, {
	{ PCI_CHIP_MACH64CT, print_mach64 },
	{ PCI_CHIP_MACH64CX, print_mach64 },
	{ PCI_CHIP_MACH64ET, print_mach64 },
	{ PCI_CHIP_MACH64GB, print_mach64 },
	{ PCI_CHIP_MACH64GD, print_mach64 },
	{ PCI_CHIP_MACH64GI, print_mach64 },
	{ PCI_CHIP_MACH64GL, print_mach64 },
	{ PCI_CHIP_MACH64GM, print_mach64 },
	{ PCI_CHIP_MACH64GN, print_mach64 },
	{ PCI_CHIP_MACH64GO, print_mach64 },
	{ PCI_CHIP_MACH64GP, print_mach64 },
	{ PCI_CHIP_MACH64GQ, print_mach64 },
	{ PCI_CHIP_MACH64GR, print_mach64 },
	{ PCI_CHIP_MACH64GS, print_mach64 },
	{ PCI_CHIP_MACH64GT, print_mach64 },
	{ PCI_CHIP_MACH64GU, print_mach64 },
	{ PCI_CHIP_MACH64GV, print_mach64 },
	{ PCI_CHIP_MACH64GW, print_mach64 },
	{ PCI_CHIP_MACH64GX, print_mach64 },
	{ PCI_CHIP_MACH64GY, print_mach64 },
	{ PCI_CHIP_MACH64GZ, print_mach64 },
	{ PCI_CHIP_MACH64LB, print_mach64 },
	{ PCI_CHIP_MACH64LD, print_mach64 },
	{ PCI_CHIP_MACH64LG, print_mach64 },
	{ PCI_CHIP_MACH64LI, print_mach64 },
	{ PCI_CHIP_MACH64LM, print_mach64 },
	{ PCI_CHIP_MACH64LN, print_mach64 },
	{ PCI_CHIP_MACH64LP, print_mach64 },
	{ PCI_CHIP_MACH64LQ, print_mach64 },
	{ PCI_CHIP_MACH64LR, print_mach64 },
	{ PCI_CHIP_MACH64LS, print_mach64 },
	{ PCI_CHIP_MACH64VT, print_mach64 },
	{ PCI_CHIP_MACH64VU, print_mach64 },
	{ PCI_CHIP_MACH64VV, print_mach64 },
	{ 0x0000,  NULL } } },
    { PCI_VENDOR_DIGITAL, {
	{ PCI_CHIP_DC21050, print_dc21050},
	{ 0x0000, NULL } } },
    { PCI_VENDOR_NUMNINE, {
	{ PCI_CHIP_I128, print_i128 },
	{ PCI_CHIP_I128_2, print_i128 },
	{ PCI_CHIP_I128_T2R, print_i128 },
	{ PCI_CHIP_I128_T2R4, print_i128 },
	{ 0x0000, NULL } } },
    { PCI_VENDOR_SUN, {
	{ PCI_CHIP_SIMBA, print_simba },
	{ 0x0000, NULL } } },
    { PCI_VENDOR_INTEL, {
	{ PCI_CHIP_460GX_SAC, print_460gx_sac },
	{ PCI_CHIP_460GX_PXB, print_460gx_pxb },
	{ PCI_CHIP_460GX_GXB_1, print_460gx_gxb },
	{ PCI_CHIP_460GX_WXB, print_460gx_pxb },	/* Uncertain */
	{ 0x0000, NULL } } },
    { 0x0000, {
	{ 0x0000, NULL } } }
};

static void
usage(void)
{
    printf("Usage: scanpci [-v12OfV]\n");
    printf("           -v print config space\n");
    printf("           -1 config type 1\n");
    printf("           -2 config type 2\n");
    printf("           -O use OS config support\n");
    printf("           -f force config type\n");
    printf("           -V set message verbosity level\n");
}

int
main(int argc, char *argv[])
{
    pciConfigPtr *pcrpp = NULL;
    int Verbose = 0;
    int i = 0;
    int force = 0;
    int c;

    xf86Info.pciFlags = PCIProbe1;

    while ((c = getopt(argc, argv, "?v12OfV:")) != -1)
	switch(c) {
	case 'v':
	    Verbose = 1;
	    break;
	case '1':
	    xf86Info.pciFlags = PCIProbe1;
	    break;
	case '2':
	    xf86Info.pciFlags = PCIProbe2;
	    break;
	case 'O':
	    xf86Info.pciFlags = PCIOsConfig;
	    break;
	case 'f':
	    force = 1;
	    break;
	case 'V':
	    xf86Verbose = atoi(optarg);
	    break;
	case '?':
	default:
	    usage();
	    exit (1);
	    break;
	}

    if (force)
	switch (xf86Info.pciFlags) {
	case PCIProbe1:
	    xf86Info.pciFlags = PCIForceConfig1;
	    break;
	case PCIProbe2:
	    xf86Info.pciFlags = PCIForceConfig2;
	    break;
	default:
	    break;
	}

    xf86EnableIO();
    pcrpp = xf86scanpci(0);

    if (!pcrpp) {
	printf("No PCI devices found\n");
	xf86DisableIO();
	exit (1);
    }

    while (pcrpp[i])
	identify_card(pcrpp[i++],Verbose);

    xf86DisableIO();
    exit(0);
}

static void
identify_card(pciConfigPtr pcr, int verbose)
{
    int i, j;
    int foundit = 0;
    int foundvendor = 0;
    const char *vname, *dname, *svname, *sname;

    pciVendorDevFuncInfo *vdf = vendorDeviceFuncInfo;

    if (!ScanPciSetupPciIds()) {
	fprintf(stderr, "xf86SetupPciIds() failed\n");
	exit(1);
    }

    printf("\npci bus 0x%04x cardnum 0x%02x function 0x%02x:"
	   " vendor 0x%04x device 0x%04x\n",
	   pcr->busnum, pcr->devnum, pcr->funcnum,
	   pcr->pci_vendor, pcr->pci_device);

    ScanPciFindPciNamesByDevice(pcr->pci_vendor, pcr->pci_device,
			     pcr->pci_subsys_vendor, pcr->pci_subsys_card,
			     &vname, &dname, &svname, &sname);

    if (vname) {
	printf(" %s ", vname);
	if (dname) {
	    printf("%s", dname);
	    foundit = 1;
	}
    }

    if (!foundit)
	printf(" Device unknown\n");
    else {
	printf("\n");
	if (verbose) {
	    for (i = 0;  vdf[i].Vendor;  i++) {
		if (vdf[i].Vendor == pcr->pci_vendor) {
		    for (j = 0;  vdf[i].Device[j].DeviceID;  j++) {
			if (vdf[i].Device[j].DeviceID == pcr->pci_device) {
			    (*vdf[i].Device[j].func)(pcr);
			    return;
			}
		    }
		    break;
		}
	    }
	}
    }

    if (verbose && !(pcr->pci_header_type & 0x7f) &&
	(pcr->pci_subsys_vendor != 0 || pcr->pci_subsys_card != 0) &&
	(pcr->pci_vendor != pcr->pci_subsys_vendor ||
	 pcr->pci_device != pcr->pci_subsys_card)) {
	foundit = 0;
	foundvendor = 0;
	printf(" CardVendor 0x%04x card 0x%04x",
	       pcr->pci_subsys_vendor, pcr->pci_subsys_card);
	if (svname) {
	    printf(" (%s", svname);
	    foundvendor = 1;
	    if (sname) {
		printf(" %s)", sname);
		foundit = 1;
	    }
	}

	if (!foundit) {
	    if (!foundvendor)
		printf(" (");
	    else
		printf(", ");
	    printf("Card unknown)");
	}
	printf("\n");
    }

    if (verbose) {
	printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	       pcr->pci_status, pcr->pci_command);
	printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	       pcr->pci_base_class, pcr->pci_sub_class, pcr->pci_prog_if,
	       pcr->pci_rev_id);
	if ((pcr->pci_base_class == PCI_CLASS_BRIDGE) &&
	    (pcr->pci_sub_class == PCI_SUBCLASS_BRIDGE_PCI))
	    print_bridge_pci_class(pcr);
	else
	    print_default_class(pcr);
    }
}

static void
print_default_class(pciConfigPtr pcr)
{
    printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
	   pcr->pci_bist, pcr->pci_header_type, pcr->pci_latency_timer,
	   pcr->pci_cache_line_size);
    if (pcr->pci_base0) {
	if ((pcr->pci_base0 & 0x7) == 0x4) {
	    printf("  BASE0     0x%08x%08x  addr 0x%08x%08x  MEM%s 64BIT\n",
		   (int)pcr->pci_base1, (int)pcr->pci_base0,
		   (int)pcr->pci_base1,
		   (int)(pcr->pci_base0 &
			 (pcr->pci_base0 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base0 & 0x8) ? " PREFETCHABLE" :"");
	} else {
	    printf("  BASE0     0x%08x  addr 0x%08x  %s%s\n",
		   (int)pcr->pci_base0,
		   (int)(pcr->pci_base0 &
			 (pcr->pci_base0 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base0 & 0x1) ? "I/O" : "MEM",
		   ((pcr->pci_base0 & 0x9) == 0x8) ? " PREFETCHABLE" :"");
	}
    }
    if ((pcr->pci_base1) && ((pcr->pci_base0 & 0x7) != 0x4)) {
	if ((pcr->pci_base1 & 0x7) == 0x4) {
	    printf("  BASE1     0x%08x%08x  addr 0x%08x%08x  MEM%s 64BIT\n",
		   (int)pcr->pci_base2, (int)pcr->pci_base1,
		   (int)pcr->pci_base2,
		   (int)(pcr->pci_base1 &
			 (pcr->pci_base1 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base1 & 0x8) ? " PREFETCHABLE" :"");
	} else {
	    printf("  BASE1     0x%08x  addr 0x%08x  %s%s\n",
		   (int)pcr->pci_base1,
		   (int)(pcr->pci_base1 &
			 (pcr->pci_base1 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base1 & 0x1) ? "I/O" : "MEM",
		   ((pcr->pci_base1 & 0x9) == 0x8) ? " PREFETCHABLE" :"");
	}
    }
    if ((pcr->pci_base2) && ((pcr->pci_base1 & 0x7) != 0x4)) {
	if ((pcr->pci_base2 & 0x7) == 0x4) {
	    printf("  BASE2     0x%08x%08x  addr 0x%08x%08x  MEM%s 64BIT\n",
		   (int)pcr->pci_base3, (int)pcr->pci_base2,
		   (int)pcr->pci_base3,
		   (int)(pcr->pci_base2 &
			 (pcr->pci_base2 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base2 & 0x8) ? " PREFETCHABLE" :"");
	} else {
	    printf("  BASE2     0x%08x  addr 0x%08x  %s%s\n",
		   (int)pcr->pci_base2,
		   (int)(pcr->pci_base2 &
			 (pcr->pci_base2 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base2 & 0x1) ? "I/O" : "MEM",
		   ((pcr->pci_base2 & 0x9) == 0x8) ? " PREFETCHABLE" :"");
	}
    }
    if ((pcr->pci_base3) && ((pcr->pci_base2 & 0x7) != 0x4)) {
	if ((pcr->pci_base3 & 0x7) == 0x4) {
	    printf("  BASE3     0x%08x%08x  addr 0x%08x%08x  MEM%s 64BIT\n",
		   (int)pcr->pci_base4, (int)pcr->pci_base3,
		   (int)pcr->pci_base4,
		   (int)(pcr->pci_base3 &
			 (pcr->pci_base3 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base3 & 0x8) ? " PREFETCHABLE" :"");
	} else {
	    printf("  BASE3     0x%08x  addr 0x%08x  %s%s\n",
		   (int)pcr->pci_base3,
		   (int)(pcr->pci_base3 &
			 (pcr->pci_base3 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base3 & 0x1) ? "I/O" : "MEM",
		   ((pcr->pci_base3 & 0x9) == 0x8) ? " PREFETCHABLE" :"");
	}
    }
    if ((pcr->pci_base4) && ((pcr->pci_base3 & 0x7) != 0x4)) {
	if ((pcr->pci_base4 & 0x7) == 0x4) {
	    printf("  BASE4     0x%08x%08x  addr 0x%08x%08x  MEM%s 64BIT\n",
		   (int)pcr->pci_base5, (int)pcr->pci_base4,
		   (int)pcr->pci_base5,
		   (int)(pcr->pci_base4 &
			 (pcr->pci_base4 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base4 & 0x8) ? " PREFETCHABLE" :"");
	} else {
	    printf("  BASE4     0x%08x  addr 0x%08x  %s%s\n",
		   (int)pcr->pci_base4,
		   (int)(pcr->pci_base4 &
			 (pcr->pci_base4 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)),
		   (pcr->pci_base4 & 0x1) ? "I/O" : "MEM",
		   ((pcr->pci_base4 & 0x9) == 0x8) ? " PREFETCHABLE" :"");
	}
    }
    if ((pcr->pci_base5) && ((pcr->pci_base4 & 0x7) != 0x4)) {
	printf("  BASE5     0x%08x  addr 0x%08x  %s%s%s\n",
	       (int)pcr->pci_base5,
	       (int)(pcr->pci_base5 &
		     (pcr->pci_base5 & 0x1 ?  0xFFFFFFFC : 0xFFFFFFF0)),
	       (pcr->pci_base5 & 0x1) ? "I/O" : "MEM",
	       ((pcr->pci_base5 & 0x9) == 0x8) ? " PREFETCHABLE" :"",
	       ((pcr->pci_base5 & 0x7) == 0x4) ? " 64BIT" : "");
    }
    if (pcr->pci_baserom)
	printf("  BASEROM   0x%08x  addr 0x%08x  %sdecode-enabled\n",
	       (int)pcr->pci_baserom, (int)(pcr->pci_baserom & 0xFFFF8000),
	       pcr->pci_baserom & 0x1 ? "" : "not-");
    if (pcr->pci_max_min_ipin_iline)
	printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x"
	       "  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
	       pcr->pci_max_lat, pcr->pci_min_gnt,
	       pcr->pci_int_pin, pcr->pci_int_line);
    if (pcr->pci_user_config)
	printf("  BYTE_0    0x%02x  BYTE_1  0x%02x"
	       "  BYTE_2  0x%02x  BYTE_3  0x%02x\n",
	       (int)pcr->pci_user_config_0, (int)pcr->pci_user_config_1,
	       (int)pcr->pci_user_config_2, (int)pcr->pci_user_config_3);
}

#define PCI_B_FAST_B_B 0x80
#define PCI_B_SB_RESET 0x40
#define PCI_B_M_ABORT  0x20
#define PCI_B_VGA_EN   0x08
#define PCI_B_ISA_EN   0x04
#define PCI_B_SERR_EN  0x02
#define PCI_B_P_ERR    0x01

static void
print_bridge_pci_class(pciConfigPtr pcr)
{
    printf("  HEADER    0x%02x  LATENCY 0x%02x\n",
	   pcr->pci_header_type, pcr->pci_latency_timer);
    printf("  PRIBUS    0x%02x  SECBUS 0x%02x  SUBBUS 0x%02x\n",
	   pcr->pci_primary_bus_number, pcr->pci_secondary_bus_number,
	   pcr->pci_subordinate_bus_number);
    printf("  SECLT     0x%02x  SECSTATUS 0x%04x\n",
	   pcr->pci_secondary_latency_timer, pcr->pci_secondary_status);

    if (pcr->pci_io_base || pcr->pci_io_limit ||
	pcr->pci_upper_io_base || pcr->pci_upper_io_limit) {
	if (((pcr->pci_io_base & 0x0f) == 0x01) ||
	    ((pcr->pci_io_limit & 0x0f) == 0x01)) {
	    printf("  IOBASE    0x%04x%04x  IOLIM 0x%04x%04x\n",
		   pcr->pci_upper_io_base, (pcr->pci_io_base & 0x00f0) << 8,
		   pcr->pci_upper_io_limit, (pcr->pci_io_limit << 8) | 0x0fff);
	} else {
	    printf("  IOBASE    0x%04x  IOLIM 0x%04x\n",
		   (pcr->pci_io_base & 0x00f0) << 8,
		   (pcr->pci_io_limit << 8) | 0x0fff);
	}
    }

    if (pcr->pci_mem_base || pcr->pci_mem_limit)
	printf("  NOPREFETCH_MEMBASE 0x%08x  MEMLIM 0x%08x\n",
	       (pcr->pci_mem_base & 0x00fff0) << 16,
	       (pcr->pci_mem_limit << 16) | 0x0fffff);

    if (pcr->pci_prefetch_mem_base || pcr->pci_prefetch_mem_limit ||
	pcr->pci_prefetch_upper_mem_base ||
	pcr->pci_prefetch_upper_mem_limit) {
	if (((pcr->pci_prefetch_mem_base & 0x0f) == 0x01) ||
	    ((pcr->pci_prefetch_mem_limit & 0x0f) == 0x01)) {
	    printf("  PREFETCH_MEMBASE   0x%08x%08x  MEMLIM 0x%08x%08x\n",
		   (int)pcr->pci_prefetch_upper_mem_base,
		   (pcr->pci_prefetch_mem_base & 0x00fff0) << 16,
		   (int)pcr->pci_prefetch_upper_mem_limit,
		   (pcr->pci_prefetch_mem_limit << 16) | 0x0fffff);
	} else {
	    printf("  PREFETCH_MEMBASE   0x%08x  MEMLIM 0x%08x\n",
		   (pcr->pci_prefetch_mem_base & 0x00fff0) << 16,
		   (pcr->pci_prefetch_mem_limit << 16) | 0x0fffff);
	}
    }

    printf("  %sFAST_B2B %sSEC_BUS_RST %sM_ABRT %sVGA_EN %sISA_EN"
	   " %sSERR_EN %sPERR_EN\n",
	   (pcr->pci_bridge_control & PCI_B_FAST_B_B) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_SB_RESET) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_M_ABORT) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_VGA_EN) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_ISA_EN) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_SERR_EN) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_P_ERR) ? "" : "NO_");
}

static void
print_mach64(pciConfigPtr pcr)
{
    CARD32 sparse_io = 0;

    printf(" CardVendor 0x%04x card 0x%04x\n",
	   pcr->pci_subsys_vendor, pcr->pci_subsys_card);
    printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	   pcr->pci_status, pcr->pci_command);
    printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	   pcr->pci_base_class, pcr->pci_sub_class,
	   pcr->pci_prog_if, pcr->pci_rev_id);
    printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
	   pcr->pci_bist, pcr->pci_header_type, pcr->pci_latency_timer,
	   pcr->pci_cache_line_size);
    if (pcr->pci_base0)
	printf("  APBASE    0x%08x  addr 0x%08x\n",
	       (int)pcr->pci_base0, (int)(pcr->pci_base0 &
		(pcr->pci_base0 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)));
    if (pcr->pci_base1)
	printf("  BLOCKIO   0x%08x  addr 0x%08x\n",
	       (int)pcr->pci_base1, (int)(pcr->pci_base1 &
		(pcr->pci_base1 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)));
    if (pcr->pci_base2)
	printf("  REGBASE   0x%08x  addr 0x%08x\n",
	       (int)pcr->pci_base2, (int)(pcr->pci_base2 &
		(pcr->pci_base2 & 0x1 ? 0xFFFFFFFC : 0xFFFFFFF0)));
    if (pcr->pci_baserom)
	printf("  BASEROM   0x%08x  addr 0x%08x  %sdecode-enabled\n",
	       (int)pcr->pci_baserom, (int)(pcr->pci_baserom & 0xFFFF8000),
	       pcr->pci_baserom & 0x1 ? "" : "not-");
    if (pcr->pci_max_min_ipin_iline)
	printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x"
	       "  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
	       pcr->pci_max_lat, pcr->pci_min_gnt,
	       pcr->pci_int_pin, pcr->pci_int_line);
    switch (pcr->pci_user_config_0 & 0x03) {
    case 0:
	sparse_io = 0x2ec;
	break;
    case 1:
	sparse_io = 0x1cc;
	break;
    case 2:
	sparse_io = 0x1c8;
	break;
    }
    printf("  SPARSEIO  0x%03x    %s IO enabled    %sable 0x46E8\n",
	   (int)sparse_io, pcr->pci_user_config_0 & 0x04 ? "Block" : "Sparse",
	   pcr->pci_user_config_0 & 0x08 ? "Dis" : "En");
}

static void
print_i128(pciConfigPtr pcr)
{
    printf(" CardVendor 0x%04x card 0x%04x\n",
	   pcr->pci_subsys_vendor, pcr->pci_subsys_card);
    printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	   pcr->pci_status, pcr->pci_command);
    printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	   pcr->pci_base_class, pcr->pci_sub_class, pcr->pci_prog_if,
	   pcr->pci_rev_id);
    printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
	   pcr->pci_bist, pcr->pci_header_type, pcr->pci_latency_timer,
	   pcr->pci_cache_line_size);
    printf("  MW0_AD    0x%08x  addr 0x%08x  %spre-fetchable\n",
	   (int)pcr->pci_base0, (int)(pcr->pci_base0 & 0xFFC00000),
	   pcr->pci_base0 & 0x8 ? "" : "not-");
    printf("  MW1_AD    0x%08x  addr 0x%08x  %spre-fetchable\n",
	   (int)pcr->pci_base1, (int)(pcr->pci_base1 & 0xFFC00000),
	   pcr->pci_base1 & 0x8 ? "" : "not-");
    printf("  XYW_AD(A) 0x%08x  addr 0x%08x\n",
	   (int)pcr->pci_base2, (int)(pcr->pci_base2 & 0xFFC00000));
    printf("  XYW_AD(B) 0x%08x  addr 0x%08x\n",
	   (int)pcr->pci_base3, (int)(pcr->pci_base3 & 0xFFC00000));
    printf("  RBASE_G   0x%08x  addr 0x%08x\n",
	   (int)pcr->pci_base4, (int)(pcr->pci_base4 & 0xFFFF0000));
    printf("  IO        0x%08x  addr 0x%08x\n",
	   (int)pcr->pci_base5, (int)(pcr->pci_base5 & 0xFFFFFF00));
    printf("  RBASE_E   0x%08x  addr 0x%08x  %sdecode-enabled\n",
	   (int)pcr->pci_baserom, (int)(pcr->pci_baserom & 0xFFFF8000),
	   pcr->pci_baserom & 0x1 ? "" : "not-");
    if (pcr->pci_max_min_ipin_iline)
	printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x"
	       "  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
	       pcr->pci_max_lat, pcr->pci_min_gnt,
	       pcr->pci_int_pin, pcr->pci_int_line);
}

static void
print_dc21050(pciConfigPtr pcr)
{
    printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	   pcr->pci_status, pcr->pci_command);
    printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	   pcr->pci_base_class, pcr->pci_sub_class, pcr->pci_prog_if,
	   pcr->pci_rev_id);
    printf("  BIST      0x%02x  HEADER 0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
	   pcr->pci_bist, pcr->pci_header_type, pcr->pci_latency_timer,
	   pcr->pci_cache_line_size);
    printf("  PRIBUS    0x%02x  SECBUS 0x%02x  SUBBUS 0x%02x  SECLT 0x%02x\n",
	   pcr->pci_primary_bus_number, pcr->pci_secondary_bus_number,
	   pcr->pci_subordinate_bus_number, pcr->pci_secondary_latency_timer);
    printf("  IOBASE    0x%02x  IOLIM 0x%02x  SECSTATUS 0x%04x\n",
	   pcr->pci_io_base << 8, (pcr->pci_io_limit << 8) | 0xfff,
	   pcr->pci_secondary_status);
    printf("  NOPREFETCH_MEMBASE 0x%08x  MEMLIM 0x%08x\n",
	   pcr->pci_mem_base << 16, (pcr->pci_mem_limit << 16) | 0xfffff);
    printf("  PREFETCH_MEMBASE   0x%08x  MEMLIM 0x%08x\n",
	   pcr->pci_prefetch_mem_base << 16,
	   (pcr->pci_prefetch_mem_limit << 16) | 0xfffff);
    printf("  RBASE_E   0x%08x  addr 0x%08x  %sdecode-enabled\n",
	   (int)pcr->pci_baserom, (int)(pcr->pci_baserom & 0xFFFF8000),
	   pcr->pci_baserom & 0x1 ? "" : "not-");
    if (pcr->pci_max_min_ipin_iline)
	printf("  MAX_LAT   0x%02x  MIN_GNT 0x%02x"
	       "  INT_PIN 0x%02x  INT_LINE 0x%02x\n",
	       pcr->pci_max_lat, pcr->pci_min_gnt,
	       pcr->pci_int_pin, pcr->pci_int_line);
}

static void
print_simba(pciConfigPtr pcr)
{
    int   i;
    CARD8 io, mem;

    printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	   pcr->pci_status, pcr->pci_command);
    printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	   pcr->pci_base_class, pcr->pci_sub_class, pcr->pci_prog_if,
	   pcr->pci_rev_id);
    printf("  HEADER    0x%02x  LATENCY 0x%02x  CACHE 0x%02x\n",
	   pcr->pci_header_type, pcr->pci_latency_timer,
	   pcr->pci_cache_line_size);
    printf("  PRIBUS    0x%02x  SECBUS 0x%02x  SUBBUS 0x%02x  SECLT 0x%02x\n",
	   pcr->pci_primary_bus_number, pcr->pci_secondary_bus_number,
	   pcr->pci_subordinate_bus_number, pcr->pci_secondary_latency_timer);
    printf("  SECSTATUS 0x%04x\n",
	   pcr->pci_secondary_status);
    printf("  %sFAST_B2B %sSEC_BUS_RST %sM_ABRT %sVGA_EN %sISA_EN"
	   " %sSERR_EN %sPERR_EN\n",
	   (pcr->pci_bridge_control & PCI_B_FAST_B_B) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_SB_RESET) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_M_ABORT) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_VGA_EN) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_ISA_EN) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_SERR_EN) ? "" : "NO_",
	   (pcr->pci_bridge_control & PCI_B_P_ERR) ? "" : "NO_");
    printf("  TICK      0x%08lx         SECCNTL 0x%02x\n", (long)
	   pciReadLong(pcr->tag, 0x00b0), pciReadByte(pcr->tag, 0x00dd));
    printf("  MASTER RETRIES:  PRIMARY 0x%02x,  SECONDARY 0x%02x\n",
	   pciReadByte(pcr->tag, 0x00c0), pciReadByte(pcr->tag, 0x00dc));
    printf("  TARGET RETRIES:  PIO     0x%02x,  DMA       0x%02x\n",
	   pciReadByte(pcr->tag, 0x00d8), pciReadByte(pcr->tag, 0x00da));
    printf("  TARGET LATENCY:  PIO     0x%02x,  DMA       0x%02x\n",
	   pciReadByte(pcr->tag, 0x00d9), pciReadByte(pcr->tag, 0x00db));
    printf("  DMA AFSR  0x%08lx%08lx    AFAR 0x%08lx%08lx\n",
	   (long)pciReadLong(pcr->tag, 0x00cc),
	   (long)pciReadLong(pcr->tag, 0x00c8),
	   (long)pciReadLong(pcr->tag, 0x00d4),
	   (long)pciReadLong(pcr->tag, 0x00d0));
    printf("  PIO AFSR  0x%08lx%08lx    AFAR 0x%08lx%08lx\n",
	   (long)pciReadLong(pcr->tag, 0x00ec),
	   (long)pciReadLong(pcr->tag, 0x00e8),
	   (long)pciReadLong(pcr->tag, 0x00f4),
	   (long)pciReadLong(pcr->tag, 0x00f0));
    printf("  PCI CNTL  0x%08lx%08lx    DIAG 0x%08lx%08lx\n",
	   (long)pciReadLong(pcr->tag, 0x00e4),
	   (long)pciReadLong(pcr->tag, 0x00e0),
	   (long)pciReadLong(pcr->tag, 0x00fc),
	   (long)pciReadLong(pcr->tag, 0x00f8));
    printf("  MAPS:            I/O     0x%02x,  MEM       0x%02x\n",
	   (io  = pciReadByte(pcr->tag, 0x00de)),
	   (mem = pciReadByte(pcr->tag, 0x00df)));
    for (i = 0;  i < 8;  i++)
	if (io & (1 << i))
	    printf("  BUS I/O  0x%06x-0x%06x\n", i << 21, ((i + 1) << 21) - 1);
    for (i = 0;  i < 8;  i++)
	if (mem & (1 << i))
	    printf("  BUS MEM  0x%08x-0x%08x\n", i << 29, ((i + 1) << 29) - 1);
}

static int cbn_460gx = -1;

static void
print_460gx_sac(pciConfigPtr pcr)
{
    CARD32 tmp;

    /* Print generalities */
    printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	   pcr->pci_status, pcr->pci_command);
    printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	   pcr->pci_base_class, pcr->pci_sub_class, pcr->pci_prog_if,
	   pcr->pci_rev_id);

    tmp = pcr->pci_user_config;
    pcr->pci_user_config = 0;
    print_default_class(pcr);
    pcr->pci_user_config = tmp;

    /* Only print what XFree86 might be interested in */
    if (pcr->busnum == 0) {
	if ((pcr->devnum != 0x10) || (pcr->funcnum != 0))
	    return;

	/* Get Chipset Bus Number */
	cbn_460gx = (unsigned int)pciReadByte(pcr->tag, 0x0040);
	printf("  CBN       0x%02x  CBUSES 0x%02x\n",
	       cbn_460gx, pciReadByte(pcr->tag, 0x0044));

	return;
    }

    if ((pcr->busnum != cbn_460gx) || (pcr->funcnum != 0))
	return;

    switch (pcr->devnum) {
    case 0:
	printf("  F16NUM    0x%02x  F16CPL 0x%02x  DEVNPRES 0x%08lx\n",
	       pciReadByte(pcr->tag, 0x0060), pciReadByte(pcr->tag, 0x0078),
	       (long)pciReadLong(pcr->tag, 0x0070));

	return;

    case 0x10:
	printf("  TOM       0x%04x  IORD  0x%04x\n",
	       pciReadWord(pcr->tag, 0x0050), pciReadWord(pcr->tag, 0x008E));
	/* Fall through */

    case 0x11:  case 0x12:  case 0x13:
    case 0x14:  case 0x15:  case 0x16:  case 0x17:
	printf("  BUSNO     0x%02x    SUBNO 0x%02x\n",
	       pciReadByte(pcr->tag, 0x0048), pciReadByte(pcr->tag, 0x0049));
	printf("  VGASE     0x%02x    PCIS  0x%02x    IOR 0x%02x\n",
	       pciReadByte(pcr->tag, 0x0080), pciReadByte(pcr->tag, 0x0084),
	       pciReadByte(pcr->tag, 0x008C));
	/* Fall through */

    default:
	return;
    }
}

static void
print_460gx_pxb(pciConfigPtr pcr)
{
    CARD32 tmp;

    /* Print generalities */
    printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	   pcr->pci_status, pcr->pci_command);
    printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	   pcr->pci_base_class, pcr->pci_sub_class, pcr->pci_prog_if,
	   pcr->pci_rev_id);

    tmp = pcr->pci_user_config;
    pcr->pci_user_config = 0;
    print_default_class(pcr);
    pcr->pci_user_config = tmp;

    /* Only print what XFree86 might be interested in */
    printf("  ERRCMD    0x%02x  GAPEN 0x%02x\n",
	   pciReadByte(pcr->tag, 0x0046), pciReadByte(pcr->tag, 0x0060));
}

static void
print_460gx_gxb(pciConfigPtr pcr)
{
    CARD32 tmp;

    /* Print generalities */
    printf("  STATUS    0x%04x  COMMAND 0x%04x\n",
	   pcr->pci_status, pcr->pci_command);
    printf("  CLASS     0x%02x 0x%02x 0x%02x  REVISION 0x%02x\n",
	   pcr->pci_base_class, pcr->pci_sub_class, pcr->pci_prog_if,
	   pcr->pci_rev_id);

    tmp = pcr->pci_user_config;
    pcr->pci_user_config = 0;
    print_default_class(pcr);
    pcr->pci_user_config = tmp;

    /* Only print what XFree86 might be interested in */
    printf("  BAPBASE   0x%08lx%08lx   AGPSIZ  0x%02x   VGAGE     0x%02x\n",
	   (long)pciReadLong(pcr->tag, 0x009C),
	   (long)pciReadLong(pcr->tag, 0x0098),
	   pciReadByte(pcr->tag, 0x00A2), pciReadByte(pcr->tag, 0x0060));
}

#include "xf86getpagesize.c"
