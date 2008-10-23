/*
 * Copyright (c) 2007, 2008 Apple Inc.
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 *
 * Portions of this file are copied from Mesa's xf86glx.c,
 * which contains the following copyright:
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "dri.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLContext.h>

#include <GL/gl.h>
#include <GL/glxproto.h>
#include <windowstr.h>
#include <resource.h>
#include <GL/glxint.h>
#include <GL/glxtokens.h>
#include <scrnintstr.h>
#include <glxserver.h>
#include <glxscreens.h>
#include <glxdrawable.h>
#include <glxcontext.h>
#include <glxext.h>
#include <glxutil.h>
#include <glxscreens.h>
#include <GL/internal/glcore.h>

#include "capabilities.h"
#include "visualConfigs.h"

/* Based originally on code from indirect.c which was based on code from i830_dri.c. */
void setVisualConfigs(void) {
    int numConfigs = 0;
    __GLXvisualConfig *visualConfigs = NULL;
    void **visualPrivates = NULL;
    struct glCapabilities caps[1];
    int stereo, depth, aux, buffers, stencil, accum;
    int i = 0; 

    if(getGlCapabilities(caps)) {
	ErrorF("error from getGlCapabilities()!\n");
	return;
    }
    
    /*
      caps->stereo is 0 or 1, but we need at least 1 iteration of the loop, so we treat
      a true caps->stereo as 2.

      The depth size is 0 or 24.  Thus we do 2 iterations for that.

      caps->aux_buffers (when available/non-zero) result in 2 iterations instead of 1.

      caps->buffers indicates whether we have single or double buffering.
      
      2 iterations for stencil (on and off (with a stencil size of 8)).

      2 iterations for accum (on and off (with an accum color size of 16)).
     */

    numConfigs = (caps->stereo ? 2 : 1) * 2 * 
	(caps->aux_buffers ? 2 : 1) * (caps->buffers) * 2 * 2;

    visualConfigs = xcalloc(sizeof(*visualConfigs), numConfigs);

    if(NULL == visualConfigs) {
	ErrorF("xcalloc failure when allocating visualConfigs\n");
	return;
    }
    
    visualPrivates = xcalloc(sizeof(void *), numConfigs);

    if(NULL == visualPrivates) {
	ErrorF("xcalloc failure when allocating visualPrivates");
	xfree(visualConfigs);
	return;
    }

 
    i = 0; /* current buffer */
    for (stereo = 0; stereo < (caps->stereo ? 2 : 1); ++stereo) {
	for (depth = 0; depth < 2; ++depth) {
	    for (aux = 0; aux < (caps->aux_buffers ? 2 : 1); ++aux) {
		for (buffers = 0; buffers < caps->buffers; ++buffers) {
		    for (stencil = 0; stencil < 2; ++stencil) {
			for (accum = 0; accum < 2; ++accum) {
			    visualConfigs[i].vid = -1;
			    visualConfigs[i].class = -1;
			    visualConfigs[i].rgba = TRUE;
			    visualConfigs[i].redSize = -1;
			    visualConfigs[i].greenSize = -1;
			    visualConfigs[i].blueSize = -1;
			    visualConfigs[i].redMask = -1;
			    visualConfigs[i].greenMask = -1;
			    visualConfigs[i].blueMask = -1;
			    visualConfigs[i].alphaMask = 0;
			    if (accum) {
				visualConfigs[i].accumRedSize = 16;
				visualConfigs[i].accumGreenSize = 16;
				visualConfigs[i].accumBlueSize = 16;
				visualConfigs[i].accumAlphaSize = 16;
			    } else {
				visualConfigs[i].accumRedSize = 0;
				visualConfigs[i].accumGreenSize = 0;
				visualConfigs[i].accumBlueSize = 0;
				visualConfigs[i].accumAlphaSize = 0;
			    }
			    visualConfigs[i].doubleBuffer = buffers ? TRUE : FALSE;
			    visualConfigs[i].stereo = stereo ? TRUE : FALSE;
			    visualConfigs[i].bufferSize = -1;
			    
			    visualConfigs[i].depthSize = depth ? 24 : 0;
			    visualConfigs[i].stencilSize = stencil ? 8 : 0;
			    visualConfigs[i].auxBuffers = aux ? caps->aux_buffers : 0;
			    visualConfigs[i].level = 0;
			    visualConfigs[i].visualRating = GLX_NONE_EXT;
			    visualConfigs[i].transparentPixel = 0;
			    visualConfigs[i].transparentRed = 0;
			    visualConfigs[i].transparentGreen = 0;
			    visualConfigs[i].transparentBlue = 0;
			    visualConfigs[i].transparentAlpha = 0;
			    visualConfigs[i].transparentIndex = 0;
			    ++i;
			}
		    }
		}
	    }
	}
    }

    if (i != numConfigs) {
	ErrorF("numConfigs calculation error in setVisualConfigs!\n");
	abort();
    }

    GlxSetVisualConfigs(numConfigs, visualConfigs, visualPrivates);
}
