/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */
#ifndef GLAMOR_PRIV_H
#define GLAMOR_PRIV_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include <xorg-config.h>
#endif
#include <xorg-server.h>

#include "glamor.h"

#define GL_GLEXT_PROTOTYPES

#ifdef GLAMOR_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define GLAMOR_DEFAULT_PRECISION   "precision mediump float;\n"
#include "glamor_glext.h"
#else
#include <GL/gl.h>
#include <GL/glext.h>
#define GLAMOR_DEFAULT_PRECISION
#endif

#ifdef RENDER
#include "glyphstr.h"
#endif

#include "glamor_debug.h"

#include <list.h>

typedef struct glamor_composite_shader {
	GLuint prog;
	GLint dest_to_dest_uniform_location;
	GLint dest_to_source_uniform_location;
	GLint dest_to_mask_uniform_location;
	GLint source_uniform_location;
	GLint mask_uniform_location;
} glamor_composite_shader;

typedef struct {
	INT16 x_src;
	INT16 y_src;
	INT16 x_mask;
	INT16 y_mask;
	INT16 x_dst;
	INT16 y_dst;
	INT16 width;
	INT16 height;
} glamor_composite_rect_t;


enum glamor_vertex_type {
	GLAMOR_VERTEX_POS,
	GLAMOR_VERTEX_SOURCE,
	GLAMOR_VERTEX_MASK
};

enum shader_source {
	SHADER_SOURCE_SOLID,
	SHADER_SOURCE_TEXTURE,
	SHADER_SOURCE_TEXTURE_ALPHA,
	SHADER_SOURCE_COUNT,
};

enum shader_mask {
	SHADER_MASK_NONE,
	SHADER_MASK_SOLID,
	SHADER_MASK_TEXTURE,
	SHADER_MASK_TEXTURE_ALPHA,
	SHADER_MASK_COUNT,
};

enum shader_in {
	SHADER_IN_SOURCE_ONLY,
	SHADER_IN_NORMAL,
	SHADER_IN_CA_SOURCE,
	SHADER_IN_CA_ALPHA,
	SHADER_IN_COUNT,
};

struct glamor_screen_private;
struct glamor_pixmap_private;
typedef void (*glamor_pixmap_validate_function_t) (struct
						   glamor_screen_private *,
						   struct
						   glamor_pixmap_private
						   *);

enum glamor_gl_flavor {
	GLAMOR_GL_DESKTOP,	// OPENGL API
	GLAMOR_GL_ES2		// OPENGL ES2.0 API
};

#define GLAMOR_CREATE_PIXMAP_CPU  0x100
#define GLAMOR_CREATE_PIXMAP_FIXUP 0x101

#define GLAMOR_NUM_GLYPH_CACHE_FORMATS 2

typedef struct {
	PicturePtr picture;	/* Where the glyphs of the cache are stored */
	GlyphPtr *glyphs;
	uint16_t count;
	uint16_t evict;
} glamor_glyph_cache_t;


#include "glamor_gl_dispatch.h"

struct glamor_saved_procs { 
	CloseScreenProcPtr close_screen;
	CreateGCProcPtr create_gc;
	CreatePixmapProcPtr create_pixmap;
	DestroyPixmapProcPtr destroy_pixmap;
	GetSpansProcPtr get_spans;
	GetImageProcPtr get_image;
	CompositeProcPtr composite;
	TrapezoidsProcPtr trapezoids;
	GlyphsProcPtr glyphs;
	ChangeWindowAttributesProcPtr change_window_attributes;
	CopyWindowProcPtr copy_window;
	BitmapToRegionProcPtr bitmap_to_region;
	TrianglesProcPtr triangles;
	AddTrapsProcPtr addtraps;
	CreatePictureProcPtr create_picture;
	DestroyPictureProcPtr destroy_picture;
	UnrealizeGlyphProcPtr unrealize_glyph;
};

#define CACHE_FORMAT_COUNT 1
#define CACHE_BUCKET_WCOUNT 4
#define CACHE_BUCKET_HCOUNT 4

#define GLAMOR_TICK_AFTER(t0, t1) 	\
	(((int)(t1) - (int)(t0)) < 0)

typedef struct glamor_screen_private {
	struct glamor_gl_dispatch dispatch;
	int yInverted;
	unsigned int tick;
	enum glamor_gl_flavor gl_flavor;
	int has_pack_invert;
	int has_fbo_blit;
	int max_fbo_size;

	struct list fbo_cache[CACHE_FORMAT_COUNT][CACHE_BUCKET_WCOUNT][CACHE_BUCKET_HCOUNT];

	/* glamor_solid */
	GLint solid_prog;
	GLint solid_color_uniform_location;

	/* vertext/elment_index buffer object for render */
	GLuint vbo, ebo;
	int vbo_offset;
	int vbo_size;
	char *vb;
	int vb_stride;
	Bool has_source_coords, has_mask_coords;
	int render_nr_verts;
	glamor_composite_shader composite_shader[SHADER_SOURCE_COUNT]
						[SHADER_MASK_COUNT]
						[SHADER_IN_COUNT];
	glamor_glyph_cache_t glyphCaches[GLAMOR_NUM_GLYPH_CACHE_FORMATS];
	Bool glyph_cache_initialized;

	/* shaders to restore a texture to another texture.*/
	GLint finish_access_prog[2];
	GLint finish_access_no_revert[2];
	GLint finish_access_swap_rb[2];

	/* glamor_tile */
	GLint tile_prog;

	/* glamor_putimage */
	GLint put_image_xybitmap_prog;
	GLint put_image_xybitmap_fg_uniform_location;
	GLint put_image_xybitmap_bg_uniform_location;

	int screen_fbo;
	struct glamor_saved_procs saved_procs;
	char delayed_fallback_string[GLAMOR_DELAYED_STRING_MAX + 1];
	int delayed_fallback_pending;
	glamor_pixmap_validate_function_t *pixmap_validate_funcs;
	int flags;
} glamor_screen_private;

typedef enum glamor_access {
	GLAMOR_ACCESS_RO,
	GLAMOR_ACCESS_RW,
	GLAMOR_ACCESS_WO,
} glamor_access_t;

enum _glamor_pending_op_type {
	GLAMOR_PENDING_NONE,
	GLAMOR_PENDING_FILL
};

typedef struct _glamor_pending_fill {
	unsigned int type;
	GLfloat color4fv[4];
	CARD32 colori;
} glamor_pending_fill;

typedef union _glamor_pending_op {
	unsigned int type;
	glamor_pending_fill fill;
} glamor_pending_op;

/*
 * glamor_pixmap_private - glamor pixmap's private structure.
 * @gl_fbo:  The pixmap is attached to a fbo originally.
 * @gl_tex:  The pixmap is in a gl texture originally.
 * @pbo_valid: The pbo has a valid copy of the pixmap's data.
 * @is_picture: The drawable is attached to a picture.
 * @tex:     attached texture.
 * @fb:      attached fbo.
 * @pbo:     attached pbo.
 * @pict_format: the corresponding picture's format.
 * #pending_op: currently only support pending filling.
 * @container: The corresponding pixmap's pointer.
 **/

typedef struct glamor_pixmap_fbo {
	struct list list;
	unsigned int expire;
	unsigned char pbo_valid;
	GLuint tex;
	GLuint fb;
	GLuint pbo;
	int width;
	int height;
	GLenum format;
	GLenum type;
	glamor_screen_private *glamor_priv;
} glamor_pixmap_fbo;


typedef struct glamor_pixmap_private {
	unsigned char gl_fbo:1;
	unsigned char is_picture:1;
	unsigned char gl_tex:1;
	glamor_pixmap_type_t type;
	glamor_pixmap_fbo *fbo;
	PictFormatShort pict_format;
	glamor_pending_op pending_op;
	PixmapPtr container;
	glamor_screen_private *glamor_priv;
} glamor_pixmap_private;

/* 
 * Pixmap dynamic status, used by dynamic upload feature.
 *
 * GLAMOR_NONE:  initial status, don't need to do anything.
 * GLAMOR_UPLOAD_PENDING: marked as need to be uploaded to gl texture.
 * GLAMOR_UPLOAD_DONE: the pixmap has been uploaded successfully.
 * GLAMOR_UPLOAD_FAILED: fail to upload the pixmap.
 *
 * */
typedef enum glamor_pixmap_status {
	GLAMOR_NONE,
	GLAMOR_UPLOAD_PENDING,
	GLAMOR_UPLOAD_DONE,
	GLAMOR_UPLOAD_FAILED
} glamor_pixmap_status_t;


extern DevPrivateKey glamor_screen_private_key;
extern DevPrivateKey glamor_pixmap_private_key;
static inline glamor_screen_private *
glamor_get_screen_private(ScreenPtr screen)
{
	return (glamor_screen_private *)
	    dixLookupPrivate(&screen->devPrivates,
			     glamor_screen_private_key);
}

static inline glamor_pixmap_private *
glamor_get_pixmap_private(PixmapPtr pixmap)
{
	return dixLookupPrivate(&pixmap->devPrivates,
				glamor_pixmap_private_key);
}


/**
 * Returns TRUE if the given planemask covers all the significant bits in the
 * pixel values for pDrawable.
 */
static inline Bool
glamor_pm_is_solid(DrawablePtr drawable, unsigned long planemask)
{
	return (planemask & FbFullMask(drawable->depth)) ==
	    FbFullMask(drawable->depth);
}

extern int glamor_debug_level;

/* glamor.c */
PixmapPtr glamor_get_drawable_pixmap(DrawablePtr drawable);

PixmapPtr glamor_create_pixmap(ScreenPtr screen, int w, int h, int depth,
			       unsigned int usage);

Bool glamor_destroy_pixmap(PixmapPtr pixmap);

glamor_pixmap_fbo* glamor_pixmap_detach_fbo(glamor_pixmap_private *pixmap_priv);
void glamor_pixmap_attach_fbo(PixmapPtr pixmap, glamor_pixmap_fbo *fbo);
glamor_pixmap_fbo * glamor_create_fbo_from_tex(glamor_screen_private *glamor_priv,
					       int w, int h, int depth, GLint tex, int flag);
glamor_pixmap_fbo * glamor_create_fbo(glamor_screen_private *glamor_priv,
				      int w, int h, int depth, int flag);
void glamor_destroy_fbo(glamor_pixmap_fbo *fbo);

void glamor_init_pixmap_fbo(ScreenPtr screen);
void glamor_fini_pixmap_fbo(ScreenPtr screen);
Bool glamor_pixmap_fbo_fixup(ScreenPtr screen, PixmapPtr pixmap);
void glamor_fbo_expire(glamor_screen_private *glamor_priv);
void glamor_init_pixmap_fbo(ScreenPtr screen);
void glamor_fini_pixmap_fbo(ScreenPtr screen);

Bool glamor_fixup_pixmap_priv(ScreenPtr screen, glamor_pixmap_private *pixmap_priv);

/* glamor_copyarea.c */
RegionPtr
glamor_copy_area(DrawablePtr src, DrawablePtr dst, GCPtr gc,
		 int srcx, int srcy, int width, int height, int dstx,
		 int dsty);
void glamor_copy_n_to_n(DrawablePtr src, DrawablePtr dst, GCPtr gc,
			BoxPtr box, int nbox, int dx, int dy, Bool reverse,
			Bool upsidedown, Pixel bitplane, void *closure);

/* glamor_copywindow.c */
void glamor_copy_window(WindowPtr win, DDXPointRec old_origin,
			RegionPtr src_region);

/* glamor_core.c */
Bool glamor_prepare_access(DrawablePtr drawable, glamor_access_t access);
void glamor_finish_access(DrawablePtr drawable, glamor_access_t access);
Bool glamor_prepare_access_window(WindowPtr window);
void glamor_finish_access_window(WindowPtr window);
Bool glamor_prepare_access_gc(GCPtr gc);
void glamor_finish_access_gc(GCPtr gc);
void glamor_init_finish_access_shaders(ScreenPtr screen);
void glamor_fini_finish_access_shaders(ScreenPtr screen);
const Bool glamor_get_drawable_location(const DrawablePtr drawable);
void glamor_get_drawable_deltas(DrawablePtr drawable, PixmapPtr pixmap,
				int *x, int *y);
Bool glamor_create_gc(GCPtr gc);
Bool glamor_stipple(PixmapPtr pixmap, PixmapPtr stipple,
		    int x, int y, int width, int height,
		    unsigned char alu, unsigned long planemask,
		    unsigned long fg_pixel, unsigned long bg_pixel,
		    int stipple_x, int stipple_y);
GLint glamor_compile_glsl_prog(glamor_gl_dispatch * dispatch, GLenum type,
			       const char *source);
void glamor_link_glsl_prog(glamor_gl_dispatch * dispatch, GLint prog);
void glamor_get_color_4f_from_pixel(PixmapPtr pixmap,
				    unsigned long fg_pixel,
				    GLfloat * color);

int glamor_set_destination_pixmap(PixmapPtr pixmap);
int glamor_set_destination_pixmap_priv(glamor_pixmap_private *
				       pixmap_priv);

/* nc means no check. caller must ensure this pixmap has valid fbo.
 * usually use the GLAMOR_PIXMAP_PRIV_HAS_FBO firstly. 
 * */
void glamor_set_destination_pixmap_priv_nc(glamor_pixmap_private *
					   pixmap_priv);


PixmapPtr
glamor_es2_pixmap_read_prepare(PixmapPtr source, GLenum * format,
			       GLenum * type, int no_alpha, int no_revert);

void glamor_set_alu(struct glamor_gl_dispatch *dispatch,
		    unsigned char alu);
Bool glamor_set_planemask(PixmapPtr pixmap, unsigned long planemask);
Bool glamor_change_window_attributes(WindowPtr pWin, unsigned long mask);
RegionPtr glamor_bitmap_to_region(PixmapPtr pixmap);
Bool glamor_gl_has_extension(char *extension);
int glamor_gl_get_version(void);

#define GLAMOR_GL_VERSION_ENCODE(major, minor) ( \
          ((major) * 256)                       \
        + ((minor) *   1))




/* glamor_fill.c */
Bool glamor_fill(DrawablePtr drawable,
		 GCPtr gc, int x, int y, int width, int height, Bool fallback);
Bool glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
		  unsigned char alu, unsigned long planemask,
		  unsigned long fg_pixel);
void glamor_solid_fail_region(PixmapPtr pixmap,
			      int x, int y, int width, int height);

/* glamor_fillspans.c */
void glamor_fill_spans(DrawablePtr drawable,
		       GCPtr gc,
		       int n, DDXPointPtr points, int *widths, int sorted);

void glamor_init_solid_shader(ScreenPtr screen);
void glamor_fini_solid_shader(ScreenPtr screen);

/* glamor_getspans.c */
void

glamor_get_spans(DrawablePtr drawable,
		 int wmax,
		 DDXPointPtr points,
		 int *widths, int nspans, char *dst_start);

/* glamor_glyphs.c */
void glamor_glyphs_fini(ScreenPtr screen);
void glamor_glyphs(CARD8 op,
		   PicturePtr pSrc,
		   PicturePtr pDst,
		   PictFormatPtr maskFormat,
		   INT16 xSrc,
		   INT16 ySrc, int nlist, GlyphListPtr list,
		   GlyphPtr * glyphs);

void glamor_glyph_unrealize(ScreenPtr screen, GlyphPtr glyph);
/* glamor_setspans.c */
void glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		      DDXPointPtr points, int *widths, int n, int sorted);

/* glamor_polyfillrect.c */
void
glamor_poly_fill_rect(DrawablePtr drawable,
		      GCPtr gc, int nrect, xRectangle * prect);

/* glamor_polylines.c */
void

glamor_poly_lines(DrawablePtr drawable, GCPtr gc, int mode, int n,
		  DDXPointPtr points);

/* glamor_putimage.c */
void

glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
		 int w, int h, int leftPad, int format, char *bits);
void glamor_init_putimage_shaders(ScreenPtr screen);
void glamor_fini_putimage_shaders(ScreenPtr screen);

/* glamor_render.c */
void glamor_composite(CARD8 op,
		      PicturePtr pSrc,
		      PicturePtr pMask,
		      PicturePtr pDst,
		      INT16 xSrc,
		      INT16 ySrc,
		      INT16 xMask,
		      INT16 yMask,
		      INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);
void glamor_trapezoids(CARD8 op,
		       PicturePtr src, PicturePtr dst,
		       PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
		       int ntrap, xTrapezoid * traps);
void glamor_init_composite_shaders(ScreenPtr screen);
void glamor_fini_composite_shaders(ScreenPtr screen);
void glamor_composite_glyph_rects(CARD8 op,
				  PicturePtr src, PicturePtr mask,
				  PicturePtr dst, int nrect,
				  glamor_composite_rect_t * rects);
void glamor_composite_rects (CARD8         op,
			     PicturePtr    pDst,
			     xRenderColor  *color,
			     int           nRect,
			     xRectangle    *rects);


/* glamor_tile.c */
Bool glamor_tile(PixmapPtr pixmap, PixmapPtr tile,
		 int x, int y, int width, int height,
		 unsigned char alu, unsigned long planemask,
		 int tile_x, int tile_y);
void glamor_init_tile_shader(ScreenPtr screen);
void glamor_fini_tile_shader(ScreenPtr screen);

/* glamor_triangles.c */
void

glamor_triangles(CARD8 op,
		 PicturePtr pSrc,
		 PicturePtr pDst,
		 PictFormatPtr maskFormat,
		 INT16 xSrc, INT16 ySrc, int ntris, xTriangle * tris);

/* glamor_pixmap.c */

void glamor_pixmap_init(ScreenPtr screen);
void glamor_pixmap_fini(ScreenPtr screen);
/** 
 * Download a pixmap's texture to cpu memory. If success,
 * One copy of current pixmap's texture will be put into
 * the pixmap->devPrivate.ptr. Will use pbo to map to 
 * the pointer if possible.
 * The pixmap must be a gl texture pixmap. gl_fbo and
 * gl_tex must be 1. Used by glamor_prepare_access.
 *
 */
Bool glamor_download_pixmap_to_cpu(PixmapPtr pixmap,
				   glamor_access_t access);

/**
 * Restore a pixmap's data which is downloaded by 
 * glamor_download_pixmap_to_cpu to its original 
 * gl texture. Used by glamor_finish_access. 
 *
 * The pixmap must be
 * in texture originally. In other word, the gl_fbo
 * must be 1.
 **/
void glamor_restore_pixmap_to_texture(PixmapPtr pixmap);
/**
 * Ensure to have a fbo has a valid/complete glfbo.
 * If the fbo already has a valid glfbo then do nothing.
 * Otherwise, it will generate a new glfbo, and bind
 * the fbo's texture to the glfbo.
 * The fbo must has a valid texture before call this
 * API, othersie, it will trigger a assert.
 */
void glamor_pixmap_ensure_fb(glamor_pixmap_fbo *fbo);

/**
 * Upload a pixmap to gl texture. Used by dynamic pixmap
 * uploading feature. The pixmap must be a software pixmap.
 * This function will change current FBO and current shaders.
 */
enum glamor_pixmap_status glamor_upload_pixmap_to_texture(PixmapPtr
							  pixmap);

/** 
 * Upload a picture to gl texture. Similar to the
 * glamor_upload_pixmap_to_texture. Used in rendering.
 **/
enum glamor_pixmap_status
 glamor_upload_picture_to_texture(PicturePtr picture);

/**
 * Destroy all the resources allocated on the uploading
 * phase, includs the tex and fbo.
 **/
void glamor_destroy_upload_pixmap(PixmapPtr pixmap);

void glamor_validate_pixmap(PixmapPtr pixmap);

int glamor_create_picture(PicturePtr picture);

Bool
glamor_prepare_access_picture(PicturePtr picture, glamor_access_t access);

void glamor_finish_access_picture(PicturePtr picture, glamor_access_t access);

void glamor_destroy_picture(PicturePtr picture);

enum glamor_pixmap_status
 glamor_upload_picture_to_texture(PicturePtr picture);

/* fixup a fbo to the exact size as the pixmap. */
Bool
glamor_fixup_pixmap_priv(ScreenPtr screen, glamor_pixmap_private *pixmap_priv);

void
glamor_picture_format_fixup(PicturePtr picture,
			    glamor_pixmap_private * pixmap_priv);

void
glamor_get_image(DrawablePtr pDrawable, int x, int y, int w, int h,
		 unsigned int format, unsigned long planeMask, char *d);

void
glamor_add_traps(PicturePtr pPicture,
		 INT16 x_off, 
		 INT16 y_off, int ntrap, xTrap * traps);

RegionPtr
glamor_copy_plane(DrawablePtr pSrc, DrawablePtr pDst, GCPtr pGC,
		  int srcx, int srcy, int w, int h, int dstx, int dsty,
		  unsigned long bitPlane);

void
glamor_image_glyph_blt(DrawablePtr pDrawable, GCPtr pGC,
                    int x, int y, unsigned int nglyph,
                    CharInfoPtr * ppci, pointer pglyphBase);

void
glamor_poly_glyph_blt(DrawablePtr pDrawable, GCPtr pGC,
                    int x, int y, unsigned int nglyph,
                    CharInfoPtr * ppci, pointer pglyphBase);

void
glamor_push_pixels(GCPtr pGC, PixmapPtr pBitmap,
		   DrawablePtr pDrawable, int w, int h, int x, int y);

void
glamor_poly_point(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		  DDXPointPtr ppt);

void
glamor_poly_segment(DrawablePtr pDrawable, GCPtr pGC, int nseg,
		    xSegment *pSeg);

void
glamor_poly_line(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
		 DDXPointPtr ppt);

#include"glamor_utils.h"

/* Dynamic pixmap upload to texture if needed. 
 * Sometimes, the target is a gl texture pixmap/picture,
 * but the source or mask is in cpu memory. In that case,
 * upload the source/mask to gl texture and then avoid 
 * fallback the whole process to cpu. Most of the time,
 * this will increase performance obviously. */

#define GLAMOR_PIXMAP_DYNAMIC_UPLOAD
//#define GLAMOR_DELAYED_FILLING



#endif				/* GLAMOR_PRIV_H */
