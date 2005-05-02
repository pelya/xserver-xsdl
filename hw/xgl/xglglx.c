/*
 * Copyright © 2005 Novell, Inc.
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Novell, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * Novell, Inc. makes no representations about the suitability of this
 * software for any purpose. It is provided "as is" without express or
 * implied warranty.
 *
 * NOVELL, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN
 * NO EVENT SHALL NOVELL, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: David Reveman <davidr@novell.com>
 */

#include "xgl.h"

#ifdef GLXEXT

#include <GL/gl.h>

#include "glxserver.h"
#include "glxdrawable.h"
#include "glxscreens.h"
#include "micmap.h"

extern void
GlxSetVisualConfigs (int	       nconfigs,
		     __GLXvisualConfig *configs,
		     void	       **privates);

extern void
GlxWrapInitVisuals (miInitVisualsProcPtr *);

extern int
GlxInitVisuals (VisualPtr     *visualp,
		DepthPtr      *depthp,
		int	      *nvisualp,
		int	      *ndepthp,
		int	      *rootDepthp,
		VisualID      *defaultVisp,
		unsigned long sizes,
		int	      bitsPerRGB,
		int	      preferredVis);

extern void
__glXFlushContextCache (void);

extern __GLXscreenInfo __glDDXScreenInfo;

extern __glProcTable    __glMesaRenderTable;
extern __glProcTableEXT __glMesaRenderTableEXT;

typedef Bool	      (*GLXScreenProbeProc)    (int screen);
typedef __GLinterface *(*GLXCreateContextProc) (__GLimports      *imports,
						__GLcontextModes *modes,
						__GLinterface    *shareGC);
typedef void	      (*GLXCreateBufferProc)   (__GLXdrawablePrivate *glxPriv);
typedef GLboolean     (*GLXSwapBuffersProc)    (__GLXdrawablePrivate *glxPriv);

typedef struct _xglGLXScreenInfo {
    GLXScreenProbeProc   screenProbe;
    GLXCreateContextProc createContext;
    GLXCreateBufferProc  createBuffer;
} xglGLXScreenInfoRec, *xglGLXScreenInfoPtr;

static xglGLXScreenInfoRec screenInfoPriv;

typedef GLboolean (*GLResizeBuffersProc) (__GLdrawableBuffer   *buffer,
					  GLint		       x,
					  GLint		       y,
					  GLuint	       width,
					  GLuint	       height, 
					  __GLdrawablePrivate  *glPriv,
					  GLuint	       bufferMask);
typedef void	  (*GLFreeBuffersProc)   (__GLdrawablePrivate  *glPriv);
    
typedef struct _xglGLBuffer {
    GLXSwapBuffersProc  swapBuffers;
    GLResizeBuffersProc resizeBuffers;
    GLFreeBuffersProc   freeBuffers;
    ScreenPtr		pScreen;
    DrawablePtr		pDrawable;
    PixmapPtr		pPixmap;
    GCPtr		pGC, swapGC;
    RegionRec		damage;
    void	        *private;
} xglGLBufferRec, *xglGLBufferPtr;

typedef int xglGLXVisualConfigRec, *xglGLXVisualConfigPtr;

typedef struct _xglDisplayList *xglDisplayListPtr;

#define XGL_LIST_OP_CALLS 0
#define XGL_LIST_OP_DRAW  1
#define XGL_LIST_OP_COPY  2
#define XGL_LIST_OP_LIST  3

typedef struct _xglCopyOp {
    void (*copyProc) (struct _xglCopyOp *pOp);
    union {
	struct {
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height;
	    GLenum  type; 
	} pixels;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLint   border; 
	} tex_image_1d;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height;
	    GLint   border; 
	} tex_image_2d;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLint   xoffset;
	    GLint   x;
	    GLint   y;
	    GLsizei width; 
	} tex_sub_image_1d;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLint   xoffset;
	    GLint   yoffset;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height; 
	} tex_sub_image_2d;
	struct {
	    GLenum  target;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	} color_table;
	struct {
	    GLenum  target;
	    GLsizei start;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	} color_sub_table;
	struct {
	    GLenum  target;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	} convolution_filter_1d;
	struct {
	    GLenum  target;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height;
	} convolution_filter_2d;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLint   xoffset;
	    GLint   yoffset;
	    GLint   zoffset;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height; 
	} tex_sub_image_3d;
    } u;
} xglCopyOpRec, *xglCopyOpPtr;

typedef struct _xglListOp {
    int type;
    union {
	GLuint		  name;
	xglCopyOpPtr	  copy;
	xglDisplayListPtr list;
    } u;
} xglListOpRec, *xglListOpPtr;

typedef struct _xglDisplayList {
    xglListOpPtr pOp;
    int		 nOp;
    int		 size;
    int		 refcnt;
} xglDisplayListRec;

typedef struct _xglTexObj {
    GLuint name;
} xglTexObjRec, *xglTexObjPtr;

typedef struct _xglGLAttributes {
    GLbitfield mask;
    GLenum     drawBuffer;
    GLenum     readBuffer;
    xRectangle viewport;
    xRectangle scissor;
    GLboolean  scissorTest;
} xglGLAttributesRec, *xglGLAttributesPtr;

typedef struct _xglGLContext {
    __GLinterface	     iface;
    __GLinterface	     *mIface;
    int			     refcnt;
    struct _xglGLContext     *shared;
    glitz_context_t	     *context;
    __glProcTableEXT	     glRenderTableEXT;
    PFNGLWINDOWPOS3FMESAPROC WindowPos3fMESA;
    Bool		     needInit;
    Bool		     target;
    glitz_surface_t	     *draw;
    int			     tx, ty;
    xglGLBufferPtr	     pDrawBuffer;
    char		     *versionString;
    GLenum		     error;
    xglHashTablePtr	     texObjects;
    xglHashTablePtr	     displayLists;
    GLuint		     list;
    GLenum		     listMode;
    xglDisplayListPtr	     pList;
    GLuint		     groupList;
    xglGLAttributesRec	     attrib;
    xglGLAttributesPtr	     pAttribStack;
    int			     nAttribStack;
} xglGLContextRec, *xglGLContextPtr;

xglGLContextPtr cctx = NULL;

static void
xglViewport (GLint   x,
	     GLint   y,
	     GLsizei width,
	     GLsizei height)
{
    cctx->attrib.viewport.x	 = x;
    cctx->attrib.viewport.y	 = y;
    cctx->attrib.viewport.width  = width;
    cctx->attrib.viewport.height = height;

    glitz_context_set_viewport (cctx->context, x, y, width, height);
}

static void
xglScissor (GLint   x,
	    GLint   y,
	    GLsizei width,
	    GLsizei height)
{
    cctx->attrib.scissor.x	= x;
    cctx->attrib.scissor.y	= y;
    cctx->attrib.scissor.width  = width;
    cctx->attrib.scissor.height = height;

    if (cctx->attrib.scissorTest)
	glitz_context_set_scissor (cctx->context, x, y, width, height);
}

static void
xglDrawBuffer (GLenum mode)
{
    if (mode != cctx->attrib.drawBuffer)
	ErrorF ("NYI xglDrawBuffer: 0x%x\n", mode);
}

static void
xglDisable (GLenum cap)
{
    switch (cap) {
    case GL_SCISSOR_TEST:
	if (cctx->attrib.scissorTest)
	{
	    cctx->attrib.scissorTest = GL_FALSE;
	    glitz_context_set_scissor (cctx->context,
				       0, 0,
				       cctx->pDrawBuffer->pDrawable->width,
				       cctx->pDrawBuffer->pDrawable->height);
	}
	break;
    default:
	glDisable (cap);
    }
}

static void
xglEnable (GLenum cap)
{
    switch (cap) {
    case GL_SCISSOR_TEST:
	cctx->attrib.scissorTest = GL_TRUE;
	glitz_context_set_scissor (cctx->context,
				   cctx->attrib.scissor.x,
				   cctx->attrib.scissor.y,
				   cctx->attrib.scissor.width,
				   cctx->attrib.scissor.height);
	break;
    default:
	glEnable (cap);
    }
}

static void
xglReadBuffer (GLenum mode)
{
    if (mode != cctx->attrib.readBuffer)
	ErrorF ("NYI xglReadBuffer: 0x%x\n", mode);
}

static void
xglPushAttrib (GLbitfield mask)
{
    xglGLAttributesPtr pAttrib;
    GLint	       depth;
    
    glGetIntegerv (GL_MAX_ATTRIB_STACK_DEPTH, &depth);
    
    if (cctx->nAttribStack == depth)
    {
	cctx->error = GL_STACK_OVERFLOW;
	return;
    }
    
    cctx->pAttribStack =
	realloc (cctx->pAttribStack,
		 sizeof (xglGLAttributesRec) * (cctx->nAttribStack + 1));
    
    if (!cctx->pAttribStack)
    {
	cctx->error = GL_OUT_OF_MEMORY;
	return;
    }

    pAttrib = &cctx->pAttribStack[cctx->nAttribStack];
    pAttrib->mask = mask;

    if (mask & GL_COLOR_BUFFER_BIT)
	pAttrib->drawBuffer = cctx->attrib.drawBuffer;

    if (mask & GL_PIXEL_MODE_BIT)
	pAttrib->readBuffer = cctx->attrib.readBuffer;
 
    if (mask & GL_SCISSOR_BIT)
    {
	pAttrib->scissorTest = cctx->attrib.scissorTest;
	pAttrib->scissor = cctx->attrib.scissor;
    }
    else if (mask & GL_ENABLE_BIT)
	pAttrib->scissorTest = cctx->attrib.scissorTest;

    if (mask & GL_VIEWPORT_BIT)
	pAttrib->viewport = cctx->attrib.viewport;

    cctx->nAttribStack++;
    
    glPushAttrib (mask);
}

static void
xglPopAttrib (void)
{
    xglGLAttributesPtr pAttrib;
    GLbitfield	       mask;
    
    if (!cctx->nAttribStack)
    {
	cctx->error = GL_STACK_UNDERFLOW;
	return;
    }

    cctx->nAttribStack--;
    
    pAttrib = &cctx->pAttribStack[cctx->nAttribStack];
    mask = pAttrib->mask;

    if (mask & GL_COLOR_BUFFER_BIT)
	xglDrawBuffer (pAttrib->drawBuffer);
	
    if (mask & GL_PIXEL_MODE_BIT)
	xglReadBuffer (pAttrib->readBuffer);
	
    if (mask & GL_SCISSOR_BIT)
    {
	xglScissor (pAttrib->scissor.x,
		    pAttrib->scissor.y,
		    pAttrib->scissor.width,
		    pAttrib->scissor.height);

	if (pAttrib->scissorTest)
	    xglEnable (GL_SCISSOR_TEST);
	else
	    xglDisable (GL_SCISSOR_TEST);
    }
    else if (mask & GL_ENABLE_BIT)
    {
	if (pAttrib->scissorTest)
	    xglEnable (GL_SCISSOR_TEST);
	else
	    xglDisable (GL_SCISSOR_TEST);
    }
    
    if (mask & GL_VIEWPORT_BIT)
	xglViewport (pAttrib->viewport.x,
		     pAttrib->viewport.y,
		     pAttrib->viewport.width,
		     pAttrib->viewport.height);

    cctx->pAttribStack =
	realloc (cctx->pAttribStack,
		 sizeof (xglGLAttributesRec) * cctx->nAttribStack);
    
    glPopAttrib ();
}

static GLint
xglGetTextureBinding (GLenum pname)
{
    xglTexObjPtr pTexObj;
    GLuint	 key;
    GLint	 texture;
    
    glGetIntegerv (pname, &texture);
    
    key = xglHashFirstEntry (cctx->shared->texObjects);
    while (key)
    {
	pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects, key);
	if (pTexObj && pTexObj->name == texture)
	    return key;
	
	key = xglHashNextEntry (cctx->shared->texObjects, key);
    }
    
    return 0;
}

#define INT_TO_BOOLEAN(I)  ((I) ? GL_TRUE : GL_FALSE)
#define ENUM_TO_BOOLEAN(E) ((E) ? GL_TRUE : GL_FALSE)

static void
xglGetBooleanv (GLenum	  pname,
		GLboolean *params)
{
    switch (pname) {
    case GL_DOUBLEBUFFER:
	params[0] = GL_TRUE;
	break;
    case GL_DRAW_BUFFER:
	params[0] = ENUM_TO_BOOLEAN (cctx->attrib.drawBuffer);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_BOOLEAN (cctx->attrib.readBuffer);
	break;
    case GL_SCISSOR_BOX:
	params[0] = INT_TO_BOOLEAN (cctx->attrib.scissor.x);
	params[1] = INT_TO_BOOLEAN (cctx->attrib.scissor.y);
	params[2] = INT_TO_BOOLEAN (cctx->attrib.scissor.width);
	params[3] = INT_TO_BOOLEAN (cctx->attrib.scissor.height); 
	break;
    case GL_SCISSOR_TEST:
	params[0] = cctx->attrib.scissorTest;
	break;
    case GL_VIEWPORT:
	params[0] = INT_TO_BOOLEAN (cctx->attrib.viewport.x);
	params[1] = INT_TO_BOOLEAN (cctx->attrib.viewport.y);
	params[2] = INT_TO_BOOLEAN (cctx->attrib.viewport.width);
	params[3] = INT_TO_BOOLEAN (cctx->attrib.viewport.height);
	break;
    case GL_TEXTURE_BINDING_1D:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_3D:
	/* should be safe to fall-through here */
    default:
	glGetBooleanv (pname, params);
    }
}

#define ENUM_TO_DOUBLE(E)    ((GLdouble) (E))
#define BOOLEAN_TO_DOUBLE(B) ((B) ? 1.0F : 0.0F)

static void
xglGetDoublev (GLenum	pname,
	       GLdouble *params)
{
    switch (pname) {
    case GL_DOUBLEBUFFER:
	params[0] = BOOLEAN_TO_DOUBLE (GL_TRUE);
	break;
    case GL_DRAW_BUFFER:
	params[0] = ENUM_TO_DOUBLE (cctx->attrib.drawBuffer);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_DOUBLE (cctx->attrib.readBuffer);
	break;
    case GL_SCISSOR_BOX:
	params[0] = cctx->attrib.scissor.x;
	params[1] = cctx->attrib.scissor.y;
	params[2] = cctx->attrib.scissor.width;
	params[3] = cctx->attrib.scissor.height; 
	break;
    case GL_SCISSOR_TEST:
	params[0] = BOOLEAN_TO_DOUBLE (cctx->attrib.scissorTest);
	break;
    case GL_VIEWPORT:
	params[0] = cctx->attrib.viewport.x;
	params[1] = cctx->attrib.viewport.y;
	params[2] = cctx->attrib.viewport.width;
	params[3] = cctx->attrib.viewport.height;
	break;
    case GL_TEXTURE_BINDING_1D:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_3D:
	params[0] = xglGetTextureBinding (pname);
	break;
    default:
	glGetDoublev (pname, params);
    }
}

#define ENUM_TO_FLOAT(E)    ((GLfloat) (E))
#define BOOLEAN_TO_FLOAT(B) ((B) ? 1.0F : 0.0F)

static void
xglGetFloatv (GLenum  pname,
	      GLfloat *params)
{
    switch (pname) {
    case GL_DOUBLEBUFFER:
	params[0] = BOOLEAN_TO_FLOAT (GL_TRUE);
	break;
    case GL_DRAW_BUFFER:
	params[0] = ENUM_TO_FLOAT (cctx->attrib.drawBuffer);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_FLOAT (cctx->attrib.readBuffer);
	break;
    case GL_SCISSOR_BOX:
	params[0] = cctx->attrib.scissor.x;
	params[1] = cctx->attrib.scissor.y;
	params[2] = cctx->attrib.scissor.width;
	params[3] = cctx->attrib.scissor.height; 
	break;
    case GL_SCISSOR_TEST:
	params[0] = BOOLEAN_TO_FLOAT (cctx->attrib.scissorTest);
	break;
    case GL_VIEWPORT:
	params[0] = cctx->attrib.viewport.x;
	params[1] = cctx->attrib.viewport.y;
	params[2] = cctx->attrib.viewport.width;
	params[3] = cctx->attrib.viewport.height;
	break;
    case GL_TEXTURE_BINDING_1D:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_3D:
	params[0] = xglGetTextureBinding (pname);
	break;
    default:
	glGetFloatv (pname, params);
    }
}

#define ENUM_TO_INT(E)    ((GLint) (E))
#define BOOLEAN_TO_INT(B) ((GLint) (B))

static void
xglGetIntegerv (GLenum pname,
		GLint  *params)
{
    switch (pname) {
    case GL_DOUBLEBUFFER:
	params[0] = BOOLEAN_TO_INT (GL_TRUE);
	break;
    case GL_DRAW_BUFFER:
	params[0] = ENUM_TO_INT (cctx->attrib.drawBuffer);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_INT (cctx->attrib.readBuffer);
	break;
    case GL_SCISSOR_BOX:
	params[0] = cctx->attrib.scissor.x;
	params[1] = cctx->attrib.scissor.y;
	params[2] = cctx->attrib.scissor.width;
	params[3] = cctx->attrib.scissor.height; 
	break;
    case GL_SCISSOR_TEST:
	params[0] = BOOLEAN_TO_INT (cctx->attrib.scissorTest);
	break;
    case GL_VIEWPORT:
	params[0] = cctx->attrib.viewport.x;
	params[1] = cctx->attrib.viewport.y;
	params[2] = cctx->attrib.viewport.width;
	params[3] = cctx->attrib.viewport.height;
	break;
    case GL_TEXTURE_BINDING_1D:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_3D:
	params[0] = xglGetTextureBinding (pname);
	break;
    default:
	glGetIntegerv (pname, params);
    }
}

static GLboolean
xglIsEnabled (GLenum cap)
{
    switch (cap) {
    case GL_SCISSOR_TEST:
	return cctx->attrib.scissorTest;
    default:
	return glIsEnabled (cap);
    }
}

static GLenum
xglGetError (void)
{
    if (cctx->error != GL_NO_ERROR)
	return cctx->error;

    return glGetError ();
}

static const GLubyte *
xglGetString (GLenum name)
{
    switch (name) {
    case GL_VERSION:
	if (!cctx->versionString)
	{
	    static const char *version = "1.2 (%s)";
	    const char	      *nativeVersion = glGetString (GL_VERSION);

	    cctx->versionString = xalloc (strlen (version) +
					  strlen (nativeVersion));
	    if (cctx->versionString)
		sprintf (cctx->versionString, version, nativeVersion);
	}
	return cctx->versionString;
    default:
	return glGetString (name);
    }
}

static void
xglGenTextures (GLsizei n,
		GLuint  *textures)
{
    xglTexObjPtr pTexObj;
    GLuint	 name;
    
    name = xglHashFindFreeKeyBlock (cctx->shared->texObjects, n);

    glGenTextures (n, textures);

    while (n--)
    {
	pTexObj = xalloc (sizeof (xglTexObjRec));
	if (pTexObj)
	{
	    pTexObj->name = *textures;
	    xglHashInsert (cctx->shared->texObjects, name, pTexObj);
	}
	else
	{
	    glDeleteTextures (1, textures);
	    cctx->error = GL_OUT_OF_MEMORY;
	}
	
	*textures++ = name++;
    }
}

static void
xglBindTexture (GLenum target,
		GLuint texture)
{
    if (texture)
    {
	xglTexObjPtr pTexObj;
	
	pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects,
						texture);
	if (!pTexObj)
	{
	    pTexObj = xalloc (sizeof (xglTexObjRec));
	    if (!pTexObj)
	    {
		cctx->error = GL_OUT_OF_MEMORY;
		return;
	    }

	    glGenTextures (1, &pTexObj->name);
	    xglHashInsert (cctx->shared->texObjects, texture, pTexObj);
	}
	
	glBindTexture (target, pTexObj->name);
    }
    else
	glBindTexture (target, 0);
}
 
static GLboolean
xglAreTexturesResident (GLsizei	     n,
			const GLuint *textures,
			GLboolean    *residences)
{
    GLboolean allResident = GL_TRUE;
    int	      i, j;
    
    if (n < 0)
    {
	cctx->error = GL_INVALID_VALUE;
	return GL_FALSE;
    }
    
    if (!textures || !residences)
	return GL_FALSE;

    for (i = 0; i < n; i++)
    {
	xglTexObjPtr pTexObj;
	GLboolean    resident;
	
	if (textures[i] == 0)
	{
	    cctx->error = GL_INVALID_VALUE;
	    return GL_FALSE;
	}

	pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects,
						textures[i]);
	if (!pTexObj)
	{
	    cctx->error = GL_INVALID_VALUE;
	    return GL_FALSE;
	}
	
	if (glAreTexturesResident (1, &pTexObj->name, &resident))
	{
	    if (!allResident)
		residences[i] = GL_TRUE;
	}
	else
	{
	    if (allResident)
	    {
		allResident = GL_FALSE;

		for (j = 0; j < i; j++)
		    residences[j] = GL_TRUE;
	    }
	    residences[i] = GL_FALSE;
	}
    }
   
    return allResident;
}

static void
xglDeleteTextures (GLsizei	n,
		   const GLuint *textures)
{
    xglTexObjPtr pTexObj;
    
    while (n--)
    {
	pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects,
						*textures);
	if (pTexObj)
	{
	    glDeleteTextures (1, &pTexObj->name);
	    xglHashRemove (cctx->shared->texObjects, *textures);
	    xfree (pTexObj);
	}
	textures++;
    }
}

static GLboolean
xglIsTexture (GLuint texture)
{
    xglTexObjPtr pTexObj;

    if (!texture)
	return GL_FALSE;
    
    pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects, texture);
    if (pTexObj)
	return glIsTexture (pTexObj->name);

    return GL_FALSE;
}

static void
xglPrioritizeTextures (GLsizei	      n,
		       const GLuint   *textures,
		       const GLclampf *priorities)
{
    xglTexObjPtr pTexObj;
    int		 i;
    
    if (n < 0)
    {
	cctx->error = GL_INVALID_VALUE;
	return;
    }
    
    if (!priorities)
	return;
    
    for (i = 0; i < n; i++)
    {
	if (textures[i] <= 0)
	    continue;
	
	pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects,
						textures[i]);
	if (pTexObj)
	    glPrioritizeTextures (1, &pTexObj->name, &priorities[i]);
    }
}

static xglDisplayListPtr
xglCreateList (void)
{
    xglDisplayListPtr pDisplayList;
    
    pDisplayList = xalloc (sizeof (xglDisplayListRec));
    if (!pDisplayList)
	return NULL;
    
    pDisplayList->pOp    = NULL;
    pDisplayList->nOp    = 0;
    pDisplayList->size   = 0;
    pDisplayList->refcnt = 1;

    return pDisplayList;
}

static void
xglDestroyList (xglDisplayListPtr pDisplayList)
{
    xglListOpPtr pOp = pDisplayList->pOp;
    int		 nOp = pDisplayList->nOp;

    pDisplayList->refcnt--;
    if (pDisplayList->refcnt)
	return;

    while (nOp--)
    {
	switch (pOp->type) {
	case XGL_LIST_OP_CALLS:
	case XGL_LIST_OP_DRAW:
	    glDeleteLists (pOp->u.name, 1);
	    break;    
	case XGL_LIST_OP_COPY:
	    xfree (pOp->u.copy);
	    break;
	case XGL_LIST_OP_LIST:
	    xglDestroyList (pOp->u.list);
	    break;
	}
	
	pOp++;
    }
    
    if (pDisplayList->pOp)
	xfree (pDisplayList->pOp);

    xfree (pDisplayList);
}

static Bool
xglResizeList (xglDisplayListPtr pDisplayList,
	       int		 nOp)
{
    if (pDisplayList->size < nOp)
    {
	int size = pDisplayList->nOp ? pDisplayList->nOp : 4;

	while (size < nOp)
	    size <<= 1;
	
	pDisplayList->pOp = xrealloc (pDisplayList->pOp,
				      sizeof (xglListOpRec) * size);
	if (!pDisplayList->pOp)
	    return FALSE;
	
	pDisplayList->size = size;
    }

    return TRUE;
}

static void
xglStartList (int    type,
	      GLenum mode)
{
    if (!xglResizeList (cctx->pList, cctx->pList->nOp + 1))
    {
	cctx->error = GL_OUT_OF_MEMORY;
	return;
    }
    
    cctx->pList->pOp[cctx->pList->nOp].type   = type;
    cctx->pList->pOp[cctx->pList->nOp].u.name = glGenLists (1);

    glNewList (cctx->pList->pOp[cctx->pList->nOp].u.name, mode);

    cctx->pList->nOp++;
}

static GLuint
xglGenLists (GLsizei range)
{
    xglDisplayListPtr pDisplayList;
    GLuint	      first, name;
    
    first = xglHashFindFreeKeyBlock (cctx->shared->displayLists, range);

    name = first;
    for (name = first; range--; name++)
    {
	pDisplayList = xglCreateList ();
	if (pDisplayList)
	{
	    xglHashInsert (cctx->shared->displayLists, name, pDisplayList);
	}
	else
	{
	    cctx->error = GL_OUT_OF_MEMORY;
	}
    }

    return first;
}

static void
xglNewList (GLuint list,
	    GLenum mode)
{
    if (!list)
    {
	cctx->error = GL_INVALID_VALUE;
	return;
    }

    if (cctx->list)
    {
	cctx->error = GL_INVALID_OPERATION;
	return;
    }

    cctx->pList = xglCreateList ();
    if (!cctx->pList)
    {
	cctx->error = GL_OUT_OF_MEMORY;
	return;
    }

    cctx->list     = list;
    cctx->listMode = mode;

    xglStartList (XGL_LIST_OP_CALLS, mode);
}

static void
xglEndList (void)
{
    xglDisplayListPtr pDisplayList;

    if (!cctx->list)
    {
	cctx->error = GL_INVALID_OPERATION;
	return;
    }

    glEndList ();

    pDisplayList = (xglDisplayListPtr)
	xglHashLookup (cctx->shared->displayLists, cctx->list);
    if (pDisplayList)
    {
	xglHashRemove (cctx->shared->displayLists, cctx->list);
	xglDestroyList (pDisplayList);
    }

    xglHashInsert (cctx->shared->displayLists, cctx->list, cctx->pList);

    cctx->list = 0;
}

#define XGL_GLX_DRAW_DECLARATIONS(pBox, nBox)				   \
    GCPtr	   pGC = cctx->pDrawBuffer->pGC;			   \
    BoxPtr	   (pBox) = REGION_RECTS (pGC->pCompositeClip);		   \
    int		   (nBox) = REGION_NUM_RECTS (pGC->pCompositeClip);	   \
    DrawablePtr	   pDrawable = cctx->pDrawBuffer->pDrawable;		   \
    int		   scissorX1 = cctx->attrib.scissor.x;			   \
    int		   scissorX2 = scissorX1 + cctx->attrib.scissor.width;	   \
    int		   scissorY2 = pDrawable->height - cctx->attrib.scissor.y; \
    int		   scissorY1 = scissorY2 - cctx->attrib.scissor.height

#define XGL_GLX_DRAW_SCISSOR_CLIP(box) \
    if ((box)->x1 < scissorX1)	       \
	(box)->x1 = scissorX1;	       \
    if ((box)->y1 < scissorY1)	       \
	(box)->y1 = scissorY1;	       \
    if ((box)->x2 > scissorX2)	       \
	(box)->x2 = scissorX2;	       \
    if ((box)->y2 > scissorY2)	       \
	(box)->y2 = scissorY2

#define XGL_GLX_DRAW_DAMAGE(pBox, pRegion)	      \
    if (cctx->attrib.drawBuffer == GL_FRONT)	      \
    {						      \
	REGION_INIT (pGC->pScreen, pRegion, pBox, 1); \
	REGION_UNION (pGC->pScreen,		      \
		      &cctx->pDrawBuffer->damage,     \
		      &cctx->pDrawBuffer->damage,     \
		      pRegion);			      \
	REGION_UNINIT (pGC->pScreen, pRegion);	      \
    } 

#define XGL_GLX_DRAW_SET_SCISSOR_BOX(box)		      \
    glitz_context_set_scissor (cctx->context,		      \
			       (box)->x1,		      \
			       pDrawable->height - (box)->y2, \
			       (box)->x2 - (box)->x1,	      \
			       (box)->y2 - (box)->y1)
		    
		    
static void
xglDrawList (GLuint list)
{
    RegionRec region;
    BoxRec    box;
	
    XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);

    while (nBox--)
    {
	box = *pBox++;
	
	XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	
	if (box.x1 < box.x2 && box.y1 < box.y2)
	{
	    XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

	    glCallList (list);

	    XGL_GLX_DRAW_DAMAGE (&box, &region);
	}
    }
}

static void
xglCallDisplayList (xglDisplayListPtr pDisplayList)
{
    if (cctx->list)
    {
	if (!xglResizeList (cctx->pList, cctx->pList->nOp + 1))
	{
	    cctx->error = GL_OUT_OF_MEMORY;
	    return;
	}

	pDisplayList->refcnt++;

	cctx->pList->pOp[cctx->pList->nOp].type   = XGL_LIST_OP_LIST;
	cctx->pList->pOp[cctx->pList->nOp].u.list = pDisplayList;
	cctx->pList->nOp++;
    }
    else
    {
	xglListOpPtr pOp = pDisplayList->pOp;
	int	     nOp = pDisplayList->nOp;

	while (nOp--)
	{
	    switch (pOp->type) {
	    case XGL_LIST_OP_CALLS:
		glCallList (pOp->u.name);
		break;
	    case XGL_LIST_OP_DRAW:
		xglDrawList (pOp->u.name);
		break;
	    case XGL_LIST_OP_COPY:
		(*pOp->u.copy->copyProc) (pOp->u.copy);
		break;
	    case XGL_LIST_OP_LIST:
		xglCallDisplayList (pOp->u.list);
		break;
	    }

	    pOp++;
	}
    }
}

static void
xglCallList (GLuint list)
{
    xglDisplayListPtr pDisplayList;
    
    if (!list)
    {
	cctx->error = GL_INVALID_VALUE;
	return;
    }
       
    pDisplayList = (xglDisplayListPtr)
	xglHashLookup (cctx->shared->displayLists, list);
    if (pDisplayList)
	xglCallDisplayList (pDisplayList);
}

static void
xglCallLists (GLsizei	   n,
	      GLenum	   type,
	      const GLvoid *lists)
{
    xglDisplayListPtr pDisplayList;
    GLuint	      list;
    GLint	      base, i;

    glGetIntegerv (GL_LIST_BASE, &base);

    for (i = 0; i < n; i++)
    {
	switch (type) {
	case GL_BYTE:
	    list = (GLuint) *(((GLbyte *) lists) + n);
	    break;
	case GL_UNSIGNED_BYTE:
	    list = (GLuint) *(((GLubyte *) lists) + n);
	    break;
	case GL_SHORT:
	    list = (GLuint) *(((GLshort *) lists) + n);
	    break;
	case GL_UNSIGNED_SHORT:
	    list = (GLuint) *(((GLushort *) lists) + n);
	    break;
	case GL_INT:
	    list = (GLuint) *(((GLint *) lists) + n);
	    break;
	case GL_UNSIGNED_INT:
	    list = (GLuint) *(((GLuint *) lists) + n);
	    break;
	case GL_FLOAT:
	    list = (GLuint) *(((GLfloat *) lists) + n);
	    break;
	case GL_2_BYTES:
	{
	    GLubyte *ubptr = ((GLubyte *) lists) + 2 * n;
	    list = (GLuint) *ubptr * 256 + (GLuint) *(ubptr + 1);
	} break;
	case GL_3_BYTES:
	{
	    GLubyte *ubptr = ((GLubyte *) lists) + 3 * n;
	    list = (GLuint) * ubptr * 65536
		+ (GLuint) * (ubptr + 1) * 256
		+ (GLuint) * (ubptr + 2);
	} break;
	case GL_4_BYTES:
	{
	    GLubyte *ubptr = ((GLubyte *) lists) + 4 * n;
	    list = (GLuint) * ubptr * 16777216
		+ (GLuint) * (ubptr + 1) * 65536
		+ (GLuint) * (ubptr + 2) * 256
		+ (GLuint) * (ubptr + 3);
	} break;
	default:
	    cctx->error = GL_INVALID_ENUM;
	    return;
	}
	
	pDisplayList = (xglDisplayListPtr)
	    xglHashLookup (cctx->shared->displayLists, base + list);
	if (pDisplayList)
	    xglCallDisplayList (pDisplayList);
    }
}

static void
xglDeleteLists (GLuint  list,
		GLsizei range)
{
    xglDisplayListPtr pDisplayList;
    GLint	      i;
    
    if (range < 0)
    {
	cctx->error = GL_INVALID_VALUE;
	return;
    }
    
    for (i = 0; i < range; i++)
    {
	pDisplayList = (xglDisplayListPtr)
	    xglHashLookup (cctx->shared->displayLists, list + i);
	if (pDisplayList)
	{
	    xglHashRemove (cctx->shared->displayLists, list + i);
	    xglDestroyList (pDisplayList);
	}
    }
}

static GLboolean
xglIsList (GLuint list)
{
    xglDisplayListPtr pDisplayList;

    if (!list)
	return GL_FALSE;
    
    pDisplayList = (xglDisplayListPtr)
	xglHashLookup (cctx->shared->displayLists, list);
    if (pDisplayList)
	return GL_TRUE;

    return GL_FALSE;
}

static void
xglFlush (void)
{
    xglGLBufferPtr pBuffer = cctx->pDrawBuffer;
    
    glFlush ();

    if (REGION_NOTEMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage))
    {
	DamageDamageRegion (pBuffer->pDrawable, &pBuffer->damage);
	REGION_EMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage);
    }
}

static void
xglFinish (void)
{
    xglGLBufferPtr pBuffer = cctx->pDrawBuffer;
    
    glFinish ();

    if (REGION_NOTEMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage))
    {
	DamageDamageRegion (pBuffer->pDrawable, &pBuffer->damage);
	REGION_EMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage);
    }
}

static void
xglClear (GLbitfield mask)
{
    GLenum mode;
    
    if (cctx->list)
    {
	glEndList ();
	xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
	glClear (mask);
	glEndList ();
	
	mode = cctx->listMode;
    }
    else
	mode = GL_COMPILE_AND_EXECUTE;

    if (mode == GL_COMPILE_AND_EXECUTE)
    {
	RegionRec region;
	BoxRec	  box;

	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);
    
	while (nBox--)
	{
	    box = *pBox++;
	    
	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		glClear (mask);
		
		if (mask & GL_COLOR_BUFFER_BIT)
		    XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }

    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglAccum (GLenum  op,
	  GLfloat value)
{
    if (op == GL_RETURN)
    {
	GLenum listMode;
    
	if (cctx->list)
	{
	    glEndList ();
	    xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
	    glAccum (GL_RETURN, value);
	    glEndList ();
	    
	    listMode = cctx->listMode;
	}
	else
	    listMode = GL_COMPILE_AND_EXECUTE;
	
	if (listMode == GL_COMPILE_AND_EXECUTE)
	{
	    RegionRec region;
	    BoxRec    box;

	    XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);
	
	    while (nBox--)
	    {
		box = *pBox++;
		
		XGL_GLX_DRAW_SCISSOR_CLIP (&box);
		
		if (box.x1 < box.x2 && box.y1 < box.y2)
		{
		    XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		    glAccum (GL_RETURN, value);
		    
		    XGL_GLX_DRAW_DAMAGE (&box, &region);
		}
	    }
	}
	
	if (cctx->list)
	    xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
    }
    else
	glAccum (op, value);
}

static void
xglDrawArrays (GLenum  mode,
	       GLint   first,
	       GLsizei count)
{
    GLenum listMode;
    
    if (cctx->list)
    {
	glEndList ();
	xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
	glDrawArrays (mode, first, count);
	glEndList ();
	    
	listMode = cctx->listMode;
    }
    else
	listMode = GL_COMPILE_AND_EXECUTE;
	
    if (listMode == GL_COMPILE_AND_EXECUTE)
    {
	RegionRec region;
	BoxRec    box;

	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		glDrawArrays (mode, first, count);
	
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }

    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglDrawElements (GLenum	      mode,
		 GLsizei      count,
		 GLenum	      type,
		 const GLvoid *indices)
{
    GLenum listMode;
    
    if (cctx->list)
    {
	glEndList ();
	xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
	glDrawElements (mode, count, type, indices);
	glEndList ();
	    
	listMode = cctx->listMode;
    }
    else
	listMode = GL_COMPILE_AND_EXECUTE;
	
    if (listMode == GL_COMPILE_AND_EXECUTE)
    {
	RegionRec region;
	BoxRec    box;

	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		glDrawElements (mode, count, type, indices);
	
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }

    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglDrawPixels (GLsizei	    width,
	       GLsizei	    height,
	       GLenum	    format,
	       GLenum	    type,
	       const GLvoid *pixels)
{
    GLenum listMode;
    
    if (cctx->list)
    {
	glEndList ();
	xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
	glDrawPixels (width, height, format, type, pixels);
	glEndList ();
	    
	listMode = cctx->listMode;
    }
    else
	listMode = GL_COMPILE_AND_EXECUTE;
	
    if (listMode == GL_COMPILE_AND_EXECUTE)
    {
	RegionRec region;
	BoxRec    box;
	    
	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		glDrawPixels (width, height, format, type, pixels);

		if (format != GL_STENCIL_INDEX)
		    XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }

    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglBitmap (GLsizei	 width,
	   GLsizei	 height,
	   GLfloat	 xorig,
	   GLfloat	 yorig,
	   GLfloat	 xmove,
	   GLfloat	 ymove,
	   const GLubyte *bitmap)
{
    GLenum listMode;
    
    if (cctx->list)
    {
	glEndList ();
	xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
	glBitmap (width, height, xorig, yorig, xmove, ymove, bitmap);
	glEndList ();
	    
	listMode = cctx->listMode;
    }
    else
	listMode = GL_COMPILE_AND_EXECUTE;
	
    if (listMode == GL_COMPILE_AND_EXECUTE)
    {
	RegionRec region;
	BoxRec    box;

	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		glBitmap (width, height, xorig, yorig, xmove, ymove, bitmap);
		
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }
    
    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglRectdv (const GLdouble *v1,
	   const GLdouble *v2)
{
    GLenum listMode;
    
    if (cctx->list)
    {
	glEndList ();
	xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
	glRectdv (v1, v2);
	glEndList ();
	    
	listMode = cctx->listMode;
    }
    else
	listMode = GL_COMPILE_AND_EXECUTE;
	
    if (listMode == GL_COMPILE_AND_EXECUTE)
    {
	RegionRec region;
	BoxRec    box;

	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		glRectdv (v1, v2);
		
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }

    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglRectfv (const GLfloat *v1,
	   const GLfloat *v2)
{
    GLdouble dv1[2];
    GLdouble dv2[2];

    dv1[0] = (GLdouble) v1[0];
    dv1[1] = (GLdouble) v1[1];
    dv2[0] = (GLdouble) v2[0];
    dv2[1] = (GLdouble) v2[1];

    xglRectdv (dv1, dv2);
}

static void
xglRectiv (const GLint *v1,
	   const GLint *v2)
{
    GLdouble dv1[2];
    GLdouble dv2[2];

    dv1[0] = (GLdouble) v1[0];
    dv1[1] = (GLdouble) v1[1];
    dv2[0] = (GLdouble) v2[0];
    dv2[1] = (GLdouble) v2[1];

    xglRectdv (dv1, dv2);
}

static void
xglRectsv (const GLshort *v1,
	   const GLshort *v2)
{
    GLdouble dv1[2];
    GLdouble dv2[2];

    dv1[0] = (GLdouble) v1[0];
    dv1[1] = (GLdouble) v1[1];
    dv2[0] = (GLdouble) v2[0];
    dv2[1] = (GLdouble) v2[1];

    xglRectdv (dv1, dv2);
}

static void
xglBegin (GLenum mode)
{
    if (cctx->list)
    {
	glEndList ();
	xglStartList (XGL_LIST_OP_DRAW, GL_COMPILE);
    }
    else
    {
	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);
	    
	if (nBox == 1)
	{
	    BoxRec box = *pBox;

	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);
	}
	else
	{
	    if (!cctx->groupList)
		cctx->groupList = glGenLists (1);
	    
	    glNewList (cctx->groupList, GL_COMPILE);
	}
    }
    
    glBegin (mode);
}

static void
xglEnd (void)
{
    glEnd ();
    
    if (!cctx->list || cctx->listMode == GL_COMPILE_AND_EXECUTE)
    {
	RegionRec region;
	BoxRec	  box;
	GLuint	  list;

	XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);
	
	if (cctx->list)
	{
	    list = cctx->pList->pOp[cctx->pList->nOp - 1].u.name;
	}
	else
	{
	    if (nBox == 1)
	    {
		box = *pBox++;
	    
		XGL_GLX_DRAW_SCISSOR_CLIP (&box);
		XGL_GLX_DRAW_DAMAGE (&box, &region);
		return;
	    }
	
	    list = cctx->groupList;
	}

	glEndList ();

	while (nBox--)
	{
	    box = *pBox++;
	    
	    XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

		glCallList (list);
		
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }
	
    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglCopyOp (xglCopyOpPtr pOp)
{
    if (cctx->list)
    {
	xglCopyOpPtr pCopyOp;

	pCopyOp = xalloc (sizeof (xglCopyOpRec));
	if (!pCopyOp)
	{
	    cctx->error = GL_OUT_OF_MEMORY;
	    return;
	}
	
	if (!xglResizeList (cctx->pList, cctx->pList->nOp + 1))
	{
	    xfree (pCopyOp);
	    cctx->error = GL_OUT_OF_MEMORY;
	    return;
	}

	glEndList ();

	*pCopyOp = *pOp;
	
	cctx->pList->pOp[cctx->pList->nOp].type   = XGL_LIST_OP_COPY;
	cctx->pList->pOp[cctx->pList->nOp].u.copy = pCopyOp;
	cctx->pList->nOp++;

	if (cctx->listMode == GL_COMPILE_AND_EXECUTE)
	    (*pOp->copyProc) (pOp);

	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
    }
    else
	(*pOp->copyProc) (pOp);   
}

static void
xglDoCopyPixels (xglCopyOpPtr pOp)
{
    RegionRec region;
    BoxRec    box;
	    
    XGL_GLX_DRAW_DECLARATIONS (pBox, nBox);

    while (nBox--)
    {
	box = *pBox++;
	
	XGL_GLX_DRAW_SCISSOR_CLIP (&box);
	
	if (box.x1 < box.x2 && box.y1 < box.y2)
	{
	    XGL_GLX_DRAW_SET_SCISSOR_BOX (&box);

	    glCopyPixels (cctx->tx + pOp->u.pixels.x,
			  cctx->ty + pOp->u.pixels.y,
			  pOp->u.pixels.width,
			  pOp->u.pixels.height,
			  pOp->u.pixels.type);

	    if (pOp->u.pixels.type == GL_COLOR)
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	}
    }
}

static void
xglCopyPixels (GLint   x,
	       GLint   y,
	       GLsizei width,
	       GLsizei height,
	       GLenum  type)
{
    xglCopyOpRec copy;
    
    copy.copyProc	 = xglDoCopyPixels;
    copy.u.pixels.x	 = x;
    copy.u.pixels.y	 = y;
    copy.u.pixels.width  = width;
    copy.u.pixels.height = height;
    copy.u.pixels.type	 = type;

    xglCopyOp (&copy);   
}

static void
xglReadPixels (GLint   x,
	       GLint   y,
	       GLsizei width,
	       GLsizei height,
	       GLenum  format,
	       GLenum  type,
	       GLvoid  *pixels)
{
    glReadPixels (cctx->tx + x, cctx->ty + y,
		  width, height, format, type, pixels);
}

static void
xglDoCopyTexImage1D (xglCopyOpPtr pOp)
{
    glCopyTexImage1D (pOp->u.tex_image_1d.target,
		      pOp->u.tex_image_1d.level,
		      pOp->u.tex_image_1d.internalformat,
		      cctx->tx + pOp->u.tex_image_1d.x,
		      cctx->ty + pOp->u.tex_image_1d.y,
		      pOp->u.tex_image_1d.width,
		      pOp->u.tex_image_1d.border);
}

static void
xglCopyTexImage1D (GLenum  target,
		   GLint   level,
		   GLenum  internalformat,
		   GLint   x,
		   GLint   y,
		   GLsizei width,
		   GLint   border)
{
    xglCopyOpRec copy;
    
    copy.copyProc		       = xglDoCopyTexImage1D;
    copy.u.tex_image_1d.target	       = target;
    copy.u.tex_image_1d.level	       = level;
    copy.u.tex_image_1d.internalformat = internalformat;
    copy.u.tex_image_1d.x	       = x;
    copy.u.tex_image_1d.y	       = y;
    copy.u.tex_image_1d.width	       = width;
    copy.u.tex_image_1d.border	       = border;

    xglCopyOp (&copy);
}

static void
xglDoCopyTexImage2D (xglCopyOpPtr pOp)
{
    glCopyTexImage2D (pOp->u.tex_image_2d.target,
		      pOp->u.tex_image_2d.level,
		      pOp->u.tex_image_2d.internalformat,
		      cctx->tx + pOp->u.tex_image_2d.x,
		      cctx->ty + pOp->u.tex_image_2d.y,
		      pOp->u.tex_image_2d.width,
		      pOp->u.tex_image_2d.height,
		      pOp->u.tex_image_2d.border);
}

static void
xglCopyTexImage2D (GLenum  target,
		   GLint   level,
		   GLenum  internalformat,
		   GLint   x,
		   GLint   y,
		   GLsizei width,
		   GLsizei height,
		   GLint   border)
{
    xglCopyOpRec copy;
    
    copy.copyProc		       = xglDoCopyTexImage2D;
    copy.u.tex_image_2d.target	       = target;
    copy.u.tex_image_2d.level	       = level;
    copy.u.tex_image_2d.internalformat = internalformat;
    copy.u.tex_image_2d.x	       = x;
    copy.u.tex_image_2d.y	       = y;
    copy.u.tex_image_2d.width	       = width;
    copy.u.tex_image_2d.height	       = height;
    copy.u.tex_image_2d.border	       = border;

    xglCopyOp (&copy);
}

static void
xglDoCopyTexSubImage1D (xglCopyOpPtr pOp)
{
    glCopyTexSubImage1D (pOp->u.tex_sub_image_1d.target,
			 pOp->u.tex_sub_image_1d.level,
			 pOp->u.tex_sub_image_1d.xoffset,
			 cctx->tx + pOp->u.tex_sub_image_1d.x,
			 cctx->ty + pOp->u.tex_sub_image_1d.y,
			 pOp->u.tex_sub_image_1d.width);
}

static void
xglCopyTexSubImage1D (GLenum  target,
		      GLint   level,
		      GLint   xoffset,
		      GLint   x,
		      GLint   y,
		      GLsizei width)
{
    xglCopyOpRec copy;
    
    copy.copyProc		    = xglDoCopyTexSubImage1D;
    copy.u.tex_sub_image_1d.target  = target;
    copy.u.tex_sub_image_1d.level   = level;
    copy.u.tex_sub_image_1d.xoffset = xoffset;
    copy.u.tex_sub_image_1d.x	    = x;
    copy.u.tex_sub_image_1d.y	    = y;
    copy.u.tex_sub_image_1d.width   = width;

    xglCopyOp (&copy);
}

static void
xglDoCopyTexSubImage2D (xglCopyOpPtr pOp)
{
    glCopyTexSubImage2D (pOp->u.tex_sub_image_2d.target,
			 pOp->u.tex_sub_image_2d.level,
			 pOp->u.tex_sub_image_2d.xoffset,
			 pOp->u.tex_sub_image_2d.yoffset,
			 cctx->tx + pOp->u.tex_sub_image_2d.x,
			 cctx->ty + pOp->u.tex_sub_image_2d.y,
			 pOp->u.tex_sub_image_2d.width,
			 pOp->u.tex_sub_image_2d.height);
}

static void
xglCopyTexSubImage2D (GLenum  target,
		      GLint   level,
		      GLint   xoffset,
		      GLint   yoffset,
		      GLint   x,
		      GLint   y,
		      GLsizei width,
		      GLsizei height)
{
    xglCopyOpRec copy;
    
    copy.copyProc		    = xglDoCopyTexSubImage2D;
    copy.u.tex_sub_image_2d.target  = target;
    copy.u.tex_sub_image_2d.level   = level;
    copy.u.tex_sub_image_2d.xoffset = xoffset;
    copy.u.tex_sub_image_2d.yoffset = yoffset;
    copy.u.tex_sub_image_2d.x	    = x;
    copy.u.tex_sub_image_2d.y	    = y;
    copy.u.tex_sub_image_2d.width   = width;
    copy.u.tex_sub_image_2d.height  = height;

    xglCopyOp (&copy);
}    

static void
xglDoCopyColorTable (xglCopyOpPtr pOp)
{
    glCopyColorTable (pOp->u.color_table.target,
		      pOp->u.color_table.internalformat,
		      cctx->tx + pOp->u.color_table.x,
		      cctx->ty + pOp->u.color_table.y,
		      pOp->u.color_table.width);
}

static void
xglCopyColorTable (GLenum  target,
		   GLenum  internalformat,
		   GLint   x,
		   GLint   y,
		   GLsizei width)
{
    xglCopyOpRec copy;
    
    copy.copyProc		      = xglDoCopyColorTable;
    copy.u.color_table.target	      = target;
    copy.u.color_table.internalformat = internalformat;
    copy.u.color_table.x	      = x;
    copy.u.color_table.y	      = y;
    copy.u.color_table.width	      = width;
    
    xglCopyOp (&copy);
}

static void
xglDoCopyColorSubTable (xglCopyOpPtr pOp)
{
    glCopyColorTable (pOp->u.color_sub_table.target,
		      pOp->u.color_sub_table.start,
		      cctx->tx + pOp->u.color_sub_table.x,
		      cctx->ty + pOp->u.color_sub_table.y,
		      pOp->u.color_sub_table.width);
}

static void
xglCopyColorSubTable (GLenum  target,
		      GLsizei start,
		      GLint   x,
		      GLint   y,
		      GLsizei width)
{
    xglCopyOpRec copy;
    
    copy.copyProc		  = xglDoCopyColorSubTable;
    copy.u.color_sub_table.target = target;
    copy.u.color_sub_table.start  = start;
    copy.u.color_sub_table.x	  = x;
    copy.u.color_sub_table.y	  = y;
    copy.u.color_sub_table.width  = width;
    
    xglCopyOp (&copy);
}

static void
xglDoCopyConvolutionFilter1D (xglCopyOpPtr pOp)
{
    glCopyConvolutionFilter1D (pOp->u.convolution_filter_1d.target,
			       pOp->u.convolution_filter_1d.internalformat,
			       cctx->tx + pOp->u.convolution_filter_1d.x,
			       cctx->ty + pOp->u.convolution_filter_1d.y,
			       pOp->u.convolution_filter_1d.width);
}

static void
xglCopyConvolutionFilter1D (GLenum  target,
			    GLenum  internalformat,
			    GLint   x,
			    GLint   y,
			    GLsizei width)
{
    xglCopyOpRec copy;
    
    copy.copyProc				= xglDoCopyConvolutionFilter1D;
    copy.u.convolution_filter_1d.target		= target;
    copy.u.convolution_filter_1d.internalformat = internalformat;
    copy.u.convolution_filter_1d.x		= x;
    copy.u.convolution_filter_1d.y		= y;
    copy.u.convolution_filter_1d.width		= width;
    
    xglCopyOp (&copy);
}

static void
xglDoCopyConvolutionFilter2D (xglCopyOpPtr pOp)
{
    glCopyConvolutionFilter2D (pOp->u.convolution_filter_2d.target,
			       pOp->u.convolution_filter_2d.internalformat,
			       cctx->tx + pOp->u.convolution_filter_2d.x,
			       cctx->ty + pOp->u.convolution_filter_2d.y,
			       pOp->u.convolution_filter_2d.width,
			       pOp->u.convolution_filter_2d.height);
}

static void
xglCopyConvolutionFilter2D (GLenum  target,
			    GLenum  internalformat,
			    GLint   x,
			    GLint   y,
			    GLsizei width,
			    GLsizei height)
{
    xglCopyOpRec copy;
    
    copy.copyProc				= xglDoCopyConvolutionFilter2D;
    copy.u.convolution_filter_2d.target		= target;
    copy.u.convolution_filter_2d.internalformat = internalformat;
    copy.u.convolution_filter_2d.x		= x;
    copy.u.convolution_filter_2d.y		= y;
    copy.u.convolution_filter_2d.width		= width;
    copy.u.convolution_filter_2d.height		= height;
    
    xglCopyOp (&copy);
}

static void
xglDoCopyTexSubImage3D (xglCopyOpPtr pOp)
{
    glCopyTexSubImage3D (pOp->u.tex_sub_image_3d.target,
			 pOp->u.tex_sub_image_3d.level,
			 pOp->u.tex_sub_image_3d.xoffset,
			 pOp->u.tex_sub_image_3d.yoffset,
			 pOp->u.tex_sub_image_3d.zoffset,
			 cctx->tx + pOp->u.tex_sub_image_3d.x,
			 cctx->ty + pOp->u.tex_sub_image_3d.y,
			 pOp->u.tex_sub_image_3d.width,
			 pOp->u.tex_sub_image_3d.height);
}

static void
xglCopyTexSubImage3D (GLenum  target,
		      GLint   level,
		      GLint   xoffset,
		      GLint   yoffset,
		      GLint   zoffset,
		      GLint   x,
		      GLint   y,
		      GLsizei width,
		      GLsizei height)
{
    xglCopyOpRec copy;
    
    copy.copyProc		    = xglDoCopyTexSubImage3D;
    copy.u.tex_sub_image_3d.target  = target;
    copy.u.tex_sub_image_3d.level   = level;
    copy.u.tex_sub_image_3d.xoffset = xoffset;
    copy.u.tex_sub_image_3d.yoffset = yoffset;
    copy.u.tex_sub_image_3d.zoffset = zoffset;
    copy.u.tex_sub_image_3d.x	    = x;
    copy.u.tex_sub_image_3d.y	    = y;
    copy.u.tex_sub_image_3d.width   = width;
    copy.u.tex_sub_image_3d.height  = height;

    xglCopyOp (&copy);
}    

__glProcTable __glNativeRenderTable = {
    xglNewList, /* glNewList */
    xglEndList, /* glEndList */
    xglCallList, /* glCallList */
    xglCallLists, /* glCallLists */
    xglDeleteLists, /* glDeleteLists */
    xglGenLists, /* glGenLists */
    glListBase,
    xglBegin, /* glBegin */
    xglBitmap, /* glBitmap */
    glColor3bv,
    glColor3dv,
    glColor3fv,
    glColor3iv,
    glColor3sv,
    glColor3ubv,
    glColor3uiv,
    glColor3usv,
    glColor4bv,
    glColor4dv,
    glColor4fv,
    glColor4iv,
    glColor4sv,
    glColor4ubv,
    glColor4uiv,
    glColor4usv,
    glEdgeFlagv,
    xglEnd, /* glEnd */
    glIndexdv,
    glIndexfv,
    glIndexiv,
    glIndexsv,
    glNormal3bv,
    glNormal3dv,
    glNormal3fv,
    glNormal3iv,
    glNormal3sv,
    glRasterPos2dv,
    glRasterPos2fv,
    glRasterPos2iv,
    glRasterPos2sv,
    glRasterPos3dv,
    glRasterPos3fv,
    glRasterPos3iv,
    glRasterPos3sv,
    glRasterPos4dv,
    glRasterPos4fv,
    glRasterPos4iv,
    glRasterPos4sv,
    xglRectdv, /* glRectdv */
    xglRectfv, /* glRectfv */
    xglRectiv, /* glRectiv */
    xglRectsv, /* glRectsv */
    glTexCoord1dv,
    glTexCoord1fv,
    glTexCoord1iv,
    glTexCoord1sv,
    glTexCoord2dv,
    glTexCoord2fv,
    glTexCoord2iv,
    glTexCoord2sv,
    glTexCoord3dv,
    glTexCoord3fv,
    glTexCoord3iv,
    glTexCoord3sv,
    glTexCoord4dv,
    glTexCoord4fv,
    glTexCoord4iv,
    glTexCoord4sv,
    glVertex2dv,
    glVertex2fv,
    glVertex2iv,
    glVertex2sv,
    glVertex3dv,
    glVertex3fv,
    glVertex3iv,
    glVertex3sv,
    glVertex4dv,
    glVertex4fv,
    glVertex4iv,
    glVertex4sv,
    glClipPlane,
    glColorMaterial,
    glCullFace,
    glFogf,
    glFogfv,
    glFogi,
    glFogiv,
    glFrontFace,
    glHint,
    glLightf,
    glLightfv,
    glLighti,
    glLightiv,
    glLightModelf,
    glLightModelfv,
    glLightModeli,
    glLightModeliv,
    glLineStipple,
    glLineWidth,
    glMaterialf,
    glMaterialfv,
    glMateriali,
    glMaterialiv,
    glPointSize,
    glPolygonMode,
    glPolygonStipple,
    xglScissor, /* glScissor */
    glShadeModel,
    glTexParameterf,
    glTexParameterfv,
    glTexParameteri,
    glTexParameteriv,
    glTexImage1D,
    glTexImage2D,
    glTexEnvf,
    glTexEnvfv,
    glTexEnvi,
    glTexEnviv,
    glTexGend,
    glTexGendv,
    glTexGenf,
    glTexGenfv,
    glTexGeni,
    glTexGeniv,
    glFeedbackBuffer,
    glSelectBuffer,
    glRenderMode,
    glInitNames,
    glLoadName,
    glPassThrough,
    glPopName,
    glPushName,
    xglDrawBuffer, /* glDrawBuffer */
    xglClear, /* glClear */
    glClearAccum,
    glClearIndex,
    glClearColor,
    glClearStencil,
    glClearDepth,
    glStencilMask,
    glColorMask,
    glDepthMask,
    glIndexMask,
    xglAccum, /* glAccum */
    xglDisable, /* glDisable */
    xglEnable, /* glEnable */
    xglFinish, /* glFinish */
    xglFlush, /* glFlush */
    xglPopAttrib, /* glPopAttrib */
    xglPushAttrib, /* glPushAttrib */
    glMap1d,
    glMap1f,
    glMap2d,
    glMap2f,
    glMapGrid1d,
    glMapGrid1f,
    glMapGrid2d,
    glMapGrid2f,
    glEvalCoord1dv,
    glEvalCoord1fv,
    glEvalCoord2dv,
    glEvalCoord2fv,
    glEvalMesh1,
    glEvalPoint1,
    glEvalMesh2,
    glEvalPoint2,
    glAlphaFunc,
    glBlendFunc,
    glLogicOp,
    glStencilFunc,
    glStencilOp,
    glDepthFunc,
    glPixelZoom,
    glPixelTransferf,
    glPixelTransferi,
    glPixelStoref,
    glPixelStorei,
    glPixelMapfv,
    glPixelMapuiv,
    glPixelMapusv,
    xglReadBuffer, /* glReadBuffer */
    xglCopyPixels, /* glCopyPixels */
    xglReadPixels, /* glReadPixels */
    xglDrawPixels, /* glDrawPixels */
    xglGetBooleanv, /* glGetBooleanv */
    glGetClipPlane,
    xglGetDoublev, /* glGetDoublev */
    xglGetError, /* glGetError */
    xglGetFloatv, /* glGetFloatv */
    xglGetIntegerv, /* glGetIntegerv */
    glGetLightfv,
    glGetLightiv,
    glGetMapdv,
    glGetMapfv,
    glGetMapiv,
    glGetMaterialfv,
    glGetMaterialiv,
    glGetPixelMapfv,
    glGetPixelMapuiv,
    glGetPixelMapusv,
    glGetPolygonStipple,
    xglGetString, /* glGetString */
    glGetTexEnvfv,
    glGetTexEnviv,
    glGetTexGendv,
    glGetTexGenfv,
    glGetTexGeniv,
    glGetTexImage,
    glGetTexParameterfv,
    glGetTexParameteriv,
    glGetTexLevelParameterfv,
    glGetTexLevelParameteriv,
    xglIsEnabled, /* glIsEnabled */
    xglIsList, /* glIsList */
    glDepthRange,
    glFrustum,
    glLoadIdentity,
    glLoadMatrixf,
    glLoadMatrixd,
    glMatrixMode,
    glMultMatrixf,
    glMultMatrixd,
    glOrtho,
    glPopMatrix,
    glPushMatrix,
    glRotated,
    glRotatef,
    glScaled,
    glScalef,
    glTranslated,
    glTranslatef,
    xglViewport, /* glViewport */
    glArrayElement,
    xglBindTexture, /* glBindTexture */
    glColorPointer,
    glDisableClientState,
    xglDrawArrays, /* glDrawArrays */
    xglDrawElements, /* glDrawElements */
    glEdgeFlagPointer,
    glEnableClientState,
    glIndexPointer,
    glIndexubv,
    glInterleavedArrays,
    glNormalPointer,
    glPolygonOffset,
    glTexCoordPointer,
    glVertexPointer,
    xglAreTexturesResident, /* glAreTexturesResident */
    xglCopyTexImage1D, /* glCopyTexImage1D */
    xglCopyTexImage2D, /* glCopyTexImage2D */
    xglCopyTexSubImage1D, /* glCopyTexSubImage1D */
    xglCopyTexSubImage2D, /* glCopyTexSubImage2D */
    xglDeleteTextures, /* glDeleteTextures */
    xglGenTextures, /* glGenTextures */
    glGetPointerv,
    xglIsTexture, /* glIsTexture */
    xglPrioritizeTextures, /* glPrioritizeTextures */
    glTexSubImage1D,
    glTexSubImage2D,
    glPopClientAttrib,
    glPushClientAttrib,
    glBlendColor,
    glBlendEquation,
    glColorTable,
    glColorTableParameterfv,
    glColorTableParameteriv,
    xglCopyColorTable, /* glCopyColorTable */
    glGetColorTable,
    glGetColorTableParameterfv,
    glGetColorTableParameteriv,
    glColorSubTable,
    xglCopyColorSubTable, /* glCopyColorSubTable */
    glConvolutionFilter1D,
    glConvolutionFilter2D,
    glConvolutionParameterf,
    glConvolutionParameterfv,
    glConvolutionParameteri,
    glConvolutionParameteriv,
    xglCopyConvolutionFilter1D, /* glCopyConvolutionFilter1D */
    xglCopyConvolutionFilter2D, /* glCopyConvolutionFilter2D */
    glGetConvolutionFilter,
    glGetConvolutionParameterfv,
    glGetConvolutionParameteriv,
    glGetSeparableFilter,
    glSeparableFilter2D,
    glGetHistogram,
    glGetHistogramParameterfv,
    glGetHistogramParameteriv,
    glGetMinmax,
    glGetMinmaxParameterfv,
    glGetMinmaxParameteriv,
    glHistogram,
    glMinmax,
    glResetHistogram,
    glResetMinmax,
    glTexImage3D,
    glTexSubImage3D,
    xglCopyTexSubImage3D /* glCopyTexSubImage3D */
};

/* GL_ARB_multitexture */
static void
xglNoOpActiveTextureARB (GLenum texture) {}
static void
xglNoOpClientActiveTextureARB (GLenum texture) {}
static void
xglNoOpMultiTexCoord1dvARB (GLenum target, const GLdouble *v) {}
static void
xglNoOpMultiTexCoord1fvARB (GLenum target, const GLfloat *v) {}
static void
xglNoOpMultiTexCoord1ivARB (GLenum target, const GLint *v) {}
static void
xglNoOpMultiTexCoord1svARB (GLenum target, const GLshort *v) {}
static void
xglNoOpMultiTexCoord2dvARB (GLenum target, const GLdouble *v) {}
static void
xglNoOpMultiTexCoord2fvARB (GLenum target, const GLfloat *v) {}
static void
xglNoOpMultiTexCoord2ivARB (GLenum target, const GLint *v) {}
static void
xglNoOpMultiTexCoord2svARB (GLenum target, const GLshort *v) {}
static void
xglNoOpMultiTexCoord3dvARB (GLenum target, const GLdouble *v) {}
static void
xglNoOpMultiTexCoord3fvARB (GLenum target, const GLfloat *v) {}
static void
xglNoOpMultiTexCoord3ivARB (GLenum target, const GLint *v) {}
static void
xglNoOpMultiTexCoord3svARB (GLenum target, const GLshort *v) {}
static void
xglNoOpMultiTexCoord4dvARB (GLenum target, const GLdouble *v) {}
static void
xglNoOpMultiTexCoord4fvARB (GLenum target, const GLfloat *v) {}
static void
xglNoOpMultiTexCoord4ivARB (GLenum target, const GLint *v) {}
static void
xglNoOpMultiTexCoord4svARB (GLenum target, const GLshort *v) {}

/* GL_ARB_multisample */
static void
xglNoOpSampleCoverageARB (GLclampf value, GLboolean invert) {}

/* GL_EXT_texture_object */
static GLboolean
xglNoOpAreTexturesResidentEXT (GLsizei n,
			       const GLuint *textures,
			       GLboolean *residences)
{
    return GL_FALSE;
}
static void
xglNoOpGenTexturesEXT (GLsizei n, GLuint *textures) {}
static GLboolean
xglNoOpIsTextureEXT (GLuint texture)
{
    return GL_FALSE;
}

/* GL_SGIS_multisample */
static void
xglNoOpSampleMaskSGIS (GLclampf value, GLboolean invert) {}
static void
xglNoOpSamplePatternSGIS (GLenum pattern) {}

/* GL_EXT_point_parameters */
static void
xglNoOpPointParameterfEXT (GLenum pname, GLfloat param) {}
static void
xglNoOpPointParameterfvEXT (GLenum pname, const GLfloat *params) {}

/* GL_MESA_window_pos */
static void
xglNoOpWindowPos3fMESA (GLfloat x, GLfloat y, GLfloat z) {}
static void
xglWindowPos3fMESA (GLfloat x, GLfloat y, GLfloat z)
{
    (*cctx->glRenderTableEXT.WindowPos3fMESA) (cctx->tx + x, cctx->ty + y, z);
}

/* GL_EXT_blend_func_separate */
static void
xglNoOpBlendFuncSeparateEXT (GLenum sfactorRGB, GLenum dfactorRGB,
			     GLenum sfactorAlpha, GLenum dfactorAlpha) {}

/* GL_EXT_fog_coord */ 
static void
xglNoOpFogCoordfvEXT (const GLfloat *coord) {}
static void
xglNoOpFogCoorddvEXT (const GLdouble *coord) {}
static void
xglNoOpFogCoordPointerEXT (GLenum type, GLsizei stride,
			   const GLvoid *pointer) {}

/* GL_EXT_secondary_color */
static void
xglNoOpSecondaryColor3bvEXT (const GLbyte *v) {}
static void
xglNoOpSecondaryColor3dvEXT (const GLdouble *v) {}
static void
xglNoOpSecondaryColor3fvEXT (const GLfloat *v) {}
static void
xglNoOpSecondaryColor3ivEXT (const GLint *v) {}
static void
xglNoOpSecondaryColor3svEXT (const GLshort *v) {}
static void
xglNoOpSecondaryColor3ubvEXT (const GLubyte *v) {}
static void
xglNoOpSecondaryColor3uivEXT (const GLuint *v) {}
static void
xglNoOpSecondaryColor3usvEXT (const GLushort *v) {}
static void
xglNoOpSecondaryColorPointerEXT (GLint size, GLenum type, GLsizei stride,
				 const GLvoid *pointer) {}


/* GL_NV_point_sprite */
static void
xglNoOpPointParameteriNV (GLenum pname, GLint params) {}
static void
xglNoOpPointParameterivNV (GLenum pname, const GLint *params) {}

/* GL_EXT_stencil_two_side */
static void
xglNoOpActiveStencilFaceEXT (GLenum face) {}

__glProcTableEXT __glNoOpRenderTableEXT = {
    xglNoOpActiveTextureARB,
    xglNoOpClientActiveTextureARB,
    xglNoOpMultiTexCoord1dvARB,
    xglNoOpMultiTexCoord1fvARB,
    xglNoOpMultiTexCoord1ivARB,
    xglNoOpMultiTexCoord1svARB,
    xglNoOpMultiTexCoord2dvARB,
    xglNoOpMultiTexCoord2fvARB,
    xglNoOpMultiTexCoord2ivARB,
    xglNoOpMultiTexCoord2svARB,
    xglNoOpMultiTexCoord3dvARB,
    xglNoOpMultiTexCoord3fvARB,
    xglNoOpMultiTexCoord3ivARB,
    xglNoOpMultiTexCoord3svARB,
    xglNoOpMultiTexCoord4dvARB,
    xglNoOpMultiTexCoord4fvARB,
    xglNoOpMultiTexCoord4ivARB,
    xglNoOpMultiTexCoord4svARB,
    xglNoOpSampleCoverageARB,
    xglNoOpAreTexturesResidentEXT,
    xglNoOpGenTexturesEXT,
    xglNoOpIsTextureEXT,
    xglNoOpSampleMaskSGIS,
    xglNoOpSamplePatternSGIS,
    xglNoOpPointParameterfEXT,
    xglNoOpPointParameterfvEXT,
    xglNoOpWindowPos3fMESA,
    xglNoOpBlendFuncSeparateEXT,
    xglNoOpFogCoordfvEXT,
    xglNoOpFogCoorddvEXT,
    xglNoOpFogCoordPointerEXT,
    xglNoOpSecondaryColor3bvEXT,
    xglNoOpSecondaryColor3dvEXT,
    xglNoOpSecondaryColor3fvEXT,
    xglNoOpSecondaryColor3ivEXT,
    xglNoOpSecondaryColor3svEXT,
    xglNoOpSecondaryColor3ubvEXT,
    xglNoOpSecondaryColor3uivEXT,
    xglNoOpSecondaryColor3usvEXT,
    xglNoOpSecondaryColorPointerEXT,
    xglNoOpPointParameteriNV,
    xglNoOpPointParameterivNV,
    xglNoOpActiveStencilFaceEXT
};

static void
xglInitExtensions (xglGLContextPtr pContext)
{
    const char *extensions;

    pContext->glRenderTableEXT = __glNoOpRenderTableEXT;

    extensions = glGetString (GL_EXTENSIONS);

    if (strstr (extensions, "GL_ARB_multitexture"))
    {
	pContext->glRenderTableEXT.ActiveTextureARB =
	    (PFNGLACTIVETEXTUREARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glActiveTextureARB");
	pContext->glRenderTableEXT.ClientActiveTextureARB =
	    (PFNGLCLIENTACTIVETEXTUREARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glClientActiveTextureARB");
	pContext->glRenderTableEXT.MultiTexCoord1dvARB =
	    (PFNGLMULTITEXCOORD1DVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord1dvARB");
	pContext->glRenderTableEXT.MultiTexCoord1fvARB =
	    (PFNGLMULTITEXCOORD1FVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord1fvARB");
	pContext->glRenderTableEXT.MultiTexCoord1ivARB =
	    (PFNGLMULTITEXCOORD1IVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord1ivARB");
	pContext->glRenderTableEXT.MultiTexCoord1svARB =
	    (PFNGLMULTITEXCOORD1SVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord1svARB");
	pContext->glRenderTableEXT.MultiTexCoord2dvARB =
	    (PFNGLMULTITEXCOORD2DVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord2dvARB");
	pContext->glRenderTableEXT.MultiTexCoord2fvARB =
	    (PFNGLMULTITEXCOORD2FVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord2fvARB");
	pContext->glRenderTableEXT.MultiTexCoord2ivARB =
	    (PFNGLMULTITEXCOORD2IVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord2ivARB");
	pContext->glRenderTableEXT.MultiTexCoord2svARB =
	    (PFNGLMULTITEXCOORD2SVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord2svARB");
	pContext->glRenderTableEXT.MultiTexCoord3dvARB =
	    (PFNGLMULTITEXCOORD3DVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord3dvARB");
	pContext->glRenderTableEXT.MultiTexCoord3fvARB =
	    (PFNGLMULTITEXCOORD3FVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord3fvARB");
	pContext->glRenderTableEXT.MultiTexCoord3ivARB =
	    (PFNGLMULTITEXCOORD3IVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord3ivARB");
	pContext->glRenderTableEXT.MultiTexCoord3svARB =
	    (PFNGLMULTITEXCOORD3SVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord3svARB");
	pContext->glRenderTableEXT.MultiTexCoord4dvARB =
	    (PFNGLMULTITEXCOORD4DVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord4dvARB");
	pContext->glRenderTableEXT.MultiTexCoord4fvARB =
	    (PFNGLMULTITEXCOORD4FVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord4fvARB");
	pContext->glRenderTableEXT.MultiTexCoord4ivARB =
	    (PFNGLMULTITEXCOORD4IVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord4ivARB");
	pContext->glRenderTableEXT.MultiTexCoord4svARB =
	    (PFNGLMULTITEXCOORD4SVARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glMultiTexCoord4svARB");
    }

    if (strstr (extensions, "GL_ARB_multisample"))
    {
	pContext->glRenderTableEXT.SampleCoverageARB =
	    (PFNGLSAMPLECOVERAGEARBPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSampleCoverageARB");
    }

    if (strstr (extensions, "GL_EXT_texture_object"))
    {
	pContext->glRenderTableEXT.AreTexturesResidentEXT =
	    xglAreTexturesResident;
	pContext->glRenderTableEXT.GenTexturesEXT = xglGenTextures;
	pContext->glRenderTableEXT.IsTextureEXT = xglIsTexture;
    }

    if (strstr (extensions, "GL_SGIS_multisample"))
    {
	pContext->glRenderTableEXT.SampleMaskSGIS =
	    (PFNGLSAMPLEMASKSGISPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSampleMaskSGIS");
	pContext->glRenderTableEXT.SamplePatternSGIS =
	    (PFNGLSAMPLEPATTERNSGISPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSamplePatternSGIS");
    }

    if (strstr (extensions, "GL_EXT_point_parameters"))
    {
	pContext->glRenderTableEXT.PointParameterfEXT =
	    (PFNGLPOINTPARAMETERFEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glPointParameterfEXT");
	pContext->glRenderTableEXT.PointParameterfvEXT =
	    (PFNGLPOINTPARAMETERFVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glPointParameterfvEXT");
    }

    if (strstr (extensions, "GL_MESA_window_pos"))
    {
	pContext->WindowPos3fMESA =
	    (PFNGLWINDOWPOS3FMESAPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glWindowPos3fMESA");
	pContext->glRenderTableEXT.WindowPos3fMESA = xglWindowPos3fMESA;
    }

    if (strstr (extensions, "GL_EXT_blend_func_separate"))
    {
	pContext->glRenderTableEXT.BlendFuncSeparateEXT =
	    (PFNGLBLENDFUNCSEPARATEEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glBlendFuncSeparateEXT");
    }

    if (strstr (extensions, "GL_EXT_fog_coord"))
    {
	pContext->glRenderTableEXT.FogCoordfvEXT =
	    (PFNGLFOGCOORDFVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glFogCoordfvEXT");
	pContext->glRenderTableEXT.FogCoorddvEXT =
	    (PFNGLFOGCOORDDVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glFogCoorddvEXT");
	pContext->glRenderTableEXT.FogCoordPointerEXT =
	    (PFNGLFOGCOORDPOINTEREXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glFogCoordPointerEXT");
    }

    if (strstr (extensions, "GL_EXT_secondary_color"))
    {
	pContext->glRenderTableEXT.SecondaryColor3bvEXT =
	    (PFNGLSECONDARYCOLOR3BVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3bvEXT");
	pContext->glRenderTableEXT.SecondaryColor3dvEXT =
	    (PFNGLSECONDARYCOLOR3DVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3dvEXT");
	pContext->glRenderTableEXT.SecondaryColor3fvEXT =
	    (PFNGLSECONDARYCOLOR3FVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3fvEXT");
	pContext->glRenderTableEXT.SecondaryColor3ivEXT =
	    (PFNGLSECONDARYCOLOR3IVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3ivEXT");
	pContext->glRenderTableEXT.SecondaryColor3svEXT =
	    (PFNGLSECONDARYCOLOR3SVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3svEXT");
	pContext->glRenderTableEXT.SecondaryColor3ubvEXT =
	    (PFNGLSECONDARYCOLOR3UBVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3ubvEXT");
	pContext->glRenderTableEXT.SecondaryColor3uivEXT =
	    (PFNGLSECONDARYCOLOR3UIVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3uivEXT");
	pContext->glRenderTableEXT.SecondaryColor3usvEXT =
	    (PFNGLSECONDARYCOLOR3USVEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColor3usvEXT");
	pContext->glRenderTableEXT.SecondaryColorPointerEXT =
	    (PFNGLSECONDARYCOLORPOINTEREXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glSecondaryColorPointerEXT");
    }

    if (strstr (extensions, "GL_NV_point_sprite"))
    {
	pContext->glRenderTableEXT.PointParameteriNV =
	    (PFNGLPOINTPARAMETERINVPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glPointParameteriNV");
	pContext->glRenderTableEXT.PointParameterivNV =
	    (PFNGLPOINTPARAMETERIVNVPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glPointParameterivNV");
    }

    if (strstr (extensions, "GL_EXT_stencil_two_side"))
    {
	pContext->glRenderTableEXT.ActiveStencilFaceEXT =
	    (PFNGLACTIVESTENCILFACEEXTPROC)
	    glitz_context_get_proc_address (pContext->context,
					    "glActiveStencilFaceEXT");
    }
    
    pContext->needInit = FALSE;
}

static void
xglFreeContext (xglGLContextPtr pContext)
{
    pContext->refcnt--;
    if (pContext->shared == pContext)
	pContext->refcnt--;
	
    if (pContext->refcnt)
	return;

    if (pContext->shared != pContext)
	xglFreeContext (pContext->shared);

    if (pContext->texObjects)
    {
	xglTexObjPtr pTexObj;
	GLuint	     key;

	do {
	    key = xglHashFirstEntry (pContext->texObjects);
	    if (key)
	    {
		pTexObj = (xglTexObjPtr) xglHashLookup (pContext->texObjects,
							key);
		if (pTexObj)
		{
		    glDeleteTextures (1, &pTexObj->name);
		    xfree (pTexObj);
		}
		xglHashRemove (pContext->texObjects, key);
	    }
	} while (key);
	
	xglDeleteHashTable (pContext->texObjects);
    }

    if (pContext->displayLists)
    {
	xglDisplayListPtr pDisplayList;
	GLuint		  key;

	do {
	    key = xglHashFirstEntry (pContext->displayLists);
	    if (key)
	    {
		pDisplayList = (xglDisplayListPtr)
		    xglHashLookup (pContext->displayLists, key);
		if (pDisplayList)
		    xglDestroyList (pDisplayList);
		
		xglHashRemove (pContext->displayLists, key);
	    }
	} while (key);
	
	xglDeleteHashTable (pContext->displayLists);
    }

    if (pContext->groupList)
	glDeleteLists (pContext->groupList, 1);

    if (pContext->pAttribStack)
	xfree (pContext->pAttribStack);
    
    glitz_context_destroy (pContext->context);

    if (pContext->versionString)
	xfree (pContext->versionString);

    xfree (pContext);
}

static GLboolean
xglDestroyContext (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    xglFreeContext (pContext);

    return (*iface->exports.destroyContext) ((__GLcontext *) iface);
}

static GLboolean
xglLoseCurrent (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    __glXFlushContextCache ();

    return (*iface->exports.loseCurrent) ((__GLcontext *) iface);   
}

static GLboolean
xglMakeCurrent (__GLcontext *gc)
{
    xglGLContextPtr	pContext = (xglGLContextPtr) gc;
    __GLinterface	*iface = pContext->mIface;
    __GLdrawablePrivate *drawPriv = iface->imports.getDrawablePrivate (gc);
    __GLdrawablePrivate *readPriv = iface->imports.getReadablePrivate (gc);
    xglGLBufferPtr	pDrawBufferPriv = drawPriv->private;
    xglGLBufferPtr	pReadBufferPriv = readPriv->private;
    ScreenPtr		pScreen = pDrawBufferPriv->pScreen;
    DrawablePtr		pDrawable = pDrawBufferPriv->pDrawable;
    PixmapPtr		pPixmap = pDrawBufferPriv->pPixmap;
    GLboolean		status;

    pContext->target = xglPrepareTarget (pDrawable);
    if (pContext->target && pPixmap)
    {
	pContext->target = xglPrepareTarget (&pPixmap->drawable);
	if (pContext->target)
	{
	    xglPixmapPtr pFront = XGL_GET_DRAWABLE_PIXMAP_PRIV (pDrawable);
	    xglPixmapPtr pBack  = XGL_GET_PIXMAP_PRIV (pPixmap);
	    XID		 value = FALSE;
	    int		 status;
	    
	    pContext->attrib.scissor.x = pContext->attrib.scissor.y = 0;
	    pContext->attrib.scissor.width = pDrawable->width;
	    pContext->attrib.scissor.height = pDrawable->height;

	    pContext->attrib.viewport.x = pContext->attrib.viewport.y = 0;
	    pContext->attrib.viewport.width = pDrawable->width;
	    pContext->attrib.viewport.height = pDrawable->height;

	    pContext->draw = pBack->surface;

	    if (pBack->pArea)
	    {
		xglOffscreenPtr pOffscreen = (xglOffscreenPtr)
		    pBack->pArea->pRoot->closure;

		pContext->tx = pBack->pArea->x;
		pContext->ty =
		    glitz_drawable_get_height (pOffscreen->drawable) -
		    pBack->pArea->y - pDrawable->height;
	    }
	    else
		pContext->tx = pContext->ty = 0;
	    
	    pContext->pDrawBuffer = pDrawBufferPriv;
	    
	    if (!pFront->lock) pFront->lock++;
	    if (!pBack->lock) pBack->lock++;

	    glitz_context_set_surface (pContext->context, pContext->draw);

	    if (!pDrawBufferPriv->pGC)
		pDrawBufferPriv->pGC =
		    CreateGC (&pPixmap->drawable,
			      GCGraphicsExposures, &value,
			      &status);
	    
	    ValidateGC (&pPixmap->drawable, pDrawBufferPriv->pGC);
	    
	    if (!pDrawBufferPriv->swapGC)
		pDrawBufferPriv->swapGC =
		    CreateGC (pDrawable,
			      GCGraphicsExposures, &value,
			      &status);
	    
	    ValidateGC (pDrawable, pDrawBufferPriv->swapGC);

	    return GL_TRUE;
	}
    } else
	pContext->target = xglPixmapTargetNo;

    if (pPixmap)
    {
	(*pScreen->DestroyPixmap) (pPixmap);
	pDrawBufferPriv->pPixmap = NULL;
    }
    
    drawPriv->private = pDrawBufferPriv->private;
    readPriv->private = pReadBufferPriv->private;

    status = (*iface->exports.makeCurrent) ((__GLcontext *) iface);

    drawPriv->private = pDrawBufferPriv;
    readPriv->private = pReadBufferPriv;

    return status;
}

static GLboolean
xglShareContext (__GLcontext *gc,
		 __GLcontext *gcShare)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    xglGLContextPtr pContextShare = (xglGLContextPtr) gcShare;
    __GLinterface   *iface = pContext->mIface;
    __GLinterface   *ifaceShare = pContextShare->mIface;

    return (*iface->exports.shareContext) ((__GLcontext *) iface,
					   (__GLcontext *) ifaceShare);
}

static GLboolean
xglCopyContext (__GLcontext	  *dst,
		const __GLcontext *src,
		GLuint		  mask)
{
    xglGLContextPtr pDst = (xglGLContextPtr) dst;
    xglGLContextPtr pSrc = (xglGLContextPtr) src;
    __GLinterface   *dstIface = pDst->mIface;
    __GLinterface   *srcIface = pSrc->mIface;

    glitz_context_copy (pSrc->context, pDst->context, mask);
    
    return (*dstIface->exports.copyContext) ((__GLcontext *) dstIface,
					     (const __GLcontext *) srcIface,
					     mask);
}

static GLboolean
xglForceCurrent (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    if (pContext->target)
    {
	cctx = pContext;
	
	glitz_context_make_current (cctx->context);
	
	if (cctx->needInit)
	    xglInitExtensions (cctx);
	
	__glRenderTable = &__glNativeRenderTable;
	__glRenderTableEXT = &cctx->glRenderTableEXT;
	
	return GL_TRUE;
    }

    cctx = NULL;
    __glRenderTable = &__glMesaRenderTable;
    __glRenderTableEXT = &__glMesaRenderTableEXT;
	
    return (*iface->exports.forceCurrent) ((__GLcontext *) iface);
}

static GLboolean
xglNotifyResize (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    return (*iface->exports.notifyResize) ((__GLcontext *) iface);
}

static void
xglNotifyDestroy (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    return (*iface->exports.notifyDestroy) ((__GLcontext *) iface);
}

static void
xglNotifySwapBuffers (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    return (*iface->exports.notifySwapBuffers) ((__GLcontext *) iface);
}

static struct __GLdispatchStateRec *
xglDispatchExec (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    return (*iface->exports.dispatchExec) ((__GLcontext *) iface);
}

static void
xglBeginDispatchOverride (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    return (*iface->exports.beginDispatchOverride) ((__GLcontext *) iface);
}

static void
xglEndDispatchOverride (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    return (*iface->exports.endDispatchOverride) ((__GLcontext *) iface);
}

static void
xglLoseCurrentContext (void *closure)
{
    cctx = NULL;
    __glRenderTable = &__glMesaRenderTable;
    __glRenderTableEXT = &__glMesaRenderTableEXT;

    __glXFlushContextCache ();
}

static __GLinterface *
xglCreateContext (__GLimports      *imports,
		  __GLcontextModes *modes,
		  __GLinterface    *shareGC)
{
    glitz_drawable_format_t *format;
    xglGLContextPtr	    pShareContext = (xglGLContextPtr) shareGC;
    xglGLContextPtr	    pContext;
    __GLinterface	    *shareIface = NULL;
    __GLinterface	    *iface;
    __GLXcontext	    *glxCtx = (__GLXcontext *) imports->other;
    
    XGL_SCREEN_PRIV (glxCtx->pScreen);

    format = glitz_drawable_get_format (pScreenPriv->drawable);

    pContext = xalloc (sizeof (xglGLContextRec));
    if (!pContext)
	return NULL;

    pContext->context = glitz_context_create (pScreenPriv->drawable, format);
    glitz_context_set_user_data (pContext->context, NULL, 
				 xglLoseCurrentContext);
	
    pContext->needInit	    = TRUE;
    pContext->draw	    = NULL;
    pContext->target	    = FALSE;
    pContext->versionString = NULL;
    pContext->error	    = GL_NO_ERROR;
    pContext->shared	    = NULL;
    pContext->list	    = 0;
    pContext->groupList	    = 0;
    pContext->pAttribStack  = NULL;
    pContext->nAttribStack  = 0;
    pContext->refcnt	    = 1;

    pContext->attrib.drawBuffer  = GL_BACK;
    pContext->attrib.readBuffer  = GL_BACK;
    pContext->attrib.scissorTest = GL_FALSE;
    
    if (shareGC)
    {
	pContext->texObjects   = NULL;
	pContext->displayLists = NULL;
	
	pContext->shared = pShareContext->shared;
	shareIface = pShareContext->mIface;
    }
    else
    {
	pContext->texObjects = xglNewHashTable ();
	if (!pContext->texObjects)
	{
	    xglFreeContext (pContext);
	    return NULL;
	}
	
	pContext->displayLists = xglNewHashTable ();
	if (!pContext->displayLists)
	{
	    xglFreeContext (pContext);
	    return NULL;
	}

	pContext->shared = pContext;
    }

    pContext->shared->refcnt++;
    
    iface = (*screenInfoPriv.createContext) (imports, modes, shareIface);
    if (!iface)
    {
	xglFreeContext (pContext);
	return NULL;
    }

    pContext->mIface = iface;
    pContext->iface.imports = *imports;
    
    pContext->iface.exports.destroyContext	  = xglDestroyContext;
    pContext->iface.exports.loseCurrent		  = xglLoseCurrent;
    pContext->iface.exports.makeCurrent		  = xglMakeCurrent;
    pContext->iface.exports.shareContext	  = xglShareContext;
    pContext->iface.exports.copyContext		  = xglCopyContext;
    pContext->iface.exports.forceCurrent	  = xglForceCurrent;
    pContext->iface.exports.notifyResize	  = xglNotifyResize;
    pContext->iface.exports.notifyDestroy	  = xglNotifyDestroy;
    pContext->iface.exports.notifySwapBuffers     = xglNotifySwapBuffers;
    pContext->iface.exports.dispatchExec	  = xglDispatchExec;
    pContext->iface.exports.beginDispatchOverride = xglBeginDispatchOverride;
    pContext->iface.exports.endDispatchOverride   = xglEndDispatchOverride;
    
    return (__GLinterface *) pContext;
}

static GLboolean
xglSwapBuffers (__GLXdrawablePrivate *glxPriv)
{
    __GLdrawablePrivate	*glPriv = &glxPriv->glPriv;
    xglGLBufferPtr	pBufferPriv = glPriv->private;
    DrawablePtr		pDrawable = pBufferPriv->pDrawable;
    PixmapPtr		pPixmap = pBufferPriv->pPixmap;
    GCPtr		pGC = pBufferPriv->swapGC;
    GLboolean		ret;
    
    if (pPixmap)
    {
	/* Discard front buffer damage */
	REGION_EMPTY (pGC->pScreen, &pBufferPriv->damage);
	
	if (pGC)
	{
	    (*pGC->ops->CopyArea) ((DrawablePtr) pPixmap,
				   pDrawable, pGC,
				   0, 0,
				   pPixmap->drawable.width,
				   pPixmap->drawable.height,
				   0, 0);
	    
	    return GL_TRUE;
	}

	return GL_FALSE;
    }
    
    glPriv->private = pBufferPriv->private;
    ret = (*pBufferPriv->swapBuffers) (glxPriv);
    glPriv->private = pBufferPriv;
    
    return ret;
}

static GLboolean
xglResizeBuffers (__GLdrawableBuffer  *buffer,
		  GLint		      x,
		  GLint		      y,
		  GLuint	      width,
		  GLuint	      height, 
		  __GLdrawablePrivate *glPriv,
		  GLuint	      bufferMask)
{
    xglGLBufferPtr pBufferPriv = glPriv->private;
    DrawablePtr    pDrawable = pBufferPriv->pDrawable;
    PixmapPtr	   pPixmap = pBufferPriv->pPixmap;
    ScreenPtr      pScreen = pBufferPriv->pScreen;
    Bool	   status;
    
    if (pPixmap)
    {
	if (pPixmap->drawable.width  != width ||
	    pPixmap->drawable.height != height)
	{   
	    (*pScreen->DestroyPixmap) (pPixmap);
	    pPixmap = (*pScreen->CreatePixmap) (pScreen, width, height,
						pDrawable->depth);

	    /* give good initial score */
	    XGL_GET_PIXMAP_PRIV (pPixmap)->score = 4000;
	    pBufferPriv->pPixmap = pPixmap;
	}
	ValidateGC (pDrawable, pBufferPriv->swapGC);
    }
    
    glPriv->private = pBufferPriv->private;
    status = (*pBufferPriv->resizeBuffers) (buffer,
					    x, y, width, height,
					    glPriv,
					    bufferMask);
    glPriv->private = pBufferPriv;
    
    return status;
}

static void
xglFreeBuffers (__GLdrawablePrivate *glPriv)
{
    xglGLBufferPtr pBufferPriv = glPriv->private;
    ScreenPtr      pScreen = pBufferPriv->pScreen;
    
    glPriv->private = pBufferPriv->private;

    (*pBufferPriv->freeBuffers) (glPriv);
    
    if (pBufferPriv->pPixmap)
	(*pScreen->DestroyPixmap) (pBufferPriv->pPixmap);

    if (pBufferPriv->pGC)
	FreeGC (pBufferPriv->pGC, (GContext) 0);

    if (pBufferPriv->swapGC)
	FreeGC (pBufferPriv->swapGC, (GContext) 0);
    
    xfree (pBufferPriv);
}

static void
xglCreateBuffer (__GLXdrawablePrivate *glxPriv)
{
    __GLdrawablePrivate	*glPriv = &glxPriv->glPriv;
    DrawablePtr		pDrawable = glxPriv->pDraw;
    ScreenPtr		pScreen = pDrawable->pScreen;
    xglGLBufferPtr	pBufferPriv;

    XGL_DRAWABLE_PIXMAP_PRIV (pDrawable);
    
    pBufferPriv = xalloc (sizeof (xglGLBufferRec));
    if (!pBufferPriv)
	FatalError ("No memory");

    pBufferPriv->pScreen = pScreen;
    pBufferPriv->pDrawable = pDrawable;
    pBufferPriv->pPixmap = NULL;
    pBufferPriv->pGC = NULL;
    pBufferPriv->swapGC = NULL;

    REGION_INIT (pScreen, &pBufferPriv->damage, NullBox, 0);
    
    if (glxPriv->pGlxVisual->doubleBuffer)
	pBufferPriv->pPixmap =
	    (*pScreen->CreatePixmap) (pScreen,
				      pDrawable->width,
				      pDrawable->height,
				      pDrawable->depth);

    /* give good initial score */
    pPixmapPriv->score = 4000;

    (*screenInfoPriv.createBuffer) (glxPriv);

    /* Wrap the front buffer's resize routine */
    pBufferPriv->resizeBuffers = glPriv->frontBuffer.resize;
    glPriv->frontBuffer.resize = xglResizeBuffers;

    /* Wrap the swap buffers routine */
    pBufferPriv->swapBuffers = glxPriv->swapBuffers;
    glxPriv->swapBuffers = xglSwapBuffers;

    /* Wrap private and freePrivate */
    pBufferPriv->private = glPriv->private;
    pBufferPriv->freeBuffers = glPriv->freePrivate;
    glPriv->private = (void *) pBufferPriv;
    glPriv->freePrivate = xglFreeBuffers;
}

static Bool
xglScreenProbe (int screen)
{
    Bool status;

    status = (*screenInfoPriv.screenProbe) (screen);

    /* Wrap createBuffer */
    if (__glDDXScreenInfo.createBuffer != xglCreateBuffer)
    {
	screenInfoPriv.createBuffer    = __glDDXScreenInfo.createBuffer;
	__glDDXScreenInfo.createBuffer = xglCreateBuffer;
    }
    
    /* Wrap createContext */
    if (__glDDXScreenInfo.createContext != xglCreateContext)
    {
	screenInfoPriv.createContext    = __glDDXScreenInfo.createContext;
	__glDDXScreenInfo.createContext = xglCreateContext;
    }
    
    return status;
}

Bool
xglInitVisualConfigs (ScreenPtr pScreen)
{
    miInitVisualsProcPtr    initVisualsProc = NULL;
    VisualPtr		    visuals;
    int			    nvisuals;
    DepthPtr		    depths;
    int			    ndepths;
    int			    rootDepth;
    VisualID		    defaultVis;
    glitz_drawable_format_t *format;
    xglPixelFormatPtr	    pPixel;
    __GLXvisualConfig	    *pConfig;
    xglGLXVisualConfigPtr   pConfigPriv, *ppConfigPriv;
    XID			    *installedCmaps;
    ColormapPtr		    installedCmap;
    int			    numInstalledCmaps;
    int			    numConfig = 1;
    int			    depth, bpp, i;

    XGL_SCREEN_PRIV (pScreen);
    
    depth  = pScreenPriv->pVisual->pPixel->depth;
    bpp    = pScreenPriv->pVisual->pPixel->masks.bpp;
    format = glitz_drawable_get_format (pScreenPriv->drawable);
    pPixel = pScreenPriv->pixmapFormats[depth].pPixel;

    pConfig = xcalloc (sizeof (__GLXvisualConfig), numConfig);
    if (!pConfig)
	return FALSE;

    pConfigPriv = xcalloc (sizeof (xglGLXVisualConfigRec), numConfig);
    if (!pConfigPriv)
    {
	xfree (pConfig);
	return FALSE;
    }

    ppConfigPriv = xcalloc (sizeof (xglGLXVisualConfigPtr), numConfig);
    if (!ppConfigPriv)
    {
	xfree (pConfigPriv);
	xfree (pConfig);
	return FALSE;
    }

    installedCmaps = xalloc (pScreen->maxInstalledCmaps * sizeof (XID));
    if (!installedCmaps)
    {
	xfree (ppConfigPriv);
	xfree (pConfigPriv);
	xfree (pConfig);
	return FALSE;
    }
    
    for (i = 0; i < numConfig; i++)
    {
	ppConfigPriv[i] = &pConfigPriv[i];
	
	pConfig[i].vid   = (VisualID) (-1);
	pConfig[i].class = -1;
	pConfig[i].rgba  = TRUE;
	
	pConfig[i].redSize   = format->color.red_size;
	pConfig[i].greenSize = format->color.green_size;
	pConfig[i].blueSize  = format->color.blue_size;

	pConfig[i].redMask   = pPixel->masks.red_mask;
	pConfig[i].greenMask = pPixel->masks.green_mask;
	pConfig[i].blueMask  = pPixel->masks.blue_mask;

	if (format->color.alpha_size)
	{
	    pConfig[i].alphaSize = format->color.alpha_size;
	    pConfig[i].alphaMask = pPixel->masks.alpha_mask;
	}
	else
	{
	    pConfig[i].alphaSize = 0;
	    pConfig[i].alphaMask = 0;
	}

	pConfig[i].doubleBuffer = TRUE;
	pConfig[i].depthSize = format->depth_size;
	pConfig[i].stencilSize = format->stencil_size;
	pConfig[i].stereo = FALSE;
	
	if (depth == 16)
	    pConfig[i].bufferSize = 16;
	else
	    pConfig[i].bufferSize = 32;

	pConfig[i].auxBuffers = 0;
	pConfig[i].level      = 0;
	
	pConfig[i].visualRating = GLX_NONE;
	
	pConfig[i].transparentPixel = GLX_NONE;
	pConfig[i].transparentRed   = 0;
	pConfig[i].transparentGreen = 0;
	pConfig[i].transparentBlue  = 0;
	pConfig[i].transparentAlpha = 0;
	pConfig[i].transparentIndex = 0;
    }

    GlxSetVisualConfigs (numConfig, pConfig, (void **) ppConfigPriv);

    /* Wrap screenProbe */
    if (__glDDXScreenInfo.screenProbe != xglScreenProbe)
    {
	screenInfoPriv.screenProbe    = __glDDXScreenInfo.screenProbe;
	__glDDXScreenInfo.screenProbe = xglScreenProbe;
    }

    visuals    = pScreen->visuals;
    nvisuals   = pScreen->numVisuals;
    depths     = pScreen->allowedDepths;
    ndepths    = pScreen->numDepths;
    rootDepth  = pScreen->rootDepth;
    defaultVis = pScreen->rootVisual;
    
    /* Find installed colormaps */
    numInstalledCmaps = (*pScreen->ListInstalledColormaps) (pScreen, 
							    installedCmaps);
	
    GlxWrapInitVisuals (&initVisualsProc);
    GlxInitVisuals (&visuals, &depths, &nvisuals, &ndepths, &rootDepth,
		    &defaultVis, ((unsigned long) 1 << (bpp - 1)), 8, -1);

    /* Fix up any existing installed colormaps. */
    for (i = 0; i < numInstalledCmaps; i++)
    {
	int j;
	
	installedCmap = LookupIDByType (installedCmaps[i], RT_COLORMAP);
	if (!installedCmap)
	    continue;
	
	j = installedCmap->pVisual - pScreen->visuals;
	installedCmap->pVisual = &visuals[j];
    }
    
    pScreen->visuals       = visuals;
    pScreen->numVisuals    = nvisuals;
    pScreen->allowedDepths = depths;
    pScreen->numDepths     = ndepths;
    pScreen->rootDepth     = rootDepth;
    pScreen->rootVisual    = defaultVis;

    xfree (installedCmaps);
    xfree (pConfigPriv);
    xfree (pConfig);

    ErrorF ("[glx] initialized\n");
    
    return TRUE;
}

#endif /* GLXEXT */
