/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Author: Alan Hourihane <alanh@tungstengraphics.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* FIXME ! */
#define DRI_DRIVER_PATH "/ISO/X.Org/modular/i386/lib/dri"

#include "xf86.h"
#include "xf86_OSproc.h"
#include "driver.h"
#include <dlfcn.h>

#include "pipe/p_winsys.h"
#include "pipe/p_format.h"
#include "pipe/p_context.h"
#include "pipe/p_util.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"

/* EXA winsys */
struct exa_context
{
};

struct exa_winsys
{
    struct pipe_winsys base;
    modesettingPtr ms;
};

struct exa_buffer
{
    struct pipe_buffer base;
    drmBO bo;
    boolean userBuffer;	/** Is this a user-space buffer? */
    //void *data;
    //void *mapped;
};

struct exa_surface
{
    struct pipe_surface surface;
};

struct exa_entity
{
    ExaDriverPtr pExa;
    struct exa_context *c;
    struct pipe_winsys *ws;
    struct pipe_context *ctx;
    struct pipe_screen *scrn;
};

static INLINE struct exa_winsys *
exa_get_winsys(struct pipe_winsys *ws)
{
    return (struct exa_winsys *)ws;
}

static INLINE struct exa_surface *
exa_get_surface(struct pipe_surface *ps)
{
    return (struct exa_surface *)ps;
}

static INLINE struct exa_buffer *
exa_get_buffer(struct pipe_buffer *buf)
{
    return (struct exa_buffer *)buf;
}

static void *
exa_buffer_map(struct pipe_winsys *pws, struct pipe_buffer *buf,
	       unsigned flags)
{
    struct exa_buffer *exa_buf = exa_get_buffer(buf);
    struct exa_winsys *exa_winsys = exa_get_winsys(pws);
    void *virtual;

    drmBOMap(exa_winsys->ms->fd,
	     &exa_buf->bo, DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0, &virtual);

    return virtual;
}

static void
exa_buffer_unmap(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
    struct exa_buffer *exa_buf = exa_get_buffer(buf);
    struct exa_winsys *exa_winsys = exa_get_winsys(pws);

    drmBOUnmap(exa_winsys->ms->fd, &exa_buf->bo);
}

static void
exa_buffer_destroy(struct pipe_winsys *pws, struct pipe_buffer *buf)
{
    struct exa_winsys *exa_winsys = exa_get_winsys(pws);
    struct exa_buffer *exa_buf = exa_get_buffer(buf);

    drmBOUnreference(exa_winsys->ms->fd, &exa_buf->bo);

    free(exa_buf);
}

static void
exa_flush_frontbuffer(struct pipe_winsys *pws,
		      struct pipe_surface *surf, void *context_private)
{
    struct exa_buffer *exa_buf = exa_get_buffer(surf->buffer);

    ErrorF("WANT TO FLUSH\n");
}

static const char *
exa_get_name(struct pipe_winsys *pws)
{
    return "EXA";
}

static struct pipe_buffer *
exa_buffer_create(struct pipe_winsys *pws,
		  unsigned alignment, unsigned usage, unsigned size)
{
    struct exa_buffer *buffer = xcalloc(1, sizeof(struct exa_buffer));
    struct exa_winsys *exa_winsys = exa_get_winsys(pws);
    unsigned int flags = 0;

    buffer->base.refcount = 1;
    buffer->base.alignment = alignment;
    buffer->base.usage = usage;
    buffer->base.size = size;

    if (exa_winsys->ms->noEvict) {
	flags = DRM_BO_FLAG_NO_EVICT;
	ErrorF("DISPLAY TARGET\n");
    }

    ErrorF("SIZE %d %d\n", size, alignment);
    if (!buffer->bo.handle) {
	// buffer->data = align_malloc(size, alignment);
	drmBOCreate(exa_winsys->ms->fd, size, 0, NULL,
		    DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE |
		    DRM_BO_FLAG_SHAREABLE | DRM_BO_FLAG_MEM_TT |
		    DRM_BO_FLAG_MAPPABLE | flags,
		    0, &buffer->bo);
    }

    return &buffer->base;
}

static struct pipe_buffer *
exa_user_buffer_create(struct pipe_winsys *pws, void *ptr, unsigned bytes)
{
    struct exa_buffer *buffer = xcalloc(1, sizeof(struct exa_buffer));

    buffer->base.refcount = 1;
    buffer->base.size = bytes;
    buffer->userBuffer = TRUE;
    //buffer->data = ptr;
    ErrorF("USERBUFFER\n");

    return &buffer->base;
}

/**
 * Round n up to next multiple.
 */
static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
    return (n + multiple - 1) & ~(multiple - 1);
}

static int
exa_surface_alloc_storage(struct pipe_winsys *winsys,
			  struct pipe_surface *surf,
			  unsigned width, unsigned height,
			  enum pipe_format format,
			  unsigned flags, unsigned tex_usage)
{
    const unsigned alignment = 64;

    surf->width = width;
    surf->height = height;
    surf->format = format;
    pf_get_block(format, &surf->block);
    surf->stride = round_up(surf->nblocksx * surf->block.size, alignment);

    assert(!surf->buffer);
    surf->buffer = winsys->buffer_create(winsys, alignment,
					 PIPE_BUFFER_USAGE_PIXEL,
					 surf->stride * height);
    if (!surf->buffer)
	return -1;

    return 0;
}

/**
 * Called via winsys->surface_alloc() to create new surfaces.
 */
static struct pipe_surface *
exa_surface_alloc(struct pipe_winsys *ws)
{
    struct exa_surface *wms = xcalloc(1, sizeof(struct exa_surface));

    assert(ws);

    wms->surface.refcount = 1;
    wms->surface.winsys = ws;

    return &wms->surface;
}

static void
exa_surface_release(struct pipe_winsys *winsys, struct pipe_surface **s)
{
    struct pipe_surface *surf = *s;

    surf->refcount--;
    if (surf->refcount == 0) {
	if (surf->buffer)
	    pipe_buffer_reference(winsys, &surf->buffer, NULL);
	free(surf);
    }
    *s = NULL;
}

/*
 * Fence functions - basically nothing to do, as we don't create any actual
 * fence objects.
 */
static void
exa_fence_reference(struct pipe_winsys *sws, struct pipe_fence_handle **ptr,
		    struct pipe_fence_handle *fence)
{
}

static int
exa_fence_signalled(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
		    unsigned flag)
{
    return 0;
}

static int
exa_fence_finish(struct pipe_winsys *sws, struct pipe_fence_handle *fence,
		 unsigned flag)
{
    return 0;
}

struct pipe_winsys *
exa_get_pipe_winsys(modesettingPtr ms)
{
    static struct exa_winsys *ws = NULL;

    if (!ws) {
	ws = xcalloc(1, sizeof(struct exa_winsys));

	/* Fill in this struct with callbacks that pipe will need to
	 * communicate with the window system, buffer manager, etc. 
	 */
	ws->base.buffer_create = exa_buffer_create;
	ws->base.user_buffer_create = exa_user_buffer_create;
	ws->base.buffer_map = exa_buffer_map;
	ws->base.buffer_unmap = exa_buffer_unmap;
	ws->base.buffer_destroy = exa_buffer_destroy;

	ws->base.surface_alloc = exa_surface_alloc;
	ws->base.surface_alloc_storage = exa_surface_alloc_storage;
	ws->base.surface_release = exa_surface_release;

	ws->base.fence_reference = exa_fence_reference;
	ws->base.fence_signalled = exa_fence_signalled;
	ws->base.fence_finish = exa_fence_finish;

	ws->base.flush_frontbuffer = exa_flush_frontbuffer;
	ws->base.get_name = exa_get_name;

	ws->ms = ms;
    }

    return &ws->base;
}

/* EXA functions */

struct PixmapPriv
{
    drmBO bo;
#if 0
    dri_fence *fence;
#endif
    int flags;

    struct pipe_texture *tex;
    unsigned int color;
    struct pipe_surface *src_surf;     /* for copies */
};

static enum pipe_format
exa_get_pipe_format(int depth)
{
    switch (depth) {
    case 32:
    case 24:
	return PIPE_FORMAT_A8R8G8B8_UNORM;
    case 16:
	return PIPE_FORMAT_R5G6B5_UNORM;
    case 15:
	return PIPE_FORMAT_A1R5G5B5_UNORM;
    case 8:
    case 4:
    case 1:
	return PIPE_FORMAT_A8R8G8B8_UNORM;	/* bad bad bad */
    default:
	assert(0);
	return 0;
    }
}

/*
 * EXA functions
 */

static void
ExaWaitMarker(ScreenPtr pScreen, int marker)
{
}

static int
ExaMarkSync(ScreenPtr pScreen)
{
    return 1;
}

Bool
ExaPrepareAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv;
    int ret;

    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return FALSE;

    if (!priv->tex)
	return FALSE;
    {
	struct pipe_surface *surf =
	    exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				       PIPE_BUFFER_USAGE_CPU_READ |
				       PIPE_BUFFER_USAGE_CPU_WRITE);
	pPix->devPrivate.ptr =
	    exa->scrn->surface_map(exa->scrn, surf,
				   PIPE_BUFFER_USAGE_CPU_READ |
				   PIPE_BUFFER_USAGE_CPU_WRITE);
	exa->scrn->tex_surface_release(exa->scrn, &surf);
    }

    return TRUE;
}

void
ExaFinishAccess(PixmapPtr pPix, int index)
{
    ScreenPtr pScreen = pPix->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct PixmapPriv *priv;
    struct exa_entity *exa = ms->exa;
    int ret;

    priv = exaGetPixmapDriverPrivate(pPix);

    if (!priv)
	return;

    if (!priv->tex)
	return;
    {
	struct pipe_surface *surf =
	    exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				       PIPE_BUFFER_USAGE_CPU_READ |
				       PIPE_BUFFER_USAGE_CPU_WRITE);
	exa->scrn->surface_unmap(exa->scrn, surf);
	exa->scrn->tex_surface_release(exa->scrn, &surf);
	pPix->devPrivate.ptr = NULL;
    }
}

static void
ExaDone(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_entity *exa = ms->exa;

    if (!priv)
	return;

    if (priv->src_surf)
	exa->scrn->tex_surface_release(exa->scrn, &priv->src_surf);
    priv->src_surf = NULL;
}

static void
ExaDoneComposite(PixmapPtr pPixmap)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
}

static Bool
ExaPrepareSolid(PixmapPtr pPixmap, int alu, Pixel planeMask, Pixel fg)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct exa_entity *exa = ms->exa;

    if (pPixmap->drawable.depth < 15)
	return FALSE;

    if (!EXA_PM_IS_SOLID(&pPixmap->drawable, planeMask))
	return FALSE;

    if (!priv || !priv->tex)
	return FALSE;

    if (alu != GXcopy)
	return FALSE;

    if (!exa->ctx || !exa->ctx->surface_fill)
	return FALSE;

    priv->color = fg;

    return TRUE;
}

static void
ExaSolid(PixmapPtr pPixmap, int x0, int y0, int x1, int y1)
{
    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    struct pipe_surface *surf =
	exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				   PIPE_BUFFER_USAGE_GPU_READ |
				   PIPE_BUFFER_USAGE_GPU_WRITE);

    exa->ctx->surface_fill(exa->ctx, surf, x0, y0, x1 - x0, y1 - y0,
			   priv->color);

    exa->scrn->tex_surface_release(exa->scrn, &surf);
}

static Bool
ExaPrepareCopy(PixmapPtr pSrcPixmap, PixmapPtr pDstPixmap, int xdir,
	       int ydir, int alu, Pixel planeMask)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct pipe_surface *src_surf;
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pDstPixmap);
    struct PixmapPriv *src_priv = exaGetPixmapDriverPrivate(pSrcPixmap);

    if (alu != GXcopy)
	return FALSE;

    if (pSrcPixmap->drawable.depth < 15 || pDstPixmap->drawable.depth < 15)
	return FALSE;

    if (!EXA_PM_IS_SOLID(&pSrcPixmap->drawable, planeMask))
	return FALSE;

    if (!priv || !src_priv)
	return FALSE;

    if (!priv->tex || !src_priv->tex)
	return FALSE;

    if (!exa->ctx || !exa->ctx->surface_copy)
	return FALSE;

    priv->src_surf =
	exa->scrn->get_tex_surface(exa->scrn, src_priv->tex, 0, 0, 0,
				   PIPE_BUFFER_USAGE_GPU_READ |
				   PIPE_BUFFER_USAGE_GPU_WRITE);

    return FALSE;
}

static void
ExaCopy(PixmapPtr pDstPixmap, int srcX, int srcY, int dstX, int dstY,
	int width, int height)
{
    ScrnInfoPtr pScrn = xf86Screens[pDstPixmap->drawable.pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pDstPixmap);
    struct pipe_surface *surf =
	exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				   PIPE_BUFFER_USAGE_GPU_READ |
				   PIPE_BUFFER_USAGE_GPU_WRITE);

    exa->ctx->surface_copy(exa->ctx, 0, surf, dstX, dstY, priv->src_surf,
			   srcX, srcY, width, height);
    exa->scrn->tex_surface_release(exa->scrn, &surf);
}

static Bool
ExaPrepareComposite(int op, PicturePtr pSrcPicture,
		    PicturePtr pMaskPicture, PicturePtr pDstPicture,
		    PixmapPtr pSrc, PixmapPtr pMask, PixmapPtr pDst)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    return FALSE;
}

static Bool
ExaUploadToScreen(PixmapPtr pDst, int x, int y, int w, int h, char *src,
		  int src_pitch)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    ErrorF("UPLOAD\n");

    return FALSE;
}

static void
ExaComposite(PixmapPtr pDst, int srcX, int srcY, int maskX, int maskY,
	     int dstX, int dstY, int width, int height)
{
    ScreenPtr pScreen = pDst->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
}

static Bool
ExaCheckComposite(int op,
		  PicturePtr pSrcPicture, PicturePtr pMaskPicture,
		  PicturePtr pDstPicture)
{
    DrawablePtr pDraw = pSrcPicture->pDrawable;
    int w = pDraw->width;
    int h = pDraw->height;

    return FALSE;
}

static void *
ExaCreatePixmap(ScreenPtr pScreen, int size, int align)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct PixmapPriv *priv;
    void *virtual;

    priv = xcalloc(1, sizeof(struct PixmapPriv));
    if (!priv)
	return NULL;

    return priv;
}

static void
ExaDestroyPixmap(ScreenPtr pScreen, void *dPriv)
{
    struct PixmapPriv *priv = (struct PixmapPriv *)dPriv;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;

    if (!priv)
	return;

    if (priv->tex)
	exa->scrn->texture_release(exa->scrn, &priv->tex);

    xfree(priv);
}

static Bool
ExaPixmapIsOffscreen(PixmapPtr pPixmap)
{
    struct PixmapPriv *priv;
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv)
	return FALSE;

    if (priv->tex)
	return TRUE;

    return FALSE;
}

/* FIXME !! */
unsigned int
driGetPixmapHandle(PixmapPtr pPixmap, unsigned int *flags)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;
    struct exa_buffer *exa_buf;
    struct pipe_surface *surf;
    struct PixmapPriv *priv;

    *flags = 0;

    if (!ms->exa) {
	FatalError("NO MS->EXA\n");
	return 0;
    }

    priv = exaGetPixmapDriverPrivate(pPixmap);

    if (!priv) {
	FatalError("NO PIXMAP PRIVATE\n");
	return 0;
    }

    surf =
	exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				   PIPE_BUFFER_USAGE_CPU_READ |
				   PIPE_BUFFER_USAGE_CPU_WRITE);
    exa_buf = exa_get_buffer(surf->buffer);
    exa->scrn->tex_surface_release(exa->scrn, &surf);

    if (exa_buf->bo.handle)
	return exa_buf->bo.handle;

    return 0;
}

static Bool
ExaModifyPixmapHeader(PixmapPtr pPixmap, int width, int height,
		      int depth, int bitsPerPixel, int devKind,
		      pointer pPixData)
{
    ScreenPtr pScreen = pPixmap->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    struct PixmapPriv *priv = exaGetPixmapDriverPrivate(pPixmap);
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap = pScreen->GetScreenPixmap(pScreen);
    struct exa_entity *exa = ms->exa;

    if (!priv)
	return FALSE;

    if (depth <= 0)
	depth = pPixmap->drawable.depth;

    if (bitsPerPixel <= 0)
	bitsPerPixel = pPixmap->drawable.bitsPerPixel;

    if (width <= 0)
	width = pPixmap->drawable.width;

    if (height <= 0)
	height = pPixmap->drawable.height;

    if (width <= 0 || height <= 0 || depth <= 0)
	return FALSE;

    miModifyPixmapHeader(pPixmap, width, height, depth,
			     bitsPerPixel, devKind, NULL);

    /* Deal with screen resize */
    if (priv->tex) {
	struct pipe_surface *surf =
	    exa->scrn->get_tex_surface(exa->scrn, priv->tex, 0, 0, 0,
				       PIPE_BUFFER_USAGE_CPU_READ |
				       PIPE_BUFFER_USAGE_CPU_WRITE);

	ErrorF("RESIZE %d %d to %d %d\n", surf->width, surf->height, width,
	       height);
	if (surf->width != width || surf->height != height) {
	    exa->scrn->texture_release(exa->scrn, &priv->tex);
	    priv->tex = NULL;
	}
	exa->scrn->tex_surface_release(exa->scrn, &surf);
    }

    if (!priv->tex) {
	struct pipe_texture template;

	memset(&template, 0, sizeof(template));
	template.target = PIPE_TEXTURE_2D;
	template.compressed = 0;
	template.format = exa_get_pipe_format(depth);
	pf_get_block(template.format, &template.block);
	template.width[0] = width;
	template.height[0] = height;
	template.depth[0] = 1;
	template.last_level = 0;
	template.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
	priv->tex = exa->scrn->texture_create(exa->scrn, &template);
    }

    return TRUE;
}

void
ExaClose(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa = ms->exa;

    exaDriverFini(pScrn->pScreen);

    dlclose(ms->driver);
}

void *
ExaInit(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    struct exa_entity *exa;
    ExaDriverPtr pExa;

    exa = xcalloc(1, sizeof(struct exa_entity));
    if (!exa)
	return NULL;

    pExa = exaDriverAlloc();
    if (!pExa) {
	goto out_err;
    }

    memset(pExa, 0, sizeof(*pExa));
    pExa->exa_major = 2;
    pExa->exa_minor = 4;
    pExa->memoryBase = 0;
    pExa->memorySize = 0;
    pExa->offScreenBase = 0;
    pExa->pixmapOffsetAlign = 0;
    pExa->pixmapPitchAlign = 1;
    pExa->flags = EXA_OFFSCREEN_PIXMAPS | EXA_HANDLES_PIXMAPS;
    pExa->maxX = 8191;		       /* FIXME */
    pExa->maxY = 8191;		       /* FIXME */
    pExa->WaitMarker = ExaWaitMarker;
    pExa->MarkSync = ExaMarkSync;
    pExa->PrepareSolid = ExaPrepareSolid;
    pExa->Solid = ExaSolid;
    pExa->DoneSolid = ExaDone;
    pExa->PrepareCopy = ExaPrepareCopy;
    pExa->Copy = ExaCopy;
    pExa->DoneCopy = ExaDone;
    pExa->CheckComposite = ExaCheckComposite;
    pExa->PrepareComposite = ExaPrepareComposite;
    pExa->Composite = ExaComposite;
    pExa->DoneComposite = ExaDoneComposite;
    pExa->PixmapIsOffscreen = ExaPixmapIsOffscreen;
    pExa->PrepareAccess = ExaPrepareAccess;
    pExa->FinishAccess = ExaFinishAccess;
    pExa->UploadToScreen = ExaUploadToScreen;
    pExa->CreatePixmap = ExaCreatePixmap;
    pExa->DestroyPixmap = ExaDestroyPixmap;
    pExa->ModifyPixmapHeader = ExaModifyPixmapHeader;

    if (!exaDriverInit(pScrn->pScreen, pExa)) {
	goto out_err;
    }

    {
	char filename[128];
	char dri_driver_path[] = DRI_DRIVER_PATH;

	snprintf(filename, sizeof filename,
		 "%s/%s_dri.so", dri_driver_path, "i915");

	ms->driver = dlopen(filename, RTLD_NOW | RTLD_DEEPBIND | RTLD_GLOBAL);
	if (!ms->driver)
		FatalError("failed to initialize i915 - for softpipe only.\n");

	exa->c = xcalloc(1, sizeof(struct exa_context));

	exa->ws = exa_get_pipe_winsys(ms);
	if (!exa->ws)
		FatalError("BAD WINSYS\n");

	exa->scrn = softpipe_create_screen(exa->ws);
	if (!exa->scrn)
		FatalError("BAD SCREEN\n");

	exa->ctx = softpipe_create(exa->scrn, exa->ws, NULL);
	if (!exa->ctx)
	   	FatalError("BAD CTX\n");

	exa->ctx->priv = exa->c;
    }

    return (void *)exa;

  out_err:
    ExaClose(pScrn);

    return NULL;
}
