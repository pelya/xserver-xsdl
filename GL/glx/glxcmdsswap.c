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

#define NEED_REPLIES
#define FONT_PCF
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <string.h>
#include "glxserver.h"
#include "glxutil.h"
#include <GL/glxtokens.h>
#include <unpack.h>
#include <g_disptab.h>
#include <g_disptab_EXT.h>
#include <pixmapstr.h>
#include <windowstr.h>
#include "glxext.h"
#include "glapitable.h"
#include "glapi.h"
#include "glthread.h"
#include "dispatch.h"
#include "indirect_dispatch.h"
#include "indirect_table.h"
#include "indirect_util.h"


/************************************************************************/

/*
** Byteswapping versions of GLX commands.  In most cases they just swap
** the incoming arguments and then call the unswapped routine.  For commands
** that have replies, a separate swapping routine for the reply is provided;
** it is called at the end of the unswapped routine.
*/

int __glXDispSwap_CreateContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateContextReq *req = (xGLXCreateContextReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->context);
    __GLX_SWAP_INT(&req->visual);
    __GLX_SWAP_INT(&req->screen);
    __GLX_SWAP_INT(&req->shareList);

    return DoCreateContext( cl, req->context, req->shareList, req->visual,
			    req->screen, req->isDirect );
}

int __glXDispSwap_CreateNewContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateNewContextReq *req = (xGLXCreateNewContextReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->context);
    __GLX_SWAP_INT(&req->fbconfig);
    __GLX_SWAP_INT(&req->screen);
    __GLX_SWAP_INT(&req->renderType);
    __GLX_SWAP_INT(&req->shareList);

    return DoCreateContext( cl, req->context, req->shareList, req->fbconfig,
			    req->screen, req->isDirect );
}

int __glXDispSwap_CreateContextWithConfigSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateContextWithConfigSGIXReq *req =
	(xGLXCreateContextWithConfigSGIXReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->context);
    __GLX_SWAP_INT(&req->fbconfig);
    __GLX_SWAP_INT(&req->screen);
    __GLX_SWAP_INT(&req->renderType);
    __GLX_SWAP_INT(&req->shareList);

    return DoCreateContext( cl, req->context, req->shareList, req->fbconfig,
			    req->screen, req->isDirect );
}

int __glXDispSwap_DestroyContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyContextReq *req = (xGLXDestroyContextReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->context);

    return __glXDisp_DestroyContext(cl, pc);
}

int __glXDispSwap_MakeCurrent(__GLXclientState *cl, GLbyte *pc)
{
    xGLXMakeCurrentReq *req = (xGLXMakeCurrentReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->drawable);
    __GLX_SWAP_INT(&req->context);
    __GLX_SWAP_INT(&req->oldContextTag);

    return DoMakeCurrent( cl, req->drawable, req->drawable,
			  req->context, req->oldContextTag );
}

int __glXDispSwap_MakeContextCurrent(__GLXclientState *cl, GLbyte *pc)
{
    xGLXMakeContextCurrentReq *req = (xGLXMakeContextCurrentReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->drawable);
    __GLX_SWAP_INT(&req->readdrawable);
    __GLX_SWAP_INT(&req->context);
    __GLX_SWAP_INT(&req->oldContextTag);

    return DoMakeCurrent( cl, req->drawable, req->readdrawable,
			  req->context, req->oldContextTag );
}

int __glXDispSwap_MakeCurrentReadSGI(__GLXclientState *cl, GLbyte *pc)
{
    xGLXMakeCurrentReadSGIReq *req = (xGLXMakeCurrentReadSGIReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->drawable);
    __GLX_SWAP_INT(&req->readable);
    __GLX_SWAP_INT(&req->context);
    __GLX_SWAP_INT(&req->oldContextTag);

    return DoMakeCurrent( cl, req->drawable, req->readable,
			  req->context, req->oldContextTag );
}

int __glXDispSwap_IsDirect(__GLXclientState *cl, GLbyte *pc)
{
    xGLXIsDirectReq *req = (xGLXIsDirectReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->context);

    return __glXDisp_IsDirect(cl, pc);
}

int __glXDispSwap_QueryVersion(__GLXclientState *cl, GLbyte *pc)
{
    xGLXQueryVersionReq *req = (xGLXQueryVersionReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->majorVersion);
    __GLX_SWAP_INT(&req->minorVersion);

    return __glXDisp_QueryVersion(cl, pc);
}

int __glXDispSwap_WaitGL(__GLXclientState *cl, GLbyte *pc)
{
    xGLXWaitGLReq *req = (xGLXWaitGLReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);

    return __glXDisp_WaitGL(cl, pc);
}

int __glXDispSwap_WaitX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXWaitXReq *req = (xGLXWaitXReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);

    return __glXDisp_WaitX(cl, pc);
}

int __glXDispSwap_CopyContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCopyContextReq *req = (xGLXCopyContextReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->source);
    __GLX_SWAP_INT(&req->dest);
    __GLX_SWAP_INT(&req->mask);

    return __glXDisp_CopyContext(cl, pc);
}

int __glXDispSwap_GetVisualConfigs(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetVisualConfigsReq *req = (xGLXGetVisualConfigsReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_INT(&req->screen);
    return DoGetVisualConfigs( cl, req->screen, GL_TRUE );
}

int __glXDispSwap_GetFBConfigs(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetFBConfigsReq *req = (xGLXGetFBConfigsReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_INT(&req->screen);
    return DoGetFBConfigs( cl, req->screen, GL_TRUE );
}

int __glXDispSwap_GetFBConfigsSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetFBConfigsSGIXReq *req = (xGLXGetFBConfigsSGIXReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_INT(&req->screen);
    return DoGetFBConfigs( cl, req->screen, GL_TRUE );
}

int __glXDispSwap_CreateGLXPixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateGLXPixmapReq *req = (xGLXCreateGLXPixmapReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->screen);
    __GLX_SWAP_INT(&req->visual);
    __GLX_SWAP_INT(&req->pixmap);
    __GLX_SWAP_INT(&req->glxpixmap);

    return DoCreateGLXPixmap( cl, req->visual, req->screen,
			      req->pixmap, req->glxpixmap );
}

int __glXDispSwap_CreatePixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreatePixmapReq *req = (xGLXCreatePixmapReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->screen);
    __GLX_SWAP_INT(&req->fbconfig);
    __GLX_SWAP_INT(&req->pixmap);
    __GLX_SWAP_INT(&req->glxpixmap);

    return DoCreateGLXPixmap( cl, req->fbconfig, req->screen,
			      req->pixmap, req->glxpixmap );
}

int __glXDispSwap_CreateGLXPixmapWithConfigSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateGLXPixmapWithConfigSGIXReq *req = 
	(xGLXCreateGLXPixmapWithConfigSGIXReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->screen);
    __GLX_SWAP_INT(&req->fbconfig);
    __GLX_SWAP_INT(&req->pixmap);
    __GLX_SWAP_INT(&req->glxpixmap);

    return DoCreateGLXPixmap( cl, req->fbconfig, req->screen,
			      req->pixmap, req->glxpixmap );
}

int __glXDispSwap_DestroyGLXPixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyGLXPixmapReq *req = (xGLXDestroyGLXPixmapReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->glxpixmap);

    return __glXDisp_DestroyGLXPixmap(cl, pc);
}

int __glXDispSwap_DestroyPixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyGLXPixmapReq *req = (xGLXDestroyGLXPixmapReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->glxpixmap);

    return __glXDisp_DestroyGLXPixmap(cl, pc);
}

int __glXDispSwap_QueryContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXQueryContextReq *req = (xGLXQueryContextReq *) pc;    

    (void) req;

    return BadRequest;    
}

int __glXDispSwap_CreatePbuffer(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreatePbufferReq *req = (xGLXCreatePbufferReq *) pc;    

    (void) req;

    return BadRequest;    
}

int __glXDispSwap_DestroyPbuffer(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyPbufferReq *req = (xGLXDestroyPbufferReq *) req;

    return BadRequest;
}

int __glXDispSwap_ChangeDrawableAttributes(__GLXclientState *cl, GLbyte *pc)
{
    xGLXChangeDrawableAttributesReq *req =
	(xGLXChangeDrawableAttributesReq *) req;

    return BadRequest;
}

int __glXDispSwap_CreateWindow(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateWindowReq *req = (xGLXCreateWindowReq *) pc;

    (void) req;

    return BadRequest;
}

int __glXDispSwap_DestroyWindow(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyWindowReq *req = (xGLXDestroyWindowReq *) pc;

    (void) req;

    return BadRequest;
}

int __glXDispSwap_SwapBuffers(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSwapBuffersReq *req = (xGLXSwapBuffersReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);
    __GLX_SWAP_INT(&req->drawable);

    return __glXDisp_SwapBuffers(cl, pc);
}

int __glXDispSwap_UseXFont(__GLXclientState *cl, GLbyte *pc)
{
    xGLXUseXFontReq *req = (xGLXUseXFontReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);
    __GLX_SWAP_INT(&req->font);
    __GLX_SWAP_INT(&req->first);
    __GLX_SWAP_INT(&req->count);
    __GLX_SWAP_INT(&req->listBase);

    return __glXDisp_UseXFont(cl, pc);
}


int __glXDispSwap_QueryExtensionsString(__GLXclientState *cl, GLbyte *pc)
{
    xGLXQueryExtensionsStringReq *req = (xGLXQueryExtensionsStringReq *)pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->screen);

    return __glXDisp_QueryExtensionsString(cl, pc);
}

int __glXDispSwap_QueryServerString(__GLXclientState *cl, GLbyte *pc)
{
    xGLXQueryServerStringReq *req = (xGLXQueryServerStringReq *)pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->screen);
    __GLX_SWAP_INT(&req->name);

    return __glXDisp_QueryServerString(cl, pc);
}

int __glXDispSwap_ClientInfo(__GLXclientState *cl, GLbyte *pc)
{
    xGLXClientInfoReq *req = (xGLXClientInfoReq *)pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->major);
    __GLX_SWAP_INT(&req->minor);
    __GLX_SWAP_INT(&req->numbytes);

    return __glXDisp_ClientInfo(cl, pc);
}

int __glXDispSwap_QueryContextInfoEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXQueryContextInfoEXTReq *req = (xGLXQueryContextInfoEXTReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->context);

    return __glXDisp_QueryContextInfoEXT(cl, pc);
}

int __glXDispSwap_BindTexImageEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXDrawable		 *drawId;
    int			 *buffer;
    
    __GLX_DECLARE_SWAP_VARIABLES;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = ((GLXDrawable *) (pc));
    buffer = ((int *)	      (pc + 4));
    
    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);
    __GLX_SWAP_INT(drawId);
    __GLX_SWAP_INT(buffer);

    return __glXDisp_BindTexImageEXT(cl, (GLbyte *)pc);
}

int __glXDispSwap_ReleaseTexImageEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXDrawable		 *drawId;
    int			 *buffer;
    
    __GLX_DECLARE_SWAP_VARIABLES;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = ((GLXDrawable *) (pc));
    buffer = ((int *)	      (pc + 4));
    
    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);
    __GLX_SWAP_INT(drawId);
    __GLX_SWAP_INT(buffer);

    return __glXDisp_ReleaseTexImageEXT(cl, (GLbyte *)pc);
}

int __glXDispSwap_CopySubBufferMESA(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXDrawable		 *drawId;
    int			 *buffer;

    (void) drawId;
    (void) buffer;

    __GLX_DECLARE_SWAP_VARIABLES;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);
    __GLX_SWAP_INT(pc);
    __GLX_SWAP_INT(pc + 4);
    __GLX_SWAP_INT(pc + 8);
    __GLX_SWAP_INT(pc + 12);
    __GLX_SWAP_INT(pc + 16);

    return __glXDisp_CopySubBufferMESA(cl, pc);

}

int __glXDispSwap_GetDrawableAttributesSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateWithReplyReq *req = (xGLXVendorPrivateWithReplyReq *)pc;
    CARD32 *data;
    
    __GLX_DECLARE_SWAP_VARIABLES;

    data = (CARD32 *) (req + 1);
    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);
    __GLX_SWAP_INT(data);

    return __glXDisp_GetDrawableAttributesSGIX(cl, pc);
}

int __glXDispSwap_GetDrawableAttributes(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetDrawableAttributesReq *req = (xGLXGetDrawableAttributesReq *)pc;
    
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->drawable);

    return __glXDisp_GetDrawableAttributes(cl, pc);
}


/************************************************************************/

/*
** Swap replies.
*/

void __glXSwapMakeCurrentReply(ClientPtr client, xGLXMakeCurrentReply *reply)
{
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_SWAP_SHORT(&reply->sequenceNumber);
    __GLX_SWAP_INT(&reply->length);
    __GLX_SWAP_INT(&reply->contextTag);
    WriteToClient(client, sz_xGLXMakeCurrentReply, (char *)reply);
}

void __glXSwapIsDirectReply(ClientPtr client, xGLXIsDirectReply *reply)
{
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_SWAP_SHORT(&reply->sequenceNumber);
    __GLX_SWAP_INT(&reply->length);
    WriteToClient(client, sz_xGLXIsDirectReply, (char *)reply);
}

void __glXSwapQueryVersionReply(ClientPtr client, xGLXQueryVersionReply *reply)
{
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_SWAP_SHORT(&reply->sequenceNumber);
    __GLX_SWAP_INT(&reply->length);
    __GLX_SWAP_INT(&reply->majorVersion);
    __GLX_SWAP_INT(&reply->minorVersion);
    WriteToClient(client, sz_xGLXQueryVersionReply, (char *)reply);
}

void glxSwapQueryExtensionsStringReply(ClientPtr client,
				       xGLXQueryExtensionsStringReply *reply, char *buf)
{
    int length = reply->length;
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;
    __GLX_SWAP_SHORT(&reply->sequenceNumber);
    __GLX_SWAP_INT(&reply->length);
    __GLX_SWAP_INT(&reply->n);
    WriteToClient(client, sz_xGLXQueryExtensionsStringReply, (char *)reply);
    __GLX_SWAP_INT_ARRAY((int *)buf, length);
    WriteToClient(client, length << 2, buf);
}

void glxSwapQueryServerStringReply(ClientPtr client,
				   xGLXQueryServerStringReply *reply, char *buf)
{
    int length = reply->length;
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_SWAP_SHORT(&reply->sequenceNumber);
    __GLX_SWAP_INT(&reply->length);
    __GLX_SWAP_INT(&reply->n);
    WriteToClient(client, sz_xGLXQueryServerStringReply, (char *)reply);
    /** no swap is needed for an array of chars **/
    /* __GLX_SWAP_INT_ARRAY((int *)buf, length); */
    WriteToClient(client, length << 2, buf);
}

void __glXSwapQueryContextInfoEXTReply(ClientPtr client, xGLXQueryContextInfoEXTReply *reply, int *buf)
{
    int length = reply->length;
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;
    __GLX_SWAP_SHORT(&reply->sequenceNumber);
    __GLX_SWAP_INT(&reply->length);
    __GLX_SWAP_INT(&reply->n);
    WriteToClient(client, sz_xGLXQueryContextInfoEXTReply, (char *)reply);
    __GLX_SWAP_INT_ARRAY((int *)buf, length);
    WriteToClient(client, length << 2, (char *)buf);
}

void __glXSwapGetDrawableAttributesReply(ClientPtr client,
					 xGLXGetDrawableAttributesReply *reply, CARD32 *buf)
{
    int length = reply->length;
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;
    __GLX_SWAP_SHORT(&reply->sequenceNumber);
    __GLX_SWAP_INT(&reply->length);
    __GLX_SWAP_INT(&reply->numAttribs);
    WriteToClient(client, sz_xGLXGetDrawableAttributesReply, (char *)reply);
    __GLX_SWAP_INT_ARRAY((int *)buf, length);
    WriteToClient(client, length << 2, (char *)buf);
}

/************************************************************************/

/*
** Render and Renderlarge are not in the GLX API.  They are used by the GLX
** client library to send batches of GL rendering commands.
*/

int __glXDispSwap_Render(__GLXclientState *cl, GLbyte *pc)
{
    xGLXRenderReq *req;
    ClientPtr client= cl->client;
    int left, cmdlen, error;
    int commandsDone;
    CARD16 opcode;
    __GLXrenderHeader *hdr;
    __GLXcontext *cx;
    __GLX_DECLARE_SWAP_VARIABLES;

    /*
    ** NOTE: much of this code also appears in the nonswapping version of this
    ** routine, __glXRender().  Any changes made here should also be
    ** duplicated there.
    */
    
    req = (xGLXRenderReq *) pc;
    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);

    cx = __glXForceCurrent(cl, req->contextTag, &error);
    if (!cx) {
	return error;
    }

    commandsDone = 0;
    pc += sz_xGLXRenderReq;
    left = (req->length << 2) - sz_xGLXRenderReq;
    while (left > 0) {
        __GLXrenderSizeData entry;
        int extra;
	__GLXdispatchRenderProcPtr proc;
	int err;

	/*
	** Verify that the header length and the overall length agree.
	** Also, each command must be word aligned.
	*/
	hdr = (__GLXrenderHeader *) pc;
	__GLX_SWAP_SHORT(&hdr->length);
	__GLX_SWAP_SHORT(&hdr->opcode);
	cmdlen = hdr->length;
	opcode = hdr->opcode;

	err = __glXGetProtocolSizeData(& Render_dispatch_info, opcode, & entry);
	proc = (__GLXdispatchRenderProcPtr)
	  __glXGetProtocolDecodeFunction(& Render_dispatch_info, opcode, 1);

	if ((err < 0) || (proc == NULL)) {
	    client->errorValue = commandsDone;
	    return __glXError(GLXBadRenderRequest);
	}

        if (entry.varsize) {
            /* variable size command */
            extra = (*entry.varsize)(pc + __GLX_RENDER_HDR_SIZE, True);
            if (extra < 0) {
                extra = 0;
            }
            if (cmdlen != __GLX_PAD(entry.bytes + extra)) {
                return BadLength;
            }
        } else {
            /* constant size command */
            if (cmdlen != __GLX_PAD(entry.bytes)) {
                return BadLength;
            }
        }
	if (left < cmdlen) {
	    return BadLength;
	}

	/*
	** Skip over the header and execute the command.  We allow the
	** caller to trash the command memory.  This is useful especially
	** for things that require double alignment - they can just shift
	** the data towards lower memory (trashing the header) by 4 bytes
	** and achieve the required alignment.
	*/
	(*proc)(pc + __GLX_RENDER_HDR_SIZE);
	pc += cmdlen;
	left -= cmdlen;
	commandsDone++;
    }
    __GLX_NOTE_UNFLUSHED_CMDS(cx);
    return Success;
}

/*
** Execute a large rendering request (one that spans multiple X requests).
*/
int __glXDispSwap_RenderLarge(__GLXclientState *cl, GLbyte *pc)
{
    xGLXRenderLargeReq *req;
    ClientPtr client= cl->client;
    size_t dataBytes;
    __GLXrenderLargeHeader *hdr;
    __GLXcontext *cx;
    int error;
    CARD16 opcode;
    __GLX_DECLARE_SWAP_VARIABLES;

    /*
    ** NOTE: much of this code also appears in the nonswapping version of this
    ** routine, __glXRenderLarge().  Any changes made here should also be
    ** duplicated there.
    */
    
    req = (xGLXRenderLargeReq *) pc;
    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->contextTag);
    __GLX_SWAP_INT(&req->dataBytes);
    __GLX_SWAP_SHORT(&req->requestNumber);
    __GLX_SWAP_SHORT(&req->requestTotal);
    cx = __glXForceCurrent(cl, req->contextTag, &error);
    if (!cx) {
	/* Reset in case this isn't 1st request. */
	__glXResetLargeCommandStatus(cl);
	return error;
    }
    dataBytes = req->dataBytes;

    /*
    ** Check the request length.
    */
    if ((req->length << 2) != __GLX_PAD(dataBytes) + sz_xGLXRenderLargeReq) {
	client->errorValue = req->length;
	/* Reset in case this isn't 1st request. */
	__glXResetLargeCommandStatus(cl);
	return BadLength;
    }
    pc += sz_xGLXRenderLargeReq;
    
    if (cl->largeCmdRequestsSoFar == 0) {
	__GLXrenderSizeData entry;
	int extra;
	size_t cmdlen;
	int err;

	/*
	** This is the first request of a multi request command.
	** Make enough space in the buffer, then copy the entire request.
	*/
	if (req->requestNumber != 1) {
	    client->errorValue = req->requestNumber;
	    return __glXError(GLXBadLargeRequest);
	}
	hdr = (__GLXrenderLargeHeader *) pc;
	__GLX_SWAP_INT(&hdr->length);
	__GLX_SWAP_INT(&hdr->opcode);
	cmdlen = hdr->length;
	opcode = hdr->opcode;

	err = __glXGetProtocolSizeData(& Render_dispatch_info, opcode, & entry);
	if (err < 0) {
	    client->errorValue = opcode;
	    return __glXError(GLXBadLargeRequest);
	}

	if (entry.varsize) {
	    /*
	    ** If it's a variable-size command (a command whose length must
	    ** be computed from its parameters), all the parameters needed
	    ** will be in the 1st request, so it's okay to do this.
	    */
	    extra = (*entry.varsize)(pc + __GLX_RENDER_LARGE_HDR_SIZE, True);
	    if (extra < 0) {
		extra = 0;
	    }
	    /* large command's header is 4 bytes longer, so add 4 */
	    if (cmdlen != __GLX_PAD(entry.bytes + 4 + extra)) {
		return BadLength;
	    }
	} else {
	    /* constant size command */
	    if (cmdlen != __GLX_PAD(entry.bytes + 4)) {
		return BadLength;
	    }
	}
	/*
	** Make enough space in the buffer, then copy the entire request.
	*/
	if (cl->largeCmdBufSize < cmdlen) {
	    if (!cl->largeCmdBuf) {
		cl->largeCmdBuf = (GLbyte *) xalloc(cmdlen);
	    } else {
		cl->largeCmdBuf = (GLbyte *) xrealloc(cl->largeCmdBuf, cmdlen);
	    }
	    if (!cl->largeCmdBuf) {
		return BadAlloc;
	    }
	    cl->largeCmdBufSize = cmdlen;
	}
	memcpy(cl->largeCmdBuf, pc, dataBytes);

	cl->largeCmdBytesSoFar = dataBytes;
	cl->largeCmdBytesTotal = cmdlen;
	cl->largeCmdRequestsSoFar = 1;
	cl->largeCmdRequestsTotal = req->requestTotal;
	return Success;
	
    } else {
	/*
	** We are receiving subsequent (i.e. not the first) requests of a
	** multi request command.
	*/

	/*
	** Check the request number and the total request count.
	*/
	if (req->requestNumber != cl->largeCmdRequestsSoFar + 1) {
	    client->errorValue = req->requestNumber;
	    __glXResetLargeCommandStatus(cl);
	    return __glXError(GLXBadLargeRequest);
	}
	if (req->requestTotal != cl->largeCmdRequestsTotal) {
	    client->errorValue = req->requestTotal;
	    __glXResetLargeCommandStatus(cl);
	    return __glXError(GLXBadLargeRequest);
	}

	/*
	** Check that we didn't get too much data.
	*/
	if ((cl->largeCmdBytesSoFar + dataBytes) > cl->largeCmdBytesTotal) {
	    client->errorValue = dataBytes;
	    __glXResetLargeCommandStatus(cl);
	    return __glXError(GLXBadLargeRequest);
	}
	memcpy(cl->largeCmdBuf + cl->largeCmdBytesSoFar, pc, dataBytes);
	cl->largeCmdBytesSoFar += dataBytes;
	cl->largeCmdRequestsSoFar++;

	if (req->requestNumber == cl->largeCmdRequestsTotal) {
	    __GLXdispatchRenderProcPtr proc;

	    /*
	    ** This is the last request; it must have enough bytes to complete
	    ** the command.
	    */
	    /* NOTE: the two pad macros have been added below; they are needed
	    ** because the client library pads the total byte count, but not
	    ** the per-request byte counts.  The Protocol Encoding says the
	    ** total byte count should not be padded, so a proposal will be 
	    ** made to the ARB to relax the padding constraint on the total 
	    ** byte count, thus preserving backward compatibility.  Meanwhile, 
	    ** the padding done below fixes a bug that did not allow
	    ** large commands of odd sizes to be accepted by the server.
	    */
	    if (__GLX_PAD(cl->largeCmdBytesSoFar) !=
		__GLX_PAD(cl->largeCmdBytesTotal)) {
		client->errorValue = dataBytes;
		__glXResetLargeCommandStatus(cl);
		return __glXError(GLXBadLargeRequest);
	    }
	    hdr = (__GLXrenderLargeHeader *) cl->largeCmdBuf;
	    /*
	    ** The opcode and length field in the header had already been
	    ** swapped when the first request was received.
	    */

	    /*
	    ** Use the opcode to index into the procedure table.
	    */
	    opcode = hdr->opcode;

	    proc = (__GLXdispatchRenderProcPtr)
	      __glXGetProtocolDecodeFunction(& Render_dispatch_info, opcode, 1);
	    if (proc == NULL) {
		client->errorValue = opcode;
		return __glXError(GLXBadLargeRequest);
	    }

	    /*
	    ** Skip over the header and execute the command.
	    */
	    (*proc)(cl->largeCmdBuf + __GLX_RENDER_LARGE_HDR_SIZE);
	    __GLX_NOTE_UNFLUSHED_CMDS(cx);

	    /*
	    ** Reset for the next RenderLarge series.
	    */
	    __glXResetLargeCommandStatus(cl);
	} else {
	    /*
	    ** This is neither the first nor the last request.
	    */
	}
	return Success;
    }
}

/************************************************************************/

/*
** No support is provided for the vendor-private requests other than
** allocating these entry points in the dispatch table.
*/

int __glXDispSwap_VendorPrivate(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req;
    GLint vendorcode;
    __GLXdispatchVendorPrivProcPtr proc;

    __GLX_DECLARE_SWAP_VARIABLES;

    req = (xGLXVendorPrivateReq *) pc;
    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->vendorCode);

    vendorcode = req->vendorCode;

    proc = (__GLXdispatchVendorPrivProcPtr)
      __glXGetProtocolDecodeFunction(& VendorPriv_dispatch_info,
				     vendorcode, 1);
    if (proc != NULL) {
	(*proc)(cl, (GLbyte*)req);
	return Success;
    }

    cl->client->errorValue = req->vendorCode;
    return __glXError(GLXUnsupportedPrivateRequest);
}


int __glXDispSwap_VendorPrivateWithReply(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateWithReplyReq *req;
    GLint vendorcode;
    __GLXdispatchVendorPrivProcPtr proc;

    __GLX_DECLARE_SWAP_VARIABLES;

    req = (xGLXVendorPrivateWithReplyReq *) pc;
    __GLX_SWAP_SHORT(&req->length);
    __GLX_SWAP_INT(&req->vendorCode);

    vendorcode = req->vendorCode;

    proc = (__GLXdispatchVendorPrivProcPtr)
      __glXGetProtocolDecodeFunction(& VendorPriv_dispatch_info,
				     vendorcode, 1);
    if (proc != NULL) {
	return (*proc)(cl, (GLbyte*)req);
    }

    cl->client->errorValue = req->vendorCode;
    return __glXError(GLXUnsupportedPrivateRequest);
}
