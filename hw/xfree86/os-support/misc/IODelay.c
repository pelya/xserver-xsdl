
/* $XConsortium: IODelay.c /main/1 1996/05/07 17:13:43 kaleb $ */
/*******************************************************************************
  Stub for Alpha Linux
*******************************************************************************/

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/misc/IODelay.c,v 1.3 2000/08/04 16:13:41 eich Exp $ */
 
#include "X.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

/* 
 *   All we really need is a delay of about 40ns for I/O recovery for just
 *   about any occasion, but we'll be more conservative here:  On a
 *   100-MHz CPU, produce at least a delay of 1,000ns.
 */ 
void
xf86IODelay()
{
	xf86UDelay(1);
}

