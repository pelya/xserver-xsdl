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
    struct glCapabilities caps;
    struct glCapabilitiesConfig *conf = NULL;
    int stereo, depth, aux, buffers, stencil, accum, color;
    int i = 0; 
   
    if(getGlCapabilities(&caps)) {
	ErrorF("error from getGlCapabilities()!\n");
	return;
    }
    
    /*
      conf->stereo is 0 or 1, but we need at least 1 iteration of the loop, 
      so we treat a true conf->stereo as 2.

      The depth size is 0 or 24.  Thus we do 2 iterations for that.

      conf->aux_buffers (when available/non-zero) result in 2 iterations instead of 1.

      conf->buffers indicates whether we have single or double buffering.
     
      conf->total_stencil_bit_depths
       
      conf->total_color_buffers indicates the RGB/RGBA color depths.
      
      conf->total_accum_buffers iterations for accum (with at least 1 if equal to 0) 
     */

    assert(NULL != caps.configurations);
    conf = caps.configurations;
  
    numConfigs = 0;

    for(conf = caps.configurations; conf; conf = conf->next) {
	if(conf->total_color_buffers <= 0)
	    continue;

	numConfigs += (conf->stereo ? 2 : 1) 
	    * 2 /*depth*/ 
	    * (conf->aux_buffers ? 2 : 1) 
	    * conf->buffers
	    * ((conf->total_stencil_bit_depths > 0) ? conf->total_stencil_bit_depths : 1)
	    * conf->total_color_buffers
	    * ((conf->total_accum_buffers > 0) ? conf->total_accum_buffers : 1);
    }

    visualConfigs = xcalloc(sizeof(*visualConfigs), numConfigs);

    if(NULL == visualConfigs) {
	ErrorF("xcalloc failure when allocating visualConfigs\n");
	freeGlCapabilities(&caps);
	return;
    }
    
    visualPrivates = xcalloc(sizeof(void *), numConfigs);

    if(NULL == visualPrivates) {
	ErrorF("xcalloc failure when allocating visualPrivates");
	freeGlCapabilities(&caps);
	xfree(visualConfigs);
	return;
    }
    
    i = 0; /* current buffer */
    for(conf = caps.configurations; conf; conf = conf->next) {
	for(stereo = 0; stereo < (conf->stereo ? 2 : 1); ++stereo) {
	    for(depth = 0; depth < 2; ++depth) {
		for(aux = 0; aux < (conf->aux_buffers ? 2 : 1); ++aux) {
		    for(buffers = 0; buffers < conf->buffers; ++buffers) {
			for(stencil = 0; stencil < ((conf->total_stencil_bit_depths > 0) ? 
						    conf->total_stencil_bit_depths : 1); ++stencil) {
			    for(color = 0; color < conf->total_color_buffers; ++color) {
				for(accum = 0; accum < ((conf->total_accum_buffers > 0) ?
							conf->total_accum_buffers : 1); ++accum) {
				    visualConfigs[i].vid = -1;
				    visualConfigs[i].class = -1;
		     
				    visualConfigs[i].rgba = true;
				    visualConfigs[i].redSize = conf->color_buffers[color].r;
				    visualConfigs[i].greenSize = conf->color_buffers[color].g;
				    visualConfigs[i].blueSize = conf->color_buffers[color].b;
				    visualConfigs[i].alphaSize = conf->color_buffers[color].a;
				
				    visualConfigs[i].redMask = -1;
				    visualConfigs[i].greenMask = -1;
				    visualConfigs[i].blueMask = -1;
				    visualConfigs[i].alphaMask = -1;

				    if(conf->total_accum_buffers > 0) {
					visualConfigs[i].accumRedSize = conf->accum_buffers[accum].r;
					visualConfigs[i].accumGreenSize = conf->accum_buffers[accum].g;
					visualConfigs[i].accumBlueSize = conf->accum_buffers[accum].b;
					if(GLCAPS_COLOR_BUF_INVALID_VALUE != conf->accum_buffers[accum].a) {
					    visualConfigs[i].accumAlphaSize = conf->accum_buffers[accum].a;
					} else {
					    visualConfigs[i].accumAlphaSize = 0;
					}
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
				    
				    if(conf->total_stencil_bit_depths > 0) {
					visualConfigs[i].stencilSize = conf->stencil_bit_depths[stencil];
				    } else {
					visualConfigs[i].stencilSize = 0;
				    }
				    visualConfigs[i].auxBuffers = aux ? conf->aux_buffers : 0;
				    visualConfigs[i].level = 0;

				    if(conf->accelerated) {
					visualConfigs[i].visualRating = GLX_NONE;
				    } else {
					visualConfigs[i].visualRating = GLX_SLOW_VISUAL_EXT;
				    }

				    visualConfigs[i].transparentPixel = GLX_NONE;
				    visualConfigs[i].transparentRed = GLX_NONE;
				    visualConfigs[i].transparentGreen = GLX_NONE;
				    visualConfigs[i].transparentBlue = GLX_NONE;
				    visualConfigs[i].transparentAlpha = GLX_NONE;
				    visualConfigs[i].transparentIndex = GLX_NONE;

				    /*
				      TODO possibly handle:
				      multiSampleSize;
				      nMultiSampleBuffers;
				      visualSelectGroup;
				    */

				    ++i;
				}
			    }
			}
		    }
		}
	    }
	}
    }

    if (i != numConfigs) {
	ErrorF("numConfigs calculation error in setVisualConfigs!  numConfigs is %d  i is %d\n", numConfigs, i);
	abort();
    }

    freeGlCapabilities(&caps);

    GlxSetVisualConfigs(numConfigs, visualConfigs, visualPrivates);
}
