/* $Xorg: xf86_OSproc.h,v 1.3 2000/08/17 19:51:20 cpqbld Exp $ */
/*
 * Copyright 1990, 1991 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1992 by David Dawes <dawes@XFree86.org>
 * Copyright 1992 by Jim Tsillas <jtsilla@damon.ccs.northeastern.edu>
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1992 by Robert Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 * Copyright 1992 by Orest Zborowski <obz@eskimo.com>
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
 * Copyright 1994, 1996 by Holger Veit <Holger.Veit@gmd.de>
 * Copyright 1994, 1995 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE 
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY 
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER 
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING 
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_OSproc.h,v 3.0.2.1 1998/02/07 14:27:24 dawes Exp $ */

#ifndef _XF86_OSPROC_H
#define _XF86_OSPROC_H

/*
 * The actual prototypes have been pulled into this seperate file so
 * that they can can be used without pulling in all of the OS specific
 * stuff like sys/stat.h, etc. This casues problem for loadable modules.
 */ 

/* The Region arg to xf86[Un]Map* */
#define NUM_REGIONS 4
#define VGA_REGION 0
#define LINEAR_REGION 1
#define EXTENDED_REGION 2
#define MMIO_REGION 3

#ifndef NO_OSLIB_PROTOTYPES
/*
 * This is to prevent re-entrancy to FatalError() when aborting.
 * Anything that can be called as a result of AbortDDX() should use this
 * instead of FatalError(). (xf86Exiting gets set to TRUE the first time
 * AbortDDX() is called.)
 */

extern Bool xf86Exiting;

#define xf86FatalError(a, b) \
	if (xf86Exiting) { \
		ErrorF(a, b); \
		return; \
	} else FatalError(a, b)

/***************************************************************************/
/* Prototypes                                                              */
/***************************************************************************/

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

/* xf86_Util.c */
extern int StrCaseCmp(
#if NeedFunctionPrototypes
	const char *,
	const char *
#endif
);

/* OS-support layer */
extern void xf86OpenConsole(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86CloseConsole(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86VTSwitchPending(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86VTSwitchAway(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86VTSwitchTo(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86LinearVidMem(
#if NeedFunctionPrototypes
	void
#endif
);
extern pointer xf86MapVidMem(
#if NeedFunctionPrototypes
	int,
	int,
	pointer,
	unsigned long
#endif
);
extern void xf86UnMapVidMem(
#if NeedFunctionPrototypes
	int,
	int,
	pointer,
	unsigned long
#endif
);
#if defined(__alpha__)
/* entry points for SPARSE memory access routines */
extern pointer xf86MapVidMemSparse(
#if NeedFunctionPrototypes
	int,
	int,
	pointer,
	unsigned long
#endif
);
extern void xf86UnMapVidMemSparse(
#if NeedFunctionPrototypes
	int,
	int,
	pointer,
	unsigned long
#endif
);
extern int xf86ReadSparse8(
#if NeedFunctionPrototypes
	pointer,
	unsigned long
#endif
);
extern int xf86ReadSparse16(
#if NeedFunctionPrototypes
	pointer,
	unsigned long
#endif
);
extern int xf86ReadSparse32(
#if NeedFunctionPrototypes
	pointer,
	unsigned long
#endif
);
extern void xf86WriteSparse8(
#if NeedFunctionPrototypes
	int,
	pointer,
	unsigned long
#endif
);
extern void xf86WriteSparse16(
#if NeedFunctionPrototypes
	int,
	pointer,
	unsigned long
#endif
);
extern void xf86WriteSparse32(
#if NeedFunctionPrototypes
	int,
	pointer,
	unsigned long
#endif
);
#endif /* __alpha__ */
extern void xf86MapDisplay(
#if NeedFunctionPrototypes
	int,
	int
#endif
);
extern void xf86UnMapDisplay(
#if NeedFunctionPrototypes
	int,
	int
#endif
);
extern int xf86ReadBIOS(
#if NeedFunctionPrototypes
	unsigned long,
	unsigned long,
	unsigned char *,
	int
#endif
);
extern void xf86ClearIOPortList(
#if NeedFunctionPrototypes
	int
#endif
);
extern void xf86AddIOPorts(
#if NeedFunctionPrototypes
	int,
	int,
	unsigned *
#endif
);
void xf86EnableIOPorts(
#if NeedFunctionPrototypes
	int
#endif
);
void xf86DisableIOPorts(
#if NeedFunctionPrototypes
	int
#endif
);
void xf86DisableIOPrivs(
#if NeedFunctionPrototypes
	void
#endif
);
extern Bool xf86DisableInterrupts(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86EnableInterrupts(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86ProcessArgument(
#if NeedFunctionPrototypes
	int,
	char **,
	int
#endif
);
extern void xf86UseMsg(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86SoundKbdBell(
#if NeedFunctionPrototypes
	int,
	int,
	int
#endif
);
extern void xf86SetKbdLeds(
#if NeedFunctionPrototypes
	int
#endif
);
extern int xf86GetKbdLeds(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86SetKbdRepeat(
#if NeedFunctionPrototypes
	char
#endif
);
extern void xf86KbdInit(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86KbdOn(
#if NeedFunctionPrototypes
	void
#endif
);
extern int xf86KbdOff(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86KbdEvents(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86SetMouseSpeed(
#if NeedFunctionPrototypes
        MouseDevPtr,
	int,
	int,
	unsigned
#endif
);
extern void xf86MouseInit(
#if NeedFunctionPrototypes
        MouseDevPtr
#endif
);
extern int xf86MouseOn(
#if NeedFunctionPrototypes
        MouseDevPtr
#endif
);
extern int xf86MouseOff(
#if NeedFunctionPrototypes
        MouseDevPtr,
	Bool
#endif
);
extern void xf86MouseEvents(
#if NeedFunctionPrototypes
        MouseDevPtr
#endif
);
extern int xf86FlushInput(
#if NeedFunctionPrototypes
	int
#endif
);
extern int  xf86XqueKbdProc(
#if NeedFunctionPrototypes
	DeviceIntPtr,
	int
#endif
);
extern int  xf86XqueMseProc(
#if NeedFunctionPrototypes
	DeviceIntPtr,
	int
#endif
);
extern void xf86XqueEvents(
#if NeedFunctionPrototypes
	void
#endif
);


/* These are privates */
extern void xf86InitPortLists(
#if NeedFunctionPrototypes
	unsigned **,
	int *,
	Bool *,
	Bool *,
	int
#endif
);
extern Bool xf86CheckPorts(
#if NeedFunctionPrototypes
	unsigned,
	unsigned **,
	int *,
	Bool *,
	int
#endif
);
extern int  xf86OsMouseProc(
#if NeedFunctionPrototypes
	DeviceIntPtr,
	int
#endif
);
extern void xf86OsMouseEvents(
#if NeedFunctionPrototypes
	void
#endif
);
extern void xf86OsMouseOption(
#if NeedFunctionPrototypes
	int,
	pointer /* gets cast to LexPtr later, saves include file hassles */
#endif
);

_XFUNCPROTOEND
#endif /* NO_OSLIB_PROTOTYPES */

#endif /* _XF86_OSPROC_H */
