/* $XFree86: xc/programs/Xserver/afb/afbcmap.c,v 3.1 1998/11/22 10:36:58 dawes Exp $ */
/* $XConsortium: afbcmap.c,v 4.19 94/04/17 20:28:46 dpw Exp $ */
/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or X Consortium
not be used in advertising or publicity pertaining to
distribution  of  the software  without specific prior
written permission. Sun and X Consortium make no
representations about the suitability of this software for
any purpose. It is provided "as is" without any express or
implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#include "X.h"
#include "Xproto.h"
#include "scrnintstr.h"
#include "colormapst.h"
#include "resource.h"
#include "micmap.h"

int
afbListInstalledColormaps(pScreen, pmaps)
	ScreenPtr		pScreen;
	Colormap		*pmaps;
{
	return miListInstalledColormaps(pScreen, pmaps);
}


void
afbInstallColormap(pmap)
	ColormapPtr		pmap;
{
	miInstallColormap(pmap);
}

void
afbUninstallColormap(pmap)
	ColormapPtr		pmap;
{
	miUninstallColormap(pmap);
}

void
afbResolveColor(pred, pgreen, pblue, pVisual)
	unsigned short		*pred, *pgreen, *pblue;
	register VisualPtr		pVisual;
{
	miResolveColor(pred, pgreen, pblue, pVisual);
}

Bool
afbInitializeColormap(pmap)
	register ColormapPtr		pmap;
{
	return miInitializeColormap(pmap);
}

int
afbExpandDirectColors(pmap, ndef, indefs, outdefs)
	ColormapPtr		pmap;
	int				ndef;
	xColorItem		*indefs, *outdefs;
{
	return miExpandDirectColors(pmap, ndef, indefs, outdefs);
}

Bool
afbCreateDefColormap(pScreen)
	ScreenPtr pScreen;
{
	return miCreateDefColormap(pScreen);
}

Bool
afbSetVisualTypes(depth, visuals, bitsPerRGB)
	int depth;
	int visuals;
	int bitsPerRGB;
{
	return miSetVisualTypes(depth, visuals, bitsPerRGB, -1);
}

/*
 * Given a list of formats for a screen, create a list
 * of visuals and depths for the screen which correspond to
 * the set which can be used with this version of afb.
 */

Bool
afbInitVisuals(visualp, depthp, nvisualp, ndepthp, rootDepthp, defaultVisp,
					 sizes, bitsPerRGB)
	VisualPtr *visualp;
	DepthPtr *depthp;
	int *nvisualp, *ndepthp;
	int *rootDepthp;
	VisualID *defaultVisp;
	unsigned long sizes;
	int bitsPerRGB;
{
	return miInitVisuals(visualp, depthp, nvisualp, ndepthp, rootDepthp,
				defaultVisp, sizes, bitsPerRGB, -1);
}
