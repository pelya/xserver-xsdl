/* $XFree86: xc/programs/Xserver/include/extnsionst.h,v 3.9 2003/07/16 01:38:52 dawes Exp $ */

#ifndef EXTENSIONSTRUCT_H
#define EXTENSIONSTRUCT_H 

#include "misc.h"
#include "screenint.h"
#include "extension.h"
#include "gc.h"

typedef struct _ExtensionEntry {
    int index;
    void (* CloseDown)(	/* called at server shutdown */
	struct _ExtensionEntry * /* extension */);
    char *name;               /* extension name */
    int base;                 /* base request number */
    int eventBase;            
    int eventLast;
    int errorBase;
    int errorLast;
    int num_aliases;
    char **aliases;
    pointer extPrivate;
    unsigned short (* MinorOpcode)(	/* called for errors */
	ClientPtr /* client */);
#ifdef XCSECURITY
    Bool secure;		/* extension visible to untrusted clients? */
#endif
} ExtensionEntry;

/* 
 * The arguments may be different for extension event swapping functions.
 * Deal with this by casting when initializing the event's EventSwapVector[]
 * entries.
 */
typedef void (*EventSwapPtr) (xEvent *, xEvent *);

extern EventSwapPtr EventSwapVector[128];

extern void NotImplemented (	/* FIXME: this may move to another file... */
	xEvent *,
	xEvent *);

typedef void (* ExtensionLookupProc)(
#ifdef EXTENSION_PROC_ARGS
    EXTENSION_PROC_ARGS
#else
    /* args no longer indeterminate */
    char *name,
    GCPtr pGC
#endif
);

typedef struct _ProcEntry {
    char *name;
    ExtensionLookupProc proc;
} ProcEntryRec, *ProcEntryPtr;

typedef struct _ScreenProcEntry {
    int num;
    ProcEntryPtr procList;
} ScreenProcEntry;

#define    SetGCVector(pGC, VectorElement, NewRoutineAddress, Atom)    \
    pGC->VectorElement = NewRoutineAddress;

#define    GetGCValue(pGC, GCElement)    (pGC->GCElement)


extern ExtensionEntry *AddExtension(
    char* /*name*/,
    int /*NumEvents*/,
    int /*NumErrors*/,
    int (* /*MainProc*/)(ClientPtr /*client*/),
    int (* /*SwappedMainProc*/)(ClientPtr /*client*/),
    void (* /*CloseDownProc*/)(ExtensionEntry * /*extension*/),
    unsigned short (* /*MinorOpcodeProc*/)(ClientPtr /*client*/)
);

extern Bool AddExtensionAlias(
    char* /*alias*/,
    ExtensionEntry * /*extension*/);

extern ExtensionEntry *CheckExtension(const char *extname);

extern ExtensionEntry *CheckExtension(const char *extname);

extern ExtensionLookupProc LookupProc(
    char* /*name*/,
    GCPtr /*pGC*/);

extern Bool RegisterProc(
    char* /*name*/,
    GCPtr /*pGC*/,
    ExtensionLookupProc /*proc*/);

extern Bool RegisterScreenProc(
    char* /*name*/,
    ScreenPtr /*pScreen*/,
    ExtensionLookupProc /*proc*/);

extern void DeclareExtensionSecurity(
    char * /*extname*/,
    Bool /*secure*/);

#endif /* EXTENSIONSTRUCT_H */

