/* $XFree86: xc/programs/Xserver/hw/xfree86/int10/xf86x86emu.c,v 1.13 2002/09/16 18:06:09 eich Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include <x86emu.h>
#include "xf86.h"
#include "xf86_ansic.h"
#include "compiler.h"
#include "xf86_OSproc.h"
#include "xf86Pci.h"
#include "xf86_libc.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#include "int10Defines.h"

#define M _X86EMU_env

static void
x86emu_do_int(int num)
{
    Int10Current->num = num;

    if (!int_handler(Int10Current)) {
	X86EMU_halt_sys();
    }
}

void
xf86ExecX86int10(xf86Int10InfoPtr pInt)
{
    int sig = setup_int(pInt);

    if (sig < 0)
	return;

    if (int_handler(pInt)) {
	X86EMU_exec();
    }

    finish_int(pInt, sig);
}

Bool
xf86Int10ExecSetup(xf86Int10InfoPtr pInt)
{
    int i;
    X86EMU_intrFuncs intFuncs[256];
    X86EMU_pioFuncs pioFuncs = {
	(&x_inb),
	(&x_inw),
	(&x_inl),
	(&x_outb),
	(&x_outw),
	(&x_outl)
    };

    X86EMU_memFuncs memFuncs = {
	(&Mem_rb),
	(&Mem_rw),
	(&Mem_rl),
	(&Mem_wb),
	(&Mem_ww),
	(&Mem_wl)
    };

    X86EMU_setupMemFuncs(&memFuncs);

    pInt->cpuRegs = &M;
    M.mem_base = 0;
    M.mem_size = 1024*1024 + 1024;
    X86EMU_setupPioFuncs(&pioFuncs);

    for (i=0;i<256;i++)
	intFuncs[i] = x86emu_do_int;
    X86EMU_setupIntrFuncs(intFuncs);
    return TRUE;
}

void
printk(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    VErrorF(fmt, argptr);
    va_end(argptr);
}
