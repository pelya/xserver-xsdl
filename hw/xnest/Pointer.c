/* $Xorg: Pointer.c,v 1.3 2000/08/17 19:53:28 cpqbld Exp $ */
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
#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "mipointer.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "Pointer.h"
#include "Args.h"

void xnestChangePointerControl(pDev, ctrl)
     DeviceIntPtr pDev;
     PtrCtrl *ctrl;
{
  XChangePointerControl(xnestDisplay, True, True, 
			ctrl->num, ctrl->den, ctrl->threshold); 
}

int xnestPointerProc(pDev, onoff, argc, argv)
     DevicePtr pDev;
     int onoff, argc;
     char *argv[];
{
  CARD8 map[MAXBUTTONS];
  int nmap;
  int i;

  switch (onoff)
    {
    case DEVICE_INIT: 
      nmap = XGetPointerMapping(xnestDisplay, map, MAXBUTTONS);
      for (i = 0; i <= nmap; i++)
	map[i] = i; /* buttons are already mapped */
      InitPointerDeviceStruct(pDev, map, nmap,
			      miPointerGetMotionEvents,
			      xnestChangePointerControl,
			      miPointerGetMotionBufferSize());
      break;
    case DEVICE_ON: 
      xnestEventMask |= XNEST_POINTER_EVENT_MASK;
      for (i = 0; i < xnestNumScreens; i++)
	XSelectInput(xnestDisplay, xnestDefaultWindows[i], xnestEventMask);
      break;
    case DEVICE_OFF: 
      xnestEventMask &= ~XNEST_POINTER_EVENT_MASK;
      for (i = 0; i < xnestNumScreens; i++)
	XSelectInput(xnestDisplay, xnestDefaultWindows[i], xnestEventMask);
      break;
    case DEVICE_CLOSE: 
      break;
    }
  return Success;
}
