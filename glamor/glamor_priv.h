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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include <xorg-config.h>
#endif


#include "glamor.h"

#ifdef GLAMOR_GLES2

#define GLEW_ES_ONLY 1

#define GL_BGRA                                 GL_BGRA_EXT
#define GL_COLOR_INDEX                          0x1900
#define GL_BITMAP                               0x1A00
#define GL_UNSIGNED_INT_8_8_8_8                 0x8035
#define GL_UNSIGNED_INT_8_8_8_8_REV             0x8367
#define GL_UNSIGNED_INT_2_10_10_10_REV          0x8368
#define GL_UNSIGNED_SHORT_5_6_5_REV             0x8364
#define GL_UNSIGNED_SHORT_1_5_5_5_REV           0x8366
#define GL_UNSIGNED_SHORT_4_4_4_4_REV           0x8365

#endif

#include <GL/glew.h>

#ifdef RENDER
#include "glyphstr.h"
#endif


#ifndef MAX_WIDTH
#define MAX_WIDTH 4096
#endif

#ifndef MAX_HEIGHT
#define MAX_HEIGHT 4096
#endif

#include "glamor_debug.h"

#define glamor_check_fbo_width_height(_w_, _h_)    ((_w_) > 0 && (_h_) > 0 \
                                                    && (_w_) < MAX_WIDTH   \
                                                    && (_h_) < MAX_HEIGHT)

#define glamor_check_fbo_depth(_depth_) (			\
                                         _depth_ == 8		\
	                                 || _depth_ == 15	\
                                         || _depth_ == 16	\
                                         || _depth_ == 24	\
                                         || _depth_ == 30	\
                                         || _depth_ == 32)


#define GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv) (pixmap_priv->is_picture == 1)
#define GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)    (pixmap_priv->gl_fbo == 1)

typedef struct glamor_transform_uniforms {
  GLint x_bias;
  GLint x_scale;
  GLint y_bias;
  GLint y_scale;
} glamor_transform_uniforms;

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

typedef struct {
  unsigned char sha1[20];
} glamor_cached_glyph_t;

typedef struct {
  /* The identity of the cache, statically configured at initialization */
  unsigned int format;
  int glyph_width;
  int glyph_height;

  /* Size of cache; eventually this should be dynamically determined */
  int size;

  /* Hash table mapping from glyph sha1 to position in the glyph; we use
   * open addressing with a hash table size determined based on size and large
   * enough so that we always have a good amount of free space, so we can
   * use linear probing. (Linear probing is preferrable to double hashing
   * here because it allows us to easily remove entries.)
   */
  int *hash_entries;
  int hash_size;

  glamor_cached_glyph_t *glyphs;
  int glyph_count;		/* Current number of glyphs */

  PicturePtr picture;	/* Where the glyphs of the cache are stored */
  int y_offset;		/* y location within the picture where the cache starts */
  int columns;		/* Number of columns the glyphs are layed out in */
  int eviction_position;	/* Next random position to evict a glyph */
} glamor_glyph_cache_t;

#define GLAMOR_NUM_GLYPH_CACHES 4

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
typedef void (*glamor_pixmap_validate_function_t)(struct glamor_screen_private*, 
					          struct glamor_pixmap_private*);
#define GLAMOR_CREATE_PIXMAP_CPU  0x100
typedef struct glamor_screen_private {
  CloseScreenProcPtr saved_close_screen;
  CreateGCProcPtr saved_create_gc;
  CreatePixmapProcPtr saved_create_pixmap;
  DestroyPixmapProcPtr saved_destroy_pixmap;
  GetSpansProcPtr saved_get_spans;
  GetImageProcPtr saved_get_image;
  CompositeProcPtr saved_composite;
  TrapezoidsProcPtr saved_trapezoids;
  GlyphsProcPtr saved_glyphs;
  ChangeWindowAttributesProcPtr saved_change_window_attributes;
  CopyWindowProcPtr saved_copy_window;
  BitmapToRegionProcPtr saved_bitmap_to_region;
  TrianglesProcPtr saved_triangles;
  CreatePictureProcPtr saved_create_picture;
  DestroyPictureProcPtr saved_destroy_picture;

  int yInverted;
  int screen_fbo;
  GLuint vbo;
  int vbo_offset;
  int vbo_size;
  char *vb;
  int vb_stride;

  /* glamor_finishaccess */
  GLint finish_access_prog[2];

  /* glamor_solid */
  GLint solid_prog;
  GLint solid_color_uniform_location;

  /* glamor_tile */
  GLint tile_prog;

  /* glamor_putimage */
  GLint put_image_xybitmap_prog;
  glamor_transform_uniforms put_image_xybitmap_transform;
  GLint put_image_xybitmap_fg_uniform_location;
  GLint put_image_xybitmap_bg_uniform_location;

  /* glamor_composite */
  glamor_composite_shader composite_shader[SHADER_SOURCE_COUNT]
  [SHADER_MASK_COUNT]
  [SHADER_IN_COUNT];
  Bool has_source_coords, has_mask_coords;
  int render_nr_verts;
  glamor_pixmap_validate_function_t *pixmap_validate_funcs;
  glamor_glyph_cache_t glyph_caches[GLAMOR_NUM_GLYPH_CACHES];
  char delayed_fallback_string[GLAMOR_DELAYED_STRING_MAX + 1];
  int  delayed_fallback_pending;
} glamor_screen_private;

typedef enum glamor_access {
  GLAMOR_ACCESS_RO,
  GLAMOR_ACCESS_RW,
  GLAMOR_ACCESS_WO,
} glamor_access_t;

/*
 * glamor_pixmap_private - glamor pixmap's private structure.
 * @gl_fbo:  The pixmap is attached to a fbo originally.
 * @gl_tex:  The pixmap is in a gl texture originally.
 * @pbo_valid: The pbo has a valid copy of the pixmap's data.
 * @is_picture: The drawable is attached to a picture.
 * @tex:     attached texture.
 * @fb:      attached fbo.
 * @pbo:     attached pbo.
 * @access_mode: access mode during the prepare/finish pair.
 * @pict_format: the corresponding picture's format.
 * #pending_op: currently only support pending filling.
 * @container: The corresponding pixmap's pointer.
 **/

#define GLAMOR_PIXMAP_PRIV_NEED_VALIDATE(pixmap_priv)  \
	(GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv) \
	&& (pixmap_priv->pending_op.type != GLAMOR_PENDING_NONE))

#define GLAMOR_PIXMAP_PRIV_NO_PENDING(pixmap_priv)   \
	(pixmap_priv->pending_op.type == GLAMOR_PENDING_NONE)

enum _glamor_pending_op_type{
    GLAMOR_PENDING_NONE,
    GLAMOR_PENDING_FILL
};

typedef struct _glamor_pending_fill {
    unsigned int type;
    GLfloat color4fv[4];
    CARD32  colori;
} glamor_pending_fill;

typedef union _glamor_pending_op {
    unsigned int type;
    glamor_pending_fill fill;
} glamor_pending_op;


typedef struct glamor_pixmap_private {
  unsigned char gl_fbo:1;
  unsigned char gl_tex:1;
  unsigned char pbo_valid:1;
  unsigned char is_picture:1;
  GLuint tex;			
  GLuint fb;
  GLuint pbo;                
  glamor_access_t access_mode;
  PictFormatShort pict_format;
  glamor_pending_op pending_op;
  PixmapPtr container;
} glamor_pixmap_private;

#define GLAMOR_CHECK_PENDING_FILL(_glamor_priv_, _pixmap_priv_) do \
  { \
      if (_pixmap_priv_->pending_op.type == GLAMOR_PENDING_FILL) { \
        glUseProgramObjectARB(_glamor_priv_->solid_prog); \
        glUniform4fvARB(_glamor_priv_->solid_color_uniform_location, 1,  \
                        _pixmap_priv_->pending_op.fill.color4fv); \
      } \
  } while(0)
 

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
  return (glamor_screen_private *)dixLookupPrivate(&screen->devPrivates,
						   glamor_screen_private_key);
}
static inline glamor_pixmap_private *
glamor_get_pixmap_private(PixmapPtr pixmap)
{
  return dixLookupPrivate(&pixmap->devPrivates, glamor_pixmap_private_key);
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#define ALIGN(i,m)	(((i) + (m) - 1) & ~((m) - 1))
#define MIN(a,b)	((a) < (b) ? (a) : (b))

/**
 * Borrow from uxa.
 */
static inline CARD32
format_for_depth(int depth)
{
  switch (depth) {
  case 1: return PICT_a1;
  case 4: return PICT_a4;
  case 8: return PICT_a8;
  case 15: return PICT_x1r5g5b5;
  case 16: return PICT_r5g6b5;
  default:
  case 24: return PICT_x8r8g8b8;
#if XORG_VERSION_CURRENT >= 10699900
  case 30: return PICT_x2r10g10b10;
#endif
  case 32: return PICT_a8r8g8b8;
  }
}

static inline CARD32
format_for_pixmap(PixmapPtr pixmap)
{
  glamor_pixmap_private *pixmap_priv;
  PictFormatShort pict_format;

  pixmap_priv = glamor_get_pixmap_private(pixmap);
  if (GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv))
    pict_format = pixmap_priv->pict_format;
  else
    pict_format = format_for_depth(pixmap->drawable.depth);

  return pict_format;
}

/*
 * Map picture's format to the correct gl texture format and type.
 * xa is used to indicate whehter we need to wire alpha to 1. 
 *
 * Return 0 if find a matched texture type. Otherwise return -1.
 **/
static inline int 
glamor_get_tex_format_type_from_pictformat(PictFormatShort format,
					   GLenum *tex_format, 
					   GLenum *tex_type,
					   int *xa)
{
  *xa = 0;
  switch (format) {
  case PICT_a1:
    *tex_format = GL_COLOR_INDEX;
    *tex_type = GL_BITMAP;
    break;
  case PICT_b8g8r8x8:
    *xa = 1;
  case PICT_b8g8r8a8:
    *tex_format = GL_BGRA;
    *tex_type = GL_UNSIGNED_INT_8_8_8_8;
    break;

  case PICT_x8r8g8b8:
    *xa = 1;
  case PICT_a8r8g8b8:
    *tex_format = GL_BGRA;
    *tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    break;
  case PICT_x8b8g8r8:
    *xa = 1;
  case PICT_a8b8g8r8:
    *tex_format = GL_RGBA;
    *tex_type = GL_UNSIGNED_INT_8_8_8_8_REV;
    break;
  case PICT_x2r10g10b10:
    *xa = 1;
  case PICT_a2r10g10b10:
    *tex_format = GL_BGRA;
    *tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
    break;
  case PICT_x2b10g10r10:
    *xa = 1;
  case PICT_a2b10g10r10:
    *tex_format = GL_RGBA;
    *tex_type = GL_UNSIGNED_INT_2_10_10_10_REV;
    break;
 
  case PICT_r5g6b5:
    *tex_format = GL_RGB;
    *tex_type = GL_UNSIGNED_SHORT_5_6_5;
    break;
  case PICT_b5g6r5:
    *tex_format = GL_RGB;
    *tex_type = GL_UNSIGNED_SHORT_5_6_5_REV;
    break;
  case PICT_x1b5g5r5:
    *xa = 1;
  case PICT_a1b5g5r5:
    *tex_format = GL_RGBA;
    *tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
    break;
               
  case PICT_x1r5g5b5:
    *xa = 1;
  case PICT_a1r5g5b5:
    *tex_format = GL_BGRA;
    *tex_type = GL_UNSIGNED_SHORT_1_5_5_5_REV;
    break;
  case PICT_a8:
    *tex_format = GL_ALPHA;
    *tex_type = GL_UNSIGNED_BYTE;
    break;
  case PICT_x4r4g4b4:
    *xa = 1;
  case PICT_a4r4g4b4:
    *tex_format = GL_BGRA;
    *tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
    break;

  case PICT_x4b4g4r4:
    *xa = 1;
  case PICT_a4b4g4r4:
    *tex_format = GL_RGBA;
    *tex_type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
    break;
 
  default:
    LogMessageVerb(X_INFO, 0, "fail to get matched format for %x \n", format);
    return -1;
  }
  return 0;
}


static inline int 
glamor_get_tex_format_type_from_pixmap(PixmapPtr pixmap,
                                       GLenum *format, 
                                       GLenum *type, 
                                       int *ax)
{
  glamor_pixmap_private *pixmap_priv;
  PictFormatShort pict_format;

  pixmap_priv = glamor_get_pixmap_private(pixmap);
  if (GLAMOR_PIXMAP_PRIV_IS_PICTURE(pixmap_priv))
    pict_format = pixmap_priv->pict_format;
  else
    pict_format = format_for_depth(pixmap->drawable.depth);

  return glamor_get_tex_format_type_from_pictformat(pict_format, 
						    format, type, ax);  
}


/* borrowed from uxa */
static inline Bool
glamor_get_rgba_from_pixel(CARD32 pixel,
			   float * red,
			   float * green,
			   float * blue,
			   float * alpha,
			   CARD32 format)
{
  int rbits, bbits, gbits, abits;
  int rshift, bshift, gshift, ashift;

  rbits = PICT_FORMAT_R(format);
  gbits = PICT_FORMAT_G(format);
  bbits = PICT_FORMAT_B(format);
  abits = PICT_FORMAT_A(format);

  if (PICT_FORMAT_TYPE(format) == PICT_TYPE_A) {
    rshift = gshift = bshift = ashift = 0;
  } else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ARGB) {
    bshift = 0;
    gshift = bbits;
    rshift = gshift + gbits;
    ashift = rshift + rbits;
  } else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ABGR) {
    rshift = 0;
    gshift = rbits;
    bshift = gshift + gbits;
    ashift = bshift + bbits;
#if XORG_VERSION_CURRENT >= 10699900
  } else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_BGRA) {
    ashift = 0;
    rshift = abits;
    if (abits == 0)
      rshift = PICT_FORMAT_BPP(format) - (rbits+gbits+bbits);
    gshift = rshift + rbits;
    bshift = gshift + gbits;
#endif
  } else {
    return FALSE;
  }
#define COLOR_INT_TO_FLOAT(_fc_, _p_, _s_, _bits_)	\
  *_fc_ = (((_p_) >> (_s_)) & (( 1 << (_bits_)) - 1))	\
    / (float)((1<<(_bits_)) - 1) 

  if (rbits) 
    COLOR_INT_TO_FLOAT(red, pixel, rshift, rbits);
  else
    *red = 0;

  if (gbits) 
    COLOR_INT_TO_FLOAT(green, pixel, gshift, gbits);
  else
    *green = 0;

  if (bbits) 
    COLOR_INT_TO_FLOAT(blue, pixel, bshift, bbits);
  else
    *blue = 0;

  if (abits) 
    COLOR_INT_TO_FLOAT(alpha, pixel, ashift, abits);
  else
    *alpha = 1;

  return TRUE;
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

Bool glamor_close_screen(int idx, ScreenPtr screen);


/* glamor_copyarea.c */
RegionPtr
glamor_copy_area(DrawablePtr src, DrawablePtr dst, GCPtr gc,
		 int srcx, int srcy, int width, int height, int dstx, int dsty);
void
glamor_copy_n_to_n(DrawablePtr src,
		   DrawablePtr dst,
		   GCPtr gc,
		   BoxPtr box,
		   int nbox,
		   int		dx,
		   int		dy,
		   Bool		reverse,
		   Bool		upsidedown,
		   Pixel		bitplane,
		   void		*closure);

/* glamor_copywindow.c */
void glamor_copy_window(WindowPtr win, DDXPointRec old_origin,
			RegionPtr src_region);

/* glamor_core.c */
Bool glamor_prepare_access(DrawablePtr drawable, glamor_access_t access);
void glamor_finish_access(DrawablePtr drawable);
Bool glamor_prepare_access_window(WindowPtr window);
void glamor_finish_access_window(WindowPtr window);
Bool glamor_prepare_access_gc(GCPtr gc);
void glamor_finish_access_gc(GCPtr gc);
void glamor_init_finish_access_shaders(ScreenPtr screen);
const Bool glamor_get_drawable_location(const DrawablePtr drawable);
void glamor_get_drawable_deltas(DrawablePtr drawable, PixmapPtr pixmap,
				int *x, int *y);
Bool glamor_create_gc(GCPtr gc);
Bool glamor_stipple(PixmapPtr pixmap, PixmapPtr stipple,
		    int x, int y, int width, int height,
		    unsigned char alu, unsigned long planemask,
		    unsigned long fg_pixel, unsigned long bg_pixel,
		    int stipple_x, int stipple_y);
GLint glamor_compile_glsl_prog(GLenum type, const char *source);
void glamor_link_glsl_prog(GLint prog);
void glamor_get_color_4f_from_pixel(PixmapPtr pixmap, unsigned long fg_pixel,
				    GLfloat *color);

int glamor_set_destination_pixmap(PixmapPtr pixmap);
int glamor_set_destination_pixmap_priv(glamor_pixmap_private *pixmap_priv);

/* nc means no check. caller must ensure this pixmap has valid fbo.
 * usually use the GLAMOR_PIXMAP_PRIV_HAS_FBO firstly. 
 * */
void glamor_set_destination_pixmap_priv_nc(glamor_pixmap_private *pixmap_priv);

void glamor_set_alu(unsigned char alu);
Bool glamor_set_planemask(PixmapPtr pixmap, unsigned long planemask);
void glamor_get_transform_uniform_locations(GLint prog,
					    glamor_transform_uniforms *uniform_locations);
void glamor_set_transform_for_pixmap(PixmapPtr pixmap,
				     glamor_transform_uniforms *uniform_locations);
Bool glamor_change_window_attributes(WindowPtr pWin, unsigned long mask);
RegionPtr glamor_bitmap_to_region(PixmapPtr pixmap);

/* glamor_fill.c */
void glamor_fill(DrawablePtr drawable,
		 GCPtr gc,
		 int x,
		 int y,
		 int width,
		 int height);
Bool glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
		  unsigned char alu, unsigned long planemask,
		  unsigned long fg_pixel);
void glamor_solid_fail_region(PixmapPtr pixmap,
			      int x, int y, int width, int height);

/* glamor_fillspans.c */
void glamor_fill_spans(DrawablePtr drawable,
		       GCPtr	gc,
		       int n,
		       DDXPointPtr points,
		       int *widths,
		       int sorted);

void glamor_init_solid_shader(ScreenPtr screen);

/* glamor_getspans.c */
void
glamor_get_spans(DrawablePtr drawable,
		 int wmax,
		 DDXPointPtr points,
		 int *widths,
		 int nspans,
		 char *dst_start);

/* glamor_glyphs.c */
void glamor_glyphs_init(ScreenPtr screen);
void glamor_glyphs_fini(ScreenPtr screen);
void glamor_glyphs(CARD8 op,
		   PicturePtr pSrc,
		   PicturePtr pDst,
		   PictFormatPtr maskFormat,
		   INT16 xSrc,
		   INT16 ySrc, int nlist, GlyphListPtr list, GlyphPtr * glyphs);

/* glamor_setspans.c */
void glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
		      DDXPointPtr points, int *widths, int n, int sorted);

/* glamor_polyfillrect.c */
void
glamor_poly_fill_rect(DrawablePtr drawable,
		      GCPtr gc,
		      int nrect,
		      xRectangle *prect);

/* glamor_polylines.c */
void
glamor_poly_lines(DrawablePtr drawable, GCPtr gc, int mode, int n,
		  DDXPointPtr points);

/* glamor_putimage.c */
void
glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
		 int w, int h, int leftPad, int format, char *bits);
void glamor_init_putimage_shaders(ScreenPtr screen);

/* glamor_render.c */
void glamor_composite(CARD8 op,
		      PicturePtr pSrc,
		      PicturePtr pMask,
		      PicturePtr pDst,
		      INT16 xSrc,
		      INT16 ySrc,
		      INT16 xMask,
		      INT16 yMask,
		      INT16 xDst,
		      INT16 yDst,
		      CARD16 width,
		      CARD16 height);
void glamor_trapezoids(CARD8 op,
		       PicturePtr src, PicturePtr dst,
		       PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
		       int ntrap, xTrapezoid *traps);
void glamor_init_composite_shaders(ScreenPtr screen);
void glamor_composite_rects(CARD8 op,
			    PicturePtr src, PicturePtr dst,
			    int nrect, glamor_composite_rect_t *rects);

/* glamor_tile.c */
Bool glamor_tile(PixmapPtr pixmap, PixmapPtr tile,
		 int x, int y, int width, int height,
		 unsigned char alu, unsigned long planemask,
		 int tile_x, int tile_y);
void glamor_init_tile_shader(ScreenPtr screen);

/* glamor_triangles.c */
void
glamor_triangles (CARD8	    op,
		  PicturePtr    pSrc,
		  PicturePtr    pDst,
		  PictFormatPtr maskFormat,
		  INT16	    xSrc,
		  INT16	    ySrc,
		  int	    ntris,
		  xTriangle    *tris);

/* glamor_pixmap.c */

void
glamor_pixmap_init(ScreenPtr screen);
/** 
 * Download a pixmap's texture to cpu memory. If success,
 * One copy of current pixmap's texture will be put into
 * the pixmap->devPrivate.ptr. Will use pbo to map to 
 * the pointer if possible.
 * The pixmap must be a gl texture pixmap. gl_fbo and
 * gl_tex must be 1. Used by glamor_prepare_access.
 *
 */
Bool 
glamor_download_pixmap_to_cpu(PixmapPtr pixmap, glamor_access_t access);

/**
 * Restore a pixmap's data which is downloaded by 
 * glamor_download_pixmap_to_cpu to its original 
 * gl texture. Used by glamor_finish_access. 
 *
 * The pixmap must be
 * in texture originally. In other word, the gl_fbo
 * must be 1.
 **/
void
glamor_restore_pixmap_to_texture(PixmapPtr pixmap);

/**
 * Upload a pixmap to gl texture. Used by dynamic pixmap
 * uploading feature. The pixmap must be a software pixmap.
 * This function will change current FBO and current shaders.
 */
enum glamor_pixmap_status 
glamor_upload_pixmap_to_texture(PixmapPtr pixmap);

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
void
glamor_destroy_upload_pixmap(PixmapPtr pixmap);

void
glamor_validate_pixmap(PixmapPtr pixmap);

int
glamor_create_picture(PicturePtr picture);

Bool
glamor_prepare_access_picture(PicturePtr picture, glamor_access_t access);

void
glamor_finish_access_picture(PicturePtr picture);

void
glamor_destroy_picture(PicturePtr picture);

enum glamor_pixmap_status 
glamor_upload_picture_to_texture(PicturePtr picture);

void 
glamor_picture_format_fixup(PicturePtr picture, glamor_pixmap_private *pixmap_priv);

/* Dynamic pixmap upload to texture if needed. 
 * Sometimes, the target is a gl texture pixmap/picture,
 * but the source or mask is in cpu memory. In that case,
 * upload the source/mask to gl texture and then avoid 
 * fallback the whole process to cpu. Most of the time,
 * this will increase performance obviously. */


#define GLAMOR_PIXMAP_DYNAMIC_UPLOAD 
#define GLAMOR_DELAYED_FILLING


#include"glamor_utils.h" 

#endif /* GLAMOR_PRIV_H */
