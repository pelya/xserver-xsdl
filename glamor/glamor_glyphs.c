/*
 * Copyright © 2008 Red Hat, Inc.
 * Partly based on code Copyright © 2000 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * Red Hat DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL Red Hat
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Owen Taylor <otaylor@fishsoup.net>
 * Based on code by: Keith Packard
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdlib.h>

#include "glamor_priv.h"

#include "mipict.h"

#if DEBUG_GLYPH_CACHE
#define DBG_GLYPH_CACHE(a) ErrorF a
#else
#define DBG_GLYPH_CACHE(a)
#endif

/* Width of the pixmaps we use for the caches; this should be less than
 * max texture size of the driver; this may need to actually come from
 * the driver.
 */

/* Maximum number of glyphs we buffer on the stack before flushing
 * rendering to the mask or destination surface.
 */
#define GLYPH_BUFFER_SIZE 1024

#define CACHE_PICTURE_SIZE  1024
#define GLYPH_MIN_SIZE 8
#define GLYPH_MAX_SIZE 64
#define GLYPH_CACHE_SIZE (CACHE_PICTURE_SIZE * CACHE_PICTURE_SIZE / (GLYPH_MIN_SIZE * GLYPH_MIN_SIZE))

typedef struct {
	PicturePtr source;
	glamor_composite_rect_t rects[GLYPH_BUFFER_SIZE];
	int count;
} glamor_glyph_buffer_t;

struct glamor_glyph {
	glamor_glyph_cache_t *cache;
	uint16_t x, y;
	uint16_t size, pos;
};

typedef enum {
	GLAMOR_GLYPH_SUCCESS,	/* Glyph added to render buffer */
	GLAMOR_GLYPH_FAIL,	/* out of memory, etc */
	GLAMOR_GLYPH_NEED_FLUSH,	/* would evict a glyph already in the buffer */
} glamor_glyph_cache_result_t;



#define NeedsComponent(f) (PICT_FORMAT_A(f) != 0 && PICT_FORMAT_RGB(f) != 0)
static DevPrivateKeyRec glamor_glyph_key;

static inline struct glamor_glyph *
glamor_glyph_get_private(GlyphPtr glyph)
{
	return dixGetPrivate(&glyph->devPrivates, &glamor_glyph_key);
}

static inline void
glamor_glyph_set_private(GlyphPtr glyph, struct glamor_glyph *priv)
{
	dixSetPrivate(&glyph->devPrivates, &glamor_glyph_key, priv);
}


static void
glamor_unrealize_glyph_caches(ScreenPtr pScreen)
{
	glamor_screen_private *glamor = glamor_get_screen_private(pScreen);
	int i;

	if (!glamor->glyph_cache_initialized)
		return;

	for (i = 0; i < GLAMOR_NUM_GLYPH_CACHE_FORMATS; i++) {
		glamor_glyph_cache_t *cache = &glamor->glyphCaches[i];

		if (cache->picture)
			FreePicture(cache->picture, 0);

		if (cache->glyphs)
			free(cache->glyphs);
	}
	glamor->glyph_cache_initialized = FALSE;
}

void
glamor_glyphs_fini(ScreenPtr pScreen)
{
	glamor_unrealize_glyph_caches(pScreen);
}

/* All caches for a single format share a single pixmap for glyph storage,
 * allowing mixing glyphs of different sizes without paying a penalty
 * for switching between source pixmaps. (Note that for a size of font
 * right at the border between two sizes, we might be switching for almost
 * every glyph.)
 *
 * This function allocates the storage pixmap, and then fills in the
 * rest of the allocated structures for all caches with the given format.
 */
static Bool
glamor_realize_glyph_caches(ScreenPtr pScreen)
{
	glamor_screen_private *glamor = glamor_get_screen_private(pScreen);
	unsigned int formats[] = {
		PIXMAN_a8,
		PIXMAN_a8r8g8b8,
	};
	int i;

	if (glamor->glyph_cache_initialized)
		return TRUE;

	glamor->glyph_cache_initialized = TRUE;
	memset(glamor->glyphCaches, 0, sizeof(glamor->glyphCaches));

	for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
		glamor_glyph_cache_t *cache = &glamor->glyphCaches[i];
		PixmapPtr pixmap;
		PicturePtr picture;
		CARD32 component_alpha;
		int depth = PIXMAN_FORMAT_DEPTH(formats[i]);
		int error;
		PictFormatPtr pPictFormat =
		    PictureMatchFormat(pScreen, depth, formats[i]);
		if (!pPictFormat)
			goto bail;

		/* Now allocate the pixmap and picture */
		pixmap = pScreen->CreatePixmap(pScreen,
					       CACHE_PICTURE_SIZE,
					       CACHE_PICTURE_SIZE, depth,
					       0);
		if (!pixmap)
			goto bail;

		component_alpha = NeedsComponent(pPictFormat->format);
		picture = CreatePicture(0, &pixmap->drawable, pPictFormat,
					CPComponentAlpha, &component_alpha,
					serverClient, &error);

		pScreen->DestroyPixmap(pixmap);
		if (!picture)
			goto bail;

		ValidatePicture(picture);

		cache->picture = picture;
		cache->glyphs = calloc(sizeof(GlyphPtr), GLYPH_CACHE_SIZE);
		if (!cache->glyphs)
			goto bail;

		cache->evict = rand() % GLYPH_CACHE_SIZE;
	}
	assert(i == GLAMOR_NUM_GLYPH_CACHE_FORMATS);

	return TRUE;

      bail:
	glamor_unrealize_glyph_caches(pScreen);
	return FALSE;
}


Bool
glamor_glyphs_init(ScreenPtr pScreen)
{
	if (!dixRegisterPrivateKey(&glamor_glyph_key, PRIVATE_GLYPH, 0))
		return FALSE;

	/* Skip pixmap creation if we don't intend to use it. */

	return glamor_realize_glyph_caches(pScreen);
}

/* The most efficient thing to way to upload the glyph to the screen
 * is to use CopyArea; glamor pixmaps are always offscreen.
 */
static void
glamor_glyph_cache_upload_glyph(ScreenPtr screen,
				glamor_glyph_cache_t * cache,
				GlyphPtr glyph, int x, int y)
{
	PicturePtr pGlyphPicture = GlyphPicture(glyph)[screen->myNum];
	PixmapPtr pGlyphPixmap = (PixmapPtr) pGlyphPicture->pDrawable;
	PixmapPtr pCachePixmap = (PixmapPtr) cache->picture->pDrawable;
	PixmapPtr scratch;
	GCPtr gc;

	gc = GetScratchGC(pCachePixmap->drawable.depth, screen);
	if (!gc)
		return;

	ValidateGC(&pCachePixmap->drawable, gc);

	scratch = pGlyphPixmap;
	(*gc->ops->CopyArea)(&scratch->drawable, &pCachePixmap->drawable, gc,
			     0, 0, glyph->info.width, glyph->info.height, x,
			     y);

	if (scratch != pGlyphPixmap)
		screen->DestroyPixmap(scratch);

	FreeScratchGC(gc);
}


void
glamor_glyph_unrealize(ScreenPtr screen, GlyphPtr glyph)
{
	struct glamor_glyph *priv;

	/* Use Lookup in case we have not attached to this glyph. */
	priv = dixLookupPrivate(&glyph->devPrivates, &glamor_glyph_key);
	if (priv == NULL)
		return;

	priv->cache->glyphs[priv->pos] = NULL;

	glamor_glyph_set_private(glyph, NULL);
	free(priv);
}

/* Cut and paste from render/glyph.c - probably should export it instead */
static void
glamor_glyph_extents(int nlist,
		     GlyphListPtr list, GlyphPtr * glyphs, BoxPtr extents)
{
	int x1, x2, y1, y2;
	int x, y, n;

	x1 = y1 = MAXSHORT;
	x2 = y2 = MINSHORT;
	x = y = 0;
	while (nlist--) {
		x += list->xOff;
		y += list->yOff;
		n = list->len;
		list++;
		while (n--) {
			GlyphPtr glyph = *glyphs++;
			int v;

			v = x - glyph->info.x;
			if (v < x1)
				x1 = v;
			v += glyph->info.width;
			if (v > x2)
				x2 = v;

			v = y - glyph->info.y;
			if (v < y1)
				y1 = v;
			v += glyph->info.height;
			if (v > y2)
				y2 = v;

			x += glyph->info.xOff;
			y += glyph->info.yOff;
		}
	}

	extents->x1 = x1 < MINSHORT ? MINSHORT : x1;
	extents->x2 = x2 > MAXSHORT ? MAXSHORT : x2;
	extents->y1 = y1 < MINSHORT ? MINSHORT : y1;
	extents->y2 = y2 > MAXSHORT ? MAXSHORT : y2;
}

/**
 * Returns TRUE if the glyphs in the lists intersect.  Only checks based on
 * bounding box, which appears to be good enough to catch most cases at least.
 */
static Bool
glamor_glyphs_intersect(int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
	int x1, x2, y1, y2;
	int n;
	int x, y;
	BoxRec extents;
	Bool first = TRUE;

	x = 0;
	y = 0;
	extents.x1 = 0;
	extents.y1 = 0;
	extents.x2 = 0;
	extents.y2 = 0;
	while (nlist--) {
		x += list->xOff;
		y += list->yOff;
		n = list->len;
		list++;
		while (n--) {
			GlyphPtr glyph = *glyphs++;

			if (glyph->info.width == 0
			    || glyph->info.height == 0) {
				x += glyph->info.xOff;
				y += glyph->info.yOff;
				continue;
			}

			x1 = x - glyph->info.x;
			if (x1 < MINSHORT)
				x1 = MINSHORT;
			y1 = y - glyph->info.y;
			if (y1 < MINSHORT)
				y1 = MINSHORT;
			x2 = x1 + glyph->info.width;
			if (x2 > MAXSHORT)
				x2 = MAXSHORT;
			y2 = y1 + glyph->info.height;
			if (y2 > MAXSHORT)
				y2 = MAXSHORT;

			if (first) {
				extents.x1 = x1;
				extents.y1 = y1;
				extents.x2 = x2;
				extents.y2 = y2;
				first = FALSE;
			} else {
				if (x1 < extents.x2 && x2 > extents.x1
				    && y1 < extents.y2
				    && y2 > extents.y1) {
					return TRUE;
				}

				if (x1 < extents.x1)
					extents.x1 = x1;
				if (x2 > extents.x2)
					extents.x2 = x2;
				if (y1 < extents.y1)
					extents.y1 = y1;
				if (y2 > extents.y2)
					extents.y2 = y2;
			}
			x += glyph->info.xOff;
			y += glyph->info.yOff;
		}
	}

	return FALSE;
}


static inline unsigned int
glamor_glyph_size_to_count(int size)
{
	size /= GLYPH_MIN_SIZE;
	return size * size;
}

static inline unsigned int
glamor_glyph_count_to_mask(int count)
{
	return ~(count - 1);
}

static inline unsigned int
glamor_glyph_size_to_mask(int size)
{
	return
	    glamor_glyph_count_to_mask(glamor_glyph_size_to_count(size));
}

static PicturePtr
glamor_glyph_cache(ScreenPtr screen, GlyphPtr glyph, int *out_x,
		   int *out_y)
{
	glamor_screen_private *glamor = glamor_get_screen_private(screen);
	PicturePtr glyph_picture = GlyphPicture(glyph)[screen->myNum];
	glamor_glyph_cache_t *cache =
	    &glamor->glyphCaches[PICT_FORMAT_RGB(glyph_picture->format) !=
				 0];
	struct glamor_glyph *priv = NULL;
	int size, mask, pos, s;

	if (glyph->info.width > GLYPH_MAX_SIZE
	    || glyph->info.height > GLYPH_MAX_SIZE)
		return NULL;

	for (size = GLYPH_MIN_SIZE; size <= GLYPH_MAX_SIZE; size *= 2)
		if (glyph->info.width <= size
		    && glyph->info.height <= size)
			break;

	s = glamor_glyph_size_to_count(size);
	mask = glamor_glyph_count_to_mask(s);
	pos = (cache->count + s - 1) & mask;
	if (pos < GLYPH_CACHE_SIZE) {
		cache->count = pos + s;
	} else {
		for (s = size; s <= GLYPH_MAX_SIZE; s *= 2) {
			int i =
			    cache->evict & glamor_glyph_size_to_mask(s);
			GlyphPtr evicted = cache->glyphs[i];
			if (evicted == NULL)
				continue;

			priv = glamor_glyph_get_private(evicted);
			if (priv->size >= s) {
				cache->glyphs[i] = NULL;
				glamor_glyph_set_private(evicted, NULL);
				pos = cache->evict &
				    glamor_glyph_size_to_mask(size);
			} else
				priv = NULL;
			break;
		}
		if (priv == NULL) {
			int count = glamor_glyph_size_to_count(size);
			mask = glamor_glyph_count_to_mask(count);
			pos = cache->evict & mask;
			for (s = 0; s < count; s++) {
				GlyphPtr evicted = cache->glyphs[pos + s];
				if (evicted != NULL) {
					if (priv != NULL)
						free(priv);

					priv =
					    glamor_glyph_get_private
					    (evicted);
					glamor_glyph_set_private(evicted,
								 NULL);
					cache->glyphs[pos + s] = NULL;
				}
			}
		}
		/* And pick a new eviction position */
		cache->evict = rand() % GLYPH_CACHE_SIZE;
	}

	if (priv == NULL) {
		priv = malloc(sizeof(struct glamor_glyph));
		if (priv == NULL)
			return NULL;
	}

	glamor_glyph_set_private(glyph, priv);
	cache->glyphs[pos] = glyph;

	priv->cache = cache;
	priv->size = size;
	priv->pos = pos;
	s = pos / ((GLYPH_MAX_SIZE / GLYPH_MIN_SIZE) *
		   (GLYPH_MAX_SIZE / GLYPH_MIN_SIZE));
	priv->x =
	    s % (CACHE_PICTURE_SIZE / GLYPH_MAX_SIZE) * GLYPH_MAX_SIZE;
	priv->y =
	    (s / (CACHE_PICTURE_SIZE / GLYPH_MAX_SIZE)) * GLYPH_MAX_SIZE;
	for (s = GLYPH_MIN_SIZE; s < GLYPH_MAX_SIZE; s *= 2) {
		if (pos & 1)
			priv->x += s;
		if (pos & 2)
			priv->y += s;
		pos >>= 2;
	}

	glamor_glyph_cache_upload_glyph(screen, cache, glyph, priv->x,
					priv->y);

	*out_x = priv->x;
	*out_y = priv->y;
	return cache->picture;
}

static glamor_glyph_cache_result_t
glamor_buffer_glyph(ScreenPtr screen,
		    glamor_glyph_buffer_t * buffer,
		    GlyphPtr glyph, int x_glyph, int y_glyph)
{
	glamor_screen_private *glamor_screen =
	    glamor_get_screen_private(screen);
	unsigned int format = (GlyphPicture(glyph)[screen->myNum])->format;
	glamor_composite_rect_t *rect;
	PicturePtr source;
	struct glamor_glyph *priv;
	int x, y;
	PicturePtr glyph_picture = GlyphPicture(glyph)[screen->myNum];
	glamor_glyph_cache_t *cache;

	if (PICT_FORMAT_BPP(format) == 1)
		format = PICT_a8;

	cache =
	    &glamor_screen->glyphCaches[PICT_FORMAT_RGB
					(glyph_picture->format) != 0];

	if (buffer->source && buffer->source != cache->picture)
		return GLAMOR_GLYPH_NEED_FLUSH;

	if (buffer->count == GLYPH_BUFFER_SIZE)
		return GLAMOR_GLYPH_NEED_FLUSH;

	priv = glamor_glyph_get_private(glyph);

	if (priv) {
		rect = &buffer->rects[buffer->count++];
		rect->x_src = priv->x;
		rect->y_src = priv->y;
		if (buffer->source == NULL)
			buffer->source = priv->cache->picture;
	} else {
		source = glamor_glyph_cache(screen, glyph, &x, &y);
		if (source != NULL) {
			rect = &buffer->rects[buffer->count++];
			rect->x_src = x;
			rect->y_src = y;
			if (buffer->source == NULL)
				buffer->source = source;
		} else {
			source = GlyphPicture(glyph)[screen->myNum];
			if (buffer->source && buffer->source != source)
				return GLAMOR_GLYPH_NEED_FLUSH;
			buffer->source = source;

			rect = &buffer->rects[buffer->count++];
			rect->x_src = 0;
			rect->y_src = 0;
		}
	}

	rect->x_dst = x_glyph - glyph->info.x;
	rect->y_dst = y_glyph - glyph->info.y;
	rect->width = glyph->info.width;
	rect->height = glyph->info.height;

	/* Couldn't find the glyph in the cache, use the glyph picture directly */

	return GLAMOR_GLYPH_SUCCESS;
}


static void
glamor_glyphs_flush_mask(PicturePtr mask, glamor_glyph_buffer_t * buffer)
{
#ifdef RENDER
	glamor_composite_rects(PictOpAdd, buffer->source, NULL, mask,
			       buffer->count, buffer->rects);
#endif
	buffer->count = 0;
	buffer->source = NULL;
}

static void
glamor_glyphs_via_mask(CARD8 op,
		       PicturePtr src,
		       PicturePtr dst,
		       PictFormatPtr mask_format,
		       INT16 x_src,
		       INT16 y_src,
		       int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
	PixmapPtr mask_pixmap = 0;
	PicturePtr mask;
	ScreenPtr screen = dst->pDrawable->pScreen;
	int width = 0, height = 0;
	int x, y;
	int x_dst = list->xOff, y_dst = list->yOff;
	int n;
	GlyphPtr glyph;
	int error;
	BoxRec extents = { 0, 0, 0, 0 };
	CARD32 component_alpha;
	glamor_glyph_buffer_t buffer;
	xRectangle fill_rect;

	GCPtr gc;

	glamor_glyph_extents(nlist, list, glyphs, &extents);

	if (extents.x2 <= extents.x1 || extents.y2 <= extents.y1)
		return;
	width = extents.x2 - extents.x1;
	height = extents.y2 - extents.y1;

	if (mask_format->depth == 1) {
		PictFormatPtr a8Format =
		    PictureMatchFormat(screen, 8, PICT_a8);

		if (a8Format)
			mask_format = a8Format;
	}

	mask_pixmap = screen->CreatePixmap(screen, width, height,
					   mask_format->depth,
					   CREATE_PIXMAP_USAGE_SCRATCH);
	if (!mask_pixmap)
		return;
	component_alpha = NeedsComponent(mask_format->format);
	mask = CreatePicture(0, &mask_pixmap->drawable,
			     mask_format, CPComponentAlpha,
			     &component_alpha, serverClient, &error);
	if (!mask) {
		screen->DestroyPixmap(mask_pixmap);
		return;
	}
	gc = GetScratchGC(mask_pixmap->drawable.depth, screen);
	ValidateGC(&mask_pixmap->drawable, gc);
	gc->fillStyle = FillSolid;
	//glamor_fill(&mask_pixmap->drawable, gc, 0, 0, width, height, TRUE);
	fill_rect.x = 0;
	fill_rect.y = 0;
	fill_rect.width = width;
	fill_rect.height = height;
	gc->ops->PolyFillRect(&mask_pixmap->drawable, gc, 1, &fill_rect);
	
	FreeScratchGC(gc);
	x = -extents.x1;
	y = -extents.y1;

	buffer.count = 0;
	buffer.source = NULL;
	while (nlist--) {
		x += list->xOff;
		y += list->yOff;
		n = list->len;
		while (n--) {
			glyph = *glyphs++;

			if (glyph->info.width > 0
			    && glyph->info.height > 0
			    && glamor_buffer_glyph(screen, &buffer,
						   glyph, x,
						   y) ==
			    GLAMOR_GLYPH_NEED_FLUSH) {

				glamor_glyphs_flush_mask(mask, &buffer);

				glamor_buffer_glyph(screen, &buffer,
						    glyph, x, y);
			}

			x += glyph->info.xOff;
			y += glyph->info.yOff;
		}
		list++;
	}

	if (buffer.count)
		glamor_glyphs_flush_mask(mask, &buffer);

	x = extents.x1;
	y = extents.y1;
	CompositePicture(op,
			 src,
			 mask,
			 dst,
			 x_src + x - x_dst,
			 y_src + y - y_dst, 0, 0, x, y, width, height);
	FreePicture(mask, 0);
	screen->DestroyPixmap(mask_pixmap);
}

static void
glamor_glyphs_flush_dst(CARD8 op,
			PicturePtr src,
			PicturePtr dst,
			glamor_glyph_buffer_t * buffer,
			INT16 x_src, INT16 y_src, INT16 x_dst, INT16 y_dst)
{
	int i;
	glamor_composite_rect_t *rect = &buffer->rects[0];
	for (i = 0; i < buffer->count; i++, rect++) {
		rect->x_mask = rect->x_src;
		rect->y_mask = rect->y_src;
		rect->x_src = x_src + rect->x_dst - x_dst;
		rect->y_src = y_src + rect->y_dst - y_dst;
	}

	glamor_composite_rects(op, src, buffer->source, dst,
			       buffer->count, &buffer->rects[0]);

	buffer->count = 0;
	buffer->source = NULL;
}

static void
glamor_glyphs_to_dst(CARD8 op,
		     PicturePtr src,
		     PicturePtr dst,
		     INT16 x_src,
		     INT16 y_src,
		     int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
	ScreenPtr screen = dst->pDrawable->pScreen;
	int x = 0, y = 0;
	int x_dst = list->xOff, y_dst = list->yOff;
	int n;
	GlyphPtr glyph;
	glamor_glyph_buffer_t buffer;

	buffer.count = 0;
	buffer.source = NULL;
	while (nlist--) {
		x += list->xOff;
		y += list->yOff;
		n = list->len;
		while (n--) {
			glyph = *glyphs++;

			if (glyph->info.width > 0
			    && glyph->info.height > 0
			    && glamor_buffer_glyph(screen, &buffer,
						   glyph, x,
						   y) ==
			    GLAMOR_GLYPH_NEED_FLUSH) {
				glamor_glyphs_flush_dst(op, src, dst,
							&buffer, x_src,
							y_src, x_dst,
							y_dst);
				glamor_buffer_glyph(screen, &buffer,
						    glyph, x, y);
			}

			x += glyph->info.xOff;
			y += glyph->info.yOff;
		}
		list++;
	}

	if (buffer.count)
		glamor_glyphs_flush_dst(op, src, dst, &buffer,
					x_src, y_src, x_dst, y_dst);
}

static Bool
_glamor_glyphs(CARD8 op,
	       PicturePtr src,
	       PicturePtr dst,
	       PictFormatPtr mask_format,
	       INT16 x_src,
	       INT16 y_src, int nlist, GlyphListPtr list, 
	       GlyphPtr * glyphs, Bool fallback)
{
	/* If we don't have a mask format but all the glyphs have the same format
	 * and don't intersect, use the glyph format as mask format for the full
	 * benefits of the glyph cache.
	 */
	if (!mask_format) {
		Bool same_format = TRUE;
		int i;

		mask_format = list[0].format;

		for (i = 0; i < nlist; i++) {
			if (mask_format->format != list[i].format->format) {
				same_format = FALSE;
				break;
			}
		}

		if (!same_format || (mask_format->depth != 1 &&
				     glamor_glyphs_intersect(nlist, list,
							     glyphs))) {
			mask_format = NULL;
		}
	}

	if (mask_format)
		glamor_glyphs_via_mask(op, src, dst, mask_format,
				       x_src, y_src, nlist, list, glyphs);
	else
		glamor_glyphs_to_dst(op, src, dst, x_src, y_src, nlist,
				     list, glyphs);
	return TRUE;
}

void
glamor_glyphs(CARD8 op,
	      PicturePtr src,
	      PicturePtr dst,
	      PictFormatPtr mask_format,
	      INT16 x_src,
	      INT16 y_src, int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
	_glamor_glyphs(op, src, dst, mask_format, x_src, 
		       y_src, nlist, list, glyphs, TRUE);
}

Bool
glamor_glyphs_nf(CARD8 op,
		 PicturePtr src,
		 PicturePtr dst,
		 PictFormatPtr mask_format,
		 INT16 x_src,
		 INT16 y_src, int nlist, 
		 GlyphListPtr list, GlyphPtr * glyphs)
{
	return _glamor_glyphs(op, src, dst, mask_format, x_src, 
			      y_src, nlist, list, glyphs, FALSE);
}

