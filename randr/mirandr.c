/*
 * Copyright © 2000 Compaq Computer Corporation
 * Copyright © 2002 Hewlett-Packard Company
 * Copyright © 2006 Intel Corporation
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
 *
 * Author:  Jim Gettys, Hewlett-Packard Company, Inc.
 *	    Keith Packard, Intel Corporation
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "scrnintstr.h"
#include "mi.h"
#include "randrstr.h"
#include <stdio.h>

/*
 * This function assumes that only a single depth can be
 * displayed at a time, but that all visuals of that depth
 * can be displayed simultaneously.  It further assumes that
 * only a single size is available.  Hardware providing
 * additional capabilties should use different code.
 * XXX what to do here....
 */

Bool
miRRGetInfo (ScreenPtr pScreen, Rotation *rotations)
{
    int	i;
    Bool setConfig = FALSE;
    RRMonitorPtr pMonitor;
    
    pMonitor = RRRegisterMonitor (pScreen, NULL, RR_Rotate_0);
    for (i = 0; i < pScreen->numDepths; i++)
    {
	if (pScreen->allowedDepths[i].numVids)
	{
	    xRRMonitorMode		rrMode;
	    RRModePtr			pMode;
	    char			name[64];

	    sprintf (name, "%dx%d", pScreen->width, pScreen->height);
	    memset (&rrMode, '\0', sizeof (rrMode));
	    rrMode.width = pScreen->width;
	    rrMode.height = pScreen->height;
	    rrMode.widthInMillimeters = pScreen->mmWidth;
	    rrMode.heightInMillimeters = pScreen->mmHeight;
	    pMonitor = RRRegisterMonitor (pScreen, RR_Rotate_0);
	    pMode = RRRegisterMode (pMonitor,
				    &rrMode,
				    name,
				    strlen (name));
	    if (!pMode)
		return FALSE;
	    if (!setConfig)
	    {
		RRSetCurrentMode (pMonitor, pMode, 0, 0, RR_Rotate_0);
		setConfig = TRUE;
	    }
	}
    }
    return TRUE;
}

/*
 * Any hardware that can actually change anything will need something
 * different here
 */
Bool
miRRSetMode (ScreenPtr	pScreen,
	     int	monitor,
	     RRModePtr	pMode,
	     Rotation	rotation)
{
    return TRUE;
}


Bool
miRandRInit (ScreenPtr pScreen)
{
    rrScrPrivPtr    rp;
    
    if (!RRScreenInit (pScreen))
	return FALSE;
    rp = rrGetScrPriv(pScreen);
    rp->rrGetInfo = miRRGetInfo;
    rp->rrSetMode = miRRSetMode;
    return TRUE;
}
