/*
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#ifndef _DRI2_H_
#define _DRI2_H_

#include <X11/extensions/dri2tokens.h>

typedef struct {
    unsigned int attachment;
    unsigned int name;
    unsigned int pitch;
    unsigned int cpp;
    unsigned int flags;
    void *driverPrivate;
} DRI2BufferRec, *DRI2BufferPtr;

typedef DRI2BufferPtr	(*DRI2CreateBuffersProcPtr)(DrawablePtr pDraw,
						    unsigned int *attachments,
						    int count);
typedef void		(*DRI2DestroyBuffersProcPtr)(DrawablePtr pDraw,
						     DRI2BufferPtr buffers,
						     int count);
typedef void		(*DRI2CopyRegionProcPtr)(DrawablePtr pDraw,
						 RegionPtr pRegion,
						 DRI2BufferPtr pDestBuffer,
						 DRI2BufferPtr pSrcBuffer);

typedef void		(*DRI2WaitProcPtr)(WindowPtr pWin,
					   unsigned int sequence);

typedef struct {
    unsigned int version;	/* Version of this struct */
    int fd;
    const char *driverName;
    const char *deviceName;

    DRI2CreateBuffersProcPtr	CreateBuffers;
    DRI2DestroyBuffersProcPtr	DestroyBuffers;
    DRI2CopyRegionProcPtr	CopyRegion;
    DRI2WaitProcPtr		Wait;

}  DRI2InfoRec, *DRI2InfoPtr;

Bool DRI2ScreenInit(ScreenPtr	pScreen,
		    DRI2InfoPtr info);

void DRI2CloseScreen(ScreenPtr pScreen);

Bool DRI2Connect(ScreenPtr pScreen,
		 unsigned int driverType,
		 int *fd,
		 const char **driverName,
		 const char **deviceName);

Bool DRI2Authenticate(ScreenPtr pScreen, drm_magic_t magic);

int DRI2CreateDrawable(DrawablePtr pDraw);

void DRI2DestroyDrawable(DrawablePtr pDraw);

DRI2BufferPtr DRI2GetBuffers(DrawablePtr pDraw,
			     int *width,
			     int *height,
			     unsigned int *attachments,
			     int count,
			     int *out_count);

int DRI2CopyRegion(DrawablePtr pDraw,
		   RegionPtr pRegion,
		   unsigned int dest,
		   unsigned int src);

#endif
