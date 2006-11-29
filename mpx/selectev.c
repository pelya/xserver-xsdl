/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */
#include "windowstr.h"	/* window structure  */
#include <X11/extensions/MPX.h>
#include <X11/extensions/MPXproto.h>
#include "extnsionst.h"
#include "extinit.h"	/* LookupDeviceIntRec */

