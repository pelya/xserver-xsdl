/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#define NEED_EVENTS
#define NEED_REPLIES
#include <X11/Xproto.h>
#include "misc.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "dispatch.h"
#ifdef XACE
#include "xace.h"
#endif

#define EXTENSION_BASE  128
#define EXTENSION_EVENT_BASE  64
#define LAST_EVENT  128
#define LAST_ERROR 255

ScreenProcEntry AuxillaryScreenProcs[MAXSCREENS];

static ExtensionEntry **extensions = (ExtensionEntry **)NULL;

int lastEvent = EXTENSION_EVENT_BASE;
static int lastError = FirstExtensionError;
static unsigned int NumExtensions = 0;

extern int extensionPrivateLen;
extern unsigned *extensionPrivateSizes;
extern unsigned totalExtensionSize;

static void
InitExtensionPrivates(ExtensionEntry *ext)
{
    register char *ptr;
    DevUnion *ppriv;
    register unsigned *sizes;
    register unsigned size;
    register int i;

    if (totalExtensionSize == sizeof(ExtensionEntry))
	ppriv = (DevUnion *)NULL;
    else
	ppriv = (DevUnion *)(ext + 1);

    ext->devPrivates = ppriv;
    sizes = extensionPrivateSizes;
    ptr = (char *)(ppriv + extensionPrivateLen);
    for (i = extensionPrivateLen; --i >= 0; ppriv++, sizes++)
    {
	if ( (size = *sizes) )
	{
	    ppriv->ptr = (pointer)ptr;
	    ptr += size;
	}
	else
	    ppriv->ptr = (pointer)NULL;
    }
}

_X_EXPORT ExtensionEntry *
AddExtension(char *name, int NumEvents, int NumErrors, 
	     int (*MainProc)(ClientPtr c1), 
	     int (*SwappedMainProc)(ClientPtr c2), 
	     void (*CloseDownProc)(ExtensionEntry *e), 
	     unsigned short (*MinorOpcodeProc)(ClientPtr c3))
{
    int i;
    register ExtensionEntry *ext, **newexts;

    if (!MainProc || !SwappedMainProc || !CloseDownProc || !MinorOpcodeProc)
        return((ExtensionEntry *) NULL);
    if ((lastEvent + NumEvents > LAST_EVENT) || 
	        (unsigned)(lastError + NumErrors > LAST_ERROR))
        return((ExtensionEntry *) NULL);

    ext = (ExtensionEntry *) xalloc(totalExtensionSize);
    if (!ext)
	return((ExtensionEntry *) NULL);
    bzero(ext, totalExtensionSize);
    InitExtensionPrivates(ext);
    ext->name = (char *)xalloc(strlen(name) + 1);
    ext->num_aliases = 0;
    ext->aliases = (char **)NULL;
    if (!ext->name)
    {
	xfree(ext);
	return((ExtensionEntry *) NULL);
    }
    strcpy(ext->name,  name);
    i = NumExtensions;
    newexts = (ExtensionEntry **) xrealloc(extensions,
					   (i + 1) * sizeof(ExtensionEntry *));
    if (!newexts)
    {
	xfree(ext->name);
	xfree(ext);
	return((ExtensionEntry *) NULL);
    }
    NumExtensions++;
    extensions = newexts;
    extensions[i] = ext;
    ext->index = i;
    ext->base = i + EXTENSION_BASE;
    ext->CloseDown = CloseDownProc;
    ext->MinorOpcode = MinorOpcodeProc;
    ProcVector[i + EXTENSION_BASE] = MainProc;
    SwappedProcVector[i + EXTENSION_BASE] = SwappedMainProc;
    if (NumEvents)
    {
        ext->eventBase = lastEvent;
	ext->eventLast = lastEvent + NumEvents;
	lastEvent += NumEvents;
    }
    else
    {
        ext->eventBase = 0;
        ext->eventLast = 0;
    }
    if (NumErrors)
    {
        ext->errorBase = lastError;
	ext->errorLast = lastError + NumErrors;
	lastError += NumErrors;
    }
    else
    {
        ext->errorBase = 0;
        ext->errorLast = 0;
    }

    return(ext);
}

_X_EXPORT Bool AddExtensionAlias(char *alias, ExtensionEntry *ext)
{
    char *name;
    char **aliases;

    aliases = (char **)xrealloc(ext->aliases,
				(ext->num_aliases + 1) * sizeof(char *));
    if (!aliases)
	return FALSE;
    ext->aliases = aliases;
    name = (char *)xalloc(strlen(alias) + 1);
    if (!name)
	return FALSE;
    strcpy(name,  alias);
    ext->aliases[ext->num_aliases] = name;
    ext->num_aliases++;
    return TRUE;
}

static int
FindExtension(char *extname, int len)
{
    int i, j;

    for (i=0; i<NumExtensions; i++)
    {
	if ((strlen(extensions[i]->name) == len) &&
	    !strncmp(extname, extensions[i]->name, len))
	    break;
	for (j = extensions[i]->num_aliases; --j >= 0;)
	{
	    if ((strlen(extensions[i]->aliases[j]) == len) &&
		!strncmp(extname, extensions[i]->aliases[j], len))
		break;
	}
	if (j >= 0) break;
    }
    return ((i == NumExtensions) ? -1 : i);
}

/*
 * CheckExtension returns the extensions[] entry for the requested
 * extension name.  Maybe this could just return a Bool instead?
 */
_X_EXPORT ExtensionEntry *
CheckExtension(const char *extname)
{
    int n;

    n = FindExtension((char*)extname, strlen(extname));
    if (n != -1)
	return extensions[n];
    else
	return NULL;
}

/*
 * Added as part of Xace.
 */
ExtensionEntry *
GetExtensionEntry(int major)
{    
    if (major < EXTENSION_BASE)
	return NULL;
    major -= EXTENSION_BASE;
    if (major >= NumExtensions)
	return NULL;
    return extensions[major];
}

_X_EXPORT void
DeclareExtensionSecurity(char *extname, Bool secure)
{
#ifdef XACE
    int i = FindExtension(extname, strlen(extname));
    if (i >= 0)
	XaceHook(XACE_DECLARE_EXT_SECURE, extensions[i], secure);
#endif
}

_X_EXPORT unsigned short
StandardMinorOpcode(ClientPtr client)
{
    return ((xReq *)client->requestBuffer)->data;
}

_X_EXPORT unsigned short
MinorOpcodeOfRequest(ClientPtr client)
{
    unsigned char major;

    major = ((xReq *)client->requestBuffer)->reqType;
    if (major < EXTENSION_BASE)
	return 0;
    major -= EXTENSION_BASE;
    if (major >= NumExtensions)
	return 0;
    return (*extensions[major]->MinorOpcode)(client);
}

void
CloseDownExtensions()
{
    register int i,j;

    for (i = NumExtensions - 1; i >= 0; i--)
    {
	(* extensions[i]->CloseDown)(extensions[i]);
	NumExtensions = i;
	xfree(extensions[i]->name);
	for (j = extensions[i]->num_aliases; --j >= 0;)
	    xfree(extensions[i]->aliases[j]);
	xfree(extensions[i]->aliases);
	xfree(extensions[i]);
    }
    xfree(extensions);
    extensions = (ExtensionEntry **)NULL;
    lastEvent = EXTENSION_EVENT_BASE;
    lastError = FirstExtensionError;
    for (i=0; i<MAXSCREENS; i++)
    {
	register ScreenProcEntry *spentry = &AuxillaryScreenProcs[i];

	while (spentry->num)
	{
	    spentry->num--;
	    xfree(spentry->procList[spentry->num].name);
	}
	xfree(spentry->procList);
	spentry->procList = (ProcEntryPtr)NULL;
    }
}


int
ProcQueryExtension(ClientPtr client)
{
    xQueryExtensionReply reply;
    int i;
    REQUEST(xQueryExtensionReq);

    REQUEST_FIXED_SIZE(xQueryExtensionReq, stuff->nbytes);
    
    reply.type = X_Reply;
    reply.length = 0;
    reply.major_opcode = 0;
    reply.sequenceNumber = client->sequence;

    if ( ! NumExtensions )
        reply.present = xFalse;
    else
    {
	i = FindExtension((char *)&stuff[1], stuff->nbytes);
        if (i < 0
#ifdef XACE
	    /* call callbacks to find out whether to show extension */
	    || !XaceHook(XACE_EXT_ACCESS, client, extensions[i])
#endif
	    )
            reply.present = xFalse;
        else
        {            
            reply.present = xTrue;
	    reply.major_opcode = extensions[i]->base;
	    reply.first_event = extensions[i]->eventBase;
	    reply.first_error = extensions[i]->errorBase;
	}
    }
    WriteReplyToClient(client, sizeof(xQueryExtensionReply), &reply);
    return(client->noClientException);
}

int
ProcListExtensions(ClientPtr client)
{
    xListExtensionsReply reply;
    char *bufptr, *buffer;
    int total_length = 0;

    REQUEST_SIZE_MATCH(xReq);

    reply.type = X_Reply;
    reply.nExtensions = 0;
    reply.length = 0;
    reply.sequenceNumber = client->sequence;
    buffer = NULL;

    if ( NumExtensions )
    {
        register int i, j;

        for (i=0;  i<NumExtensions; i++)
	{
#ifdef XACE
	    /* call callbacks to find out whether to show extension */
	    if (!XaceHook(XACE_EXT_ACCESS, client, extensions[i]))
		continue;
#endif
	    total_length += strlen(extensions[i]->name) + 1;
	    reply.nExtensions += 1 + extensions[i]->num_aliases;
	    for (j = extensions[i]->num_aliases; --j >= 0;)
		total_length += strlen(extensions[i]->aliases[j]) + 1;
	}
        reply.length = (total_length + 3) >> 2;
	buffer = bufptr = (char *)ALLOCATE_LOCAL(total_length);
	if (!buffer)
	    return(BadAlloc);
        for (i=0;  i<NumExtensions; i++)
        {
	    int len;
#ifdef XACE
	    if (!XaceHook(XACE_EXT_ACCESS, client, extensions[i]))
		continue;
#endif
            *bufptr++ = len = strlen(extensions[i]->name);
	    memmove(bufptr, extensions[i]->name,  len);
	    bufptr += len;
	    for (j = extensions[i]->num_aliases; --j >= 0;)
	    {
		*bufptr++ = len = strlen(extensions[i]->aliases[j]);
		memmove(bufptr, extensions[i]->aliases[j],  len);
		bufptr += len;
	    }
	}
    }
    WriteReplyToClient(client, sizeof(xListExtensionsReply), &reply);
    if (reply.length)
    {
        WriteToClient(client, total_length, buffer);
    	DEALLOCATE_LOCAL(buffer);
    }
    return(client->noClientException);
}


ExtensionLookupProc 
LookupProc(char *name, GCPtr pGC)
{
    register int i;
    register ScreenProcEntry *spentry;
    spentry  = &AuxillaryScreenProcs[pGC->pScreen->myNum];
    if (spentry->num)    
    {
        for (i = 0; i < spentry->num; i++)
            if (strcmp(name, spentry->procList[i].name) == 0)
                return(spentry->procList[i].proc);
    }
    return (ExtensionLookupProc)NULL;
}

Bool
RegisterProc(char *name, GC *pGC, ExtensionLookupProc proc)
{
    return RegisterScreenProc(name, pGC->pScreen, proc);
}

Bool
RegisterScreenProc(char *name, ScreenPtr pScreen, ExtensionLookupProc proc)
{
    register ScreenProcEntry *spentry;
    register ProcEntryPtr procEntry = (ProcEntryPtr)NULL;
    char *newname;
    int i;

    spentry = &AuxillaryScreenProcs[pScreen->myNum];
    /* first replace duplicates */
    if (spentry->num)
    {
        for (i = 0; i < spentry->num; i++)
            if (strcmp(name, spentry->procList[i].name) == 0)
	    {
                procEntry = &spentry->procList[i];
		break;
	    }
    }
    if (procEntry)
        procEntry->proc = proc;
    else
    {
	newname = (char *)xalloc(strlen(name)+1);
	if (!newname)
	    return FALSE;
	procEntry = (ProcEntryPtr)
			    xrealloc(spentry->procList,
				     sizeof(ProcEntryRec) * (spentry->num+1));
	if (!procEntry)
	{
	    xfree(newname);
	    return FALSE;
	}
	spentry->procList = procEntry;
        procEntry += spentry->num;
        procEntry->name = newname;
        strcpy(newname, name);
        procEntry->proc = proc;
        spentry->num++;        
    }
    return TRUE;
}
