/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */


#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef SELECTEV_H
#define SELECTEV_H 1

int SProcMPXSelectEvents(ClientPtr	/* client */
    );

int ProcMPXSelectEvents(ClientPtr	/* client */
    );

#endif /* SELECTEV_H */
