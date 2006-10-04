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

static Bool
RRModeEqual (xRRModeInfo *a, xRRModeInfo *b)
{
    if (a->width != b->width) return FALSE;
    if (a->height != b->height) return FALSE;
    if (a->mmWidth != b->mmWidth) return FALSE;
    if (a->mmHeight != b->mmHeight) return FALSE;
    if (a->dotClock != b->dotClock) return FALSE;
    if (a->hSyncStart != b->hSyncStart) return FALSE;
    if (a->hSyncEnd != b->hSyncEnd) return FALSE;
    if (a->hTotal != b->hTotal) return FALSE;
    if (a->hSkew != b->hSkew) return FALSE;
    if (a->vSyncStart != b->vSyncStart) return FALSE;
    if (a->vSyncEnd != b->vSyncEnd) return FALSE;
    if (a->vTotal != b->vTotal) return FALSE;
    if (a->nameLength != b->nameLength) return FALSE;
    if (a->modeFlags != b->modeFlags) return FALSE;
    if (a->origin != b->origin) return FALSE;
    return TRUE;
}

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
	if (RRModeEqual (&mode->mode, modeInfo) &&
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
    mode->screen = pScreen;

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
    ScreenPtr	    pScreen;
    rrScrPrivPtr    pScrPriv;
    int	m;
    
    if (--mode->refcnt > 0)
	return;
    pScreen = mode->screen;
    pScrPriv = rrGetScrPriv (pScreen);
    for (m = 0; m < pScrPriv->numModes; m++)
    {
	if (pScrPriv->modes[m] == mode)
	{
	    memmove (pScrPriv->modes + m, pScrPriv->modes + m + 1,
		     (pScrPriv->numModes - m - 1) * sizeof (RRModePtr));
	    pScrPriv->numModes--;
	    if (!pScrPriv->numModes)
	    {
		xfree (pScrPriv->modes);
		pScrPriv->modes = NULL;
	    }
	    pScrPriv->changed = TRUE;
	    break;
	}
    }
    
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

void
RRModePruneUnused (ScreenPtr pScreen)
{
    rrScrPriv (pScreen);
    RRModePtr	*unused, mode;
    int		m;
    int		num = pScrPriv->numModes;

    unused = xalloc (num * sizeof (RRModePtr));
    if (!unused)
	return;
    memcpy (unused, pScrPriv->modes, num * sizeof (RRModePtr));
    for (m = 0; m < num; m++) {
	mode = unused[m];
	if (mode->refcnt == 1 && mode->mode.origin != RRModeOriginUser)
	    FreeResource (mode->mode.id, 0);
    }
    xfree (unused);
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

