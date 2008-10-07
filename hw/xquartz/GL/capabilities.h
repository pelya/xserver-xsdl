#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include <stdbool.h>

struct glCapabilities {
    int stereo;
    int aux_buffers;
    int buffers;
    /*TODO handle STENCIL and ACCUM*/
};

bool getGlCapabilities(struct glCapabilities *cap);

#endif
