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
#define CACHE_PICTURE_WIDTH 1024

/* Maximum number of glyphs we buffer on the stack before flushing
 * rendering to the mask or destination surface.
 */
#define GLYPH_BUFFER_SIZE 256

typedef struct {
    PicturePtr source;
    glamor_composite_rect_t rects[GLYPH_BUFFER_SIZE];
    int count;
} glamor_glyph_buffer_t;

typedef enum {
    GLAMOR_GLYPH_SUCCESS,	/* Glyph added to render buffer */
    GLAMOR_GLYPH_FAIL,		/* out of memory, etc */
    GLAMOR_GLYPH_NEED_FLUSH,	/* would evict a glyph already in the buffer */
} glamor_glyph_cache_result_t;

void glamor_glyphs_init(ScreenPtr screen)
{
    glamor_screen_private *glamor_screen = glamor_get_screen_private(screen);
    int i = 0;

    memset(glamor_screen->glyph_caches, 0, sizeof(glamor_screen->glyph_caches));

    glamor_screen->glyph_caches[i].format = PICT_a8;
    glamor_screen->glyph_caches[i].glyph_width =
	glamor_screen->glyph_caches[i].glyph_height = 16;
    i++;
    glamor_screen->glyph_caches[i].format = PICT_a8;
    glamor_screen->glyph_caches[i].glyph_width =
	glamor_screen->glyph_caches[i].glyph_height = 32;
    i++;
    glamor_screen->glyph_caches[i].format = PICT_a8r8g8b8;
    glamor_screen->glyph_caches[i].glyph_width =
	glamor_screen->glyph_caches[i].glyph_height = 16;
    i++;
    glamor_screen->glyph_caches[i].format = PICT_a8r8g8b8;
    glamor_screen->glyph_caches[i].glyph_width =
	glamor_screen->glyph_caches[i].glyph_height = 32;
    i++;

    assert(i == GLAMOR_NUM_GLYPH_CACHES);

    for (i = 0; i < GLAMOR_NUM_GLYPH_CACHES; i++) {
	glamor_screen->glyph_caches[i].columns =
	    CACHE_PICTURE_WIDTH / glamor_screen->glyph_caches[i].glyph_width;
	glamor_screen->glyph_caches[i].size = 256;
	glamor_screen->glyph_caches[i].hash_size = 557;
    }
}

static void glamor_unrealize_glyph_caches(ScreenPtr screen, unsigned int format)
{
    glamor_screen_private *glamor_screen = glamor_get_screen_private(screen);
    int i;

    for (i = 0; i < GLAMOR_NUM_GLYPH_CACHES; i++) {
	glamor_glyph_cache_t *cache = &glamor_screen->glyph_caches[i];

	if (cache->format != format)
	    continue;

	if (cache->picture) {
	    FreePicture((pointer) cache->picture, (XID) 0);
	    cache->picture = NULL;
	}

	if (cache->hash_entries) {
	    xfree(cache->hash_entries);
	    cache->hash_entries = NULL;
	}

	if (cache->glyphs) {
	    xfree(cache->glyphs);
	    cache->glyphs = NULL;
	}
	cache->glyph_count = 0;
    }
}

#define NeedsComponent(f) (PICT_FORMAT_A(f) != 0 && PICT_FORMAT_RGB(f) != 0)

/* All caches for a single format share a single pixmap for glyph storage,
 * allowing mixing glyphs of different sizes without paying a penalty
 * for switching between source pixmaps. (Note that for a size of font
 * right at the border between two sizes, we might be switching for almost
 * every glyph.)
 *
 * This function allocates the storage pixmap, and then fills in the
 * rest of the allocated structures for all caches with the given format.
 */
static Bool glamor_realize_glyph_caches(ScreenPtr screen, unsigned int format)
{
    glamor_screen_private *glamor_screen = glamor_get_screen_private(screen);
    int depth = PIXMAN_FORMAT_DEPTH(format);
    PictFormatPtr pict_format;
    PixmapPtr pixmap;
    PicturePtr picture;
    CARD32 component_alpha;
    int height;
    int i;
    int error;

    pict_format = PictureMatchFormat(screen, depth, format);
    if (!pict_format)
	return FALSE;

    /* Compute the total vertical size needed for the format */

    height = 0;
    for (i = 0; i < GLAMOR_NUM_GLYPH_CACHES; i++) {
	glamor_glyph_cache_t *cache = &glamor_screen->glyph_caches[i];
	int rows;

	if (cache->format != format)
	    continue;

	cache->y_offset = height;

	rows = (cache->size + cache->columns - 1) / cache->columns;
	height += rows * cache->glyph_height;
    }

    /* Now allocate the pixmap and picture */

    pixmap = screen->CreatePixmap(screen,
				  CACHE_PICTURE_WIDTH,
				  height, depth,
				  0);
    if (!pixmap)
	return FALSE;

    component_alpha = NeedsComponent(pict_format->format);
    picture = CreatePicture(0, &pixmap->drawable, pict_format,
			    CPComponentAlpha, &component_alpha,
			    serverClient, &error);

    screen->DestroyPixmap(pixmap);	/* picture holds a refcount */

    if (!picture)
	return FALSE;

    /* And store the picture in all the caches for the format */

    for (i = 0; i < GLAMOR_NUM_GLYPH_CACHES; i++) {
	glamor_glyph_cache_t *cache = &glamor_screen->glyph_caches[i];
	int j;

	if (cache->format != format)
	    continue;

	cache->picture = picture;
	cache->picture->refcnt++;
	cache->hash_entries = xalloc(sizeof(int) * cache->hash_size);
	cache->glyphs =
	    xalloc(sizeof(glamor_cached_glyph_t) * cache->size);
	cache->glyph_count = 0;

	if (!cache->hash_entries || !cache->glyphs)
	    goto bail;

	for (j = 0; j < cache->hash_size; j++)
	    cache->hash_entries[j] = -1;

	cache->eviction_position = rand() % cache->size;
    }

    /* Each cache references the picture individually */
    FreePicture(picture, 0);
    return TRUE;

 bail:
    glamor_unrealize_glyph_caches(screen, format);
    return FALSE;
}

void glamor_glyphs_fini(ScreenPtr screen)
{
    glamor_screen_private *glamor_screen = glamor_get_screen_private(screen);
    int i;

    for (i = 0; i < GLAMOR_NUM_GLYPH_CACHES; i++) {
	glamor_glyph_cache_t *cache = &glamor_screen->glyph_caches[i];

	if (cache->picture)
	    glamor_unrealize_glyph_caches(screen, cache->format);
    }
}

static int
glamor_glyph_cache_hash_lookup(glamor_glyph_cache_t * cache, GlyphPtr glyph)
{
    int slot;

    slot = (*(CARD32 *) glyph->sha1) % cache->hash_size;

    while (TRUE) {		/* hash table can never be full */
	int entryPos = cache->hash_entries[slot];
	if (entryPos == -1)
	    return -1;

	if (memcmp(glyph->sha1, cache->glyphs[entryPos].sha1,
		   sizeof(glyph->sha1)) == 0) {
	    return entryPos;
	}

	slot--;
	if (slot < 0)
	    slot = cache->hash_size - 1;
    }
}

static void
glamor_glyph_cache_hash_insert(glamor_glyph_cache_t * cache, GlyphPtr glyph, int pos)
{
    int slot;

    memcpy(cache->glyphs[pos].sha1, glyph->sha1, sizeof(glyph->sha1));

    slot = (*(CARD32 *) glyph->sha1) % cache->hash_size;

    while (TRUE) {		/* hash table can never be full */
	if (cache->hash_entries[slot] == -1) {
	    cache->hash_entries[slot] = pos;
	    return;
	}

	slot--;
	if (slot < 0)
	    slot = cache->hash_size - 1;
    }
}

static void glamor_glyph_cache_hash_remove(glamor_glyph_cache_t * cache, int pos)
{
    int slot;
    int emptied_slot = -1;

    slot = (*(CARD32 *) cache->glyphs[pos].sha1) % cache->hash_size;

    while (TRUE) {		/* hash table can never be full */
	int entryPos = cache->hash_entries[slot];

	if (entryPos == -1)
	    return;

	if (entryPos == pos) {
	    cache->hash_entries[slot] = -1;
	    emptied_slot = slot;
	} else if (emptied_slot != -1) {
	    /* See if we can move this entry into the emptied slot,
	     * we can't do that if if entry would have hashed
	     * between the current position and the emptied slot.
	     * (taking wrapping into account). Bad positions
	     * are:
	     *
	     * |   XXXXXXXXXX             |
	     *     i         j
	     *
	     * |XXX                   XXXX|
	     *     j                  i
	     *
	     * i - slot, j - emptied_slot
	     *
	     * (Knuth 6.4R)
	     */

	    int entry_slot = (*(CARD32 *) cache->glyphs[entryPos].sha1) %
		cache->hash_size;

	    if (!((entry_slot >= slot && entry_slot < emptied_slot) ||
		  (emptied_slot < slot
		   && (entry_slot < emptied_slot
		       || entry_slot >= slot)))) {
		cache->hash_entries[emptied_slot] = entryPos;
		cache->hash_entries[slot] = -1;
		emptied_slot = slot;
	    }
	}

	slot--;
	if (slot < 0)
	    slot = cache->hash_size - 1;
    }
}

#define CACHE_X(pos) (((pos) % cache->columns) * cache->glyph_width)
#define CACHE_Y(pos) (cache->y_offset + ((pos) / cache->columns) * cache->glyph_height)

/* The most efficient thing to way to upload the glyph to the screen
 * is to use CopyArea; glamor pixmaps are always offscreen.
 */
static Bool
glamor_glyph_cache_upload_glyph(ScreenPtr screen,
				glamor_glyph_cache_t * cache,
				int pos, GlyphPtr glyph)
{
    PicturePtr glyph_picture = GlyphPicture(glyph)[screen->myNum];
    PixmapPtr glyph_pixmap = (PixmapPtr) glyph_picture->pDrawable;
    PixmapPtr cache_pixmap = (PixmapPtr) cache->picture->pDrawable;
    PixmapPtr scratch;
    GCPtr gc;

    /* UploadToScreen only works if bpp match */
    if (glyph_pixmap->drawable.bitsPerPixel !=
	cache_pixmap->drawable.bitsPerPixel)
	return FALSE;

    gc = GetScratchGC(cache_pixmap->drawable.depth, screen);
    ValidateGC(&cache_pixmap->drawable, gc);

    /* Create a temporary bo to stream the updates to the cache */
    scratch = screen->CreatePixmap(screen,
				   glyph->info.width,
				   glyph->info.height,
				   glyph_pixmap->drawable.depth,
				   0);
    if (scratch) {
	(void)glamor_copy_area(&glyph_pixmap->drawable,
			       &scratch->drawable,
			       gc,
			       0, 0,
			       glyph->info.width, glyph->info.height,
			       0, 0);
    } else {
	scratch = glyph_pixmap;
    }

    (void)glamor_copy_area(&scratch->drawable,
			   &cache_pixmap->drawable,
			   gc,
			   0, 0, glyph->info.width, glyph->info.height,
			   CACHE_X(pos), CACHE_Y(pos));

    if (scratch != glyph_pixmap)
	screen->DestroyPixmap(scratch);

    FreeScratchGC(gc);

    return TRUE;
}

static glamor_glyph_cache_result_t
glamor_glyph_cache_buffer_glyph(ScreenPtr screen,
				glamor_glyph_cache_t * cache,
				glamor_glyph_buffer_t * buffer,
				GlyphPtr glyph, int x_glyph, int y_glyph)
{
    glamor_composite_rect_t *rect;
    int pos;

    if (buffer->source && buffer->source != cache->picture)
	return GLAMOR_GLYPH_NEED_FLUSH;

    if (!cache->picture) {
	if (!glamor_realize_glyph_caches(screen, cache->format))
	    return GLAMOR_GLYPH_FAIL;
    }

    DBG_GLYPH_CACHE(("(%d,%d,%s): buffering glyph %lx\n",
		     cache->glyph_width, cache->glyph_height,
		     cache->format == PICT_a8 ? "A" : "ARGB",
		     (long)*(CARD32 *) glyph->sha1));

    pos = glamor_glyph_cache_hash_lookup(cache, glyph);
    if (pos != -1) {
	DBG_GLYPH_CACHE(("  found existing glyph at %d\n", pos));
    } else {
	if (cache->glyph_count < cache->size) {
	    /* Space remaining; we fill from the start */
	    pos = cache->glyph_count;
	    cache->glyph_count++;
	    DBG_GLYPH_CACHE(("  storing glyph in free space at %d\n", pos));

	    glamor_glyph_cache_hash_insert(cache, glyph, pos);

	} else {
	    /* Need to evict an entry. We have to see if any glyphs
	     * already in the output buffer were at this position in
	     * the cache
	     */

	    pos = cache->eviction_position;
	    DBG_GLYPH_CACHE(("  evicting glyph at %d\n", pos));
	    if (buffer->count) {
		int x, y;
		int i;

		x = CACHE_X(pos);
		y = CACHE_Y(pos);

		for (i = 0; i < buffer->count; i++) {
		    if (buffer->rects[i].x_src == x
			&& buffer->rects[i].y_src == y) {
			DBG_GLYPH_CACHE(("  must flush buffer\n"));
			return GLAMOR_GLYPH_NEED_FLUSH;
		    }
		}
	    }

	    /* OK, we're all set, swap in the new glyph */
	    glamor_glyph_cache_hash_remove(cache, pos);
	    glamor_glyph_cache_hash_insert(cache, glyph, pos);

	    /* And pick a new eviction position */
	    cache->eviction_position = rand() % cache->size;
	}

	/* Now actually upload the glyph into the cache picture; if
	 * we can't do it with UploadToScreen (because the glyph is
	 * offscreen, etc), we fall back to CompositePicture.
	 */
	if (!glamor_glyph_cache_upload_glyph(screen, cache, pos, glyph)) {
	    CompositePicture(PictOpSrc,
			     GlyphPicture(glyph)[screen->myNum],
			     None,
			     cache->picture,
			     0, 0,
			     0, 0,
			     CACHE_X(pos),
			     CACHE_Y(pos),
			     glyph->info.width,
			     glyph->info.height);
	}

    }

    buffer->source = cache->picture;

    rect = &buffer->rects[buffer->count];
    rect->x_src = CACHE_X(pos);
    rect->y_src = CACHE_Y(pos);
    rect->x_dst = x_glyph - glyph->info.x;
    rect->y_dst = y_glyph - glyph->info.y;
    rect->width = glyph->info.width;
    rect->height = glyph->info.height;

    buffer->count++;

    return GLAMOR_GLYPH_SUCCESS;
}

#undef CACHE_X
#undef CACHE_Y

static glamor_glyph_cache_result_t
glamor_buffer_glyph(ScreenPtr screen,
		    glamor_glyph_buffer_t *buffer,
		    GlyphPtr glyph, int x_glyph, int y_glyph)
{
    glamor_screen_private *glamor_screen = glamor_get_screen_private(screen);
    unsigned int format = (GlyphPicture(glyph)[screen->myNum])->format;
    int width = glyph->info.width;
    int height = glyph->info.height;
    glamor_composite_rect_t *rect;
    PicturePtr source;
    int i;

    if (buffer->count == GLYPH_BUFFER_SIZE)
	return GLAMOR_GLYPH_NEED_FLUSH;

    if (PICT_FORMAT_BPP(format) == 1)
	format = PICT_a8;

    for (i = 0; i < GLAMOR_NUM_GLYPH_CACHES; i++) {
	glamor_glyph_cache_t *cache = &glamor_screen->glyph_caches[i];

	if (format == cache->format &&
	    width <= cache->glyph_width &&
	    height <= cache->glyph_height) {
	    glamor_glyph_cache_result_t result =
		glamor_glyph_cache_buffer_glyph(screen,
						&glamor_screen->glyph_caches[i],
						buffer,
						glyph, x_glyph,
						y_glyph);
	    switch (result) {
	    case GLAMOR_GLYPH_FAIL:
		break;
	    case GLAMOR_GLYPH_SUCCESS:
	    case GLAMOR_GLYPH_NEED_FLUSH:
		return result;
	    }
	}
    }

    /* Couldn't find the glyph in the cache, use the glyph picture directly */

    source = GlyphPicture(glyph)[screen->myNum];
    if (buffer->source && buffer->source != source)
	return GLAMOR_GLYPH_NEED_FLUSH;

    buffer->source = source;

    rect = &buffer->rects[buffer->count];
    rect->x_src = 0;
    rect->y_src = 0;
    rect->x_dst = x_glyph - glyph->info.x;
    rect->y_dst = y_glyph - glyph->info.y;
    rect->width = glyph->info.width;
    rect->height = glyph->info.height;

    buffer->count++;

    return GLAMOR_GLYPH_SUCCESS;
}

static void glamor_glyphs_to_mask(PicturePtr mask,
				  glamor_glyph_buffer_t *buffer)
{
    glamor_composite_rects(PictOpAdd, buffer->source, mask,
			   buffer->count, buffer->rects);

    buffer->count = 0;
    buffer->source = NULL;
}

static void
glamor_glyphs_to_dst(CARD8 op,
		     PicturePtr src,
		     PicturePtr dst,
		     glamor_glyph_buffer_t *buffer,
		     INT16 x_src, INT16 y_src, INT16 x_dst, INT16 y_dst)
{
    int i;

    for (i = 0; i < buffer->count; i++) {
	glamor_composite_rect_t *rect = &buffer->rects[i];

	CompositePicture(op,
			 src,
			 buffer->source,
			 dst,
			 x_src + rect->x_dst - x_dst,
			 y_src + rect->y_dst - y_dst,
			 rect->x_src,
			 rect->y_src,
			 rect->x_dst,
			 rect->y_dst, rect->width, rect->height);
    }

    buffer->count = 0;
    buffer->source = NULL;
}

/* Cut and paste from render/glyph.c - probably should export it instead */
static void
glamor_glyph_extents(int nlist,
		     GlyphListPtr list, GlyphPtr *glyphs, BoxPtr extents)
{
    int x1, x2, y1, y2;
    int n;
    GlyphPtr glyph;
    int x, y;

    x = 0;
    y = 0;
    extents->x1 = MAXSHORT;
    extents->x2 = MINSHORT;
    extents->y1 = MAXSHORT;
    extents->y2 = MINSHORT;
    while (nlist--) {
	x += list->xOff;
	y += list->yOff;
	n = list->len;
	list++;
	while (n--) {
	    glyph = *glyphs++;
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
	    if (x1 < extents->x1)
		extents->x1 = x1;
	    if (x2 > extents->x2)
		extents->x2 = x2;
	    if (y1 < extents->y1)
		extents->y1 = y1;
	    if (y2 > extents->y2)
		extents->y2 = y2;
	    x += glyph->info.xOff;
	    y += glyph->info.yOff;
	}
    }
}

/**
 * Returns TRUE if the glyphs in the lists intersect.  Only checks based on
 * bounding box, which appears to be good enough to catch most cases at least.
 */
static Bool
glamor_glyphs_intersect(int nlist, GlyphListPtr list, GlyphPtr *glyphs)
{
    int x1, x2, y1, y2;
    int n;
    GlyphPtr glyph;
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
	    glyph = *glyphs++;

	    if (glyph->info.width == 0 || glyph->info.height == 0) {
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
		if (x1 < extents.x2 && x2 > extents.x1 &&
		    y1 < extents.y2 && y2 > extents.y1) {
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

void
glamor_glyphs(CARD8 op,
	      PicturePtr src,
	      PicturePtr dst,
	      PictFormatPtr mask_format,
	      INT16 x_src,
	      INT16 y_src, int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
    PicturePtr picture;
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

    if (mask_format) {
	GCPtr gc;
	xRectangle rect;

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
	rect.x = 0;
	rect.y = 0;
	rect.width = width;
	rect.height = height;
	gc->ops->PolyFillRect(&mask_pixmap->drawable, gc, 1, &rect);
	FreeScratchGC(gc);
	x = -extents.x1;
	y = -extents.y1;
    } else {
	mask = dst;
	x = 0;
	y = 0;
    }
    buffer.count = 0;
    buffer.source = NULL;
    while (nlist--) {
	x += list->xOff;
	y += list->yOff;
	n = list->len;
	while (n--) {
	    glyph = *glyphs++;
	    picture = GlyphPicture(glyph)[screen->myNum];

	    if (glyph->info.width > 0 && glyph->info.height > 0 &&
		glamor_buffer_glyph(screen, &buffer, glyph, x,
				    y) == GLAMOR_GLYPH_NEED_FLUSH) {
		if (mask_format)
		    glamor_glyphs_to_mask(mask, &buffer);
		else
		    glamor_glyphs_to_dst(op, src, dst,
					 &buffer, x_src, y_src,
					 x_dst, y_dst);

		glamor_buffer_glyph(screen, &buffer, glyph, x, y);
	    }

	    x += glyph->info.xOff;
	    y += glyph->info.yOff;
	}
	list++;
    }

    if (buffer.count) {
	if (mask_format)
	    glamor_glyphs_to_mask(mask, &buffer);
	else
	    glamor_glyphs_to_dst(op, src, dst, &buffer,
				 x_src, y_src, x_dst, y_dst);
    }

    if (mask_format) {
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
}
