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
#endif


#include "glamor.h"
#include <GL/glew.h>

#ifdef RENDER
#include "glyphstr.h"
#endif

typedef enum glamor_access {
    GLAMOR_ACCESS_RO,
    GLAMOR_ACCESS_RW,
} glamor_access_t;

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

    char *delayed_fallback_string;
    int yInverted;
    GLuint vbo;
    int vbo_offset;
    int vbo_size;
    char *vb;
    int vb_stride;

    /* glamor_finishaccess */
    GLint finish_access_prog;

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

    glamor_glyph_cache_t glyph_caches[GLAMOR_NUM_GLYPH_CACHES];
} glamor_screen_private;

typedef struct glamor_pixmap_private {
    GLuint tex;
    GLuint fb;
    GLuint pbo;
} glamor_pixmap_private;

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
 * Returns TRUE if the given planemask covers all the significant bits in the
 * pixel values for pDrawable.
 */
static inline Bool
glamor_pm_is_solid(DrawablePtr drawable, unsigned long planemask)
{
    return (planemask & FbFullMask(drawable->depth)) ==
	FbFullMask(drawable->depth);
}

static inline void
glamor_fallback(char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogMessageVerb(X_INFO, 0, "fallback: ");
    LogVMessageVerb(X_NONE, 0, format, ap);
    va_end(ap);
}

static inline void
glamor_delayed_fallback(ScreenPtr screen, char *format, ...)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    va_list ap;

    if (glamor_priv->delayed_fallback_string != NULL)
	return;

    va_start(ap, format);
    glamor_priv->delayed_fallback_string = XNFvprintf(format, ap);
    va_end(ap);
}

static inline void
glamor_clear_delayed_fallbacks(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    xfree(glamor_priv->delayed_fallback_string);
    glamor_priv->delayed_fallback_string = NULL;
}

static inline void
glamor_report_delayed_fallbacks(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    if (glamor_priv->delayed_fallback_string) {
	LogMessageVerb(X_INFO, 0, "fallback: %s",
		       glamor_priv->delayed_fallback_string);
	glamor_clear_delayed_fallbacks(screen);
    }
}

static inline float
v_from_x_coord_x(PixmapPtr pixmap, int x)
{
    return (float)x / pixmap->drawable.width * 2.0 - 1.0;
}

static inline float
v_from_x_coord_y(PixmapPtr pixmap, int y)
{
    return (float)y / pixmap->drawable.height * -2.0 + 1.0;
}

static inline float
v_from_x_coord_y_inverted(PixmapPtr pixmap, int y)
{
    return (float)y / pixmap->drawable.height * 2.0 - 1.0;
}


static inline float
t_from_x_coord_x(PixmapPtr pixmap, int x)
{
    return (float)x / pixmap->drawable.width;
}

static inline float
t_from_x_coord_y(PixmapPtr pixmap, int y)
{
    return 1.0 - (float)y / pixmap->drawable.height;
}

static inline float
t_from_x_coord_y_inverted(PixmapPtr pixmap, int y)
{
    return (float)y / pixmap->drawable.height;
}


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
void glamor_stipple(PixmapPtr pixmap, PixmapPtr stipple,
		    int x, int y, int width, int height,
		    unsigned char alu, unsigned long planemask,
		    unsigned long fg_pixel, unsigned long bg_pixel,
		    int stipple_x, int stipple_y);
GLint glamor_compile_glsl_prog(GLenum type, const char *source);
void glamor_link_glsl_prog(GLint prog);
void glamor_get_color_4f_from_pixel(PixmapPtr pixmap, unsigned long fg_pixel,
				    GLfloat *color);
Bool glamor_set_destination_pixmap(PixmapPtr pixmap);
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
void glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
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
void glamor_tile(PixmapPtr pixmap, PixmapPtr tile,
		 int x, int y, int width, int height,
		 unsigned char alu, unsigned long planemask,
		 int tile_x, int tile_y);
void glamor_init_tile_shader(ScreenPtr screen);

#endif /* GLAMOR_PRIV_H */
