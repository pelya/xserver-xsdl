/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/sco/sco_mouse.c,v 3.8 1996/12/23 06:50:50 dawes Exp $ */





/* $Xorg: sco_mouse.c,v 1.3 2000/08/17 19:51:29 cpqbld Exp $ */

/******************************************************************************/

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "scrnintstr.h"
#include "compiler.h"

#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

/******************************************************************************/
#ifdef USE_OSMOUSE
/******************************************************************************/

#include	<sys/event.h>
#include	<mouse.h>
#include	"xf86_Config.h"

static dmask_t		real_mask = (dmask_t) (D_REL | D_BUTTON);
static int		config_buttons = 0;

extern int miPointerGetMotionEvents(DeviceIntPtr pPtr, xTimecoord *coords,
				    unsigned long start, unsigned long stop,
				    ScreenPtr pScreen);

/******************************************************************************/
/*
 *	Handle any XF86Config options for "OsMouse", How you treat errors
 *	is up to you, they may or may not be Fatal
 */

void
xf86OsMouseOption(lt, lp)
	int		lt;	/* type returned by gettoken */
	pointer	lp;	/* The lexical return symbol */
{
	if (lt != NUMBER) {
		ErrorF("%s: Invalid Argument to OsMouse, %s\n",
		       "xf86OsMouseOption", "Number of buttons expected");
		return;
	}
	config_buttons = ((LexPtr)lp)->num;
}

/******************************************************************************/
/*
 * xf86OsMouseProc --
 *      Handle the initialization, etc. of a mouse
 */

int
xf86OsMouseProc(pPointer, what)
     DeviceIntPtr	 pPointer;
     int		 what;
{
  unchar		*map;
  int			 i, err, buttons;
  struct devinfo	*dip;
  dmask_t		 dmask;

  switch (what) {
    case DEVICE_INIT: 
      
      pPointer->public.on = FALSE;

      if (ev_init() < 0)
	ErrorF("ev_init: Failed to initialize event driver\n");

      dmask = real_mask;
      xf86Info.mouseDev->mseFd = ev_open(&dmask);
      switch (xf86Info.mouseDev->mseFd) {
	case -1: FatalError("ev_open: Error in Configuration files\n");
	case -2: FatalError("ev_open: No mouse devices to attach\n");
	case -3: FatalError("ev_open: Unable to open a found device\n");
	case -4: FatalError("ev_open: unable to open an event queue\n");
	default:
	  if (xf86Info.mouseDev->mseFd < 0)
	    FatalError("ev_open: Failed to open device, reason unkown\n");
	  break;
      }
      if (dmask != real_mask)
	FatalError("Could not attach the mouse device (0x%x)\n", dmask);
	
      dip = (struct devinfo *) NULL;
      if ((dip = ev_getdev(D_REL, dip)) == (struct devinfo *) NULL)
	FatalError("Could not find info on mouse device\n");
	
      buttons = config_buttons > 0 ? config_buttons : ((int) dip->buttons);
      buttons = buttons > 0 ? buttons : 3; /* just in case */
	
      ErrorF("%s OsMouse has %d buttons\n",
	     buttons == config_buttons ? XCONFIG_GIVEN : XCONFIG_PROBED,
	     buttons);

      map = (unchar *) xalloc(buttons + 1);
      if (map == (unchar *) NULL)
	FatalError("Failed to allocate OsMouse map structure\n");

      for (i = 1; i <= buttons; i++)
	map[i] = i;

      InitPointerDeviceStruct((DevicePtr)pPointer, 
			      map, 
			      buttons,
			      miPointerGetMotionEvents, 
			      (PtrCtrlProcPtr)xf86MseCtrl, 
			      0);
      xfree(map);
      ev_suspend(); /* suspend device until its turned on */
      break;
      
    case DEVICE_ON:
      ev_resume();
      AddEnabledDevice(xf86Info.mouseDev->mseFd);
      xf86Info.mouseDev->lastButtons = 0;
      xf86Info.mouseDev->emulateState = 0;
      pPointer->public.on = TRUE;
      break;
      
    case DEVICE_CLOSE:
    case DEVICE_OFF:
      pPointer->public.on = FALSE;
      RemoveEnabledDevice(xf86Info.mouseDev->mseFd);
      if (what == DEVICE_CLOSE) {
	ev_close();
	xf86Info.mouseDev->mseFd = -1;
      } else
	ev_suspend();
      break;
    }
  
  return Success;
}

/******************************************************************************/
/*
 * xf86OsMouseEvents --
 *      Get some events from our queue.  Process all outstanding events now.
 */

void
xf86OsMouseEvents()
{
	EVENT *evp;
	static long time = -1;

	while ((evp = ev_read()) != (EVENT *) NULL ) {
#if DEBUG
		if (time == -1)
			time = EV_TIME(*evp);
		ErrorF("sco_event time(%ld) tag(%d) butts(%d) x(%ld) y(%ld)\n",
			EV_TIME(*evp) - time, EV_TAG(*evp), EV_BUTTONS(*evp),
			EV_DX(*evp), EV_DY(*evp));
#endif
		xf86PostMseEvent(xf86Info.pMouse,EV_BUTTONS(*evp), EV_DX(*evp), -(EV_DY(*evp)));
		ev_pop();
	}

	xf86Info.inputPending = TRUE;
}

/******************************************************************************/
#endif /* USE_OSMOUSE */
/******************************************************************************/
