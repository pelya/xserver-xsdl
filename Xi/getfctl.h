/* $XFree86: xc/programs/Xserver/Xi/getfctl.h,v 3.1 1996/04/15 11:18:39 dawes Exp $ */
/************************************************************

Copyright 1996 by Thomas E. Dickey <dickey@clark.net>

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of the above listed
copyright holder(s) not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.

THE ABOVE LISTED COPYRIGHT HOLDER(S) DISCLAIM ALL WARRANTIES WITH REGARD
TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDER(S) BE
LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifndef GETFCTL_H
#define GETFCTL_H 1

int
SProcXGetFeedbackControl(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

int
ProcXGetFeedbackControl(
#if NeedFunctionPrototypes
	ClientPtr              /* client */
#endif
	);

void
CopySwapKbdFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	KbdFeedbackPtr         /* k */,
	char **                /* buf */
#endif
	);

void
CopySwapPtrFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	PtrFeedbackPtr         /* p */,
	char **                /* buf */
#endif
	);

void
CopySwapIntegerFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	IntegerFeedbackPtr     /* i */,
	char **                /* buf */
#endif
	);

void
CopySwapStringFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	StringFeedbackPtr      /* s */,
	char **                /* buf */
#endif
	);

void
CopySwapLedFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	LedFeedbackPtr         /* l */,
	char **                /* buf */
#endif
	);

void
CopySwapBellFeedback (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	BellFeedbackPtr        /* b */,
	char **                /* buf */
#endif
	);

void
SRepXGetFeedbackControl (
#if NeedFunctionPrototypes
	ClientPtr              /* client */,
	int                    /* size */,
	xGetFeedbackControlReply * /* rep */
#endif
	);

#endif /* GETFCTL_H */
