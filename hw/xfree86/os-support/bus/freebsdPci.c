/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/freebsdPci.c,v 1.5 2002/08/27 22:07:07 tsi Exp $ */
/*
 * Copyright 1998 by Concurrent Computer Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Concurrent Computer
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Concurrent Computer Corporation makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * CONCURRENT COMPUTER CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CONCURRENT COMPUTER CORPORATION BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *
 * Copyright 1998 by Metro Link Incorporated
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Metro Link
 * Incorporated not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Metro Link Incorporated makes no representations
 * about the suitability of this software for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * METRO LINK INCORPORATED DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL METRO LINK INCORPORATED BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <stdio.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "Pci.h"

#include <sys/pciio.h>

/*
 * freebsd platform specific PCI access functions -- using /dev/pci
 * needs kernel version 2.2.x
 */
static CARD32 freebsdPciCfgRead(PCITAG tag, int off);
static void freebsdPciCfgWrite(PCITAG, int off, CARD32 val);
static void freebsdPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits);

static pciBusFuncs_t freebsdFuncs0 = {
/* pciReadLong      */	freebsdPciCfgRead,
/* pciWriteLong     */	freebsdPciCfgWrite,
/* pciSetBitsLong   */	freebsdPciCfgSetBits,
/* pciAddrHostToBus */	pciAddrNOOP,
/* pciAddrBusToHost */	pciAddrNOOP
};

static pciBusInfo_t freebsdPci0 = {
/* configMech  */	PCI_CFG_MECH_OTHER,
/* numDevices  */	32,
/* secondary   */	FALSE,
/* primary_bus */	0,
#ifdef PowerMAX_OS
/* ppc_io_base */	0,
/* ppc_io_size */	0,
#endif
/* funcs       */	&freebsdFuncs0,
/* pciBusPriv  */	NULL,
/* bridge      */	NULL
};

#if !defined(__OpenBSD__)
#if X_BYTE_ORDER == X_BIG_ENDIAN
#ifdef __sparc__
#ifndef ASI_PL
#define ASI_PL 0x88
#endif
#define PCI_CPU(val) ({									\
int __ret;										\
__asm__ __volatile__("lduwa [%1] %2, %0" : "=r" (__ret) : "r" (&val), "i" (ASI_PL));	\
__ret;											\
})
#else
#define PCI_CPU(val)	(((val >> 24) & 0x000000ff) |	\
			 ((val >>  8) & 0x0000ff00) |	\
			 ((val <<  8) & 0x00ff0000) |	\
			 ((val << 24) & 0xff000000))
#endif
#else
#define PCI_CPU(val)	(val)
#endif
#else /* ! OpenBSD */
/* OpenBSD has already the bytes in the right order
   for all architectures */
#define PCI_CPU(val)	(val)
#endif


#define BUS(tag) (((tag)>>16)&0xff)
#define DFN(tag) (((tag)>>8)&0xff)

static int pciFd = -1;

void
freebsdPciInit()
{
	pciFd = open("/dev/pci", O_RDWR);
	if (pciFd < 0)
		return;

	pciNumBuses    = 1;
	pciBusInfo[0]  = &freebsdPci0;
	pciFindFirstFP = pciGenFindFirst;
	pciFindNextFP  = pciGenFindNext;
}

static CARD32
freebsdPciCfgRead(PCITAG tag, int off)
{
	struct pci_io io;
	int error;
	io.pi_sel.pc_bus = BUS(tag);
	io.pi_sel.pc_dev = DFN(tag) >> 3;
	io.pi_sel.pc_func = DFN(tag) & 7;
	io.pi_reg = off;
	io.pi_width = 4;
	error = ioctl(pciFd, PCIOCREAD, &io);
	if (error)
		return ~0;
	return PCI_CPU(io.pi_data);
}

static void
freebsdPciCfgWrite(PCITAG tag, int off, CARD32 val)
{
	struct pci_io io;
	io.pi_sel.pc_bus = BUS(tag);
	io.pi_sel.pc_dev = DFN(tag) >> 3;
	io.pi_sel.pc_func = DFN(tag) & 7;
	io.pi_reg = off;
	io.pi_width = 4;
	io.pi_data = PCI_CPU(val);
	ioctl(pciFd, PCIOCWRITE, &io);
}

static void
freebsdPciCfgSetBits(PCITAG tag, int off, CARD32 mask, CARD32 bits)
{
	CARD32	val = freebsdPciCfgRead(tag, off);
	val = (val & ~mask) | (bits & mask);
	freebsdPciCfgWrite(tag, off, val);
}
