/* Copyright 2006 by Peter Hutterer <peter@cs.unisa.edu.au> */

/********************************************************************
 * Interface of mpx/extinit.c
 */

#ifndef MPXEXTINIT_H
#define MPXEXTINIT_H

#include "extnsionst.h"

void
MPXExtensionInit(
	void
	);

int
ProcMPXDispatch (
	ClientPtr              /* client */
	);

int
SProcMPXDispatch(
	ClientPtr              /* client */
	);

void
SReplyMPXDispatch (
	ClientPtr              /* client */,
	int                    /* len */,
	xMPXGetExtensionVersionReply *     /* rep */
	);

void
SEventMPXDispatch (
	xEvent *               /* from */,
	xEvent *               /* to */
	);

void
MPXFixExtensionEvents (
	ExtensionEntry 	*      /* extEntry */
	);

void
MPXResetProc(
	ExtensionEntry *       /* unused */
	);

Mask
MPXGetNextExtEventMask (
	void
);

void
MPXSetMaskForExtEvent(
	Mask                   /* mask */,
	int                    /* event */
	);

void
MPXAllowPropagateSuppress (
	Mask                   /* mask */
	);

void
MPXRestoreExtensionEvents (
	void
	);
#endif


