/* $XFree86: xc/programs/Xserver/GL/glx/glxscreens.c,v 1.10 2002/04/04 14:05:36 eich Exp $ */
/*
** License Applicability. Except to the extent portions of this file are
** made subject to an alternative license as permitted in the SGI Free
** Software License B, Version 1.1 (the "License"), the contents of this
** file are subject only to the provisions of the License. You may not use
** this file except in compliance with the License. You may obtain a copy
** of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
** Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
** 
** http://oss.sgi.com/projects/FreeB
** 
** Note that, as provided in the License, the Software is distributed on an
** "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
** DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
** CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
** PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
** 
** Original Code. The Original Code is: OpenGL Sample Implementation,
** Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
** Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
** Copyright in any portions created by third parties is as indicated
** elsewhere herein. All Rights Reserved.
** 
** Additional Notice Provisions: The application programming interfaces
** established by SGI in conjunction with the Original Code are The
** OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
** April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
** 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
** Window System(R) (Version 1.3), released October 19, 1998. This software
** was created using the OpenGL(R) version 1.2.1 Sample Implementation
** published by SGI, but has not been independently verified as being
** compliant with the OpenGL(R) version 1.2.1 Specification.
**
*/

#ifdef IN_MODULE
#include <xf86_ansic.h>
#else
#include <string.h>
#include <signal.h>
#endif

#include <windowstr.h>

#include "glxserver.h"
#include "glxutil.h"

static char GLServerExtensions[] = 
			"GL_ARB_imaging "
			"GL_ARB_multitexture "
			"GL_ARB_texture_border_clamp "
			"GL_ARB_texture_cube_map "
			"GL_ARB_texture_env_add "
			"GL_ARB_texture_env_combine "
			"GL_ARB_texture_env_dot3 "
			"GL_ARB_transpose_matrix "
			"GL_EXT_abgr "
			"GL_EXT_blend_color "
			"GL_EXT_blend_minmax "
			"GL_EXT_blend_subtract "
			"GL_EXT_texture_env_add "
			"GL_EXT_texture_env_combine "
			"GL_EXT_texture_env_dot3 "
			"GL_EXT_texture_lod_bias "
			;

/*
** We have made the simplifying assuption that the same extensions are 
** supported across all screens in a multi-screen system.
*/
static char GLXServerVendorName[] = "SGI";
static char GLXServerVersion[] = "1.2";
static char GLXServerExtensions[] = 
			"GLX_EXT_visual_info "
			"GLX_EXT_visual_rating "
			"GLX_EXT_import_context "
			;

/*
** This comes from the GL library that the server will link with.  Right
** now, that is the DDX Sample OpenGL.
*/
extern __GLXscreenInfo __glDDXScreenInfo;

__GLXscreenInfo *__glXScreens[] = {
    &__glDDXScreenInfo,
};

GLint __glXNumStaticScreens = (sizeof __glXScreens / sizeof __glXScreens[0]);

__GLXscreenInfo *__glXActiveScreens;
GLint __glXNumActiveScreens;

RESTYPE __glXDrawableRes;

static int
CountBits(unsigned long mask)
{
    int count = 0;

    while(mask) {
	count += (mask&1);
	mask >>= 1;
    }

    return count;
}

#if 0
/*
** A typical implementation would not probably not run through the screen's
** visuals to find ones that match the visual configs supplied by the DDX
** Sample OpenGL as we do here; we have done this to make this code easy to
** drop into an existing X server.
*/
static int matchVisuals(__GLXvisualConfig *pGlxVisual, int numVisuals,
			 int screen)
{
    int i, j;
    __GLXvisualConfig *pvis = pGlxVisual;
    ScreenPtr pScreen = screenInfo.screens[screen];
    VisualPtr pVisual;
    int numMatchingVisuals = 0; 
    int *used;

    used = (int *)__glXMalloc(pScreen->numVisuals*sizeof(int));
    __glXMemset(used, 0, pScreen->numVisuals*sizeof(int));

    for (i=0; i < numVisuals; i++, pvis++) {
	/*
	** Look through all the server's visuals to see which match.
	*/
	pvis->vid = 0;
	pVisual = pScreen->visuals;
	for (j=0; j < pScreen->numVisuals; j++, pVisual++) {
	    if (pvis->class == pVisual->class &&
		pvis->bufferSize == pVisual->nplanes &&
		!used[j]) {
		int rBits, gBits, bBits, aBits;

		/* count bits per rgb */
		rBits = CountBits(pVisual->redMask);
		gBits = CountBits(pVisual->greenMask);
		bBits = CountBits(pVisual->blueMask);
		aBits = 0;
		if ((pvis->redSize == rBits) &&
		    (pvis->greenSize == gBits) &&
		    (pvis->blueSize == bBits) &&
		    (pvis->alphaSize == aBits)) {
		    /*
		    ** We'll consider this a match.
		    */
		    pvis->vid = pVisual->vid;
		    pvis->redMask = pVisual->redMask;
		    pvis->greenMask = pVisual->greenMask;
		    pvis->blueMask = pVisual->blueMask;
		    pvis->alphaMask = 0;
		    numMatchingVisuals++;
		    used[j] = 1;
		    break;
		}
	    }
	}
    }
    __glXFree(used);
    return numMatchingVisuals;
}
#endif

/*
** Destroy routine that gets called when a drawable is freed.  A drawable
** contains the ancillary buffers needed for rendering.
*/
static Bool DrawableGone(__GLXdrawablePrivate *glxPriv, XID xid)
{
    __GLXcontext *cx, *cx1;

    /*
    ** Use glxPriv->type to figure out what kind of drawable this is. Don't
    ** use glxPriv->pDraw->type because by the time this routine is called,
    ** the pDraw might already have been freed.
    */
    if (glxPriv->type == DRAWABLE_WINDOW) {
	/*
	** When a window is destroyed, notify all context bound to 
	** it, that there are no longer bound to anything.
	*/
	for (cx = glxPriv->glxc; cx; cx = cx1) {
	    cx1 = cx->nextPriv;
	    cx->pendingState |= __GLX_PENDING_DESTROY;
	}
    }

    /*
    ** set the size to 0, so that context that may still be using this 
    ** drawable not do anything harmful
    */
    glxPriv->xorigin = 0;
    glxPriv->yorigin = 0;
    glxPriv->width = 0;
    glxPriv->height = 0;

    __glXUnrefDrawablePrivate(glxPriv);

    return True;
}

/*
** This hook gets called when a window moves or changes size.
*/
static Bool PositionWindow(WindowPtr pWin, int x, int y)
{
    ScreenPtr pScreen;
    __GLXcontext *glxc;
    __GLXdrawablePrivate *glxPriv;
    Bool ret;

    /*
    ** Call wrapped position window routine
    */
    pScreen = pWin->drawable.pScreen;
    pScreen->PositionWindow =
	__glXActiveScreens[pScreen->myNum].WrappedPositionWindow;
    ret = (*pScreen->PositionWindow)(pWin, x, y);
    pScreen->PositionWindow = PositionWindow;

    /*
    ** Tell all contexts rendering into this window that the window size
    ** has changed.
    */
    glxPriv = (__GLXdrawablePrivate *) LookupIDByType(pWin->drawable.id,
						      __glXDrawableRes);
    if (glxPriv == NULL) {
	/*
	** This window is not being used by the OpenGL.
	*/
	return ret;
    }

    /*
    ** resize the drawable
    */
    /* first change the drawable size */
    if (__glXResizeDrawableBuffers(glxPriv) == GL_FALSE) {
	/* resize failed! */
	/* XXX: what can we possibly do here? */
	ret = False;
    }
    /* mark contexts as needing resize */
    glxc = NULL;
    for (glxc = glxPriv->glxc; glxc; glxc = glxc->nextPriv) {
	glxc->pendingState |= __GLX_PENDING_RESIZE;
    }

    return ret;
}

/*
** Wrap our own PositionWindow routine around the server's, so we can
** be notified when a window changes size
*/
static void wrapPositionWindow(int screen)
{
    ScreenPtr pScreen = screenInfo.screens[screen];

    __glXActiveScreens[screen].WrappedPositionWindow = pScreen->PositionWindow;
    pScreen->PositionWindow = PositionWindow;
}

void __glXScreenInit(GLint numscreens)
{
    GLint i,j;

    /*
    ** This alloc has to work or else the server might as well core dump.
    */
    __glXActiveScreens =
      (__GLXscreenInfo *) __glXMalloc(sizeof(__GLXscreenInfo) * numscreens);
    
    for (i=0; i < numscreens; i++) {
	/*
	** Probe each static screen to see which exists.
	*/
	for (j=0; j < __glXNumStaticScreens; j++) {
	    if ((*__glXScreens[j]->screenProbe)(i)) {
		__glXActiveScreens[i] = *__glXScreens[j];

#if 0
                /* we don't use this since matchVisuals doesn't allow alpha */
		__glXActiveScreens[i].numUsableVisuals =
		    matchVisuals(__glXActiveScreens[i].pGlxVisual,
			         __glXActiveScreens[i].numVisuals, i); 
#else
		__glXActiveScreens[i].numUsableVisuals = __glXActiveScreens[i].numVisuals;
#endif
		__glXActiveScreens[i].GLextensions = __glXStrdup(GLServerExtensions);
		__glXActiveScreens[i].GLXvendor = __glXStrdup(GLXServerVendorName);
		__glXActiveScreens[i].GLXversion = __glXStrdup(GLXServerVersion);
		__glXActiveScreens[i].GLXextensions = __glXStrdup(GLXServerExtensions);

#ifdef X11R5
		__glXDrawableRes = CreateNewResourceType(DrawableGone);
#else
		__glXDrawableRes = CreateNewResourceType((DeleteType)DrawableGone);
#endif
		wrapPositionWindow(i);
	    }
	}
    }
    __glXNumActiveScreens = numscreens;
}

void __glXScreenReset(void)
{
  int i;

  for (i = 0; i < __glXNumActiveScreens; i++) {
      __glXFree(__glXActiveScreens[i].GLXvendor);
      __glXFree(__glXActiveScreens[i].GLXversion);
      __glXFree(__glXActiveScreens[i].GLXextensions);
      __glXFree(__glXActiveScreens[i].GLextensions);
  }
  xfree(__glXActiveScreens);
  __glXActiveScreens = NULL;
  __glXNumActiveScreens = 0;
}
