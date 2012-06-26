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

#include <stdlib.h>

#include "glamor_priv.h"

#include <mipict.h>

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

#define CACHE_PICTURE_SIZE 1024
#define GLYPH_MIN_SIZE 8
#define GLYPH_MAX_SIZE 64
#define GLYPH_CACHE_SIZE (CACHE_PICTURE_SIZE * CACHE_PICTURE_SIZE / (GLYPH_MIN_SIZE * GLYPH_MIN_SIZE))

typedef struct {
	PicturePtr source;
	glamor_composite_rect_t rects[GLYPH_BUFFER_SIZE + 4];
	int count;
} glamor_glyph_buffer_t;

struct glamor_glyph {
	glamor_glyph_cache_t *cache;
	uint16_t x, y;
	uint16_t size, pos;
	unsigned long long left_x1_map, left_x2_map;
	unsigned long long right_x1_map, right_x2_map;  /* Use to check real pixel overlap or not. */
	Bool has_edge_map;
	Bool cached;
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
	return (struct glamor_glyph*)glyph->devPrivates;
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
		XID component_alpha;
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
	if (!dixRegisterPrivateKey(&glamor_glyph_key,
		PRIVATE_GLYPH, sizeof(struct glamor_glyph)))
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
	BoxRec box;
	GCPtr gc;

	gc = GetScratchGC(pCachePixmap->drawable.depth, screen);
	if (!gc)
		return;

	ValidateGC(&pCachePixmap->drawable, gc);

	scratch = pGlyphPixmap;
	if (pGlyphPixmap->drawable.depth != pCachePixmap->drawable.depth) {

		scratch = glamor_create_pixmap(screen,
					       glyph->info.width,
					       glyph->info.height,
					       pCachePixmap->
					       drawable.depth, 0);
		if (scratch) {
			PicturePtr picture;
			int error;

			picture =
			    CreatePicture(0,
					  &scratch->drawable,
					  PictureMatchFormat
					  (screen,
					   pCachePixmap->
					   drawable.depth,
					   cache->picture->format),
					  0, NULL, serverClient,
				  &error);
			if (picture) {
				ValidatePicture(picture);
				glamor_composite(PictOpSrc,
					      pGlyphPicture,
					      NULL, picture,
					      0, 0, 0, 0, 0,
					      0,
					      glyph->info.width,
					      glyph->info.height);
				FreePicture(picture, 0);
			}
		} else {
			scratch = pGlyphPixmap;
		}
	}

	box.x1 = x;
	box.y1 = y;
	box.x2 = x + glyph->info.width;
	box.y2 = y + glyph->info.height;
	glamor_copy_n_to_n_nf(&scratch->drawable,
			    &pCachePixmap->drawable, NULL,
			    &box, 1,
			    -x, -y,
			    FALSE, FALSE, 0, NULL);
	if (scratch != pGlyphPixmap)
		screen->DestroyPixmap(scratch);

	FreeScratchGC(gc);
}


void
glamor_glyph_unrealize(ScreenPtr screen, GlyphPtr glyph)
{
	struct glamor_glyph *priv;

	/* Use Lookup in case we have not attached to this glyph. */
	priv = glamor_glyph_get_private(glyph);

	if (priv->cached)
		priv->cache->glyphs[priv->pos] = NULL;
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

static void
glamor_glyph_priv_get_edge_map(GlyphPtr glyph, struct glamor_glyph *priv,
			     PicturePtr glyph_picture)
{
	PixmapPtr glyph_pixmap = (PixmapPtr) glyph_picture->pDrawable;
	struct glamor_pixmap_private *pixmap_priv;
	int j;
	unsigned long long left_x1_map, left_x2_map, right_x1_map, right_x2_map;
	int bitsPerPixel;
	int stride;
	void *bits;
	int width;
	unsigned int left_x1_data, left_x2_data, right_x1_data, right_x2_data;

	bitsPerPixel = glyph_pixmap->drawable.bitsPerPixel;
	stride = glyph_pixmap->devKind;
	bits = glyph_pixmap->devPrivate.ptr;
	width = glyph->info.width;
	pixmap_priv = glamor_get_pixmap_private(glyph_pixmap);

	if (glyph_pixmap->drawable.width < 2
	    || !(glyph_pixmap->drawable.depth == 8
		|| glyph_pixmap->drawable.depth == 1
		|| glyph_pixmap->drawable.depth == 32)) {
		priv->has_edge_map = FALSE;
		return;
	}

	left_x1_map = left_x2_map = 0;
	right_x1_map = right_x2_map = 0;

	for(j = 0; j < glyph_pixmap->drawable.height; j++)
	{
		if (bitsPerPixel == 8) {
			unsigned char *data;
			data =	(unsigned char*)((unsigned char*)bits + stride * j);
			left_x1_data = *data++;
			left_x2_data = *data;
			data =	(unsigned char*)((unsigned char*)bits + stride * j + width - 2);
			right_x1_data = *data++;
			right_x2_data = *data;
		} else if (bitsPerPixel == 32) {
			left_x1_data = *((unsigned int*)bits + stride/4 * j);
			left_x2_data = *((unsigned int*)bits + stride/4 * j + 1);
			right_x1_data = *((unsigned int*)bits + stride/4 * j + width - 2);
			right_x2_data = *((unsigned int*)bits + stride/4 * j + width - 1);
		} else if (bitsPerPixel == 1) {
			unsigned char temp;
			temp = *((unsigned char*)glyph_pixmap->devPrivate.ptr
				+ glyph_pixmap->devKind * j) & 0x3;
			left_x1_data = temp & 0x1;
			left_x2_data = temp & 0x2;

			temp = *((unsigned char*)glyph_pixmap->devPrivate.ptr
				+ glyph_pixmap->devKind * j
				+ (glyph_pixmap->drawable.width - 2)/8);
			right_x1_data = temp
				    & (1 << ((glyph_pixmap->drawable.width - 2) % 8));
			temp = *((unsigned char*)glyph_pixmap->devPrivate.ptr
				+ glyph_pixmap->devKind * j
				+ (glyph_pixmap->drawable.width - 1)/8);
			right_x2_data = temp
				   & (1 << ((glyph_pixmap->drawable.width - 1) % 8));
		}
		left_x1_map |= (left_x1_data !=0) << j;
		left_x2_map |= (left_x2_data !=0) << j;
		right_x1_map |= (right_x1_data !=0) << j;
		right_x2_map |= (right_x2_data !=0) << j;
	}

	priv->left_x1_map = left_x1_map;
	priv->left_x2_map = left_x2_map;
	priv->right_x1_map = right_x1_map;
	priv->right_x2_map = right_x2_map;
	priv->has_edge_map = TRUE;
	return;
}



/**
 * Returns TRUE if the glyphs in the lists intersect.  Only checks based on
 * bounding box, which appears to be good enough to catch most cases at least.
 */
static Bool
glamor_glyphs_intersect(int nlist, GlyphListPtr list, GlyphPtr * glyphs,
			PictFormatShort mask_format,
			ScreenPtr screen, Bool check_fake_overlap)
{
	int x1, x2, y1, y2;
	int n;
	int x, y;
	BoxRec extents;
	Bool first = TRUE;
	struct glamor_glyph *priv;

	x = 0;
	y = 0;
	extents.x1 = 0;
	extents.y1 = 0;
	extents.x2 = 0;
	extents.y2 = 0;
	while (nlist--) {
		BoxRec left_box, right_box;
		Bool has_left_edge_box = FALSE, has_right_edge_box = FALSE;
		Bool left_to_right = TRUE;
		struct glamor_glyph *left_priv, *right_priv;

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
			if (mask_format
			    && mask_format != GlyphPicture(glyph)[screen->myNum]->format)
				return TRUE;
			x1 = x - glyph->info.x;
			if (x1 < MINSHORT)
				x1 = MINSHORT;
			y1 = y - glyph->info.y;
			if (y1 < MINSHORT)
				y1 = MINSHORT;
			if (check_fake_overlap)
				priv = glamor_glyph_get_private(glyph);
			x2 = x1 + glyph->info.width;
			y2 = y1 + glyph->info.height;

			if (x2 > MAXSHORT)
				x2 = MAXSHORT;
			if (y2 > MAXSHORT)
				y2 = MAXSHORT;

			if (first) {
				extents.x1 = x1;
				extents.y1 = y1;
				extents.x2 = x2;
				extents.y2 = y2;

				if (check_fake_overlap && priv
				    && priv->has_edge_map && glyph->info.yOff == 0) {
					left_box.x1 = x1;
					left_box.x2 = x1 + 1;
					left_box.y1 = y1;

					right_box.x1 = x2 - 2;
					right_box.x2 = x2 - 1;
					right_box.y1 = y1;
					left_priv = right_priv = priv;
					has_left_edge_box = TRUE;
					has_right_edge_box = TRUE;
				}

				first = FALSE;
			} else {

				if (x1 < extents.x2 && x2 > extents.x1
				    && y1 < extents.y2
				    && y2 > extents.y1) {
					if (check_fake_overlap && priv
					    && priv->has_edge_map && glyph->info.yOff == 0) {
						int left_dx, right_dx;
						unsigned long long intersected;

						left_dx = has_left_edge_box ? 1 : 0;
						right_dx = has_right_edge_box ? 1 : 0;

						if (x1 + 1 < extents.x2 - right_dx && x2 - 1 > extents.x1 + left_dx)
							return TRUE;

						if (left_to_right && has_right_edge_box) {
							if (x1 == right_box.x1) {
								intersected = ((priv->left_x1_map & right_priv->right_x1_map)
										| (priv->left_x2_map & right_priv->right_x2_map));
								if (intersected)
									return TRUE;
							} else if (x1 == right_box.x2) {
								intersected = (priv->left_x1_map & right_priv->right_x2_map);
								if (intersected) {
								#ifdef  GLYPHS_EDEGE_OVERLAP_LOOSE_CHECK
								/* tolerate with two pixels overlap. */
									intersected &= ~(1<<__fls(intersected));
									if ((intersected & (intersected - 1)))
								#endif
										return TRUE;
								}
							}
						} else if (!left_to_right && has_left_edge_box) {
							if (x2 - 1 == left_box.x1) {
								intersected = (priv->right_x2_map & left_priv->left_x1_map);
								if (intersected) {
								#ifdef  GLYPHS_EDEGE_OVERLAP_LOOSE_CHECK
								/* tolerate with two pixels overlap. */
									intersected &= ~(1<<__fls(intersected));
									if ((intersected & (intersected - 1)))
								#endif
										return TRUE;
								}
							} else if (x2 - 1 == right_box.x2) {
								if ((priv->right_x1_map & left_priv->left_x1_map)
								   || (priv->right_x2_map & left_priv->left_x2_map))
										return TRUE;
							}
						} else {
							if (x1 < extents.x2 && x1 + 2 > extents.x1)
								return TRUE;
						}
					} else
						return TRUE;
				}
			}

			if (check_fake_overlap && priv
			    && priv->has_edge_map && glyph->info.yOff == 0) {
				if (!has_left_edge_box || x1 < extents.x1) {
					left_box.x1 = x1;
					left_box.x2 = x1 + 1;
					left_box.y1 = y1;
					has_left_edge_box = TRUE;
					left_priv = priv;
			}

			if (!has_right_edge_box || x2 > extents.x2) {
				right_box.x1 = x2 - 2;
				right_box.x2 = x2 - 1;
					right_box.y1 = y1;
					has_right_edge_box = TRUE;
					right_priv = priv;
				}
			}

			if (x1 < extents.x1)
				extents.x1 = x1;
			if (x2 > extents.x2)
				extents.x2 = x2;

			if (y1 < extents.y1)
				extents.y1 = y1;
			if (y2 > extents.y2)
				extents.y2 = y2;

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
glamor_glyph_cache(glamor_screen_private *glamor, GlyphPtr glyph, int *out_x,
		   int *out_y)
{
	ScreenPtr screen = glamor->screen;
	PicturePtr glyph_picture = GlyphPicture(glyph)[screen->myNum];
	glamor_glyph_cache_t *cache =
	    &glamor->glyphCaches[PICT_FORMAT_RGB(glyph_picture->format) !=
				 0];
	struct glamor_glyph *priv = NULL, *evicted_priv = NULL;
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

	priv = glamor_glyph_get_private(glyph);
	if (pos < GLYPH_CACHE_SIZE) {
		cache->count = pos + s;
	} else {
		for (s = size; s <= GLYPH_MAX_SIZE; s *= 2) {
			int i =
			    cache->evict & glamor_glyph_size_to_mask(s);
			GlyphPtr evicted = cache->glyphs[i];
			if (evicted == NULL)
				continue;

			evicted_priv = glamor_glyph_get_private(evicted);
			assert(evicted_priv->pos == i);
			if (evicted_priv->size >= s) {
				cache->glyphs[i] = NULL;
				evicted_priv->cached = FALSE;
				pos = cache->evict &
				    glamor_glyph_size_to_mask(size);
			} else
				evicted_priv = NULL;
			break;
		}
		if (evicted_priv == NULL) {
			int count = glamor_glyph_size_to_count(size);
			mask = glamor_glyph_count_to_mask(count);
			pos = cache->evict & mask;
			for (s = 0; s < count; s++) {
				GlyphPtr evicted = cache->glyphs[pos + s];
				if (evicted != NULL) {

					evicted_priv =
					    glamor_glyph_get_private
					    (evicted);

					assert(evicted_priv->pos == pos + s);
					evicted_priv->cached = FALSE;
					cache->glyphs[pos + s] = NULL;
				}
			}

		}
		/* And pick a new eviction position */
		cache->evict = rand() % GLYPH_CACHE_SIZE;
	}

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
#ifndef GLYPHS_NO_EDEGEMAP_OVERLAP_CHECK
	if (priv->has_edge_map == FALSE && glyph->info.width >= 2)
		glamor_glyph_priv_get_edge_map(glyph, priv, glyph_picture);
#endif
	priv->cached = TRUE;

	*out_x = priv->x;
	*out_y = priv->y;
	return cache->picture;
}
typedef void (*glyphs_flush)(void * arg);

static glamor_glyph_cache_result_t
glamor_buffer_glyph(glamor_screen_private *glamor_priv,
		    glamor_glyph_buffer_t * buffer,
		    GlyphPtr glyph, int x_glyph, int y_glyph,
		    int dx, int dy, int w, int h,
		    glyphs_flush glyphs_flush, void *flush_arg)
{
	ScreenPtr screen = glamor_priv->screen;
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
	    &glamor_priv->glyphCaches[PICT_FORMAT_RGB
					(glyph_picture->format) != 0];

	if (buffer->source
	    && buffer->source != cache->picture
	    && glyphs_flush) {
		(*glyphs_flush)(flush_arg);
		glyphs_flush = NULL;
	}

	if (buffer->count == GLYPH_BUFFER_SIZE
	    && glyphs_flush) {
		(*glyphs_flush)(flush_arg);
		glyphs_flush = NULL;
	}

	priv = glamor_glyph_get_private(glyph);

	if (priv && priv->cached) {
		rect = &buffer->rects[buffer->count++];
		rect->x_src = priv->x + dx;
		rect->y_src = priv->y + dy;
		if (buffer->source == NULL)
			buffer->source = priv->cache->picture;
		assert(priv->cache->glyphs[priv->pos] == glyph);
	} else {
		if (glyphs_flush)
			(*glyphs_flush)(flush_arg);
		source = glamor_glyph_cache(glamor_priv, glyph, &x, &y);
		if (source != NULL) {
			rect = &buffer->rects[buffer->count++];
			rect->x_src = x + dx;
			rect->y_src = y + dy;
			if (buffer->source == NULL)
				buffer->source = source;
		} else {
	/* Couldn't find the glyph in the cache, use the glyph picture directly */
			source = GlyphPicture(glyph)[screen->myNum];
			if (buffer->source
			    && buffer->source != source
			    && glyphs_flush)
				(*glyphs_flush)(flush_arg);
			buffer->source = source;

			rect = &buffer->rects[buffer->count++];
			rect->x_src = 0 + dx;
			rect->y_src = 0 + dy;
		}
		priv = glamor_glyph_get_private(glyph);
	}

	rect->x_dst = x_glyph - glyph->info.x;
	rect->y_dst = y_glyph - glyph->info.y;
	rect->width = w;
	rect->height = h;
	return GLAMOR_GLYPH_SUCCESS;
}

struct glyphs_flush_mask_arg {
	PicturePtr mask;
	glamor_glyph_buffer_t *buffer;
};

static void
glamor_glyphs_flush_mask(struct glyphs_flush_mask_arg *arg)
{
#ifdef RENDER
	glamor_composite_glyph_rects(PictOpAdd, arg->buffer->source,
				     NULL, arg->mask,
				     arg->buffer->count,
				     arg->buffer->rects);
#endif
	arg->buffer->count = 0;
	arg->buffer->source = NULL;
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
	XID component_alpha;
	glamor_glyph_buffer_t buffer;
	xRectangle fill_rect;
	struct glyphs_flush_mask_arg arg;
	GCPtr gc;
	glamor_screen_private *glamor_priv;

	glamor_glyph_extents(nlist, list, glyphs, &extents);

	if (extents.x2 <= extents.x1 || extents.y2 <= extents.y1)
		return;
	glamor_priv = glamor_get_screen_private(screen);
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
			    && glyph->info.height > 0) {
				glyphs_flush flush_func;
				if (buffer.count) {
					arg.mask = mask;
					arg.buffer = &buffer;
					flush_func = (glyphs_flush)glamor_glyphs_flush_mask;
				}
				else
					flush_func = NULL;

				glamor_buffer_glyph(glamor_priv, &buffer,
						    glyph, x, y,
						    0, 0,
						    glyph->info.width, glyph->info.height,
						    flush_func,
						    (void*)&arg);
			}

			x += glyph->info.xOff;
			y += glyph->info.yOff;
		}
		list++;
	}

	if (buffer.count) {
		arg.mask = mask;
		arg.buffer = &buffer;
		glamor_glyphs_flush_mask(&arg);
	}

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

struct glyphs_flush_dst_arg {
	CARD8 op;
	PicturePtr src;
	PicturePtr dst;
	glamor_glyph_buffer_t * buffer;
	INT16 x_src;
	INT16 y_src;
	INT16 x_dst;
	INT16 y_dst;
};

static void
glamor_glyphs_flush_dst(struct glyphs_flush_dst_arg * arg)
{
	int i;
	glamor_composite_rect_t *rect = &arg->buffer->rects[0];
	for (i = 0; i < arg->buffer->count; i++, rect++) {
		rect->x_mask = rect->x_src;
		rect->y_mask = rect->y_src;
		rect->x_src = arg->x_src + rect->x_dst - arg->x_dst;
		rect->y_src = arg->y_src + rect->y_dst - arg->y_dst;
	}

	glamor_composite_glyph_rects(arg->op, arg->src,
				arg->buffer->source, arg->dst,
				arg->buffer->count,
				&arg->buffer->rects[0]);

	arg->buffer->count = 0;
	arg->buffer->source = NULL;
}

static void
glamor_glyphs_to_dst(CARD8 op,
		     PicturePtr src,
		     PicturePtr dst,
		     INT16 x_src,
		     INT16 y_src,
		     int nlist, GlyphListPtr list,
		     GlyphPtr * glyphs)
{
	ScreenPtr screen = dst->pDrawable->pScreen;
	int x = 0, y = 0;
	int x_dst = list->xOff, y_dst = list->yOff;
	int n;
	GlyphPtr glyph;
	glamor_glyph_buffer_t buffer;
	struct glyphs_flush_dst_arg arg;
	BoxPtr rects;
	int nrect;
	glamor_screen_private *glamor_priv;

	buffer.count = 0;
	buffer.source = NULL;

	rects = REGION_RECTS(dst->pCompositeClip);
	nrect = REGION_NUM_RECTS(dst->pCompositeClip);

	glamor_priv = glamor_get_screen_private(screen);

	arg.op = op;
	arg.src = src;
	arg.dst = dst;
	arg.buffer = &buffer;
	arg.x_src = x_src;
	arg.y_src = y_src;
	arg.x_dst = x_dst;
	arg.y_dst = y_dst;


	while (nlist--) {
		x += list->xOff;
		y += list->yOff;
		n = list->len;
		while (n--) {
			int i;
			glyph = *glyphs++;

			if (glyph->info.width > 0
			    && glyph->info.height > 0) {
				glyphs_flush flush_func;

				if (buffer.count)
					flush_func = (glyphs_flush)glamor_glyphs_flush_dst;
				else
					flush_func = NULL;

				for (i = 0; i < nrect; i++) {
					int dst_x, dst_y;
					int dx, dy;
					int x2, y2;

					dst_x = x - glyph->info.x;
					dst_y = y - glyph->info.y;
					x2 = dst_x + glyph->info.width;
					y2 = dst_y + glyph->info.height;
					dx = dy = 0;
					if (rects[i].y1 >= y2)
						break;

					if (dst_x < rects[i].x1)
						dx = rects[i].x1 - dst_x, dst_x = rects[i].x1;
					if (x2 > rects[i].x2)
						x2 = rects[i].x2;
					if (dst_y < rects[i].y1)
						dy = rects[i].y1 - dst_y, dst_y = rects[i].y1;
					if (y2 > rects[i].y2)
						y2 = rects[i].y2;

					if (dst_x < x2 && dst_y < y2) {

						glamor_buffer_glyph(glamor_priv,
								    &buffer,
								    glyph,
								    dst_x + glyph->info.x,
								    dst_y + glyph->info.y,
								    dx, dy,
								    x2 - dst_x, y2 - dst_y,
								    flush_func,
								    (void*)&arg);
					}
				}
			}

			x += glyph->info.xOff;
			y += glyph->info.yOff;
		}
		list++;
	}

	if (buffer.count)
		glamor_glyphs_flush_dst(&arg);
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
	Bool intersected = FALSE;
	PictFormatShort format;
#ifndef GLYPHS_NO_EDEGEMAP_OVERLAP_CHECK
	Bool check_fake_overlap = TRUE;
	if (!(op == PictOpOver
	    || op == PictOpAdd
	    || op == PictOpXor)) {
	/* C = (0,0,0,0) D = glyphs , SRC = A, DEST = B (faked overlapped glyphs, overlapped with (0,0,0,0)).
	 * For those op, (A IN (C ADD D)) OP B !=  (A IN D) OP ((A IN C) OP B)
	 *		or (A IN (D ADD C)) OP B != (A IN C) OP ((A IN D) OP B)
	 * We need to split the faked regions to three or two, and composite the disoverlapped small
	 * boxes one by one. For other Ops, it's safe to composite the whole box.  */
		check_fake_overlap = FALSE;
	}
#else
	Bool check_fake_overlap = FALSE;
#endif

	if (!mask_format || (((nlist == 1 && list->len == 1) || op == PictOpAdd)
	    && (dst->format == ((mask_format->depth << 24) | mask_format->format)))) {
		glamor_glyphs_to_dst(op, src, dst, x_src, y_src, nlist,
				     list, glyphs);
		return TRUE;
	}

	if (mask_format)
		format = mask_format->depth << 24 | mask_format->format;
	else
		format = 0;

	intersected = glamor_glyphs_intersect(nlist, list, glyphs,
				format, dst->pDrawable->pScreen,
				check_fake_overlap);

	if (!intersected) {
		glamor_glyphs_to_dst(op, src, dst, x_src, y_src, nlist,
				     list, glyphs);
		return TRUE;
	}

	if (mask_format)
		glamor_glyphs_via_mask(op, src, dst, mask_format,
				       x_src, y_src, nlist, list, glyphs);
	else {
		/* No mask_format and has intersect and glyphs have different format.
		 * XXX do we need to implement a new glyphs rendering function for
		 * this case?*/
		glamor_glyphs_to_dst(op, src, dst, x_src, y_src, nlist, list, glyphs);
	}

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

