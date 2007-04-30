/* 

Copyright 2007 Peter Hutterer <peter@cs.unisa.edu.au>

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the author shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the author.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _GEEXT_H_
#define _GEEXT_H_
#include <X11/extensions/geproto.h>

/* Returns the extension offset from the event */
#define GEEXT(ev) (((xGenericEvent*)(ev))->extension)

#define GEEXTIDX(ev) (GEEXT(ev) & 0x7F)
/* Typecast to generic event */
#define GEV(ev) ((xGenericEvent*)(ev))
/* True if mask is set for extension on window */
#define GEMaskIsSet(pWin, extension, mask) \
    ((pWin)->optional && \
     (pWin)->optional->geMasks && \
     ((pWin)->optional->geMasks->eventMasks[(extension) & 0x7F] & (mask)))

/* Returns first client */
#define GECLIENT(pWin) \
    (((pWin)->optional) ? (pWin)->optional->geMasks->geClients : NULL)

/* Interface for other extensions */
Mask GENextMask(int extension);
void GEWindowSetMask(ClientPtr pClient, WindowPtr pWin, int extension, Mask mask);
void GERegisterExtension(
        int extension,
        void (*ev_dispatch)(xGenericEvent* from, xGenericEvent* to));
void GEInitEvent(xGenericEvent* ev, int extension);


void GEExtensionInit(void);

#endif /* _GEEXT_H_ */
