/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/helper_exec.c,v 1.24 2002/11/25 21:05:49 tsi Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 *
 *   Part of this is based on code taken from DOSEMU
 *   (C) Copyright 1992, ..., 1999 the "DOSEMU-Development-Team"
 */

/*
 * To debug port accesses define PRINT_PORT.
 * Note! You also have to comment out ioperm()
 * in xf86EnableIO(). Otherwise we won't trap
 * on PIO.
 */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"
#define _INT10_PRIVATE
#include "int10Defines.h"
#include "xf86int10.h"

#if !defined (_PC) && !defined (_PC_PCI)
static int pciCfg1in(CARD16 addr, CARD32 *val);
static int pciCfg1out(CARD16 addr, CARD32 val);
#endif
#if defined (_PC)
static void SetResetBIOSVars(xf86Int10InfoPtr pInt, Bool set);
#endif

#define REG pInt

int
setup_int(xf86Int10InfoPtr pInt)
{
    if (pInt != Int10Current) {
	if (!MapCurrentInt10(pInt))
	    return -1;
	Int10Current = pInt;
    }
    X86_EAX = (CARD32) pInt->ax;
    X86_EBX = (CARD32) pInt->bx;
    X86_ECX = (CARD32) pInt->cx;
    X86_EDX = (CARD32) pInt->dx;
    X86_ESI = (CARD32) pInt->si;
    X86_EDI = (CARD32) pInt->di;
    X86_EBP = (CARD32) pInt->bp;
    X86_ESP = 0x1000; X86_SS = pInt->stackseg >> 4;
    X86_EIP = 0x0600; X86_CS = 0x0;	/* address of 'hlt' */
    X86_DS = 0x40;			/* standard pc ds */
    X86_ES = pInt->es;
    X86_FS = 0;
    X86_GS = 0;
    X86_EFLAGS = X86_IF_MASK | X86_IOPL_MASK;
#if defined (_PC)
    if (pInt->flags & SET_BIOS_SCRATCH)
	SetResetBIOSVars(pInt, TRUE);
#endif
    return xf86BlockSIGIO();
}

void
finish_int(xf86Int10InfoPtr pInt, int sig)
{
    xf86UnblockSIGIO(sig);
    pInt->ax = (CARD32) X86_EAX;
    pInt->bx = (CARD32) X86_EBX;
    pInt->cx = (CARD32) X86_ECX;
    pInt->dx = (CARD32) X86_EDX;
    pInt->si = (CARD32) X86_ESI;
    pInt->di = (CARD32) X86_EDI;
    pInt->es = (CARD16) X86_ES;
    pInt->bp = (CARD32) X86_EBP;
    pInt->flags = (CARD32) X86_FLAGS;
#if defined (_PC)
    if (pInt->flags & RESTORE_BIOS_SCRATCH)
	SetResetBIOSVars(pInt, FALSE);
#endif
}

/* general software interrupt handler */
CARD32
getIntVect(xf86Int10InfoPtr pInt,int num)
{
    return MEM_RW(pInt, num << 2) + (MEM_RW(pInt, (num << 2) + 2) << 4);
}

void
pushw(xf86Int10InfoPtr pInt, CARD16 val)
{
    X86_ESP -= 2;
    MEM_WW(pInt, ((CARD32) X86_SS << 4) + X86_SP, val);
}

int
run_bios_int(int num, xf86Int10InfoPtr pInt)
{
    CARD32 eflags;
#ifndef _PC
    /* check if bios vector is initialized */
    if (MEM_RW(pInt, (num << 2) + 2) == (SYS_BIOS >> 4)) { /* SYS_BIOS_SEG ?*/

	if (num == 21 && X86_AH == 0x4e) {
 	    xf86DrvMsg(pInt->scrnIndex, X_NOTICE,
		       "Failing Find-Matching-File on non-PC"
			" (int 21, func 4e)\n");
 	    X86_AX = 2;
 	    SET_FLAG(F_CF);
 	    return 1;
 	} else {
	    xf86DrvMsgVerb(pInt->scrnIndex, X_NOT_IMPLEMENTED, 2,
			   "Ignoring int 0x%02x call\n", num);
	    if (xf86GetVerbosity() > 3) {
		dump_registers(pInt);
		stack_trace(pInt);
	    }
	    return 1;
	}
    }
#endif
#ifdef PRINT_INT
    ErrorF("calling card BIOS at: ");
#endif
    eflags = X86_EFLAGS;
#if 0
    eflags = eflags | IF_MASK;
    X86_EFLAGS = X86_EFLAGS  & ~(VIF_MASK | TF_MASK | IF_MASK | NT_MASK);
#endif
    pushw(pInt, eflags);
    pushw(pInt, X86_CS);
    pushw(pInt, X86_IP);
    X86_CS = MEM_RW(pInt, (num << 2) + 2);
    X86_IP = MEM_RW(pInt,  num << 2);
#ifdef PRINT_INT
    ErrorF("0x%x:%lx\n", X86_CS, X86_EIP);
#endif
    return 1;
}

/* Debugging stuff */
void
dump_code(xf86Int10InfoPtr pInt)
{
    int i;
    CARD32 lina = SEG_ADR((CARD32), X86_CS, IP);

    xf86DrvMsgVerb(pInt->scrnIndex, X_INFO, 3, "code at 0x%8.8lx:\n", lina);
    for (i=0; i<0x10; i++)
	xf86ErrorFVerb(3, " %2.2x", MEM_RB(pInt, lina + i));
    xf86ErrorFVerb(3, "\n");
    for (; i<0x20; i++)
	xf86ErrorFVerb(3, " %2.2x", MEM_RB(pInt, lina + i));
    xf86ErrorFVerb(3, "\n");
}

void
dump_registers(xf86Int10InfoPtr pInt)
{
    xf86DrvMsgVerb(pInt->scrnIndex, X_INFO, 3,
	"EAX=0x%8.8x, EBX=0x%8.8x, ECX=0x%8.8x, EDX=0x%8.8x\n",
	X86_EAX, X86_EBX, X86_ECX, X86_EDX);
    xf86DrvMsgVerb(pInt->scrnIndex, X_INFO, 3,
	"ESP=0x%8.8x, EBP=0x%8.8x, ESI=0x%8.8x, EDI=0x%8.8x\n",
	X86_ESP, X86_EBP, X86_ESI, X86_EDI);
    xf86DrvMsgVerb(pInt->scrnIndex, X_INFO, 3,
	"CS=0x%4.4x, SS=0x%4.4x,"
	" DS=0x%4.4x, ES=0x%4.4x, FS=0x%4.4x, GS=0x%4.4x\n",
	X86_CS, X86_SS, X86_DS, X86_ES, X86_FS, X86_GS);
    xf86DrvMsgVerb(pInt->scrnIndex, X_INFO, 3,
	"EIP=0x%8.8x, EFLAGS=0x%8.8x\n", X86_EIP, X86_EFLAGS);
}

void
stack_trace(xf86Int10InfoPtr pInt)
{
    int i = 0;
    CARD32 stack = SEG_ADR((CARD32), X86_SS, SP);
    CARD32 tail  = (CARD32)((X86_SS << 4) + 0x1000);

    if (stack >= tail) return;

    xf86MsgVerb(X_INFO, 3, "stack at 0x%8.8lx:\n", stack);
    for (; stack < tail; stack++) {
	xf86ErrorFVerb(3, " %2.2x", MEM_RB(pInt, stack));
	i = (i + 1) % 0x10;
	if (!i)
	    xf86ErrorFVerb(3, "\n");
    }
    if (i)
	xf86ErrorFVerb(3, "\n");
}

int
port_rep_inb(xf86Int10InfoPtr pInt,
	     CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -1 : 1;
    CARD32 dst = base;
#ifdef PRINT_PORT
    ErrorF(" rep_insb(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	MEM_WB(pInt, dst, x_inb(port));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_inw(xf86Int10InfoPtr pInt,
	     CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -2 : 2;
    CARD32 dst = base;
#ifdef PRINT_PORT
    ErrorF(" rep_insw(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	MEM_WW(pInt, dst, x_inw(port));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_inl(xf86Int10InfoPtr pInt,
	     CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -4 : 4;
    CARD32 dst = base;
#ifdef PRINT_PORT
    ErrorF(" rep_insl(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	MEM_WL(pInt, dst, x_inl(port));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_outb(xf86Int10InfoPtr pInt,
	      CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -1 : 1;
    CARD32 dst = base;
#ifdef PRINT_PORT
    ErrorF(" rep_outb(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	x_outb(port, MEM_RB(pInt, dst));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_outw(xf86Int10InfoPtr pInt,
	      CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -2 : 2;
    CARD32 dst = base;
#ifdef PRINT_PORT
    ErrorF(" rep_outw(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	x_outw(port, MEM_RW(pInt, dst));
	dst += inc;
    }
    return dst - base;
}

int
port_rep_outl(xf86Int10InfoPtr pInt,
	      CARD16 port, CARD32 base, int d_f, CARD32 count)
{
    register int inc = d_f ? -4 : 4;
    CARD32 dst = base;
#ifdef PRINT_PORT
    ErrorF(" rep_outl(%#x) %d bytes at %p %s\n",
	     port, count, base, d_f ? "up" : "down");
#endif
    while (count--) {
	x_outl(port, MEM_RL(pInt, dst));
	dst += inc;
    }
    return dst - base;
}

CARD8
x_inb(CARD16 port)
{
    CARD8 val;

    if (port == 0x40) {
	Int10Current->inb40time++;
	val = (CARD8)(Int10Current->inb40time >>
		      ((Int10Current->inb40time & 1) << 3));
#ifdef PRINT_PORT
	ErrorF(" inb(%#x) = %2.2x\n", port, val);
#endif
#ifdef __NOT_YET__
    } else if (port < 0x0100) {		/* Don't interfere with mainboard */
	val = 0;
	xf86DrvMsgVerb(Int10Current->scrnIndex, X_NOT_IMPLEMENTED, 2,
	    "inb 0x%4.4x\n", port);
	if (xf86GetVerbosity() > 3) {
	    dump_registers(Int10Current);
	    stack_trace(Int10Current);
	}
#endif /* __NOT_YET__ */
    } else {
	val = inb(Int10Current->ioBase + port);
#ifdef PRINT_PORT
	ErrorF(" inb(%#x) = %2.2x\n", port, val);
#endif
    }
    return val;
}

CARD16
x_inw(CARD16 port)
{
    CARD16 val;

    if (port == 0x5c) {
	/*
	 * Emulate a PC98's timer.  Typical resolution is 3.26 usec.
	 * Approximate this by dividing by 3.
	 */
	long sec, usec;
	(void)getsecs(&sec, &usec);
	val = (CARD16)(usec / 3);
    } else {
	val = inw(Int10Current->ioBase + port);
    }
#ifdef PRINT_PORT
    ErrorF(" inw(%#x) = %4.4x\n", port, val);
#endif
    return val;
}

void
x_outb(CARD16 port, CARD8 val)
{
    if ((port == 0x43) && (val == 0)) {
	/*
	 * Emulate a PC's timer 0.  Such timers typically have a resolution of
	 * some .838 usec per tick, but this can only provide 1 usec per tick.
	 * (Not that this matters much, given inherent emulation delays.)  Use
	 * the bottom bit as a byte select.  See inb(0x40) above.
	 */
	long sec, usec;
	(void) getsecs(&sec, &usec);
	Int10Current->inb40time = (CARD16)(usec | 1);
#ifdef PRINT_PORT
	ErrorF(" outb(%#x, %2.2x)\n", port, val);
#endif
#ifdef __NOT_YET__
    } else if (port < 0x0100) {		/* Don't interfere with mainboard */
	xf86DrvMsgVerb(Int10Current->scrnIndex, X_NOT_IMPLEMENTED, 2,
	    "outb 0x%4.4x,0x%2.2x\n", port, val);
	if (xf86GetVerbosity() > 3) {
	    dump_registers(Int10Current);
	    stack_trace(Int10Current);
	}
#endif /* __NOT_YET__ */
    } else {
#ifdef PRINT_PORT
	ErrorF(" outb(%#x, %2.2x)\n", port, val);
#endif
	outb(Int10Current->ioBase + port, val);
    }
}

void
x_outw(CARD16 port, CARD16 val)
{
#ifdef PRINT_PORT
    ErrorF(" outw(%#x, %4.4x)\n", port, val);
#endif

    outw(Int10Current->ioBase + port, val);
}

CARD32
x_inl(CARD16 port)
{
    CARD32 val;

#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1in(port, &val))
#endif
    val = inl(Int10Current->ioBase + port);

#ifdef PRINT_PORT
    ErrorF(" inl(%#x) = %8.8x\n", port, val);
#endif
    return val;
}

void
x_outl(CARD16 port, CARD32 val)
{
#ifdef PRINT_PORT
    ErrorF(" outl(%#x, %8.8x)\n", port, val);
#endif

#if !defined(_PC) && !defined(_PC_PCI)
    if (!pciCfg1out(port, val))
#endif
    outl(Int10Current->ioBase + port, val);
}

CARD8
Mem_rb(CARD32 addr)
{
    return (*Int10Current->mem->rb)(Int10Current, addr);
}

CARD16
Mem_rw(CARD32 addr)
{
    return (*Int10Current->mem->rw)(Int10Current, addr);
}

CARD32
Mem_rl(CARD32 addr)
{
    return (*Int10Current->mem->rl)(Int10Current, addr);
}

void
Mem_wb(CARD32 addr, CARD8 val)
{
    (*Int10Current->mem->wb)(Int10Current, addr, val);
}

void
Mem_ww(CARD32 addr, CARD16 val)
{
    (*Int10Current->mem->ww)(Int10Current, addr, val);
}

void
Mem_wl(CARD32 addr, CARD32 val)
{
    (*Int10Current->mem->wl)(Int10Current, addr, val);
}

#if !defined(_PC) && !defined(_PC_PCI)
static CARD32 PciCfg1Addr = 0;

#define TAG(Cfg1Addr) (Cfg1Addr & 0xffff00)
#define OFFSET(Cfg1Addr) (Cfg1Addr & 0xff)

static int
pciCfg1in(CARD16 addr, CARD32 *val)
{
    if (addr == 0xCF8) {
	*val = PciCfg1Addr;
	return 1;
    }
    if (addr == 0xCFC) {
	*val = pciReadLong(TAG(PciCfg1Addr), OFFSET(PciCfg1Addr));
	return 1;
    }
    return 0;
}

static int
pciCfg1out(CARD16 addr, CARD32 val)
{
    if (addr == 0xCF8) {
	PciCfg1Addr = val;
	return 1;
    }
    if (addr == 0xCFC) {
	pciWriteLong(TAG(PciCfg1Addr), OFFSET(PciCfg1Addr),val);
	return 1;
    }
    return 0;
}
#endif

CARD8
bios_checksum(CARD8 *start, int size)
{
    CARD8 sum = 0;

    while (size-- > 0)
	sum += *start++;
    return sum;
}

/*
 * Lock/Unlock legacy VGA. Some Bioses try to be very clever and make
 * an attempt to detect a legacy ISA card. If they find one they might
 * act very strange: for example they might configure the card as a
 * monochrome card. This might cause some drivers to choke.
 * To avoid this we attempt legacy VGA by writing to all know VGA
 * disable registers before we call the BIOS initialization and
 * restore the original values afterwards. In beween we hold our
 * breath. To get to a (possibly exising) ISA card need to disable
 * our current PCI card.
 */
/*
 * This is just for booting: we just want to catch pure
 * legacy vga therefore we don't worry about mmio etc.
 * This stuff should really go into vgaHW.c. However then
 * the driver would have to load the vga-module prior to
 * doing int10.
 */
void
LockLegacyVGA(xf86Int10InfoPtr pInt, legacyVGAPtr vga)
{
    xf86SetCurrentAccess(FALSE, xf86Screens[pInt->scrnIndex]);
    vga->save_msr    = inb(pInt->ioBase + 0x03CC);
    vga->save_vse    = inb(pInt->ioBase + 0x03C3);
#ifndef __ia64__
    vga->save_46e8   = inb(pInt->ioBase + 0x46E8);
#endif
    vga->save_pos102 = inb(pInt->ioBase + 0x0102);
    outb(pInt->ioBase + 0x03C2, ~(CARD8)0x03 & vga->save_msr);
    outb(pInt->ioBase + 0x03C3, ~(CARD8)0x01 & vga->save_vse);
#ifndef __ia64__
    outb(pInt->ioBase + 0x46E8, ~(CARD8)0x08 & vga->save_46e8);
#endif
    outb(pInt->ioBase + 0x0102, ~(CARD8)0x01 & vga->save_pos102);
    xf86SetCurrentAccess(TRUE, xf86Screens[pInt->scrnIndex]);
}

void
UnlockLegacyVGA(xf86Int10InfoPtr pInt, legacyVGAPtr vga)
{
    xf86SetCurrentAccess(FALSE, xf86Screens[pInt->scrnIndex]);
    outb(pInt->ioBase + 0x0102, vga->save_pos102);
#ifndef __ia64__
    outb(pInt->ioBase + 0x46E8, vga->save_46e8);
#endif
    outb(pInt->ioBase + 0x03C3, vga->save_vse);
    outb(pInt->ioBase + 0x03C2, vga->save_msr);
    xf86SetCurrentAccess(TRUE, xf86Screens[pInt->scrnIndex]);
}

#if defined (_PC)
static void
SetResetBIOSVars(xf86Int10InfoPtr pInt, Bool set)
{
    int pagesize = getpagesize();
    unsigned char* base = xf86MapVidMem(pInt->scrnIndex,
					VIDMEM_MMIO, 0, pagesize);
    int i;

    if (set) {
	for (i = BIOS_SCRATCH_OFF; i < BIOS_SCRATCH_END; i++)
	    MEM_WW(pInt, i, *(base + i));
    } else {
	for (i = BIOS_SCRATCH_OFF; i < BIOS_SCRATCH_END; i++)
	    *(base + i) = MEM_RW(pInt, i);
    }
    
    xf86UnMapVidMem(pInt->scrnIndex,base,pagesize);
}

void
xf86Int10SaveRestoreBIOSVars(xf86Int10InfoPtr pInt, Bool save)
{
    int pagesize = getpagesize();
    unsigned char* base;
    int i;

    if (!xf86IsEntityPrimary(pInt->entityIndex)
	|| (!save && !pInt->BIOSScratch))
	return;
    
    base = xf86MapVidMem(pInt->scrnIndex, VIDMEM_MMIO, 0, pagesize);
    base += BIOS_SCRATCH_OFF;
    if (save) {
	if ((pInt->BIOSScratch
	     = xnfalloc(BIOS_SCRATCH_LEN)))
	    for (i = 0; i < BIOS_SCRATCH_LEN; i++)
		*(((char*)pInt->BIOSScratch + i)) = *(base + i);	
    } else {
	if (pInt->BIOSScratch) {
	    for (i = 0; i < BIOS_SCRATCH_LEN; i++)
		*(base + i) = *(pInt->BIOSScratch + i); 
	    xfree(pInt->BIOSScratch);
	    pInt->BIOSScratch = NULL;
	}
    }
    
    xf86UnMapVidMem(pInt->scrnIndex,base - BIOS_SCRATCH_OFF ,pagesize);
}
#endif
