/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef MPXGLOBALS_H
#define MPXGLOBALS_H 1

extern int MPXReqCode;
extern int MPXEventBase;
extern int MPXErrorBase;

extern Mask PropagateMask[];

extern int MPXmskidx;

/* events */
extern int MPXButtonPress;
extern int MPXButtonRelease;
extern int MPXMotionNotify;
extern int MPXLastEvent;

#define IsMPXEvent(xE) \
    ((xE)->u.u.type >= MPXEventBase \
     && (xE)->u.u.type < MPXLastEvent) 


#endif
