/* $Xorg: Pointer.h,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/
/* $XFree86: xc/programs/Xserver/hw/xnest/Pointer.h,v 1.2 2003/11/16 05:05:20 dawes Exp $ */

#ifndef XNESTPOINTER_H
#define XNESTPOINTER_H

#define MAXBUTTONS 256

#define XNEST_POINTER_EVENT_MASK \
        (ButtonPressMask | ButtonReleaseMask | PointerMotionMask | \
	 EnterWindowMask | LeaveWindowMask)

void xnestChangePointerControl(DeviceIntPtr pDev, PtrCtrl *ctrl);
int xnestPointerProc(DeviceIntPtr pDev, int onoff);

#endif /* XNESTPOINTER_H */
