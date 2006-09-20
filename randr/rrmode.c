/*
 * Copyright © 2006 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "randrstr.h"

RESTYPE	RRModeType;

RRModePtr
RRModeGet (ScreenPtr	pScreen,
	   xRRModeInfo	*modeInfo,
	   const char	*name)
{
    rrScrPriv (pScreen);
    int	i;
    RRModePtr	mode;
    RRModePtr	*modes;

    for (i = 0; i < pScrPriv->numModes; i++)
    {
	mode = pScrPriv->modes[i];
	modeInfo->id = mode->mode.id;
	if (!memcmp (modeInfo, &mode->mode, sizeof (xRRModeInfo)) &&
	    !memcmp (name, mode->name, modeInfo->nameLength))
	{
	    ++mode->refcnt;
	    return mode;
	}
    }

    mode = xalloc (sizeof (RRModeRec) + modeInfo->nameLength + 1);
    if (!mode)
	return NULL;
    mode->refcnt = 1;
    mode->mode = *modeInfo;
    mode->name = (char *) (mode + 1);
    memcpy (mode->name, name, modeInfo->nameLength);
    mode->name[modeInfo->nameLength] = '\0';

    if (pScrPriv->numModes)
	modes = xrealloc (pScrPriv->modes,
			  (pScrPriv->numModes + 1) * sizeof (RRModePtr));
    else
	modes = xalloc (sizeof (RRModePtr));

    if (!modes)
    {
	xfree (mode);
	return NULL;
    }

    mode->mode.id = FakeClientID(0);
    if (!AddResource (mode->mode.id, RRModeType, (pointer) mode))
	return NULL;
    ++mode->refcnt;
    pScrPriv->modes = modes;
    pScrPriv->modes[pScrPriv->numModes++] = mode;
    pScrPriv->changed = TRUE;
    return mode;
}

void
RRModeDestroy (RRModePtr mode)
{
    if (--mode->refcnt > 0)
	return;
    xfree (mode);
}

static int
RRModeDestroyResource (pointer value, XID pid)
{
    RRModeDestroy ((RRModePtr) value);
    return 1;
}

Bool
RRModeInit (void)
{
    RRModeType = CreateNewResourceType (RRModeDestroyResource);
    if (!RRModeType)
	return FALSE;
#ifdef XResExtension
    RegisterResourceName (RRModeType, "MODE");
#endif
    return TRUE;
}

int
ProcRRCreateMode (ClientPtr client)
{
    REQUEST(xRRCreateModeReq);
    
    REQUEST_SIZE_MATCH(xRRCreateModeReq);
    (void) stuff;
    return BadImplementation; 
}

int
ProcRRDestroyMode (ClientPtr client)
{
    REQUEST(xRRDestroyModeReq);
    
    REQUEST_SIZE_MATCH(xRRDestroyModeReq);
    (void) stuff;
    return BadImplementation; 
}

int
ProcRRAddOutputMode (ClientPtr client)
{
    REQUEST(xRRAddOutputModeReq);
    
    REQUEST_SIZE_MATCH(xRRAddOutputModeReq);
    (void) stuff;
    return BadImplementation; 
}

int
ProcRRDeleteOutputMode (ClientPtr client)
{
    REQUEST(xRRDeleteOutputModeReq);
    
    REQUEST_SIZE_MATCH(xRRDeleteOutputModeReq);
    (void) stuff;
    return BadImplementation; 
}

