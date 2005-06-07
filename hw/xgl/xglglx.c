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
#include "glxutil.h"
#include "unpack.h"
#include "g_disptab.h"
#include "micmap.h"

#define XGL_MAX_TEXTURE_UNITS 8

#define XGL_TEXTURE_1D_BIT	  (1 << 0)
#define XGL_TEXTURE_2D_BIT	  (1 << 1)
#define XGL_TEXTURE_3D_BIT	  (1 << 2)
#define XGL_TEXTURE_RECTANGLE_BIT (1 << 3)
#define XGL_TEXTURE_CUBE_MAP_BIT  (1 << 4)
	
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

extern __glProcTable     __glMesaRenderTable;
extern __glProcTableEXT  __glMesaRenderTableEXT;

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
    glitz_surface_t	*backSurface;
    PixmapPtr		pPixmap;
    GCPtr		pGC;
    RegionRec		damage;
    void	        *private;
    int			xOff, yOff;
    int			yFlip;
} xglGLBufferRec, *xglGLBufferPtr;

typedef int xglGLXVisualConfigRec, *xglGLXVisualConfigPtr;
typedef struct _xglDisplayList *xglDisplayListPtr;

#define XGL_LIST_OP_CALLS 0
#define XGL_LIST_OP_DRAW  1
#define XGL_LIST_OP_GL    2
#define XGL_LIST_OP_LIST  3

typedef struct _xglGLOp {
    void (*glProc) (struct _xglGLOp *pOp);
    union {
	GLenum     enumeration;
	GLbitfield bitfield;
	GLsizei    size;
	struct {
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height;
	} rect;
	struct {
	    GLenum target;
	    GLuint texture;
	} bind_texture;
	struct {
	    GLenum target;
	    GLenum pname;
	    GLint  params[4];
	} tex_parameter_iv;
	struct {
	    GLenum  target;
	    GLenum  pname;
	    GLfloat params[4];
	} tex_parameter_fv;
	struct {
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height;
	    GLenum  type; 
	} copy_pixels;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLint   border; 
	} copy_tex_image_1d;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height;
	    GLint   border; 
	} copy_tex_image_2d;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLint   xoffset;
	    GLint   x;
	    GLint   y;
	    GLsizei width; 
	} copy_tex_sub_image_1d;
	struct {
	    GLenum  target;
	    GLint   level;
	    GLint   xoffset;
	    GLint   yoffset;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height; 
	} copy_tex_sub_image_2d;
	struct {
	    GLenum  target;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	} copy_color_table;
	struct {
	    GLenum  target;
	    GLsizei start;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	} copy_color_sub_table;
	struct {
	    GLenum  target;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	} copy_convolution_filter_1d;
	struct {
	    GLenum  target;
	    GLenum  internalformat;
	    GLint   x;
	    GLint   y;
	    GLsizei width;
	    GLsizei height;
	} copy_convolution_filter_2d;
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
	} copy_tex_sub_image_3d;
	struct {
	    GLfloat x;
	    GLfloat y;
	    GLfloat z;
	} window_pos_3f;
    } u;
} xglGLOpRec, *xglGLOpPtr;

typedef struct _xglListOp {
    int type;
    union {
	GLuint	   list;
	xglGLOpPtr gl;
    } u;
} xglListOpRec, *xglListOpPtr;

typedef struct _xglDisplayList {
    xglListOpPtr pOp;
    int		 nOp;
    int		 size;
} xglDisplayListRec;

typedef struct _xglTexObj {
    GLuint    key;
    GLuint    name;
    PixmapPtr pPixmap;
    int	      refcnt;
} xglTexObjRec, *xglTexObjPtr;

typedef struct _xglTexUnit {
    GLbitfield   enabled;
    xglTexObjPtr p1D;
    xglTexObjPtr p2D;
    xglTexObjPtr p3D;
    xglTexObjPtr pRect;
    xglTexObjPtr pCubeMap;
} xglTexUnitRec, *xglTexUnitPtr;

typedef struct _xglGLAttributes {
    GLbitfield	  mask;
    GLenum	  drawBuffer;
    GLenum	  readBuffer;
    xRectangle	  viewport;
    xRectangle	  scissor;
    GLboolean	  scissorTest;
    xglTexUnitRec texUnits[XGL_MAX_TEXTURE_UNITS];
} xglGLAttributesRec, *xglGLAttributesPtr;

typedef struct _xglGLContext {
    __GLinterface	      iface;
    __GLinterface	      *mIface;
    int			      refcnt;
    struct _xglGLContext      *shared;
    glitz_context_t	      *context;
    __glProcTableEXT	      glRenderTableEXT;
    PFNGLACTIVETEXTUREARBPROC ActiveTextureARB;
    PFNGLWINDOWPOS3FMESAPROC  WindowPos3fMESA;
    Bool		      needInit;
    xglGLBufferPtr	      pDrawBuffer;
    xglGLBufferPtr	      pReadBuffer;
    int			      drawXoff, drawYoff;
    char		      *versionString;
    GLenum		      errorValue;
    GLboolean		      doubleBuffer;
    GLint		      depthBits;
    GLint		      stencilBits;
    xglHashTablePtr	      texObjects;
    xglHashTablePtr	      displayLists;
    GLuint		      list;
    GLenum		      listMode;
    xglDisplayListPtr	      pList;
    GLuint		      groupList;
    xglGLAttributesRec	      attrib;
    xglGLAttributesPtr	      pAttribStack;
    int			      nAttribStack;
    int			      activeTexUnit;
    GLint		      maxTexUnits;
    GLint		      maxListNesting;
} xglGLContextRec, *xglGLContextPtr;

static xglGLContextPtr cctx = NULL;

static void
xglSetCurrentContext (xglGLContextPtr pContext);

#define XGL_GLX_INTERSECT_BOX(pBox1, pBox2) \
    {					    \
	if ((pBox1)->x1 < (pBox2)->x1)      \
	    (pBox1)->x1 = (pBox2)->x1;      \
	if ((pBox1)->y1 < (pBox2)->y1)      \
	    (pBox1)->y1 = (pBox2)->y1;      \
	if ((pBox1)->x2 > (pBox2)->x2)      \
	    (pBox1)->x2 = (pBox2)->x2;      \
	if ((pBox1)->y2 > (pBox2)->y2)      \
	    (pBox1)->y2 = (pBox2)->y2;      \
    }

#define XGL_GLX_SET_SCISSOR_BOX(pBox)		      \
    glScissor ((pBox)->x1,			      \
	       cctx->pDrawBuffer->yFlip - (pBox)->y2, \
	       (pBox)->x2 - (pBox)->x1,		      \
	       (pBox)->y2 - (pBox)->y1)

#define XGL_GLX_DRAW_PROLOGUE(pBox, nBox, pScissorBox)			  \
    (pBox) = REGION_RECTS (cctx->pDrawBuffer->pGC->pCompositeClip);	  \
    (nBox) = REGION_NUM_RECTS (cctx->pDrawBuffer->pGC->pCompositeClip);   \
    (pScissorBox)->x1 = cctx->attrib.scissor.x + cctx->pDrawBuffer->xOff; \
    (pScissorBox)->x2 = (pScissorBox)->x1 + cctx->attrib.scissor.width;	  \
    (pScissorBox)->y2 = cctx->attrib.scissor.y + cctx->pDrawBuffer->yOff; \
    (pScissorBox)->y2 = cctx->pDrawBuffer->yFlip - (pScissorBox)->y2;     \
    (pScissorBox)->y1 = (pScissorBox)->y2 - cctx->attrib.scissor.height;  \
    xglSetupTextures ()

#define XGL_GLX_DRAW_DAMAGE(pBox, pRegion)				 \
    if (cctx->attrib.drawBuffer != GL_BACK &&				 \
	cctx->pDrawBuffer->pDrawable &&					 \
	cctx->pDrawBuffer->pDrawable->type != DRAWABLE_PIXMAP)		 \
    {									 \
	REGION_INIT (cctx->pDrawBuffer->pGC->pScreen, pRegion, pBox, 1); \
	REGION_UNION (cctx->pDrawBuffer->pGC->pScreen,			 \
		      &cctx->pDrawBuffer->damage,			 \
		      &cctx->pDrawBuffer->damage,			 \
		      pRegion);						 \
	REGION_UNINIT (cctx->pDrawBuffer->pGC->pScreen, pRegion);	 \
    }

static void
xglRecordError (GLenum error)
{
    if (cctx->errorValue == GL_NO_ERROR)
	cctx->errorValue = error;
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

    return pDisplayList;
}

static void
xglDestroyList (xglDisplayListPtr pDisplayList)
{
    xglListOpPtr pOp = pDisplayList->pOp;
    int		 nOp = pDisplayList->nOp;

    while (nOp--)
    {
	switch (pOp->type) {
	case XGL_LIST_OP_CALLS:
	case XGL_LIST_OP_DRAW:
	    glDeleteLists (pOp->u.list, 1);
	    break;    
	case XGL_LIST_OP_GL:
	    xfree (pOp->u.gl);
	    break;
	case XGL_LIST_OP_LIST:
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
	xglRecordError (GL_OUT_OF_MEMORY);
	return;
    }
    
    cctx->pList->pOp[cctx->pList->nOp].type   = type;
    cctx->pList->pOp[cctx->pList->nOp].u.list = glGenLists (1);

    glNewList (cctx->pList->pOp[cctx->pList->nOp].u.list, mode);

    cctx->pList->nOp++;
}

static void
xglGLOp (xglGLOpPtr pOp)
{
    if (cctx->list)
    {
	xglGLOpPtr pGLOp;

	pGLOp = xalloc (sizeof (xglGLOpRec));
	if (!pGLOp)
	{
	    xglRecordError (GL_OUT_OF_MEMORY);
	    return;
	}
	
	if (!xglResizeList (cctx->pList, cctx->pList->nOp + 1))
	{
	    xfree (pGLOp);
	    xglRecordError (GL_OUT_OF_MEMORY);
	    return;
	}

	glEndList ();

	*pGLOp = *pOp;
	
	cctx->pList->pOp[cctx->pList->nOp].type = XGL_LIST_OP_GL;
	cctx->pList->pOp[cctx->pList->nOp].u.gl = pGLOp;
	cctx->pList->nOp++;

	if (cctx->listMode == GL_COMPILE_AND_EXECUTE)
	    (*pOp->glProc) (pOp);

	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
    }
    else
	(*pOp->glProc) (pOp);   
}

static void
xglViewportProc (xglGLOpPtr pOp)
{
    cctx->attrib.viewport.x	 = pOp->u.rect.x;
    cctx->attrib.viewport.y	 = pOp->u.rect.y;
    cctx->attrib.viewport.width  = pOp->u.rect.width;
    cctx->attrib.viewport.height = pOp->u.rect.height;

    glViewport (pOp->u.rect.x + cctx->pDrawBuffer->xOff,
		pOp->u.rect.y + cctx->pDrawBuffer->yOff,
		pOp->u.rect.width,
		pOp->u.rect.height);
}

static void
xglViewport (GLint   x,
	     GLint   y,
	     GLsizei width,
	     GLsizei height)
{
    xglGLOpRec gl;
    
    gl.glProc = xglViewportProc;

    gl.u.rect.x	     = x;
    gl.u.rect.y	     = y;
    gl.u.rect.width  = width;
    gl.u.rect.height = height;

    xglGLOp (&gl);
}

static void
xglScissorProc (xglGLOpPtr pOp)
{
    cctx->attrib.scissor.x	= pOp->u.rect.x;
    cctx->attrib.scissor.y	= pOp->u.rect.y;
    cctx->attrib.scissor.width  = pOp->u.rect.width;
    cctx->attrib.scissor.height = pOp->u.rect.height;
}

static void
xglScissor (GLint   x,
	    GLint   y,
	    GLsizei width,
	    GLsizei height)
{
    xglGLOpRec gl;
    
    gl.glProc = xglScissorProc;

    gl.u.rect.x	     = x;
    gl.u.rect.y	     = y;
    gl.u.rect.width  = width;
    gl.u.rect.height = height;

    xglGLOp (&gl);
}

static void
xglDrawBufferProc (xglGLOpPtr pOp)
{
    switch (pOp->u.enumeration) {
    case GL_FRONT:
    case GL_FRONT_AND_BACK:
	break;
    case GL_BACK:
	if (!cctx->doubleBuffer)
	{
	    xglRecordError (GL_INVALID_OPERATION);
	    return;
	}
	break;
    default:
	xglRecordError (GL_INVALID_ENUM);
	return;
    }
    
    cctx->attrib.drawBuffer = pOp->u.enumeration;
    
    glDrawBuffer (pOp->u.enumeration);
}

static void
xglDrawBuffer (GLenum mode)
{
    xglGLOpRec gl;
    
    gl.glProc = xglDrawBufferProc;

    gl.u.enumeration = mode;

    xglGLOp (&gl);
}

static void
xglReadBufferProc (xglGLOpPtr pOp)
{
    switch (pOp->u.enumeration) {
    case GL_FRONT:
	break;
    case GL_BACK:
	if (!cctx->doubleBuffer)
	{
	    xglRecordError (GL_INVALID_OPERATION);
	    return;
	}
	break;
    default:
	xglRecordError (GL_INVALID_ENUM);
	return;
    }
    
    cctx->attrib.readBuffer = pOp->u.enumeration;
	
    glReadBuffer (pOp->u.enumeration);
}

static void
xglReadBuffer (GLenum mode)
{
    xglGLOpRec gl;
    
    gl.glProc = xglReadBufferProc;

    gl.u.enumeration = mode;

    xglGLOp (&gl);
}

static void
xglDisableProc (xglGLOpPtr pOp)
{
    xglTexUnitPtr pTexUnit = &cctx->attrib.texUnits[cctx->activeTexUnit];
    
    switch (pOp->u.enumeration) {
    case GL_SCISSOR_TEST:
	cctx->attrib.scissorTest = GL_FALSE;
	return;
    case GL_TEXTURE_1D:
	pTexUnit->enabled &= ~XGL_TEXTURE_1D_BIT;
	break;
    case GL_TEXTURE_2D:
	pTexUnit->enabled &= ~XGL_TEXTURE_2D_BIT;
	break;
    case GL_TEXTURE_3D:
	pTexUnit->enabled &= ~XGL_TEXTURE_3D_BIT;
	break;
    case GL_TEXTURE_RECTANGLE_NV:
	pTexUnit->enabled &= ~XGL_TEXTURE_RECTANGLE_BIT;
	break;
    case GL_TEXTURE_CUBE_MAP_ARB:
	pTexUnit->enabled &= ~XGL_TEXTURE_CUBE_MAP_BIT;
	break;
    default:
	break;
    }

    glDisable (pOp->u.enumeration);
}

static void
xglDisable (GLenum cap)
{
    xglGLOpRec gl;
    
    gl.glProc = xglDisableProc;

    gl.u.enumeration = cap;

    xglGLOp (&gl);
}

static void
xglEnableProc (xglGLOpPtr pOp)
{
    xglTexUnitPtr pTexUnit = &cctx->attrib.texUnits[cctx->activeTexUnit];
    
    switch (pOp->u.enumeration) {
    case GL_SCISSOR_TEST:
	cctx->attrib.scissorTest = GL_TRUE;
	return;
    case GL_DEPTH_TEST:
	if (!cctx->depthBits)
	    return;
    case GL_STENCIL_TEST:
	if (!cctx->stencilBits)
	    return;
    case GL_TEXTURE_1D:
	pTexUnit->enabled |= XGL_TEXTURE_1D_BIT;
	break;
    case GL_TEXTURE_2D:
	pTexUnit->enabled |= XGL_TEXTURE_2D_BIT;
	break;
    case GL_TEXTURE_3D:
	pTexUnit->enabled |= XGL_TEXTURE_3D_BIT;
	break;
    case GL_TEXTURE_RECTANGLE_NV:
	pTexUnit->enabled |= XGL_TEXTURE_RECTANGLE_BIT;
	break;
    case GL_TEXTURE_CUBE_MAP_ARB:
	pTexUnit->enabled |= XGL_TEXTURE_CUBE_MAP_BIT;
	break;
    default:
	break;
    }

    glEnable (pOp->u.enumeration);
}

static void
xglEnable (GLenum cap)
{
    xglGLOpRec gl;
    
    gl.glProc = xglEnableProc;

    gl.u.enumeration = cap;

    xglGLOp (&gl);
}

static void
xglDeleteTexObj (xglTexObjPtr pTexObj)
{
    if (pTexObj->pPixmap)
    {
	ScreenPtr pScreen = pTexObj->pPixmap->drawable.pScreen;
	
	(*pScreen->DestroyPixmap) (pTexObj->pPixmap);
    }

    if (pTexObj->name)
    {
	glDeleteTextures (1, &pTexObj->name);
    }

    pTexObj->key     = 0;
    pTexObj->name    = 0;
    pTexObj->pPixmap = NULL;
}

static void
xglRefTexObj (xglTexObjPtr pTexObj)
{
    if (!pTexObj)
	return;
    
    pTexObj->refcnt++;
}

static void
xglUnrefTexObj (xglTexObjPtr pTexObj)
{
    if (!pTexObj)
	return;
    
    pTexObj->refcnt--;
    if (pTexObj->refcnt)
	return;

    xglDeleteTexObj (pTexObj);

    xfree (pTexObj);
}

static void
xglPushAttribProc (xglGLOpPtr pOp)
{
    xglGLAttributesPtr pAttrib;
    GLint	       depth;
    
    glGetIntegerv (GL_MAX_ATTRIB_STACK_DEPTH, &depth);
    
    if (cctx->nAttribStack == depth)
    {
	xglRecordError (GL_STACK_OVERFLOW);
	return;
    }
    
    cctx->pAttribStack =
	realloc (cctx->pAttribStack,
		 sizeof (xglGLAttributesRec) * (cctx->nAttribStack + 1));
    
    if (!cctx->pAttribStack)
    {
	xglRecordError (GL_OUT_OF_MEMORY);
	return;
    }

    pAttrib = &cctx->pAttribStack[cctx->nAttribStack];
    pAttrib->mask = pOp->u.bitfield;
    
    *pAttrib = cctx->attrib;
    
    if (pOp->u.bitfield & GL_TEXTURE_BIT)
    {
	int i;
	
	for (i = 0; i < cctx->maxTexUnits; i++)
	{
	    xglRefTexObj (pAttrib->texUnits[i].p1D);
	    xglRefTexObj (pAttrib->texUnits[i].p2D);
	    xglRefTexObj (pAttrib->texUnits[i].p3D);
	    xglRefTexObj (pAttrib->texUnits[i].pRect);
	    xglRefTexObj (pAttrib->texUnits[i].pCubeMap);
	}
    }
    
    cctx->nAttribStack++;
    
    glPushAttrib (pOp->u.bitfield);
}

static void
xglPushAttrib (GLbitfield mask)
{
    xglGLOpRec gl;
    
    gl.glProc = xglPushAttribProc;

    gl.u.bitfield = mask;

    xglGLOp (&gl);
}

static void
xglPopAttribProc (xglGLOpPtr pOp)
{
    xglGLAttributesPtr pAttrib;
    GLbitfield	       mask;
    
    if (!cctx->nAttribStack)
    {
	xglRecordError (GL_STACK_UNDERFLOW);
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

    if (mask & GL_TEXTURE_BIT)
    {
	int i;
	
	for (i = 0; i < cctx->maxTexUnits; i++)
	{
	    xglUnrefTexObj (cctx->attrib.texUnits[i].p1D);
	    xglUnrefTexObj (cctx->attrib.texUnits[i].p2D);
	    xglUnrefTexObj (cctx->attrib.texUnits[i].p3D);
	    xglUnrefTexObj (cctx->attrib.texUnits[i].pRect);
	    xglUnrefTexObj (cctx->attrib.texUnits[i].pCubeMap);

	    cctx->attrib.texUnits[i] = pAttrib->texUnits[i];
	}
    }
    else if (mask & GL_ENABLE_BIT)
    {
	int i;
	
	for (i = 0; i < cctx->maxTexUnits; i++)
	   cctx->attrib.texUnits[i].enabled = pAttrib->texUnits[i].enabled;
    }
    
    cctx->pAttribStack =
	realloc (cctx->pAttribStack,
		 sizeof (xglGLAttributesRec) * cctx->nAttribStack);
    
    glPopAttrib ();
}

static void
xglPopAttrib (void)
{
    xglGLOpRec gl;
    
    gl.glProc = xglPopAttribProc;

    xglGLOp (&gl);
}

static GLuint
xglActiveTextureBinding (GLenum target)
{
    xglTexObjPtr pTexObj;
    
    switch (target) {
    case GL_TEXTURE_BINDING_1D:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p1D;
	break;
    case GL_TEXTURE_BINDING_2D:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p2D;
	break;
    case GL_TEXTURE_BINDING_3D:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p3D;
	break;
    case GL_TEXTURE_BINDING_RECTANGLE_NV:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].pRect;
	break;
    case GL_TEXTURE_BINDING_CUBE_MAP_ARB:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].pCubeMap;
	break;
    default:
	return 0;
    }

    if (pTexObj)
	return pTexObj->key;
    
    return 0;
}

#define DOUBLE_TO_BOOLEAN(X) ((X) ? GL_TRUE : GL_FALSE)
#define INT_TO_BOOLEAN(I)    ((I) ? GL_TRUE : GL_FALSE)
#define ENUM_TO_BOOLEAN(E)   ((E) ? GL_TRUE : GL_FALSE)

static void
xglGetBooleanv (GLenum	  pname,
		GLboolean *params)
{
    switch (pname) {
    case GL_CURRENT_RASTER_POSITION:
    {
	GLdouble v[4];
	
	glGetDoublev (GL_CURRENT_RASTER_POSITION, v);

	params[0] = DOUBLE_TO_BOOLEAN (v[0] - (GLdouble) cctx->drawXoff);
	params[1] = DOUBLE_TO_BOOLEAN (v[1] - (GLdouble) cctx->drawYoff);
	params[2] = DOUBLE_TO_BOOLEAN (v[2]);
	params[3] = DOUBLE_TO_BOOLEAN (v[3]);
    } break;
    case GL_DOUBLEBUFFER:
	params[0] = cctx->doubleBuffer;
	break;
    case GL_DEPTH_BITS:
	params[0] = INT_TO_BOOLEAN (cctx->depthBits);
	break;
    case GL_STENCIL_BITS:
	params[0] = INT_TO_BOOLEAN (cctx->stencilBits);
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
    case GL_TEXTURE_RECTANGLE_NV:
	/* should be safe to fall-through here */
    default:
	glGetBooleanv (pname, params);
    }
}

#define INT_TO_DOUBLE(I)     ((GLdouble) (I))
#define ENUM_TO_DOUBLE(E)    ((GLdouble) (E))
#define BOOLEAN_TO_DOUBLE(B) ((B) ? 1.0F : 0.0F)

static void
xglGetDoublev (GLenum	pname,
	       GLdouble *params)
{
    switch (pname) {
    case GL_CURRENT_RASTER_POSITION:
	glGetDoublev (GL_CURRENT_RASTER_POSITION, params);

	params[0] -= (GLdouble) cctx->drawXoff;
	params[1] -= (GLdouble) cctx->drawYoff;
	break;
    case GL_DOUBLEBUFFER:
	params[0] = BOOLEAN_TO_DOUBLE (cctx->doubleBuffer);
	break;
    case GL_DEPTH_BITS:
	params[0] = INT_TO_DOUBLE (cctx->depthBits);
	break;
    case GL_STENCIL_BITS:
	params[0] = INT_TO_DOUBLE (cctx->stencilBits);
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
    case GL_TEXTURE_RECTANGLE_NV:
	params[0] = xglActiveTextureBinding (pname);
	break;
    case GL_MAX_TEXTURE_UNITS_ARB:
	params[0] = cctx->maxTexUnits;
	break;
    default:
	glGetDoublev (pname, params);
    }
}

#define INT_TO_FLOAT(I)     ((GLfloat) (I))
#define ENUM_TO_FLOAT(E)    ((GLfloat) (E))
#define BOOLEAN_TO_FLOAT(B) ((B) ? 1.0F : 0.0F)

static void
xglGetFloatv (GLenum  pname,
	      GLfloat *params)
{
    switch (pname) {
    case GL_CURRENT_RASTER_POSITION:
	glGetFloatv (GL_CURRENT_RASTER_POSITION, params);

	params[0] -= (GLfloat) cctx->drawXoff;
	params[1] -= (GLfloat) cctx->drawYoff;
	break;
    case GL_DOUBLEBUFFER:
	params[0] = BOOLEAN_TO_FLOAT (cctx->doubleBuffer);
	break;
    case GL_DEPTH_BITS:
	params[0] = INT_TO_FLOAT (cctx->depthBits);
	break;
    case GL_STENCIL_BITS:
	params[0] = INT_TO_FLOAT (cctx->stencilBits);
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
    case GL_TEXTURE_RECTANGLE_NV:
	params[0] = xglActiveTextureBinding (pname);
	break;
    case GL_MAX_TEXTURE_UNITS_ARB:
	params[0] = cctx->maxTexUnits;
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
    case GL_CURRENT_RASTER_POSITION:
	glGetIntegerv (GL_CURRENT_RASTER_POSITION, params);
	
	params[0] -= (GLint) cctx->drawXoff;
	params[1] -= (GLint) cctx->drawYoff;
	break;
    case GL_DOUBLEBUFFER:
	params[0] = BOOLEAN_TO_INT (cctx->doubleBuffer);
	break;
    case GL_DEPTH_BITS:
	params[0] = cctx->depthBits;
	break;
    case GL_STENCIL_BITS:
	params[0] = cctx->stencilBits;
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
    case GL_TEXTURE_RECTANGLE_NV:
	params[0] = xglActiveTextureBinding (pname);
	break;
    case GL_MAX_TEXTURE_UNITS_ARB:
	params[0] = cctx->maxTexUnits;
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
    GLenum error = cctx->errorValue;
	
    if (error != GL_NO_ERROR)
    {
	cctx->errorValue = GL_NO_ERROR;
	return error;
    }

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
	    pTexObj->key     = name;
	    pTexObj->name    = *textures;
	    pTexObj->pPixmap = NULL;
	    pTexObj->refcnt  = 1;

	    xglHashInsert (cctx->shared->texObjects, name, pTexObj);
	}
	else
	{
	    xglRecordError (GL_OUT_OF_MEMORY);
	}
	
	*textures++ = name++;
    }
}

static void
xglBindTextureProc (xglGLOpPtr pOp)
{
    xglTexObjPtr *ppTexObj;
    
    switch (pOp->u.bind_texture.target) {
    case GL_TEXTURE_1D:
	ppTexObj = &cctx->attrib.texUnits[cctx->activeTexUnit].p1D;
	break;
    case GL_TEXTURE_2D:
	ppTexObj = &cctx->attrib.texUnits[cctx->activeTexUnit].p2D;
	break;
    case GL_TEXTURE_3D:
	ppTexObj = &cctx->attrib.texUnits[cctx->activeTexUnit].p3D;
	break;
    case GL_TEXTURE_RECTANGLE_NV:
	ppTexObj = &cctx->attrib.texUnits[cctx->activeTexUnit].pRect;
	break;
    case GL_TEXTURE_CUBE_MAP_ARB:
	ppTexObj = &cctx->attrib.texUnits[cctx->activeTexUnit].pCubeMap;
	break;
    default:
	xglRecordError (GL_INVALID_ENUM);
	return;
    }
    
    if (pOp->u.bind_texture.texture)
    {
	xglTexObjPtr pTexObj;

	pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects,
						pOp->u.bind_texture.texture);
	if (!pTexObj)
	{
	    pTexObj = xalloc (sizeof (xglTexObjRec));
	    if (!pTexObj)
	    {
		xglRecordError (GL_OUT_OF_MEMORY);
		return;
	    }

	    pTexObj->key     = pOp->u.bind_texture.texture;
	    pTexObj->pPixmap = NULL;
	    pTexObj->refcnt  = 1;

	    glGenTextures (1, &pTexObj->name);
	    
	    xglHashInsert (cctx->shared->texObjects,
			   pOp->u.bind_texture.texture,
			   pTexObj);
	}

	xglRefTexObj (pTexObj);
	xglUnrefTexObj (*ppTexObj);
	*ppTexObj = pTexObj;

	glBindTexture (pOp->u.bind_texture.target, pTexObj->name);
    }
    else
    {
	xglUnrefTexObj (*ppTexObj);
	*ppTexObj = NULL;
	
	glBindTexture (pOp->u.bind_texture.target, 0);
    }
}

static void
xglBindTexture (GLenum target,
		GLuint texture)
{
    xglGLOpRec gl;
    
    gl.glProc = xglBindTextureProc;

    gl.u.bind_texture.target  = target;
    gl.u.bind_texture.texture = texture;
    
    xglGLOp (&gl);
}

static void
xglSetupTextures (void)
{
    xglGLContextPtr pContext = cctx;
    xglTexUnitPtr   pTexUnit;
    xglTexObjPtr    pTexObj[XGL_MAX_TEXTURE_UNITS];
    int		    i, activeTexUnit;
    
    for (i = 0; i < pContext->maxTexUnits; i++)
    {
	pTexObj[i] = NULL;

	pTexUnit = &pContext->attrib.texUnits[i];
	if (pTexUnit->enabled)
	{
	    if (pTexUnit->enabled & XGL_TEXTURE_RECTANGLE_BIT)
		pTexObj[i] = pTexUnit->pRect;
	    else if (pTexUnit->enabled & XGL_TEXTURE_2D_BIT)
		pTexObj[i] = pTexUnit->p2D;
	    
	    if (pTexObj[i] && pTexObj[i]->pPixmap)
	    {
		if (!xglSyncSurface (&pTexObj[i]->pPixmap->drawable))
		    pTexObj[i] = NULL;
	    }
	    else
		pTexObj[i] = NULL;
	}
    }

    if (pContext != cctx)
    {
	XGL_SCREEN_PRIV (pContext->pDrawBuffer->pGC->pScreen);
	
	glitz_drawable_finish (pScreenPriv->drawable);
	
	xglSetCurrentContext (pContext);
    }
    
    activeTexUnit = cctx->activeTexUnit;
    for (i = 0; i < pContext->maxTexUnits; i++)
    {
	if (pTexObj[i])
	{
	    XGL_PIXMAP_PRIV (pTexObj[i]->pPixmap);

	    activeTexUnit = GL_TEXTURE0_ARB + i;
	    cctx->ActiveTextureARB (activeTexUnit);
	    glitz_context_bind_texture (cctx->context, pPixmapPriv->surface);
	}
    }

    if (cctx->activeTexUnit != activeTexUnit) 
	cctx->ActiveTextureARB (cctx->activeTexUnit);  
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
	xglRecordError (GL_INVALID_VALUE);
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
	    xglRecordError (GL_INVALID_VALUE);
	    return GL_FALSE;
	}

	pTexObj = (xglTexObjPtr) xglHashLookup (cctx->shared->texObjects,
						textures[i]);
	if (!pTexObj)
	{
	    xglRecordError (GL_INVALID_VALUE);
	    return GL_FALSE;
	}
	
	if (pTexObj->name == 0 ||
	    glAreTexturesResident (1, &pTexObj->name, &resident))
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
	    xglDeleteTexObj (pTexObj);
	    xglUnrefTexObj (pTexObj);
	    xglHashRemove (cctx->shared->texObjects, *textures);
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
	return GL_TRUE;

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
	xglRecordError (GL_INVALID_VALUE);
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
	if (pTexObj && pTexObj->name)
	    glPrioritizeTextures (1, &pTexObj->name, &priorities[i]);
    }
}

static void
xglTexParameterfvProc (xglGLOpPtr pOp)
{
    xglTexObjPtr pTexObj;

    glTexParameterfv (pOp->u.tex_parameter_fv.target,
		      pOp->u.tex_parameter_fv.pname,
		      pOp->u.tex_parameter_fv.params);

    switch (pOp->u.tex_parameter_fv.target) {
    case GL_TEXTURE_2D:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p2D;
	break;
    case GL_TEXTURE_RECTANGLE_NV:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].pRect;
	break;
    default:
	pTexObj = NULL;
	break;
    }

    if (pTexObj && pTexObj->pPixmap)
    {
	XGL_PIXMAP_PRIV (pTexObj->pPixmap);

	/* texture parameters should eventually go into a
	   glitz_texture_object_t */
	glitz_context_bind_texture (cctx->context, pPixmapPriv->surface);

	glTexParameterfv (pOp->u.tex_parameter_fv.target,
			  pOp->u.tex_parameter_fv.pname,
			  pOp->u.tex_parameter_fv.params);
	
	glBindTexture (pOp->u.tex_parameter_fv.target, pTexObj->name);
    }
}

static void
xglTexParameterfv (GLenum	 target,
		   GLenum	 pname,
		   const GLfloat *params)
{
    xglGLOpRec gl;
    
    gl.glProc = xglTexParameterfvProc;

    gl.u.tex_parameter_fv.target = target;
    gl.u.tex_parameter_fv.pname  = pname;

    switch (pname) {
    case GL_TEXTURE_BORDER_COLOR:
	gl.u.tex_parameter_fv.params[3] = params[3];
	gl.u.tex_parameter_fv.params[2] = params[2];
	gl.u.tex_parameter_fv.params[1] = params[1];
	/* fall-through */
    default:
	gl.u.tex_parameter_fv.params[0] = params[0];
	break;
    }
    
    xglGLOp (&gl);
}

static void
xglTexParameterivProc (xglGLOpPtr pOp)
{
    xglTexObjPtr pTexObj;

    glTexParameteriv (pOp->u.tex_parameter_iv.target,
		      pOp->u.tex_parameter_iv.pname,
		      pOp->u.tex_parameter_iv.params);

    switch (pOp->u.tex_parameter_iv.target) {
    case GL_TEXTURE_2D:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p2D;
	break;
    case GL_TEXTURE_RECTANGLE_NV:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].pRect;
	break;
    default:
	pTexObj = NULL;
	break;
    }

    if (pTexObj && pTexObj->pPixmap)
    {
	XGL_PIXMAP_PRIV (pTexObj->pPixmap);

	/* texture parameters should eventually go into a
	   glitz_texture_object_t */
	glitz_context_bind_texture (cctx->context, pPixmapPriv->surface);

	glTexParameteriv (pOp->u.tex_parameter_iv.target,
			  pOp->u.tex_parameter_iv.pname,
			  pOp->u.tex_parameter_iv.params);
	
	glBindTexture (pOp->u.tex_parameter_iv.target, pTexObj->name);
    }
}

static void
xglTexParameteriv (GLenum      target,
		   GLenum      pname,
		   const GLint *params)
{
    xglGLOpRec gl;
    
    gl.glProc = xglTexParameterivProc;

    gl.u.tex_parameter_iv.target = target;
    gl.u.tex_parameter_iv.pname  = pname;

    switch (pname) {
    case GL_TEXTURE_BORDER_COLOR:
	gl.u.tex_parameter_iv.params[3] = params[3];
	gl.u.tex_parameter_iv.params[2] = params[2];
	gl.u.tex_parameter_iv.params[1] = params[1];
	/* fall-through */
    default:
	gl.u.tex_parameter_iv.params[0] = params[0];
	break;
    }
    
    xglGLOp (&gl);
}

static void
xglTexParameterf (GLenum  target,
		  GLenum  pname,
		  GLfloat param)
{
    xglTexParameterfv (target, pname, (const GLfloat *) &param);
}

static void
xglTexParameteri (GLenum target,
		  GLenum pname,
		  GLint  param)
{
    xglTexParameteriv (target, pname, (const GLint *) &param);
}

static void
xglGetTexLevelParameterfv (GLenum  target,
			   GLint   level,
			   GLenum  pname,
			   GLfloat *params)
{
    xglTexObjPtr pTexObj;

    switch (target) {
    case GL_TEXTURE_2D:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p2D;
	break;
    case GL_TEXTURE_RECTANGLE_NV:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].pRect;
	break;
    default:
	pTexObj = NULL;
	break;
    }

    if (pTexObj && pTexObj->pPixmap)
    {
	XGL_PIXMAP_PRIV (pTexObj->pPixmap);

	glitz_context_bind_texture (cctx->context, pPixmapPriv->surface);

	glGetTexLevelParameterfv (target, level, pname, params);
	
	glBindTexture (target, pTexObj->name);
    }
    else
	glGetTexLevelParameterfv (target, level, pname, params);
}

static void
xglGetTexLevelParameteriv (GLenum target,
			   GLint  level,
			   GLenum pname,
			   GLint  *params)
{
    xglTexObjPtr pTexObj;

    switch (target) {
    case GL_TEXTURE_2D:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p2D;
	break;
    case GL_TEXTURE_RECTANGLE_NV:
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].pRect;
	break;
    default:
	pTexObj = NULL;
	break;
    }

    if (pTexObj && pTexObj->pPixmap)
    {
	XGL_PIXMAP_PRIV (pTexObj->pPixmap);

	glitz_context_bind_texture (cctx->context, pPixmapPriv->surface);

	glGetTexLevelParameteriv (target, level, pname, params);
	
	glBindTexture (target, pTexObj->name);
    }
    else
	glGetTexLevelParameteriv (target, level, pname, params);
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
	    xglRecordError (GL_OUT_OF_MEMORY);
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
	xglRecordError (GL_INVALID_VALUE);
	return;
    }

    if (cctx->list)
    {
	xglRecordError (GL_INVALID_OPERATION);
	return;
    }

    cctx->pList = xglCreateList ();
    if (!cctx->pList)
    {
	xglRecordError (GL_OUT_OF_MEMORY);
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
	xglRecordError (GL_INVALID_OPERATION);
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

static void
xglDrawList (GLuint list)
{
    RegionRec region;
    BoxRec    scissor, box;
    BoxPtr    pBox;
    int	      nBox;
    
    XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);

    while (nBox--)
    {
	box = *pBox++;

	if (cctx->attrib.scissorTest)
	    XGL_GLX_INTERSECT_BOX (&box, &scissor);
	
	if (box.x1 < box.x2 && box.y1 < box.y2)
	{
	    XGL_GLX_SET_SCISSOR_BOX (&box);

	    glCallList (list);

	    XGL_GLX_DRAW_DAMAGE (&box, &region);
	}
    }
}

static void
xglCallDisplayList (GLuint list,
		    int	   nesting)
{
    if (nesting > cctx->maxListNesting)
	return;

    if (!list)
    {
	xglRecordError (GL_INVALID_VALUE);
	return;
    }
    
    if (cctx->list)
    {
	if (!xglResizeList (cctx->pList, cctx->pList->nOp + 1))
	{
	    xglRecordError (GL_OUT_OF_MEMORY);
	    return;
	}

	cctx->pList->pOp[cctx->pList->nOp].type   = XGL_LIST_OP_LIST;
	cctx->pList->pOp[cctx->pList->nOp].u.list = list;
	cctx->pList->nOp++;
    }
    else
    {
	xglDisplayListPtr pDisplayList;

	pDisplayList = (xglDisplayListPtr)
	    xglHashLookup (cctx->shared->displayLists, list);
	if (pDisplayList)
	{
	    xglListOpPtr pOp = pDisplayList->pOp;
	    int		 nOp = pDisplayList->nOp;
	    
	    while (nOp--)
	    {
		switch (pOp->type) {
		case XGL_LIST_OP_CALLS:
		    glCallList (pOp->u.list);
		    break;
		case XGL_LIST_OP_DRAW:
		    xglDrawList (pOp->u.list);
		    break;
		case XGL_LIST_OP_GL:
		    (*pOp->u.gl->glProc) (pOp->u.gl);
		    break;
		case XGL_LIST_OP_LIST:
		    xglCallDisplayList (pOp->u.list, nesting + 1);
		    break;
		}

		pOp++;
	    }
	}
    }
}

static void
xglCallList (GLuint list)
{
    xglCallDisplayList (list, 1);
}

static void
xglCallLists (GLsizei	   n,
	      GLenum	   type,
	      const GLvoid *lists)
{
    GLuint list;
    GLint  base, i;

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
	    xglRecordError (GL_INVALID_ENUM);
	    return;
	}
	
	xglCallDisplayList (base + list, 1);
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
	xglRecordError (GL_INVALID_VALUE);
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
    glFlush ();

    if (cctx->pDrawBuffer->pDrawable)
    {
	xglGLBufferPtr pBuffer = cctx->pDrawBuffer;
    
	if (REGION_NOTEMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage))
	{
	    DamageDamageRegion (pBuffer->pDrawable, &pBuffer->damage);
	    REGION_EMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage);
	}
    }
}

static void
xglFinish (void)
{
    glFinish ();

    if (cctx->pDrawBuffer->pDrawable)
    {
	xglGLBufferPtr pBuffer = cctx->pDrawBuffer;
    
	if (REGION_NOTEMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage))
	{
	    DamageDamageRegion (pBuffer->pDrawable, &pBuffer->damage);
	    REGION_EMPTY (pBuffer->pDrawable->pScreen, &pBuffer->damage);
	}
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
	BoxRec    scissor, box;
	BoxPtr    pBox;
	int	  nBox;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_SET_SCISSOR_BOX (&box);

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
	    BoxRec    scissor, box;
	    BoxPtr    pBox;
	    int	      nBox;
    
	    XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);
    
	    while (nBox--)
	    {
		box = *pBox++;
		
		if (cctx->attrib.scissorTest)
		    XGL_GLX_INTERSECT_BOX (&box, &scissor);
		
		if (box.x1 < box.x2 && box.y1 < box.y2)
		{
		    XGL_GLX_SET_SCISSOR_BOX (&box);

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
	BoxRec    scissor, box;
	BoxPtr    pBox;
	int	  nBox;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);
    
	while (nBox--)
	{
	    box = *pBox++;
	    
	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_SET_SCISSOR_BOX (&box);

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
	BoxRec    scissor, box;
	BoxPtr    pBox;
	int	  nBox;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_SET_SCISSOR_BOX (&box);

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
	BoxRec    scissor, box;
	BoxPtr    pBox;
	int	  nBox;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);

	while (nBox--)
	{
	    box = *pBox++;
	    
	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_SET_SCISSOR_BOX (&box);

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
	glBitmap (width, height, xorig, yorig, 0, 0, bitmap);
	glEndList ();
	
	listMode = cctx->listMode;
    }
    else
	listMode = GL_COMPILE_AND_EXECUTE;
	
    if (listMode == GL_COMPILE_AND_EXECUTE && width && height)
    {
	RegionRec region;
	BoxRec    scissor, box;
	BoxPtr    pBox;
	int	  nBox;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);
    
	while (nBox--)
	{
	    box = *pBox++;
	    
	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_SET_SCISSOR_BOX (&box);

		glBitmap (width, height, xorig, yorig, 0, 0, bitmap);
		
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }
    
    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);

    glBitmap (0, 0, 0, 0, xmove, ymove, NULL);
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
	BoxRec    scissor, box;
	BoxPtr    pBox;
	int	  nBox;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);
    
	while (nBox--)
	{
	    box = *pBox++;
	    
	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_SET_SCISSOR_BOX (&box);

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
	BoxRec    scissor;
	BoxPtr    pBox;
	int	  nBox;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);
	    
	if (nBox == 1)
	{
	    BoxRec box = *pBox;

	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    XGL_GLX_SET_SCISSOR_BOX (&box);
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
	BoxRec    scissor, box;
	BoxPtr    pBox;
	int	  nBox;
	GLuint	  list;
    
	XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);
	
	if (cctx->list)
	{
	    list = cctx->pList->pOp[cctx->pList->nOp - 1].u.list;
	}
	else
	{
	    if (nBox == 1)
	    {
		box = *pBox++;
	    
		if (cctx->attrib.scissorTest)
		    XGL_GLX_INTERSECT_BOX (&box, &scissor);
		
		XGL_GLX_DRAW_DAMAGE (&box, &region);
		return;
	    }
	
	    list = cctx->groupList;
	}

	glEndList ();

	while (nBox--)
	{
	    box = *pBox++;
	    
	    if (cctx->attrib.scissorTest)
		XGL_GLX_INTERSECT_BOX (&box, &scissor);
	    
	    if (box.x1 < box.x2 && box.y1 < box.y2)
	    {
		XGL_GLX_SET_SCISSOR_BOX (&box);

		glCallList (list);
		
		XGL_GLX_DRAW_DAMAGE (&box, &region);
	    }
	}
    }
	
    if (cctx->list)
	xglStartList (XGL_LIST_OP_CALLS, cctx->listMode);
}

static void
xglCopyPixelsProc (xglGLOpPtr pOp)
{
    RegionRec region;
    BoxRec    scissor, box;
    BoxPtr    pBox;
    int	      nBox;

    XGL_GLX_DRAW_PROLOGUE (pBox, nBox, &scissor);

    while (nBox--)
    {
	box = *pBox++;
	
	if (cctx->attrib.scissorTest)
	    XGL_GLX_INTERSECT_BOX (&box, &scissor);
	
	if (box.x1 < box.x2 && box.y1 < box.y2)
	{
	    XGL_GLX_SET_SCISSOR_BOX (&box);

	    glCopyPixels (pOp->u.copy_pixels.x + cctx->pReadBuffer->xOff,
			  pOp->u.copy_pixels.y + cctx->pReadBuffer->yOff,
			  pOp->u.copy_pixels.width,
			  pOp->u.copy_pixels.height,
			  pOp->u.copy_pixels.type);

	    if (pOp->u.copy_pixels.type == GL_COLOR)
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
    xglGLOpRec gl;

    gl.glProc = xglCopyPixelsProc;

    gl.u.copy_pixels.x	    = x;
    gl.u.copy_pixels.y	    = y;
    gl.u.copy_pixels.width  = width;
    gl.u.copy_pixels.height = height;
    gl.u.copy_pixels.type   = type;

    xglGLOp (&gl);   
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
    glReadPixels (x + cctx->pReadBuffer->xOff,
		  y + cctx->pReadBuffer->yOff,
		  width, height, format, type, pixels);
}

static void
xglCopyTexImage1DProc (xglGLOpPtr pOp)
{
    glCopyTexImage1D (pOp->u.copy_tex_image_1d.target,
		      pOp->u.copy_tex_image_1d.level,
		      pOp->u.copy_tex_image_1d.internalformat,
		      pOp->u.copy_tex_image_1d.x + cctx->pReadBuffer->xOff,
		      pOp->u.copy_tex_image_1d.y + cctx->pReadBuffer->yOff,
		      pOp->u.copy_tex_image_1d.width,
		      pOp->u.copy_tex_image_1d.border);
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
    xglGLOpRec gl;
    
    gl.glProc = xglCopyTexImage1DProc;
    
    gl.u.copy_tex_image_1d.target	  = target;
    gl.u.copy_tex_image_1d.level	  = level;
    gl.u.copy_tex_image_1d.internalformat = internalformat;
    gl.u.copy_tex_image_1d.x		  = x;
    gl.u.copy_tex_image_1d.y		  = y;
    gl.u.copy_tex_image_1d.width	  = width;
    gl.u.copy_tex_image_1d.border	  = border;

    xglGLOp (&gl);
}

static void
xglCopyTexImage2DProc (xglGLOpPtr pOp)
{
    glCopyTexImage2D (pOp->u.copy_tex_image_2d.target,
		      pOp->u.copy_tex_image_2d.level,
		      pOp->u.copy_tex_image_2d.internalformat,
		      pOp->u.copy_tex_image_2d.x + cctx->pReadBuffer->xOff,
		      pOp->u.copy_tex_image_2d.y + cctx->pReadBuffer->yOff,
		      pOp->u.copy_tex_image_2d.width,
		      pOp->u.copy_tex_image_2d.height,
		      pOp->u.copy_tex_image_2d.border);
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
    xglGLOpRec gl;
    
    gl.glProc = xglCopyTexImage2DProc;

    gl.u.copy_tex_image_2d.target	  = target;
    gl.u.copy_tex_image_2d.level	  = level;
    gl.u.copy_tex_image_2d.internalformat = internalformat;
    gl.u.copy_tex_image_2d.x		  = x;
    gl.u.copy_tex_image_2d.y		  = y;
    gl.u.copy_tex_image_2d.width	  = width;
    gl.u.copy_tex_image_2d.height	  = height;
    gl.u.copy_tex_image_2d.border	  = border;

    xglGLOp (&gl);
}

static void
xglCopyTexSubImage1DProc (xglGLOpPtr pOp)
{
    glCopyTexSubImage1D (pOp->u.copy_tex_sub_image_1d.target,
			 pOp->u.copy_tex_sub_image_1d.level,
			 pOp->u.copy_tex_sub_image_1d.xoffset,
			 pOp->u.copy_tex_sub_image_1d.x +
			 cctx->pReadBuffer->xOff,
			 pOp->u.copy_tex_sub_image_1d.y +
			 cctx->pReadBuffer->yOff,
			 pOp->u.copy_tex_sub_image_1d.width);
}

static void
xglCopyTexSubImage1D (GLenum  target,
		      GLint   level,
		      GLint   xoffset,
		      GLint   x,
		      GLint   y,
		      GLsizei width)
{
    xglGLOpRec gl;
    
    gl.glProc = xglCopyTexSubImage1DProc;
    
    gl.u.copy_tex_sub_image_1d.target  = target;
    gl.u.copy_tex_sub_image_1d.level   = level;
    gl.u.copy_tex_sub_image_1d.xoffset = xoffset;
    gl.u.copy_tex_sub_image_1d.x       = x;
    gl.u.copy_tex_sub_image_1d.y       = y;
    gl.u.copy_tex_sub_image_1d.width   = width;

    xglGLOp (&gl);
}

static void
xglCopyTexSubImage2DProc (xglGLOpPtr pOp)
{
    glCopyTexSubImage2D (pOp->u.copy_tex_sub_image_2d.target,
			 pOp->u.copy_tex_sub_image_2d.level,
			 pOp->u.copy_tex_sub_image_2d.xoffset,
			 pOp->u.copy_tex_sub_image_2d.yoffset,
			 pOp->u.copy_tex_sub_image_2d.x +
			 cctx->pReadBuffer->xOff,
			 pOp->u.copy_tex_sub_image_2d.y +
			 cctx->pReadBuffer->yOff,
			 pOp->u.copy_tex_sub_image_2d.width,
			 pOp->u.copy_tex_sub_image_2d.height);
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
    xglGLOpRec gl;
    
    gl.glProc = xglCopyTexSubImage2DProc;

    gl.u.copy_tex_sub_image_2d.target  = target;
    gl.u.copy_tex_sub_image_2d.level   = level;
    gl.u.copy_tex_sub_image_2d.xoffset = xoffset;
    gl.u.copy_tex_sub_image_2d.yoffset = yoffset;
    gl.u.copy_tex_sub_image_2d.x       = x;
    gl.u.copy_tex_sub_image_2d.y       = y;
    gl.u.copy_tex_sub_image_2d.width   = width;
    gl.u.copy_tex_sub_image_2d.height  = height;

    xglGLOp (&gl);
}    

static void
xglCopyColorTableProc (xglGLOpPtr pOp)
{
    glCopyColorTable (pOp->u.copy_color_table.target,
		      pOp->u.copy_color_table.internalformat,
		      pOp->u.copy_color_table.x + cctx->pReadBuffer->xOff,
		      pOp->u.copy_color_table.y + cctx->pReadBuffer->yOff,
		      pOp->u.copy_color_table.width);
}

static void
xglCopyColorTable (GLenum  target,
		   GLenum  internalformat,
		   GLint   x,
		   GLint   y,
		   GLsizei width)
{
    xglGLOpRec gl;
    
    gl.glProc = xglCopyColorTableProc;
    
    gl.u.copy_color_table.target	 = target;
    gl.u.copy_color_table.internalformat = internalformat;
    gl.u.copy_color_table.x		 = x;
    gl.u.copy_color_table.y		 = y;
    gl.u.copy_color_table.width		 = width;
    
    xglGLOp (&gl);
}

static void
xglCopyColorSubTableProc (xglGLOpPtr pOp)
{
    glCopyColorTable (pOp->u.copy_color_sub_table.target,
		      pOp->u.copy_color_sub_table.start,
		      pOp->u.copy_color_sub_table.x + cctx->pReadBuffer->xOff,
		      pOp->u.copy_color_sub_table.y + cctx->pReadBuffer->yOff,
		      pOp->u.copy_color_sub_table.width);
}

static void
xglCopyColorSubTable (GLenum  target,
		      GLsizei start,
		      GLint   x,
		      GLint   y,
		      GLsizei width)
{
    xglGLOpRec gl;
    
    gl.glProc = xglCopyColorSubTableProc;

    gl.u.copy_color_sub_table.target = target;
    gl.u.copy_color_sub_table.start  = start;
    gl.u.copy_color_sub_table.x	     = x;
    gl.u.copy_color_sub_table.y	     = y;
    gl.u.copy_color_sub_table.width  = width;
    
    xglGLOp (&gl);
}

static void
xglCopyConvolutionFilter1DProc (xglGLOpPtr pOp)
{
    GLenum internalformat = pOp->u.copy_convolution_filter_1d.internalformat;
    
    glCopyConvolutionFilter1D (pOp->u.copy_convolution_filter_1d.target,
			       internalformat,
			       pOp->u.copy_convolution_filter_1d.x +
			       cctx->pReadBuffer->xOff,
			       pOp->u.copy_convolution_filter_1d.y +
			       cctx->pReadBuffer->yOff,
			       pOp->u.copy_convolution_filter_1d.width);
}

static void
xglCopyConvolutionFilter1D (GLenum  target,
			    GLenum  internalformat,
			    GLint   x,
			    GLint   y,
			    GLsizei width)
{
    xglGLOpRec gl;
    
    gl.glProc = xglCopyConvolutionFilter1DProc;

    gl.u.copy_convolution_filter_1d.target	   = target;
    gl.u.copy_convolution_filter_1d.internalformat = internalformat;
    gl.u.copy_convolution_filter_1d.x		   = x;
    gl.u.copy_convolution_filter_1d.y		   = y;
    gl.u.copy_convolution_filter_1d.width	   = width;
    
    xglGLOp (&gl);
}

static void
xglCopyConvolutionFilter2DProc (xglGLOpPtr pOp)
{
    GLenum internalformat = pOp->u.copy_convolution_filter_2d.internalformat;
    
    glCopyConvolutionFilter2D (pOp->u.copy_convolution_filter_2d.target,
			       internalformat,
			       pOp->u.copy_convolution_filter_2d.x +
			       cctx->pReadBuffer->xOff,
			       pOp->u.copy_convolution_filter_2d.y +
			       cctx->pReadBuffer->yOff,
			       pOp->u.copy_convolution_filter_2d.width,
			       pOp->u.copy_convolution_filter_2d.height);
}

static void
xglCopyConvolutionFilter2D (GLenum  target,
			    GLenum  internalformat,
			    GLint   x,
			    GLint   y,
			    GLsizei width,
			    GLsizei height)
{
    xglGLOpRec gl;
    
    gl.glProc = xglCopyConvolutionFilter2DProc;
    
    gl.u.copy_convolution_filter_2d.target	   = target;
    gl.u.copy_convolution_filter_2d.internalformat = internalformat;
    gl.u.copy_convolution_filter_2d.x		   = x;
    gl.u.copy_convolution_filter_2d.y		   = y;
    gl.u.copy_convolution_filter_2d.width	   = width;
    gl.u.copy_convolution_filter_2d.height	   = height;
    
    xglGLOp (&gl);
}

static void
xglCopyTexSubImage3DProc (xglGLOpPtr pOp)
{
    glCopyTexSubImage3D (pOp->u.copy_tex_sub_image_3d.target,
			 pOp->u.copy_tex_sub_image_3d.level,
			 pOp->u.copy_tex_sub_image_3d.xoffset,
			 pOp->u.copy_tex_sub_image_3d.yoffset,
			 pOp->u.copy_tex_sub_image_3d.zoffset,
			 pOp->u.copy_tex_sub_image_3d.x +
			 cctx->pReadBuffer->xOff,
			 pOp->u.copy_tex_sub_image_3d.y +
			 cctx->pReadBuffer->yOff,
			 pOp->u.copy_tex_sub_image_3d.width,
			 pOp->u.copy_tex_sub_image_3d.height);
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
    xglGLOpRec gl;
    
    gl.glProc = xglCopyTexSubImage3DProc;
    
    gl.u.copy_tex_sub_image_3d.target  = target;
    gl.u.copy_tex_sub_image_3d.level   = level;
    gl.u.copy_tex_sub_image_3d.xoffset = xoffset;
    gl.u.copy_tex_sub_image_3d.yoffset = yoffset;
    gl.u.copy_tex_sub_image_3d.zoffset = zoffset;
    gl.u.copy_tex_sub_image_3d.x       = x;
    gl.u.copy_tex_sub_image_3d.y       = y;
    gl.u.copy_tex_sub_image_3d.width   = width;
    gl.u.copy_tex_sub_image_3d.height  = height;

    xglGLOp (&gl);
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
    xglTexParameterf, /* glTexParameterf */
    xglTexParameterfv, /* glTexParameterfv */
    xglTexParameteri, /* glTexParameteri */
    xglTexParameteriv, /* glTexParameteriv */
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
    xglGetTexLevelParameterfv, /* glGetTexLevelParameterfv */
    xglGetTexLevelParameteriv, /* glGetTexLevelParameteriv */
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
xglActiveTextureARBProc (xglGLOpPtr pOp)
{
    GLenum texUnit;

    texUnit = pOp->u.enumeration - GL_TEXTURE0;
    if (texUnit < 0 || texUnit >= cctx->maxTexUnits)
    {
	xglRecordError (GL_INVALID_ENUM);
    }
    else
    {
	cctx->activeTexUnit = texUnit;
	(*cctx->ActiveTextureARB) (pOp->u.enumeration);
    }
}
static void
xglActiveTextureARB (GLenum texture)
{
    xglGLOpRec gl;
    
    gl.glProc = xglActiveTextureARBProc;

    gl.u.enumeration = texture;

    xglGLOp (&gl);
}
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
xglWindowPos3fMESAProc (xglGLOpPtr pOp)
{
    (*cctx->WindowPos3fMESA) (pOp->u.window_pos_3f.x + cctx->pDrawBuffer->xOff,
			      pOp->u.window_pos_3f.y + cctx->pDrawBuffer->yOff,
			      pOp->u.window_pos_3f.z);
}
static void
xglWindowPos3fMESA (GLfloat x, GLfloat y, GLfloat z)
{
    xglGLOpRec gl;
    
    gl.glProc = xglWindowPos3fMESAProc;

    gl.u.window_pos_3f.x = x;
    gl.u.window_pos_3f.y = y;
    gl.u.window_pos_3f.z = z;

    xglGLOp (&gl);
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


/* GL_MESA_render_texture */
#define GLX_TEXTURE_TARGET_MESA	   0x1
#define GLX_TEXTURE_2D_MESA	   0x2
#define GLX_TEXTURE_RECTANGLE_MESA 0x3
#define GLX_NO_TEXTURE_MESA	   0x4
#define GLX_FRONT_LEFT_MESA	   0x5
static int
xglXBindTexImageMESA (DrawablePtr pDrawable,
		      int	  buffer)
{
    if (buffer != GLX_FRONT_LEFT_MESA)
	return FALSE;

    if (pDrawable->type != DRAWABLE_WINDOW)
    {
        xglGLContextPtr pContext = cctx;
	xglTexUnitPtr   pTexUnit = &cctx->attrib.texUnits[cctx->activeTexUnit];
	xglTexObjPtr	pTexObj = NULL;
	
	if (xglSyncSurface (pDrawable))
	{
	    glitz_point_fixed_t point = { 1 << 16 , 1 << 16 };
	    
	    XGL_DRAWABLE_PIXMAP (pDrawable);
	    XGL_PIXMAP_PRIV (pPixmap);
    
	    /* FIXME: doesn't work with 1x1 textures */
	    glitz_surface_translate_point (pPixmapPriv->surface,
					   &point, &point);
	    if (point.x > (1 << 16) || point.y > (1 << 16))
		pTexObj = pTexUnit->pRect;
	    else
		pTexObj = pTexUnit->p2D;
	    
	    if (pTexObj)
	    {
		pPixmap->refcnt++;

		if (pTexObj->pPixmap)
		    (*pDrawable->pScreen->DestroyPixmap) (pTexObj->pPixmap);
	    
		pTexObj->pPixmap = pPixmap;
	    }
	}

	if (pContext != cctx)
	    xglSetCurrentContext (pContext);

	if (pTexObj)
	    return TRUE;
    }

    return FALSE;
}
static int
xglXReleaseTexImageMESA (DrawablePtr pDrawable,
			 int	     buffer)
{
    xglTexObjPtr pTexObj;
    
    XGL_DRAWABLE_PIXMAP (pDrawable);

    if (buffer != GLX_FRONT_LEFT_MESA)
	return FALSE;
    
    pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].p2D;
    if (pTexObj && pTexObj->pPixmap == pPixmap)
    {
	(*pDrawable->pScreen->DestroyPixmap) (pTexObj->pPixmap);
	pTexObj->pPixmap = NULL;
    }
    else
    {
	pTexObj = cctx->attrib.texUnits[cctx->activeTexUnit].pRect;
	if (pTexObj && pTexObj->pPixmap == pPixmap)
	{
	    (*pDrawable->pScreen->DestroyPixmap) (pTexObj->pPixmap);
	    pTexObj->pPixmap = NULL;
	}
	else
	    return FALSE;
    }
    
    return TRUE;
}
static int
xglXQueryDrawableMESA (DrawablePtr  pDrawable,
		       int	    attribute,
		       unsigned int *value)
{
    switch (attribute) {
    case GLX_TEXTURE_TARGET_MESA:
	if (pDrawable->type != DRAWABLE_WINDOW)
	{
	    glitz_point_fixed_t point = { 1 << 16 , 1 << 16 };
	    xglGLContextPtr	pContext = cctx;

	    XGL_DRAWABLE_PIXMAP (pDrawable);

	    if (xglCreatePixmapSurface (pPixmap))
	    {
		XGL_PIXMAP_PRIV (pPixmap);

		/* FIXME: doesn't work for 1x1 textures */
		glitz_surface_translate_point (pPixmapPriv->surface,
					       &point, &point);
		if (point.x > (1 << 16) || point.y > (1 << 16))
		    *value = GLX_TEXTURE_RECTANGLE_MESA;
		else
		    *value = GLX_TEXTURE_2D_MESA;
	    }
	    else
		*value = GLX_NO_TEXTURE_MESA;

	    if (pContext != cctx)
		xglSetCurrentContext (pContext);
	}
	else
	    *value = GLX_NO_TEXTURE_MESA;
	
	return TRUE;
    default:
	break;
    }

    *value = 0;
    return FALSE;
}

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
    xglNoOpActiveStencilFaceEXT,
    xglXBindTexImageMESA,
    xglXReleaseTexImageMESA,
    xglXQueryDrawableMESA
};

static void
xglInitExtensions (xglGLContextPtr pContext)
{
    const char *extensions;

    pContext->glRenderTableEXT = __glNoOpRenderTableEXT;

    extensions = glGetString (GL_EXTENSIONS);

    if (strstr (extensions, "GL_ARB_multitexture"))
    {
	pContext->ActiveTextureARB =
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

	glGetIntegerv (GL_MAX_LIST_NESTING, &pContext->maxListNesting);
	glGetIntegerv (GL_MAX_TEXTURE_UNITS_ARB, &pContext->maxTexUnits);
	if (pContext->maxTexUnits > XGL_MAX_TEXTURE_UNITS)
	    pContext->maxTexUnits = XGL_MAX_TEXTURE_UNITS;

	pContext->glRenderTableEXT.ActiveTextureARB = xglActiveTextureARB;
    }
    else
	pContext->maxTexUnits = 1;

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
}

static void
xglSetCurrentContext (xglGLContextPtr pContext)
{
    cctx = pContext;

    glitz_context_make_current (cctx->context);

    __glRenderTable = &__glNativeRenderTable;
    __glRenderTableEXT = &cctx->glRenderTableEXT;
}

static void
xglFreeContext (xglGLContextPtr pContext)
{
    int i;
    
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
		    xglUnrefTexObj (pTexObj);
		
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

    for (i = 0; i < pContext->maxTexUnits; i++)
    {
	xglUnrefTexObj (pContext->attrib.texUnits[i].p1D);
	xglUnrefTexObj (pContext->attrib.texUnits[i].p2D);
	xglUnrefTexObj (pContext->attrib.texUnits[i].p3D);
	xglUnrefTexObj (pContext->attrib.texUnits[i].pRect);
	xglUnrefTexObj (pContext->attrib.texUnits[i].pCubeMap);
    }
    
    if (pContext->groupList)
	glDeleteLists (pContext->groupList, 1);

    if (pContext->pAttribStack)
	xfree (pContext->pAttribStack);

    if (pContext->context)
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

    if (!iface)
	return GL_TRUE;

    return (*iface->exports.destroyContext) ((__GLcontext *) iface);
}

static GLboolean
xglLoseCurrent (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    __glXFlushContextCache ();

    if (!iface)
	return GL_TRUE;

    return (*iface->exports.loseCurrent) ((__GLcontext *) iface); 
}

static GLboolean
xglMakeCurrent (__GLcontext *gc)
{
    xglGLContextPtr	pContext = (xglGLContextPtr) gc; 
    __GLinterface	*iface = &pContext->iface;
    __GLinterface	*mIface = pContext->mIface;
    __GLdrawablePrivate *drawPriv = iface->imports.getDrawablePrivate (gc);
    __GLdrawablePrivate *readPriv = iface->imports.getReadablePrivate (gc);
    xglGLBufferPtr	pDrawBufferPriv = drawPriv->private;
    xglGLBufferPtr	pReadBufferPriv = readPriv->private;
    GLboolean		status = GL_TRUE;

    if (pReadBufferPriv->pDrawable && pDrawBufferPriv->pDrawable)
    {
	XID values[2] = { ClipByChildren, 0 };
	int status;

#ifdef COMPOSITE
	/* XXX: temporary hack for root window drawing using
	   IncludeInferiors */
	if (pDrawBufferPriv->pDrawable->type == DRAWABLE_WINDOW &&
	    (!((WindowPtr) (pDrawBufferPriv->pDrawable))->parent))
	    values[0] = IncludeInferiors;
#endif
	    
	/* this happens if client previously used this context with a buffer
	   not supported by the native GL stack */
	if (!pContext->context)
	    return GL_FALSE;
	
	/* XXX: GLX_SGI_make_current_read disabled for now */
	if (pDrawBufferPriv != pReadBufferPriv)
	    return GL_FALSE;

	pContext->pReadBuffer = pReadBufferPriv;
	pContext->pDrawBuffer = pDrawBufferPriv;

	if (!pReadBufferPriv->pGC)
	    pReadBufferPriv->pGC =
		CreateGC (pReadBufferPriv->pDrawable,
			  GCSubwindowMode | GCGraphicsExposures, values,
			  &status);
	
	ValidateGC (pReadBufferPriv->pDrawable, pReadBufferPriv->pGC);
	
	if (!pDrawBufferPriv->pGC)
	    pDrawBufferPriv->pGC =
		CreateGC (pDrawBufferPriv->pDrawable,
			  GCSubwindowMode | GCGraphicsExposures, values,
			  &status);

	ValidateGC (pDrawBufferPriv->pDrawable, pDrawBufferPriv->pGC);

	pContext->pReadBuffer = pReadBufferPriv;
	pContext->pDrawBuffer = pDrawBufferPriv;

	pReadBufferPriv->pPixmap = (PixmapPtr) 0;
	pDrawBufferPriv->pPixmap = (PixmapPtr) 0;

	/* from now on this context can only be used with native GL stack */
	if (mIface)
	{
	    (*mIface->exports.destroyContext) ((__GLcontext *) mIface);
	    pContext->mIface = NULL;
	}
    }
    else
    {
	/* this happens if client previously used this context with a buffer
	   supported by the native GL stack */
	if (!mIface)
	    return GL_FALSE;

	drawPriv->private = pDrawBufferPriv->private;
	readPriv->private = pReadBufferPriv->private;

	status = (*mIface->exports.makeCurrent) ((__GLcontext *) mIface);
	
	drawPriv->private = pDrawBufferPriv;
	readPriv->private = pReadBufferPriv;

	/* from now on this context can not be used with native GL stack */
	if (status == GL_TRUE && pContext->context)
	{
	    glitz_context_destroy (pContext->context);
	    pContext->context = NULL;
	}
    }

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

    if (!iface || !ifaceShare)
	return GL_TRUE;
    
    return (*iface->exports.shareContext) ((__GLcontext *) iface,
					   (__GLcontext *) ifaceShare);
}

static GLboolean
xglCopyContext (__GLcontext	  *dst,
		const __GLcontext *src,
		GLuint		  mask)
{
    xglGLContextPtr   pDst = (xglGLContextPtr) dst;
    xglGLContextPtr   pSrc = (xglGLContextPtr) src;
    const __GLcontext *srcCtx = (const __GLcontext *) pSrc->mIface;
    __GLinterface     *dstIface = (__GLinterface *) pDst->mIface;
    GLboolean	      status = GL_TRUE;
	
    if (pSrc->context && pDst->context)
	glitz_context_copy (pSrc->context, pDst->context, mask);
    else
	status = GL_FALSE;

    if (dstIface && srcCtx)
	status = (*dstIface->exports.copyContext) ((__GLcontext *) dstIface,
						   srcCtx,
						   mask);

    return status;
}

static GLboolean
xglForceCurrent (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;
    GLboolean	    status = GL_TRUE;
    
    if (pContext->context)
    {
	cctx = pContext;

	if (cctx->pReadBuffer->pDrawable && cctx->pDrawBuffer->pDrawable)
	{
	    DrawablePtr pDrawable;
	    PixmapPtr   pReadPixmap, pDrawPixmap;
	
	    pDrawable = cctx->pReadBuffer->pDrawable;
	    if (pDrawable->type != DRAWABLE_PIXMAP)
	    {
		pReadPixmap = XGL_GET_WINDOW_PIXMAP (pDrawable);
		cctx->pReadBuffer->xOff = pDrawable->x +
		    __XGL_OFF_X_WIN (pReadPixmap);
		cctx->pReadBuffer->yOff = pReadPixmap->drawable.height -
		    ((pDrawable->y + __XGL_OFF_Y_WIN (pReadPixmap)) +
		     pDrawable->height);
		cctx->pReadBuffer->yFlip = pReadPixmap->drawable.height;
	    }
	    else
	    {
		pReadPixmap = (PixmapPtr) pDrawable;
		cctx->pReadBuffer->xOff = cctx->pReadBuffer->yOff = 0;
		cctx->pReadBuffer->yFlip = pDrawable->height;
	    }
	    
	    pDrawable = cctx->pDrawBuffer->pDrawable;
	    if (pDrawable->type != DRAWABLE_PIXMAP)
	    {
		pDrawPixmap = XGL_GET_WINDOW_PIXMAP (pDrawable);
		cctx->pDrawBuffer->xOff = pDrawable->x +
		    __XGL_OFF_X_WIN (pDrawPixmap);
		cctx->pDrawBuffer->yOff = pDrawPixmap->drawable.height -
		    ((pDrawable->y + __XGL_OFF_Y_WIN (pDrawPixmap)) +
		     pDrawable->height);
		cctx->pDrawBuffer->yFlip = pDrawPixmap->drawable.height;
	    }
	    else
	    {
		pDrawPixmap = (PixmapPtr) pDrawable;
		cctx->pDrawBuffer->xOff = cctx->pDrawBuffer->yOff = 0;
		cctx->pDrawBuffer->yFlip = pDrawable->height;
	    }
	    
	    /* check if buffers have changed */
	    if (cctx->pReadBuffer->pPixmap != pReadPixmap ||
		cctx->pDrawBuffer->pPixmap != pDrawPixmap)
	    {
		XGL_SCREEN_PRIV (pDrawable->pScreen);
		XGL_PIXMAP_PRIV (pDrawPixmap);
		
		if (!xglPrepareTarget (pDrawable))
		    return FALSE;
		
		/* draw buffer is offscreen */
		if (pPixmapPriv->surface != pScreenPriv->surface)
		{
		    /* NYI: framebuffer object setup */
		    FatalError ("NYI: offscreen GL drawable\n");
		}

		cctx->pReadBuffer->pPixmap = pReadPixmap;
		cctx->pDrawBuffer->pPixmap = pDrawPixmap;
	    }
	}

	xglSetCurrentContext (pContext);
	
	if (cctx->needInit)
	{
	    int i;
	    
	    xglInitExtensions (cctx);

	    cctx->attrib.scissorTest = GL_FALSE;
	    cctx->attrib.scissor.x = cctx->attrib.scissor.y = 0;
	    cctx->attrib.scissor.width = cctx->pDrawBuffer->pDrawable->width;
	    cctx->attrib.scissor.height = cctx->pDrawBuffer->pDrawable->height;
	    cctx->attrib.viewport = cctx->attrib.scissor;
	
	    cctx->activeTexUnit = 0;

	    for (i = 0; i < cctx->maxTexUnits; i++)
	    {
		cctx->attrib.texUnits[i].enabled = 0;
		
		cctx->attrib.texUnits[i].p1D	  = NULL;
		cctx->attrib.texUnits[i].p2D	  = NULL;
		cctx->attrib.texUnits[i].p3D	  = NULL;
		cctx->attrib.texUnits[i].pRect	  = NULL;
		cctx->attrib.texUnits[i].pCubeMap = NULL;
	    }
	    
	    glEnable (GL_SCISSOR_TEST);

	    cctx->needInit = FALSE;
	}

	/* update viewport and raster position */
	if (cctx->pDrawBuffer->xOff != cctx->drawXoff ||
	    cctx->pDrawBuffer->yOff != cctx->drawYoff)
	{
	    glViewport (cctx->attrib.scissor.x + cctx->pDrawBuffer->xOff,
			cctx->attrib.scissor.y + cctx->pDrawBuffer->yOff,
			cctx->attrib.scissor.width,
			cctx->attrib.scissor.height);

	    glBitmap (0, 0, 0, 0,
		      cctx->pDrawBuffer->xOff - cctx->drawXoff,
		      cctx->pDrawBuffer->yOff - cctx->drawYoff,
		      NULL);

	    cctx->drawXoff = cctx->pDrawBuffer->xOff;
	    cctx->drawYoff = cctx->pDrawBuffer->yOff;
	}

	glDrawBuffer (cctx->attrib.drawBuffer);
	glReadBuffer (cctx->attrib.readBuffer);
    }
    else
    {
	cctx = NULL;
	__glRenderTable = &__glMesaRenderTable;
	__glRenderTableEXT = &__glMesaRenderTableEXT;
	
	status = (*iface->exports.forceCurrent) ((__GLcontext *) iface);
    }

    return status;
}

static GLboolean
xglNotifyResize (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    if (!iface)
	return GL_TRUE;
    
    return (*iface->exports.notifyResize) ((__GLcontext *) iface);
}

static void
xglNotifyDestroy (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    if (iface)
	(*iface->exports.notifyDestroy) ((__GLcontext *) iface);
}

static void
xglNotifySwapBuffers (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    if (iface)
	(*iface->exports.notifySwapBuffers) ((__GLcontext *) iface);
}

static struct __GLdispatchStateRec *
xglDispatchExec (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    if (!iface)
	return NULL;
    
    return (*iface->exports.dispatchExec) ((__GLcontext *) iface);
}

static void
xglBeginDispatchOverride (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    if (iface)
	(*iface->exports.beginDispatchOverride) ((__GLcontext *) iface);
}

static void
xglEndDispatchOverride (__GLcontext *gc)
{
    xglGLContextPtr pContext = (xglGLContextPtr) gc;
    __GLinterface   *iface = pContext->mIface;

    if (iface)
	(*iface->exports.endDispatchOverride) ((__GLcontext *) iface);
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
    pContext->versionString = NULL;
    pContext->errorValue    = GL_NO_ERROR;
    pContext->shared	    = NULL;
    pContext->list	    = 0;
    pContext->groupList	    = 0;
    pContext->pAttribStack  = NULL;
    pContext->nAttribStack  = 0;
    pContext->refcnt	    = 1;
    pContext->doubleBuffer  = glxCtx->pGlxVisual->doubleBuffer;
    pContext->depthBits     = glxCtx->pGlxVisual->depthSize;
    pContext->stencilBits   = glxCtx->pGlxVisual->stencilSize;
    pContext->drawXoff	    = 0;
    pContext->drawYoff	    = 0;
    pContext->maxTexUnits   = 0;

    if (pContext->doubleBuffer)
    {
	pContext->attrib.drawBuffer = GL_BACK;
	pContext->attrib.readBuffer = GL_BACK;
    }
    else
    {
	pContext->attrib.drawBuffer = GL_FRONT;
	pContext->attrib.readBuffer = GL_FRONT;
    }
    
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
    GLboolean		status = GL_TRUE;
    
    if (pDrawable)
    {
	if (glPriv->modes->doubleBufferMode)
	{
	    glitz_surface_t *surface;
	    int		    xOff, yOff;
	    GCPtr	    pGC  = pBufferPriv->pGC;
	    BoxPtr	    pBox = REGION_RECTS (pGC->pCompositeClip);
	    int		    nBox = REGION_NUM_RECTS (pGC->pCompositeClip);

	    XGL_SCREEN_PRIV (pGC->pScreen);

	    if (!xglPrepareTarget (pDrawable))
		return GL_FALSE;
	    
	    XGL_GET_DRAWABLE (pDrawable, surface, xOff, yOff);

	    /* native swap buffers for fullscreen windows */
	    if (surface == pScreenPriv->surface &&
		nBox == 1 &&
		pBox->x1 <= 0 &&
		pBox->y1 <= 0 &&
		pBox->x2 >= pGC->pScreen->width &&
		pBox->y2 >= pGC->pScreen->height)
	    {
		glitz_drawable_swap_buffers (pScreenPriv->drawable);
	    }
	    else
	    {
		glitz_surface_set_clip_region (surface, xOff, yOff,
					       (glitz_box_t *) pBox, nBox);

		glitz_copy_area (pBufferPriv->backSurface,
				 surface,
				 pDrawable->x + xOff,
				 pDrawable->y + yOff,
				 pDrawable->width,
				 pDrawable->height,
				 pDrawable->x + xOff,
				 pDrawable->y + yOff);
		
		glitz_surface_set_clip_region (surface, 0, 0, NULL, 0);
	    }

	    DamageDamageRegion (pDrawable, pGC->pCompositeClip);
	    REGION_EMPTY (pGC->pScreen, &pBufferPriv->damage);
	}
    }
    else if (pBufferPriv->private)
    {
	glPriv->private = pBufferPriv->private;
	status = (*pBufferPriv->swapBuffers) (glxPriv);
	glPriv->private = pBufferPriv;
    }

    return status;
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
    GLboolean	   status = GL_TRUE;
    
    if (pDrawable)
    {
	if (glPriv->modes->doubleBufferMode)
	{
	    glitz_surface_t *surface = pBufferPriv->backSurface;

	    XGL_SCREEN_PRIV (pDrawable->pScreen);
	
	    /* FIXME: copy color buffer bits, stencil bits and depth bits */
	
	    if (surface != pScreenPriv->backSurface &&
		(glitz_surface_get_width (surface)  != width ||
		 glitz_surface_get_height (surface) != height))
	    {
		glitz_format_t *format;

		format = pScreenPriv->pixmapFormats[pDrawable->depth].format;
		
		glitz_surface_destroy (surface);
		
		pBufferPriv->backSurface =
		    glitz_surface_create (pScreenPriv->drawable, format,
					  width, height, 0, NULL);
		if (!pBufferPriv->backSurface)
		    status = GL_FALSE;
	    }
	}
	
	ValidateGC (pDrawable, pBufferPriv->pGC);
    }
    else if (pBufferPriv->private)
    {
	glPriv->private = pBufferPriv->private;
	status = (*pBufferPriv->resizeBuffers) (buffer,
						x, y, width, height,
						glPriv,
						bufferMask);
	glPriv->private = pBufferPriv;
    }

    return status;
}

static void
xglFreeBuffers (__GLdrawablePrivate *glPriv)
{
    xglGLBufferPtr pBufferPriv = glPriv->private;
    
    glPriv->private = pBufferPriv->private;

    if (pBufferPriv->freeBuffers)
	(*pBufferPriv->freeBuffers) (glPriv);

    if (pBufferPriv->pGC)
	FreeGC (pBufferPriv->pGC, (GContext) 0);
    
    if (pBufferPriv->backSurface)
	glitz_surface_destroy (pBufferPriv->backSurface);

    xfree (pBufferPriv);
}

static void
xglCreateBuffer (__GLXdrawablePrivate *glxPriv)
{
    __GLdrawablePrivate	*glPriv = &glxPriv->glPriv;
    DrawablePtr		pDrawable = glxPriv->pDraw;
    ScreenPtr		pScreen = pDrawable->pScreen;
    xglGLBufferPtr	pBufferPriv;

    XGL_SCREEN_PRIV (pScreen);
    
    pBufferPriv = xalloc (sizeof (xglGLBufferRec));
    if (!pBufferPriv)
	FatalError ("xglCreateBuffer: No memory\n");

    pBufferPriv->pScreen       = pScreen;
    pBufferPriv->pDrawable     = NULL;
    pBufferPriv->pPixmap       = NULL;
    pBufferPriv->pGC	       = NULL;
    pBufferPriv->backSurface   = NULL;

    pBufferPriv->swapBuffers   = NULL;
    pBufferPriv->resizeBuffers = NULL;
    pBufferPriv->private       = NULL;
    pBufferPriv->freeBuffers   = NULL;

    REGION_INIT (pScreen, &pBufferPriv->damage, NullBox, 0);

    /* use native back buffer for regular windows */
    if (pDrawable->type == DRAWABLE_WINDOW

#ifdef COMPOSITE
	/* this is a root window, can't be redirected */
	&& (!((WindowPtr) pDrawable)->parent)
#endif

	)
    {
	pBufferPriv->pDrawable = pDrawable;
	
	if (glxPriv->pGlxVisual->doubleBuffer)
	{
	    pBufferPriv->backSurface = pScreenPriv->backSurface;
	    glitz_surface_reference (pScreenPriv->backSurface);
	}
    }
    else if (0) /*pScreenPriv->features &
		 GLITZ_FEATURE_FRAMEBUFFER_OBJECT_MASK) */
    {
	pBufferPriv->pDrawable = pDrawable;
	
	if (glxPriv->pGlxVisual->doubleBuffer)
	{
	    int depth = pDrawable->depth;
	    
	    pBufferPriv->backSurface =
	    	glitz_surface_create (pScreenPriv->drawable,
				      pScreenPriv->pixmapFormats[depth].format,
				      pDrawable->width, pDrawable->height,
				      0, NULL);
	    if (!pBufferPriv->backSurface)
		FatalError ("xglCreateBuffer: glitz_surface_create\n");
	}
    }
    else
    {
	(*screenInfoPriv.createBuffer) (glxPriv);

	/* wrap the swap buffers routine */
	pBufferPriv->swapBuffers = glxPriv->swapBuffers;
	
	/* wrap the front buffer's resize routine and freePrivate */
	pBufferPriv->resizeBuffers = glPriv->frontBuffer.resize;
	pBufferPriv->freeBuffers   = glPriv->freePrivate;
	pBufferPriv->private	   = glPriv->private;
    }

    glxPriv->swapBuffers = xglSwapBuffers;

    glPriv->frontBuffer.resize = xglResizeBuffers;
    glPriv->private	       = (void *) pBufferPriv;
    glPriv->freePrivate	       = xglFreeBuffers;
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

static int
xglXWaitX (__GLXclientState *cl, GLbyte *pc)
{
    xGLXWaitXReq *req = (xGLXWaitXReq *) pc;
    __GLXcontext *cx;
    
    cx = (__GLXcontext *) __glXLookupContextByTag (cl, req->contextTag);
    if (cx)
    {
	xglGLContextPtr pContext = (xglGLContextPtr) cx->gc;
	__GLXcontext    *glxCtx = (__GLXcontext *)
	    pContext->iface.imports.other;
	
	XGL_SCREEN_PRIV (glxCtx->pScreen);
	
	glitz_drawable_finish (pScreenPriv->drawable);

	return Success;
    }
    else
    { 
	cl->client->errorValue = req->contextTag;
	return __glXBadContextTag;
    }
}

static int
xglXSwapWaitX (__GLXclientState *cl, GLbyte *pc)
{
    xGLXWaitXReq *req = (xGLXWaitXReq *) pc;
    __GLX_DECLARE_SWAP_VARIABLES;

    __GLX_SWAP_SHORT (&req->length);
    __GLX_SWAP_INT (&req->contextTag);

    return xglXWaitX (cl, pc);
}

static Bool
xglDestroyWindow (WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    Bool      ret;
    
    XGL_SCREEN_PRIV (pScreen);

    if (cctx)
    {
	if (cctx->pDrawBuffer->pDrawable == &pWin->drawable)
	    cctx->pDrawBuffer->pDrawable = NULL;

	if (cctx->pReadBuffer->pDrawable == &pWin->drawable)
	    cctx->pReadBuffer->pDrawable = NULL;
    }
    
    XGL_SCREEN_UNWRAP (DestroyWindow);
    ret = (*pScreen->DestroyWindow) (pWin);
    XGL_SCREEN_WRAP (DestroyWindow, xglDestroyWindow);

    return ret;
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

    XGL_SCREEN_WRAP (DestroyWindow, xglDestroyWindow);
    
    depth  = pScreenPriv->pVisual->pPixel->depth;
    bpp    = pScreenPriv->pVisual->pPixel->masks.bpp;
    format = glitz_drawable_get_format (pScreenPriv->drawable);
    pPixel = pScreenPriv->pixmapFormats[depth].pPixel;

    if (format->doublebuffer)
    {
	pScreenPriv->backSurface =
	    glitz_surface_create (pScreenPriv->drawable,
				  pScreenPriv->pixmapFormats[depth].format,
				  pScreen->width, pScreen->height,
				  0, NULL);
	if (!pScreenPriv->backSurface)
	    return FALSE;

	glitz_surface_attach (pScreenPriv->backSurface,
			      pScreenPriv->drawable,
			      GLITZ_DRAWABLE_BUFFER_BACK_COLOR,
			      0, 0);

	numConfig *= 2;
    }

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

	if (i == 1)
	{
	    pConfig[i].doubleBuffer = FALSE;
	    pConfig[i].depthSize    = 0;
	    pConfig[i].stencilSize  = 0;
	
	}
	else
	{
	    pConfig[i].doubleBuffer = TRUE;
	    pConfig[i].depthSize    = format->depth_size;
	    pConfig[i].stencilSize  = format->stencil_size;
	}
	
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

	__glXSingleTable[9]     = xglXWaitX;
	__glXSwapSingleTable[9] = xglXSwapWaitX;
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
    
    return TRUE;
}

#endif /* GLXEXT */
