/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef QUERYPTR_H
#define QUERYPTR_H 1

int SProcMPXQueryPointer(ClientPtr	/* client */
    );

int ProcMPXQueryPointer(ClientPtr	/* client */
    );

#endif /* QUERYPTR_H */
