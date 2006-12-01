/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef MPXEVENTS_H
#define MPXEVENTS_H 1

#include <X11/X.h>	/* for inputstr.h    */
#include <X11/Xproto.h>	/* Request macro     */
#include "inputstr.h"	/* DeviceIntPtr      */

extern int MPXSelectForWindow(
	WindowPtr              /* pWin */,
	ClientPtr              /* client */,
        int                    /* mask */);

#endif
