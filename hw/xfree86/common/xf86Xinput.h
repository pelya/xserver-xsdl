/* $Xorg: xf86Xinput.h,v 1.3 2000/08/17 19:50:31 cpqbld Exp $ */
/*
 * Copyright 1995,1996 by Frederic Lepied, France. <fred@sugix.frmug.fr.net>
 *                                                                            
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.                   
 *                                                                            
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Xinput.h,v 3.14.2.1 1997/05/12 12:52:29 hohndel Exp $ */

#ifndef _xf86Xinput_h
#define _xf86Xinput_h

#ifndef NEED_EVENTS
#define NEED_EVENTS
#endif
#include "X.h"
#include "Xproto.h"
#include "inputstr.h"
#include "XI.h"
#include "XIproto.h"
#include "XIstubs.h"

#define XI86_NO_OPEN_ON_INIT    1 /* open the device only when needed */
#define XI86_CONFIGURED         2 /* the device has been configured */
#define XI86_ALWAYS_CORE	4 /* the device always controls the pointer */

#ifdef PRIVATE
#undef PRIVATE
#endif
#define PRIVATE(dev) (((LocalDevicePtr)((dev)->public.devicePrivate))->private)

#ifdef HAS_MOTION_HISTORY
#undef HAS_MOTION_HISTORY
#endif
#define HAS_MOTION_HISTORY(local) ((local)->dev->valuator && (local)->dev->valuator->numMotionEvents)

typedef struct _LocalDeviceRec {  
  char		*name;
  int           flags;
  Bool		(*device_config)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec** /*array*/,
    int /*index*/,
    int /*max*/,
    LexPtr /*val*/
#endif
    );
  Bool		(*device_control)(
#if NeedNestedPrototypes
    DeviceIntPtr /*device*/,
    int /*what*/
#endif
    );
  void		(*read_input)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec* /*local*/
#endif
    );
  int           (*control_proc)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec* /*local*/,
    xDeviceCtl* /* control */
#endif
    );
  void          (*close_proc)(
#if NeedNestedPrototypes
    struct _LocalDeviceRec* /*local*/
#endif
    );
  int           (*switch_mode)(
#if NeedNestedPrototypes
    ClientPtr /*client*/,
    DeviceIntPtr /*dev*/,
    int /*mode*/
#endif
    );
    int			fd;
    Atom		atom;
    DeviceIntPtr	dev;
    pointer		private;
    int			private_flags;
    pointer		motion_history;
    ValuatorMotionProcPtr motion_history_proc;
    unsigned int	history_size;	/* only for configuration purpose */
    unsigned int	first;
    unsigned int	last;
    char		*type_name;
    IntegerFeedbackPtr	always_core_feedback;
} LocalDeviceRec, *LocalDevicePtr;

typedef struct _DeviceAssocRec 
{
  char                  *config_section_name;
  LocalDevicePtr        (*device_allocate)(
#if NeedNestedPrototypes
    void
#endif
);
} DeviceAssocRec, *DeviceAssocPtr;

extern	int		DeviceKeyPress;
extern	int		DeviceKeyRelease;
extern	int		DeviceButtonPress;
extern	int		DeviceButtonRelease;
extern	int		DeviceMotionNotify;
extern	int		DeviceValuator;
extern	int		ProximityIn;
extern	int		ProximityOut;

extern int
xf86IsCorePointer(
#if NeedFunctionPrototypes
	      DeviceIntPtr /*dev*/
#endif
);

extern int
xf86IsCoreKeyboard(
#if NeedFunctionPrototypes
	       DeviceIntPtr /*dev*/
#endif
);

extern void
xf86AlwaysCore(
#if NeedFunctionPrototypes
	       LocalDevicePtr	/*local*/,
	       Bool		/*always*/
#endif
);

void
xf86configExtendedInputSection(
#ifdef NeedFunctionPrototypes
		LexPtr          /* val */
#endif
);

void
xf86AddDeviceAssoc(
#ifdef NeedFunctionPrototypes
		DeviceAssocPtr	/* assoc */
#endif
);

void
InitExtInput(
#ifdef NeedFunctionPrototypes
		void
#endif
);

Bool
xf86eqInit (
#ifdef NeedFunctionPrototypes
		DevicePtr       /* pKbd */,
		DevicePtr       /* pPtr */
#endif
);

void
xf86eqEnqueue (
#ifdef NeedFunctionPrototypes
		struct _xEvent * /*event */
#endif
);

void
xf86eqProcessInputEvents (
#ifdef NeedFunctionPrototypes
		void
#endif
);

void
xf86PostMotionEvent(
#if NeedVarargsPrototypes
		DeviceIntPtr	/*device*/,
		int		/*is_absolute*/,
		int		/*first_valuator*/,
		int		/*num_valuators*/,
		...
#endif		
);

void
xf86PostProximityEvent(
#if NeedVarargsPrototypes
		   DeviceIntPtr	/*device*/,
		   int		/*is_in*/,
		   int		/*first_valuator*/,
		   int		/*num_valuators*/,
		   ...
#endif
);

void
xf86PostButtonEvent(
#if NeedVarargsPrototypes
		DeviceIntPtr	/*device*/,
		int		/*is_absolute*/,
		int		/*button*/,
		int		/*is_down*/,
		int		/*first_valuator*/,
		int		/*num_valuators*/,
		...
#endif
);

void
xf86PostKeyEvent(
#if NeedVarargsPrototypes
		 DeviceIntPtr	device,
		 unsigned int	key_code,
		 int		is_down,
		 int		is_absolute,
		 int		first_valuator,
		 int		num_valuators,
		 ...
#endif
);

void
xf86AddDeviceAssoc(
#if NeedFunctionPrototypes
		DeviceAssocPtr	/*assoc*/
#endif
);

void
xf86MotionHistoryAllocate(
#if NeedFunctionPrototypes
		      LocalDevicePtr	local
#endif
);

int
xf86GetMotionEvents(
#if NeedFunctionPrototypes
		    DeviceIntPtr	dev,
		    xTimecoord		*buff,
		    unsigned long	start,
		    unsigned long	stop,
		    ScreenPtr		pScreen
#endif
);

void
xf86XinputFinalizeInit(
#if NeedFunctionPrototypes
		       DeviceIntPtr	dev
#endif
);

Bool
xf86CheckButton(
#if NeedFunctionPrototypes
		int	button,
		int	down
#endif
);

#endif /* _xf86Xinput_h */
