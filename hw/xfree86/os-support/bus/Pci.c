/*
 * Pci.c - New server PCI access functions
 *
 * The XFree86 server PCI access functions have been reimplemented as a
 * framework that allows each supported platform/OS to have their own
 * platform/OS specific pci driver.
 *
 * All of the public PCI access functions exported to the other parts of
 * the server are declared in Pci.h and defined herein.  These include:
 *	pciInit()              - Initialize PCI access functions
 *	pciFindFirst()         - Find a PCI device by dev/vend id
 *	pciFindNext()          - Find another PCI device by dev/vend id
 *	pciReadLong()          - Read a 32 bit value from a device's cfg space
 *	pciReadWord()          - Read a 16 bit value from a device's cfg space
 *	pciReadByte()          - Read an 8 bit value from a device's cfg space
 *	pciWriteLong()         - Write a 32 bit value to a device's cfg space
 *	pciWriteWord()         - Write a 16 bit value to a device's cfg space
 *	pciWriteByte()         - Write an 8 bit value to a device's cfg space
 *	pciSetBitsLong()       - Write a 32 bit value against a mask
 *	pciSetBitsByte()       - Write an 8 bit value against a mask
 *	pciTag()               - Return tag for a given PCI bus, device, &
 *                               function
 *	pciDomTag()            - Return tag for a given PCI domain, bus,
 *                               device & function
 *	pciBusAddrToHostAddr() - Convert a PCI address to a host address
 *	pciHostAddrToBusAddr() - Convert a host address to a PCI address
 *	pciGetBaseSize()       - Returns the number of bits in a PCI base
 *                               addr mapping
 *	xf86MapPciMem()        - Like xf86MapVidMem() except function expects
 *                               a PCI address and a PCITAG that identifies
 *                               a PCI device
 *	xf86ReadPciBIOS()      - Like xf86ReadBIOS() but can handle PCI/host
 *                               address translation and BIOS decode enabling
 *	xf86scanpci()          - Return info about all PCI devices
 *	xf86GetPciDomain()     - Return domain number from a PCITAG
 *	xf86MapDomainMemory()  - Like xf86MapPciMem() but can handle
 *                               domain/host address translation
 *	xf86MapDomainIO()      - Maps PCI I/O spaces
 *	xf86ReadDomainMemory() - Like xf86ReadPciBIOS() but can handle
 *                               domain/host address translation
 *
 * The actual PCI backend driver is selected by the pciInit() function
 * (see below)  using either compile time definitions, run-time checks,
 * or both.
 *
 * Certain generic functions are provided that make the implementation
 * of certain well behaved platforms (e.g. those supporting PCI config
 * mechanism 1 or some thing close to it) very easy.
 *
 * Less well behaved platforms/OS's can roll their own functions.
 *
 * To add support for another platform/OS, add a call to fooPciInit() within
 * pciInit() below under the correct compile time definition or run-time
 * conditional.
 *
 * The fooPciInit() procedure must do three things:
 *	1) Initialize the pciBusTable[] for all primary PCI buses including
 *	   the per domain PCI access functions (readLong, writeLong,
 *	   addrBusToHost, and addrHostToBus).
 *
 *	2) Add entries to pciBusTable[] for configured secondary buses.  This
 *	   step may be skipped if a platform is using the generic findFirst/
 *	   findNext functions because these procedures will automatically
 *	   discover and add secondary buses dynamically.
 *
 *      3) Overide default settings for global PCI access functions if
 *	   required. These include pciFindFirstFP, pciFindNextFP,
 *	   Of course, if you choose not to use one of the generic
 *	   functions, you will need to provide a platform specifc replacement.
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
 *
 * This software is derived from the original XFree86 PCI code
 * which includes the following copyright notices as well:
 *
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
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
 * This code is also based heavily on the code in FreeBSD-current, which was
 * written by Wolfgang Stanglmeier, and contains the following copyright:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Copyright (c) 1999-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <errno.h>
#include <signal.h>
#include <X11/Xarch.h>
#include "compiler.h"
#include "xf86.h"
#include "xf86Priv.h"
#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#include "Pci.h"

#define PCI_MFDEV_SUPPORT   1 /* Include PCI multifunction device support */
#define PCI_BRIDGE_SUPPORT  1 /* Include support for PCI-to-PCI bridges */

/*
 * Global data
 */
static int    pciInitialized = 0;

CARD32 pciDevid;            /* Requested device/vendor ID (after mask)  */
CARD32 pciDevidMask;        /* Bit mask applied (AND) before comparison */
			    /* of real devid's with requested           */

int    pciBusNum;           /* Bus Number of current device */
int    pciDevNum;           /* Device number of current device */
int    pciFuncNum;          /* Function number of current device */
PCITAG pciDeviceTag;        /* Tag for current device */

pciBusInfo_t  *pciBusInfo[MAX_PCI_BUSES] = { NULL, };
_X_EXPORT int            pciNumBuses = 0;     /* Actual number of PCI buses */
int            pciMaxBusNum = MAX_PCI_BUSES;
static Bool    inProbe = FALSE;

static pciConfigPtr pci_devp[MAX_PCI_DEVICES + 1] = {NULL, };

static int readPciBios( PCITAG Tag, CARD8* tmp, ADDRESS hostbase,
			unsigned char * buf, int len, PciBiosType BiosType );

static int (*pciOSHandleBIOS)(PCITAG Tag, int basereg, unsigned char *buf, int len);

int xf86MaxPciDevs = MAX_PCI_DEVICES;

/*
 * Platform specific PCI function pointers.
 *
 * NOTE: A platform/OS specific pci init procedure can override these defaults
 *       by setting them to the appropriate platform dependent functions.
 */
PCITAG     (*pciFindFirstFP)(void) = pciGenFindFirst;
PCITAG     (*pciFindNextFP)(void) = pciGenFindNext;

/*
 * pciInit - choose correct platform/OS specific PCI init routine
 */
void
pciInit()
{
	if (pciInitialized)
		return;

	pciInitialized = 1;

	/* XXX */
#if defined(DEBUGPCI)
	if (DEBUGPCI >= xf86Verbose)
		xf86Verbose = DEBUGPCI;
#endif

	ARCH_PCI_INIT();
#if defined(ARCH_PCI_OS_INIT)
	if (pciNumBuses <= 0)
	    ARCH_PCI_OS_INIT();
#endif
}

void pciSetOSBIOSPtr(int (*bios_fn)(PCITAG Tag, int basereg, unsigned char * buf, int len))
{
	pciOSHandleBIOS = bios_fn;
}

_X_EXPORT PCITAG
pciFindFirst(CARD32 id, CARD32 mask)
{
#ifdef DEBUGPCI
  ErrorF("pciFindFirst(0x%lx, 0x%lx), pciInit = %d\n", id, mask, pciInitialized);
#endif
  pciInit();

  pciDevid = id & mask;
  pciDevidMask = mask;

  return((*pciFindFirstFP)());
}

_X_EXPORT PCITAG
pciFindNext(void)
{
#ifdef DEBUGPCI
  ErrorF("pciFindNext(), pciInit = %d\n", pciInitialized);
#endif
  pciInit();

  return((*pciFindNextFP)());
}

_X_EXPORT CARD32
pciReadLong(PCITAG tag, int offset)
{
  int bus = PCI_BUS_FROM_TAG(tag);

#ifdef DEBUGPCI
  ErrorF("pciReadLong(0x%lx, %d)\n", tag, offset);
#endif
  pciInit();

  if ((bus >= 0) && ((bus < pciNumBuses) || inProbe) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciReadLong) {
    CARD32 rv = (*pciBusInfo[bus]->funcs->pciReadLong)(tag, offset);

    PCITRACE(1, ("pciReadLong: tag=0x%x [b=%d,d=%d,f=%d] returns 0x%08x\n",
		 tag, bus, PCI_DEV_FROM_TAG(tag), PCI_FUNC_FROM_TAG(tag), rv));
    return(rv);
   }

  return(PCI_NOT_FOUND);
}

_X_EXPORT CARD16
pciReadWord(PCITAG tag, int offset)
{
  CARD32 tmp;
  int    shift = (offset & 3) * 8;
  int    aligned_offset = offset & ~3;
  int	 bus = PCI_BUS_FROM_TAG(tag);

  if (shift != 0 && shift != 16)
	  FatalError("pciReadWord: Alignment error: Cannot read 16 bits "
		     "at offset %d\n", offset);

  pciInit();

  if ((bus >= 0) && ((bus < pciNumBuses) || inProbe) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciReadWord) {
    CARD32 rv = (*pciBusInfo[bus]->funcs->pciReadWord)(tag, offset);

    return(rv);
  } else {
    tmp = pciReadLong(tag, aligned_offset);

    return((CARD16)((tmp >> shift) & 0xffff));
  }
}

_X_EXPORT CARD8
pciReadByte(PCITAG tag, int offset)
{
  CARD32 tmp;
  int    shift = (offset & 3) * 8;
  int    aligned_offset = offset & ~3;
  int	 bus = PCI_BUS_FROM_TAG(tag);

  pciInit();

  if ((bus >= 0) && ((bus < pciNumBuses) || inProbe) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciReadByte) {
    CARD8 rv = (*pciBusInfo[bus]->funcs->pciReadByte)(tag, offset);

    return(rv);
  } else {
    tmp = pciReadLong(tag, aligned_offset);

    return((CARD8)((tmp >> shift) & 0xff));
  }
}

_X_EXPORT void
pciWriteLong(PCITAG tag, int offset, CARD32 val)
{
  int bus = PCI_BUS_FROM_TAG(tag);

  pciInit();

  if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciWriteLong)
	  (*pciBusInfo[bus]->funcs->pciWriteLong)(tag, offset, val);
}

_X_EXPORT void
pciWriteWord(PCITAG tag, int offset, CARD16 val)
{
  CARD32 tmp;
  int    aligned_offset = offset & ~3;
  int    shift = (offset & 3) * 8;
  int	 bus = PCI_BUS_FROM_TAG(tag);

  if (shift != 0 && shift != 16)
	  FatalError("pciWriteWord: Alignment Error: Cannot read 16 bits "
			"from offset %d\n", offset);

  pciInit();

  if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
      pciBusInfo[bus]->funcs->pciWriteWord) {
    (*pciBusInfo[bus]->funcs->pciWriteWord)(tag, offset, val);
  } else {
    tmp = pciReadLong(tag, aligned_offset);

    tmp &= ~(0xffffL << shift);
    tmp |= (((CARD32)val) << shift);
    
    pciWriteLong(tag, aligned_offset, tmp);
  }
}

_X_EXPORT void
pciWriteByte(PCITAG tag, int offset, CARD8 val)
{
  CARD32 tmp;
  int    aligned_offset = offset & ~3;
  int    shift = (offset & 3) *8 ;
  int	 bus = PCI_BUS_FROM_TAG(tag);

  pciInit();

  if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
      pciBusInfo[bus]->funcs->pciWriteByte) {
	  (*pciBusInfo[bus]->funcs->pciWriteByte)(tag, offset, val);
  } else {

    tmp = pciReadLong(tag, aligned_offset);
    
    tmp &= ~(0xffL << shift);
    tmp |= (((CARD32)val) << shift);
    
    pciWriteLong(tag, aligned_offset, tmp);
  }
}

_X_EXPORT void
pciSetBitsLong(PCITAG tag, int offset, CARD32 mask, CARD32 val)
{
    int bus = PCI_BUS_FROM_TAG(tag);

#ifdef DEBUGPCI
    ErrorF("pciReadLong(0x%lx, %d)\n", tag, offset);
#endif
    pciInit();

    if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciSetBitsLong) {
	(*pciBusInfo[bus]->funcs->pciSetBitsLong)(tag, offset, mask, val);
    }
}

void
pciSetBitsByte(PCITAG tag, int offset, CARD8 mask, CARD8 val)
{
  CARD32 tmp_mask, tmp_val;
  int    aligned_offset = offset & ~3;
  int    shift = (offset & 3) *8 ;

  tmp_mask = mask << shift;
  tmp_val = val << shift;
  pciSetBitsLong(tag, aligned_offset, tmp_mask, tmp_val);
}

_X_EXPORT ADDRESS
pciBusAddrToHostAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
  int bus = PCI_BUS_FROM_TAG(tag);

  pciInit();

  if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciAddrBusToHost)
	  return (*pciBusInfo[bus]->funcs->pciAddrBusToHost)(tag, type, addr);
  else
	  return(addr);
}

_X_EXPORT ADDRESS
pciHostAddrToBusAddr(PCITAG tag, PciAddrType type, ADDRESS addr)
{
  int bus = PCI_BUS_FROM_TAG(tag);

  pciInit();

  if ((bus >= 0) && (bus < pciNumBuses) && pciBusInfo[bus] &&
	pciBusInfo[bus]->funcs->pciAddrHostToBus)
	  return (*pciBusInfo[bus]->funcs->pciAddrHostToBus)(tag, type, addr);
  else
	  return(addr);
}

/*
 * pciGetBaseSize() returns the size of a PCI base address mapping in bits.
 * The index identifies the base register: 0-5 are the six standard registers,
 * and 6 is the ROM base register.  If destructive is TRUE, it will write
 * to the base address register to get an accurate result.  Otherwise it
 * makes a conservative guess based on the alignment of the already allocated
 * address.  If the result is accurate (ie, not an over-estimate), this is
 * indicated by setting *min to TRUE (when min is non-NULL).  This happens
 * when either the destructive flag is set, the information is supplied by
 * the OS if the OS supports this.
 */

int
pciGetBaseSize(PCITAG tag, int index, Bool destructive, Bool *min)
{
  int offset;
  CARD32 addr1;
  CARD32 addr2;
  CARD32 mask1;
  CARD32 mask2;
  int bits = 0;

  /*
   * Eventually a function for this should be added to pciBusFuncs_t, but for
   * now we'll just use a simple method based on the alignment of the already
   * allocated address.
   */

  /*
   * silently ignore bogus index values.  Valid values are 0-6.  0-5 are
   * the 6 base address registers, and 6 is the ROM base address register.
   */
  if (index < 0 || index > 6)
    return 0;

  pciInit();

  if (xf86GetPciSizeFromOS(tag, index, &bits)) {
      if (min)
	  *min = TRUE;
      return bits;
  }

  if (min)
    *min = destructive;

  /* Get the PCI offset */
  if (index == 6)
    offset = PCI_MAP_ROM_REG;
  else
    offset = PCI_MAP_REG_START + (index << 2);

  addr1 = pciReadLong(tag, offset);
  /*
   * Check if this is the second part of a 64 bit address.
   * XXX need to check how endianness affects 64 bit addresses.
   */
  if (index > 0 && index < 6) {
    addr2 = pciReadLong(tag, offset - 4);
    if (PCI_MAP_IS_MEM(addr2) && PCI_MAP_IS64BITMEM(addr2))
      return 0;
  }

  if (destructive) {
    pciWriteLong(tag, offset, 0xffffffff);
    mask1 = pciReadLong(tag, offset);
    pciWriteLong(tag, offset, addr1);
  } else {
    mask1 = addr1;
  }

  /* Check if this is the first part of a 64 bit address. */
  if (index < 5 && PCI_MAP_IS_MEM(mask1) && PCI_MAP_IS64BITMEM(mask1)) {
    if (PCIGETMEMORY(mask1) == 0) {
      addr2 = pciReadLong(tag, offset + 4);
      if (destructive) {
	pciWriteLong(tag, offset + 4, 0xffffffff);
	mask2 = pciReadLong(tag, offset + 4);
	pciWriteLong(tag, offset + 4, addr2);
      } else {
	mask2 = addr2;
      }
      if (mask2 == 0)
	return 0;
      bits = 32;
      while ((mask2 & 1) == 0) {
	bits++;
	mask2 >>= 1;
      }
      if (bits > 32)
	  return bits;
    }
  }
  if (index < 6)
    if (PCI_MAP_IS_MEM(mask1))
      mask1 = PCIGETMEMORY(mask1);
    else
      mask1 = PCIGETIO(mask1);
  else
    mask1 = PCIGETROM(mask1);
  if (mask1 == 0)
    return 0;
  bits = 0;
  while ((mask1 & 1) == 0) {
    bits++;
    mask1 >>= 1;
  }
  /* I/O maps can be no larger than 8 bits */

  if ((index < 6) && PCI_MAP_IS_IO(addr1) && bits > 8)
    bits = 8;
  /* ROM maps can be no larger than 24 bits */
  if (index == 6 && bits > 24)
    bits = 24;
  return bits;
}

_X_EXPORT PCITAG
pciTag(int busnum, int devnum, int funcnum)
{
	return(PCI_MAKE_TAG(busnum,devnum,funcnum));
}

_X_EXPORT PCITAG
pciDomTag(int domnum, int busnum, int devnum, int funcnum)
{
	return(PCI_MAKE_TAG(PCI_MAKE_BUS(domnum,busnum),devnum,funcnum));
}

#if defined(PCI_MFDEV_SUPPORT)

Bool
pciMfDev(int busnum, int devnum)
{
    PCITAG tag0, tag1;
    CARD32 id0, id1, val;

    /* Detect a multi-function device that complies to the PCI 2.0 spec */

    tag0 = PCI_MAKE_TAG(busnum, devnum, 0);
    id0  = pciReadLong(tag0, PCI_ID_REG);
    if ((CARD16)(id0 + 1) <= (CARD16)1UL)
	return FALSE;

    val = pciReadLong(tag0, PCI_HEADER_MISC) & 0x00ff0000;
    if ((val != 0x00ff0000) && (val & PCI_HEADER_MULTIFUNCTION))
	return TRUE;

    /*
     * Now, to find non-compliant devices...
     * If there is a valid ID for function 1 and the ID for func 0 and 1
     * are different, or the base0 values of func 0 and 1 are differend,
     * then assume there is a multi-function device.
     */
    tag1 = PCI_MAKE_TAG(busnum, devnum, 1);
    id1  = pciReadLong(tag1, PCI_ID_REG);
    if ((CARD16)(id1 + 1) <= (CARD16)1UL)
	return FALSE;

    /* Vendor IDs should match */
    if ((id0 ^ id1) & 0x0000ffff)
	return FALSE;

    if ((id0 != id1) ||
	/* Note the following test is valid for header types 0, 1 and 2 */
	(pciReadLong(tag0, PCI_MAP_REG_START) !=
	 pciReadLong(tag1, PCI_MAP_REG_START)))
	return TRUE;

    return FALSE;
}

#endif

/*
 * Generic find/read/write functions
 */
PCITAG
pciGenFindNext(void)
{
    CARD32 devid, tmp;
    int sec_bus, pri_bus;
    static int previousBus = 0;
    Bool speculativeProbe = FALSE;
    unsigned char base_class, sub_class;

#ifdef DEBUGPCI
    ErrorF("pciGenFindNext\n");
#endif

    for (;;) {

#ifdef DEBUGPCI
	ErrorF("pciGenFindNext: pciBusNum %d\n", pciBusNum);
#endif
	if (pciBusNum == -1) {
	    /*
	     * Start at top of the order
	     */
	    if (pciNumBuses <= 0)
		return(PCI_NOT_FOUND);

	    /* Skip ahead to the first bus defined by pciInit() */
	    for (pciBusNum = 0;  !pciBusInfo[pciBusNum];  ++pciBusNum);
	    pciFuncNum = 0;
	    pciDevNum = 0;
	    previousBus = pciBusNum; /* make sure previousBus exists */
	} else {
#ifdef PCI_MFDEV_SUPPORT
#ifdef DEBUGPCI
	    ErrorF("pciGenFindNext: pciFuncNum %d\n", pciFuncNum);
#endif
	    /*
	     * Somewhere in middle of order.  Determine who's
	     * next up
	     */
	    if (pciFuncNum == 0) {
		/*
		 * Is current dev a multifunction device?
		 */
		if (!speculativeProbe && pciMfDev(pciBusNum, pciDevNum))
		    /* Probe for other functions */
		    pciFuncNum = 1;
		else
		    /*
		     * No more functions this device. Next
		     * device please
		     */
		    pciDevNum ++;
	    } else if (++pciFuncNum >= 8) {
		/* No more functions for this device. Next device please */
		pciFuncNum = 0;
		pciDevNum ++;
	    }
#else
	    pciDevNum ++;
#endif
	    if (pciDevNum >= 32 ||
		!pciBusInfo[pciBusNum] ||
		pciDevNum >= pciBusInfo[pciBusNum]->numDevices) {
#ifdef DEBUGPCI
		ErrorF("pciGenFindNext: next bus\n");
#endif
		/*
		 * No more devices for this bus. Next bus please
		 */
		if (speculativeProbe) {
	NextSpeculativeBus:
		    xfree(pciBusInfo[pciBusNum]);
		    pciBusInfo[pciBusNum] = NULL;
		    speculativeProbe = FALSE;
		}

		if (++pciBusNum >= pciMaxBusNum) {
#ifdef DEBUGPCI
		    ErrorF("pciGenFindNext: out of buses\n");
#endif
		    /* No more buses.  All done for now */
		    return(PCI_NOT_FOUND);
		}

		pciDevNum = 0;
	    }
	}

#ifdef DEBUGPCI
	ErrorF("pciGenFindNext: pciBusInfo[%d] = 0x%lx\n", pciBusNum, pciBusInfo[pciBusNum]);
#endif
	if (!pciBusInfo[pciBusNum]) {
	    pciBusInfo[pciBusNum] = xnfalloc(sizeof(pciBusInfo_t));
	    *pciBusInfo[pciBusNum] = *pciBusInfo[previousBus];

	    speculativeProbe = TRUE;
	}

	/*
	 * At this point, pciBusNum, pciDevNum, and pciFuncNum have been
	 * advanced to the next device.  Compute the tag, and read the
	 * device/vendor ID field.
	 */
#ifdef DEBUGPCI
	ErrorF("pciGenFindNext: [%d, %d, %d]\n", pciBusNum, pciDevNum, pciFuncNum);
#endif
	pciDeviceTag = PCI_MAKE_TAG(pciBusNum, pciDevNum, pciFuncNum);
	inProbe = TRUE;
	devid = pciReadLong(pciDeviceTag, PCI_ID_REG);
	inProbe = FALSE;
#ifdef DEBUGPCI
	ErrorF("pciGenFindNext: pciDeviceTag = 0x%lx, devid = 0x%lx\n", pciDeviceTag, devid);
#endif
	if ((CARD16)(devid + 1U) <= (CARD16)1UL)
	    continue; /* Nobody home.  Next device please */

	/*
	 * Some devices mis-decode configuration cycles in such a way as to
	 * create phantom buses.
	 */
	if (speculativeProbe && (pciDevNum == 0) && (pciFuncNum == 0) &&
	    (PCI_BUS_NO_DOMAIN(pciBusNum) > 0)) {
	    for (;;) {
	        if (++pciDevNum >= pciBusInfo[pciBusNum]->numDevices)
		    goto NextSpeculativeBus;
		if (devid !=
		    pciReadLong(PCI_MAKE_TAG(pciBusNum, pciDevNum, 0),
			        PCI_ID_REG))
		    break;
	    }

	    pciDevNum = 0;
	}

	if (pciNumBuses <= pciBusNum)
	    pciNumBuses = pciBusNum + 1;

	speculativeProbe = FALSE;
	previousBus = pciBusNum;

#ifdef PCI_BRIDGE_SUPPORT
	/*
	 * Before checking for a specific devid, look for enabled
	 * PCI to PCI bridge devices.  If one is found, create and
	 * initialize a bus info record (if one does not already exist).
	 */
	tmp = pciReadLong(pciDeviceTag, PCI_CLASS_REG);
	base_class = PCI_CLASS_EXTRACT(tmp);
	sub_class = PCI_SUBCLASS_EXTRACT(tmp);
	if ((base_class == PCI_CLASS_BRIDGE) &&
	    ((sub_class == PCI_SUBCLASS_BRIDGE_PCI) ||
	     (sub_class == PCI_SUBCLASS_BRIDGE_CARDBUS))) {
	    tmp = pciReadLong(pciDeviceTag, PCI_PCI_BRIDGE_BUS_REG);
	    sec_bus = PCI_SECONDARY_BUS_EXTRACT(tmp, pciDeviceTag);
	    pri_bus = PCI_PRIMARY_BUS_EXTRACT(tmp, pciDeviceTag);
#ifdef DEBUGPCI
	    ErrorF("pciGenFindNext: pri_bus %d sec_bus %d\n",
		   pri_bus, sec_bus);
#endif
	    if (pciBusNum != pri_bus) {
		/* Some bridges do not implement the primary bus register */
		if ((PCI_BUS_NO_DOMAIN(pri_bus) != 0) ||
		    (sub_class != PCI_SUBCLASS_BRIDGE_CARDBUS))
		    xf86Msg(X_WARNING,
			    "pciGenFindNext:  primary bus mismatch on PCI"
			    " bridge 0x%08lx (0x%02x, 0x%02x)\n",
			    pciDeviceTag, pciBusNum, pri_bus);
		pri_bus = pciBusNum;
	    }
	    if ((pri_bus < sec_bus) && (sec_bus < pciMaxBusNum) &&
		pciBusInfo[pri_bus]) {
		/*
		 * Found a secondary PCI bus
		 */
		if (!pciBusInfo[sec_bus]) {
		    pciBusInfo[sec_bus] = xnfalloc(sizeof(pciBusInfo_t));

		    /* Copy parents settings... */
		    *pciBusInfo[sec_bus] = *pciBusInfo[pri_bus];
		}

		/* ...but not everything same as parent */
		pciBusInfo[sec_bus]->primary_bus = pri_bus;
		pciBusInfo[sec_bus]->secondary = TRUE;
		pciBusInfo[sec_bus]->numDevices = 32;

		if (pciNumBuses <= sec_bus)
		    pciNumBuses = sec_bus + 1;
	    }
	}
#endif

	/*
	 * Does this device match the requested devid after
	 * applying mask?
	 */
#ifdef DEBUGPCI
	ErrorF("pciGenFindNext: pciDevidMask = 0x%lx, pciDevid = 0x%lx\n", pciDevidMask, pciDevid);
#endif
	if ((devid & pciDevidMask) == pciDevid)
	    /* Yes - Return it.  Otherwise, next device */
	    return(pciDeviceTag); /* got a match */

    } /* for */
    /*NOTREACHED*/
}

PCITAG
pciGenFindFirst(void)
{
  /* Reset PCI bus number to start from top */
  pciBusNum = -1;

  return pciGenFindNext();
}

CARD32
pciByteSwap(CARD32 u)
{
#if X_BYTE_ORDER == X_BIG_ENDIAN

  return lswapl(u);

#else /* !BIG_ENDIAN */

  return(u);

#endif
}

ADDRESS
pciAddrNOOP(PCITAG tag, PciAddrType type, ADDRESS addr)
{
	return(addr);
}

_X_EXPORT pciConfigPtr *
xf86scanpci(int flags)
{
    pciConfigPtr devp;
    pciBusInfo_t *busp;
    int          idx = 0, i;
    PCITAG       tag;
    static Bool  done = FALSE;

    /*
     * if we haven't found PCI devices checking for pci_devp may
     * result in an endless recursion if platform/OS specific PCI
     * bus probing code calls this function from with in it.
     */
    if (done || pci_devp[0])
	return pci_devp;

    done = TRUE;

    pciInit();

#ifdef XF86SCANPCI_WRAPPER
    XF86SCANPCI_WRAPPER(SCANPCI_INIT);
#endif

    tag = pciFindFirst(0,0);  /* 0 mask means match any valid device */
    /* Check if no devices, return now */
    if (tag == PCI_NOT_FOUND) {
#ifdef XF86SCANPCI_WRAPPER
	XF86SCANPCI_WRAPPER(SCANPCI_TERM);
#endif
	return NULL;
    }

#ifdef DEBUGPCI
    ErrorF("xf86scanpci: tag = 0x%lx\n", tag);
#endif
#ifndef OLD_FORMAT
    xf86MsgVerb(X_INFO, 2, "PCI: PCI scan (all values are in hex)\n");
#endif

    while (idx < xf86MaxPciDevs && tag != PCI_NOT_FOUND) {
	devp = xcalloc(1, sizeof(pciDevice));
	if (!devp) {
	    xf86Msg(X_ERROR,
		"xf86scanpci: Out of memory after %d devices!!\n", idx);
	    return (pciConfigPtr *)NULL;
	}

	/* Identify pci device by bus, dev, func, and tag */
	devp->tag = tag;
	devp->busnum = PCI_BUS_FROM_TAG(tag);
	devp->devnum = PCI_DEV_FROM_TAG(tag);
	devp->funcnum = PCI_FUNC_FROM_TAG(tag);

	/* Read config space for this device */
	for (i = 0; i < 17; i++)  /* PCI hdr plus 1st dev spec dword */
	    devp->cfgspc.dwords[i] = pciReadLong(tag, i * sizeof(CARD32));

	/* Some broken devices don't implement this field... */
	if (devp->pci_header_type == 0xff)
	    devp->pci_header_type = 0;

	switch (devp->pci_header_type & 0x7f) {
	case 0:
	    /* Get base address sizes for type 0 headers */
	    for (i = 0; i < 7; i++)
		devp->basesize[i] =
		    pciGetBaseSize(tag, i, FALSE, &devp->minBasesize);
	    break;

	case 1:
	case 2:
	    /* Allow master aborts to complete normally on secondary buses */
	    if (!(devp->pci_bridge_control & PCI_PCI_BRIDGE_MASTER_ABORT_EN))
		break;
	    pciWriteByte(tag, PCI_PCI_BRIDGE_CONTROL_REG,
		devp->pci_bridge_control &
		     ~(PCI_PCI_BRIDGE_MASTER_ABORT_EN |
		       PCI_PCI_BRIDGE_SECONDARY_RESET));
	    break;

	default:
	    break;
	}

#ifdef OLD_FORMAT
	xf86MsgVerb(X_INFO, 2, "PCI: BusID 0x%.2x,0x%02x,0x%1x "
		    "ID 0x%04x,0x%04x Rev 0x%02x Class 0x%02x,0x%02x\n",
		    devp->busnum, devp->devnum, devp->funcnum,
		    devp->pci_vendor, devp->pci_device, devp->pci_rev_id,
		    devp->pci_base_class, devp->pci_sub_class);
#else
	xf86MsgVerb(X_INFO, 2, "PCI: %.2x:%02x:%1x: chip %04x,%04x"
		    " card %04x,%04x rev %02x class %02x,%02x,%02x hdr %02x\n",
		    devp->busnum, devp->devnum, devp->funcnum,
		    devp->pci_vendor, devp->pci_device,
		    devp->pci_subsys_vendor, devp->pci_subsys_card,
		    devp->pci_rev_id, devp->pci_base_class,
		    devp->pci_sub_class, devp->pci_prog_if,
		    devp->pci_header_type);
#endif

	pci_devp[idx++] = devp;
        if (idx == xf86MaxPciDevs)
            break;
	tag = pciFindNext();

#ifdef DEBUGPCI
	ErrorF("xf86scanpci: tag = pciFindNext = 0x%lx\n", tag);
#endif
    }

    /* Restore modified data (in reverse order), and link buses */
    while (--idx >= 0) {
	devp = pci_devp[idx];
	switch (devp->pci_header_type & 0x7f) {
	case 0:
	    if ((devp->pci_base_class != PCI_CLASS_BRIDGE) ||
		(devp->pci_sub_class != PCI_SUBCLASS_BRIDGE_HOST))
		break;
	    pciBusInfo[devp->busnum]->bridge = devp;
	    pciBusInfo[devp->busnum]->primary_bus = devp->busnum;
	    break;

	case 1:
	case 2:
	    i = PCI_SECONDARY_BUS_EXTRACT(devp->pci_pp_bus_register, devp->tag);
	    if (i > devp->busnum) {
		if (pciBusInfo[i]) {
		    pciBusInfo[i]->bridge = devp;
		    /*
                     * The back link needs to be set here, and is unlikely to
		     * change.
		     */
		    devp->businfo = pciBusInfo[i];
		}
#ifdef ARCH_PCI_PCI_BRIDGE
		ARCH_PCI_PCI_BRIDGE(devp);
#endif
	    }
	    if (!(devp->pci_bridge_control & PCI_PCI_BRIDGE_MASTER_ABORT_EN))
		break;
	    pciWriteByte(devp->tag, PCI_PCI_BRIDGE_CONTROL_REG,
		devp->pci_bridge_control & ~PCI_PCI_BRIDGE_SECONDARY_RESET);
	    break;

	default:
	    break;
	}
    }

#ifdef XF86SCANPCI_WRAPPER
    XF86SCANPCI_WRAPPER(SCANPCI_TERM);
#endif

    /*
     * Lastly, link bridges to their secondary bus, after the architecture has
     * had a chance to modify these assignments.
     */
    for (idx = 0;  idx < pciNumBuses;  idx++) {
	if (!(busp = pciBusInfo[idx]) || !(devp = busp->bridge))
	    continue;
	devp->businfo = busp;
    }

#ifndef OLD_FORMAT
    xf86MsgVerb(X_INFO, 2, "PCI: End of PCI scan\n");
#endif

    return pci_devp;
}

pciConfigPtr
xf86GetPciConfigFromTag(PCITAG Tag)
{
    pciConfigPtr pDev;
    int i = 0;

    for (i = 0 ; (pDev = pci_devp[i]) && i <= MAX_PCI_DEVICES; i++) {
	if (Tag == pDev->tag)
	    return pDev;
    }

    return NULL;	/* Bad data */
}

CARD32
pciCheckForBrokenBase(PCITAG Tag,int basereg)
{
    pciWriteLong(Tag, PCI_MAP_REG_START + (basereg << 2), 0xffffffff);
    return pciReadLong(Tag, PCI_MAP_REG_START + (basereg << 2));
}

#if defined(INCLUDE_XF86_MAP_PCI_MEM)

_X_EXPORT pointer
xf86MapPciMem(int ScreenNum, int Flags, PCITAG Tag, ADDRESS Base,
		unsigned long Size)
{
	ADDRESS hostbase = pciBusAddrToHostAddr(Tag, PCI_MEM,Base);
	pointer base;
	CARD32 save = 0;

	/*
	 * If there are possible read side-effects, disable memory while
	 * doing the mapping.
	 */
	if (Flags & VIDMEM_READSIDEEFFECT) {
		save = pciReadLong(Tag, PCI_CMD_STAT_REG);
		pciWriteLong(Tag, PCI_CMD_STAT_REG,
			     save & ~PCI_CMD_MEM_ENABLE);
	}
	base = xf86MapDomainMemory(ScreenNum, Flags, Tag, hostbase, Size);
	if (!base)	{
		FatalError("xf86MapPciMem: Could not mmap PCI memory "
			   "[base=0x%lx,hostbase=0x%lx,size=%lx] (%s)\n",
			   Base, hostbase, Size, strerror(errno));
	}
	/*
	 * If read side-effects, do whatever might be needed to prevent
	 * unintended reads, then restore PCI_CMD_STAT_REG.
	 */
	if (Flags & VIDMEM_READSIDEEFFECT) {
		xf86MapReadSideEffects(ScreenNum, Flags, base, Size);
		pciWriteLong(Tag, PCI_CMD_STAT_REG, save);
	}
	return((pointer)base);
}

static int
handlePciBIOS( PCITAG Tag, int basereg, unsigned char * buf, int len )
{
    CARD32 romsave = 0;
    int i;
    romBaseSource b_reg;
    ADDRESS hostbase;
    CARD8 tmp[64];
    int ret = 0;

    romsave = pciReadLong(Tag, PCI_MAP_ROM_REG);

    for (i = ROM_BASE_PRESET; i <= ROM_BASE_FIND; i++) {
	memType savebase = 0, newbase, romaddr;

        switch (i) {
        case ROM_BASE_PRESET:
	    /* Does the driver have a preference? */
	    if (basereg > ROM_BASE_PRESET && basereg <= ROM_BASE_FIND)
		b_reg = basereg;
	    else
		b_reg = ++i;
 	    break;
         case ROM_BASE_FIND:
 	    /*
 	     * If we have something that looks like a valid address
 	     * in romsave, it's probably not going to help to try
 	     * to guess a new address and reprogram it.
 	     */
 	    if (PCIGETROM(romsave)) {
 		pciWriteLong(Tag, PCI_MAP_ROM_REG, PCI_MAP_ROM_ADDRESS_MASK);
 		if (romsave != pciReadLong(Tag, PCI_MAP_ROM_REG)) {
 		    pciWriteLong(Tag, PCI_MAP_ROM_REG, romsave);
 	            continue;
 		}
 	    }
 	default:
	    b_reg = i;
	}

	if (!(newbase = getValidBIOSBase(Tag, b_reg)))
	    continue;  /* no valid address found */

	romaddr = PCIGETROM(newbase);

	/* if we use a mem base save it and move it out of the way */
	if (b_reg >= 0 && b_reg <= 5) {
	    memType emptybase;
	    savebase = pciReadLong(Tag, PCI_MAP_REG_START+(b_reg<<2));
	    xf86MsgVerb(X_INFO,5,"xf86ReadPciBios: modifying membase[%i]"
			" for device %i:%i:%i\n", basereg,
			(int)PCI_BUS_FROM_TAG(Tag), (int)PCI_DEV_FROM_TAG(Tag),
			(int)PCI_FUNC_FROM_TAG(Tag));
	    if (!(emptybase = getEmptyPciRange(Tag,b_reg))) {
		xf86Msg(X_ERROR,"Cannot find empty range to map base to\n");
		return 0;
	    }
	    pciWriteLong(Tag, PCI_MAP_REG_START + (b_reg << 2),
			 emptybase);
	}
	/* Set ROM base address and enable ROM address decoding */
	pciWriteLong(Tag, PCI_MAP_ROM_REG, romaddr
		     | PCI_MAP_ROM_DECODE_ENABLE);

	hostbase = pciBusAddrToHostAddr(Tag, PCI_MEM, PCIGETROM(romaddr));

	if ((xf86ReadDomainMemory(Tag, hostbase, sizeof(tmp), tmp) !=
	     sizeof(tmp)) ||
	    (tmp[0] != 0x55) || (tmp[1] != 0xaa) || !tmp[2] ) {
	  /* Restore the base registers if they were changed. */
	    pciWriteLong(Tag, PCI_MAP_ROM_REG, romsave);
	    if (savebase) pciWriteLong(Tag, PCI_MAP_REG_START + (b_reg << 2),
				       (CARD32) savebase);

	    /* No BIOS found: try another address */
	    continue;
	}

	ret = readPciBios( Tag, tmp, hostbase, buf, len, PCI_BIOS_PC );

	/* Restore the base register if it was changed. */
	if (savebase) pciWriteLong(Tag, PCI_MAP_REG_START + (b_reg << 2),
				   (CARD32) savebase);
	/* Restore ROM address decoding */
	pciWriteLong(Tag, PCI_MAP_ROM_REG, romsave);

	return ret;
    }
    /* Restore ROM address decoding */
    pciWriteLong(Tag, PCI_MAP_ROM_REG, romsave);
    return 0;
}


static int
readPciBios(PCITAG Tag, CARD8* tmp, ADDRESS hostbase, unsigned char * buf,
	    int len, PciBiosType bios_type )
{
    unsigned int image_length = 0;
    int ret;

  /* We found a PCI BIOS Image. Now we look for the correct type */
  while ((tmp[0] == 0x55) && (tmp[1] == 0xAA)) {
    unsigned short data_off = tmp[0x18] | (tmp[0x19] << 8);
    unsigned char data[0x18];
    unsigned char type;

    if ((xf86ReadDomainMemory(Tag, hostbase + data_off, sizeof(data), data)
	 != sizeof(data)) ||
	(data[0] != 'P')  ||
	(data[1] != 'C')  ||
	(data[2] != 'I')  ||
	(data[3] != 'R'))
      break;

    type = data[0x14];
#ifdef PRINT_PCI
    ErrorF("data segment in BIOS: 0x%x, type: 0x%x\n", data_off, type);
#endif
    if (type != bios_type) {	/* not correct image: find next one */
      const unsigned char indicator = data[0x15];
      unsigned int i_length;

      if (indicator & 0x80)	/* last image */
	break;

      i_length = (data[0x10] | (data[0x11] << 8)) << 9;

#ifdef PRINT_PCI
      ErrorF( "data image length: 0x%x, ind: 0x%x\n", i_length, indicator );
#endif
      hostbase += i_length;
      if (xf86ReadDomainMemory(Tag, hostbase, sizeof(tmp), tmp)
	  != sizeof(tmp))
	break;
      continue;
    }
    /* OK, we have a PCI BIOS Image of the correct type */

    if ( bios_type == PCI_BIOS_PC )
      image_length = tmp[2] << 9;
    else
      image_length = (data[0x10] | (data[0x11] << 8)) << 9;
#ifdef PRINT_PCI
    ErrorF("BIOS length: 0x%x\n", image_length);
#endif
    break;
  }

  ret = 0;
  if (image_length) {

    /* If no length is given return the full length. Beware: Area pointed to
     * by Buf must be large enough!
     */
    if (len == 0) {
      len = image_length;
    }
    else if ( len > image_length ) {
      len = image_length;
      xf86MsgVerb( X_INFO, 3, "Truncating PCI BIOS Length to %i\n",
		   len );
    }

    /* Read BIOS */
    ret = xf86ReadDomainMemory( Tag, hostbase, len, buf );
  }

  return ret;
}


static int
HandlePciBios(PCITAG Tag, int basereg, unsigned char * buf, int len)
{
  int n, num;
  CARD32 Acc1, Acc2;
  PCITAG *pTag;
  int i;

  /* fall back to the old code if the OS code fails */
  if (pciOSHandleBIOS) {
  	n = pciOSHandleBIOS(Tag, basereg, buf, len);
  	if (n)
      		return n;
  }

  n = handlePciBIOS( Tag, basereg, buf, len );
  if (n)
      return n;

  num = pciTestMultiDeviceCard(PCI_BUS_FROM_TAG(Tag),
			       PCI_DEV_FROM_TAG(Tag),
			       PCI_FUNC_FROM_TAG(Tag),&pTag);

  if (!num) return 0;

#define PCI_ENA (PCI_CMD_MEM_ENABLE | PCI_CMD_IO_ENABLE)
  Acc1 = pciReadLong(Tag, PCI_CMD_STAT_REG);
  pciWriteLong(Tag, PCI_CMD_STAT_REG, (Acc1 & ~PCI_ENA));

  for (i = 0; i < num; i++) {
    Acc2 = pciReadLong(pTag[i], PCI_CMD_STAT_REG);
    pciWriteLong(pTag[i], PCI_CMD_STAT_REG, (Acc2 | PCI_ENA));

    n = handlePciBIOS( pTag[i], 0, buf, len );

    pciWriteLong(pTag[i], PCI_CMD_STAT_REG, Acc2);
    if (n)
      break;
  }
  pciWriteLong(Tag, PCI_CMD_STAT_REG, Acc1);
  return n;
}

_X_EXPORT int
xf86ReadPciBIOS(unsigned long Offset, PCITAG Tag, int basereg,
		unsigned char *Buf, int Len)
{
    return HandlePciBios(Tag, basereg, Buf, Len);
}

#endif /* INCLUDE_XF86_MAP_PCI_MEM */

#ifdef INCLUDE_XF86_NO_DOMAIN

_X_EXPORT int
xf86GetPciDomain(PCITAG Tag)
{
    return 0;
}

_X_EXPORT pointer
xf86MapDomainMemory(int ScreenNum, int Flags, PCITAG Tag,
		    ADDRESS Base, unsigned long Size)
{
    return xf86MapVidMem(ScreenNum, Flags, Base, Size);
}

_X_EXPORT IOADDRESS
xf86MapDomainIO(int ScreenNum, int Flags, PCITAG Tag,
		IOADDRESS Base, unsigned long Size)
{
    return Base;
}

_X_EXPORT int
xf86ReadDomainMemory(PCITAG Tag, ADDRESS Base, int Len, unsigned char *Buf)
{
    int ret, length, rlength;

    /* Read in 64kB chunks */
    ret = 0;
    while ((length = Len) > 0) {
	if (length > 0x010000) length = 0x010000;
	rlength = xf86ReadBIOS(Base, 0, Buf, length);
	if (rlength < 0) {
	    ret = rlength;
	    break;
	}
	ret += rlength;
	if (rlength < length) break;
	Base += rlength;
	Buf += rlength;
	Len -= rlength;
    }

    return ret;
}

#endif /* INCLUDE_XF86_NO_DOMAIN */
