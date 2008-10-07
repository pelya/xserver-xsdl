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

extern BOOL enable_stereo;

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
    
    /* count num configs:
        2 stereo (on, off) (optional)
        2 Z buffer (0, 24 bit)
        2 AUX buffer (0, 2)
        2 buffers (single, double)
        2 stencil (0, 8 bit)
        2 accum (0, 64 bit)
        = 64 configs with stereo, or 32 without */

    numConfigs = ((enable_stereo && caps->stereo) ? 2 : 1) * 2 * 
	(caps->aux_buffers ? 2 : 1) * (caps->buffers) * 2 * 2;

    visualConfigs = xcalloc(sizeof(*visualConfigs), numConfigs);
    visualPrivates = xcalloc(sizeof(void *), numConfigs);

    if (NULL != visualConfigs) {
        i = 0; /* current buffer */
        for (stereo = 0; stereo < ((enable_stereo && caps->stereo) ? 2 : 1); ++stereo) {
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
                    
				visualConfigs[i].depthSize = depth? 24 : 0;
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
    }

    if (i != numConfigs) {
	ErrorF("numConfigs calculation error in setVisualConfigs!\n");
	abort();
    }

    GlxSetVisualConfigs(numConfigs, visualConfigs, visualPrivates);
}
