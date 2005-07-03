/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/bus/ppcPci.c,v 1.8 2002/07/24 19:06:52 tsi Exp $ */
/*
 * ppcPci.c - PowerPC PCI access functions
 *
 * PCI driver functions supporting Motorola PowerPC platforms
 * including Powerstack(RiscPC/RiscPC+), PowerStackII, MTX, and
 * MVME 160x/260x/360x/460x VME boards
 *
 * Gary Barton
 * Concurrent Computer Corporation
 * garyb@gate.net
 *
 */

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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <stdio.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "Pci.h"

#ifndef MAP_FAILED
#define MAP_FAILED (pointer)(-1)
#endif

void
ppcPciInit()
{
#if defined(PowerMAX_OS)
  extern void pmaxPciInit(void);

  pmaxPciInit();

#else
  
  static void motoppcPciInit(void);
  motoppcPciInit();

#endif
}

#if defined(PowerMAX_OS)

/*
 * Motorola PowerPC platform support
 *
 * The following code should support the MVME 1600 & 2600 VME boards
 * as well as the various PowerStack and RiscPC models.  All of these
 * machines support PCI config mechanism #1 and use the std config
 * address and data regs locations:
 *	cfg address reg = 0xcf8 (PCI I/O)
 *	cfg data reg = 0xcfc (PCI I/O)
 *
 * The moto machines do have different address maps on either side
 * of the PCI-host bridge though.
 */
static ADDRESS motoppcBusAddrToHostAddr(PCITAG, PciAddrType, ADDRESS);
static ADDRESS motoppcHostAddrToBusAddr(PCITAG, PciAddrType, ADDRESS);
static CARD32 pciCfgMech1Read(PCITAG tag, int offset);
static void pciCfgMech1Write(PCITAG tag, int offset, CARD32 val);
static void pciCfgMech1SetBits(PCITAG tag, int offset,
			       CARD32 mask, CARD32 val);


static pciBusFuncs_t motoppcFuncs0 = {
/* pciReadLong      */	pciCfgMech1Read,
/* pciWriteLong     */	pciCfgMech1Write,
/* pciSetBitsLong   */	pciCfgMech1SetBits,
/* pciAddrHostToBus */	motoppcHostAddrToBusAddr,
/* pciAddrBusToHost */	motoppcBusAddrToHostAddr
};

static pciBusInfo_t motoppcPci0 = {
/* configMech  */	PCI_CFG_MECH_1,
/* numDevices  */	32,
/* secondary   */	FALSE,
/* primary_bus */	0,
#ifdef PowerMAX_OS
/* ppc_io_base */	0x80000000,
/* ppc_io_size */	64 * 1024,
#endif
/* funcs       */	&motoppcFuncs0,
/* pciBusPriv  */	NULL,
/* bridge      */	NULL
};

extern volatile unsigned char *ioBase;

static void
motoppcPciInit()
{
  pciNumBuses    = 1;
  pciBusInfo[0]  = &motoppcPci0;
  pciFindFirstFP = pciGenFindFirst;
  pciFindNextFP  = pciGenFindNext;

  if (!xf86EnableIO())
      FatalError("motoppcPciInit: EnableIO failed\n");
  
  if (ioBase == MAP_FAILED) {
	  ppcPciIoMap(0);  /* Make inb/outb et al work for pci0 and its secondaries */

	  if (ioBase == MAP_FAILED) {
		  FatalError("motoppcPciInit: Cannot map pci0 I/O segment!!!\n");
		  /*NOTREACHED*/
	  }
  }
}

extern unsigned long motoPciMemBase    = 0;

#if defined(Lynx) && defined(__powerpc__)
extern unsigned long motoPciMemLen     = 0x40000000;
#else
extern unsigned long motoPciMemLen     = 0x3f000000;
#endif

extern unsigned long motoPciMemBaseCPU = 0xc0000000;

static ADDRESS
motoppcBusAddrToHostAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
  unsigned long addr_l = (unsigned long)addr;

  if (type == PCI_MEM) {
  if (addr_l >= motoPciMemBase && addr_l < motoPciMemLen)
	  /*
	   * PCI memory space addresses [0-0x3effffff] are
	   * are seen at [0xc0000000,0xfeffffff] on moto host
	   */
	  return((ADDRESS)((motoPciMemBaseCPU - motoPciMemBase) + addr_l));

  else if (addr_l >= 0x80000000)
	  /*
	   * Moto host memory [0,0x7fffffff] is seen at
	   * [0x80000000,0xffffffff] on PCI bus
	   */
	  return((ADDRESS)(addr_l & 0x7fffffff));
  else
	  FatalError("motoppcBusAddrToHostAddr: PCI addr 0x%x is not accessible to host!!!\n",
		     addr_l);
  } else
      return addr;

  /*NOTREACHED*/
}

static ADDRESS
motoppcHostAddrToBusAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
  unsigned long addr_l = (unsigned long)addr;

  if (type == PCI_MEM) {
      if (addr_l < 0x80000000)
	  /*
	   * Moto host memory [0,0x7fffffff] is seen at
	   * [0x80000000,0xffffffff] on PCI bus
	   */
	  return((ADDRESS)(0x80000000 | addr_l));

      else if (addr_l >= motoPciMemBaseCPU && addr_l < motoPciMemBaseCPU + motoPciMemLen)
	  /*
	   * PCI memory space addresses [0-0x3effffff] are
	   * are seen at [0xc0000000,0xfeffffff] on moto host
	   */
	  return((ADDRESS)(addr_l - (motoPciMemBaseCPU - motoPciMemBase)));

      else
	  FatalError("motoppcHostAddrToBusAddr: Host addr 0x%x is not accessible to PCI!!!\n",
		     addr_l);
  } else
      return addr;

  /*NOTREACHED*/
}

#if defined (__powerpc__)
static int buserr_detected;

static
void buserr(int sig)
{
	buserr_detected = 1;
}
#endif

static CARD32
pciCfgMech1Read(PCITAG tag, int offset)
{
  unsigned long rv = 0xffffffff;
#ifdef DEBUGPCI
  ErrorF("pciCfgMech1Read(tag=%08x,offset=%08x)\n", tag, offset);
#endif

#if defined(__powerpc__)
  signal(SIGBUS, buserr);
  buserr_detected = 0;
#endif

  outl(0xCF8, PCI_EN | tag | (offset & 0xfc));
  rv = inl(0xCFC);

#if defined(__powerpc__)
  signal(SIGBUS, SIG_DFL);
  if (buserr_detected)
  {
#ifdef DEBUGPCI
    ErrorF("pciCfgMech1Read() BUS ERROR\n");
#endif
    return(0xffffffff);
  }
  else
#endif
    return(rv);
}

static void
pciCfgMech1Write(PCITAG tag, int offset, CARD32 val)
{
#ifdef DEBUGPCI
  ErrorF("pciCfgMech1Write(tag=%08x,offset=%08x,val=%08x)\n",
        tag, offset,val);
#endif

#if defined(__powerpc__)
  signal(SIGBUS, SIG_IGN);
#endif

  outl(0xCF8, PCI_EN | tag | (offset & 0xfc));
#if defined(Lynx) && defined(__powerpc__)
  outb(0x80, 0x00);	/* without this the next access fails
                         * on my Powerstack system when we use
                         * assembler inlines for outl */
#endif
  outl(0xCFC, val);

#if defined(__powerpc__)
  signal(SIGBUS, SIG_DFL);
#endif
}

static void
pciCfgMech1SetBits(PCITAG tag, int offset, CARD32 mask, CARD32 val)
{
    unsigned long rv = 0xffffffff;

#if defined(__powerpc__)
    signal(SIGBUS, buserr);
#endif

    outl(0xCF8, PCI_EN | tag | (offset & 0xfc));
    rv = inl(0xCFC);
    rv = (rv & ~mask) | val;
    outl(0xCFC, rv);

#if defined(__powerpc__)
    signal(SIGBUS, SIG_DFL);
#endif
}

#endif /* PowerMAX_OS */
