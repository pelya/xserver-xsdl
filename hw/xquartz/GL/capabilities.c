/*
 * Copyright (c) 2008 Apple Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#include <ApplicationServices/ApplicationServices.h>

#include "capabilities.h"

//#define DIAGNOSTIC 0

static void handleBufferModes(struct glCapabilities *cap, GLint bufferModes) {
    if(bufferModes & kCGLStereoscopicBit) {
	cap->stereo = true;
    }

    if(bufferModes & kCGLDoubleBufferBit) {
	cap->buffers = 2;
    } else {
	cap->buffers = 1;
    }
}

static void initCapabilities(struct glCapabilities *cap) {
    cap->stereo = cap->buffers = cap->aux_buffers = 0;
}

enum {
    MAX_DISPLAYS = 32
};

/*Return true if an error occured. */
bool getGlCapabilities(struct glCapabilities *cap) {
    CGDirectDisplayID dspys[MAX_DISPLAYS];
    CGDisplayErr err;
    CGOpenGLDisplayMask displayMask;
    CGDisplayCount i, displayCount = 0;

    initCapabilities(cap);
    
    err = CGGetActiveDisplayList(MAX_DISPLAYS, dspys, &displayCount);
    if(err) {
#ifdef DIAGNOSTIC
	fprintf(stderr, "CGGetActiveDisplayList %s\n", CGLErrorString (err));
#endif
	return true;
    }
 
    for(i = 0; i < displayCount; ++i) {
        displayMask = CGDisplayIDToOpenGLDisplayMask(dspys[i]);
       
	CGLRendererInfoObj info;
	GLint numRenderers = 0, r, accelerated = 0, flags = 0, aux = 0;
    
	err = CGLQueryRendererInfo (displayMask, &info, &numRenderers);
        if(!err) {
            CGLDescribeRenderer (info, 0, kCGLRPRendererCount, &numRenderers);
            for(r = 0; r < numRenderers; ++r) {
                // find accelerated renderer (assume only one)
                CGLDescribeRenderer (info, r, kCGLRPAccelerated, &accelerated);
                if(accelerated) {
                    err = CGLDescribeRenderer(info, r, kCGLRPBufferModes, &flags);
		    if(err) {
			CGLDestroyRendererInfo(info);
			return true;
		    }

		    handleBufferModes(cap, flags);

		    err = CGLDescribeRenderer(info, r, kCGLRPMaxAuxBuffers, &aux);
		    if(err) {
			CGLDestroyRendererInfo(info);
			return true;
		    }

		    cap->aux_buffers = aux;
                }
            }
	    CGLDestroyRendererInfo(info);
	}
    }

    /* No error occured.  We are done. */
    return false;
}
