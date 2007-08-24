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
#include <assert.h>

#include "glxserver.h"
#include <GL/glxtokens.h>
#include <unpack.h>
#include "g_disptab.h"
#include <pixmapstr.h>
#include <windowstr.h>
#include "glxutil.h"
#include "glxext.h"
#include "glcontextmodes.h"
#include "glapitable.h"
#include "glapi.h"
#include "glthread.h"
#include "dispatch.h"
#include "indirect_dispatch.h"
#include "indirect_table.h"
#include "indirect_util.h"

/************************************************************************/

void
GlxSetRenderTables (struct _glapi_table *table)
{
    _glapi_set_dispatch (table);
}


/************************************************************************/

void
__glXContextDestroy(__GLXcontext *context)
{
    __glXFlushContextCache();
}


static void __glXdirectContextDestroy(__GLXcontext *context)
{
    __glXContextDestroy(context);
    xfree(context);
}

static __GLXcontext *__glXdirectContextCreate(__GLXscreen *screen,
					      __GLcontextModes *modes,
					      __GLXcontext *shareContext)
{
    __GLXcontext *context;

    context = xalloc (sizeof (__GLXcontext));
    if (context == NULL)
	return NULL;

    memset(context, 0, sizeof *context);
    context->destroy = __glXdirectContextDestroy;

    return context;
}

/**
 * Create a GL context with the given properties.  This routine is used
 * to implement \c glXCreateContext, \c glXCreateNewContext, and
 * \c glXCreateContextWithConfigSGIX.  This works becuase of the hack way
 * that GLXFBConfigs are implemented.  Basically, the FBConfigID is the
 * same as the VisualID.
 */

int DoCreateContext(__GLXclientState *cl, GLXContextID gcId,
		    GLXContextID shareList, VisualID visual,
		    GLuint screen, GLboolean isDirect)
{
    ClientPtr client = cl->client;
    VisualPtr pVisual;
    ScreenPtr pScreen;
    __GLXcontext *glxc, *shareglxc;
    __GLcontextModes *modes;
    __GLXscreen *pGlxScreen;
    GLint i;

    LEGAL_NEW_RESOURCE(gcId, client);
    
    /*
    ** Check if screen exists.
    */
    if (screen >= screenInfo.numScreens) {
	client->errorValue = screen;
	return BadValue;
    }
    pScreen = screenInfo.screens[screen];
    pGlxScreen = __glXActiveScreens[screen];
    
    /*
    ** Check if the visual ID is valid for this screen.
    */
    pVisual = pScreen->visuals;
    for (i = 0; i < pScreen->numVisuals; i++, pVisual++) {
	if (pVisual->vid == visual) {
	    break;
	}
    }
    if (i == pScreen->numVisuals) {
	client->errorValue = visual;
	return BadValue;
    }

    /*
    ** Get configuration of the visual.  This assumes that the
    ** glXActiveScreens structure contains visual configurations only for the
    ** subset of Visuals that are supported by this implementation of the
    ** OpenGL.
    */

    modes = _gl_context_modes_find_visual( pGlxScreen->modes, visual );
    if (modes == NULL) {
	/*
	** Visual not support on this screen by this OpenGL implementation.
	*/
	client->errorValue = visual;
	return BadValue;
    }

    /*
    ** Find the display list space that we want to share.  
    **
    ** NOTE: In a multithreaded X server, we would need to keep a reference
    ** count for each display list so that if one client detroyed a list that 
    ** another client was using, the list would not really be freed until it 
    ** was no longer in use.  Since this sample implementation has no support 
    ** for multithreaded servers, we don't do this.  
    */
    if (shareList == None) {
	shareglxc = 0;
    } else {
	shareglxc = (__GLXcontext *) LookupIDByType(shareList, __glXContextRes);
	if (!shareglxc) {
	    client->errorValue = shareList;
	    return __glXError(GLXBadContext);
	}
	if (shareglxc->isDirect) {
	    /*
	    ** NOTE: no support for sharing display lists between direct
	    ** contexts, even if they are in the same address space.
	    */
#if 0
            /* Disabling this code seems to allow shared display lists
             * and texture objects to work.  We'll leave it disabled for now.
             */
	    client->errorValue = shareList;
	    return BadMatch;
#endif
	} else {
	    /*
	    ** Create an indirect context regardless of what the client asked
	    ** for; this way we can share display list space with shareList.
	    */
	    isDirect = GL_FALSE;
	}
    }

    /*
    ** Allocate memory for the new context
    */
    if (!isDirect)
	glxc = pGlxScreen->createContext(pGlxScreen, modes, shareglxc);
    else
	glxc = __glXdirectContextCreate(pGlxScreen, modes, shareglxc);
    if (!glxc) {
	return BadAlloc;
    }

    /*
    ** Initially, setup the part of the context that could be used by
    ** a GL core that needs windowing information (e.g., Mesa).
    */
    glxc->pScreen = pScreen;
    glxc->pGlxScreen = pGlxScreen;
    glxc->pVisual = pVisual;
    glxc->modes = modes;

    /*
    ** Register this context as a resource.
    */
    if (!AddResource(gcId, __glXContextRes, (pointer)glxc)) {
	(*glxc->destroy)(glxc);
	client->errorValue = gcId;
	return BadAlloc;
    }
    
    /*
    ** Finally, now that everything is working, setup the rest of the
    ** context.
    */
    glxc->id = gcId;
    glxc->share_id = shareList;
    glxc->idExists = GL_TRUE;
    glxc->isCurrent = GL_FALSE;
    glxc->isDirect = isDirect;
    glxc->renderMode = GL_RENDER;

    return Success;
}


int __glXDisp_CreateContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateContextReq *req = (xGLXCreateContextReq *) pc;
    return DoCreateContext( cl, req->context, req->shareList, req->visual,
			    req->screen, req->isDirect );
}


int __glXDisp_CreateNewContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateNewContextReq *req = (xGLXCreateNewContextReq *) pc;
    return DoCreateContext( cl, req->context, req->shareList, req->fbconfig,
			    req->screen, req->isDirect );
}


int __glXDisp_CreateContextWithConfigSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateContextWithConfigSGIXReq *req = 
	(xGLXCreateContextWithConfigSGIXReq *) pc;
    return DoCreateContext( cl, req->context, req->shareList, req->fbconfig,
			    req->screen, req->isDirect );
}

/*
** Destroy a GL context as an X resource.
*/
int __glXDisp_DestroyContext(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyContextReq *req = (xGLXDestroyContextReq *) pc;
    GLXContextID gcId = req->context;
    __GLXcontext *glxc;
    
    glxc = (__GLXcontext *) LookupIDByType(gcId, __glXContextRes);
    if (glxc) {
	/*
	** Just free the resource; don't actually destroy the context,
	** because it might be in use.  The
	** destroy method will be called by the resource destruction routine
	** if necessary.
	*/
	FreeResourceByType(gcId, __glXContextRes, FALSE);
	return Success;
    } else {
	client->errorValue = gcId;
	return __glXError(GLXBadContext);
    }
}

/*****************************************************************************/

/*
** For each client, the server keeps a table of all the contexts that are
** current for that client (each thread of a client may have its own current
** context).  These routines add, change, and lookup contexts in the table.
*/

/*
** Add a current context, and return the tag that will be used to refer to it.
*/
static int AddCurrentContext(__GLXclientState *cl, __GLXcontext *glxc)
{
    int i;
    int num = cl->numCurrentContexts;
    __GLXcontext **table = cl->currentContexts;

    if (!glxc) return -1;
    
    /*
    ** Try to find an empty slot and use it.
    */
    for (i=0; i < num; i++) {
	if (!table[i]) {
	    table[i] = glxc;
	    return i+1;
	}
    }
    /*
    ** Didn't find a free slot, so we'll have to grow the table.
    */
    if (!num) {
	table = (__GLXcontext **) xalloc(sizeof(__GLXcontext *));
    } else {
	table = (__GLXcontext **) xrealloc(table,
					   (num+1)*sizeof(__GLXcontext *));
    }
    table[num] = glxc;
    cl->currentContexts = table;
    cl->numCurrentContexts++;
    return num+1;
}

/*
** Given a tag, change the current context for the corresponding entry.
*/
static void ChangeCurrentContext(__GLXclientState *cl, __GLXcontext *glxc,
				GLXContextTag tag)
{
    __GLXcontext **table = cl->currentContexts;
    table[tag-1] = glxc;
}

/*
** For this implementation we have chosen to simply use the index of the
** context's entry in the table as the context tag.  A tag must be greater
** than 0.
*/
__GLXcontext *__glXLookupContextByTag(__GLXclientState *cl, GLXContextTag tag)
{
    int num = cl->numCurrentContexts;

    if (tag < 1 || tag > num) {
	return 0;
    } else {
	return cl->currentContexts[tag-1];
    }
}

/*****************************************************************************/

static void StopUsingContext(__GLXcontext *glxc)
{
    if (glxc) {
	if (glxc == __glXLastContext) {
	    /* Tell server GL library */
	    __glXLastContext = 0;
	}
	glxc->isCurrent = GL_FALSE;
	if (!glxc->idExists) {
	    __glXFreeContext(glxc);
	}
    }
}

static void StartUsingContext(__GLXclientState *cl, __GLXcontext *glxc)
{
    glxc->isCurrent = GL_TRUE;
}

/*****************************************************************************/
/*
** Make an OpenGL context and drawable current.
*/

int __glXDisp_MakeCurrent(__GLXclientState *cl, GLbyte *pc)
{
    xGLXMakeCurrentReq *req = (xGLXMakeCurrentReq *) pc;

    return DoMakeCurrent( cl, req->drawable, req->drawable,
			  req->context, req->oldContextTag );
}

int __glXDisp_MakeContextCurrent(__GLXclientState *cl, GLbyte *pc)
{
    xGLXMakeContextCurrentReq *req = (xGLXMakeContextCurrentReq *) pc;

    return DoMakeCurrent( cl, req->drawable, req->readdrawable,
			  req->context, req->oldContextTag );
}

int __glXDisp_MakeCurrentReadSGI(__GLXclientState *cl, GLbyte *pc)
{
    xGLXMakeCurrentReadSGIReq *req = (xGLXMakeCurrentReadSGIReq *) pc;

    return DoMakeCurrent( cl, req->drawable, req->readable,
			  req->context, req->oldContextTag );
}


/**
 * Given a drawable ID, get the associated drawable and / or pixmap.
 * 
 * If the specified drawable ID is not a pixmap, \c ppPixmap will be set
 * to \c NULL on return.  In either case, \c ppDraw will be set to a drawable.
 * In the case where the drawable ID is a pixmap, \c ppDraw will be set to
 * the drawable associated with that pixmap.
 *
 * \param glxc      Associated GLX context.
 * \param drawId    ID of the drawable.
 * \param ppDraw    Location to store the pointer to the drawable.
 * \param ppPixmap  Location to store the pointer to the pixmap.
 * \param client    Pointer to the client state.
 * \return  Zero is returned on success.  Otherwise a GLX / X11 protocol error
 *          is returned.
 * 
 * \notes This function will need some modification when support pbuffers
 *        is added.
 */
static int GetDrawableOrPixmap( __GLXcontext *glxc, GLXDrawable drawId,
				__GLXdrawable **ppGlxDraw,
				__GLXpixmap **ppPixmap,
				ClientPtr client )
{
    DrawablePtr pDraw;
    __GLcontextModes *modes;
    __GLXdrawable *pGlxDraw;
    __GLXpixmap *drawPixmap = NULL;
    int rc;

    /* This is the GLX 1.3 case - the client passes in a GLXWindow and
     * we just return the __GLXdrawable.  The first time a GLXPixmap
     * comes in, it doesn't have a corresponding __GLXdrawable, so it
     * falls through to the else-case below, but after that it'll have
     * a __GLXdrawable and we'll handle it here. */
    pGlxDraw = (__GLXdrawable *) LookupIDByType(drawId, __glXDrawableRes);
    if (pGlxDraw != NULL) {
	if (glxc != NULL && pGlxDraw->modes != glxc->modes) {
	    client->errorValue = drawId;
	    return BadMatch;
	}

	*ppGlxDraw = pGlxDraw;
	*ppPixmap = pGlxDraw->pGlxPixmap;
	return Success;
    }

    /* The drawId wasn't a GLXWindow, so presumably it's a regular X
     * window.  In that case, we create a shadow GLXWindow for it on
     * demand here for pre GLX 1.3 compatibility and use the X Window
     * XID as its GLXWindow XID.  The client can't explicitly create a
     * GLXWindow with the same XID as an X Window, so we wont get any
     * resource ID clashes.  Effectively, the X Window is now also a
     * GLXWindow. */
    rc = dixLookupDrawable(&pDraw, drawId, client, 0, DixUnknownAccess);
    if (rc == Success) {
	if (pDraw->type == DRAWABLE_WINDOW) {
	    VisualID vid = wVisual((WindowPtr)pDraw);

	    modes = _gl_context_modes_find_visual(glxc->pGlxScreen->modes,
						  vid);
	} else {
	    /*
	    ** An X Pixmap is not allowed as a parameter (a GLX Pixmap
	    ** is, but it must first be created with glxCreateGLXPixmap).
	    */
	    client->errorValue = drawId;
	    return __glXError(GLXBadDrawable);
	}
    } else {
	drawPixmap = (__GLXpixmap *) LookupIDByType(drawId, __glXPixmapRes);
	if (drawPixmap) {
	    pDraw = drawPixmap->pDraw;
	    modes = drawPixmap->modes;
	} else {
	    /*
	    ** Drawable is neither a Window nor a GLXPixmap.
	    */
	    client->errorValue = drawId;
	    return __glXError(GLXBadDrawable);
	}
    }

    /* If we're not given a context, don't create the __GLXdrawable */
    if (glxc == NULL) {
	*ppPixmap = NULL;
	*ppGlxDraw = NULL;
	return Success;
    }

    /* We're binding an X Window or a GLX Pixmap for the first time
     * and need to create a GLX drawable for it.  First check that the
     * drawable screen and fbconfig matches the context ditto. */
    if (pDraw->pScreen != glxc->pScreen || modes != glxc->modes) {
	client->errorValue = drawId;
	return BadMatch;
    }

    pGlxDraw =
	glxc->pGlxScreen->createDrawable(glxc->pGlxScreen,
					 pDraw, drawId, modes);

    /* since we are creating the drawablePrivate, drawId should be new */
    if (!AddResource(drawId, __glXDrawableRes, pGlxDraw)) {
	pGlxDraw->destroy (pGlxDraw);
	return BadAlloc;
    }

    *ppPixmap = drawPixmap;
    *ppGlxDraw = pGlxDraw;

    return 0;
}


int DoMakeCurrent( __GLXclientState *cl,
		   GLXDrawable drawId, GLXDrawable readId,
		   GLXContextID contextId, GLXContextTag tag )
{
    ClientPtr client = cl->client;
    xGLXMakeCurrentReply reply;
    __GLXpixmap *drawPixmap = NULL;
    __GLXpixmap *readPixmap = NULL;
    __GLXcontext *glxc, *prevglxc;
    __GLXdrawable *drawPriv = NULL;
    __GLXdrawable *readPriv = NULL;
    GLint error;
    GLuint  mask;

    /*
    ** If one is None and the other isn't, it's a bad match.
    */

    mask  = (drawId == None)    ? (1 << 0) : 0;
    mask |= (readId == None)    ? (1 << 1) : 0;
    mask |= (contextId == None) ? (1 << 2) : 0;

    if ( (mask != 0x00) && (mask != 0x07) ) {
	return BadMatch;
    }
    
    /*
    ** Lookup old context.  If we have one, it must be in a usable state.
    */
    if (tag != 0) {
	prevglxc = __glXLookupContextByTag(cl, tag);
	if (!prevglxc) {
	    /*
	    ** Tag for previous context is invalid.
	    */
	    return __glXError(GLXBadContextTag);
	}
	if (prevglxc->renderMode != GL_RENDER) {
	    /* Oops.  Not in render mode render. */
	    client->errorValue = prevglxc->id;
	    return __glXError(GLXBadContextState);
	}
    } else {
	prevglxc = 0;
    }

    /*
    ** Lookup new context.  It must not be current for someone else.
    */
    if (contextId != None) {
	int  status;

	glxc = (__GLXcontext *) LookupIDByType(contextId, __glXContextRes);
	if (!glxc) {
	    client->errorValue = contextId;
	    return __glXError(GLXBadContext);
	}
	if ((glxc != prevglxc) && glxc->isCurrent) {
	    /* Context is current to somebody else */
	    return BadAccess;
	}

	assert( drawId != None );
	assert( readId != None );

	status = GetDrawableOrPixmap(glxc, drawId, &drawPriv, &drawPixmap,
				     client);
	if ( status != 0 ) {
	    return status;
	}

	if ( readId != drawId ) {
	    status = GetDrawableOrPixmap(glxc, readId, &readPriv, &readPixmap,
					 client);
	    if ( status != 0 ) {
		return status;
	    }
	} else {
	    readPriv = drawPriv;
	}

    } else {
	/* Switching to no context.  Ignore new drawable. */
	glxc = 0;
	drawPriv = 0;
	readPriv = 0;
    }


    if (prevglxc) {
	/*
	** Flush the previous context if needed.
	*/
	if (__GLX_HAS_UNFLUSHED_CMDS(prevglxc)) {
	    if (__glXForceCurrent(cl, tag, (int *)&error)) {
		CALL_Flush( GET_DISPATCH(), () );
		__GLX_NOTE_FLUSHED_CMDS(prevglxc);
	    } else {
		return error;
	    }
	}

	/*
	** Make the previous context not current.
	*/
	if (!(*prevglxc->loseCurrent)(prevglxc)) {
	    return __glXError(GLXBadContext);
	}
	__glXFlushContextCache();
	__glXDeassociateContext(prevglxc);
    }
	

    if ((glxc != 0) && !glxc->isDirect) {

	glxc->drawPriv = drawPriv;
	glxc->readPriv = readPriv;

	/* make the context current */
	if (!(*glxc->makeCurrent)(glxc)) {
	    glxc->drawPriv = NULL;
	    glxc->readPriv = NULL;
	    return __glXError(GLXBadContext);
	}

	/* resize the buffers */
	if (!(*drawPriv->resize)(drawPriv)) {
	    /* could not do initial resize.  make current failed */
	    (*glxc->loseCurrent)(glxc);
	    glxc->drawPriv = NULL;
	    glxc->readPriv = NULL;
	    return __glXError(GLXBadContext);
	}

	glxc->isCurrent = GL_TRUE;
	__glXAssociateContext(glxc);
	assert(drawPriv->drawGlxc == glxc);
	assert(readPriv->readGlxc == glxc);
    }

    if (prevglxc) {
	if (prevglxc->drawPixmap) {
	    if (prevglxc->readPixmap &&
		prevglxc->drawPixmap != prevglxc->readPixmap) {
		/*
		** The previous drawable was a glx pixmap, release it.
		*/
		prevglxc->readPixmap->refcnt--;
		if (!prevglxc->readPixmap->idExists &&
		    !prevglxc->readPixmap->refcnt) {
		    PixmapPtr pPixmap = (PixmapPtr) prevglxc->readPixmap->pDraw;
		    /*
		    ** The DestroyPixmap routine should decrement the
		    ** refcount of the X pixmap and free only if it's zero.
		    */
		    (*prevglxc->readPixmap->pScreen->DestroyPixmap)(pPixmap);
		    xfree(prevglxc->readPixmap);
		}
	    }

	    /*
	    ** The previous drawable was a glx pixmap, release it.
	    */
	    prevglxc->drawPixmap->refcnt--;
	    if (!prevglxc->drawPixmap->idExists &&
		!prevglxc->drawPixmap->refcnt) {
		PixmapPtr pPixmap = (PixmapPtr) prevglxc->drawPixmap->pDraw;
		/*
		** The DestroyPixmap routine should decrement the
		** refcount of the X pixmap and free only if it's zero.
		*/
		(*prevglxc->drawPixmap->pScreen->DestroyPixmap)(pPixmap);
		xfree(prevglxc->drawPixmap);
	    }

	    prevglxc->drawPixmap = NULL;
	}
	ChangeCurrentContext(cl, glxc, tag);
	StopUsingContext(prevglxc);
    } else {
	tag = AddCurrentContext(cl, glxc);
    }

    if (glxc) {
	if (drawPixmap) {
	    drawPixmap->refcnt++;
	    glxc->drawPixmap = drawPixmap;
	}

	if (readPixmap && (readPixmap != drawPixmap)) {
	    readPixmap->refcnt++;
	    glxc->readPixmap = readPixmap;
	}

	StartUsingContext(cl, glxc);
	reply.contextTag = tag;
    } else {
	reply.contextTag = 0;
    }

    reply.length = 0;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if (client->swapped) {
	__glXSwapMakeCurrentReply(client, &reply);
    } else {
	WriteToClient(client, sz_xGLXMakeCurrentReply, (char *)&reply);
    }
    return Success;
}

int __glXDisp_IsDirect(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXIsDirectReq *req = (xGLXIsDirectReq *) pc;
    xGLXIsDirectReply reply;
    __GLXcontext *glxc;

    /*
    ** Find the GL context.
    */
    glxc = (__GLXcontext *) LookupIDByType(req->context, __glXContextRes);
    if (!glxc) {
	client->errorValue = req->context;
	return __glXError(GLXBadContext);
    }

    reply.isDirect = glxc->isDirect;
    reply.length = 0;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if (client->swapped) {
	__glXSwapIsDirectReply(client, &reply);
    } else {
	WriteToClient(client, sz_xGLXIsDirectReply, (char *)&reply);
    }

    return Success;
}

int __glXDisp_QueryVersion(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXQueryVersionReq *req = (xGLXQueryVersionReq *) pc;
    xGLXQueryVersionReply reply;
    GLuint major, minor;

    major = req->majorVersion;
    minor = req->minorVersion;
    (void)major;
    (void)minor;

    /*
    ** Server should take into consideration the version numbers sent by the
    ** client if it wants to work with older clients; however, in this
    ** implementation the server just returns its version number.
    */
    reply.majorVersion = GLX_SERVER_MAJOR_VERSION;
    reply.minorVersion = GLX_SERVER_MINOR_VERSION;
    reply.length = 0;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if (client->swapped) {
	__glXSwapQueryVersionReply(client, &reply);
    } else {
	WriteToClient(client, sz_xGLXQueryVersionReply, (char *)&reply);
    }
    return Success;
}

int __glXDisp_WaitGL(__GLXclientState *cl, GLbyte *pc)
{
    xGLXWaitGLReq *req = (xGLXWaitGLReq *)pc;
    int error;
    
    if (!__glXForceCurrent(cl, req->contextTag, &error)) {
	return error;
    }
    CALL_Finish( GET_DISPATCH(), () );
    return Success;
}

int __glXDisp_WaitX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXWaitXReq *req = (xGLXWaitXReq *)pc;
    int error;
    
    if (!__glXForceCurrent(cl, req->contextTag, &error)) {
	return error;
    }
    /*
    ** In a multithreaded server that had separate X and GL threads, we would
    ** have to wait for the X thread to finish before returning.  As it stands,
    ** this sample implementation only supports singlethreaded servers, and
    ** nothing needs to be done here.
    */
    return Success;
}

int __glXDisp_CopyContext(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXCopyContextReq *req = (xGLXCopyContextReq *) pc;
    GLXContextID source = req->source;
    GLXContextID dest = req->dest;
    GLXContextTag tag = req->contextTag;
    unsigned long mask = req->mask;
    __GLXcontext *src, *dst;
    int error;

    /*
    ** Check that each context exists.
    */
    src = (__GLXcontext *) LookupIDByType(source, __glXContextRes);
    if (!src) {
	client->errorValue = source;
	return __glXError(GLXBadContext);
    }
    dst = (__GLXcontext *) LookupIDByType(dest, __glXContextRes);
    if (!dst) {
	client->errorValue = dest;
	return __glXError(GLXBadContext);
    }

    /*
    ** They must be in the same address space, and same screen.
    ** NOTE: no support for direct rendering contexts here.
    */
    if (src->isDirect || dst->isDirect ||
	(src->pGlxScreen != dst->pGlxScreen)) {
	client->errorValue = source;
	return BadMatch;
    }

    /*
    ** The destination context must not be current for any client.
    */
    if (dst->isCurrent) {
	client->errorValue = dest;
	return BadAccess;
    }

    if (tag) {
	__GLXcontext *tagcx = __glXLookupContextByTag(cl, tag);
	
	if (!tagcx) {
	    return __glXError(GLXBadContextTag);
	}
	if (tagcx != src) {
	    /*
	    ** This would be caused by a faulty implementation of the client
	    ** library.
	    */
	    return BadMatch;
	}
	/*
	** In this case, glXCopyContext is in both GL and X streams, in terms
	** of sequentiality.
	*/
	if (__glXForceCurrent(cl, tag, &error)) {
	    /*
	    ** Do whatever is needed to make sure that all preceding requests
	    ** in both streams are completed before the copy is executed.
	    */
	    CALL_Finish( GET_DISPATCH(), () );
	    __GLX_NOTE_FLUSHED_CMDS(tagcx);
	} else {
	    return error;
	}
    }
    /*
    ** Issue copy.  The only reason for failure is a bad mask.
    */
    if (!(*dst->copy)(dst, src, mask)) {
	client->errorValue = mask;
	return BadValue;
    }
    return Success;
}


int DoGetVisualConfigs(__GLXclientState *cl, unsigned screen,
		       GLboolean do_swap)
{
    ClientPtr client = cl->client;
    xGLXGetVisualConfigsReply reply;
    __GLXscreen *pGlxScreen;
    __GLcontextModes *modes;
    CARD32 buf[__GLX_TOTAL_CONFIG];
    int p;
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;

    if (screen >= screenInfo.numScreens) {
	/* The client library must send a valid screen number. */
	client->errorValue = screen;
	return BadValue;
    }
    pGlxScreen = __glXActiveScreens[screen];

    reply.numVisuals = pGlxScreen->numUsableVisuals;
    reply.numProps = __GLX_TOTAL_CONFIG;
    reply.length = (pGlxScreen->numUsableVisuals * __GLX_SIZE_CARD32 *
		    __GLX_TOTAL_CONFIG) >> 2;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if ( do_swap ) {
	__GLX_SWAP_SHORT(&reply.sequenceNumber);
	__GLX_SWAP_INT(&reply.length);
	__GLX_SWAP_INT(&reply.numVisuals);
	__GLX_SWAP_INT(&reply.numProps);
    }

    WriteToClient(client, sz_xGLXGetVisualConfigsReply, (char *)&reply);

    for ( modes = pGlxScreen->modes ; modes != NULL ; modes = modes->next ) {
	if (modes->visualID == 0) {
	    /* not a usable visual */
	    continue;
	}
	p = 0;
	buf[p++] = modes->visualID;
	buf[p++] = _gl_convert_to_x_visual_type( modes->visualType );
	buf[p++] = modes->rgbMode;

	buf[p++] = modes->redBits;
	buf[p++] = modes->greenBits;
	buf[p++] = modes->blueBits;
	buf[p++] = modes->alphaBits;
	buf[p++] = modes->accumRedBits;
	buf[p++] = modes->accumGreenBits;
	buf[p++] = modes->accumBlueBits;
	buf[p++] = modes->accumAlphaBits;

	buf[p++] = modes->doubleBufferMode;
	buf[p++] = modes->stereoMode;

	buf[p++] = modes->rgbBits;
	buf[p++] = modes->depthBits;
	buf[p++] = modes->stencilBits;
	buf[p++] = modes->numAuxBuffers;
	buf[p++] = modes->level;
	/* 
	** Add token/value pairs for extensions.
	*/
	buf[p++] = GLX_VISUAL_CAVEAT_EXT;
	buf[p++] = modes->visualRating;
	buf[p++] = GLX_TRANSPARENT_TYPE;
	buf[p++] = modes->transparentPixel;
	buf[p++] = GLX_TRANSPARENT_RED_VALUE;
	buf[p++] = modes->transparentRed;
	buf[p++] = GLX_TRANSPARENT_GREEN_VALUE;
	buf[p++] = modes->transparentGreen;
	buf[p++] = GLX_TRANSPARENT_BLUE_VALUE;
	buf[p++] = modes->transparentBlue;
	buf[p++] = GLX_TRANSPARENT_ALPHA_VALUE;
	buf[p++] = modes->transparentAlpha;
	buf[p++] = GLX_TRANSPARENT_INDEX_VALUE;
	buf[p++] = modes->transparentIndex;

	if ( do_swap ) {
	    __GLX_SWAP_INT_ARRAY(buf, __GLX_TOTAL_CONFIG);
	}
	WriteToClient(client, __GLX_SIZE_CARD32 * __GLX_TOTAL_CONFIG, 
		(char *)buf);
    }
    return Success;
}

int __glXDisp_GetVisualConfigs(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetVisualConfigsReq *req = (xGLXGetVisualConfigsReq *) pc;
    return DoGetVisualConfigs( cl, req->screen, GL_FALSE );
}


/* Composite adds a 32 bit ARGB visual after glxvisuals.c have created
 * the context modes for the screens.  This visual is useful for GLX
 * pixmaps, so we create a single mode for this visual with no extra
 * buffers. */
static void
__glXCreateARGBConfig(__GLXscreen *screen)
{
    __GLcontextModes *modes;
    VisualPtr visual;
    int i;

    /* search for a 32-bit visual */
    visual = NULL;
    for (i = 0; i < screen->pScreen->numVisuals; i++) 
	if (screen->pScreen->visuals[i].nplanes == 32) {
	    visual = &screen->pScreen->visuals[i];
	    break;
	}

    if (visual == NULL || visual->class != TrueColor)
	return;

    /* Stop now if we already added the mode. */
    if (_gl_context_modes_find_visual (screen->modes, visual->vid))
	return;

    modes = _gl_context_modes_create(1, sizeof(__GLcontextModes));
    if (modes == NULL)
	return;

    /* Insert this new mode at the TAIL of the linked list.
     * Previously, the mode was incorrectly inserted at the head of the
     * list, causing find_mesa_visual() to be off by one.  This would
     * GLX clients to blow up if they attempted to use the last mode
     * in the list!
     */
    {
        __GLcontextModes *prev = NULL, *m;
        for (m = screen->modes; m; m = m->next)
            prev = m;
        if (prev)
            prev->next = modes;
        else
            screen->modes = modes;
    }

    screen->numUsableVisuals++;
    screen->numVisuals++;

    modes->visualID = visual->vid;
    modes->fbconfigID = visual->vid;
    modes->visualType = GLX_TRUE_COLOR;
    modes->drawableType = GLX_WINDOW_BIT | GLX_PIXMAP_BIT;
    modes->renderType = GLX_RGBA_BIT;
    modes->xRenderable = GL_TRUE;
    modes->rgbMode = TRUE;
    modes->colorIndexMode = FALSE;
    modes->doubleBufferMode = FALSE;
    modes->stereoMode = FALSE;
    modes->haveAccumBuffer = FALSE;

    modes->redBits = visual->bitsPerRGBValue;;
    modes->greenBits = visual->bitsPerRGBValue;
    modes->blueBits = visual->bitsPerRGBValue;
    modes->alphaBits = visual->bitsPerRGBValue;

    modes->rgbBits = 4 * visual->bitsPerRGBValue;
    modes->indexBits = 0;
    modes->level = 0;
    modes->numAuxBuffers = 0;

    modes->haveDepthBuffer = FALSE;
    modes->depthBits = 0;
    modes->haveStencilBuffer = FALSE;
    modes->stencilBits = 0;

    modes->visualRating = GLX_NON_CONFORMANT_CONFIG;
}


#define __GLX_TOTAL_FBCONFIG_ATTRIBS (28)
#define __GLX_FBCONFIG_ATTRIBS_LENGTH (__GLX_TOTAL_FBCONFIG_ATTRIBS * 2)
/**
 * Send the set of GLXFBConfigs to the client.  There is not currently
 * and interface into the driver on the server-side to get GLXFBConfigs,
 * so we "invent" some based on the \c __GLXvisualConfig structures that
 * the driver does supply.
 * 
 * The reply format for both \c glXGetFBConfigs and \c glXGetFBConfigsSGIX
 * is the same, so this routine pulls double duty.
 */

int DoGetFBConfigs(__GLXclientState *cl, unsigned screen, GLboolean do_swap)
{
    ClientPtr client = cl->client;
    xGLXGetFBConfigsReply reply;
    __GLXscreen *pGlxScreen;
    CARD32 buf[__GLX_FBCONFIG_ATTRIBS_LENGTH];
    int p;
    __GLcontextModes *modes;
    __GLX_DECLARE_SWAP_VARIABLES;
    __GLX_DECLARE_SWAP_ARRAY_VARIABLES;


    if (screen >= screenInfo.numScreens) {
	/* The client library must send a valid screen number. */
	client->errorValue = screen;
	return BadValue;
    }
    pGlxScreen = __glXActiveScreens[screen];

    /* Create the "extra" 32bpp ARGB visual, if not already added.
     * XXX This is questionable place to do so!  Re-examine this someday.
     */
    __glXCreateARGBConfig(pGlxScreen);

    reply.numFBConfigs = pGlxScreen->numUsableVisuals;
    reply.numAttribs = __GLX_TOTAL_FBCONFIG_ATTRIBS;
    reply.length = (__GLX_FBCONFIG_ATTRIBS_LENGTH * reply.numFBConfigs);
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if ( do_swap ) {
	__GLX_SWAP_SHORT(&reply.sequenceNumber);
	__GLX_SWAP_INT(&reply.length);
	__GLX_SWAP_INT(&reply.numFBConfigs);
	__GLX_SWAP_INT(&reply.numAttribs);
    }

    WriteToClient(client, sz_xGLXGetFBConfigsReply, (char *)&reply);

    for ( modes = pGlxScreen->modes ; modes != NULL ; modes = modes->next ) {
	if (modes->visualID == 0) {
	    /* not a usable visual */
	    continue;
	}
	p = 0;

#define WRITE_PAIR(tag,value) \
    do { buf[p++] = tag ; buf[p++] = value ; } while( 0 )

	WRITE_PAIR( GLX_VISUAL_ID,        modes->visualID );
	WRITE_PAIR( GLX_FBCONFIG_ID,      modes->visualID );
	WRITE_PAIR( GLX_X_RENDERABLE,     GL_TRUE );

	WRITE_PAIR( GLX_RGBA,             modes->rgbMode );
	WRITE_PAIR( GLX_DOUBLEBUFFER,     modes->doubleBufferMode );
	WRITE_PAIR( GLX_STEREO,           modes->stereoMode );

	WRITE_PAIR( GLX_BUFFER_SIZE,      modes->rgbBits );
	WRITE_PAIR( GLX_LEVEL,            modes->level );
	WRITE_PAIR( GLX_AUX_BUFFERS,      modes->numAuxBuffers );
	WRITE_PAIR( GLX_RED_SIZE,         modes->redBits );
	WRITE_PAIR( GLX_GREEN_SIZE,       modes->greenBits );
	WRITE_PAIR( GLX_BLUE_SIZE,        modes->blueBits );
	WRITE_PAIR( GLX_ALPHA_SIZE,       modes->alphaBits );
	WRITE_PAIR( GLX_ACCUM_RED_SIZE,   modes->accumRedBits );
	WRITE_PAIR( GLX_ACCUM_GREEN_SIZE, modes->accumGreenBits );
	WRITE_PAIR( GLX_ACCUM_BLUE_SIZE,  modes->accumBlueBits );
	WRITE_PAIR( GLX_ACCUM_ALPHA_SIZE, modes->accumAlphaBits );
	WRITE_PAIR( GLX_DEPTH_SIZE,       modes->depthBits );
	WRITE_PAIR( GLX_STENCIL_SIZE,     modes->stencilBits );

	WRITE_PAIR( GLX_X_VISUAL_TYPE,    modes->visualType );

	/* 
	** Add token/value pairs for extensions.
	*/
	WRITE_PAIR( GLX_CONFIG_CAVEAT, modes->visualRating );
	WRITE_PAIR( GLX_TRANSPARENT_TYPE, modes->transparentPixel );
	WRITE_PAIR( GLX_TRANSPARENT_RED_VALUE, modes->transparentRed );
	WRITE_PAIR( GLX_TRANSPARENT_GREEN_VALUE, modes->transparentGreen );
	WRITE_PAIR( GLX_TRANSPARENT_BLUE_VALUE, modes->transparentBlue );
	WRITE_PAIR( GLX_TRANSPARENT_ALPHA_VALUE, modes->transparentAlpha );
	WRITE_PAIR( GLX_TRANSPARENT_INDEX_VALUE, modes->transparentIndex );
	WRITE_PAIR( GLX_SWAP_METHOD_OML, modes->swapMethod );

	if ( do_swap ) {
	    __GLX_SWAP_INT_ARRAY(buf, __GLX_FBCONFIG_ATTRIBS_LENGTH);
	}
	WriteToClient(client, __GLX_SIZE_CARD32 * __GLX_FBCONFIG_ATTRIBS_LENGTH,
		      (char *)buf);
    }
    return Success;
}


int __glXDisp_GetFBConfigs(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetFBConfigsReq *req = (xGLXGetFBConfigsReq *) pc;
    return DoGetFBConfigs( cl, req->screen, GL_FALSE );
}


int __glXDisp_GetFBConfigsSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetFBConfigsSGIXReq *req = (xGLXGetFBConfigsSGIXReq *) pc;
    return DoGetFBConfigs( cl, req->screen, GL_FALSE );
}

static int ValidateCreateDrawable(ClientPtr client,
				  int screenNum, XID fbconfigId,
				  XID drawablId, XID glxDrawableId,
				  int type, __GLcontextModes **modes,
				  DrawablePtr *ppDraw)
{
    DrawablePtr pDraw;
    ScreenPtr pScreen;
    VisualPtr pVisual;
    __GLXscreen *pGlxScreen;
    int i, rc;

    LEGAL_NEW_RESOURCE(glxDrawableId, client);

    rc = dixLookupDrawable(&pDraw, drawablId, client, 0, DixUnknownAccess);
    if (rc != Success || pDraw->type != type) {
	client->errorValue = drawablId;
	return type == DRAWABLE_WINDOW ? BadWindow : BadPixmap;
    }

    /* Check if screen of the fbconfig matches screen of drawable. */
    pScreen = pDraw->pScreen;
    if (screenNum != pScreen->myNum) {
	return BadMatch;
    }

    /* If this fbconfig has a corresponding VisualRec the number of
     * planes must match the drawable depth. */
    pVisual = pScreen->visuals;
    for (i = 0; i < pScreen->numVisuals; i++, pVisual++) {
	if (pVisual->vid == fbconfigId && pVisual->nplanes != pDraw->depth)
	    return BadMatch;
    }

    /* Get configuration of the visual. */
    pGlxScreen = __glXgetActiveScreen(screenNum);
    *modes = _gl_context_modes_find_visual(pGlxScreen->modes, fbconfigId);
    if (*modes == NULL) {
	/* Visual not support on this screen by this OpenGL implementation. */
	client->errorValue = fbconfigId;
	return BadValue;
    }

    *ppDraw = pDraw;

    return Success;
}

/*
** Create a GLX Pixmap from an X Pixmap.
*/
int DoCreateGLXPixmap(__GLXclientState *cl, XID fbconfigId,
		      GLuint screenNum, XID pixmapId, XID glxPixmapId,
		      CARD32 *attribs, CARD32 numAttribs)
{
    ClientPtr client = cl->client;
    DrawablePtr pDraw;
    __GLXpixmap *pGlxPixmap;
    __GLcontextModes *modes;
    GLenum target = 0;
    int retval, i;

    retval = ValidateCreateDrawable (client, screenNum, fbconfigId,
				     pixmapId, glxPixmapId,
				     DRAWABLE_PIXMAP, &modes, &pDraw);
    if (retval != Success)
	return retval;

    pGlxPixmap = (__GLXpixmap *) xalloc(sizeof(__GLXpixmap));
    if (!pGlxPixmap) {
	return BadAlloc;
    }
    if (!(AddResource(glxPixmapId, __glXPixmapRes, pGlxPixmap))) {
	return BadAlloc;
    }
    pGlxPixmap->pDraw = pDraw;
    pGlxPixmap->pGlxScreen = __glXgetActiveScreen(screenNum);
    pGlxPixmap->pScreen = pDraw->pScreen;
    pGlxPixmap->idExists = True;
#ifdef XF86DRI
    pGlxPixmap->pDamage = NULL;
#endif
    pGlxPixmap->refcnt = 0;

    pGlxPixmap->modes = modes;

    for (i = 0; i < numAttribs; i++) {
	if (attribs[2 * i] == GLX_TEXTURE_TARGET_EXT) {
	    switch (attribs[2 * i + 1]) {
	    case GLX_TEXTURE_2D_EXT:
		target = GL_TEXTURE_2D;
		break;
	    case GLX_TEXTURE_RECTANGLE_EXT:
		target = GL_TEXTURE_RECTANGLE_ARB;
		break;
	    }
	}
    }

    if (!target) {
	int w = pDraw->width, h = pDraw->height;

	if (h & (h - 1) || w & (w - 1))
	    target = GL_TEXTURE_RECTANGLE_ARB;
	else
	    target = GL_TEXTURE_2D;
    }

    pGlxPixmap->target = target;

    /*
    ** Bump the ref count on the X pixmap so it won't disappear.
    */
    ((PixmapPtr) pDraw)->refcnt++;

    return Success;
}

int __glXDisp_CreateGLXPixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateGLXPixmapReq *req = (xGLXCreateGLXPixmapReq *) pc;
    return DoCreateGLXPixmap( cl, req->visual, req->screen,
			      req->pixmap, req->glxpixmap, NULL, 0 );
}

int __glXDisp_CreatePixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreatePixmapReq *req = (xGLXCreatePixmapReq *) pc;
    return DoCreateGLXPixmap( cl, req->fbconfig, req->screen,
			      req->pixmap, req->glxpixmap,
			      (CARD32*)(req + 1),
			      req->numAttribs );
}

int __glXDisp_CreateGLXPixmapWithConfigSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateGLXPixmapWithConfigSGIXReq *req = 
	(xGLXCreateGLXPixmapWithConfigSGIXReq *) pc;
    return DoCreateGLXPixmap( cl, req->fbconfig, req->screen,
			      req->pixmap, req->glxpixmap, NULL, 0 );
}


int DoDestroyPixmap(__GLXclientState *cl, XID glxpixmap)
{
    ClientPtr client = cl->client;

    /*
    ** Check if it's a valid GLX pixmap.
    */
    if (!LookupIDByType(glxpixmap, __glXPixmapRes)) {
	client->errorValue = glxpixmap;
	return __glXError(GLXBadPixmap);
    }
    FreeResource(glxpixmap, FALSE);

    return Success;
}

int __glXDisp_DestroyGLXPixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyGLXPixmapReq *req = (xGLXDestroyGLXPixmapReq *) pc;

    return DoDestroyPixmap(cl, req->glxpixmap);
}

int __glXDisp_DestroyPixmap(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyPixmapReq *req = (xGLXDestroyPixmapReq *) pc;

    return DoDestroyPixmap(cl, req->glxpixmap);
}

int __glXDisp_CreatePbuffer(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreatePbufferReq *req = (xGLXCreatePbufferReq *) pc;

    (void) req;

    return BadRequest;
}

int __glXDisp_DestroyPbuffer(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyPbufferReq *req = (xGLXDestroyPbufferReq *) pc;

    (void) req;

    return BadRequest;
}

int __glXDisp_ChangeDrawableAttributes(__GLXclientState *cl, GLbyte *pc)
{
    xGLXChangeDrawableAttributesReq *req =
	(xGLXChangeDrawableAttributesReq *) pc;

    (void) req;

    return BadRequest;
}

int __glXDisp_CreateWindow(__GLXclientState *cl, GLbyte *pc)
{
    xGLXCreateWindowReq *req = (xGLXCreateWindowReq *) pc;
    ClientPtr client = cl->client;
    DrawablePtr pDraw;
    __GLXdrawable *glxPriv;
    __GLXscreen *screen;
    __GLcontextModes *modes;
    int retval;

    retval = ValidateCreateDrawable (client, req->screen, req->fbconfig,
				     req->window, req->glxwindow,
				     DRAWABLE_WINDOW, &modes, &pDraw);
    if (retval != Success)
	return retval;

    /* FIXME: We need to check that the window visual is compatible
     * with the specified fbconfig. */

    screen = __glXgetActiveScreen(req->screen);
    glxPriv = screen->createDrawable(screen, pDraw, req->glxwindow, modes);
    if (glxPriv == NULL)
	return BadAlloc;

    if (!AddResource(req->glxwindow, __glXDrawableRes, glxPriv)) {
	glxPriv->destroy (glxPriv);
	return BadAlloc;
    }

    return Success;
}

int __glXDisp_DestroyWindow(__GLXclientState *cl, GLbyte *pc)
{
    xGLXDestroyWindowReq *req = (xGLXDestroyWindowReq *) pc;
    ClientPtr client = cl->client;

    /*
    ** Check if it's a valid GLX window.
    */
    if (!LookupIDByType(req->glxwindow, __glXDrawableRes)) {
	client->errorValue = req->glxwindow;
	return __glXError(GLXBadWindow);
    }
    FreeResource(req->glxwindow, FALSE);

    return Success;
}


/*****************************************************************************/

/*
** NOTE: There is no portable implementation for swap buffers as of
** this time that is of value.  Consequently, this code must be
** implemented by somebody other than SGI.
*/
int __glXDisp_SwapBuffers(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXSwapBuffersReq *req = (xGLXSwapBuffersReq *) pc;
    GLXContextTag tag = req->contextTag;
    XID drawId = req->drawable;
    __GLXcontext *glxc = NULL;
    __GLXdrawable *pGlxDraw;
    __GLXpixmap *pPixmap;
    int error;

    if (tag) {
	glxc = __glXLookupContextByTag(cl, tag);
	if (!glxc) {
	    return __glXError(GLXBadContextTag);
	}
	/*
	** The calling thread is swapping its current drawable.  In this case,
	** glxSwapBuffers is in both GL and X streams, in terms of
	** sequentiality.
	*/
	if (__glXForceCurrent(cl, tag, &error)) {
	    /*
	    ** Do whatever is needed to make sure that all preceding requests
	    ** in both streams are completed before the swap is executed.
	    */
	    CALL_Finish( GET_DISPATCH(), () );
	    __GLX_NOTE_FLUSHED_CMDS(glxc);
	} else {
	    return error;
	}
    }

    error = GetDrawableOrPixmap(glxc, drawId, &pGlxDraw, &pPixmap, client);
    if (error != Success)
	return error;

    if (pGlxDraw != NULL && pGlxDraw->type == DRAWABLE_WINDOW &&
	(*pGlxDraw->swapBuffers)(pGlxDraw) == GL_FALSE)
	return __glXError(GLXBadDrawable);

    return Success;
}


int DoQueryContext(__GLXclientState *cl, GLXContextID gcId)
{
    ClientPtr client = cl->client;
    __GLXcontext *ctx;
    xGLXQueryContextInfoEXTReply reply;
    int nProps;
    int *sendBuf, *pSendBuf;
    int nReplyBytes;

    ctx = (__GLXcontext *) LookupIDByType(gcId, __glXContextRes);
    if (!ctx) {
	client->errorValue = gcId;
	return __glXError(GLXBadContext);
    }

    nProps = 3;
    reply.length = nProps << 1;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.n = nProps;

    nReplyBytes = reply.length << 2;
    sendBuf = (int *)xalloc((size_t)nReplyBytes);
    if (sendBuf == NULL) {
	return __glXError(GLXBadContext);	/* XXX: Is this correct? */
    }
    pSendBuf = sendBuf;
    *pSendBuf++ = GLX_SHARE_CONTEXT_EXT;
    *pSendBuf++ = (int)(ctx->share_id);
    *pSendBuf++ = GLX_VISUAL_ID_EXT;
    *pSendBuf++ = (int)(ctx->pVisual->vid);
    *pSendBuf++ = GLX_SCREEN_EXT;
    *pSendBuf++ = (int)(ctx->pScreen->myNum);

    if (client->swapped) {
	__glXSwapQueryContextInfoEXTReply(client, &reply, sendBuf);
    } else {
	WriteToClient(client, sz_xGLXQueryContextInfoEXTReply, (char *)&reply);
	WriteToClient(client, nReplyBytes, (char *)sendBuf);
    }
    xfree((char *)sendBuf);

    return Success;
}

int __glXDisp_QueryContextInfoEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXQueryContextInfoEXTReq *req = (xGLXQueryContextInfoEXTReq *) pc;

    return DoQueryContext(cl, req->context);
}

int __glXDisp_QueryContext(__GLXclientState *cl, GLbyte *pc)
{
    xGLXQueryContextReq *req = (xGLXQueryContextReq *) pc;

    return DoQueryContext(cl, req->context);
}

int __glXDisp_BindTexImageEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    ClientPtr		 client = cl->client;
    __GLXpixmap		*pGlxPixmap;
    __GLXcontext	*context;
    GLXDrawable		 drawId;
    int			 buffer;
    int			 error;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = *((CARD32 *) (pc));
    buffer = *((INT32 *)  (pc + 4));

    if (buffer != GLX_FRONT_LEFT_EXT)
	return __glXError(GLXBadPixmap);

    context = __glXForceCurrent (cl, req->contextTag, &error);
    if (!context)
	return error;

    pGlxPixmap = (__GLXpixmap *)LookupIDByType(drawId, __glXPixmapRes);
    if (!pGlxPixmap) {
	client->errorValue = drawId;
	return __glXError(GLXBadPixmap);
    }

    if (!context->textureFromPixmap)
	return __glXError(GLXUnsupportedPrivateRequest);

    return context->textureFromPixmap->bindTexImage(context,
						    buffer,
						    pGlxPixmap);
}

int __glXDisp_ReleaseTexImageEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    ClientPtr		 client = cl->client;
    __GLXpixmap		*pGlxPixmap;
    __GLXcontext	*context;
    GLXDrawable		 drawId;
    int			 buffer;
    int			 error;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = *((CARD32 *) (pc));
    buffer = *((INT32 *)  (pc + 4));
    
    context = __glXForceCurrent (cl, req->contextTag, &error);
    if (!context)
	return error;

    pGlxPixmap = (__GLXpixmap *)LookupIDByType(drawId, __glXPixmapRes);
    if (!pGlxPixmap) {
	client->errorValue = drawId;
	return __glXError(GLXBadDrawable);
    }

    if (!context->textureFromPixmap)
	return __glXError(GLXUnsupportedPrivateRequest);

    return context->textureFromPixmap->releaseTexImage(context,
						       buffer,
						       pGlxPixmap);
}

int __glXDisp_CopySubBufferMESA(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLXContextTag         tag = req->contextTag;
    __GLXcontext         *glxc = NULL;
    __GLXdrawable        *pGlxDraw;
    __GLXpixmap          *pPixmap;
    ClientPtr		  client = cl->client;
    GLXDrawable		  drawId;
    int                   error;
    int                   x, y, width, height;

    (void) client;
    (void) req;

    pc += __GLX_VENDPRIV_HDR_SIZE;

    drawId = *((CARD32 *) (pc));
    x      = *((INT32 *)  (pc + 4));
    y      = *((INT32 *)  (pc + 8));
    width  = *((INT32 *)  (pc + 12));
    height = *((INT32 *)  (pc + 16));

    if (tag) {
	glxc = __glXLookupContextByTag(cl, tag);
	if (!glxc) {
	    return __glXError(GLXBadContextTag);
	}
	/*
	** The calling thread is swapping its current drawable.  In this case,
	** glxSwapBuffers is in both GL and X streams, in terms of
	** sequentiality.
	*/
	if (__glXForceCurrent(cl, tag, &error)) {
	    /*
	    ** Do whatever is needed to make sure that all preceding requests
	    ** in both streams are completed before the swap is executed.
	    */
	    CALL_Finish( GET_DISPATCH(), () );
	    __GLX_NOTE_FLUSHED_CMDS(glxc);
	} else {
	    return error;
	}
    }

    error = GetDrawableOrPixmap(glxc, drawId, &pGlxDraw, &pPixmap, client);
    if (error != Success)
	return error;

    if (pGlxDraw == NULL ||
	pGlxDraw->type != DRAWABLE_WINDOW ||
	pGlxDraw->copySubBuffer == NULL)
	return __glXError(GLXBadDrawable);

    (*pGlxDraw->copySubBuffer)(pGlxDraw, x, y, width, height);

    return Success;
}

/*
** Get drawable attributes
*/
static int
DoGetDrawableAttributes(__GLXclientState *cl, XID drawId)
{
    ClientPtr client = cl->client;
    __GLXpixmap *glxPixmap;
    xGLXGetDrawableAttributesReply reply;
    CARD32 attributes[4];
    int numAttribs;

    glxPixmap = (__GLXpixmap *)LookupIDByType(drawId, __glXPixmapRes);
    if (!glxPixmap) {
	client->errorValue = drawId;
	return __glXError(GLXBadPixmap);
    }

    numAttribs = 2;
    reply.length = numAttribs << 1;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.numAttribs = numAttribs;

    attributes[0] = GLX_TEXTURE_TARGET_EXT;
    attributes[1] = glxPixmap->target == GL_TEXTURE_2D ? GLX_TEXTURE_2D_EXT :
	GLX_TEXTURE_RECTANGLE_EXT;
    attributes[2] = GLX_Y_INVERTED_EXT;
    attributes[3] = GL_FALSE;

    if (client->swapped) {
	__glXSwapGetDrawableAttributesReply(client, &reply, attributes);
    } else {
	WriteToClient(client, sz_xGLXGetDrawableAttributesReply,
		      (char *)&reply);
	WriteToClient(client, reply.length * sizeof (CARD32),
		      (char *)attributes);
    }

    return Success;
}

int __glXDisp_GetDrawableAttributesSGIX(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateWithReplyReq *req = (xGLXVendorPrivateWithReplyReq *)pc;
    CARD32 *data;
    XID drawable;
    
    data = (CARD32 *) (req + 1);
    drawable = data[0];

    return DoGetDrawableAttributes(cl, drawable);
}

int __glXDisp_GetDrawableAttributes(__GLXclientState *cl, GLbyte *pc)
{
    xGLXGetDrawableAttributesReq *req = (xGLXGetDrawableAttributesReq *)pc;

    return DoGetDrawableAttributes(cl, req->drawable);
}

/************************************************************************/

/*
** Render and Renderlarge are not in the GLX API.  They are used by the GLX
** client library to send batches of GL rendering commands.
*/

int DoRender(__GLXclientState *cl, GLbyte *pc, int do_swap)
{
    xGLXRenderReq *req;
    ClientPtr client= cl->client;
    int left, cmdlen, error;
    int commandsDone;
    CARD16 opcode;
    __GLXrenderHeader *hdr;
    __GLXcontext *glxc;
    __GLX_DECLARE_SWAP_VARIABLES;

    
    req = (xGLXRenderReq *) pc;
    if (do_swap) {
	__GLX_SWAP_SHORT(&req->length);
	__GLX_SWAP_INT(&req->contextTag);
    }

    glxc = __glXForceCurrent(cl, req->contextTag, &error);
    if (!glxc) {
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
	if (do_swap) {
	    __GLX_SWAP_SHORT(&hdr->length);
	    __GLX_SWAP_SHORT(&hdr->opcode);
	}
	cmdlen = hdr->length;
	opcode = hdr->opcode;

	/*
	** Check for core opcodes and grab entry data.
	*/
	err = __glXGetProtocolSizeData(& Render_dispatch_info, opcode, & entry);
	proc = (__GLXdispatchRenderProcPtr)
	  __glXGetProtocolDecodeFunction(& Render_dispatch_info, opcode, do_swap);

	if ((err < 0) || (proc == NULL)) {
	    client->errorValue = commandsDone;
	    return __glXError(GLXBadRenderRequest);
	}

        if (entry.varsize) {
            /* variable size command */
            extra = (*entry.varsize)(pc + __GLX_RENDER_HDR_SIZE, do_swap);
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
    __GLX_NOTE_UNFLUSHED_CMDS(glxc);
    return Success;
}

/*
** Execute all the drawing commands in a request.
*/
int __glXDisp_Render(__GLXclientState *cl, GLbyte *pc)
{
    return DoRender(cl, pc, False);
}

int DoRenderLarge(__GLXclientState *cl, GLbyte *pc, int do_swap)
{
    xGLXRenderLargeReq *req;
    ClientPtr client= cl->client;
    size_t dataBytes;
    __GLXrenderLargeHeader *hdr;
    __GLXcontext *glxc;
    int error;
    CARD16 opcode;
    __GLX_DECLARE_SWAP_VARIABLES;

    
    req = (xGLXRenderLargeReq *) pc;
    if (do_swap) {
	__GLX_SWAP_SHORT(&req->length);
	__GLX_SWAP_INT(&req->contextTag);
	__GLX_SWAP_INT(&req->dataBytes);
	__GLX_SWAP_SHORT(&req->requestNumber);
	__GLX_SWAP_SHORT(&req->requestTotal);
    }

    glxc = __glXForceCurrent(cl, req->contextTag, &error);
    if (!glxc) {
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
	if (do_swap) {
	    __GLX_SWAP_INT(&hdr->length);
	    __GLX_SWAP_INT(&hdr->opcode);
	}
	cmdlen = hdr->length;
	opcode = hdr->opcode;

	/*
	** Check for core opcodes and grab entry data.
	*/
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
	    extra = (*entry.varsize)(pc + __GLX_RENDER_LARGE_HDR_SIZE, do_swap);
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
	    **
	    ** Use the opcode to index into the procedure table.
	    */
	    opcode = hdr->opcode;

	    proc = (__GLXdispatchRenderProcPtr)
	      __glXGetProtocolDecodeFunction(& Render_dispatch_info, opcode, do_swap);
	    if (proc == NULL) {
		client->errorValue = opcode;
		return __glXError(GLXBadLargeRequest);
	    }

	    /*
	    ** Skip over the header and execute the command.
	    */
	    (*proc)(cl->largeCmdBuf + __GLX_RENDER_LARGE_HDR_SIZE);
	    __GLX_NOTE_UNFLUSHED_CMDS(glxc);

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

/*
** Execute a large rendering request (one that spans multiple X requests).
*/
int __glXDisp_RenderLarge(__GLXclientState *cl, GLbyte *pc)
{
    return DoRenderLarge(cl, pc, False);
}

extern RESTYPE __glXSwapBarrierRes;

int __glXDisp_BindSwapBarrierSGIX(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXBindSwapBarrierSGIXReq *req = (xGLXBindSwapBarrierSGIXReq *) pc;
    XID drawable = req->drawable;
    int barrier = req->barrier;
    DrawablePtr pDraw;
    int screen, rc;

    rc = dixLookupDrawable(&pDraw, drawable, client, 0, DixUnknownAccess);
    if (rc == Success && (pDraw->type == DRAWABLE_WINDOW)) {
	screen = pDraw->pScreen->myNum;
        if (__glXSwapBarrierFuncs &&
            __glXSwapBarrierFuncs[screen].bindSwapBarrierFunc) {
            int ret = __glXSwapBarrierFuncs[screen].bindSwapBarrierFunc(screen, drawable, barrier);
            if (ret == Success) {
                if (barrier)
                    /* add source for cleanup when drawable is gone */
                    AddResource(drawable, __glXSwapBarrierRes, (pointer)screen);
                else
                    /* delete source */
                    FreeResourceByType(drawable, __glXSwapBarrierRes, FALSE);
            }
            return ret;
        }
    }
    client->errorValue = drawable;
    return __glXError(GLXBadDrawable);
}


int __glXDisp_QueryMaxSwapBarriersSGIX(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXQueryMaxSwapBarriersSGIXReq *req =
                                    (xGLXQueryMaxSwapBarriersSGIXReq *) pc;
    xGLXQueryMaxSwapBarriersSGIXReply reply;
    int screen = req->screen;

    if (__glXSwapBarrierFuncs &&
        __glXSwapBarrierFuncs[screen].queryMaxSwapBarriersFunc)
        reply.max = __glXSwapBarrierFuncs[screen].queryMaxSwapBarriersFunc(screen);
    else
        reply.max = 0;


    reply.length = 0;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;

    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
    }

    WriteToClient(client, sz_xGLXQueryMaxSwapBarriersSGIXReply,
                        (char *) &reply);
    return Success;
}

#define GLX_BAD_HYPERPIPE_SGIX 92

int __glXDisp_QueryHyperpipeNetworkSGIX(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXQueryHyperpipeNetworkSGIXReq * req = (xGLXQueryHyperpipeNetworkSGIXReq *) pc;
    xGLXQueryHyperpipeNetworkSGIXReply reply;
    int screen = req->screen;
    void *rdata = NULL;

    int length=0;
    int npipes=0;

    int n= 0;

    if (__glXHyperpipeFuncs &&
        __glXHyperpipeFuncs[screen].queryHyperpipeNetworkFunc != NULL) {
        rdata =
            (__glXHyperpipeFuncs[screen].queryHyperpipeNetworkFunc(screen, &npipes, &n));
    }
    length = __GLX_PAD(n) >> 2;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.length = length;
    reply.n = n;
    reply.npipes = npipes;

    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.n);
        __GLX_SWAP_INT(&reply.npipes);
    }
    WriteToClient(client, sz_xGLXQueryHyperpipeNetworkSGIXReply,
                  (char *) &reply);

    WriteToClient(client, length << 2, (char *)rdata);

    return Success;
}

int __glXDisp_DestroyHyperpipeConfigSGIX (__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXDestroyHyperpipeConfigSGIXReq * req =
        (xGLXDestroyHyperpipeConfigSGIXReq *) pc;
    xGLXDestroyHyperpipeConfigSGIXReply reply;
    int screen = req->screen;
    int  success = GLX_BAD_HYPERPIPE_SGIX;
    int hpId ;

    hpId = req->hpId;


    if (__glXHyperpipeFuncs &&
        __glXHyperpipeFuncs[screen].destroyHyperpipeConfigFunc != NULL) {
        success = __glXHyperpipeFuncs[screen].destroyHyperpipeConfigFunc(screen, hpId);
    }

    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.length = __GLX_PAD(0) >> 2;
    reply.n = 0;
    reply.success = success;


    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
    }
    WriteToClient(client,
                  sz_xGLXDestroyHyperpipeConfigSGIXReply,
                  (char *) &reply);
    return Success;
}

int __glXDisp_QueryHyperpipeConfigSGIX(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXQueryHyperpipeConfigSGIXReq * req =
        (xGLXQueryHyperpipeConfigSGIXReq *) pc;
    xGLXQueryHyperpipeConfigSGIXReply reply;
    int screen = req->screen;
    void *rdata = NULL;
    int length;
    int npipes=0;
    int n= 0;
    int hpId;

    hpId = req->hpId;

    if (__glXHyperpipeFuncs &&
        __glXHyperpipeFuncs[screen].queryHyperpipeConfigFunc != NULL) {
        rdata = __glXHyperpipeFuncs[screen].queryHyperpipeConfigFunc(screen, hpId,&npipes, &n);
    }

    length = __GLX_PAD(n) >> 2;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.length = length;
    reply.n = n;
    reply.npipes = npipes;


    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.length);
        __GLX_SWAP_INT(&reply.n);
        __GLX_SWAP_INT(&reply.npipes);
    }

    WriteToClient(client, sz_xGLXQueryHyperpipeConfigSGIXReply,
                  (char *) &reply);

    WriteToClient(client, length << 2, (char *)rdata);

    return Success;
}

int __glXDisp_HyperpipeConfigSGIX(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXHyperpipeConfigSGIXReq * req =
        (xGLXHyperpipeConfigSGIXReq *) pc;
    xGLXHyperpipeConfigSGIXReply reply;
    int screen = req->screen;
    void *rdata;

    int npipes=0, networkId;
    int hpId=-1;

    networkId = (int)req->networkId;
    npipes = (int)req->npipes;
    rdata = (void *)(req +1);

    if (__glXHyperpipeFuncs &&
        __glXHyperpipeFuncs[screen].hyperpipeConfigFunc != NULL) {
        __glXHyperpipeFuncs[screen].hyperpipeConfigFunc(screen,networkId,
                                                        &hpId, &npipes,
                                                        (void *) rdata);
    }

    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.length = __GLX_PAD(0) >> 2;
    reply.n = 0;
    reply.npipes = npipes;
    reply.hpId = hpId;

    if (client->swapped) {
        __GLX_DECLARE_SWAP_VARIABLES;
        __GLX_SWAP_SHORT(&reply.sequenceNumber);
        __GLX_SWAP_INT(&reply.npipes);
        __GLX_SWAP_INT(&reply.hpId);
    }

    WriteToClient(client, sz_xGLXHyperpipeConfigSGIXReply,
                  (char *) &reply);

    return Success;
}


/************************************************************************/

/*
** No support is provided for the vendor-private requests other than
** allocating the entry points in the dispatch table.
*/

int __glXDisp_VendorPrivate(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLint vendorcode = req->vendorCode;
    __GLXdispatchVendorPrivProcPtr proc;


    proc = (__GLXdispatchVendorPrivProcPtr)
      __glXGetProtocolDecodeFunction(& VendorPriv_dispatch_info,
				     vendorcode, 0);
    if (proc != NULL) {
	(*proc)(cl, (GLbyte*)req);
	return Success;
    }

    cl->client->errorValue = req->vendorCode;
    return __glXError(GLXUnsupportedPrivateRequest);
}

int __glXDisp_VendorPrivateWithReply(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq *req = (xGLXVendorPrivateReq *) pc;
    GLint vendorcode = req->vendorCode;
    __GLXdispatchVendorPrivProcPtr proc;


    proc = (__GLXdispatchVendorPrivProcPtr)
      __glXGetProtocolDecodeFunction(& VendorPriv_dispatch_info,
				     vendorcode, 0);
    if (proc != NULL) {
	return (*proc)(cl, (GLbyte*)req);
    }

    cl->client->errorValue = vendorcode;
    return __glXError(GLXUnsupportedPrivateRequest);
}

int __glXDisp_QueryExtensionsString(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXQueryExtensionsStringReq *req = (xGLXQueryExtensionsStringReq *) pc;
    xGLXQueryExtensionsStringReply reply;
    GLuint screen;
    size_t n, length;
    const char *ptr;
    char *buf;

    screen = req->screen;
    /*
    ** Check if screen exists.
    */
    if (screen >= screenInfo.numScreens) {
	client->errorValue = screen;
	return BadValue;
    }

    ptr = __glXActiveScreens[screen]->GLXextensions;

    n = strlen(ptr) + 1;
    length = __GLX_PAD(n) >> 2;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.length = length;
    reply.n = n;

    /* Allocate buffer to make sure it's a multiple of 4 bytes big.*/
    buf = (char *) xalloc(length << 2);
    if (buf == NULL)
        return BadAlloc;
    memcpy(buf, ptr, n);

    if (client->swapped) {
        glxSwapQueryExtensionsStringReply(client, &reply, buf);
    } else {
        WriteToClient(client, sz_xGLXQueryExtensionsStringReply,(char *)&reply);
        WriteToClient(client, (int)(length << 2), (char *)buf);
    }

    xfree(buf);
    return Success;
}

int __glXDisp_QueryServerString(__GLXclientState *cl, GLbyte *pc)
{
    ClientPtr client = cl->client;
    xGLXQueryServerStringReq *req = (xGLXQueryServerStringReq *) pc;
    xGLXQueryServerStringReply reply;
    int name;
    GLuint screen;
    size_t n, length;
    const char *ptr;
    char *buf;

    name = req->name;
    screen = req->screen;
    /*
    ** Check if screen exists.
    */
    if (screen >= screenInfo.numScreens) {
	client->errorValue = screen;
	return BadValue;
    }
    switch(name) {
	case GLX_VENDOR:
	    ptr = __glXActiveScreens[screen]->GLXvendor;
	    break;
	case GLX_VERSION:
	    ptr = __glXActiveScreens[screen]->GLXversion;
	    break;
	case GLX_EXTENSIONS:
	    ptr = __glXActiveScreens[screen]->GLXextensions;
	    break;
	default:
	    return BadValue; 
    }

    n = strlen(ptr) + 1;
    length = __GLX_PAD(n) >> 2;
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.length = length;
    reply.n = n;

    buf = (char *) xalloc(length << 2);
    if (buf == NULL) {
        return BadAlloc;
    }
    memcpy(buf, ptr, n);

    if (client->swapped) {
        glxSwapQueryServerStringReply(client, &reply, buf);
    } else {
        WriteToClient(client, sz_xGLXQueryServerStringReply, (char *)&reply);
        WriteToClient(client, (int)(length << 2), buf);
    }

    xfree(buf);
    return Success;
}

int __glXDisp_ClientInfo(__GLXclientState *cl, GLbyte *pc)
{
    xGLXClientInfoReq *req = (xGLXClientInfoReq *) pc;
    const char *buf;
   
    cl->GLClientmajorVersion = req->major;
    cl->GLClientminorVersion = req->minor;
    if (cl->GLClientextensions)
	xfree(cl->GLClientextensions);
    buf = (const char *)(req+1);
    cl->GLClientextensions = xstrdup(buf);

    return Success;
}
