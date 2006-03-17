/*
 * Copyright © 2006  Red Hat, Inc.
 * (C) Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * RED HAT, INC, OR PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
 * OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 *   Kristian Høgsberg <krh@redhat.com>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <assert.h>
#include <string.h>
#include <regionstr.h>
#include <resource.h>
#include <GL/gl.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <GL/internal/glcore.h>
#include <scrnintstr.h>
#include <config.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxext.h>
#include <glxutil.h>
#include <micmap.h>

void GlxWrapInitVisuals(miInitVisualsProcPtr *);

#include "glcontextmodes.h"

struct ScreenVisualsRec {
    int num_vis;
    void *private;
    __GLcontextModes *modes;
};
typedef struct ScreenVisualsRec ScreenVisuals;

static ScreenVisuals screenVisuals[MAXSCREENS];

static int                 numConfigs     = 0;
static __GLXvisualConfig  *visualConfigs  = NULL;
static void              **visualPrivates = NULL;

static int count_bits(unsigned int n)
{
   int bits = 0;

   while (n > 0) {
      if (n & 1) bits++;
      n >>= 1;
   }
   return bits;
}

/*
 * In the case the driver defines no GLX visuals we'll use these.
 * Note that for TrueColor and DirectColor visuals, bufferSize is the 
 * sum of redSize, greenSize, blueSize and alphaSize, which may be larger 
 * than the nplanes/rootDepth of the server's X11 visuals
 */
#define NUM_FALLBACK_CONFIGS 5
static __GLXvisualConfig FallbackConfigs[NUM_FALLBACK_CONFIGS] = {
  /* [0] = RGB, double buffered, Z */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
     0,  0,  0, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    0,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [1] = RGB, double buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
    16, 16, 16, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [2] = RGB+Alpha, double buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 8,      /* rgba sizes */
    -1, -1, -1, -1,     /* rgba masks */
    16, 16, 16, 16,     /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [3] = RGB+Alpha, single buffered, Z, stencil, accum */
  {
    -1,                 /* vid */
    -1,                 /* class */
    True,               /* rgba */
    -1, -1, -1, 8,      /* rgba sizes */
    -1, -1, -1, -1,     /* rgba masks */
    16, 16, 16, 16,     /* rgba accum sizes */
    False,              /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    8,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
  /* [4] = CI, double buffered, Z */
  {
    -1,                 /* vid */
    -1,                 /* class */
    False,              /* rgba? (false = color index) */
    -1, -1, -1, 0,      /* rgba sizes */
    -1, -1, -1, 0,      /* rgba masks */
     0,  0,  0, 0,      /* rgba accum sizes */
    True,               /* doubleBuffer */
    False,              /* stereo */
    -1,                 /* bufferSize */
    16,                 /* depthSize */
    0,                  /* stencilSize */
    0,                  /* auxBuffers */
    0,                  /* level */
    GLX_NONE,           /* visualRating */
    GLX_NONE,           /* transparentPixel */
    0, 0, 0, 0,         /* transparent rgba color (floats scaled to ints) */
    0                   /* transparentIndex */
  },
};


static Bool init_visuals(int *nvisualp, VisualPtr *visualp,
			 VisualID *defaultVisp,
			 int ndepth, DepthPtr pdepth,
			 int rootDepth)
{
    int numRGBconfigs;
    int numCIconfigs;
    int numVisuals = *nvisualp;
    int numNewVisuals;
    int numNewConfigs;
    VisualPtr pVisual = *visualp;
    VisualPtr pVisualNew = NULL;
    VisualID *orig_vid = NULL;
    __GLcontextModes *modes;
    __GLXvisualConfig *pNewVisualConfigs = NULL;
    void **glXVisualPriv;
    void **pNewVisualPriv;
    int found_default;
    int i, j, k;

    if (numConfigs > 0)
        numNewConfigs = numConfigs;
    else
        numNewConfigs = NUM_FALLBACK_CONFIGS;

    /* Alloc space for the list of new GLX visuals */
    pNewVisualConfigs = (__GLXvisualConfig *)
	xalloc(numNewConfigs * sizeof(__GLXvisualConfig));
    if (!pNewVisualConfigs) {
	return FALSE;
    }

    /* Alloc space for the list of new GLX visual privates */
    pNewVisualPriv = (void **) xalloc(numNewConfigs * sizeof(void *));
    if (!pNewVisualPriv) {
	xfree(pNewVisualConfigs);
	return FALSE;
    }

    /*
    ** If SetVisualConfigs was not called, then use default GLX
    ** visual configs.
    */
    if (numConfigs == 0) {
	memcpy(pNewVisualConfigs, FallbackConfigs,
               NUM_FALLBACK_CONFIGS * sizeof(__GLXvisualConfig));
	memset(pNewVisualPriv, 0, NUM_FALLBACK_CONFIGS * sizeof(void *));
    }
    else {
        /* copy driver's visual config info */
        for (i = 0; i < numConfigs; i++) {
            pNewVisualConfigs[i] = visualConfigs[i];
            pNewVisualPriv[i] = visualPrivates[i];
        }
    }

    /* Count the number of RGB and CI visual configs */
    numRGBconfigs = 0;
    numCIconfigs = 0;
    for (i = 0; i < numNewConfigs; i++) {
	if (pNewVisualConfigs[i].rgba)
	    numRGBconfigs++;
	else
	    numCIconfigs++;
    }

    /* Count the total number of visuals to compute */
    numNewVisuals = 0;
    for (i = 0; i < numVisuals; i++) {
        numNewVisuals +=
	    (pVisual[i].class == TrueColor || pVisual[i].class == DirectColor)
	    ? numRGBconfigs : numCIconfigs;
    }

    /* Reset variables for use with the next screen/driver's visual configs */
    visualConfigs = NULL;
    numConfigs = 0;

    /* Alloc temp space for the list of orig VisualIDs for each new visual */
    orig_vid = (VisualID *) xalloc(numNewVisuals * sizeof(VisualID));
    if (!orig_vid) {
	xfree(pNewVisualPriv);
	xfree(pNewVisualConfigs);
	return FALSE;
    }

    /* Alloc space for the list of glXVisuals */
    modes = _gl_context_modes_create(numNewVisuals, sizeof(__GLcontextModes));
    if (modes == NULL) {
	xfree(orig_vid);
	xfree(pNewVisualPriv);
	xfree(pNewVisualConfigs);
	return FALSE;
    }

    /* Alloc space for the list of glXVisualPrivates */
    glXVisualPriv = (void **)xalloc(numNewVisuals * sizeof(void *));
    if (!glXVisualPriv) {
	_gl_context_modes_destroy( modes );
	xfree(orig_vid);
	xfree(pNewVisualPriv);
	xfree(pNewVisualConfigs);
	return FALSE;
    }

    /* Alloc space for the new list of the X server's visuals */
    pVisualNew = (VisualPtr)xalloc(numNewVisuals * sizeof(VisualRec));
    if (!pVisualNew) {
	xfree(glXVisualPriv);
	_gl_context_modes_destroy( modes );
	xfree(orig_vid);
	xfree(pNewVisualPriv);
	xfree(pNewVisualConfigs);
	return FALSE;
    }

    /* Initialize the new visuals */
    found_default = FALSE;
    screenVisuals[screenInfo.numScreens-1].modes = modes;
    for (i = j = 0; i < numVisuals; i++) {
        int is_rgb = (pVisual[i].class == TrueColor ||
		      pVisual[i].class == DirectColor);

	for (k = 0; k < numNewConfigs; k++) {
	    if (pNewVisualConfigs[k].rgba != is_rgb)
		continue;

	    assert( modes != NULL );

	    /* Initialize the new visual */
	    pVisualNew[j] = pVisual[i];
	    pVisualNew[j].vid = FakeClientID(0);

	    /* Check for the default visual */
	    if (!found_default && pVisual[i].vid == *defaultVisp) {
		*defaultVisp = pVisualNew[j].vid;
		found_default = TRUE;
	    }

	    /* Save the old VisualID */
	    orig_vid[j] = pVisual[i].vid;

	    /* Initialize the glXVisual */
	    _gl_copy_visual_to_context_mode( modes, & pNewVisualConfigs[k] );
	    modes->visualID = pVisualNew[j].vid;
	    if (modes->fbconfigID == GLX_DONT_CARE)
		modes->fbconfigID = modes->visualID;

	    /*
	     * If the class is -1, then assume the X visual information
	     * is identical to what GLX needs, and take them from the X
	     * visual.  NOTE: if class != -1, then all other fields MUST
	     * be initialized.
	     */
	    if (modes->visualType == GLX_NONE) {
		modes->visualType = _gl_convert_from_x_visual_type( pVisual[i].class );
		modes->redBits    = count_bits(pVisual[i].redMask);
		modes->greenBits  = count_bits(pVisual[i].greenMask);
		modes->blueBits   = count_bits(pVisual[i].blueMask);
		modes->alphaBits  = modes->alphaBits;
		modes->redMask    = pVisual[i].redMask;
		modes->greenMask  = pVisual[i].greenMask;
		modes->blueMask   = pVisual[i].blueMask;
		modes->alphaMask  = modes->alphaMask;
		modes->rgbBits = (is_rgb)
		    ? (modes->redBits + modes->greenBits +
		       modes->blueBits + modes->alphaBits)
		    : rootDepth;
	    }

	    /* Save the device-dependent private for this visual */
	    glXVisualPriv[j] = pNewVisualPriv[k];

	    j++;
	    modes = modes->next;
	}
    }

    assert(j <= numNewVisuals);

    /* Save the GLX visuals in the screen structure */
    screenVisuals[screenInfo.numScreens-1].num_vis = numNewVisuals;
    screenVisuals[screenInfo.numScreens-1].private = glXVisualPriv;

    /* Set up depth's VisualIDs */
    for (i = 0; i < ndepth; i++) {
	int numVids = 0;
	VisualID *pVids = NULL;
	int k, n = 0;

	/* Count the new number of VisualIDs at this depth */
	for (j = 0; j < pdepth[i].numVids; j++)
	    for (k = 0; k < numNewVisuals; k++)
		if (pdepth[i].vids[j] == orig_vid[k])
		    numVids++;

	/* Allocate a new list of VisualIDs for this depth */
	pVids = (VisualID *)xalloc(numVids * sizeof(VisualID));

	/* Initialize the new list of VisualIDs for this depth */
	for (j = 0; j < pdepth[i].numVids; j++)
	    for (k = 0; k < numNewVisuals; k++)
		if (pdepth[i].vids[j] == orig_vid[k])
		    pVids[n++] = pVisualNew[k].vid;

	/* Update this depth's list of VisualIDs */
	xfree(pdepth[i].vids);
	pdepth[i].vids = pVids;
	pdepth[i].numVids = numVids;
    }

    /* Update the X server's visuals */
    *nvisualp = numNewVisuals;
    *visualp = pVisualNew;

    /* Free the old list of the X server's visuals */
    xfree(pVisual);

    /* Clean up temporary allocations */
    xfree(orig_vid);
    xfree(pNewVisualPriv);
    xfree(pNewVisualConfigs);

    /* Free the private list created by DDX HW driver */
    if (visualPrivates)
        xfree(visualPrivates);
    visualPrivates = NULL;

    return TRUE;
}

void GlxSetVisualConfigs(int nconfigs, 
                         __GLXvisualConfig *configs, void **privates)
{
    numConfigs = nconfigs;
    visualConfigs = configs;
    visualPrivates = privates;
}

static miInitVisualsProcPtr saveInitVisualsProc;

Bool GlxInitVisuals(VisualPtr *visualp, DepthPtr *depthp,
		    int *nvisualp, int *ndepthp,
		    int *rootDepthp, VisualID *defaultVisp,
		    unsigned long sizes, int bitsPerRGB,
		    int preferredVis)
{
    Bool ret;

    if (saveInitVisualsProc) {
        ret = saveInitVisualsProc(visualp, depthp, nvisualp, ndepthp,
                                  rootDepthp, defaultVisp, sizes, bitsPerRGB,
                                  preferredVis);
        if (!ret)
            return False;
    }

    /*
     * Setup the visuals supported by this particular screen.
     */
    init_visuals(nvisualp, visualp, defaultVisp,
		 *ndepthp, *depthp, *rootDepthp);


    return True;
}


/************************************************************************/


void
GlxWrapInitVisuals(miInitVisualsProcPtr *initVisProc)
{
    saveInitVisualsProc = *initVisProc;
    *initVisProc = GlxInitVisuals;
}

static void fixup_visuals(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];
    ScreenVisuals *psv = &screenVisuals[screen];
    int j;
    __GLcontextModes *modes;

    for ( modes = psv->modes ; modes != NULL ; modes = modes->next ) {
	const int vis_class = _gl_convert_to_x_visual_type( modes->visualType );
	const int nplanes = (modes->rgbBits - modes->alphaBits);
	const VisualPtr pVis = pScreen->visuals;

	/* Find a visual that matches the GLX visual's class and size */
	for (j = 0; j < pScreen->numVisuals; j++) {
	    if (pVis[j].class == vis_class &&
		pVis[j].nplanes == nplanes) {

		/* Fixup the masks */
		modes->redMask   = pVis[j].redMask;
		modes->greenMask = pVis[j].greenMask;
		modes->blueMask  = pVis[j].blueMask;

		/* Recalc the sizes */
		modes->redBits   = count_bits(modes->redMask);
		modes->greenBits = count_bits(modes->greenMask);
		modes->blueBits  = count_bits(modes->blueMask);
	    }
	}
    }
}

void __glXScreenInitVisuals(__GLXscreen *screen)
{
    int index = screen->pScreen->myNum;

    screen->modes = screenVisuals[index].modes;
    screen->pVisualPriv = screenVisuals[index].private;
    screen->numVisuals = screenVisuals[index].num_vis;
    screen->numUsableVisuals = screenVisuals[index].num_vis;

    /*
     * The ordering of the rgb compenents might have been changed by the
     * driver after mi initialized them.
     */
    fixup_visuals(index);
}
