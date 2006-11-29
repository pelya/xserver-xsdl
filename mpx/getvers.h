/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef GETVERS_H
#define GETVERS_H 1

int SProcMPXGetExtensionVersion(ClientPtr	/* client */
    );

int ProcMPXGetExtensionVersion(ClientPtr	/* client */
    );

void SRepMPXGetExtensionVersion(ClientPtr /* client */ ,
			      int /* size */ ,
			      mpxGetExtensionVersionReply *	/* rep */
    );

#endif

