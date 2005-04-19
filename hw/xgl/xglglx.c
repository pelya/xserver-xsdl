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
    GCPtr		pGC;
    void	        *private;
} xglGLBufferRec, *xglGLBufferPtr;

typedef int xglGLXVisualConfigRec, *xglGLXVisualConfigPtr;

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
    xRectangle		     viewport;
    xRectangle		     scissor;
    GLboolean		     scissorTest;
    char		     *versionString;
    GLenum		     error;
    xglHashTablePtr	     texObjects;
    xglHashTablePtr	     displayLists;
} xglGLContextRec, *xglGLContextPtr;

typedef struct _xglTexObj {
    GLuint name;
} xglTexObjRec, *xglTexObjPtr;

typedef struct _xglDisplayList {
    GLuint name;
} xglDisplayListRec, *xglDisplayListPtr;

xglGLContextPtr cctx = NULL;

static void
xglViewport (GLint   x,
	     GLint   y,
	     GLsizei width,
	     GLsizei height)
{
    cctx->viewport.x	  = x;
    cctx->viewport.y	  = y;
    cctx->viewport.width  = width;
    cctx->viewport.height = height;

    glitz_context_set_viewport (cctx->context,
				cctx->tx + x, cctx->ty + y, width, height);
}

static void
xglScissor (GLint   x,
	    GLint   y,
	    GLsizei width,
	    GLsizei height)
{
    cctx->scissor.x	 = x;
    cctx->scissor.y	 = y;
    cctx->scissor.width  = width;
    cctx->scissor.height = height;

    if (cctx->scissorTest)
	glitz_context_set_scissor (cctx->context,
				   cctx->tx + x, cctx->ty + y, width, height);
}

static void
xglDrawBuffer (GLenum mode)
{
    ErrorF ("NYI xglDrawBuffer: 0x%x\n", mode);
}

static void
xglDisable (GLenum cap)
{
    switch (cap) {
    case GL_SCISSOR_TEST:
	if (cctx->scissorTest)
	{
	    __GLcontext		*gc = (__GLcontext *) cctx;
	    __GLinterface	*i = cctx->mIface;
	    __GLdrawablePrivate *drawPriv = i->imports.getDrawablePrivate (gc);
	    xglGLBufferPtr	pDrawBufferPriv = drawPriv->private;
	    DrawablePtr		pDrawable = pDrawBufferPriv->pDrawable;
    
	    cctx->scissorTest = GL_FALSE;
	    glitz_context_set_scissor (cctx->context,
				       cctx->tx, cctx->ty,
				       pDrawable->width, pDrawable->height);
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
	cctx->scissorTest = GL_TRUE;
	glitz_context_set_scissor (cctx->context,
				   cctx->tx + cctx->scissor.x,
				   cctx->ty + cctx->scissor.y,
				   cctx->scissor.width,
				   cctx->scissor.height);
	break;
    default:
	glEnable (cap);
    }
}

static void
xglReadBuffer (GLenum mode)
{
    ErrorF ("NYI xglReadBuffer: 0x%x\n", mode);
}

static void
xglCopyPixels (GLint   x,
	       GLint   y,
	       GLsizei width,
	       GLsizei height,
	       GLenum  type)
{
    ErrorF ("NYI glCopyPixels: %d %d %d %d 0x%x\n",
	    x, y, width, height, type);   
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
	params[0] = ENUM_TO_BOOLEAN (GL_BACK);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_BOOLEAN (GL_BACK);
	break;
    case GL_SCISSOR_BOX:
	params[0] = INT_TO_BOOLEAN (cctx->scissor.x);
	params[1] = INT_TO_BOOLEAN (cctx->scissor.y);
	params[2] = INT_TO_BOOLEAN (cctx->scissor.width);
	params[3] = INT_TO_BOOLEAN (cctx->scissor.height); 
	break;
    case GL_SCISSOR_TEST:
	params[0] = cctx->scissorTest;
	break;
    case GL_VIEWPORT:
	params[0] = INT_TO_BOOLEAN (cctx->viewport.x);
	params[1] = INT_TO_BOOLEAN (cctx->viewport.y);
	params[2] = INT_TO_BOOLEAN (cctx->viewport.width);
	params[3] = INT_TO_BOOLEAN (cctx->viewport.height);
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
	params[0] = ENUM_TO_DOUBLE (GL_BACK);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_DOUBLE (GL_BACK);
	break;
    case GL_SCISSOR_BOX:
	params[0] = cctx->scissor.x;
	params[1] = cctx->scissor.y;
	params[2] = cctx->scissor.width;
	params[3] = cctx->scissor.height; 
	break;
    case GL_SCISSOR_TEST:
	params[0] = BOOLEAN_TO_DOUBLE (cctx->scissorTest);
	break;
    case GL_VIEWPORT:
	params[0] = cctx->viewport.x;
	params[1] = cctx->viewport.y;
	params[2] = cctx->viewport.width;
	params[3] = cctx->viewport.height;
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
	params[0] = ENUM_TO_FLOAT (GL_BACK);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_FLOAT (GL_BACK);
	break;
    case GL_SCISSOR_BOX:
	params[0] = cctx->scissor.x;
	params[1] = cctx->scissor.y;
	params[2] = cctx->scissor.width;
	params[3] = cctx->scissor.height; 
	break;
    case GL_SCISSOR_TEST:
	params[0] = BOOLEAN_TO_FLOAT (cctx->scissorTest);
	break;
    case GL_VIEWPORT:
	params[0] = cctx->viewport.x;
	params[1] = cctx->viewport.y;
	params[2] = cctx->viewport.width;
	params[3] = cctx->viewport.height;
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
	params[0] = ENUM_TO_INT (GL_BACK);
	break;
    case GL_READ_BUFFER:
	params[0] = ENUM_TO_INT (GL_BACK);
	break;
    case GL_SCISSOR_BOX:
	params[0] = cctx->scissor.x;
	params[1] = cctx->scissor.y;
	params[2] = cctx->scissor.width;
	params[3] = cctx->scissor.height; 
	break;
    case GL_SCISSOR_TEST:
	params[0] = BOOLEAN_TO_INT (cctx->scissorTest);
	break;
    case GL_VIEWPORT:
	params[0] = cctx->viewport.x;
	params[1] = cctx->viewport.y;
	params[2] = cctx->viewport.width;
	params[3] = cctx->viewport.height;
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
	return cctx->scissorTest;
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

static GLuint
xglGenLists (GLsizei range)
{
    xglDisplayListPtr pDisplayList;
    GLuint	      first, name, lists;
    
    lists = glGenLists (range);
    if (!lists)
	return 0;
    
    first = xglHashFindFreeKeyBlock (cctx->shared->displayLists, range);

    name = first;
    for (name = first; range--; name++)
    {
	pDisplayList = xalloc (sizeof (xglDisplayListRec));
	if (pDisplayList)
	{
	    pDisplayList->name = lists;
	    xglHashInsert (cctx->shared->displayLists, name, pDisplayList);
	}
	else
	{
	    glDeleteLists (lists, 1);
	    cctx->error = GL_OUT_OF_MEMORY;
	}
	lists++;
    }

    return first;
}

static void
xglNewList (GLuint list,
	    GLenum mode)
{
    xglDisplayListPtr pDisplayList;
    
    if (!list)
    {
	cctx->error = GL_INVALID_VALUE;
	return;
    }
       
    pDisplayList = (xglDisplayListPtr)
	xglHashLookup (cctx->shared->displayLists, list);
    if (!pDisplayList)
    {
	pDisplayList = xalloc (sizeof (xglDisplayListRec));
	if (!pDisplayList)
	{
	    cctx->error = GL_OUT_OF_MEMORY;
	    return;
	}
	    
	pDisplayList->name = glGenLists (1);
	xglHashInsert (cctx->shared->displayLists, list, pDisplayList);
    }
    
    glNewList (pDisplayList->name, mode);
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
	glCallList (pDisplayList->name);
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
	    glCallList (pDisplayList->name);
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
	    glDeleteLists (pDisplayList->name, 1);
	    xglHashRemove (cctx->shared->displayLists, list + i);
	    xfree (pDisplayList);
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
	return glIsList (pDisplayList->name);

    return GL_FALSE;
}

__glProcTable __glNativeRenderTable = {
    xglNewList, /* glNewList */
    glEndList,
    xglCallList, /* glCallList */
    xglCallLists, /* glCallLists */
    xglDeleteLists, /* glDeleteLists */
    xglGenLists, /* glGenLists */
    glListBase,
    glBegin,
    glBitmap,
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
    glEnd,
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
    glRectdv,
    glRectfv,
    glRectiv,
    glRectsv,
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
    glClear,
    glClearAccum,
    glClearIndex,
    glClearColor,
    glClearStencil,
    glClearDepth,
    glStencilMask,
    glColorMask,
    glDepthMask,
    glIndexMask,
    glAccum,
    xglDisable, /* glDisable */
    xglEnable, /* glEnable */
    glFinish,
    glFlush,
    glPopAttrib,
    glPushAttrib,
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
    glReadPixels,
    glDrawPixels,
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
    glDrawArrays,
    glDrawElements,
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
    glCopyTexImage1D,
    glCopyTexImage2D,
    glCopyTexSubImage1D,
    glCopyTexSubImage2D,
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
    glCopyColorTable,
    glGetColorTable,
    glGetColorTableParameterfv,
    glGetColorTableParameteriv,
    glColorSubTable,
    glCopyColorSubTable,
    glConvolutionFilter1D,
    glConvolutionFilter2D,
    glConvolutionParameterf,
    glConvolutionParameterfv,
    glConvolutionParameteri,
    glConvolutionParameteriv,
    glCopyConvolutionFilter1D,
    glCopyConvolutionFilter2D,
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
    glCopyTexSubImage3D
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
		{
		    glDeleteLists (pDisplayList->name, 1);
		    xfree (pDisplayList);
		}
		xglHashRemove (pContext->displayLists, key);
	    }
	} while (key);
	
	xglDeleteHashTable (pContext->displayLists);
    }
    
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
	    
	    pContext->scissor.x = pContext->scissor.y = 0;
	    pContext->scissor.width = pDrawable->width;
	    pContext->scissor.height = pDrawable->height;

	    pContext->viewport.x = pContext->viewport.y = 0;
	    pContext->viewport.width = pDrawable->width;
	    pContext->viewport.height = pDrawable->height;

	    pContext->draw = pBack->surface;
	    pContext->tx = pContext->ty = 0;
	    
	    if (!pFront->lock) pFront->lock++;
	    if (!pBack->lock) pBack->lock++;

	    glitz_context_set_surface (pContext->context, pContext->draw);

	    return GL_TRUE;
	}
    } else
	pContext->target = xglPixmapTargetNo;

    ErrorF ("[glx] software: %d %d\n", pDrawable->x, pDrawable->y);
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
    pContext->scissorTest   = GL_FALSE;
    pContext->error	    = GL_NO_ERROR;
    
    pContext->refcnt = 1;
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
    GCPtr		pGC = pBufferPriv->pGC;
    GLboolean		ret;

    if (pPixmap)
    {
	if (!pGC)
	{
	    int status;
	    XID value;

	    value = FALSE;
	    pGC = CreateGC (pDrawable, GCGraphicsExposures, &value, &status);
	    ValidateGC (pDrawable, pGC);
	    pBufferPriv->pGC = pGC;
	}
	
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
	    ErrorF ("[glx] resizeBuffer: %d %d %dx%d\n", x, y, width, height);

	    (*pScreen->DestroyPixmap) (pPixmap);
	    pPixmap = (*pScreen->CreatePixmap) (pScreen, width, height,
						pDrawable->depth);

	    /* give good initial score */
	    XGL_GET_PIXMAP_PRIV (pPixmap)->score = 4000;
	    pBufferPriv->pPixmap = pPixmap;
	}
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

    ErrorF ("[glx] createBuffer: %dx%d\n",
	    pDrawable->width, pDrawable->height);
    
    pBufferPriv->pScreen = pScreen;
    pBufferPriv->pDrawable = pDrawable;
    pBufferPriv->pPixmap = NULL;
    pBufferPriv->pGC = NULL;
    
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
